// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProcCity/Building/Types/BuildingGenerationTypes.h"

#include "BuildingArchetypeDataAsset.generated.h"


class UBuildingModuleSetDataAsset;
class UBuildingMaterialStyleDataAsset;
class UBuildingRuleSet;


UCLASS(BlueprintType)
class PROCCITY_API UBuildingArchetypeDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UBuildingArchetypeDataAsset();
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype")
	FName ArchetypeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype", meta = (ClampMin = "1"))
	int32 Weight = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype")
	TObjectPtr<UBuildingModuleSetDataAsset> ModuleSet = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype")
	TArray<TObjectPtr<UBuildingMaterialStyleDataAsset>> MaterialStyles;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype")
	TSubclassOf<UBuildingRuleSet> RuleSetClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype")
	FBuildingDimensionConfig DefaultDimension;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype")
	FBuildingMetrics DefaultMetrics;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype")
	bool bIncludeGround = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype")
	bool bIncludeRoof = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement Constraints", meta = (ClampMin = "0.0"))
	float MinWidth = 4500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement Constraints", meta = (ClampMin = "0.0"))
	float MaxWidth = 9000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement Constraints", meta = (ClampMin = "0.0"))
	float MinDepth = 1600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement Constraints", meta = (ClampMin = "0.0"))
	float MaxDepth = 6000.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement Constraints")
	bool bAllowInteriorLots = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement Constraints")
	bool bAllowEdgeLots = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement Constraints")
	bool bAllowCornerLots = true;
	
	/*
	 * [EProcCityZoneType] is currently defined in [ProcCityRuntime] Module, and I don't wish 
	 * [BuildingGenerator] depend on [ProcCityRuntime]
	 */
	// UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement Constraints")
	// TArray<EProcCityZoneType> AllowedZoneTypes;
};
