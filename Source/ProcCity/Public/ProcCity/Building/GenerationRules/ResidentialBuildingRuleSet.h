// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProcCity/Building/GenerationRules/GridBuildingRuleSet.h"

#include "ResidentialBuildingRuleSet.generated.h"

/**
 * 
 */
UCLASS()
class PROCCITY_API UResidentialBuildingRuleSet : public UGridBuildingRuleSet
{
	GENERATED_BODY()
	
protected:
	virtual FBuildingDimensions GenerateDimensions(
		const UBuildingArchetypeDataAsset* Archetype,
		FRandomStream& RandomStream
	) const override;

	virtual EBuildingSemanticSlot ResolveGroundSlot(
		const FFacadeCellContext& Context,
		FRandomStream& RandomStream
	) const override;

	virtual EBuildingSemanticSlot ResolveUpperSlot(
		const FFacadeCellContext& Context,
		FRandomStream& RandomStream
	) const override;

};
