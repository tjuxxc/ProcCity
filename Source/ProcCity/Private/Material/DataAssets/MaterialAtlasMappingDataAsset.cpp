#include "ProcCity/Material/DataAssets/MaterialAtlasMappingDataAsset.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

const FAtlasElementEntry* 
	UMaterialAtlasMappingDataAsset::FindAtlasElementEntry(
		const int32 ElementIndex
	) const
{
	for (const FAtlasElementEntry& Entry : AtlasElementEntries)
	{
		if (!Entry.bEnabled)
		{
			continue;
		}
		
		if (Entry.AtlasElementIndex == ElementIndex)
		{
			return &Entry;
		}
	}
	return nullptr;
}

#if WITH_EDITOR
EDataValidationResult UMaterialAtlasMappingDataAsset::IsDataValid(
	FDataValidationContext& Context
) const
{
	bool bHasErrors = false;
	
	if (AtlasElementEntries.Num() == 0)
	{
		Context.AddWarning(FText::FromString(FString::Printf(
			TEXT("%s: AtlasElementEntries is empty."),
			*GetName()
		)));
	}
	
	TSet<int32> SeenElementIndices;
	for (int32 EntryIndex = 0; EntryIndex < AtlasElementEntries.Num(); ++EntryIndex)
	{
		const FAtlasElementEntry& Entry = AtlasElementEntries[EntryIndex];
		
		if (!Entry.bEnabled)
		{
			continue;
		}
		
		// Check duplicate element indices
		if (SeenElementIndices.Contains(Entry.AtlasElementIndex))
		{
			Context.AddError(FText::FromString(FString::Printf(
				TEXT("%s: AtlasElementEntries[%d] has duplicate AtlasElementIndex %d."),
				*GetName(),
				EntryIndex,
				Entry.AtlasElementIndex
			)));
			
			bHasErrors = true;
		}
		else
		{
			SeenElementIndices.Add(Entry.AtlasElementIndex);
		}
		
		if (Entry.AtlasElementIndex < 0)
		{
			Context.AddError(FText::FromString(FString::Printf(
				TEXT("%s: AtlasElementEntries[%d] has invalid AtlasElementIndex %d. Expected >= 0."),
				*GetName(),
				EntryIndex,
				Entry.AtlasElementIndex
			)));
			bHasErrors = true;
		}
		
		if (Entry.BaseScaleU <= 0.0f)
		{
			Context.AddError(FText::FromString(FString::Printf(
				TEXT("%s: AtlasElementEntries[%d] (AtlasElementIndex=%d) has invalid BaseScaleU %f. Expected > 0."),
				*GetName(),
				EntryIndex,
				Entry.AtlasElementIndex,
				Entry.BaseScaleU
			)));
			bHasErrors = true;
		}

		if (Entry.BaseScaleV <= 0.0f)
		{
			Context.AddError(FText::FromString(FString::Printf(
				TEXT("%s: AtlasElementEntries[%d] (AtlasElementIndex=%d) has invalid BaseScaleV %f. Expected > 0."),
				*GetName(),
				EntryIndex,
				Entry.AtlasElementIndex,
				Entry.BaseScaleV
			)));
			bHasErrors = true;
		}
		
		// // BaseOffsetU / BaseOffsetV may legitimately be any float, 
		// so no strict validation here.
	}
	
	return bHasErrors ?
		EDataValidationResult::Invalid : 
		EDataValidationResult::Valid;
}
#endif


