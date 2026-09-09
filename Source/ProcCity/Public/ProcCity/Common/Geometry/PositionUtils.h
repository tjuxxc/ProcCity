#pragma once

#include "CoreMinimal.h"

namespace ProcCity
{
	FORCEINLINE FVector GetRectCenter3D(const FBox2D& Box, const float Z)
	{
		const FVector2D Center2D = (Box.Min + Box.Max) * 0.5f;
		return FVector(Center2D.X, Center2D.Y, Z);
	}
}