#include "ProcCity/Material/Resolver/MaterialCustomDataResolver.h"

#include "ProcCity/Material/Types/MaterialSlotBinding.h"
#include "ProcCity/Material/Types/ResolvedMaterialContext.h"
#include "ProcCity/Material/DataAssets/MaterialAtlasMappingDataAsset.h"


bool FMaterialCustomDataResolver::ResolveCustomData(
	const TArray<FMaterialSlotBinding>& MaterialSlots,
	const FResolvedMaterialContext& Context,
	TArray<float>& OutCustomData
)
{
	OutCustomData.Reset();
	
	// Current implementation only supports the UV custom data branch.
	if (!ResolveCustomUVData(MaterialSlots, Context, OutCustomData))
	{
		return false;
	}
	
	/* TODO: Future branches can extend here, for example:
		if (!ResolveCustomFooData(MaterialSlots, Context, OutCustomData))
		{
			return false;
		} 
	*/ 
	
	return true;
}

bool FMaterialCustomDataResolver::ResolveCustomUVData(
	const TArray<FMaterialSlotBinding>& MaterialSlots,
	const FResolvedMaterialContext& Context,
	TArray<float>& InOutCustomData
)
{
	const int32 RequiredCustomDataCount =
		ComputeRequiredUVCustomDataCount(MaterialSlots);
	
	if (RequiredCustomDataCount <= 0)
	{
		return true;
	}
	
	InOutCustomData.SetNumZeroed(
		FMath::Max(RequiredCustomDataCount, MaterialSlots.Num()));
	
	for (const FMaterialSlotBinding& MaterialSlot : MaterialSlots)
	{
		if (!MaterialSlot.bEnabled)
		{
			continue;
		}
		
		if (!ResolveSlotCustomUVData(MaterialSlot, Context,
			InOutCustomData))
		{
			return false;
		}
	}
	return true;
}

bool FMaterialCustomDataResolver::ResolveSlotCustomUVData(
	const FMaterialSlotBinding& MaterialSlot,
	const FResolvedMaterialContext& Context,
	TArray<float>& InOutCustomData
)
{
	if (!ShouldResolveSlotCustomUVData(MaterialSlot))
	{
		return true;
	}
	
	// Resolve custom uv data and optionally apply runtime uv scale
	FResolvedUVData ResolvedUVData;
	
	switch (MaterialSlot.UVBehavior)
	{
	case EMaterialUVBehavior::BaseOnly:
		if (!ResolveSlotBaseUVData(MaterialSlot, ResolvedUVData))
		{
			return false;
		}
		break;
		
	case EMaterialUVBehavior::BaseAndRuntimeScale:
		if (!ResolveSlotBaseUVData(MaterialSlot, ResolvedUVData))
		{
			return false;
		}
		
		ApplySlotRuntimeUVScale(Context, ResolvedUVData);
		break;
		
	case EMaterialUVBehavior::IgnoreInstanceUV:
		// IgnoreInstanceUV should already be filtered out by 
		// ShouldResolveSlotCustomUVData.
	default:
		return false;
	}
	
	// // BaseOnly + identity UV data does not require an explicit custom data write.
	if (!ShouldWriteResolvedSlotCustomUVData(
		MaterialSlot,
		ResolvedUVData))
	{
		return true;
	}
	
	const FMaterialUVCustomDataLayout& Layout = 
		MaterialSlot.UVCustomDataLayout;
	
	if (Layout.CustomDataStartIndex < 0)
	{
		return false;
	}
	
	// Current UV custom data branch uses a fixed 4-float block layout:
	//   [Start + 0] = UVScaleU
	//   [Start + 1] = UVScaleV
	//   [Start + 2] = UVOffsetU
	//   [Start + 3] = UVOffsetV
	// TODO: This logic should be checked again if other CustomData types are included. 
	if (Layout.CustomDataStartIndex + 3 >= InOutCustomData.Num())
	{
		return false;
	}
	
	// Write custom uv data to InOutCustomData array
	WriteSlotCustomUVData(
		Layout,
		ResolvedUVData,
		InOutCustomData
	);
	
	return true;
}

bool FMaterialCustomDataResolver::ResolveSlotBaseUVData(
	const FMaterialSlotBinding& MaterialSlot,
	FResolvedUVData& OutUVData
)
{
	switch (MaterialSlot.UVBaseSource.SourceType)
	{
	case EMaterialUVBaseSourceType::None:
		return false;
		
	case EMaterialUVBaseSourceType::Default:
		return ResolveSlotDefaultBaseUVData(OutUVData);
		
	case EMaterialUVBaseSourceType::AtlasMapping:
		return ResolveSlotAtlasBaseUVData(MaterialSlot, OutUVData);
		
	default:
		return false;
	}
}

bool FMaterialCustomDataResolver::ResolveSlotDefaultBaseUVData(
	FResolvedUVData& OutUVData
)
{
	OutUVData.ScaleU = 1.0f;
	OutUVData.ScaleV = 1.0f;
	OutUVData.OffsetU = 0.0f;
	OutUVData.OffsetV = 0.0f;
	return true;
}

bool FMaterialCustomDataResolver::ResolveSlotAtlasBaseUVData(
	const FMaterialSlotBinding& MaterialSlot,
	FResolvedUVData& OutUVData
)
{
	if (!MaterialSlot.UVBaseSource.AtlasMapping)
	{
		return false;
	}
	
	const FAtlasElementEntry* AtlasEntry =
		MaterialSlot.UVBaseSource.AtlasMapping->FindAtlasElementEntry(
			MaterialSlot.UVBaseSource.AtlasElementIndex
		);
	
	if (!AtlasEntry)
	{
		return false;
	}
	
	OutUVData.ScaleU = AtlasEntry->BaseScaleU;
	OutUVData.ScaleV = AtlasEntry->BaseScaleV;
	OutUVData.OffsetU = AtlasEntry->BaseOffsetU;
	OutUVData.OffsetV = AtlasEntry->BaseOffsetV;

	return true;
}

void FMaterialCustomDataResolver::ApplySlotRuntimeUVScale(
	const FResolvedMaterialContext& Context,
	FResolvedUVData& InOutUVData
)
{
	InOutUVData.ScaleU *= Context.Metrics.CoverageScaleU;
	InOutUVData.ScaleV *= Context.Metrics.CoverageScaleV;
}
	

int32 FMaterialCustomDataResolver::ComputeRequiredUVCustomDataCount(
	const TArray<FMaterialSlotBinding>& MaterialSlots
)
{
	// TODO: This function should be checked again if other CustomData types are included. 
	int32 MaxCustomDataIndex = INDEX_NONE;
	
	for (const FMaterialSlotBinding& MaterialSlot : MaterialSlots)
	{
		if (!ShouldResolveSlotCustomUVData(MaterialSlot))
		{
			continue;
		}

		const int32 StartIndex =
			MaterialSlot.UVCustomDataLayout.CustomDataStartIndex;

		if (StartIndex < 0)
		{
			continue;
		}

		// Fixed UV block layout:
		// [Start + 0] = ScaleU
		// [Start + 1] = ScaleV
		// [Start + 2] = OffsetU
		// [Start + 3] = OffsetV
		const int32 EndIndex = StartIndex + 3;

		MaxCustomDataIndex = FMath::Max(MaxCustomDataIndex, EndIndex);
	}

	return (MaxCustomDataIndex == INDEX_NONE) ? 0 : (MaxCustomDataIndex + 1);
}

bool FMaterialCustomDataResolver::ShouldResolveSlotCustomUVData(
	const FMaterialSlotBinding& MaterialSlot
)
{
	if (!MaterialSlot.bEnabled)
	{
		return false;
	}

	if (!MaterialSlot.UVCustomDataLayout.bEnabled)
	{
		return false;
	}

	if (MaterialSlot.UVBehavior == EMaterialUVBehavior::IgnoreInstanceUV)
	{
		return false;
	}

	return true;
}

bool FMaterialCustomDataResolver::ShouldWriteResolvedSlotCustomUVData(
	const FMaterialSlotBinding& MaterialSlot,
	const FResolvedUVData& ResolvedUVData
)
{
	if (MaterialSlot.UVBehavior == EMaterialUVBehavior::IgnoreInstanceUV)
	{
		return false;
	}

	// BaseOnly + identity UV data does not require an explicit custom data write.
	if (MaterialSlot.UVBehavior == EMaterialUVBehavior::BaseOnly &&
		IsIdentityResolvedUVData(ResolvedUVData))
	{
		return false;
	}

	return true;
}

bool FMaterialCustomDataResolver::IsIdentityResolvedUVData(
	const FResolvedUVData& ResolvedUVData
)
{
	return
		FMath::IsNearlyEqual(ResolvedUVData.ScaleU, 1.0f) &&
		FMath::IsNearlyEqual(ResolvedUVData.ScaleV, 1.0f) &&
		FMath::IsNearlyEqual(ResolvedUVData.OffsetU, 0.0f) &&
		FMath::IsNearlyEqual(ResolvedUVData.OffsetV, 0.0f);
}

void FMaterialCustomDataResolver::WriteSlotCustomUVData(
	const FMaterialUVCustomDataLayout& Layout,
	const FResolvedUVData& UVData,
	TArray<float>& InOutCustomData
)
{
	const int32 StartIndex = Layout.CustomDataStartIndex;
	
	// Fixed UV custom data block layout:
	//   [Start + 0] = UVScaleU
	//   [Start + 1] = UVScaleV
	//   [Start + 2] = UVOffsetU
	//   [Start + 3] = UVOffsetV
	//
	// BlockTag is not interpreted here at runtime. It exists to help
	// authoring, debugging, and editor-side validation of block usage.
	InOutCustomData[StartIndex] = UVData.ScaleU;
	InOutCustomData[StartIndex + 1] = UVData.ScaleV;
	InOutCustomData[StartIndex + 2] = UVData.OffsetU;
	InOutCustomData[StartIndex + 3] = UVData.OffsetV;
}

















