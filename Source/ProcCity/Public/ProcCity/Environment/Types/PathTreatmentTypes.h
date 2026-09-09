#pragma once

#include "CoreMinimal.h"

#include "PathTreatmentTypes.generated.h"

UENUM(BlueprintType)
enum class EBlockPathTreatmentType : uint8
{
	None = 0,
	Corner90,
	Corner180,
	EndCap,
	EndFlare,
	Custom
};

USTRUCT(BlueprintType)
struct PROCCITY_API FPathTerminalTreatment
{
	GENERATED_BODY()
	
	// TODO:
	// The current definition only supports customized end treatment.
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	bool bEnabled = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	EBlockPathTreatmentType TreatmentType = EBlockPathTreatmentType::None;
	
	// Placement origin of the treatment module / patch
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	FVector LocalOrigin = FVector::ZeroVector;
	
	// Optional radius hint (e.g. sidewalk corner arc radius)
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	// float Radius = 0.0f;
	
	// Yaw rotation in block-local space
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	float RotationDeg = 0.0f;
	
	// How much the owning segment should retreat from the endpoint
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	float TrimLength = 0.0f;
};
