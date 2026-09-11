#pragma once

#include "CoreMinimal.h"
#include "Polygon2D.h"

/** -------------------------------------------------------------------------------------------
 * The single convention boundary between ProcCity 2D geometry and the UE 3D world.
 *
 * ==== Convention declarations (all upper-layer code depends on these; do not change) ====
 *
 * 1. FPolygon2D uses "algebraic CCW": the shoelace signed area is positive.
 *    This is independent of handedness and is a purely numerical convention.
 *
 * 2. Mapping to the world: Poly.X -> World.X, Poly.Y -> World.Y, Z is provided by the caller.
 *
 * 3. Visual note: because the UE top view (screen right = +Y, screen up = +X) is mirrored
 *    relative to the paper coordinate system, an algebraically CCW polygon appears clockwise
 *    in the editor top view. This is expected behavior, not a bug.
 *
 * 4. Rendering winding: UE treats "numerically CW" as front-facing. Therefore, indices obtained
 *    from triangulating a CCW polygon must be flipped so that face normals point toward +Z.
 *    The Emit* functions in this file handle this uniformly.
 *
 * 5. Rotation sign: FOrientedBox2D::Rotation has the same sign and meaning as FRotator::Yaw,
 *    and can be used directly after FMath::RadiansToDegrees without negation.
 ------------------------------------------------------------------------------------------- */  

namespace ProcCityGeometry
{
	enum class ESurfaceFacing : uint8
	{
		// Facing +Z (ground, road surface, roof, sidewalk) —— the vast majority of cases
		Up,
		// Facing -Z (ceilings, top surfaces of underpass structures) 
		Down
	};
	
	/** 
	 * Expands FPolygonTriangulation2D into data that can be fed directly to
	 * UStaticMesh / UDynamicMesh / ProceduralMeshComponent.
	 * Index winding has already been handled according to the UE front-facing convention.
	*/
	
	struct PROCCITYGEOMETRY_API FSurfaceMeshData
	{
		TArray<FVector>   Positions;
		TArray<FVector>   Normals;
		TArray<FVector2f> UVs;      // World-aligned UVs; unit controlled by UvScale
		TArray<int32>     Indices;

		int32 NumTriangles() const { return Indices.Num() / 3; }
		bool  IsValid() const
		{
			return Indices.Num() >= 3 && Indices.Num() % 3 == 0;
		}
	};
	
	/** 
	 * Triangulates and converts to UE mesh data.
	 * @param Z Plane height (cm)
	 * @param UvScale How many cm correspond to one UV unit; typically 100.0 (one cell per 1 m)
	*/
	PROCCITYGEOMETRY_API bool BuildSurfaceMesh(
		const FPolygon2D& Polygon,
		double Z,
		ESurfaceFacing Facing,
		double UvScale,
		FSurfaceMeshData& OutMesh);
	
	// Conversion overload when a triangulation result already exists (avoids 
	// re-triangulating).
	PROCCITYGEOMETRY_API bool BuildSurfaceMesh(
		const FPolygonTriangulation2D& Tri,
		double Z,
		ESurfaceFacing Facing,
		double UvScale,
		FSurfaceMeshData& OutMesh);
	
	// Exposes winding flip separately, for reuse with existing index arrays
	PROCCITYGEOMETRY_API void FlipTriangleWinding(TArray<int32>& Indices);
	
	/**
	 * Constructs a world transform for modules attached to a polygon edge (walls, railings, curbs). 
	 * 
	 * Resulting axes:
	 * +X = edge outward normal (Edge.Normal) —— the module's "front" faces outward
	 * +Y = edge direction (Edge.Direction()) —— along the CCW traversal direction
	 * +Z = world up
	 * Because UE has Y = Z x X, the above three axes exactly form a valid FRotationMatrix::MakeFromXZ.
	 * Meaning: during facade packing, the direction of increasing parameter t along the edge == the 
	 * direction of increasing module-local +Y.
	 * 
	 * @param AlongT Normalized position along the edge [0,1]
	*/
	PROCCITYGEOMETRY_API FTransform MakeEdgeTransform(
		const FPolyEdge& Edge, double AlongT, double Z);
	
	// Constructs a world transform from an OBB: +X along the long axis, no scaling
	PROCCITYGEOMETRY_API FTransform MakeBoxTransform(
		const FOrientedBox2D& Box, double Z);
	
	// 2D outward normal -> world Yaw (degrees), can be placed directly into FRotator
	FORCEINLINE double NormalToYawDegrees(const FVector2D& Normal2D)
	{
		return FMath::RadiansToDegrees(FMath::Atan2(Normal2D.Y, Normal2D.X));
	}
	
}
