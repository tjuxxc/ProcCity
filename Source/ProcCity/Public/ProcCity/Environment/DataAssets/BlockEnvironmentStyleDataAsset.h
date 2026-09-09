// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProcCity/Environment/Types/BlockEnvironmentModuleTypes.h"

#include "BlockEnvironmentStyleDataAsset.generated.h"

struct FBlockSurfaceFeature;
struct FBlockPointFeature;
struct FBlockPathFeature;
struct FBlockPathJunctionFeature;
struct FPathTerminalTreatment;

// TODO: Modules (meshes) and materials placed in separate DAs.
UCLASS(BlueprintType)
class PROCCITY_API UBlockEnvironmentStyleDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	// Include area, path, point object modules
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Environment")
	TArray<FBlockSurfaceModule> SurfaceModules;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Environment")
	TArray<FSurfaceEdgeStripModule> SurfaceEdgeModules;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Environment")
	TArray<FSurfaceCornerModule> SurfaceCornerModules;
	
	// Path end treatment modules
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Environment")
	TArray<FBlockPathSegmentModule> PathSegmentModules;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Environment")
	TArray<FBlockPathJunctionModule> PathJunctionModules;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Environment")
	TArray<FBlockPropModule> PropModules;
	
	// Get module functions
	const FBlockSurfaceModule* GetSurfaceModule(
		const FBlockSurfaceFeature& SurfaceFeature
	) const;
	
	const FSurfaceEdgeStripModule* GetSurfaceEdgeModule(
		const FBlockSurfaceFeature& SurfaceFeature,
		const ESurfaceEdgeTreatmentType TreatmentType,
		const float Width,
		const float WidthTolerance = 1.0f
	) const;
	
	const FSurfaceCornerModule* GetSurfaceCornerModule(
		const FBlockSurfaceFeature& SurfaceFeature,
		EBlockSurfaceCornerType CornerType,
		ERectSurfaceCorner WhichCorner,
		float HorizontalWidth,
		float VerticalWidth,
		float WidthTolerance = 1.0f
	) const;
	
	const FBlockPropModule* GetPropModule(
		const FBlockPointFeature& PointFeature
	) const;
	
	const FBlockPathSegmentModule* GetPathSegmentModule(
		const FBlockPathFeature& PathFeature
	) const;
	
	const FBlockPathJunctionModule* GetPathJunctionModule(
		const FBlockPathJunctionFeature& PathJunctionFeature
	) const;
	
	static const FPathTerminalTreatmentModule* GetPathTerminalTreatmentModule(
		const TArray<FPathTerminalTreatmentModule>& SupportedModules,
		const FPathTerminalTreatment& Treatment,
		bool bStartTreatment,
		const EHandedness PathHandedness
	);
	
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(
		FDataValidationContext& Context
	) const override;
#endif
	
};
