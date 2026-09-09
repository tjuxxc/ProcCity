#pragma once

#include "CoreMinimal.h"
#include "ProcCity/Environment/Types/BlockEnvironmentEnums.h"

#include "ResolvedMaterialContext.generated.h"


// FResolvedMaterialMetrics
// FResolvedMaterialContext

USTRUCT(BlueprintType)
struct PROCCITY_API FResolvedMaterialMetrics
{
	GENERATED_BODY()
	
	// Coverage ratios for material UV space relative to nominal/reference dimensions.
	// Derived from mesh local space using this convention:
	//   +Y -> material U
	//   +X -> material V
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material")
	float CoverageScaleU = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material")
	float CoverageScaleV = 1.0f;
	
	FResolvedMaterialMetrics() = default;
	
	FResolvedMaterialMetrics(const float InCoverageScaleU, 
		const float InCoverageScaleV)
		: CoverageScaleU(InCoverageScaleU)
		, CoverageScaleV(InCoverageScaleV)
	{
	}
	
	static FResolvedMaterialMetrics Identity()
	{
		return FResolvedMaterialMetrics(1.0f, 1.0f);
	}
};

USTRUCT(BlueprintType)
struct PROCCITY_API FResolvedMaterialContext
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material")
	FTransform RelativeTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material")
	FResolvedMaterialMetrics Metrics;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material")
	EBlockEnvironmentElementType ElementType =
		EBlockEnvironmentElementType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material")
	FName UsageTag = NAME_None;
};