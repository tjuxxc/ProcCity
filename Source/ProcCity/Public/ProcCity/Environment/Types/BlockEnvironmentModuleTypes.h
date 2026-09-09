// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProcCity/Environment/Types/PathTreatmentTypes.h"
#include "ProcCity/Environment/Types/BlockEnvironmentEnums.h"
#include "ProcCity/Environment/Types/SurfaceTreatmentTypes.h"
#include "ProcCity/Material/Types/MaterialSlotBinding.h"
#include "ProcCity/Common/Types/Handedness.h"

#include "BlockEnvironmentModuleTypes.generated.h"


class UMaterialInterface;
class UStaticMesh;

/*
 * Style module definition:
 * This is the presentation layer used by style data assets.
 * It maps a style slot to actual renderable resources.
 */


USTRUCT(BlueprintType)
struct PROCCITY_API FSurfaceEdgeStripModule
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	EBlockSurfaceCategory TargetSurfaceCategory = EBlockSurfaceCategory::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment",
		meta=(EditCondition="TargetSurfaceCategory == EBlockSurfaceCategory::Ground", 
			EditConditionHides))
	EBlockGroundRole TargetGroundRole = EBlockGroundRole::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment",
		meta=(EditCondition="TargetSurfaceCategory == EBlockSurfaceCategory::Reserve", 
			EditConditionHides))
	EBlockReserveRole TargetReserveRole = EBlockReserveRole::None;
	
	// Current v1.0 only targets rect surfaces.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	EBlockFeatureGeometryType TargetSurfaceGeometryType = 
		EBlockFeatureGeometryType::Rect;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	ESurfaceEdgeTreatmentType TreatmentType = ESurfaceEdgeTreatmentType::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	float Width = 0.0f;
	
	// Optional style variant.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	FName VariantTag = NAME_None;
	
	// Presentation payload
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	TObjectPtr<UStaticMesh> Mesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	TArray<FMaterialSlotBinding> MaterialSlots;
	
	// Nominal local size of the strip mesh.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	float NominalLength = 100.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	FVector PivotOffset = FVector::ZeroVector;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	float ZOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment", meta=(ClampMin="0.0"))
	float DefaultThickness = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	bool bScalable = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	bool bEnabled = true;
};

USTRUCT(BlueprintType)
struct PROCCITY_API FSurfaceCornerModule
{
	GENERATED_BODY()

	// ---------- Semantic matching key ----------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	EBlockSurfaceCategory TargetSurfaceCategory = EBlockSurfaceCategory::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment",
		meta=(EditCondition="TargetSurfaceCategory == EBlockSurfaceCategory::Ground", 
			EditConditionHides))
	EBlockGroundRole TargetGroundRole = EBlockGroundRole::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment",
		meta=(EditCondition="TargetSurfaceCategory == EBlockSurfaceCategory::Reserve", 
			EditConditionHides))
	EBlockReserveRole TargetReserveRole = EBlockReserveRole::None;
	
	// Current v1.0 only targets rect surfaces.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	EBlockFeatureGeometryType TargetSurfaceGeometryType = 
		EBlockFeatureGeometryType::Rect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	EBlockSurfaceCornerType CornerType = EBlockSurfaceCornerType::None;

	// Optional: limit to a specific corner if a mesh is not rotationally reusable.
	// Leave unconstrained if Corner = ERectSurfaceCorner::None.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	ERectSurfaceCorner WhichCorner = ERectSurfaceCorner::None;

	// Optional width tags for module selection.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	float HorizontalWidth = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	float VerticalWidth = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	FName VariantTag = NAME_None;

	// ---------- Presentation payload ----------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	TObjectPtr<UStaticMesh> Mesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	TArray<FMaterialSlotBinding> MaterialSlots;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	FVector PivotOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	float ZOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment", meta=(ClampMin="0.0"))
	float DefaultThickness = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	bool bScalable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	bool bEnabled = true;
};


USTRUCT(BlueprintType)
struct PROCCITY_API FBlockSurfaceModule
{
	GENERATED_BODY()
	
	// --------------------- Semantic matching key ------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	EBlockSurfaceCategory SurfaceCategory = EBlockSurfaceCategory::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	EBlockFeatureGeometryType GeometryType = EBlockFeatureGeometryType::None;
	
	// ------------------------- Important ---------------------------
	// Exactly one of the following roles is expected to be meaningful, 
	// depending on Category.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment", 
		meta=(EditCondition="SurfaceCategory == EBlockSurfaceCategory::Ground", 
			EditConditionHides))
	EBlockGroundRole GroundRole = EBlockGroundRole::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment", 
		meta=(EditCondition="SurfaceCategory == EBlockSurfaceCategory::Reserve", 
			EditConditionHides))
	EBlockReserveRole ReserveRole = EBlockReserveRole::None;

	// Optional variant tag for future theme/style expansion
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	FName VariantTag = NAME_None;
	
	// ---------------- Presentation payload -------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	TObjectPtr<UStaticMesh> Mesh = nullptr;
	
	// Optional override material
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	TArray<FMaterialSlotBinding> MaterialSlots;
	
	// The nominal unscaled size of this mesh in Unreal units (cm)
	// Example: UE default cube has size (100, 100, 100) cm
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	FVector NominalSize = FVector(100.0f, 100.0f, 100.0f);
	
	// Local pivot offset if needed; usually zero for centered slabs
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	FVector PivotOffset = FVector::ZeroVector;
	
	// Vertical offset applied when spawning / placing this module
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	float ZOffset = 0.0f;
	
	// Default thickness hint for generated slabs if you use scale-based placement
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment", meta=(ClampMin="0.0"))
	float DefaultThickness = 5.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	bool bScalable = true;
	
	// Whether this slot is enabled in the current style
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	bool bEnabled = true;
};

USTRUCT(BlueprintType)
struct PROCCITY_API FPathTerminalTreatmentModule
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	EBlockPathTreatmentType TreatmentType = EBlockPathTreatmentType::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	EHandedness Handedness = EHandedness::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	FName VariantTag = NAME_None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	TObjectPtr<UStaticMesh> Mesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	TArray<FMaterialSlotBinding> MaterialSlots;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	FVector PivotOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	float ZOffset = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment", meta=(ClampMin="0.0"))
	float DefaultThickness = 5.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	bool bScalable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	bool bEnabled = true;
};


USTRUCT(BlueprintType)
struct PROCCITY_API FBlockPathSegmentModule
{
	GENERATED_BODY()
	
	// ------ Semantic Matching key ------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	EBlockPathRole PathRole = EBlockPathRole::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	EBlockFeatureGeometryType GeometryType = 
		EBlockFeatureGeometryType::StraightSegment;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	EBlockPathWidthSpec WidthSpec = EBlockPathWidthSpec::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	EHandedness Handedness = EHandedness::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	FName VariantTag = NAME_None;
	
	// ------ Main segment payload ------ 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	TObjectPtr<UStaticMesh> Mesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	TArray<FMaterialSlotBinding> MaterialSlots;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	float NominalLength = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	FVector SegmentPivotOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	float SegmentZOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	float DefaultThickness = 5.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	float JoinOverlap = 1.0f;
	
	// ------ Supported start/end treatment payload
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	TArray<FPathTerminalTreatmentModule> SupportedTerminalTreatments;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	bool bScalable = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	bool bEnabled = true;
};

USTRUCT(BlueprintType)
struct PROCCITY_API FBlockPathJunctionModule
{
	GENERATED_BODY()
	
	// ------ Semantic Matching key ------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	EBlockPathRole PathRole = EBlockPathRole::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	EBlockPathJunctionType JunctionType = EBlockPathJunctionType::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	EBlockFeatureGeometryType GeometryType = EBlockFeatureGeometryType::None;
	
	// Width hints for environment realization
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	EBlockPathWidthSpec PrimaryWidthSpec = EBlockPathWidthSpec::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	EBlockPathWidthSpec SecondaryWidthSpec = EBlockPathWidthSpec::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	FName VariantTag = NAME_None;
	
	
	// ---------------- Presentation payload -------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	TObjectPtr<UStaticMesh> Mesh = nullptr;
	
	// Optional override material
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	TArray<FMaterialSlotBinding> MaterialSlots;
	
	// Local pivot offset if needed; usually zero for centered slabs
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	FVector PivotOffset = FVector::ZeroVector;
	
	// Vertical offset applied when spawning / placing this module
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	float ZOffset = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment", meta=(ClampMin="0.0"))
	float DefaultThickness = 5.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	bool bScalable = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	bool bEnabled = true;
};

USTRUCT(BlueprintType)
struct PROCCITY_API FBlockPropModule
{
	GENERATED_BODY()
	
	// ------ Semantic Matching key ------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	EBlockPointRole PropRole = EBlockPointRole::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	EBlockFeatureGeometryType GeometryType = EBlockFeatureGeometryType::Point;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	FName VariantTag = NAME_None;
	
	// ---------------- Presentation payload -------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	TObjectPtr<UStaticMesh> Mesh = nullptr;
	
	// Optional override material
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	TArray<FMaterialSlotBinding> MaterialSlots;
	
	// Local pivot offset if needed; usually zero for centered slabs
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	FVector PivotOffset = FVector::ZeroVector;
	
	// Vertical offset applied when spawning / placing this module
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	float ZOffset = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	bool bScalable = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	bool bEnabled = true;
};