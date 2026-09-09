// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BuildingPlanTypes.generated.h"

class UBuildingArchetypeDataAsset;

USTRUCT(BlueprintType)
struct FBuildingPlan
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	TObjectPtr<UBuildingArchetypeDataAsset> Archetype = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	int32 BuildingIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	FVector FinalOrigin = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	FRotator FinalRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	int32 BaseSeed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	int32 ArchetypeSeed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	int32 RuleSeed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	int32 ModuleSeed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	int32 MaterialSeed = 0;
	
	// Used in Block Generator
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	int32 SourceLotIndex = INDEX_NONE;
};