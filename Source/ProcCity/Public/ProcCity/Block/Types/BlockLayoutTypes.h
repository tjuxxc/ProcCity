// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProcCity/Common/Types/SpatialTypes.h"
#include "ProcCity/Block/Types/BlockLayoutEnums.h"
#include "ProcCity/Environment/Types/BlockEnvironmentEnums.h"
#include "ProcCity/Environment/Types/BlockEnvironmentFeatureTypes.h"

#include "BlockLayoutTypes.generated.h"


/*
 * Block layout types:
 * These describe the semantic/planning result of a block.
 * They do NOT contain style/presentation information.
 */

// -------------------- Building Feature --------------------- 
UENUM(BlueprintType)
enum class EBlockBuildingFeatureRole : uint8
{
	None = 0,
	PrimaryBuilding,
	AccessoryBuilding,
	UtilityStructure,
	Custom
};

USTRUCT(BlueprintType)
struct PROCCITY_API FBlockBuildingFeature
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	EBlockBuildingFeatureRole Role = EBlockBuildingFeatureRole::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	EBlockFeatureGeometryType GeometryType = EBlockFeatureGeometryType::Rect;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	FName ParcelId = NAME_None;
	
	// Used when GeometryType == Rect
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	FBox2D LocalRect = FBox2D(ForceInit);
	
	// Used when GeometryType == Polygon
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	TArray<FVector> LocalControlPoints;
	
	// Optional explicit frontage for this building footprint.
	// If None, may inherit from parcel/lot.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Block")
	ELotFrontageSide FrontageSide = ELotFrontageSide::None;
};

// ----------------- Lot Definition -----------------
USTRUCT(BlueprintType)
struct PROCCITY_API FLotDefinition
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lot")
	FName LotId = NAME_None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lot")
	FVector LocalOrigin = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lot")
	float Width = 6000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lot")
	float Depth = 3000.0f;
	
	// TODO:
	// - Lot definition currently supports only rect shape.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lot")
	FBox2D LocalRect = FBox2D{ForceInit};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lot")
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lot")
	EZoneType ZoneType = EZoneType::Residential;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lot")
	ELotPositionType PositionType = ELotPositionType::None;
	
	// Note:
	// - This [FrontageSide] is defined in lot's local space 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lot")
	ELotFrontageSide FrontageSide = ELotFrontageSide::None;
	
	// Direction convention:
	// - Left-hand system is as in UE. 
	// - X+ = West, X- = East, Y+ = North, Y- = South.
	// Note:
	// - WestStreet, EastStreet, etc. are defined in lot's local space 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lot")
	bool bHasWestStreet = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lot")
	bool bHasEastStreet = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lot")
	bool bHasNorthStreet = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lot")
	bool bHasSouthStreet = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lot")
	bool bHasRearAccess = false;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lot")
	FName SelectedArchetypeId = NAME_None;
};

// ----------------- Block Definition -----------------
USTRUCT(BlueprintType)
struct PROCCITY_API FBlockDefinition
{
	GENERATED_BODY()
	
	// TODO:
	// - Block definition currently supports only rect shape.
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block", meta = (ClampMin = "1000.0"))
	float BlockWidth = 20000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block", meta = (ClampMin = "1000.0"))
	float BlockDepth = 30000.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block", meta = (ClampMin = "0.0"))
	float StreetSetback = 2000.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block", meta = (ClampMin = "1"))
	int32 LotsAlongWidth = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block", meta = (ClampMin = "1"))
	int32 LotsAlongDepth = 4;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block")
	EZoneType ZoneType = EZoneType::Residential;
};

// ----------------- Generated Layout -----------------
USTRUCT(BlueprintType)
struct PROCCITY_API FGeneratedBlockLayout
{
	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Block")
	TArray<FBlockPathFeature> PathFeatures;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Block")
	TArray<FBlockPathJunctionFeature> PathJunctionFeatures;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Block")	
	TArray<FBlockSurfaceFeature> SurfaceFeatures;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Block")
	TArray<FBlockBuildingFeature> BuildingFeatures;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Block")	
	TArray<FBlockPointFeature> PointFeatures;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Block")
	TArray<FLotDefinition> Lots;
};














