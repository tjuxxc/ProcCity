#pragma once

#include "CoreMinimal.h"
#include "Polygon2D.h"
#include "PolygonOffset.generated.h"

/** How outer corners are joined */
UENUM(BlueprintType)
enum class EPolyJoinType : uint8
{
	// Sharp corner; degenerates to Square when MiterLimit is exceeded. 
	// Used for building setbacks and parcel red lines
	Miter,
	
	// Rounded corner, tessellated according to ArcTolerance. 
	// Used for vegetation belts and park boundaries 
	Round,
	
	// Beveled 
	Square
};

/** How open path endpoints are capped (only used by OffsetOpenPath) */
UENUM(BlueprintType)
enum class EPolyEndType : uint8
{
	// Flat, stops at the endpoint
	Butt,
	// Square extension by Delta
	Square,
	// Semicircle
	Round
};

USTRUCT(BlueprintType)
struct PROCCITYGEOMETRY_API FPolygonOffsetParams
{
	GENERATED_BODY()
	
	// Positive -> Outset; Negative -> Inset
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	double Delta = 0.0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	EPolyJoinType JoinType = EPolyJoinType::Miter;
	
	// Miter limit; when a sharp corner exceeds Delta*MiterLimit it degenerates to Square
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	double MiterLimit = 2.0;
	
	// Maximum chord height error for Round (cm) 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	double ArcTolerance = 2.0;
	
	// Fragments with an area smaller than this are discarded directly (cm^2). 
	// Default 100 = 10cm x 10cm
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	double MinResultArea = 100.0;
	
	// Perform a Cleanup-level simplification before output
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	bool bSimplifyResult = true;
	
	static FPolygonOffsetParams Inset(double Distance)
	{
		FPolygonOffsetParams Params;
		Params.Delta = -FMath::Abs(Distance);
		return Params;
	}
	
	static FPolygonOffsetParams Outset(double Distance)
	{
		FPolygonOffsetParams Params; 
		Params.Delta = FMath::Abs(Distance); 
		return Params;
	}
	
	static FPolygonOffsetParams Rounded(
		double Distance, double InArcTolerance = 2.0)
	{
		FPolygonOffsetParams Params;
		Params.Delta = Distance;
		Params.JoinType = EPolyJoinType::Round;
		Params.ArcTolerance = InArcTolerance;
		return Params;
	}
};

/** 
 * Polygon offset and boolean operations. 
 * 
 * ==== Contract ==== 
 * All output FPolygon2D are already Normalized (outer ring CCW / holes CW), 
 * and fragments with area < MinResultArea have already been culled. 
 * 
 * Output is a TArray: inward offset may cut a concave polygon into multiple 
 * pieces, and outward offset may merge multiple pieces into one. 
 * 
 * Callers must handle the 0-result and N-result cases —— this is the most error-prone 
 * part of extending from rect to general polygons (in the rect era, inset always 
 * returned exactly 1 result (TODO: Why?)).
*/

namespace ProcCityGeometry
{
	// Uniform offset
	PROCCITYGEOMETRY_API bool OffsetPolygon(
		const FPolygon2D& In, 
		const FPolygonOffsetParams& Params, 
		TArray<FPolygon2D>& Out);
	
	// Convenience overload: only wants a single largest-area result, returns 
	// false if there is no solution. Suitable for "must obtain a single parcel" scenarios
	PROCCITYGEOMETRY_API 
	bool OffsetPolygonSingle(const FPolygon2D& In, 
		const FPolygonOffsetParams& Params, FPolygon2D& Out);
	
	// Batch offset (solved internally as a merged whole, correctly handling 
	// mutually overlapping inputs)
	PROCCITYGEOMETRY_API
	bool OffsetPolygons(TArrayView<const FPolygon2D> In, 
		const FPolygonOffsetParams& Params, TArray<FPolygon2D>& Out);
	
	/** 
	 * Offset with per-edge setback distances (a core requirement of urban planning).
	 * 
	 * Typical usage: 6 m setback along arterial roads, 3 m along secondary roads, 
	 * no setback on interior edges.
	 * 
	 * Implementation: construct the inner half-plane for each edge and intersect the 
	 * half-planes (exact for convex cases; for concave cases, split by edge first and 
	 * then intersect, with conservative results).
	 * 
	 * @param PerEdgeInset Length must equal In.NumOuterEdges(); positive values mean inward setback
	*/
	PROCCITYGEOMETRY_API bool InsetPolygonPerEdge(
		const FPolygon2D& In, TArrayView<const double> PerEdgeInset,
		double MinResultArea, TArray<FPolygon2D>& Out);
	
	PROCCITYGEOMETRY_API bool UnionPolygons(TArrayView<const FPolygon2D> InA, 
		TArrayView<const FPolygon2D> InB, TArray<FPolygon2D>& Out);
	
	PROCCITYGEOMETRY_API bool IntersectPolygons(
		TArrayView<const FPolygon2D> SubjPolygons, 
		TArrayView<const FPolygon2D> ClipPolygon, 
		TArray<FPolygon2D>& Out);
	
	PROCCITYGEOMETRY_API bool SubtractPolygons(
		TArrayView<const FPolygon2D> PosPolygons, 
		TArrayView<const FPolygon2D> NegPolygons, 
		TArray<FPolygon2D>& Out);


	/**
	 * Splits a polygon with a straight line (the fundamental operation of recursive lot subdivision).
	 * @param LineNormal Line normal (no normalization required)
	 * @param LineOffset Line equation Dot(N, P) = LineOffset
	 * @param OutPositive The side where Dot(N,P) >= LineOffset
	 */
	
	/**
	 * ==== Contract ====
	 * - Line equation: Dot(LineNormal, P) = LineOffset. LineNormal does not need to be normalized.
	 * - OutPositive collects the part where Dot(N,P) >= LineOffset, and OutNegative collects the other side.
	 * - If the line does not actually cross the polygon, the whole piece ends up in the 
	 *   corresponding side's array, the other side is empty, and the function returns false. 
	 *   Therefore **when it returns false the output arrays are still valid**, and the caller can
	 *   read them directly without needing to additionally determine which side they are on.
	 * - Returns true if and only if both sides have non-degenerate results.
	 * - All outputs satisfy the FPolygon2D contract and are sorted by descending area.
	 *
	 * Implementation note: internally, the projection of In's bounding-box center onto the line is 
	 * used as the anchor to construct a half-plane rectangle, so it holds for any input far from 
	 * the origin (In is not required to be near the origin).
	 */
	
	PROCCITYGEOMETRY_API bool SplitByLine(
		const FPolygon2D& In, 
		const FVector2D& LineNormal, 
		double LineOffset, 
		TArray<FPolygon2D>& OutPositive, 
		TArray<FPolygon2D>& OutNegative);
	
	// Cuts once along the OBB long axis through the centroid; returns 
	// whether it successfully split into two pieces
	PROCCITYGEOMETRY_API bool SplitAlongLongestAxis(
		const FPolygon2D& In, 
		double SplitRatio, 
		TArray<FPolygon2D>& OutPos, 
		TArray<FPolygon2D>& OutNeg);
	
}
