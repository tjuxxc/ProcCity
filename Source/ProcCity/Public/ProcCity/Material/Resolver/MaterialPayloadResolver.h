#pragma once

#include "CoreMinimal.h"
#include "ProcCity/Material/Types/MaterialSlotBinding.h"
#include "ProcCity/Material/Types/ResolvedMaterialContext.h"
#include "ProcCity/Material/Types/ResolvedMaterialPayload.h"

class UMaterialInterface;

struct PROCCITY_API FMaterialPayloadResolver
{
public:
	static bool ResolveMaterialPayload(
		const TArray<FMaterialSlotBinding>& MaterialSlots,
		const FResolvedMaterialContext& Context,
		FResolvedMaterialPayload& OutPayload // OutPayload will be cleared before write
	);

private:
	static bool ResolveMaterials(
		const TArray<FMaterialSlotBinding>& MaterialSlots,
		TArray<TObjectPtr<UMaterialInterface>>& OutMaterials // OutPayload will be cleared before write
	);
};