// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ProcCity/Building/Types/BuildingGenerationTypes.h"
#include "BuildingRuleSet.generated.h"

class UBuildingArchetypeDataAsset;
class UBuildingMaterialStyleDataAsset;

UCLASS(Abstract, Blueprintable)
class PROCCITY_API UBuildingRuleSet : public UObject
{
	GENERATED_BODY()
	
public:

	virtual void GenerateBuilding(
		const UBuildingArchetypeDataAsset* Archetype,    // Building style
		const FBuildingGenerationInput& Input,           // Building origin, seed, module metrices.
		FGeneratedBuildingDescription& OutDescription    // Building dimension, module placements, etc.
	) const;

protected:

	virtual FBuildingDimensions GenerateDimensions(
		const UBuildingArchetypeDataAsset* Archetype,
		FRandomStream& RandomStream
	) const PURE_VIRTUAL(UBuildingRuleSet::GenerateDimensions, return FBuildingDimensions(););

	virtual int32 PickMaterialStyleIndex(
		const UBuildingArchetypeDataAsset* Archetype,
		FRandomStream& RandomStream
	) const;

	virtual void GeneratePlacements(
		const UBuildingArchetypeDataAsset* Archetype,
		const FBuildingGenerationInput& Input,        // Building location, seed, etc.
		const FBuildingDimensions& Dimensions,
		int32 StyleIndex,
		FRandomStream& RandomStream,
		TArray<FPlacedBuildingModule>& OutPlacements  // FPlacedBuildingModule: StyleIndex, Transform, and etc.
	) const PURE_VIRTUAL(UBuildingRuleSet::GeneratePlacements, );
	
	// Used for adjusting the building origin if necessary
	virtual FVector GetPlacementPivotOffset(
		const UBuildingArchetypeDataAsset* Archetype,
		const FBuildingGenerationInput& Input,
		const FBuildingDimensions& Dimensions
	) const;

protected:

	int32 PickWeightedStyleIndex(
		const TArray<TObjectPtr<UBuildingMaterialStyleDataAsset>>& Styles,  // Material styles, material role mapping.
		FRandomStream& RandomStream
	) const;

};
