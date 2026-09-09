// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProcCity/Building/Types/BuildingEnums.h"

#include "BuildingModuleTypes.generated.h"


class UStaticMesh;

UENUM(BlueprintType)
enum class EBuildingSemanticSlot : uint8
{
	None,

	GroundSolid UMETA(DisplayName = "Ground Solid"),
	GroundDoor UMETA(DisplayName = "Ground Door"),	
	GroundWindow UMETA(DisplayName = "Ground Window"),
	GroundCorner UMETA(DisplayName = "Ground Corner"),

	UpperSolid UMETA(DisplayName = "Upper Solid"),
	UpperWindow UMETA(DisplayName = "Upper Window"),
	UpperNarrowWindow UMETA(DisplayName = "Upper Narrow Window"),
	UpperCorner UMETA(DisplayName = "Upper Corner"),

	RoofFlat UMETA(DisplayName = "Roof Flat"),
	ParapetStraight UMETA(DisplayName = "Parapet Straight"),
	ParapetCorner UMETA(DisplayName = "Parapet Corner")
};


USTRUCT(BlueprintType)
struct FBuildingModuleVariant
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Module")
	TObjectPtr<UStaticMesh> Mesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Module")
	TArray<EBuildingMaterialRole> SlotRoles;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Module", meta = (ClampMin = "1"))
	int32 Weight = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Module")
	FName VariantTag = NAME_None;
};


USTRUCT(BlueprintType)
struct FBuildingSlotVariantCollection
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Module")
	EBuildingSemanticSlot Slot = EBuildingSemanticSlot::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Module")
	TArray<FBuildingModuleVariant> Variants;
};