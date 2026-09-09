#include "ProcCity/Material/Resolver/MaterialPayloadResolver.h"

#include "Materials/MaterialInterface.h"
#include "ProcCity/Material/Resolver/MaterialCustomDataResolver.h"


bool FMaterialPayloadResolver::ResolveMaterialPayload(
	const TArray<FMaterialSlotBinding>& MaterialSlots,
	const FResolvedMaterialContext& Context,
	FResolvedMaterialPayload& OutPayload
)
{
	OutPayload.Reset();
	
	if (!ResolveMaterials(MaterialSlots, OutPayload.Materials))
	{
		return false;
	}
	
	if (!FMaterialCustomDataResolver::ResolveCustomData(
		MaterialSlots,
		Context,
		OutPayload.PerInstanceCustomData
	))
	{
		return false;
	}
	
	return true;
}


bool FMaterialPayloadResolver::ResolveMaterials(
	const TArray<FMaterialSlotBinding>& MaterialSlots,
	TArray<TObjectPtr<UMaterialInterface>>& OutMaterials
)
{
	OutMaterials.Reset();
	int32 MaxMaterialSlotIndex = INDEX_NONE;
	
	for (const FMaterialSlotBinding& MaterialSlot : MaterialSlots)
	{
		if (!MaterialSlot.bEnabled)
		{
			continue;
		}
		
		if (MaterialSlot.MaterialSlotIndex < 0)
		{
			return false;
		}
		
		MaxMaterialSlotIndex = FMath::Max(
			MaterialSlot.MaterialSlotIndex,
			MaxMaterialSlotIndex
		);
	}
	
	if (MaxMaterialSlotIndex == INDEX_NONE)
	{
		// No enabled material slots. This is not necessarily an error.
		return true;
	}
	
	OutMaterials.SetNumZeroed(MaxMaterialSlotIndex + 1);
	
	TSet<int32> AssignedSlots;
	
	for (const FMaterialSlotBinding& MaterialSlot : MaterialSlots)
	{
		if (!MaterialSlot.bEnabled)
		{
			continue;
		}
		
		if (MaterialSlot.MaterialSlotIndex < 0)
		{
			return false;
		}
		
		if (!MaterialSlot.Material)
		{
			return false;
		}
		
		if (AssignedSlots.Contains(MaterialSlot.MaterialSlotIndex))
		{
			// Duplicate material slot binding is treated as invalid authoring.
			return false;
		}
		
		AssignedSlots.Add(MaterialSlot.MaterialSlotIndex);
		OutMaterials[MaterialSlot.MaterialSlotIndex] = MaterialSlot.Material;
	}
	
	return true;
}




































