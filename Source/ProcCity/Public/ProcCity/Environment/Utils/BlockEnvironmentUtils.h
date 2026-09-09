#pragma once

#include "CoreMinimal.h"
#include "ProcCity/Environment/Types/BlockEnvironmentEnums.h"

namespace ProcCity
{
	bool TryConvertWidthToPathWidthSpec(
		const float Width,
		EBlockPathWidthSpec& OutSpec,
		const float Tolerance = 1.0f
	);
	
	bool TryGetPathWidthFromWidthSpec(
		const EBlockPathWidthSpec Spec,
		float& OutWidth
	);
}