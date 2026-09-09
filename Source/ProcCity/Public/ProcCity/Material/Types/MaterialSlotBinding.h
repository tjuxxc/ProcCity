#pragma once

#include "CoreMinimal.h"
#include "ProcCity/Material/Types/MaterialUVPolicyTypes.h"

#include "MaterialSlotBinding.generated.h"


USTRUCT(BlueprintType)
struct PROCCITY_API FMaterialSlotBinding
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material")
	bool bEnabled = true;
	
	// Which mesh material slot this binding targets.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material")
	int32 MaterialSlotIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material")
	TObjectPtr<UMaterialInterface> Material = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material")
	EMaterialUVBehavior UVBehavior =
		EMaterialUVBehavior::IgnoreInstanceUV;
	
	// Where base UV values come from.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material", 
		meta=(EditCondition="UVBehavior != EMaterialUVBehavior::IgnoreInstanceUV", 
			EditConditionHides))
	FMaterialUVBaseSource UVBaseSource;
	
	// Where this slot writes UV custom data in the per-instance float array.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material", 
		meta=(EditCondition="UVBehavior != EMaterialUVBehavior::IgnoreInstanceUV", 
			EditConditionHides))
	FMaterialUVCustomDataLayout UVCustomDataLayout;
};
