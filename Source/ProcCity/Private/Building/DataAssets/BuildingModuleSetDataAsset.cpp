// Fill out your copyright notice in the Description page of Project Settings.


#include "ProcCity/Building/DataAssets/BuildingModuleSetDataAsset.h"


const TArray<FBuildingModuleVariant>* 
	UBuildingModuleSetDataAsset::FindVariants(EBuildingSemanticSlot Slot) const
{
	for (const FBuildingSlotVariantCollection& Collection : VariantCollections)
	{
		if (Collection.Slot == Slot)
		{
			return &Collection.Variants;
		}
	}
	return nullptr;
}


/**
 RandomStream is passed for future extension. This implementation always selects
 the first valid variant, but we want to have the option of adding more complex 
 selection logic.
 */

const FBuildingModuleVariant* UBuildingModuleSetDataAsset::PickVariant(
	EBuildingSemanticSlot Slot,
	FRandomStream& RandomStream 
) const
{
	const TArray<FBuildingModuleVariant>* Variants = FindVariants(Slot);

	if (!Variants || Variants->Num() == 0)
	{
		return nullptr;
	}

	for (const FBuildingModuleVariant& Variant : *Variants)
	{
		if (IsValid(Variant.Mesh))
		{
			return &Variant;
		}
	}

	return nullptr;
}
