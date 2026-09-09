#pragma once

#include "CoreMinimal.h"
#include "BlockEnvironmentEnums.generated.h"

/*
 * Environment enums:
 * These describe environment execution semantics.
 */

UENUM(BlueprintType)
enum class EBlockEnvironmentElementType : uint8
{
	None = 0,
	Surface,
	PathSegment,
	PathTerminalTreatment,
	PathJunction,
	Prop
};

// ----------------- Environment Category -----------------
// Surface category used by environment plans.
UENUM(BlueprintType)
enum class EBlockSurfaceCategory : uint8
{
	None = 0,
	Ground,
	Reserve,
};

// ----------------- Ground Role -----------------
// Ground-specific semantic role for environment plan.
UENUM(BlueprintType)
enum class EBlockGroundRole : uint8
{
	None = 0,
	Base,
	Buildable
};

// ----------------- Block Feature Roles -----------------
UENUM(BlueprintType)
enum class EBlockPathRole : uint8
{
	None = 0,
	MainPath,
	Alley,
	ServiceLane,
	PedestrianPath,
	Sidewalk
};

UENUM(BlueprintType)
enum class EBlockReserveRole : uint8
{
	None = 0,
	FlexibleOpenSpace, 
	CornerPlaza,
	Green,
	UtilityPad,
	SetbackBuffer
};

UENUM(BlueprintType)
enum class EBlockPointRole : uint8
{
	None = 0,
	Tree,
	Light,
	Bench,
	Utility,
	StreetFurniture
};

// ----------------- Path junction types enum -----------------
UENUM(BlueprintType)
enum class EBlockPathJunctionType : uint8
{
	None = 0,
	EndCap,
	Corner,
	TJunction,
	Cross,
	Custom
};

// ----------------- Path end treatment enum -----------------
UENUM(BlueprintType)
enum class EBlockPathWidthSpec : uint8
{
	None = 0,

	W150,
	W200,
	W300,

	W400,
	W600,
	W800,
	
	W1000,
	W1200,
	W1600
};

UENUM(BlueprintType)
enum class EBlockPathHandedness : uint8
{
	None = 0,
	Default,
	Flipped
};

// TODO: Enum for debug. It may be removed in the future.
UENUM(BlueprintType)
enum class EResolvedEnvironmentInstanceType : uint8
{
	None = 0,
	Surface,
	Prop,
	PathSegment,
	PathTerminalTreatment,
	PathJunction
};

UENUM(BlueprintType)
enum class EBlockFeatureGeometryType : uint8
{
	None = 0,

	// 2D area-like object
	Rect,
	Polygon,

	// 1D path-like object
	StraightSegment UMETA(DisplayName = "Straight Segment"),
	Polyline,
	Spline,

	// 0D point-like object
	Point
};