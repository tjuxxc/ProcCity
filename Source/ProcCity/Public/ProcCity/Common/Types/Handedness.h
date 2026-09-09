#pragma once

#include "CoreMinimal.h"
#include "Handedness.generated.h"

UENUM(BlueprintType)
enum class EHandedness : uint8
{
	None = 0,
	Default,
	Flipped
};