#pragma once

#include "CoreMinimal.h"
#include "BlockLayoutEnums.generated.h"

// ------------- General Spatial Semantics -------------
UENUM(BlueprintType)
enum class EStreetEdge : uint8
{
	North UMETA(DisplayName = "North"),
	East  UMETA(DisplayName = "East"),
	South UMETA(DisplayName = "South"),
	West  UMETA(DisplayName = "West")
};


// ----------------- Lot Semantics -----------------
UENUM(BlueprintType)
enum class ELotPositionType : uint8
{
	None = 0,
	Interior UMETA(DisplayName = "Interior"),
	Edge     UMETA(DisplayName = "Edge"),
	Corner   UMETA(DisplayName = "Corner")
};

UENUM(BlueprintType)
enum class ELotFrontageSide : uint8
{
	None = 0,
	South,
	North,
	West,
	East,
	InteriorLane
};




