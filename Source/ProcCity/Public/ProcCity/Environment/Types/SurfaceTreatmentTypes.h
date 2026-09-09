#pragma once

#include "CoreMinimal.h"

#include "SurfaceTreatmentTypes.generated.h"


// Rect-surface edge identifiers used by the v1.0 surface treatment pipeline.
// These edges are defined in the local 2D surface space.

UENUM(BlueprintType)
enum class ERectSurfaceEdge : uint8
{
	None = 0,
	// Use UE left-hand system: Top -> Y+, Left -> X+
	Top,
	Right,
	Bottom,
	Left
};


// Describes how one rect-surface edge should be treated.
//
// v1.0 intentionally keeps this simple:
// - None: no special handling
// - TrimBackOnly: shrink the interior away from this edge, but do not spawn
//   a dedicated edge region
// - EdgeStrip: shrink the interior and reserve a dedicated edge strip region
UENUM(BlueprintType)
enum class ESurfaceEdgeTreatmentType : uint8
{
	None = 0,
	EdgeStrip
};

// A single edge treatment rule as authored by layout/feature data.
//
// width is interpreted in local surface units. The exact realization details
// (interior trim distance, strip width, corner resolution) are derived later
// by feature/module/resolved-plan building code.
USTRUCT(BlueprintType)
struct PROCCITY_API FSurfaceEdgeTreatmentRule
{
	GENERATED_BODY()
	
	// Which rect edge this rule applies to.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface")
	ERectSurfaceEdge Edge = ERectSurfaceEdge::Top;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface")
	ESurfaceEdgeTreatmentType Type = ESurfaceEdgeTreatmentType::None;

	// Width of the treatment band measured inward from the source edge.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, 
		Category="Surface", meta=(ClampMin=0.0))
	float Width = 0.0f;
};

USTRUCT(BlueprintType)
struct PROCCITY_API FSurfaceEdgeTreatment
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface")
	bool bEnabled = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface",
		meta=(EditCondition="bEnabled", EditConditionHides))
	TArray<FSurfaceEdgeTreatmentRule> EdgeRules;
};

UENUM(BlueprintType)
enum class ERectSurfaceCorner : uint8
{
	None = 0,
	// Use UE left-hand system: Top -> Y+, Left -> X+
	TopLeft,
	TopRight,
	BottomRight,
	BottomLeft
};

UENUM(BlueprintType)
enum class EBlockSurfaceCornerType : uint8
{
	None = 0,

	// Both adjacent rect edges use EdgeStrip, so this corner needs
	// a dedicated two-edge corner patch.
	DoubleEdgeStripCorner,

	// Exactly one adjacent rect edge uses EdgeStrip, so this corner behaves
	// like the end cap of a single edge strip.
	SingleEdgeStripEndCap
};



