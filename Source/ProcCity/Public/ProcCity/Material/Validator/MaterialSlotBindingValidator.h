#pragma once

#include "CoreMinimal.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

struct FMaterialSlotBinding;

struct PROCCITY_API FMaterialSlotBindingValidator
{
#if WITH_EDITOR
public:
	// Validate a material slot binding array owned by some module/asset.
	// OwnerLabel is used only for readable validation messages, for example:
	//   "SurfaceModules[0]"
	//   "PathSegmentModules[2]"
	static EDataValidationResult ValidateMaterialSlots(
		const TArray<FMaterialSlotBinding>& MaterialSlots,
		const FString& OwnerLabel,
		FDataValidationContext& Context
	);
	
private:
	static bool ValidateSingleMaterialSlot(
		const FMaterialSlotBinding& MaterialSlot,
		int32 SlotBindIdx,
		const FString& OwnerLabel,
		FDataValidationContext& Context
	);

	static bool ValidateCrossSlotUsage(
		const TArray<FMaterialSlotBinding>& MaterialSlots,
		const FString& OwnerLabel,
		FDataValidationContext& Context
	);



	static bool GetUVBlockRange(
		const FMaterialSlotBinding& MaterialSlot,
		int32& OutStartIndex,
		int32& OutEndIndex
	);

	static bool DoRangesOverlap(
		int32 AStart,
		int32 AEnd,
		int32 BStart,
		int32 BEnd
	);
	
	// Whether uv custom data are actually needed.
	static bool RequiresUVCustomData(
		const FMaterialSlotBinding& MaterialSlot
	);
	
	// Whether uv custom data are enabled and have been written.
	// [Note]: For valid setting, RequiresUVCustomData and UsesUVCustomData 
	// should return same value.
	static bool UsesUVCustomData(
		const FMaterialSlotBinding& MaterialSlot
	);
#endif
};
