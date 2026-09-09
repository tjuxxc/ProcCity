#include "ProcCity/Material/Validator/MaterialSlotBindingValidator.h"

#include "ProcCity/Material/Types/MaterialSlotBinding.h"



#if WITH_EDITOR

EDataValidationResult FMaterialSlotBindingValidator::ValidateMaterialSlots(
	const TArray<FMaterialSlotBinding>& MaterialSlots,
	const FString& OwnerLabel,
	FDataValidationContext& Context
)
{
	bool bHasErrors = false;
	
	for (int32 SlotBindIdx = 0; 
		SlotBindIdx < MaterialSlots.Num(); ++SlotBindIdx)
	{
		const FMaterialSlotBinding& MaterialSlot = MaterialSlots[SlotBindIdx];
		
		if (!ValidateSingleMaterialSlot(MaterialSlot, SlotBindIdx,
			OwnerLabel,Context))
		{
			bHasErrors = true;
		}
	}
	
	if (!ValidateCrossSlotUsage(MaterialSlots, OwnerLabel, Context))
	{
		bHasErrors = true;
	}
	
	return bHasErrors
		? EDataValidationResult::Invalid
		: EDataValidationResult::Valid;
}

bool FMaterialSlotBindingValidator::ValidateSingleMaterialSlot(
	const FMaterialSlotBinding& MaterialSlot,
	const int32 SlotBindIdx,
	const FString& OwnerLabel,
	FDataValidationContext& Context
)
{
	bool bValid = true;
	
	const FString SlotLabel = FString::Printf(
		TEXT("%s.MaterialSlots[%d]"), 
		*OwnerLabel, 
		SlotBindIdx
	);
	
	if (!MaterialSlot.bEnabled)
	{
		return true;
	}
	
	// Validate basic material setting
	if (MaterialSlot.MaterialSlotIndex < 0)
	{
		Context.AddError(FText::FromString(FString::Printf(
			TEXT("%s: MaterialSlotIndex must be >= 0."),
			*SlotLabel
		)));
		bValid = false;
	}
	
	if (!MaterialSlot.Material)
	{
		Context.AddError(FText::FromString(FString::Printf(
			TEXT("%s: Material is null for an enabled material slot binding."),
			*SlotLabel
		)));
		bValid = false;
	}
	
	// Validate UV custom data related setting
	if (MaterialSlot.UVBehavior == EMaterialUVBehavior::IgnoreInstanceUV)
	{
		if (MaterialSlot.UVCustomDataLayout.bEnabled)
		{
			Context.AddWarning(FText::FromString(FString::Printf(
				TEXT("%s: UVCustomDataLayout is enabled but UVBehavior is "
							"IgnoreInstanceUV. UV custom data will be ignored."),
				*SlotLabel
			)));
		}
		
		// No further UV-specific validation is required for IgnoreInstanceUV.
		return bValid;
	}
	
	if (MaterialSlot.UVBaseSource.SourceType == EMaterialUVBaseSourceType::None)
	{
		Context.AddError(FText::FromString(FString::Printf(
			TEXT("%s: UVBehavior requires a valid UVBaseSource, "
						"but SourceType is None."),
			*SlotLabel
		)));
		bValid = false;
	}
	
	if (MaterialSlot.UVBaseSource.SourceType == 
		EMaterialUVBaseSourceType::AtlasMapping 
		&& !MaterialSlot.UVBaseSource.AtlasMapping)
	{
		Context.AddError(FText::FromString(FString::Printf(
			TEXT("%s: UVBaseSource.SourceType is AtlasMapping but "
						"AtlasMapping asset is null."),
			*SlotLabel
		)));
		bValid = false;
	}
	
	const bool bRequiresUVCustomData = 
		RequiresUVCustomData(MaterialSlot);
	
	if (bRequiresUVCustomData && !MaterialSlot.UVCustomDataLayout.bEnabled)
	{
		Context.AddError(FText::FromString(FString::Printf(
			TEXT("%s: This UV configuration requires UVCustomDataLayout "
						"to be enabled."),
			*SlotLabel
		)));
		bValid = false;
	}
	
	if (MaterialSlot.UVCustomDataLayout.bEnabled)
	{
		if (MaterialSlot.UVCustomDataLayout.CustomDataStartIndex < 0)
		{
			Context.AddError(FText::FromString(FString::Printf(
				TEXT("%s: UVCustomDataLayout.CustomDataStartIndex must be >= 0."),
				*SlotLabel
			)));
			bValid = false;
		}
		
		if (MaterialSlot.UVCustomDataLayout.BlockTag.IsNone())
		{
			Context.AddWarning(FText::FromString(FString::Printf(
				TEXT("%s: UVCustomDataLayout.BlockTag is None. "
							"Consider assigning a tag for authoring clarity."),
				*SlotLabel
			)));
		}
	}
	
	return bValid;
}

bool FMaterialSlotBindingValidator::ValidateCrossSlotUsage(
	const TArray<FMaterialSlotBinding>& MaterialSlots,
	const FString& OwnerLabel,
	FDataValidationContext& Context
)
{
	bool bValid = true;
	
	// -- Check duplicate slot index among TArray<FMaterialSlotBinding>& MaterialSlots
	TMap<int32, int32> FirstBindingByMaterialSlotIndex;
	for (int32 SlotBindIdx = 0; SlotBindIdx < MaterialSlots.Num();
		++SlotBindIdx)
	{
		const FMaterialSlotBinding& MaterialSlot = MaterialSlots[SlotBindIdx];
		
		if (!MaterialSlot.bEnabled)
		{
			continue;
		}
		
		const int32 SlotIndex = MaterialSlot.MaterialSlotIndex;
		if (SlotIndex < 0)
		{
			continue;
		}
		
		if (const int32* ExistingBindingIndex = 
			FirstBindingByMaterialSlotIndex.Find(SlotIndex))
		{
			Context.AddError(FText::FromString(FString::Printf(
				TEXT("%s: MaterialSlotIndex %d is used by both MaterialSlots[%d] "
					 "and MaterialSlots[%d]."),
				*OwnerLabel,
				SlotIndex,
				*ExistingBindingIndex,
				SlotBindIdx
			)));
			bValid = false;
		}
		else
		{
			FirstBindingByMaterialSlotIndex.Add(
				SlotIndex, SlotBindIdx);
		}
	}
	
	// -- Check custom data index range overlap
	for (int32 IndexA = 0; IndexA < MaterialSlots.Num(); ++IndexA)
	{
		const FMaterialSlotBinding& SlotBindA = MaterialSlots[IndexA];
		if (!UsesUVCustomData(SlotBindA))
		{
			continue;
		}
		
		int32 AStart = INDEX_NONE;
		int32 AEnd = INDEX_NONE;
		
		if (!GetUVBlockRange(SlotBindA, AStart, AEnd))
		{
			continue;
		}
		
		for (int32 IndexB = IndexA + 1; 
			IndexB < MaterialSlots.Num(); ++IndexB)
		{
			const FMaterialSlotBinding& SlotBindB = MaterialSlots[IndexB];
			if (!UsesUVCustomData(SlotBindB))
			{
				continue;
			}
			
			int32 BStart = INDEX_NONE;
			int32 BEnd = INDEX_NONE;
			if (!GetUVBlockRange(SlotBindB, BStart, BEnd))
			{
				continue;
			}
			
			if (!DoRangesOverlap(AStart, AEnd, BStart, BEnd))
			{
				// Case no Overlap
				continue;
			}
			
			// Address overlap case
			const bool bExactlySameRange = 
				(AStart == BStart && AEnd == BEnd);
			
			if (!bExactlySameRange)
			{
				Context.AddError(FText::FromString(FString::Printf(
					TEXT("%s: UV custom data ranges overlap between "
								"MaterialSlots[%d] [%d..%d] and MaterialSlots[%d] [%d..%d]."),
					*OwnerLabel,
					IndexA, AStart, AEnd,
					IndexB, BStart, BEnd
				)));
				bValid = false;
				continue;
			}
			
			// Same exact block range: allow, but validate intent lightly.
			const FName BlockTagA = SlotBindA.UVCustomDataLayout.BlockTag;
			const FName BlockTagB = SlotBindB.UVCustomDataLayout.BlockTag;
			
			if (BlockTagA != BlockTagB)
			{
				Context.AddWarning(FText::FromString(FString::Printf(
					TEXT("%s: MaterialSlots[%d] and MaterialSlots[%d] share the "
								"same UV custom data block [%d..%d] but use different "
								"BlockTag values ('%s' vs '%s')."),
					*OwnerLabel,
					IndexA,
					IndexB,
					AStart,
					AEnd,
					*BlockTagA.ToString(),
					*BlockTagB.ToString()
				)));
			}
			
			if (SlotBindA.UVBehavior != SlotBindB.UVBehavior)
			{
				Context.AddWarning(FText::FromString(FString::Printf(
					TEXT("%s: MaterialSlots[%d] and MaterialSlots[%d] share the same UV "
								"custom data block [%d..%d] but use different UVBehavior values."),
					*OwnerLabel,
					IndexA,
					IndexB,
					AStart,
					AEnd
				)));
			}
		}
	}
	
	return bValid;
}

bool FMaterialSlotBindingValidator::UsesUVCustomData(
	const FMaterialSlotBinding& MaterialSlot
)
{
	if (!MaterialSlot.bEnabled)
	{
		return false;
	}
	
	if (MaterialSlot.UVBehavior == EMaterialUVBehavior::IgnoreInstanceUV)
	{
		return false;
	}
	
	if (!MaterialSlot.UVCustomDataLayout.bEnabled)
	{
		return false;
	}
	
	return true;
}

bool FMaterialSlotBindingValidator::RequiresUVCustomData(
	const FMaterialSlotBinding& MaterialSlot
)
{
	if (!MaterialSlot.bEnabled)
	{
		return false;
	}

	switch (MaterialSlot.UVBehavior)
	{
	case EMaterialUVBehavior::IgnoreInstanceUV:
		return false;

	case EMaterialUVBehavior::BaseOnly:
		// BaseOnly + Default may resolve to identity UV data,
		// so a custom data layout is not strictly required.
		return MaterialSlot.UVBaseSource.SourceType !=
			EMaterialUVBaseSourceType::Default;

	case EMaterialUVBehavior::BaseAndRuntimeScale:
		return true;

	default:
		return false;
	}
}

bool FMaterialSlotBindingValidator::GetUVBlockRange(
	const FMaterialSlotBinding& MaterialSlot,
	int32& OutStartIndex,
	int32& OutEndIndex
)
{
	OutStartIndex = INDEX_NONE;
	OutEndIndex = INDEX_NONE;
	
	if (!UsesUVCustomData(MaterialSlot))
	{
		return false;
	}
	
	const int32 StartIndex = 
		MaterialSlot.UVCustomDataLayout.CustomDataStartIndex;
	
	if (StartIndex < 0)
	{
		return false;
	}
	
	OutStartIndex = StartIndex;
	// UV block size is fixed at 4.
	OutEndIndex = StartIndex + 3; 
	
	return true;
}

bool FMaterialSlotBindingValidator::DoRangesOverlap(
	const int32 AStart,
	const int32 AEnd,
	const int32 BStart,
	const int32 BEnd
)
{
	return !(AEnd < BStart || BEnd < AStart);
}


#endif