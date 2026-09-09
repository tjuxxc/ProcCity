// Fill out your copyright notice in the Description page of Project Settings.


#include "ProcCity/Building/GenerationRules/GridBuildingRuleSet.h"
#include "ProcCity/Building/DataAssets/BuildingArchetypeDataAsset.h"


float UGridBuildingRuleSet::GetRoofBaseZ(int32 UpperFloors, float GroundHeight,
	float UpperHeight, bool bIncludeGround) const
{
	float Z = 0.0f;

	if (bIncludeGround)
	{
		Z += GroundHeight;
	}
	Z += UpperFloors * UpperHeight;
	return Z;
}


void UGridBuildingRuleSet::GeneratePlacements(
	const UBuildingArchetypeDataAsset* Archetype,
	const FBuildingGenerationInput& Input,
	const FBuildingDimensions& Dimensions,
	int32 StyleIndex,
	FRandomStream& RandomStream,
	TArray<FPlacedBuildingModule>& OutPlacements
) const
{
	if (!Archetype)
	{
		return;
	}

	// Get module metrics, e.g., Bay size, height, thickness
	const FBuildingMetrics& Metrics = Input.Metrics;

	const float Width = Dimensions.BaysX * Metrics.BaySize;
	const float Depth = Dimensions.BaysY * Metrics.BaySize;
	
	// PivotOffset = [-BuildingWidth * 0.5, -BuildingDepth * 0.5, 0.0]
	const FVector PivotOffset = GetPlacementPivotOffset(Archetype, Input, Dimensions);
	// Offset input such that the building origin lies on the footprint center
	const FVector BaseOrigin = Input.Origin + PivotOffset;
	
	/*
	* Orientation convention:
	* West: X+; East: X-; North: Y+; South: Y-.
	*/
	const FVector SE = BaseOrigin;
	const FVector SW = BaseOrigin + FVector(Width, 0.0f, 0.0f);
	const FVector NE = BaseOrigin + FVector(0.0f, Depth, 0.0f);
	
	/*
	* The origin of the edge module (e.g. walls, doors) is at bottom center, thus a [XOffset]
	* or [YOffset] of half thickness is needed.
	* The first bay is always assumed be a part of the corner, thus an edge starts at 
	* [Metrics.BaySize * 1.5] along direction [XStepDir] or [YStepDir].
	*/
	const FVector XStepDir(1, 0, 0);
	const FVector YStepDir(0, 1, 0);

	FVector XOffset(Metrics.WallThickness / 2.0f, 0.0f, 0.0f);
	FVector YOffset(0.0f, Metrics.WallThickness / 2.0f, 0.0f);

	FVector SouthStart = SE + XStepDir * (Metrics.BaySize * 1.5f) + YOffset;
	FVector NorthStart = NE + XStepDir * (Metrics.BaySize * 1.5f) - YOffset;
	FVector WestStart = SW + YStepDir * (Metrics.BaySize * 1.5f) - XOffset;
	FVector EastStart = SE + YStepDir * (Metrics.BaySize * 1.5f) + XOffset;

	float Z = 0.0f;

	/*
	* UE's left-hand coordinate system is assumed.
	* An edge module's (e.g. walls, doors) front is assumed to be Y-.
	*/

	if (Archetype->bIncludeGround)
	{
		EmitEdgePlacements(SouthStart, XStepDir, Dimensions.BaysX - 2, Z, 0.0f,
			EBuildingFacadeSide::Front, true, 0, Dimensions.UpperFloors, StyleIndex,
			Metrics.BaySize, RandomStream, OutPlacements);

		EmitEdgePlacements(NorthStart, XStepDir, Dimensions.BaysX - 2, Z, 180.0f,
			EBuildingFacadeSide::Back, true, 0, Dimensions.UpperFloors, StyleIndex,
			Metrics.BaySize, RandomStream, OutPlacements);

		EmitEdgePlacements(WestStart, YStepDir, Dimensions.BaysY - 2, Z, 90.0f,
			EBuildingFacadeSide::Left, true, 0, Dimensions.UpperFloors, StyleIndex,
			Metrics.BaySize, RandomStream, OutPlacements);

		EmitEdgePlacements(EastStart, YStepDir, Dimensions.BaysY - 2, Z, -90.0f,
			EBuildingFacadeSide::Right, true, 0, Dimensions.UpperFloors, StyleIndex,
			Metrics.BaySize, RandomStream, OutPlacements);

		Z += Metrics.GroundHeight;
	}

	for (int32 Floor = 0; Floor < Dimensions.UpperFloors; ++Floor)
	{
		EmitEdgePlacements(SouthStart, XStepDir, Dimensions.BaysX - 2, Z, 0.0f,
			EBuildingFacadeSide::Front, false, Floor, Dimensions.UpperFloors, StyleIndex,
			Metrics.BaySize, RandomStream, OutPlacements);

		EmitEdgePlacements(NorthStart, XStepDir, Dimensions.BaysX - 2, Z, 180.0f,
			EBuildingFacadeSide::Back, false, Floor, Dimensions.UpperFloors, StyleIndex,
			Metrics.BaySize, RandomStream, OutPlacements);

		EmitEdgePlacements(WestStart, YStepDir, Dimensions.BaysY - 2, Z, 90.0f,
			EBuildingFacadeSide::Left, false, Floor, Dimensions.UpperFloors, StyleIndex,
			Metrics.BaySize, RandomStream, OutPlacements);

		EmitEdgePlacements(EastStart, YStepDir, Dimensions.BaysY - 2, Z, -90.0f,
			EBuildingFacadeSide::Right, false, Floor, Dimensions.UpperFloors, StyleIndex,
			Metrics.BaySize, RandomStream, OutPlacements);

		Z += Metrics.UpperHeight;
	}

	if (Archetype->bIncludeRoof && ShouldPlaceRoof())
	{
		EmitRoofPlacements(
			BaseOrigin, 
			Dimensions.BaysX, 
			Dimensions.BaysY, 
			Metrics.BaySize, 
			Dimensions.UpperFloors,
			Metrics.GroundHeight,
			Metrics.UpperHeight,
			Archetype->bIncludeGround,
			StyleIndex,
			OutPlacements
		);
	}

	if (Archetype->bIncludeRoof && ShouldPlaceParapet())
	{
		const float ParapetBaseZ = GetRoofBaseZ(
			Dimensions.UpperFloors, 
			Metrics.GroundHeight, 
			Metrics.UpperHeight, 
			Archetype->bIncludeGround
		) + Metrics.RoofThickness;

		XOffset = FVector(Metrics.ParapetThickness / 2.0f, 0.0f, 0.0f);
		YOffset = FVector(0.0f, Metrics.ParapetThickness / 2.0f, 0.0f);

		SouthStart = SE + XStepDir * (Metrics.BaySize * 1.5f) + YOffset;
		NorthStart = NE + XStepDir * (Metrics.BaySize * 1.5f) - YOffset;
		WestStart = SW + YStepDir * (Metrics.BaySize * 1.5f) - XOffset;
		EastStart = SE + YStepDir * (Metrics.BaySize * 1.5f) + XOffset;

		for (int32 i = 0; i < Dimensions.BaysX - 2; ++i)
		{
			OutPlacements.Add(
				{
					EBuildingSemanticSlot::ParapetStraight,
					FTransform(
						FRotator(0, 0, 0),
						SouthStart + XStepDir * (Metrics.BaySize * i) + FVector(0, 0, ParapetBaseZ), 
						FVector(1.0f)
					),
					StyleIndex
				}
			);
			OutPlacements.Add(
				{
					EBuildingSemanticSlot::ParapetStraight,
					FTransform(
						FRotator(0, 180, 0),
						NorthStart + XStepDir * (Metrics.BaySize * i) + FVector(0, 0, ParapetBaseZ),
						FVector(1.0f)),
					StyleIndex
				}
			);
		}

		for (int32 i = 0; i < Dimensions.BaysY - 2; ++i)
		{
			OutPlacements.Add(
				{
					EBuildingSemanticSlot::ParapetStraight,
					FTransform(
						FRotator(0, 90, 0),
						WestStart + YStepDir * (Metrics.BaySize * i) + FVector(0, 0, ParapetBaseZ),
						FVector(1.0f)),
					StyleIndex
				}
			);
			OutPlacements.Add(
				{
					EBuildingSemanticSlot::ParapetStraight,
					FTransform(
						FRotator(0, -90, 0),
						EastStart + YStepDir * (Metrics.BaySize * i) + FVector(0, 0, ParapetBaseZ),
						FVector(1.0f)),
					StyleIndex
				}
			);
		}
	}

	EmitCornerPlacements(
		BaseOrigin,
		Dimensions.BaysX,
		Dimensions.BaysY,
		Metrics.BaySize,
		Dimensions.UpperFloors,
		Metrics.GroundHeight,
		Metrics.UpperHeight,
		Metrics.RoofThickness,
		Archetype->bIncludeGround,
		Archetype->bIncludeRoof,
		StyleIndex,
		OutPlacements
	);
}


void UGridBuildingRuleSet::EmitEdgePlacements(
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
) const
{
	for (int32 i = 0; i < Count; ++i)
	{
		FFacadeCellContext Context;
		Context.Side = Side;
		Context.FloorIndex = FloorIndex;
		Context.BayIndex = i;
		Context.BayCount = Count;
		Context.UpperFloorCount = UpperFloorCount;
		Context.bIsGround = bIsGround;

		const EBuildingSemanticSlot Slot = bIsGround 
			? ResolveGroundSlot(Context, RandomStream) 
			: ResolveUpperSlot(Context, RandomStream);

		if (Slot == EBuildingSemanticSlot::None)
		{
			continue;
		}

		const FVector Pos = Start + StepDir * (BaySize * i) + FVector(0.0f, 0.0f, BaseZ);
		const FTransform Xf(FRotator(0.0f, YawDeg, 0.0f), Pos, FVector(1.0f));

		OutPlacements.Add({ Slot, Xf, StyleIndex });
	}
}


void UGridBuildingRuleSet::EmitRoofPlacements(
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
) const
{
	const float RoofBaseZ = GetRoofBaseZ(UpperFloors, GroundHeight, UpperHeight, bIncludeGround);

	for (int32 X = 0; X < BaysX; ++X)
	{
		for (int32 Y = 0; Y < BaysY; ++Y)
		{
			FPlacedBuildingModule Placement;
			Placement.Slot = EBuildingSemanticSlot::RoofFlat;
			Placement.StyleIndex = StyleIndex;
			Placement.Transform = FTransform(
				FRotator::ZeroRotator,
				Origin + FVector((X + 0.5f) * BaySize, (Y + 0.5f) * BaySize, RoofBaseZ),
				FVector(1.0f)
			);
			OutPlacements.Add(Placement);
		}
	}
}


void UGridBuildingRuleSet::EmitCornerPlacements(
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
) const
{
	const float Width = BaysX * BaySize;
	const float Depth = BaysY * BaySize;

	const FVector SE = BaseOrigin;
	const FVector SW = BaseOrigin + FVector(Width, 0.0f, 0.0f);
	const FVector NE = BaseOrigin + FVector(0.0f, Depth, 0.0f);
	const FVector NW = BaseOrigin + FVector(Width, Depth, 0.0f);

	float Z = 0.0f;

	/*
	* UE's left-hand coordinate system is assumed.
	* In local space, the origin of a corner module is at the bottom outer corner point.
	* Two legs of the corner module extend along X+ and Y+ directions, thus the following rotation angles are used.
	*/

	if (bIncludeGround && ShouldPlaceGroundCorners())
	{
		OutPlacements.Add(
			{ 
				EBuildingSemanticSlot::GroundCorner,
				FTransform(FRotator(0, 0, 0), SE + FVector(0,0,Z), FVector(1.0f)), 
				StyleIndex 
			}	
		);
		OutPlacements.Add(
			{ 
				EBuildingSemanticSlot::GroundCorner, 
				FTransform(FRotator(0, 90, 0), SW + FVector(0,0,Z), FVector(1.0f)), StyleIndex 
			}
		);
		OutPlacements.Add(
			{
				EBuildingSemanticSlot::GroundCorner, 
				FTransform(FRotator(0, -90, 0), NE + FVector(0,0,Z), FVector(1.0f)), 
				StyleIndex 
			}		
		);
		OutPlacements.Add(
			{ 
				EBuildingSemanticSlot::GroundCorner, 
				FTransform(FRotator(0, 180, 0), NW + FVector(0,0,Z), FVector(1.0f)), 
				StyleIndex 
			}
		);

		Z += GroundHeight;
	}

	if (ShouldPlaceUpperCorners())
	{
		for (int32 Floor = 0; Floor < UpperFloors; ++Floor)
		{
			OutPlacements.Add(
				{
					EBuildingSemanticSlot::UpperCorner,
					FTransform(FRotator(0, 0, 0), SE + FVector(0,0,Z), FVector(1.0f)),
					StyleIndex
				}
			);
			OutPlacements.Add(
				{
					EBuildingSemanticSlot::UpperCorner,
					FTransform(FRotator(0, 90, 0), SW + FVector(0,0,Z), FVector(1.0f)), StyleIndex
				}
			);
			OutPlacements.Add(
				{
					EBuildingSemanticSlot::UpperCorner,
					FTransform(FRotator(0, -90, 0), NE + FVector(0,0,Z), FVector(1.0f)),
					StyleIndex
				}
			);
			OutPlacements.Add(
				{
					EBuildingSemanticSlot::UpperCorner,
					FTransform(FRotator(0, 180, 0), NW + FVector(0,0,Z), FVector(1.0f)),
					StyleIndex
				}
			);
			Z += UpperHeight;
		}
	}

	if (bIncludeRoof && ShouldPlaceParapetCorners())
	{
		Z += RoofThickness;

		OutPlacements.Add(
			{
				EBuildingSemanticSlot::ParapetCorner,
				FTransform(FRotator(0, 0, 0), SE + FVector(0,0,Z), FVector(1.0f)),
				StyleIndex
			}
		);
		OutPlacements.Add(
			{
				EBuildingSemanticSlot::ParapetCorner,
				FTransform(FRotator(0, 90, 0), SW + FVector(0,0,Z), FVector(1.0f)), StyleIndex
			}
		);
		OutPlacements.Add(
			{
				EBuildingSemanticSlot::ParapetCorner,
				FTransform(FRotator(0, -90, 0), NE + FVector(0,0,Z), FVector(1.0f)),
				StyleIndex
			}
		);
		OutPlacements.Add(
			{
				EBuildingSemanticSlot::ParapetCorner,
				FTransform(FRotator(0, 180, 0), NW + FVector(0,0,Z), FVector(1.0f)),
				StyleIndex
			}
		);
	}
}

FVector UGridBuildingRuleSet::GetPlacementPivotOffset(
	const UBuildingArchetypeDataAsset* Archetype,
	const FBuildingGenerationInput& Input,
	const FBuildingDimensions& Dimensions
) const
{
	const float Width = Dimensions.BaysX * Input.Metrics.BaySize;
	const float Depth = Dimensions.BaysY * Input.Metrics.BaySize;
	return FVector(-Width * 0.5f, -Depth * 0.5f, 0.0f);
}




