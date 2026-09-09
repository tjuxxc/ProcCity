// Fill out your copyright notice in the Description page of Project Settings.


#include "ProcCity/Building/GenerationRules/ResidentialBuildingRuleSet.h"
#include "ProcCity/Building/DataAssets/BuildingArchetypeDataAsset.h"


FBuildingDimensions UResidentialBuildingRuleSet::GenerateDimensions(
	const UBuildingArchetypeDataAsset* Archetype,
	FRandomStream& RandomStream
) const
{
	FBuildingDimensions Dim;

	if (IsValid(Archetype))
	{
		Dim.BaysX = Archetype->DefaultDimension.BaysX;
		Dim.BaysY = Archetype->DefaultDimension.BaysY;
		int32 UpperFloors = Archetype->DefaultDimension.UpperFloors;

		const int32 MaxExtraUpperFloors
			= Archetype->DefaultDimension.ExtraUpperFloorsRandomRange;

		if (MaxExtraUpperFloors > 0)
		{
			UpperFloors += RandomStream.RandRange(0, MaxExtraUpperFloors);
		}

		Dim.UpperFloors = UpperFloors;
	}

	return Dim;
}


// RandomStream is not used in this implementation, but it can be 
// used to introduce variability in the future.
EBuildingSemanticSlot UResidentialBuildingRuleSet::ResolveGroundSlot(
	const FFacadeCellContext& Context,
	FRandomStream& RandomStream
) const
{
	const bool bFrontOrBack =
		Context.Side == EBuildingFacadeSide::Front ||
		Context.Side == EBuildingFacadeSide::Back;

	if (bFrontOrBack)
	{
		const int32 CenterIndex = (Context.BayCount - 1) / 2;
		if (Context.BayIndex == CenterIndex)
		{
			return EBuildingSemanticSlot::GroundDoor;
		}
		return EBuildingSemanticSlot::GroundWindow;
	}

	return EBuildingSemanticSlot::GroundSolid;
}


// RandomStream is not used in this implementation, but it can be 
// used to introduce variability in the future.
EBuildingSemanticSlot UResidentialBuildingRuleSet::ResolveUpperSlot(
	const FFacadeCellContext& Context,
	FRandomStream& RandomStream
) const
{
	const bool bFrontOrBack =
		Context.Side == EBuildingFacadeSide::Front ||
		Context.Side == EBuildingFacadeSide::Back;

	if (bFrontOrBack)
	{
		const int32 CenterIndex = (Context.BayCount - 1) / 2;
		if (Context.BayIndex == CenterIndex)
		{
			return EBuildingSemanticSlot::UpperNarrowWindow;
		}
		return EBuildingSemanticSlot::UpperWindow;
	}

	return EBuildingSemanticSlot::UpperSolid;
}






