#pragma once

#include "CoreMinimal.h"
#include "ProcCity/Material/Types/ResolvedMaterialPayload.h"
#include "ProcCity/Environment/Types/BlockEnvironmentEnums.h"

#include "ResolvedBlockEnvironmentPlan.generated.h"

class UStaticMesh;


/**
 * Optional debug/source category describing where this instance came from.
 * This is intentionally lightweight and may be expanded later.
 */
USTRUCT(BlueprintType)
struct PROCCITY_API FResolvedBlockEnvironmentSourceRef
{
	GENERATED_BODY()

	// Index into the source feature array when applicable.
	// Example: PathFeatures[3], PointFeatures[5], etc.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	int32 SourceIndex = INDEX_NONE;

	// Free-form category for debugging.
	// Examples: "PathFeature", "PathJunctionFeature", "PointFeature", "GroundBase"
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	FName SourceCategory = NAME_None;
};


/**
 * A single fully resolved environment instance ready for realization by
 * ABlockEnvironmentActor (or another realization system).
 */
USTRUCT(BlueprintType)
struct PROCCITY_API FResolvedBlockEnvironmentInstance
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	EBlockEnvironmentElementType InstanceType =
		EBlockEnvironmentElementType::None;
	
	// Usage/debug grouping tag used by realization side when creating HISM buckets.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	FName UsageTag = NAME_None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	TObjectPtr<UStaticMesh> Mesh = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	FResolvedMaterialPayload MaterialPayload;
	
	// Final local transform relative to the environment actor root.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	FTransform RelativeTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	bool bEnabled = true;

	// Optional debug provenance.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	FResolvedBlockEnvironmentSourceRef SourceRef;
	
	bool IsValidForRealization() const
	{
		return bEnabled && Mesh != nullptr;
	}
};

/**
 * Final resolved environment plan for a block.
 * This is the output of the environment planning / resolve stage and the
 * input to ABlockEnvironmentActor realization.
 */
USTRUCT(BlueprintType)
struct PROCCITY_API FResolvedBlockEnvironmentPlan
{
	GENERATED_BODY()

	// Unified final instance list consumed by environment realization.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
	TArray<FResolvedBlockEnvironmentInstance> Instances;

	void Reset()
	{
		Instances.Reset();
	}

	bool IsEmpty() const
	{
		return Instances.Num() == 0;
	}
	
	int32 Num() const
	{
		return Instances.Num();
	}
};