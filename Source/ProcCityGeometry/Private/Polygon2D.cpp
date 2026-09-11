#include "ProcCityGeometry/Polygon2D.h"

#include "Algo/Reverse.h"
#include "Polygon2.h"
#include "Curve/GeneralPolygon2.h"
#include "CompGeom/ConvexHull2.h"
#include "ConstrainedDelaunay2.h"

namespace
{
	UE::Geometry::FPolygon2d RingToPolygon2d(const FPolyRing& Ring)
	{
		UE::Geometry::FPolygon2d Out;
		Out.SetVertices(Ring.Vertices);
		
		return Out;
	}
	
	FPolyRing Polygon2dToRing(const UE::Geometry::FPolygon2d& Poly)
	{
		FPolyRing Ring;
		Ring.Vertices = Poly.GetVertices();
		
		return Ring;
	}
}

// ============== FPolyRing ==============

double FPolyRing::SignedArea() const
{
	const int32 N = Vertices.Num();
	if (N < 3) { return 0.0; }
	
	// Shoelace, accumulated in double. Computed relative to the centroid 
	// to reduce precision loss under large coordinates
	const FVector2D Ref = Vertices[0];
	double Acc = 0.0f;
	
	for (int32 i = 1; i < N - 1; ++i)
	{
		const FVector2D A = Vertices[i] - Ref;
		const FVector2D B = Vertices[(i + 1) % N] - Ref;
		Acc += A.X * B.Y - B.X * A.Y;
	}
	
	return Acc * 0.5;
}

EPolyWinding FPolyRing::GetWinding() const
{
	const double A = SignedArea();
	if (FMath::Abs(A) < ProcCityGeometry::AreaTolerance)
	{
		return EPolyWinding::Degenerate;
	}
	
	return A > 0.0 ? EPolyWinding::CounterClockwise : EPolyWinding::Clockwise;
}

void FPolyRing::EnsureWinding(EPolyWinding Desired)
{
	const EPolyWinding Current = GetWinding();
	if (Current == EPolyWinding::Degenerate || Current == Desired) { return; }
	
	Reverse();
}

void FPolyRing::Simplify(const FPolySimplifyParams& Params)
{
	if (Vertices.Num() <= Params.MinVertices) { return; }
	
	// --- Pass 1: remove adjacent duplicate point
	if (Params.MergeDistance > 0.0)
	{
		TArray<FVector2D> Out;
		Out.Reserve(Vertices.Num());
		for (const FVector2D& V : Vertices)
		{
			if (Out.Num() == 0 || 
				FVector2D::Distance(Out.Last(), V) > Params.MergeDistance)
			{
				Out.Add(V);
			}
		}
		while (Out.Num() >= 2 && 
			FVector2D::Distance(Out[0], Out.Last()) <= Params.MergeDistance)
		{
			Out.Pop();
		}
		Vertices = MoveTemp(Out);
	}
	
	const bool bUseDeviation = Params.MaxDeviation > 0.0;
	const bool bUseAngle     = Params.MaxAngle > 0.0;
	if (!bUseDeviation && !bUseAngle) { return; }
	
	// --- Pass 2: Simplify collinear vertices 
	bool bChanged = true;
	while (bChanged && Vertices.Num() > Params.MinVertices)
	{
		bChanged = false;
		const int32 N = Vertices.Num();
		TBitArray<> Remove(false, N);
		int32 NumRemoved = 0;
		
		// Examine every other vertex to avoid deleting consecutive vertices 
		// in the same pass, which would over-simplify
		for (int32 i = 0; i < N; ++i)
		{
			if (N - NumRemoved <= Params.MinVertices) { break; }
			
			// The predecessor must be a vertex kept in this pass
			int32 Prev = (i + N - 1) % N;
			if (Remove[Prev]) { continue; }
			
			const int32 Next = (i + 1) % N;
			if (Remove[Next]) { continue; }
			
			const FVector2D& P0 = Vertices[Prev];
			const FVector2D& P1 = Vertices[i];
			const FVector2D& P2 = Vertices[Next];
			
			bool bCollinear = true;
			
			if (bUseDeviation)
			{
				const double Cross = FMath::Abs((P1.X - P0.X) * (P2.Y - P0.Y)
											  - (P2.X - P0.X) * (P1.Y - P0.Y));
				const double Base = FVector2D::Distance(P0, P2);
				const double Deviation = (Base > UE_KINDA_SMALL_NUMBER)
					? (Cross / Base)
					: FVector2D::Distance(P0, P1);
				bCollinear &= (Deviation <= Params.MaxDeviation);
			}
			
			if (bUseAngle)
			{
				const FVector2D D0 = (P1 - P0).GetSafeNormal();
				const FVector2D D1 = (P2 - P1).GetSafeNormal();
				if (D0.IsNearlyZero() || D1.IsNearlyZero())
				{
					bCollinear = true; // Zero edge length, removable
				}
				else
				{
					const double Dot = FMath::Clamp(
						FVector2D::DotProduct(D0, D1), -1.0, 1.0);
					bCollinear &= (FMath::Acos(Dot) <= Params.MaxAngle);
				}
			}
			
			if (bCollinear)
			{
				Remove[i] = true;
				++NumRemoved;
				bChanged = true;
			}
		}
		
		if (NumRemoved > 0)
		{
			TArray<FVector2D> Out;
			Out.Reserve(N - NumRemoved);
			for (int32 i = 0; i < N; ++i)
			{
				if (!Remove[i]) { Out.Add(Vertices[i]); }
			}
			Vertices = MoveTemp(Out);
		}
	}
}

FBox2D FPolyRing::Bounds() const
{
	FBox2D Box(ForceInit);
	for (const FVector2D& V : Vertices) { Box += V; }
	return Box;
}

double FPolyRing::Perimeter() const
{
	const int32 N = Vertices.Num();
	if (N < 2) { return 0.0; }
	double L = 0.0;
	for (int32 i = 0; i < N; ++i)
	{
		L += FVector2D::Distance(Vertices[i], Vertices[(i + 1) % N]);
	}
	return L;
}

// ============== FPolygon2D ==============
FPolygon2D::FPolygon2D(const TArray<FVector2D>& InOuter)
{
	Outer.Vertices = InOuter;
	Normalize();
}

FPolygon2D::FPolygon2D(TArray<FVector2D>&& InOuter)
{
	Outer.Vertices = MoveTemp(InOuter);
	Normalize();
}

FPolygon2D FPolygon2D::MakeRect(
	const FVector2D& Center, const FVector2D& Extent, double YawDegrees)
{
	const double YawRad = FMath::DegreesToRadians(YawDegrees);
	const double C = FMath::Cos(YawRad), S = FMath::Sin(YawRad);
	
	// CCW winding: (-x,-y) -> (+x,-y) -> (+x,+y) -> (-x,+y)
	const FVector2D Local[4] = {
		FVector2D(-Extent.X, -Extent.Y),
		FVector2D( Extent.X, -Extent.Y),
		FVector2D( Extent.X,  Extent.Y),
		FVector2D(-Extent.X,  Extent.Y)
	};
	
	FPolygon2D Poly;
	Poly.Outer.Vertices.Reserve(4);
	for (const FVector2D& V : Local)
	{
		Poly.Outer.Vertices.Add(Center + 
			FVector2D(V.X * C - V.Y * S, V.X * S + V.Y * C));
	}
	return Poly; // Normalize is not needed.
}

FPolygon2D FPolygon2D::MakeFromBox(const FBox2D& Box)
{
	return MakeRect(Box.GetCenter(), Box.GetExtent(), 0.0);
}

FPolygon2D FPolygon2D::MakeFromOrientedBox(const FOrientedBox2D& Box)
{
	return MakeRect(Box.Center, Box.Extent, 
		FMath::RadiansToDegrees(Box.Rotation));
}

FPolygon2D FPolygon2D::MakeRegular(const FVector2D& Center, 
	double Radius, int32 NumSides, double StartAngleRad)
{
	FPolygon2D Poly;
	if (NumSides < 3 || Radius <= 0.0) { return Poly; }
	
	Poly.Outer.Vertices.Reserve(NumSides);
	for (int32 i = 0; i < NumSides; ++i)
	{
		const double AngRad = StartAngleRad + (2.0 * PI * i) / NumSides;
		Poly.Outer.Vertices.Add(Center + 
			FVector2D(FMath::Cos(AngRad), FMath::Sin(AngRad)) * Radius);
	}
	return Poly;
}

FPolygon2D FPolygon2D::MakeConvexHull(TArrayView<const FVector2D> Points)
{
	FPolygon2D Poly;
	if (Points.Num() < 3) { return Poly; }
	
	UE::Geometry::FConvexHull2d Hull;
	const bool bSolved = Hull.Solve(
		Points.Num(), 
		[&](int32 Idx) { return Points[Idx]; }
	);
	
	if (!bSolved) { return Poly; }
	
	const TArray<int32>& HullIndices = Hull.GetPolygonIndices();
	Poly.Outer.Vertices.Reserve(HullIndices.Num());
	for (int32 Idx : HullIndices)
	{
		Poly.Outer.Vertices.Add(Points[Idx]);
	}
	
	Poly.Normalize();
	return Poly;
}

bool FPolygon2D::IsValid() const
{
	if (Outer.NumVertices() < 3) { return false; }
	if (FMath::Abs(Outer.SignedArea()) < ProcCityGeometry::AreaTolerance)
	{
		return false;
	}
	
	for (const FPolyRing& H : Holes)
	{
		if (H.NumVertices() < 3) { return false; }
	}
	return true;
}

void FPolygon2D::Normalize()
{
	Outer.Simplify();
	Outer.EnsureWinding(EPolyWinding::CounterClockwise);
	
	for (int32 i = Holes.Num() - 1; i >= 0; --i)
	{
		Holes[i].Simplify();
		if (Holes[i].NumVertices() < 3)
		{
			Holes.RemoveAt(i);
			continue;
		}
		Holes[i].EnsureWinding(EPolyWinding::Clockwise);
	}
}

double FPolygon2D::Area() const
{
	double A = Outer.SignedArea();
	for (const FPolyRing& H : Holes)
	{
		A += H.SignedArea(); 
	}
	return FMath::Max(0.0, A);
}

FVector2D FPolygon2D::Centroid() const
{
	auto RingCentroidWeighted = [](const FPolyRing& Ring, FVector2D& OutAccum) -> double
	{
		const int32 N = Ring.NumVertices();
		if (N < 3) { return 0.0; }
		const FVector2D Ref = Ring.Vertices[0];
		double A2 = 0.0;
		FVector2D C(0, 0);
		for (int32 i = 0; i < N; ++i)
		{
			const FVector2D P = Ring.Vertices[i] - Ref;
			const FVector2D Q = Ring.Vertices[(i + 1) % N] - Ref;
			const double Cross = P.X * Q.Y - Q.X * P.Y;
			A2 += Cross;
			C += (P + Q) * Cross;
		}
		
		if (FMath::Abs(A2) < KINDA_SMALL_NUMBER)
		{
			return 0.0;
		}
		
		const double Area = A2 * 0.5;
		OutAccum += (C / (3.0 * A2)) * Area + Ref * Area;
		return Area;
	};
	
	FVector2D Accum(0, 0);
	double TotalArea = RingCentroidWeighted(Outer, Accum);
	for (const FPolyRing& H : Holes)
	{
		TotalArea += RingCentroidWeighted(H, Accum);
	}
	
	return FMath::Abs(TotalArea) > KINDA_SMALL_NUMBER 
		? Accum / TotalArea 
		: (Outer.NumVertices() > 0 ? Outer.Bounds().GetCenter() : FVector2D::ZeroVector);
}

double FPolygon2D::Perimeter() const
{
	return Outer.Perimeter();
}

FBox2D FPolygon2D::Bounds() const
{
	return Outer.Bounds();
}

double FPolygon2D::Compactness() const
{
	const double P = Perimeter();
	return P > KINDA_SMALL_NUMBER ? (4.0 * PI * Area()) / (P * P) : 0.0;
}

FOrientedBox2D FPolygon2D::MinAreaOBB() const
{
	FOrientedBox2D Result;
	
	// TODO: When VertexNum < 3, a world axis aligned bound is obtained
	//       Is this the intended behavior?
	if (Outer.NumVertices() < 3)
	{
		if (Outer.NumVertices() > 0)
		{
			const FBox2D B = Outer.Bounds();
			Result.Center = B.GetCenter();
			Result.Extent = B.GetExtent();
		}
		return Result;
	}
	
	// 1) Make convex hull
	const FPolygon2D Hull = MakeConvexHull(Outer.Vertices);
	const TArray<FVector2D>& HullVertices = Hull.Outer.Vertices;
	if (HullVertices.Num() < 3)
	{
		// TODO: A world axis aligned bound is obtained
		const FBox2D B = Outer.Bounds();
		Result.Center = B.GetCenter();
		Result.Extent = B.GetExtent();
		return Result;
	}
	
	// 2)  Rotating calipers: the minimum-area rectangle always has one side collinear
	// with an edge of the convex hull
	double BestArea = TNumericLimits<double>::Max();
	for (int32 i = 0; i < HullVertices.Num(); ++i)
	{
		const FVector2D E = 
			HullVertices[(i + 1) % HullVertices.Num()] - HullVertices[i];
		const double Len = E.Size();
		if (Len < ProcCityGeometry::PositionTolerance) { continue; }
		
		const FVector2D AxisX = E / Len;
		const FVector2D AxisY(-AxisX.Y, AxisX.X);
		
		double MinX = TNumericLimits<double>::Max(), MaxX = -MinX;
		double MinY = MinX, MaxY = MaxX;
		
		for (const FVector2D& V : HullVertices)
		{
			const FVector2D D = V - HullVertices[i];
			const double Px = FVector2D::DotProduct(D, AxisX);
			const double Py = FVector2D::DotProduct(D, AxisY);
			MinX = FMath::Min(MinX, Px); 
			MaxX = FMath::Max(MaxX, Px);
			MinY = FMath::Min(MinY, Py); 
			MaxY = FMath::Max(MaxY, Py);
		}
		
		const double Width = MaxX - MinX;
		const double Height = MaxY - MinY;
		const double Area = Width * Height;
		if (Area < BestArea)
		{
			BestArea = Area;
			const FVector2D LocalCenter(
				(MinX + MaxX) * 0.5, (MinY + MaxY) * 0.5);
			Result.Center = HullVertices[i] + 
				AxisX * LocalCenter.X + AxisY * LocalCenter.Y;
			Result.Extent = FVector2D(Width * 0.5, Height * 0.5);
			Result.Rotation = FMath::Atan2(AxisX.Y, AxisX.X);
		}
	}
	
	// 3) Convention: make X the long axis. This makes the upper-layer "split 
	//    along the long edge" logic easier
	if (Result.Extent.Y > Result.Extent.X)
	{
		Swap(Result.Extent.X, Result.Extent.Y);
		Result.Rotation += HALF_PI;
	}
	
	// Normalize to [-PI/2, PI/2)
	while (Result.Rotation >= HALF_PI) { Result.Rotation -= PI; }
	while (Result.Rotation < -HALF_PI) { Result.Rotation += PI; }
	
	return Result;
}

bool FPolygon2D::Contains(const FVector2D& Point) const
{
	auto RingContains = [](const FPolyRing& Ring, const FVector2D& P) -> bool
	{
		// Standard crossing number
		bool bInside = false;
		const int32 N = Ring.NumVertices();
		for (int32 i = 0, j = N - 1; i < N; j = i++)
		{
			const FVector2D& A = Ring.Vertices[i];
			const FVector2D& B = Ring.Vertices[j];
			if (((A.Y > P.Y) != (B.Y > P.Y)) &&
				(P.X < (B.X - A.X) * (P.Y - A.Y) / (B.Y - A.Y) + A.X))
			{
				bInside = !bInside;
			}
		}
		return bInside;
	};
	
	if (!RingContains(Outer, Point)) { return false; }
	
	for (const FPolyRing& Hole : Holes)
	{
		if (RingContains(Hole, Point)) { return false; }
	}
	return true;
}

bool FPolygon2D::Contains(const FPolygon2D& Other) const
{
	// Conservative check: all vertices inside and boundaries do not intersect
	for (const FVector2D& V : Other.Outer.Vertices)
	{
		if (!Contains(V)) { return false; }
	}
	
	TArray<FPolyEdge> EdgesA, EdgesB;
	GetEdges(EdgesA);
	Other.GetEdges(EdgesB);
	for (const FPolyEdge& Ea : EdgesA)
	{
		for (const FPolyEdge& Eb : EdgesB)
		{
			if (UE::Geometry::FSegment2d(Ea.Start, Ea.End).Intersects(
				UE::Geometry::FSegment2d(Eb.Start, Eb.End)))
			{ return false; }
		}
	}
	return true;
}

double FPolygon2D::SignedDistanceToBoundary(const FVector2D& Point) const
{
	const FVector2D Closest = ClosestPointOnBoundary(Point);
	const double D = FVector2D::Distance(Closest, Point);
	
	return Contains(Point) ? D : -D;
}

FVector2D FPolygon2D::ClosestPointOnBoundary(
	const FVector2D& Point, int32* OutRingIndex, int32* OutEdgeIndex) const
{
	TArray<FPolyEdge> Edges;
	GetEdges(Edges);
	
	double BestDistSq = TNumericLimits<double>::Max();
	FVector2D Best = Point;
	for (const FPolyEdge& E : Edges)
	{
		const FVector2D Seg = E.End - E.Start;
		const double LenSq = Seg.SizeSquared();
		double T = 0.0;
		if (LenSq > KINDA_SMALL_NUMBER)
		{
			T = FMath::Clamp(
				FVector2D::DotProduct(Point - E.Start, Seg) / LenSq, 
				0.0, 1.0);
		}
		
		// Compute the closest point on E 
		const FVector2D P = E.Start + Seg * T;
		const double DSq = FVector2D::DistSquared(Point, P);
		
		if (DSq < BestDistSq)
		{
			BestDistSq = DSq;
			Best = P;
			if (OutRingIndex) { *OutRingIndex = E.RingIndex; }
			if (OutEdgeIndex) { *OutEdgeIndex = E.EdgeIndex; }
		}
	}
	
	return Best;
}

void FPolygon2D::GetOuterEdges(TArray<FPolyEdge>& OutEdges) const
{
	const int32 N = Outer.NumVertices();
	OutEdges.Reset(N);
	for (int32 i = 0; i < N; ++i)
	{
		OutEdges.Add(GetOuterEdge(i));
	}
}

FPolyEdge FPolygon2D::GetOuterEdge(int32 EdgeIndex) const
{
	FPolyEdge E;
	const int32 N = Outer.NumVertices();
	
	if (N < 2 || !Outer.Vertices.IsValidIndex(EdgeIndex))
	{
		return E;
	}
	
	E.EdgeIndex = EdgeIndex;
	E.RingIndex = 0;
	E.Start = Outer.Vertices[EdgeIndex];
	E.End   = Outer.Vertices[(EdgeIndex + 1) % N];
	
	const FVector2D D = E.End - E.Start;
	E.Length = D.Size();
	
	if (E.Length > ProcCityGeometry::PositionTolerance)
	{
		const FVector2D Dir = D / E.Length;
		// CCW outer ring. Normal points to outer side.
		E.Normal = FVector2D(Dir.Y, -Dir.X);
	}
	
	return E;
}

void FPolygon2D::GetEdges(TArray<FPolyEdge>& OutEdges) const
{
	OutEdges.Reset();
	GetOuterEdges(OutEdges);

	for (int32 RingIdx = 0; RingIdx < Holes.Num(); ++RingIdx)
	{
		const FPolyRing& Ring = Holes[RingIdx];
		const int32 N = Ring.NumVertices();
		for (int32 i = 0; i < N; ++i)
		{
			FPolyEdge E;
			E.EdgeIndex = i;
			E.RingIndex = RingIdx + 1;
			E.Start = Ring.Vertices[i];
			E.End   = Ring.Vertices[(i + 1) % N];
			const FVector2D D = E.End - E.Start;
			E.Length = D.Size();
			if (E.Length > ProcCityGeometry::PositionTolerance)
			{
				const FVector2D Dir = D / E.Length;
				// CW holes. Normal points to hole inner.
				E.Normal = FVector2D(Dir.Y, -Dir.X);
			}
			OutEdges.Add(E);
		}
	}
}

void FPolygon2D::Simplify(const FPolySimplifyParams& Params)
{
	Outer.Simplify(Params);
	for (int32 i = Holes.Num() - 1; i >= 0; --i)
	{
		Holes[i].Simplify(Params);
		if (Holes[i].NumVertices() < 3) { Holes.RemoveAt(i); }
	}
}

void FPolygon2D::Translate(const FVector2D& Delta)
{
	for (FVector2D& V : Outer.Vertices)
	{
		V += Delta;
	}
	
	for (FPolyRing& H : Holes)
	{
		for (FVector2D& V : H.Vertices) { V += Delta; }
	}
}

void FPolygon2D::Rotate(double AngleRad, const FVector2D& Pivot)
{
	const double C = FMath::Cos(AngleRad), S = FMath::Sin(AngleRad);
	auto Rot = [&](FVector2D& V)
	{
		const FVector2D D = V - Pivot;
		V = Pivot + FVector2D(D.X * C - D.Y * S, D.X * S + D.Y * C);
	};
	for (FVector2D& V : Outer.Vertices) { Rot(V); }
	for (FPolyRing& H : Holes)
	{
		for (FVector2D& V : H.Vertices) { Rot(V); }
	}
}

void FPolygon2D::Scale(double Factor, const FVector2D& Pivot)
{
	auto Scl = [&](FVector2D& V)
	{
		V = Pivot + (V - Pivot) * Factor;
	};
	
	for (FVector2D& V : Outer.Vertices) { Scl(V); }
	for (FPolyRing& H : Holes)
	{
		for (FVector2D& V : H.Vertices) { Scl(V); }
	}
	
	// Negative scale changes winding
	if (Factor < 0.0)
	{
		Outer.EnsureWinding(EPolyWinding::CounterClockwise);
		for (FPolyRing& H : Holes)
		{
			H.EnsureWinding(EPolyWinding::Clockwise);
		}
	} 
}

void FPolygon2D::Transform(const FTransform2d& Xf)
{
	for (FVector2D& V : Outer.Vertices)
	{
		V = Xf.TransformPoint(V);
	}
	
	for (FPolyRing& H : Holes)
	{
		for (FVector2D& V : H.Vertices)
		{
			V = Xf.TransformPoint(V);
		}
	}
	
	// Negative determinant => contains a mirror => winding flips, and the invariant must be restored.
	// Note: do not call Normalize() unconditionally — it also performs Simplify,
	//       which under a pure rigid transform is unnecessary precision loss and overhead.
	const FMatrix2x2d M = Xf.GetMatrix();
	if (M.Determinant() < 0.0)
	{
		Outer.EnsureWinding(EPolyWinding::CounterClockwise);
		for (FPolyRing& H : Holes)
		{
			H.EnsureWinding(EPolyWinding::Clockwise);
		}
	}
}

FPolygon2D FPolygon2D::GetTransformed(const FTransform2d& Xf) const
{
	FPolygon2D Copy = *this;
	Copy.Transform(Xf);
	
	return Copy;
}

bool FPolygon2D::HasSelfIntersection(double Tolerance) const
{
	TArray<FPolyEdge> Edges;
	GetEdges(Edges);
	const int32 N = Edges.Num();
	
	// O(n^2): n is typically < 50 at the block/lot level, so this is acceptable.
	// If the upper layer uses this for large-scale road networks, switch to 
	// FArrangement2d / sweep line instead.
	for (int32 i = 0; i < N; ++i)
	{
		for (int32 j = i + 1; j < N; ++j)
		{
			//  Skip adjacent edges within same ring
			if (Edges[i].RingIndex == Edges[j].RingIndex)
			{
				const int32 RingSize = (Edges[i].RingIndex == 0) 
					? Outer.NumVertices() 
					: Holes[Edges[i].RingIndex - 1].NumVertices();
				
				const int32 Diff = FMath::Abs(Edges[i].EdgeIndex - Edges[j].EdgeIndex);
				if (Diff <= 1 || Diff == RingSize - 1) { continue; }
			}
			
			const UE::Geometry::FSegment2d SegA(Edges[i].Start, Edges[i].End);
			const UE::Geometry::FSegment2d SegB(Edges[j].Start, Edges[j].End);
			if (SegA.Intersects(SegB, TMathUtil<double>::Epsilon, Tolerance))
			{
				return true;
			}
		}
	}
	return false;
}

bool FPolygon2D::Triangulate(FPolygonTriangulation2D& OutMesh) const
{
	OutMesh.Vertices.Reset();
	OutMesh.Indices.Reset();
	
	if (!IsValid()) { return false; }
	
	UE::Geometry::FConstrainedDelaunay2d Delaunay;
	Delaunay.FillRule = UE::Geometry::FConstrainedDelaunay2d::EFillRule::Positive;
	Delaunay.Add(ToGeneralPolygon());
	
	if (!Delaunay.Triangulate()) { return false; }
	
	OutMesh.Vertices = Delaunay.Vertices;
	OutMesh.Indices.Reserve(Delaunay.Triangles.Num() * 3);
	for (const UE::Geometry::FIndex3i& T : Delaunay.Triangles)
	{
		OutMesh.Indices.Add(T.A);
		OutMesh.Indices.Add(T.B);
		OutMesh.Indices.Add(T.C);
	}
	
	/** 
	 * FConstrainedDelaunay2d's output winding is not a documented contract 
	 * (empirically CW in UE5.8, and it may vary with version / FillRule / presence of holes). 
	 * 
	 * Here we normalize per-triangle so that FPolygonTriangulation2D's CCW contract holds and
	 * downstream code does not need to guess.
	 */
	OutMesh.EnforceWinding(EPolyWinding::CounterClockwise);
	
	return OutMesh.IsValid();
}

void FPolygon2D::ToPoints3D(TArray<FVector>& OutPoints, double Z) const
{
	OutPoints.Reset(Outer.NumVertices());
	for (const FVector2D& V : Outer.Vertices)
	{
		OutPoints.Add(FVector(V.X, V.Y, Z));
	}
}

UE::Geometry::FGeneralPolygon2d FPolygon2D::ToGeneralPolygon() const
{
	// Ring to UE Polygon2d
	UE::Geometry::FGeneralPolygon2d GP(RingToPolygon2d(Outer));
	for (const FPolyRing& H : Holes)
	{
		GP.AddHole(RingToPolygon2d(H), 
			false, false);
	}
	return GP;
}

FPolygon2D FPolygon2D::FromGeneralPolygon(const UE::Geometry::FGeneralPolygon2d& In)
{
	FPolygon2D Out;
	Out.Outer = Polygon2dToRing(In.GetOuter());
	for (const UE::Geometry::FPolygon2d& H : In.GetHoles())
	{
		Out.Holes.Add(Polygon2dToRing(H));
	}
	Out.Normalize();
	return Out;
}

void FPolygon2D::FromGeneralPolygons(
	TArrayView<const UE::Geometry::FGeneralPolygon2d> In, TArray<FPolygon2D>& Out)
{
	Out.Reset(In.Num());
	for (const UE::Geometry::FGeneralPolygon2d& GP : In)
	{
		FPolygon2D P = FromGeneralPolygon(GP);
		if (P.IsValid()) { Out.Add(MoveTemp(P)); }
	}
}

bool FPolygon2D::operator==(const FPolygon2D& Other) const
{
	return ComputeStableHash() == Other.ComputeStableHash();
}

uint64 FPolygon2D::ComputeStableHash() const
{
	// Quantize to a 0.01 cm grid before hashing, eliminating floating-point jitter -> 
	// usable for golden tests
	auto Quantize = [](double V) -> int64
	{
		return FMath::RoundToInt64(V / ProcCityGeometry::PositionTolerance);
	};
	
	uint64 H = 1469598103934665603ull; // FNV-1a offset basis
	auto Mix = [&H](uint64 V)
	{
		H ^= V;
		H *= 1099511628211ull;
	};
	
	auto HashRing = [&](const FPolyRing& Ring)
	{
		Mix(static_cast<uint64>(Ring.Vertices.Num()));
		for (const FVector2D& V : Ring.Vertices)
		{
			Mix(Quantize(V.X));
			Mix(Quantize(V.Y));
		}
	};
	
	HashRing(Outer);
	Mix(static_cast<int64>(Holes.Num()));
	for (const FPolyRing& Hole : Holes) { HashRing(Hole); }
	
	return H;
}

uint32 GetTypeHash(const FPolygon2D& Poly)
{
	return GetTypeHash(Poly.ComputeStableHash());
}

FString FPolygon2D::ToString() const
{
	return FString::Printf(
		TEXT("FPolygon2D[Outer Vertices Num=%d, Holes=%d, Area=%.2f, Bounds=%s]"),
		Outer.NumVertices(), 
		Holes.Num(), 
		Area(), 
		*Bounds().ToString()
	);
}

// ============== FOrientedBox2D ==============
void FOrientedBox2D::GetCorners(TArray<FVector2D>& OutCorners) const
{
	OutCorners.Reset(4);
	const FVector2D AX = AxisX() * Extent.X;
	const FVector2D AY = AxisY() * Extent.Y;
	OutCorners.Add(Center - AX - AY);
	OutCorners.Add(Center + AX - AY);
	OutCorners.Add(Center + AX + AY);
	OutCorners.Add(Center - AX + AY);
}




