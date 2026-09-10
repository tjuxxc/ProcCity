#pragma once

#include "CoreMinimal.h"
#include "ProcCityGeometryTypes.h"
#include "Polygon2D.generated.h"

// Forward declarations of GeometryCore types to avoid polluting the public header
namespace UE::Geometry
{
	template<typename T> class TPolygon2;
	using FPolygon2d = TPolygon2<double>;
	
	template<typename T> class TGeneralPolygon2; 
	using FGeneralPolygon2d = TGeneralPolygon2<double>;
}

USTRUCT(BlueprintType)
struct PROCCITYGEOMETRY_API FPolySimplifyParams
{
	GENERATED_BODY()
	
	// Merge distance for adjacent points (cm). <=0 means skip deduplication
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	double MergeDistance = ProcCityGeometry::PositionTolerance;
	
	// Maximum perpendicular distance from a vertex to the line through its 
	// two neighbors (cm). <=0 disables the distance criterion
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	double MaxDeviation = ProcCityGeometry::PositionTolerance;
	
	// Upper limit on the direction angle between adjacent edges (rad). 
	// <=0 disables the angle criterion
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	double MaxAngle = 0.0;
	
	// Minimum number of vertices to keep, preventing the polygon from being simplified away
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	int32 MinVertices = 3;
	
	// Default: only cleans up floating-point noise. Does not change shape semantics
	static FPolySimplifyParams Cleanup()
	{
		return FPolySimplifyParams{};
	}
	
	// Used for layout input decimation: combined distance + angle criteria
	static FPolySimplifyParams Coarse(
		double InMaxDeviation = 2.0, double InMaxAngleRad = 0.02)
	{
		FPolySimplifyParams Params;
		Params.MaxDeviation = InMaxDeviation;
		Params.MaxAngle = InMaxAngleRad;
		
		return Params;
	}
};


/** A closed ring (first and last vertices are not duplicated) */
USTRUCT(BlueprintType)
struct PROCCITYGEOMETRY_API FPolyRing
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	TArray<FVector2D> Vertices;
	
	int32 NumVertices() const
	{
		return Vertices.Num();
	}
	
	bool IsEmpty() const
	{
		return Vertices.Num() < 3;
	}
	
	//  Signed area: positive for CCW
	double SignedArea() const;
	EPolyWinding GetWinding() const;
	void EnsureWinding(EPolyWinding Desired);
	
	void Reverse()
	{
		Algo::Reverse(Vertices);
	}
	
	// Removes duplicate and collinear vertices 
	void Simplify(const FPolySimplifyParams& Params = FPolySimplifyParams::Cleanup());

	FBox2D Bounds() const;
	double Perimeter() const;
	
};

/** 
 * ProcCity's core 2D spatial primitive: a polygon with holes.
 * Conventions (all APIs maintain these invariants):
 * Outer is always CCW (positive area)
 * Holes are always CW (negative area)
 * Units are cm, located on the XY plane
 * Implementation strategy: this class is a thin USTRUCT wrapper around UE::Geometry::FGeneralPolygon2d.
 * Heavy lifting (offset / boolean / triangulation) is all delegated to GeometryCore/GeometryAlgorithms.
*/

USTRUCT(BlueprintType)
struct PROCCITYGEOMETRY_API FPolygon2D
{
	GENERATED_BODY()
	
	FPolygon2D() = default;
	explicit FPolygon2D(const TArray<FVector2D>& InOuter);
	explicit FPolygon2D(TArray<FVector2D>&& InOuter);
	
	// ---- Data
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	FPolyRing Outer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	TArray<FPolyRing> Holes;
	
	// ---- Construction factories
	/** 
	 * Axis-aligned / rotated rectangle. Used for compatibility with Center+Extent+Yaw 
	 * descriptions in existing JSON.
	 * @param YawDegrees Rotation angle around the Z axis, in degrees
	*/
	static FPolygon2D MakeRect(const FVector2D& Center, 
		const FVector2D& Extent, double YawDegrees = 0.0);
	
	static FPolygon2D MakeFromBox(const FBox2D& Box);
	static FPolygon2D MakeFromOrientedBox(const FOrientedBox2D& Box);
	
	// Regular polygon, used for debug and circular plazas
	static FPolygon2D MakeRegular(const FVector2D& Center, 
		double Radius, int32 NumSides, double StartAngleRad = 0.0);
	
	// Convex hull; returns an empty polygon for degenerate input 
	static FPolygon2D MakeConvexHull(TArrayView<const FVector2D> Points);
	
	// ---- Validity
	bool IsValid() const;
	bool IsEmpty() const
	{
		return Outer.IsEmpty();
	}
	
	void Reset()
	{
		Outer.Vertices.Reset(); Holes.Reset();
	}
	
	//  Enforces the winding convention; should be called after any external construction
	void Normalize();
	
	/**
	 * Self-intersection detection. Offset/subdivide in city generation may 
	 * produce self-intersections.
	 * Upper layers should assert this function returns false at critical checkpoints.
	*/
	bool HasSelfIntersection(
		double Tolerance = ProcCityGeometry::PositionTolerance) const;
	
	// ----- Metrics
	
	// Net area = outer ring area - sum of all hole areas. Always non-negative
	double Area() const;
	
	// Area-weighted centroid (accounting for holes). Returns ZeroVector 
	// for an empty polygon
	FVector2D Centroid() const;
	
	// Outer ring perimeter (excluding holes) 
	double Perimeter() const;
	
	FBox2D Bounds() const;
	
	/** 
	 * Minimum-area oriented bounding box (convex hull + rotating calipers).
	 * Used for: cutting direction in recursive lot subdivision, building primary 
	 * axis alignment, prop grid alignment.
	 * Guarantees Extent.X >= Extent.Y.
	*/
	FOrientedBox2D MinAreaOBB() const;
	
	// Compactness = 4πA / P². Circle compactness is 1, compactness of an 
	// elongated sliver approaches 0. 
	// Used to filter out fragmented parcels unsuitable for buildings
	double Compactness() const;
	
	// ---- Queries
	
	// Point-in-polygon test (with hole check). 
	// Return value is undefined for points on the boundary; 
	// use DistanceToBoundary if exact classification is needed
	bool Contains(const FVector2D& Point) const;
	
	bool Contains(const FPolygon2D& Other) const;
	
	// Shortest distance to the boundary; positive inside, negative outside (signed distance)
	double SignedDistanceToBoundary(const FVector2D& Point) const;
	
	FVector2D ClosestPointOnBoundary(const FVector2D& Point, 
		int32* OutRingIndex = nullptr, int32* OutEdgeIndex = nullptr) const;
	
	// ---- Edges
	
	// Collects all edges (outer ring + holes). 
	// This is the entry point for frontage annotation and facade packing
	void GetEdges(TArray<FPolyEdge>& OutEdges) const;
	
	// Outer ring edges only
	void GetOuterEdges(TArray<FPolyEdge>& OutEdges) const;
	// The i-th outer ring edge
	FPolyEdge GetOuterEdge(int32 EdgeIndex) const;
	
	int32 NumOuterEdges() const { return Outer.NumVertices(); }
	
	void Simplify(const FPolySimplifyParams& Params = FPolySimplifyParams::Cleanup());
	
	/** Merges adjacent nearly-collinear edges. 
	 * City layouts often receive overly dense vertices from PCG/JSON.
	 * This step significantly reduces the number of fragments in subsequent facade packing.
	 * @param MaxAngleRad Adjacent edges with direction angle difference below 
	 * this value are merged
	*/
	void MergeCollinearEdges(double MaxDeviation = 2.0, double MaxAngleRad = 0.02)
	{
		Simplify(FPolySimplifyParams::Coarse(MaxDeviation, MaxAngleRad));
	}
	
	
	// ---- Transform & triangulation
	
	void Translate(const FVector2D& Delta);
	void Rotate(double AngleRad, const FVector2D& Pivot = FVector2D::ZeroVector);
	void Scale(double Factor, const FVector2D& Pivot = FVector2D::ZeroVector);
	
	/**
	 * General 2D affine transform. If Xf contains a mirror (determinant < 0), the winding flips,
	 * and Normalize is called internally to maintain the "outer ring CCW / holes CW" invariant.
	*/
	void Transform(const FTransform2d& Xf);
	
	// Non-destructive version, convenient for chaining
	// Get transform of copy of *this
	FPolygon2D GetTransformed(const FTransform2d& Xf) const;
	
	// Triangulation with hole support (internally uses ConstrainedDelaunay2)
	bool Triangulate(FPolygonTriangulation2D& OutMesh) const;
	
	// Project to 3D (Z = constant or given by a height function), used to 
	// generate the ground surface
	void ToPoints3D(TArray<FVector>& OutPoints, double Z = 0.0) const;
	
	// ---- GeometryCore interop (internal)
	UE::Geometry::FGeneralPolygon2d ToGeneralPolygon() const;
	static FPolygon2D FromGeneralPolygon(const UE::Geometry::FGeneralPolygon2d& In);
	
	static void FromGeneralPolygons(
		TArrayView<const UE::Geometry::FGeneralPolygon2d> In, 
		TArray<FPolygon2D>& Out
	);
	
	// ---- Serialization
	bool operator==(const FPolygon2D& Other) const;
	bool operator!=(const FPolygon2D& Other) const
	{
		return !(*this == Other);
	}
	
	friend PROCCITYGEOMETRY_API uint32 GetTypeHash(const FPolygon2D& Poly);
	
	// Deterministic hash for golden tests (quantized to the tolerance grid to 
	// avoid floating-point jitter)
	uint64 ComputeStableHash() const;
	
	FString ToString() const;
};