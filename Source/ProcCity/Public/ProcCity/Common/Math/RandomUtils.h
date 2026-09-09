#pragma once

#include "CoreMinimal.h"

namespace ProcCity
{
	FORCEINLINE int32 DeriveSeed(const int32 DomainSeed, const int32 Salt)
	{
		return static_cast<int32>(
			HashCombine(::GetTypeHash(DomainSeed), ::GetTypeHash(Salt))
			);
	}
}
