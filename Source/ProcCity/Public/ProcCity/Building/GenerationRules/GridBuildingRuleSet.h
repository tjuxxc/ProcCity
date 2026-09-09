// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProcCity/Building/GenerationRules/BuildingRuleSet.h"
#include "ProcCity/Building/Types/BuildingRuleTypes.h"

#include "GridBuildingRuleSet.generated.h"

/**
 * 
 */
UCLASS(Abstract, Blueprintable)
class PROCCITY_API UGridBuildingRuleSet : public UBuildingRuleSet
{
	GENERATED_BODY()
	
protected:

	virtual void GeneratePlacements(
		const UBuildingArchetypeDataAsset* Archetype,
		const FBuildingGenerationInput& Input,
		const FBuildingDimensions& Dimensions,
		int32 StyleIndex,
		FRandomStream& RandomStream,
		TArray<FPlacedBuildingModule>& OutPlacements
	) const override;

	// Pure virtual
	virtual EBuildingSemanticSlot ResolveGroundSlot(
		const FFacadeCellContext& Context,
		FRandomStream& RandomStream
	) const PURE_VIRTUAL(
		UGridBuildingRuleSet::ResolveGroundSlot, 
		return EBuildingSemanticSlot::None;
	);

	// Pure virtual
	virtual EBuildingSemanticSlot ResolveUpperSlot(
		const FFacadeCellContext& Context,
		FRandomStream& RandomStream
	) const PURE_VIRTUAL(
		UGridBuildingRuleSet::ResolveUpperSlot, 
		return EBuildingSemanticSlot::None;
	);

	virtual bool ShouldPlaceGroundCorners() const { return true; }
	virtual bool ShouldPlaceUpperCorners() const { return true; }
	virtual bool ShouldPlaceRoof() const { return true; }
	virtual bool ShouldPlaceParapet() const { return true; }
	virtual bool ShouldPlaceParapetCorners() const { return true; }

protected:

	float GetRoofBaseZ(int32 UpperFloors, float GroundHeight, 
		float UpperHeight, bool bIncludeGround) const;

	void EmitEdgePlacements(
		const FVector& Start,
		const FVector& StepDir,
		int32 Count,
		float BaseZ,
		float YawDeg,
		EBuildingFacadeSide Side,
		bool bIsGround,
		int32 FloorIndex,
		int32 UpperFloorCount,
		int32 StyleIndex,
		float BaySize,
		FRandomStream& RandomStream,
		TArray<FPlacedBuildingModule>& OutPlacements
	) const;


	void EmitRoofPlacements(
		const FVector& Origin,
		int32 BaysX,
		int32 BaysY,
		float BaySize,
		int32 UpperFloors,
		float GroundHeight,
		float UpperHeight, 
		bool bIncludeGround,
		int32 StyleIndex,
		TArray<FPlacedBuildingModule>& OutPlacements
	) const;


	void EmitCornerPlacements(
		const FVector& BaseOrigin,
		int32 BaysX,
		int32 BaysY,
		float BaySize,
		int32 UpperFloors,
		float GroundHeight,
		float UpperHeight,
		float RoofThickness,
		bool bIncludeGround,
		bool bIncludeRoof,
		int32 StyleIndex,
		TArray<FPlacedBuildingModule>& OutPlacements
	) const;

	// Used for adjusting the building origin if necessary
	virtual FVector GetPlacementPivotOffset(
		const UBuildingArchetypeDataAsset* Archetype,
		const FBuildingGenerationInput& Input,
		const FBuildingDimensions& Dimensions
	) const override;
};
