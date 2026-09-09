#pragma once

#include "CoreMinimal.h"
#include "ResolvedMaterialPayload.generated.h"


USTRUCT(BlueprintType)
struct PROCCITY_API FResolvedMaterialPayload
{
	GENERATED_BODY()
	
	// Final material array applied to mesh slots.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material")
	TArray<TObjectPtr<UMaterialInterface>> Materials;
	
	// Final per-instance custom data passed to HISM/ISM materials.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material")
	TArray<float> PerInstanceCustomData;
	
	void Reset()
	{
		Materials.Reset();
		PerInstanceCustomData.Reset();
	}
};
