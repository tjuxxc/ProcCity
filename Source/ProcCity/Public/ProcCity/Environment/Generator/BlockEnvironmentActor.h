// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProcCity/Environment/Types/ResolvedBlockEnvironmentPlan.h"

#include "BlockEnvironmentActor.generated.h"

class USceneComponent;
class UHierarchicalInstancedStaticMeshComponent;
class UBlockEnvironmentStyleDataAsset;

struct FBlockSurfaceModule;
struct FBlockPathSegmentModule;
struct FPathTerminalTreatmentModule;
struct FBlockPathJunctionModule;
struct FBlockPropModule;

// FEnvironmentHISMKey + GetTypeHash
USTRUCT()
struct FEnvironmentHISMKey
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UStaticMesh> Mesh = nullptr;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInterface>> Materials;

	UPROPERTY()
	FName UsageTag = NAME_None;

	UPROPERTY()
	int32 NumCustomDataFloats = 0;

	bool operator==(const FEnvironmentHISMKey& Other) const
	{
		if (Mesh != Other.Mesh)
		{
			return false;
		}

		if (UsageTag != Other.UsageTag)
		{
			return false;
		}

		if (NumCustomDataFloats != Other.NumCustomDataFloats)
		{
			return false;
		}

		if (Materials.Num() != Other.Materials.Num())
		{
			return false;
		}

		for (int32 Index = 0; Index < Materials.Num(); ++Index)
		{
			if (Materials[Index] != Other.Materials[Index])
			{
				return false;
			}
		}

		return true;
	}
};

FORCEINLINE uint32 GetTypeHash(const FEnvironmentHISMKey& Key)
{
	uint32 Hash = GetTypeHash(Key.Mesh);
	Hash = HashCombine(Hash, GetTypeHash(Key.UsageTag));
	Hash = HashCombine(Hash, GetTypeHash(Key.NumCustomDataFloats));
	
	for (const UMaterialInterface* Material : Key.Materials)
	{
		Hash = HashCombine(Hash, GetTypeHash(Material));
	}
	
	return Hash;
}


UCLASS(Blueprintable)
class PROCCITY_API ABlockEnvironmentActor : public AActor
{
	GENERATED_BODY()
	
public:	
	using UHISMComp = UHierarchicalInstancedStaticMeshComponent;
	
	// Sets default values for this actor's properties
	ABlockEnvironmentActor();

public:
	// Set resolved environment plan from external source
	void SetResolvedEnvironmentPlan(const FResolvedBlockEnvironmentPlan& InPlan);
	
	UFUNCTION(CallInEditor, Category="Environment")
	void RegenerateEnvironment();
	
	UFUNCTION(CallInEditor, Category="Environment")
	void ClearGeneratedEnvironment();

protected:
	UPROPERTY(VisibleAnywhere, Category="Components")
	TObjectPtr<USceneComponent> Root = nullptr;
	
	UPROPERTY(VisibleAnywhere, Transient, Category="Environment")
	FResolvedBlockEnvironmentPlan ResolvedEnvironmentPlan;

	// Debug generation toggles
	UPROPERTY(EditAnywhere, Category="Environment")
	bool bGenerateSurfaces = true;
	
	UPROPERTY(EditAnywhere, Category="Environment")
	bool bGeneratePathSegments = true;
	
	UPROPERTY(EditAnywhere, Category="Environment")
	bool bGeneratePathJunction = true;
	
	UPROPERTY(EditAnywhere, Category="Environment")
	bool bGenerateProps = true;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> GeneratedHISMComponents;
	
	// Runtime cache: one HISM per unique mesh/material combination key
	UPROPERTY(Transient)
	TMap<FEnvironmentHISMKey, 
		 TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> HISMComponentsMap;
	
private:
	void GenerateAll();
	
	bool ShouldGenerateInstanceType(EBlockEnvironmentElementType InstanceType) const;
	
	bool AddResolvedInstance(const FResolvedBlockEnvironmentInstance& Instance);
	
	// HISM creation / reuse
	UHISMComp* GetOrCreateHISMComponent(
		const FEnvironmentHISMKey& Key,
		const FString& DebugNamePrefix
	);
	
	void ApplyInstanceCustomData(
		UHISMComp* Component,
		int32 InstanceIndex,
		const TArray<float>& PerInstanceCustomData
	);

	static FEnvironmentHISMKey BuildHISMKey(
		const FResolvedBlockEnvironmentInstance& Instance
	);
};







