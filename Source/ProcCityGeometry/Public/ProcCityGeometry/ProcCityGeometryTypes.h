#pragma once

#include "CoreMinimal.h"
#include "ProcCityGeometryTypes.generated.h"

/*
 * Global geometric tolerance. Unit is UE world units (cm).
 * At city scale, coordinates can reach 1e6 cm (10 km), and double has about 15–16 significant digits,
 * so an absolute tolerance of 0.01 cm is safe across the entire city range.
*/

namespace ProcCityGeometry
{
	// Position tolerance in cm
	inline constexpr double PositionTolerance = 0.01;
	// Area tolerance in cm^2
	inline constexpr double AreaTolerance = 1.0;
	// Angle tolerance in rad (about 0.057 deg)
	inline constexpr double AngleTolerance = 1.0e-3;
	// Default maximum chord height error for curve tessellation (cm)
	inline constexpr double DefaultMaxChordError = 2.0;
}

/** Winding direction of a polygon ring */
UENUM(BlueprintType)
enum class EPolyWinding : uint8
{
	// Counterclockwise, positive area. ProcCity convention: outer rings are always CCW 
	CounterClockwise,
	// Clockwise, negative area. ProcCity convention: inner rings (holes) are always CW 
	Clockwise,
	// Degenerate: area near 0 or vertex count < 3 *
	Degenerate
};

/*
* A directed edge of a polygon. Produced by FPolygon2D::GetEdges. 
* This is the core structure consumed by upper layers (frontage annotation, 
* facade packing, sidewalk generation).
*/
USTRUCT(BlueprintType)
struct PROCCITYGEOMETRY_API FPolyEdge
{
	GENERATED_BODY()
	
	// Index of this edge within its owning ring 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	int32 EdgeIndex = INDEX_NONE;
	
	// Owning ring: 0 = outer ring; 1...N = holes 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	int32 RingIndex = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	FVector2D Start = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	FVector2D End = FVector2D::ZeroVector;
	
	// Outward normal (unit vector). For a CCW outer ring, it points to the outside of the polygon.
	// Building facade orientation and prop placement direction both depend on it.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	FVector2D Normal = FVector2D::ZeroVector;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	double Length = 0.0;
	
	FVector2D Direction() const
	{
		return Length > ProcCityGeometry::PositionTolerance
			? (End - Start) / Length
			: FVector2D::ZeroVector;
	}
	
	FVector2D Midpoint() const
	{
		return (Start + End) * 0.5;
	}
	
	// Sample a point along the edge parametrically, T ∈ [0,1]
	FVector2D PointAt(double T) const
	{
		return FMath::Lerp(Start, End, T);
	}
};

/** 2D oriented bounding box (output of minimum-area OBB) */
USTRUCT(BlueprintType)
struct PROCCITYGEOMETRY_API FOrientedBox2D
{
	GENERATED_BODY()
	
	// Center in world space
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	FVector2D Center = FVector2D::ZeroVector;
	
	// Half width and half height. Extent.X corresponds to the AxisX direction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	FVector2D Extent = FVector2D::ZeroVector;
	
	// Angle between the primary axis and the world X axis, in radians. 
	// Convention: Extent.X >= Extent.Y (long axis first)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProcCity|Geometry")
	double Rotation = 0.0;
	
	FVector2D AxisX() const
	{
		return FVector2D(FMath::Cos(Rotation), FMath::Sin(Rotation));
	}
	
	FVector2D AxisY() const
	{
		return FVector2D(-FMath::Sin(Rotation), FMath::Cos(Rotation));
	}
	
	double Area() const { return 4.0 * Extent.X * Extent.Y; }
	
	// Returns 4 corner points in CCW order, starting from the (-X,-Y) quadrant
	void GetCorners(TArray<FVector2D>& OutCorners) const;
	
	// Local coordinates -> world coordinates
	FVector2D LocalToWorld(const FVector2D& Local) const
	{
		return Center + AxisX() * Local.X + AxisY() * Local.Y;
	}
	
	// World coordinates -> local coordinates
	FVector2D WorldToLocal(const FVector2D& World) const
	{
		const FVector2D D = World - Center;
		return FVector2D(
			FVector2D::DotProduct(D, AxisX()), 
			FVector2D::DotProduct(D, AxisY())
		);
	}
};

/** Simple triangle mesh, triangulation output */
USTRUCT()
struct PROCCITYGEOMETRY_API FPolygonTriangulation2D
{
	GENERATED_BODY()

	UPROPERTY() TArray<FVector2D> Vertices;
	
	// Every 3 int32 form a triangle, CCW
	UPROPERTY() TArray<int32> Indices;

	int32 NumTriangles() const
	{
		return Indices.Num() / 3;
	}
	
	bool IsValid() const
	{
		return Indices.Num() >= 3 && Indices.Num() % 3 == 0;
	}
};