// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BuildingEnums.generated.h"


UENUM(BlueprintType)
enum class EBuildingFacadeSide : uint8
{
	Front,
	Back,
	Left,
	Right
};

UENUM(BlueprintType)
enum class EBuildingSectionType : uint8
{
	Ground,
	Upper,
	Roof,
	Parapet,
	Corner
};

UENUM(BlueprintType)
enum class EBuildingMaterialRole : uint8
{
	Wall,
	WindowFrame,
	Glass,
	WallBase,
	DoorFrame,
	Roof,
	Parapet,
	Accent
};