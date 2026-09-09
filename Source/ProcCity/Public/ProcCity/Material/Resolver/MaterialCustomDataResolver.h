#pragma once

#include "CoreMinimal.h"

struct FMaterialSlotBinding;
struct FResolvedMaterialContext;
struct FMaterialUVCustomDataLayout;


struct PROCCITY_API FMaterialCustomDataResolver
{
public:
	// Top-level entry for resolving all supported per-instance custom data.
	// Current implementation resolves only the UV custom data branch.
	static bool ResolveCustomData(
		const TArray<FMaterialSlotBinding>& MaterialSlots,
		const FResolvedMaterialContext& Context,
		TArray<float>& OutCustomData
	);

private:
	// Internal resolved UV values for one material slot.
	// This is a transient evaluation struct and is intentionally kept local
	// to the resolver instead of being part of public authoring types.
	struct FResolvedUVData
	{
		float ScaleU = 1.0f;
		float ScaleV = 1.0f;
		float OffsetU = 0.0f;
		float OffsetV = 0.0f;
	};

private:
	// UV custom data branch
	static bool ResolveCustomUVData(
		const TArray<FMaterialSlotBinding>& MaterialSlots,
		const FResolvedMaterialContext& Context,
		TArray<float>& InOutCustomData
	);

	static int32 ComputeRequiredUVCustomDataCount(
		const TArray<FMaterialSlotBinding>& MaterialSlots
	);

	static bool ResolveSlotCustomUVData(
		const FMaterialSlotBinding& MaterialSlot,
		const FResolvedMaterialContext& Context,
		TArray<float>& InOutCustomData
	);

	static bool ResolveSlotBaseUVData(
		const FMaterialSlotBinding& MaterialSlot,
		FResolvedUVData& OutUVData
	);

	static bool ResolveSlotDefaultBaseUVData(
		FResolvedUVData& OutUVData
	);

	static bool ResolveSlotAtlasBaseUVData(
		const FMaterialSlotBinding& MaterialSlot,
		FResolvedUVData& OutUVData
	);

	static void ApplySlotRuntimeUVScale(
		const FResolvedMaterialContext& Context,
		FResolvedUVData& InOutUVData
	);

	static bool ShouldResolveSlotCustomUVData(
		const FMaterialSlotBinding& MaterialSlot
	);
	
	static bool ShouldWriteResolvedSlotCustomUVData(
		const FMaterialSlotBinding& MaterialSlot,
		const FResolvedUVData& ResolvedUVData
	);

	static bool IsIdentityResolvedUVData(
		const FResolvedUVData& ResolvedUVData
	);

	static void WriteSlotCustomUVData(
		const FMaterialUVCustomDataLayout& Layout,
		const FResolvedUVData& UVData,
		TArray<float>& InOutCustomData
	);
};