#include "ProcCityGeometry/PolygonOffset.h"

#include "Curve/GeneralPolygon2.h"
#include "Curve/PolygonIntersectionUtils.h"
#include "Curve/PolygonOffsetUtils.h"

using namespace UE::Geometry;

namespace
{
	using EUEJoinType = UE::Geometry::EPolygonOffsetJoinType;
	using EUEEndType  = UE::Geometry::EPolygonOffsetEndType;
	
	EUEJoinType ToUEJoinType(EPolyJoinType In)
	{
		switch (In)
		{
		case EPolyJoinType::Round:  return EUEJoinType::Round;
		case EPolyJoinType::Square: return EUEJoinType::Square;
		default:                    return EUEJoinType::Miter;
		}
	}
	
	void ToUEGeneralPolygon2dArray(TArrayView<const FPolygon2D> In, 
		TArray<FGeneralPolygon2d>& Out)
	{
		Out.Reset(In.Num());
		for (const FPolygon2D& P : In)
		{
			if (P.IsValid()) { Out.Add(P.ToGeneralPolygon()); }
		}
	}
	
	void FinalizeResults(TArray<FGeneralPolygon2d>& Raw, double MinArea, 
		bool bSimplify, TArray<FPolygon2D>& Out)
	{
		Out.Reset();
		for (const FGeneralPolygon2d& GP : Raw)
		{
			FPolygon2D P = FPolygon2D::FromGeneralPolygon(GP);
			if (bSimplify)
			{
				P.Simplify(FPolySimplifyParams::Cleanup());
			}
			if (P.IsValid() && P.Area() >= MinArea)
			{
				Out.Add(MoveTemp(P));
			}
		}
		// Determinism: sort by descending area. 
		// Clipper's output order is not guaranteed stable, while golden tests 
		// and seed derivation both depend on reproducible order.
		Out.Sort(
			[](const FPolygon2D& A, const FPolygon2D& B)
			{return A.Area() > B.Area();});
	}
}

namespace ProcCityGeometry
{
	bool OffsetPolygons(TArrayView<const FPolygon2D> In, 
		const FPolygonOffsetParams& Params, TArray<FPolygon2D>& Out)
	{
		Out.Reset();
		if (In.Num() == 0) { return false; }
		
		// Delta approximately 0: pass through directly, avoiding 
		// Clipper's unnecessary rebuild and precision loss
		if (FMath::Abs(Params.Delta) < PositionTolerance)
		{
			for (const FPolygon2D& P : In)
			{
				if (P.IsValid() && P.Area() >= Params.MinResultArea)
				{
					Out.Add(P);
				}
				Out.Sort([](
					const FPolygon2D& A, const FPolygon2D& B)
					{ return A.Area() > B.Area(); });
				return Out.Num() > 0;
			}
		}
		
		TArray<FGeneralPolygon2d> Input;
		Input.Reserve(In.Num());
		for (const FPolygon2D& P : In)
		{
			if (P.IsValid())
			{
				Input.Add(P.ToGeneralPolygon());
			}
		}
		if (Input.Num() == 0) { return false; }
		
		TArray<FGeneralPolygon2d> Raw;
		const bool bSuccess = PolygonsOffset(
			Params.Delta,
			Input,
			Raw,
			false,
			Params.MiterLimit,
			ToUEJoinType(Params.JoinType),
			EUEEndType::Polygon,
			Params.ArcTolerance);
		
		if (!bSuccess) { return false; }
		
		FinalizeResults(Raw, 
			Params.MinResultArea, Params.bSimplifyResult, Out);
		
		return Out.Num() > 0;
	}
	
	bool OffsetPolygon(const FPolygon2D& In, 
		const FPolygonOffsetParams& Params, TArray<FPolygon2D>& Out)
	{
		return OffsetPolygons(
			MakeArrayView(&In, 1), Params, Out);
	}
	
	bool OffsetPolygonSingle(const FPolygon2D& In, 
		const FPolygonOffsetParams& Params, FPolygon2D& Out)
	{
		TArray<FPolygon2D> Results;
		if (!OffsetPolygon(In, Params, Results)) { return false; }
		
		// Polygon has been sorted descent with area
		Out = MoveTemp(Results[0]); 
		return true;
	}
	
	bool InsetPolygonPerEdge(
		const FPolygon2D& In, 
		TArrayView<const double> PerEdgeInset, 
		double MinResultArea, 
		TArray<FPolygon2D>& Out)
	{
		Out.Reset();
		if (!In.IsValid()) { return false; }
		if (PerEdgeInset.Num() != In.NumOuterEdges())
		{
			UE_LOG(LogTemp, Warning,
				   TEXT("InsetPolygonPerEdge: inset count %d != edge count %d"),
				   PerEdgeInset.Num(), In.NumOuterEdges());
			return false;
		}
		
		/**
		 * Strategy: start from the original polygon and, for each edge with a non-zero  
		 * setback, clip the half-plane by its inward-shifted line.  
		 * 
		 * For convex polygons this is equivalent to an exact straight-skeleton offset;
		 * 
		 * for concave polygons the result is conservative (possibly slightly smaller than 
		 * the theoretical value), which is the safe direction for setback semantics.
		 */
		TArray<FPolygon2D> Current;
		Current.Add(In);
		
		TArray<FPolyEdge> Edges;
		In.GetOuterEdges(Edges);
		
		for (int32 i = 0; i < Edges.Num(); ++i)
		{
			const double Inset = PerEdgeInset[i];
			if (FMath::Abs(Inset) < PositionTolerance) { continue; }
			
			const FPolyEdge& E = Edges[i];
			if (E.Normal.IsNearlyZero()) { continue; }
			
			/**
			 * Inset along the inward normal direction; retain the part on the inner 
			 * side (Dot(-N, P) >= Dot(-N,Start) + Inset)
			 * 
			 * "(Dot(-N, P) = Dot(-N,Start) + Inset" define the (candidate) split line
			*/ 
			const FVector2D InwardN = -E.Normal;
			
			const double Offset = 
				FVector2D::DotProduct(InwardN, E.Start) + Inset;
			
			TArray<FPolygon2D> Next;
			for (const FPolygon2D& Poly : Current)
			{
				TArray<FPolygon2D> Keep, Discard;
				
				// When SplitByLine returns false, Keep is still valid:
				// - If the whole piece is on the keep side, Keep contains the whole piece; 
				// - if the whole piece is on the discard side, Keep is empty
				SplitByLine(Poly, InwardN, Offset, 
					Keep, Discard);
				Next.Append(MoveTemp(Keep));
			}
			Current = MoveTemp(Next);
			// Setback too large, the parcel disappears
			if (Current.Num() == 0) { return false; }   
		}
		
		for (FPolygon2D& P : Current)
		{
			P.Simplify(FPolySimplifyParams::Cleanup());
			if (P.IsValid() && P.Area() >= MinResultArea)
			{
				Out.Add(MoveTemp(P));
			}
		}
		Out.Sort([](const FPolygon2D& A, const FPolygon2D& B)
		{
			return A.Area() > B.Area();
		});
		return Out.Num() > 0;
	}
	
	bool SplitByLine(const FPolygon2D& In, const FVector2D& LineNormal, 
		double LineOffset, TArray<FPolygon2D>& OutPositive, 
		TArray<FPolygon2D>& OutNegative)
	{
		OutPositive.Reset();
		OutNegative.Reset();
		if (!In.IsValid()) { return false; }
		

		// Normalized splitting line function: Dot(N, P) = Offset
		const double NormalLen = LineNormal.Size();
		if (NormalLen < UE_KINDA_SMALL_NUMBER) { return false; }
		const FVector2D N = LineNormal / NormalLen;
		const double Offset = LineOffset / NormalLen;
		
		{
			double MinProj = TNumericLimits<double>::Max();
			double MaxProj = -MinProj;
			for (const FVector2D& V : In.Outer.Vertices)
			{
				// Signed distance of vertex to splitting line
				const double Proj = FVector2D::DotProduct(N, V) - Offset;
				MinProj = FMath::Min(MinProj, Proj);
				MaxProj = FMath::Max(MaxProj, Proj);
			}
			
			// Whole polygon is at positive side 
			if (MinProj >= -PositionTolerance)         
			{
				OutPositive.Add(In);
				return false;
			}
			// Whole polygon is at negative side 
			if (MaxProj <= PositionTolerance)           
			{
				OutNegative.Add(In);
				return false;
			}
		}
		
		const FBox2D B = In.Bounds();
		const FVector2D BoxCenter = B.GetCenter();
		const FVector2D Tangent(-N.Y, N.X);
		
		// Signed distance from box center to splitting line
		const double SignedDist = 
			FVector2D::DotProduct(N, BoxCenter) - Offset;
		// Projection of box center on splitting line
		const FVector2D Anchor = BoxCenter - N * SignedDist;
		// Debug check: Anchor must on splitting line
		checkSlow(FMath::Abs(
			FVector2D::DotProduct(N, Anchor) - Offset) < 1.0);
		
		// Half plane radius. Large enough to cover positive or 
		// negative part of polygon 
		const double R = B.GetExtent().Size() * 2 + 1000.0;

		auto MakeHalfPlane = [&](double Sign) -> FPolygon2D
		{
			// CCW Rect: half-plane of sign direction
			const FVector2D P0 = Anchor - Tangent * R;
			const FVector2D P1 = Anchor + Tangent * R;
			const FVector2D Push = N * (Sign * R);
			FPolygon2D HP;
			HP.Outer.Vertices = { P0, P1, P1 + Push, P0 + Push };
			HP.Normalize();
			return HP;
		};
		
		const FPolygon2D Pos = MakeHalfPlane(+1.0);
		const FPolygon2D Neg = MakeHalfPlane(-1.0);
		
		IntersectPolygons(MakeArrayView(&In, 1), 
			MakeArrayView(&Pos, 1), OutPositive);
		
		IntersectPolygons(MakeArrayView(&In, 1), 
			MakeArrayView(&Neg, 1), OutNegative);
		
		return OutPositive.Num() > 0 && OutNegative.Num() > 0;
	}
	
	bool UnionPolygons(TArrayView<const FPolygon2D> InA, 
		TArrayView<const FPolygon2D> InB, TArray<FPolygon2D>& Out)
	{
		TArray<FGeneralPolygon2d> GA, GB, Raw;
		ToUEGeneralPolygon2dArray(InA, GA);
		ToUEGeneralPolygon2dArray(InB, GB);
		
		if (GA.Num() == 0 && GB.Num() == 0) 
		{
			Out.Reset(); 
			return false;
		}
		
		// TODO: Efficiency?
		TArray<FGeneralPolygon2d> Combined;
		Combined.Reserve(GA.Num() + GB.Num()); 
		Combined.Append(MoveTemp(GA));
		Combined.Append(MoveTemp(GB));
		
		if (!PolygonsUnion(Combined, Raw, false))
		{
			return false;
		}
		
		FinalizeResults(Raw, AreaTolerance, true, Out);
		return Out.Num() > 0;
	}
	
	bool IntersectPolygons(TArrayView<const FPolygon2D> SubjPolygons, 
		TArrayView<const FPolygon2D> ClipPolygon, TArray<FPolygon2D>& Out)
	{
		TArray<FGeneralPolygon2d> GSubjPolygons, GClipPolygon, Raw;
		ToUEGeneralPolygon2dArray(SubjPolygons, GSubjPolygons);
		ToUEGeneralPolygon2dArray(ClipPolygon, GClipPolygon);
		
		if (GSubjPolygons.Num() == 0 || GClipPolygon.Num() == 0)
		{
			Out.Reset(); return false;
		}
		
		if (!PolygonsIntersection(GSubjPolygons, GClipPolygon, Raw))
		{
			return false;
		}
		
		// TODO: Not AreaTolerance?
		FinalizeResults(Raw, AreaTolerance, true, Out);
		return Out.Num() > 0;
	}
	
	bool SubtractPolygons(TArrayView<const FPolygon2D> PosPolygons, 
		TArrayView<const FPolygon2D> NegPolygon, TArray<FPolygon2D>& Out)
	{
		TArray<FGeneralPolygon2d> GPosPolygons, GNegPolygon, Raw;
		ToUEGeneralPolygon2dArray(PosPolygons, GPosPolygons);
		ToUEGeneralPolygon2dArray(NegPolygon, GNegPolygon);
		if (GPosPolygons.Num() == 0) { Out.Reset(); return false; }
		if (GNegPolygon.Num() == 0)
		{
			Out.Reset(PosPolygons.Num());
			Out.Append(PosPolygons.GetData(), PosPolygons.Num());
			return Out.Num() > 0;
		}
		
		if (!PolygonsDifference(GPosPolygons, GNegPolygon, Raw))
		{
			return false;
		}

		FinalizeResults(Raw, AreaTolerance, true, Out);
		return Out.Num() > 0;
	}
	
	bool SplitAlongLongestAxis(const FPolygon2D& In, double SplitRatio, 
		TArray<FPolygon2D>& OutPos, TArray<FPolygon2D>& OutNeg)
	{
		OutPos.Reset();
		OutNeg.Reset();
		if (!In.IsValid()) { return false; }
		
		const FOrientedBox2D OBB = In.MinAreaOBB();
		// MinAreaOBB ensures longest axis == AxisX.
		// Assume N, i.e., AxisX() is normalized
		const FVector2D N = OBB.AxisX();
		const double T = FMath::Clamp(SplitRatio, 0.05, 0.95);
		const double LocalX = FMath::Lerp(-OBB.Extent.X, OBB.Extent.X, T);
		const FVector2D PointOnLine = OBB.Center + N * LocalX;
		
		const double Offset = FVector2D::DotProduct(N, PointOnLine);
		
		return SplitByLine(In, N, Offset, OutPos, OutNeg);
	}
} 