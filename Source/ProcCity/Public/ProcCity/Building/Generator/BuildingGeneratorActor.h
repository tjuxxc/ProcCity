// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProcCity/Building/Types/BuildingGenerationTypes.h"
#include "ProcCity/Building/Types/BuildingPlanTypes.h"

#include "BuildingGeneratorActor.generated.h"


class UHierarchicalInstancedStaticMeshComponent;
class UStaticMesh;
class USceneComponent;
class UBuildingArchetypeDataAsset;
class UBuildingMaterialStyleDataAsset;
class UBuildingRuleSet;
struct FBuildingModuleVariant;

USTRUCT()
struct FHISMKey
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UStaticMesh> Mesh = nullptr;

	UPROPERTY()
	TObjectPtr<UBuildingMaterialStyleDataAsset> MaterialStyle = nullptr;

	UPROPERTY()
	EBuildingSemanticSlot Slot = EBuildingSemanticSlot::None;

	bool operator==(const FHISMKey& Other) const
	{
		return Mesh == Other.Mesh 
			&& MaterialStyle == Other.MaterialStyle
			&& Slot == Other.Slot;
	}
};

FORCEINLINE uint32 GetTypeHash(const FHISMKey& Key)
{
	uint32 Hash = GetTypeHash(Key.Mesh);
	Hash = HashCombine(Hash, GetTypeHash(Key.MaterialStyle));
	Hash = HashCombine(Hash, GetTypeHash(static_cast<uint8>(Key.Slot)));
	return Hash;
}

UCLASS(Blueprintable)
class PROCCITY_API ABuildingGeneratorActor : public AActor
{
	GENERATED_BODY()
	
public:	
	using UHISMComp = UHierarchicalInstancedStaticMeshComponent;

	// Sets default values for this actor's properties
	ABuildingGeneratorActor();

public:
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(CallInEditor, Category = "Building")
	void Regenerate();
	
	// Set use external plan. 
	void SetExternalBuildingPlans(const TArray<FBuildingPlan>& InPlans);
	void SetUseExternalPlans(bool bInUseExternalPlans);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root = nullptr;
	
	// Use external plans if set true.
	UPROPERTY(EditAnywhere, Category = "Building|Mode")
	bool bUseExternalPlans = false;
	
	// Array of external building plans
	UPROPERTY(EditAnywhere, Category = "Building|Mode")
	TArray<FBuildingPlan> ExternalBuildingPlans;
	
	// General building batch parameters
	UPROPERTY(EditAnywhere, Category = "Building|Layout", meta = (ClampMin = "1"))
	int32 BuildingCount = 10;

	UPROPERTY(EditAnywhere, Category = "Building|Layout", meta = (ClampMin = "0.0"))
	float BuildingSpacingX = 3000.0f;

	UPROPERTY(EditAnywhere, Category = "Building|Layout", meta = (ClampMin = "0.0"))
	float BuildingSpacingY = 4000.0f;

	UPROPERTY(EditAnywhere, Category = "Building|Layout", meta = (ClampMin = "1"))
	int32 BuildingsPerRow = 2;
	
	// [Important] PerYawRotation is used only in Internal Plan Pipeline
	UPROPERTY(EditAnywhere, Category = "Building|Debug")
	float BaseBuildingYaw = 0.0f;
	
	int32 BuildYawSeed = 123;
	float MaxYawVariation = 0.0f;
	
	// Random seeds
	UPROPERTY(EditAnywhere, Category = "Building|Random")
	int32 MasterSeed = 12345; 

	UPROPERTY(EditAnywhere, Category = "Building|Random")
	int32 ArchetypeSeed = 12345;

	UPROPERTY(EditAnywhere, Category = "Building|Random")
	int32 RuleSeed = 22345;

	UPROPERTY(EditAnywhere, Category = "Building|Random")
	int32 ModuleSeed = 32345;

	UPROPERTY(EditAnywhere, Category = "Building|Random")
	int32 MaterialVariationSeed = 42345;

	// Input archetypes DA
	UPROPERTY(EditAnywhere, Category = "Building|Archetypes")
	TArray<TObjectPtr<UBuildingArchetypeDataAsset>> Archetypes;

	// Scratch variation parameters
	UPROPERTY(EditAnywhere, Category = "Building|Material Perturbation")
	bool bEnablePerInstanceMaterialVariation = true;

	UPROPERTY(EditAnywhere, Category = "Building|Material Perturbation", meta = (ClampMin = "0.0"))
	float DetailUVOffsetMax = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Building|Material Perturbation")
	bool bUseDiscreteQuarterTurns = true;

	UPROPERTY(EditAnywhere, Category = "Building|Material Perturbation")
	float DetailVariationMin = -0.10f;

	UPROPERTY(EditAnywhere, Category = "Building|Material Perturbation")
	float DetailVariationMax = 0.10f;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> HISMComponents;

	UPROPERTY(Transient)
	TMap<FHISMKey, TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> HISMMap;

private:
	void ClearGenerated();
	
	// Building generation logics
	void BuildAllBuildings();
	
	// -- Generate from Internal Plans
	void GenerateBuildingsFromInternalPlans();
	// -- Generate from External Plans
	void GenerateBuildingsFromExternalPlans();
	

	// -- Common generation logics
	
	// Consumed by [GenerateBuildingsFromInternalPlans] and 
	// [GenerateBuildingsFromExternalPlans]
	bool GenerateRawInternalBuildingPlan(int32 BuildingIndex, FBuildingPlan& OutPlan) const;
	
	static bool GenerateBuildingDescriptionFromPlan(
		const FBuildingPlan& Plan,
		FGeneratedBuildingDescription& OutDescription
	);
	
	static float GetDescriptionFootprintWidth(
		const FGeneratedBuildingDescription& Description
	);
	
	static float GetDescriptionFootprintDepth(
		const FGeneratedBuildingDescription& Description
	);
	
	UBuildingArchetypeDataAsset* 
		PickArchetype(FRandomStream& RandomStream) const;
	
	static const UBuildingRuleSet* 
		CreateRuleObject(const UBuildingArchetypeDataAsset* Archetype);

	void InstantiateBuildingDescription(
		const UBuildingArchetypeDataAsset* Archetype,
		const FGeneratedBuildingDescription& Description,
		const FTransform& BuildingWorldTransform,
		FRandomStream& ModuleRandom,
		FRandomStream& MaterialRandom
	);
	
	static const FBuildingModuleVariant* ResolveVariantForPlacement(
		const UBuildingArchetypeDataAsset* Archetype,
		EBuildingSemanticSlot Slot,
		FRandomStream& RandomStream
	);
	
	UHISMComp* GetOrCreateHISM(
		const FBuildingModuleVariant* Variant,
		EBuildingSemanticSlot Slot,
		const TArray<TObjectPtr<UBuildingMaterialStyleDataAsset>>& Styles,
		int32 StyleIndex
	);
	
	void AddInstance(
		const FBuildingModuleVariant* Variant,
		EBuildingSemanticSlot Slot,
		const TArray<TObjectPtr<UBuildingMaterialStyleDataAsset>>& Styles,
		int32 StyleIndex,
		const FTransform& WorldTransform,
		FRandomStream& RandomStream
	);

	void MakePerInstanceMaterialData(
		FRandomStream& RandomStream,
		float& OutDetailOffsetU,
		float& OutDetailOffsetV,
		float& OutDetailRotation,
		float& OutDetailVariation
	) const;

	static void ApplyStyleToHISM(
		UHISMComp* HISM,
		const FBuildingModuleVariant* Variant,
		const TArray<TObjectPtr<UBuildingMaterialStyleDataAsset>>& Styles,
		int32 StyleIndex
	);
	

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
