// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

template<typename First, typename... Rest>
FORCEINLINE uint32 
HashValues(const First& FirstValue, const Rest&... Values)
{
	uint32 Hash = GetTypeHash(FirstValue);
	((Hash = HashCombine(Hash, GetTypeHash(Values))), ...);
	return Hash;
}




















