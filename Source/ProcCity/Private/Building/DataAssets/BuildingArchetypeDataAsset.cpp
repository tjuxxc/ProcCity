// Fill out your copyright notice in the Description page of Project Settings.


#include "ProcCity/Building/DataAssets/BuildingArchetypeDataAsset.h"


UBuildingArchetypeDataAsset::UBuildingArchetypeDataAsset()
	: DefaultDimension{
		.BaysX = 11,
		.BaysY = 4,
		.UpperFloors = 7,
		.ExtraUpperFloorsRandomRange = 3
	}
	, DefaultMetrics{
		.BaySize = 400.0f,
		.GroundHeight = 420.0f,
		.UpperHeight = 320.0f,
		.RoofThickness = 15.0f,
		.WallThickness = 30.0f,
		.ParapetThickness = 20.0f
	}
{
	// Blank
}