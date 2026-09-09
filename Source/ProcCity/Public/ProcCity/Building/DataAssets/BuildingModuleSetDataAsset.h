// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProcCity/Building/Types/BuildingModuleTypes.h"
#include "BuildingModuleSetDataAsset.generated.h"

UCLASS(BlueprintType)
class PROCCITY_API UBuildingModuleSetDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modules")
	TArray<FBuildingSlotVariantCollection> VariantCollections;

public:
	const TArray<FBuildingModuleVariant>* FindVariants(EBuildingSemanticSlot Slot) const;

	const FBuildingModuleVariant* PickVariant(
		EBuildingSemanticSlot Slot, FRandomStream& RandomStream) const;
};
 