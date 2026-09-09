// Fill out your copyright notice in the Description page of Project Settings.


#include "ProcCity/Building/GenerationRules/BuildingRuleSet.h"
#include "ProcCity/Building/DataAssets/BuildingArchetypeDataAsset.h"
#include "ProcCity/Building/DataAssets/BuildingMaterialStyleDataAsset.h"


void UBuildingRuleSet::GenerateBuilding(
	const UBuildingArchetypeDataAsset* Archetype,
	const FBuildingGenerationInput& Input,  // Building origin, seed, module metrices.
	FGeneratedBuildingDescription& OutDescription
) const
{
	OutDescription = FGeneratedBuildingDescription();
	OutDescription.Metrics = Input.Metrics;

	FRandomStream RuleRandom(Input.RuleSeed);
	FRandomStream MaterialRandom(Input.MaterialSeed);

	OutDescription.Dimensions = GenerateDimensions(Archetype, RuleRandom);
	OutDescription.StyleIndex = PickMaterialStyleIndex(Archetype, MaterialRandom);

	GeneratePlacements(
		Archetype,
		Input,
		OutDescription.Dimensions,
		OutDescription.StyleIndex,
		RuleRandom,
		OutDescription.Placements
	);
}


int32 UBuildingRuleSet::PickMaterialStyleIndex(
	const UBuildingArchetypeDataAsset* Archetype,
	FRandomStream& RandomStream
) const
{
	if (!Archetype)
	{
		return INDEX_NONE;
	}

	return PickWeightedStyleIndex(Archetype->MaterialStyles, RandomStream);
}


int32 UBuildingRuleSet::PickWeightedStyleIndex(
	const TArray<TObjectPtr<UBuildingMaterialStyleDataAsset>>& Styles,
	FRandomStream& RandomStream
) const
{
	if (Styles.Num() == 0)
	{
		return INDEX_NONE;
	}

	int32 TotalWeight = 0;
	for (const UBuildingMaterialStyleDataAsset* Style : Styles)
	{
		if (IsValid(Style))
		{
			TotalWeight += FMath::Max(1, Style->Weight);
		}
	}

	if (TotalWeight == 0)
	{
		return INDEX_NONE;
	}

	const int32 Pick = RandomStream.RandRange(1, TotalWeight);
	int32 Running = 0;

	for (int32 Index = 0; Index < Styles.Num(); ++Index)
	{
		const UBuildingMaterialStyleDataAsset* Style = Styles[Index];
		if (!IsValid(Style))
		{
			continue;
		}

		Running += FMath::Max(1, Style->Weight);
		if (Pick <= Running)
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

FVector UBuildingRuleSet::GetPlacementPivotOffset(
	const UBuildingArchetypeDataAsset* Archetype,
	const FBuildingGenerationInput& Input,
	const FBuildingDimensions& Dimensions
) const
{
	return FVector::ZeroVector;
}











