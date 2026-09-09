// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SpatialTypes.generated.h"


UENUM(BlueprintType)
enum class EZoneType: uint8
{
	None = 0,
	Residential UMETA(DisplayName = "Residential"),
	MixedUse    UMETA(DisplayName = "Mixed Use"),
	Office      UMETA(DisplayName = "Office"),
	Commercial  UMETA(DisplayName = "Commercial"),
	Industrial  UMETA(DisplayName = "Industrial")
};






















