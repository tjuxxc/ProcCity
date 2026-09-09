#pragma once

#include "CoreMinimal.h"
#include "ProcCity/Material/DataAssets/MaterialAtlasMappingDataAsset.h"

#include "MaterialUVPolicyTypes.generated.h"


UENUM(BlueprintType)
enum class EMaterialUVBaseSourceType : uint8
{
	None = 0,
	
	// Base UVScale = (1, 1), Base UV Offset = (0, 0)
	Default,
	
	// Read UVScale / Offset from AtlasMapping DA
	AtlasMapping
};

UENUM(BlueprintType)
enum class EMaterialUVBehavior : uint8
{
	// Material ignores this UV custom data workflow entirely.
	IgnoreInstanceUV = 0,
	
	// Use only base UV values. Runtime coverage scale is ignored.
	BaseOnly,
	
	// Use base UV values and multiply ScaleU / ScaleV by runtime coverage.
	BaseAndRuntimeScale
};

USTRUCT(BlueprintType)
struct PROCCITY_API FMaterialUVCustomDataLayout
{
	GENERATED_BODY()

	// Whether this slot writes UV custom data at all.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material")
	bool bEnabled = false;

	// Semantic block identifier expected by the bound material / MI convention.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material", 
		meta=(EditCondition="bEnabled", EditConditionHides))
	FName BlockTag = NAME_None;
	
	// Start index in the instance custom data buffer.
	// Layout:
	//   [Start + 0] = UVScaleU
	//   [Start + 1] = UVScaleV
	//   [Start + 2] = UVOffsetU
	//   [Start + 3] = UVOffsetV
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material", 
		meta=(EditCondition="bEnabled", EditConditionHides, ClampMin=0))
	int32 CustomDataStartIndex = 0;
};

USTRUCT(BlueprintType)
struct PROCCITY_API FMaterialUVBaseSource
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material")
	EMaterialUVBaseSourceType SourceType =
		EMaterialUVBaseSourceType::Default;

	// Used only when SourceType == AtlasMapping
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material", 
		meta=(EditCondition="SourceType == EMaterialUVBaseSourceType::AtlasMapping", 
			EditConditionHides))
	TObjectPtr<UMaterialAtlasMappingDataAsset> AtlasMapping = nullptr;

	// Used only when SourceType == AtlasMapping
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material",
		meta=(EditCondition="SourceType == EMaterialUVBaseSourceType::AtlasMapping", 
			EditConditionHides, ClampMin=0))
	int32 AtlasElementIndex = 0;
};


