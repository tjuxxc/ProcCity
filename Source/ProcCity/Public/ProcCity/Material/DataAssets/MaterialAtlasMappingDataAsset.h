#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "MaterialAtlasMappingDataAsset.generated.h"

USTRUCT(BlueprintType)
struct PROCCITY_API FAtlasElementEntry
{
	GENERATED_BODY()
	
	// Semantic atlas element id referenced by material slot binding / module data.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material")
	int32 AtlasElementIndex = 0;
	
	// Base UV scale for this atlas element.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material")
	float BaseScaleU = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material")
	float BaseScaleV = 1.0f;

	// Base UV offset for this atlas element.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material")
	float BaseOffsetU = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material")
	float BaseOffsetV = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material")
	bool bEnabled = true;
};

UCLASS(BlueprintType)
class PROCCITY_API UMaterialAtlasMappingDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	// Atlas/base UV entries keyed by AtlasElementIndex.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Material")
	TArray<FAtlasElementEntry> AtlasElementEntries;
	

	const FAtlasElementEntry* 
		FindAtlasElementEntry(int32 ElementIndex) const;
	
#if WITH_EDITOR
	
	virtual EDataValidationResult IsDataValid(
		FDataValidationContext& Context
	) const override;
	
#endif
};