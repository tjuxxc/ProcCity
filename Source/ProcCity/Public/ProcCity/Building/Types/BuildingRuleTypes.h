// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProcCity/Building/Types/BuildingEnums.h"
#include "BuildingRuleTypes.generated.h"


USTRUCT(BlueprintType)
struct FFacadeCellContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule")
	EBuildingFacadeSide Side = EBuildingFacadeSide::Front;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule")
	int32 FloorIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule")
	int32 BayIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule")
	int32 BayCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule")
	int32 UpperFloorCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule")
	bool bIsGround = false;
};


USTRUCT(BlueprintType)
struct FCornerContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule")
	int32 UpperFloorCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule")
	bool bIncludeGround = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rule")
	bool bIncludeRoof = true;
};