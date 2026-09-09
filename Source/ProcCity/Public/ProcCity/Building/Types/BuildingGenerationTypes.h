// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProcCity/Building/Types/BuildingModuleTypes.h"

#include "BuildingGenerationTypes.generated.h"


USTRUCT(BlueprintType)
struct FBuildingDimensions
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dimensions", meta = (ClampMin = "1"))
	int32 BaysX = 7;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dimensions", meta = (ClampMin = "1"))
	int32 BaysY = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dimensions", meta = (ClampMin = "0"))
	int32 UpperFloors = 5;
};


USTRUCT(BlueprintType)
struct FBuildingDimensionConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dimensions", meta = (ClampMin = "1"))
	int32 BaysX = 11;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dimensions", meta = (ClampMin = "1"))
	int32 BaysY = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dimensions", meta = (ClampMin = "0"))
	int32 UpperFloors = 7;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dimensions", meta = (ClampMin = "0"))
	int32 ExtraUpperFloorsRandomRange = 2;
};


USTRUCT(BlueprintType)
struct FBuildingMetrics
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metrics")
	float BaySize = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metrics")
	float GroundHeight = 420.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metrics")
	float UpperHeight = 320.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metrics")
	float RoofThickness = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metrics")
	float WallThickness = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metrics")
	float ParapetThickness = 20.0f;
};


USTRUCT(BlueprintType)
struct FBuildingGenerationInput
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	FVector Origin = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	int32 RuleSeed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	int32 MaterialSeed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	FBuildingMetrics Metrics;
};


USTRUCT(BlueprintType)
struct FPlacedBuildingModule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	EBuildingSemanticSlot Slot = EBuildingSemanticSlot::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	FTransform Transform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	int32 StyleIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	FName DebugTag = NAME_None;
};


USTRUCT(BlueprintType)
struct FGeneratedBuildingDescription
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	FBuildingDimensions Dimensions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	FBuildingMetrics Metrics;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	int32 StyleIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	TArray<FPlacedBuildingModule> Placements;
};
