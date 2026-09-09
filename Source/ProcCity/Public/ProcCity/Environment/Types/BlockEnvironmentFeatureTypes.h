#pragma once

#include "CoreMinimal.h"
#include "ProcCity/Environment/Types/SurfaceTreatmentTypes.h"
#include "ProcCity/Environment/Types/BlockEnvironmentEnums.h"
#include "ProcCity/Environment/Types/PathTreatmentTypes.h"
#include "ProcCity/Common/Types/Handedness.h"

#include "BlockEnvironmentFeatureTypes.generated.h"

// -------------------- Surface Feature -------------------- 
USTRUCT(BlueprintType)
struct PROCCITY_API FBlockSurfaceFeature
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	EBlockSurfaceCategory SurfaceCategory = EBlockSurfaceCategory::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block", 
		meta=(EditCondition="SurfaceCategory == EBlockSurfaceCategory::Ground", 
			EditConditionHides))
	EBlockGroundRole GroundRole = EBlockGroundRole::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block", 
		meta=(EditCondition="SurfaceCategory == EBlockSurfaceCategory::Reserve", 
			EditConditionHides))
	EBlockReserveRole ReserveRole = EBlockReserveRole::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	EBlockFeatureGeometryType GeometryType = EBlockFeatureGeometryType::Rect;
	
	// Used when GeometryType == Rect.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block",
		meta=(EditCondition="GeometryType == EBlockFeatureGeometryType::Rect", 
			EditConditionHides))
	FBox2D LocalRect = FBox2D(ForceInit);
	
	// Used when GeometryType == Polygon.
	// Kept here for future evolution, but rect is the only v1.0 shape expected
	// to fully support edge treatment realization.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block",
		meta=(EditCondition="GeometryType == EBlockFeatureGeometryType::Polygon", 
			EditConditionHides))
	TArray<FVector> LocalControlPoints;
	
	// Optional edge treatment authoring for rect surfaces.
	//
	// This mirrors the layout json's "edgeTreatment" concept and intentionally
	// only stores edge-side intent. Corner details are derived later in 
	// [FBlockEnvironmentPlanResolver]

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	FSurfaceEdgeTreatment EdgeTreatment;
	
	// Current version [HasValidEdgeTreatment] is valid only for rect surface 
	bool HasValidEdgeTreatment() const
	{
		if (GeometryType != EBlockFeatureGeometryType::Rect)
		{
			return false;
		}

		if (!EdgeTreatment.bEnabled)
		{
			return false;
		}
		
		if (EdgeTreatment.EdgeRules.Num() == 0)
		{
			return false;
		}
		
		if (!LocalRect.bIsValid)
		{
			return false;
		}
		
		//UE_LOG(LogTemp, Warning, TEXT("Has valid edge rule"));
		return true;
	}
};

// -------------------- Path Feature --------------------- 


// Path feature definition
USTRUCT(BlueprintType)
struct PROCCITY_API FBlockPathFeature
{
	GENERATED_BODY()
	
	// Role includes: None/Alley/ServiceLane/PedestrianPath
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	EBlockPathRole Role = EBlockPathRole::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	EBlockFeatureGeometryType GeometryType = EBlockFeatureGeometryType::StraightSegment;
	
	// Used when GeometryType == Segment
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	FVector LocalStart = FVector::ZeroVector;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	FVector LocalEnd = FVector::ZeroVector;
	
	// Used when GeometryType == Polyline / Spline
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	TArray<FVector> LocalControlPoints;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	EBlockPathWidthSpec WidthSpec = EBlockPathWidthSpec::None;
	
	// Optional Handedness hint
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	EHandedness HandednessHint = EHandedness::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	FPathTerminalTreatment StartTreatment;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	FPathTerminalTreatment EndTreatment;
};

// ------------------ Path Junction feature --------------------- 
USTRUCT(BlueprintType)
struct PROCCITY_API FBlockPathJunctionFeature
{
	GENERATED_BODY()
	
	// Semantic junction type
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	EBlockPathJunctionType JunctionType = EBlockPathJunctionType::None;
	
	// Which path family visually/semantically dominates this junction
	// Example: MainPath / PedestrianPath / ServiceLane
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	EBlockPathRole Association = EBlockPathRole::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	EBlockFeatureGeometryType GeometryType = EBlockFeatureGeometryType::Rect;
	
	// Local origin and rotation for placement
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	FVector LocalOrigin = FVector::ZeroVector;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	FRotator LocalRotation = FRotator::ZeroRotator;
	
	// // Optional local rect when GeometryType == Rect
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	// FBox2D LocalRect = FBox2D(ForceInit);
	
	// Used when GeometryType == Polygon
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	TArray<FVector> LocalControlPoints;
	
	// Width hints for environment realization
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	EBlockPathWidthSpec PrimaryWidthSpec = EBlockPathWidthSpec::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	EBlockPathWidthSpec SecondaryWidthSpec = EBlockPathWidthSpec::None;
};


// ----------------- Point Feature -----------------
USTRUCT(BlueprintType)
struct PROCCITY_API FBlockPointFeature
{
	GENERATED_BODY()
	
	// Role includes: Tree, Light, etc.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	EBlockPointRole Role = EBlockPointRole::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	EBlockFeatureGeometryType GeometryType = EBlockFeatureGeometryType::Point;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	FVector LocalOrigin = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	FRotator LocalRotation = FRotator::ZeroRotator;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	float RandomYawMin = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	float RandomYawMax = 0.0f;
};