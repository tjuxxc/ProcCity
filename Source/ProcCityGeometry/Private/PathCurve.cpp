#include "ProcCityGeometry/PathCurve.h"
#include "ProcCityGeometry/PolygonOffset.h"

namespace
{
	/** LUT resolution: number of samples between each pair of control 
	 *  points. 32 is enough at city scale */
	constexpr int32 ArcSamplesPerSegment = 32;

	FORCEINLINE FVector2D Rot90CCW(const FVector2D& V)
	{
		return FVector2D(-V.Y, V.X);
	}

	/** Catmull-Rom tangent (uniform parameterization) */
	FORCEINLINE FVector2D CatmullTangent(
		const FVector2D& Prev, const FVector2D& Next)
	{
		return (Next - Prev) * 0.5;
	}
	
	/** Cubic Hermite */
	FORCEINLINE FVector2D Hermite(const FVector2D& P0, const FVector2D& T0, 
		const FVector2D& P1, const FVector2D& T1, double T)
	{
		const double T2 = T * T;
		const double T3 = T2 * T;
		
		return P0 * (2*T3 - 3*T2 + 1) + T0 * (T3 - 2*T2 + T)
			 + P1 * (-2*T3 + 3*T2)    + T1 * (T3 - T2);
	}
	
	FORCEINLINE FVector2D HermiteDeriv(
		const FVector2D& P0, const FVector2D& T0, 
		const FVector2D& P1, const FVector2D& T1, double T)
	{
		const double T2 = T * T;
		return P0 * (6*T2 - 6*T) + T0 * (3*T2 - 4*T + 1)
			 + P1 * (-6*T2 + 6*T) + T1 * (3*T2 - 2*T);
	}
}

// ------------------ FWidthProfile ------------------

void FWidthProfile::Evaluate(
	double Alpha, double& OutLeft, double& OutRight) const
{
	if (Keys.Num() == 0)
	{
		OutLeft = ConstantLeftWidth;
		OutRight = ConstantRightWidth;
		return;
	}
	if (Keys.Num() == 1 || Alpha < Keys[0].X)
	{
		OutLeft = Keys[0].Y;
		OutRight = Keys[0].Z;
		return;
	}
	if (Alpha > Keys.Last().X)
	{
		OutLeft = Keys.Last().Y; 
		OutRight = Keys.Last().Z; 
		return;
	}
	for (int32 i = 0; i + 1 < Keys.Num(); ++i)
	{
		if (Alpha >= Keys[i].X && Alpha <= Keys[i + 1].X)
		{
			const double Span = Keys[i+1].X - Keys[i].X;
			const double T = (Span > UE_KINDA_SMALL_NUMBER) 
				? (Alpha - Keys[i].X) / Span : 0.0;
			OutLeft  = FMath::Lerp(Keys[i].Y, Keys[i + 1].Y, T);
			OutRight = FMath::Lerp(Keys[i].Z, Keys[i + 1].Z, T);
			return;
		}
	}
	OutLeft = ConstantLeftWidth; 
	OutRight = ConstantRightWidth;
}

double FWidthProfile::MaxHalfWidth() const
{
	double M = FMath::Max(ConstantLeftWidth, ConstantRightWidth);
	for (const FVector& K : Keys)
	{
		M = FMath::Max3(M, K.Y, K.Z);
	}
	return M;
}

// ------------------ FPathCurve ------------------

FPathCurve FPathCurve::MakeLine(const FVector2D& A, 
	const FVector2D& B, double ZA, double ZB)
{
	FPathCurve C;
	C.Interp = EPathInterp::Linear;
	
	FPathControlPoint P0; 
	P0.Position = A; P0.Elevation = ZA;
	
	FPathControlPoint P1; 
	P1.Position = B; P1.Elevation = ZB;
	
	C.ControlPoints = {P0, P1};
	return C;
}

FPathCurve FPathCurve::MakePolyline(
	TArrayView<const FVector2D> InPoints, bool bInClosed)
{
	FPathCurve C;
	C.Interp = EPathInterp::Linear;
	C.bClosed = bInClosed;
	C.ControlPoints.Reserve(InPoints.Num());
	for (const FVector2D& P : InPoints)
	{
		FPathControlPoint CP;
		CP.Position = P; 
		C.ControlPoints.Add(CP);
	}
	return C;
}

FPathCurve FPathCurve::MakeSpline(
	TArrayView<const FVector2D> InPoints, bool bInClosed)
{
	FPathCurve C = MakePolyline(InPoints, bInClosed);
	C.Interp = EPathInterp::CatmullRom;
	return C;
}

FPathCurve FPathCurve::FromPolygonOuter(const FPolygon2D& Poly)
{
	return MakePolyline(Poly.Outer.Vertices, true);
}

int32 FPathCurve::NumSegments() const
{
	if (ControlPoints.Num() < 2) { return 0; }
	return bClosed? ControlPoints.Num() : ControlPoints.Num() - 1;
}

void FPathCurve::GetSegment(double Alpha, int32& OutSeg, double& OutLocalT) const
{
	const int32 N = NumSegments();
	if (N <= 0)
	{
		OutSeg = 0; OutLocalT = 0.0; 
		return;
	}
    
	// Alpha = SegIndex / SegCount
	const double Scaled = FMath::Clamp(Alpha, 0.0, 1.0) * N;
	OutSeg = FMath::Clamp(
		static_cast<int32>(FMath::FloorToDouble(Scaled)), 
		0, N - 1);
	OutLocalT = Scaled - OutSeg;
}

FVector2D FPathCurve::EvalPosition(double Alpha) const
{
	if (ControlPoints.Num() == 0) { return FVector2D::ZeroVector; }
	if (ControlPoints.Num() == 1) { return ControlPoints[0].Position; }
	
	int32 Seg; double T;
	GetSegment(Alpha, Seg, T);
	const int32 N = ControlPoints.Num();
	const int32 I0 = Seg;
	const int32 I1 = (Seg + 1) % N;
	
	const FVector2D P0 = ControlPoints[I0].Position;
	const FVector2D P1 = ControlPoints[I1].Position;
	
	switch (Interp)
	{
	case EPathInterp::Linear:
		return FMath::Lerp(P0, P1, T);
		
	case EPathInterp::CatmullRom:
		{
			const int32 IPrev = bClosed 
				? (I0 + N - 1) % N : FMath::Max(I0 - 1, 0);
			const int32 INext = bClosed 
				? (I1 + 1) % N : FMath::Min(I1 + 1, N - 1);
			const FVector2D T0 = 
				CatmullTangent(ControlPoints[IPrev].Position, P1);
			const FVector2D T1 = 
				CatmullTangent(P0, ControlPoints[INext].Position);
			return Hermite(P0, T0, P1, T1, T);
		}
		
	case EPathInterp::Hermite:
		return Hermite(P0, ControlPoints[I0].LeaveTangent, 
		               P1, ControlPoints[I1].ArriveTangent, T);
	}
	return FMath::Lerp(P0, P1, T);
}

FVector2D FPathCurve::EvalTangent(double Alpha) const
{
	if (ControlPoints.Num() < 2)
	{
		return FVector2D(1, 0);
	}
	
	int32 Seg; double T;
	GetSegment(Alpha, Seg, T);
	const int32 N = ControlPoints.Num();
	const int32 I0 = Seg;
	const int32 I1 = (Seg + 1) % N;
	const FVector2D& P0 = ControlPoints[I0].Position;
	const FVector2D& P1 = ControlPoints[I1].Position;
	
	FVector2D Tan;
	switch (Interp)
	{
	case EPathInterp::Linear:
		Tan = P1 - P0;
		break;
		
	case EPathInterp::CatmullRom:
		{
			const int32 IPrev = bClosed 
				? (I0 + N - 1) % N : FMath::Max(I0 - 1, 0);
			const int32 INext = bClosed 
				? (I1 + 1) % N : FMath::Min(I1 + 1, N - 1);
			const FVector2D T0 = 
				CatmullTangent(ControlPoints[IPrev].Position, P1);
			const FVector2D T1 = 
				CatmullTangent(P0, ControlPoints[INext].Position);
			Tan = HermiteDeriv(P0, T0, P1, T1, T);
			break;
		}
		
	case EPathInterp::Hermite:
		Tan = HermiteDeriv(P0, ControlPoints[I0].LeaveTangent, 
						   P1, ControlPoints[I1].ArriveTangent, T);
		break;
	}
	
	// When the derivative degenerates due to inflection points / coincident 
	// control points, fall back to the chord direction
	if (Tan.IsNearlyZero()) { Tan = P1 - P0; }
	if (Tan.IsNearlyZero()) { return FVector2D(1, 0); }
	return Tan.GetSafeNormal();
}

double FPathCurve::EvalElevation(double Alpha) const
{
	if (ControlPoints.Num() == 0) { return 0.0; }
	if (ControlPoints.Num() == 1) { return ControlPoints[0].Elevation; }
	int32 Seg; double T;
	GetSegment(Alpha, Seg, T);
	const int32 N = ControlPoints.Num();
	
	return FMath::Lerp(
		ControlPoints[Seg].Elevation, 
		ControlPoints[(Seg + 1) % N].Elevation, 
		T);
}

double FPathCurve::EvalRoll(double Alpha) const
{
	if (ControlPoints.Num() == 0) { return 0.0; }
	if (ControlPoints.Num() == 1) { return ControlPoints[0].RollDegrees; }
	int32 Seg; double T;
	GetSegment(Alpha, Seg, T);
	const int32 N = ControlPoints.Num();
	
	return FMath::Lerp(
		ControlPoints[Seg].RollDegrees, 
		ControlPoints[(Seg + 1) % N].RollDegrees, 
		T);
}

void FPathCurve::BuildArcTable() const
{
	ArcTable.Reset();
	CachedLength = 0.0;
	if (!IsValid()) { return; }
	
	const int32 Total = NumSegments() * ArcSamplesPerSegment;
	ArcTable.Reserve(Total + 1);
	
	FVector2D Prev = EvalPosition(0.0);
	ArcTable.Add({ 0.0, 0.0, Prev });
	
	double Accum = 0.0;
	for (int32 i = 1; i <= Total; ++i)
	{
		const double A = static_cast<double>(i) / Total;
		const FVector2D P = EvalPosition(A);
		Accum += FVector2D::Distance(Prev, P);
		ArcTable.Add({ A, Accum, P });
		Prev = P;
	}
	CachedLength = Accum;
}

double FPathCurve::GetLength() const
{
	if (CachedLength < 0.0) { BuildArcTable(); };
	return CachedLength;
}

FPathSample FPathCurve::EvalAtAlpha(double Alpha) const
{
	FPathSample S;
	const double A = FMath::Clamp(Alpha, 0.0, 1.0);
	S.Alpha       = A;
	S.Position    = EvalPosition(A);
	S.Elevation   = EvalElevation(A);
	S.Tangent     = EvalTangent(A);
	S.LeftNormal  = Rot90CCW(S.Tangent);
	S.RollDegrees = EvalRoll(A);
	// Approx. distance. Use EvalAtDistance for accurate value.
	S.Distance    = A * GetLength();   
	return S;
}

FPathSample FPathCurve::EvalAtDistance(double Distance) const
{
	if (CachedLength < 0.0) { BuildArcTable(); }
	if (ArcTable.Num() < 2) { return EvalAtAlpha(0.0); }
	
	const double D = FMath::Clamp(
		Distance, 0.0, CachedLength);
	
	// LUT binary search + linear interpolation within the segment. 
	// ArcTable is monotonically increasing, so UpperBound can be used directly
	int32 Low = 0, High = ArcTable.Num() - 1;
	while (Low + 1 < High)
	{
		const int32 Mid = (Low + High) / 2;
		if (ArcTable[Mid].Distance <= D)
		{
			Low = Mid;
		}
		else
		{
			High = Mid;
		}
	}
	
	const double SegLen = 
		ArcTable[High].Distance - ArcTable[Low].Distance;
	const double T = (SegLen > UE_KINDA_SMALL_NUMBER) 
		? (D - ArcTable[Low].Distance) / SegLen : 0.0;
	const double Alpha = FMath::Lerp(
		ArcTable[Low].Alpha, 
		ArcTable[High].Alpha, T);
	
	FPathSample S = EvalAtAlpha(Alpha);
	S.Distance = D;   // Accurate arc length
	return S;
}

FTransform FPathCurve::GetTransformAtDistance(double Distance) const
{
	const FPathSample S = EvalAtDistance(Distance);
	const FVector Fwd = FVector(S.Tangent.X, S.Tangent.Y, 0.0);
	FTransform Xf(
		FRotationMatrix::MakeFromXZ(Fwd, FVector::UpVector).Rotator(), 
		S.GetLocation3D());
	
	if (!FMath::IsNearlyZero(S.RollDegrees))
	{
		Xf.SetRotation(Xf.GetRotation() * FQuat(
			FVector::XAxisVector,
			FMath::DegreesToRadians(S.RollDegrees)));
	}
	return Xf;
}

double FPathCurve::FindNearestDistance(
	const FVector2D& Query, double* OutDistanceToCurve) const
{
	if (CachedLength < 0.0) { BuildArcTable(); }
	if (ArcTable.Num() < 2)
	{
		if (OutDistanceToCurve) { *OutDistanceToCurve = 0.0; }
		return 0.0;
	}
	
	double BestDistSq = TNumericLimits<double>::Max();
	double BestArc = 0.0;
	
	for (int32 i = 0; i + 1 < ArcTable.Num(); ++i)
	{
		const FVector2D& A = ArcTable[i].Position;
		const FVector2D& B = ArcTable[i+1].Position;
		const FVector2D Seg = B - A;
		const double LenSq = Seg.SizeSquared();
		double T = 0.0;
		if (LenSq > UE_KINDA_SMALL_NUMBER)
		{
			T = FMath::Clamp(
				FVector2D::DotProduct(Query - A, Seg) / LenSq, 
				0.0, 1.0);
		}
		const FVector2D P = A + Seg * T;
		// Squared distance between Query and control point
		const double DSq = FVector2D::DistSquared(Query, P);
		if (DSq < BestDistSq)
		{
			BestDistSq = DSq;
			BestArc = FMath::Lerp(ArcTable[i].Distance, 
				ArcTable[i+1].Distance, T);
		}
	}
	if (OutDistanceToCurve)
	{
		*OutDistanceToCurve = FMath::Sqrt(BestDistSq);
	}
	return BestArc;
}

/**
 * TODO: Problem:
 * - The first and last samples have large errors on tangent directions
 *   due to the boundary effect of CatmullTangent method
 * - Perhaps a better interp method is needed at boundary
 */
void FPathCurve::SampleAdaptive(
	double MaxChordError, TArray<FPathSample>& Out) const
{
	Out.Reset();
	if (!IsValid()) { return; }
	
	// Linear: tessellation is not necessary
	// Out array corresponds to control points
	if (Interp == EPathInterp::Linear)
	{
		const int32 N = ControlPoints.Num();
		const int32 Count = bClosed ? N + 1 : N;
		for (int32 i = 0; i < Count; ++i)
		{
			Out.Add(EvalAtAlpha(
				static_cast<double>(i) / FMath::Max(NumSegments(), 1)
			));
		}
		return;
	}
	
	const double Tol = FMath::Max(MaxChordError, 0.01);
	
	// Recursive chord-height subdivision. Depth limit is used prevents 
	// degenerate curves from blowing the stack
	TFunction<void(double, double, int32)> Recurse = [&](double A0, double A1, int32 Depth)
	{
		// A is short for alpha
		const double Am = (A0 + A1) * 0.5;
		const FVector2D P0 = EvalPosition(A0);
		const FVector2D P1 = EvalPosition(A1);
		const FVector2D Pm = EvalPosition(Am);
		
		const FVector2D Chord = P1 - P0;
		const double ChordLen = Chord.Size();
		double Deviation;
		if (ChordLen > UE_KINDA_SMALL_NUMBER)
		{
			const FVector2D D = Pm - P0;
			Deviation = FMath::Abs(D.X * Chord.Y - D.Y * Chord.X) / ChordLen;
		}
		else
		{
			Deviation = FVector2D::Distance(P0, Pm);
		}
		
		if (Depth < MaxTessellateDepth && Deviation > Tol)
		{
			Out.Add(EvalAtAlpha(Am));
			Recurse(A0, Am, Depth + 1);
			Recurse(A1, Am, Depth + 1);
		}
	};
	
	const int32 NumSeg = NumSegments();
	Out.Add(EvalAtAlpha(0.0));
	for (int32 s = 0; s < NumSeg; ++s)
	{
		const double A0 = static_cast<double>(s) / NumSeg;
		const double A1 = static_cast<double>(s + 1) / NumSeg;
		Recurse(A0, A1, 0);
		Out.Add(EvalAtAlpha(A1));
	}
}

void FPathCurve::SampleUniform(double Spacing, TArray<FPathSample>& Out) const
{
	Out.Reset();
	if (!IsValid() || Spacing <= UE_KINDA_SMALL_NUMBER) { return; }
	
	const double L = GetLength();
	const int32 Count = 
		FMath::Max(1, FMath::RoundToInt32(L / Spacing));
	const double Step = L / Count;
	Out.Reserve(Count + 1);
	for (int32 i = 0; i <= Count; ++i)
	{
		Out.Add(EvalAtDistance(i * Step));
	}
}

void FPathCurve::SampleCount(int32 Count, TArray<FPathSample>& Out) const
{
	Out.Reset();
	if (!IsValid() || Count < 2) { return; }
	const double L = GetLength();
	Out.Reserve(Count);
	for (int32 i = 0; i < Count; ++i)
	{
		Out.Add(EvalAtDistance(L * i / (Count - 1)));
	}
}

FPathCurve FPathCurve::OffsetLateral(double Offset, double MaxChordError) const
{
	TArray<FPathSample> Samples;
	SampleAdaptive(MaxChordError, Samples);
	
	FPathCurve Result;
	// Tessellated, linear is enough
	Result.Interp = EPathInterp::Linear;
	Result.bClosed = bClosed;
	Result.ControlPoints.Reserve(Samples.Num());
	for (const FPathSample& S : Samples)
	{
		FPathControlPoint CP;
		CP.Position    = S.Position + S.LeftNormal * Offset;
		CP.Elevation   = S.Elevation;
		CP.RollDegrees = S.RollDegrees;
		Result.ControlPoints.Add(CP);
	}
	// For closed path, the first and last samples overlap. Remove the last
	if (bClosed && Result.ControlPoints.Num() >= 2 &&
		FVector2D::Distance(
			Result.ControlPoints[0].Position, Result.ControlPoints.Last().Position)
			< ProcCityGeometry::PositionTolerance)
	{
		Result.ControlPoints.Pop();
	}
	return Result;
}

bool FPathCurve::BuildRibbon(const FWidthProfile& Profile, 
	double MaxChordError, TArray<FPolygon2D>& Out) const
{
	Out.Reset();
	if (!IsValid()) { return false; }
	
	TArray<FPathSample> Samples;
	SampleAdaptive(MaxChordError, Samples);
	if (Samples.Num() < 2) { return false; }
	
	/**
	 * - Build quadrilaterals segment by segment, then union.  
	 * - Compared to "walk the left boundary forward + the right boundary backward" to 
	 *   stitch one big ring, the advantage of per-segment union is that inner-side 
	 *   self-intersections at sharp turns are resolved automatically by Clipper, 
	 *   without producing flipped negative-area regions. */
	TArray<FPolygon2D> Quads;
	Quads.Reserve(Samples.Num() - 1);
	
	for (int32 i = 0; i + 1 < Samples.Num(); ++i)
	{
		const FPathSample& S0 = Samples[i];
		const FPathSample& S1 = Samples[i + 1];
		
		// Left and right widths
		double L0, R0, L1, R1;
		Profile.Evaluate(S0.Alpha, L0, R0);
		Profile.Evaluate(S1.Alpha, L1, R1);
		
		// Right side vertices
		const FVector2D A = S0.Position - S0.LeftNormal * R0;  
		const FVector2D B = S1.Position - S1.LeftNormal * R1;
		// Left side vertices
		const FVector2D C = S1.Position + S1.LeftNormal * L1; 
		const FVector2D D = S0.Position + S0.LeftNormal * L0;
		
		FPolygon2D Q;
		Q.Outer.Vertices = {A, B, C, D};
		// CCW guarantee
		Q.Normalize();
		if (Q.IsValid()) {Quads.Add(MoveTemp(Q));}
	}
	
	if (Quads.Num() == 0) { return false; }
	
	// Iterative union 
	TArray<FPolygon2D> Accum;
	Accum.Add(Quads[0]);
	for (int32 i = 1; i < Quads.Num(); ++i)
	{
		TArray<FPolygon2D> Merged;
		if (ProcCityGeometry::UnionPolygons(Accum, 
			MakeArrayView(&Quads[i], 1), Merged))
		{
			Accum = MoveTemp(Merged);
		}
		else
		{
			// Kee quads [i] as independent piece if union 
			// fails to avoid losing data
			Accum.Add(Quads[i]);
		}
	}
	
	for (FPolygon2D& P : Accum)
	{
		P.Simplify(FPolySimplifyParams::Cleanup());
		if (P.IsValid()) { Out.Add(MoveTemp(P)); }
	}
	Out.Sort(
		[](const FPolygon2D& A, const FPolygon2D& B) 
		{ return A.Area() > B.Area(); });
	return Out.Num() > 0;
}

bool FPathCurve::BuildRibbonSingle(const FWidthProfile& Profile, 
	double MaxChordError, FPolygon2D& Out) const
{
	TArray<FPolygon2D> Results;
	if (!BuildRibbon(Profile, MaxChordError, Results))
	{
		return false;
	}
	
	// Results are sorted in area-descent order
	Out = MoveTemp(Results[0]);
	return true;
}

void FPathCurve::ScatterAlong(double Spacing, double LateralOffset, 
	double StartOffset, TArray<FTransform>& Out) const
{
	Out.Reset();
	if (!IsValid() || Spacing <= UE_KINDA_SMALL_NUMBER) { return; }
	
	const double L = GetLength();
	for (double D = StartOffset; 
		D <= L + UE_KINDA_SMALL_NUMBER; D += Spacing)
	{
		const FPathSample S = EvalAtDistance(D);
		const FVector2D P = S.Position + S.LeftNormal * LateralOffset;
		const FVector Fwd(S.Tangent.X, S.Tangent.Y, 0.0);
		Out.Add(FTransform(
			FRotationMatrix::MakeFromXZ(Fwd, FVector::UpVector).Rotator(),
			FVector(P.X, P.Y, S.Elevation)));
	}
}

FBox2D FPathCurve::Bounds2D() const
{
	FBox2D B(ForceInit);
	TArray<FPathSample> Samples;
	SampleAdaptive(ProcCityGeometry::DefaultMaxChordError, Samples);
	for (const FPathSample& S : Samples) { B += S.Position; }
	return B;
}

uint64 FPathCurve::ComputeStableHash() const
{
	uint64 H = 1469598103934665603ull;
	auto Mix = [&H](uint64 V)
	{
		H ^= V; 
		H *= 1099511628211ull;
	};
	
	// Quantize for stable hash
	auto Q = [](double V) -> int64
	{
		return static_cast<int64>(
			FMath::RoundToDouble(V / ProcCityGeometry::PositionTolerance));
	};
	
	Mix(static_cast<uint64>(Interp));
	Mix(bClosed ? 1u : 0u);
	Mix(static_cast<uint64>(ControlPoints.Num()));
	for (const FPathControlPoint& P : ControlPoints)
	{
		Mix(static_cast<uint64>(Q(P.Position.X)));
		Mix(static_cast<uint64>(Q(P.Position.Y)));
		Mix(static_cast<uint64>(Q(P.Elevation)));
	}
	return H;
}














