#pragma once

#include "CoreMinimal.h"

#include "ProcCity/Block/Types/BlockLayoutTypes.h"
#include "ProcCity/Common/Types/SpatialTypes.h"
#include "ProcCity/Common/Types/Handedness.h"
#include "BlockLayoutJsonImporter.generated.h"

// ------------------------------
// JSON DTO structs
// ------------------------------

USTRUCT(BlueprintType)
struct PROCCITY_API FBlockJsonBounds2D
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<float> Min;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<float> Max;
};

USTRUCT(BlueprintType)
struct PROCCITY_API FBlockJsonSurfaceEdgeTreatmentRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Width = 0.0f;
};

USTRUCT(BlueprintType)
struct PROCCITY_API FBlockJsonSurfaceEdgeTreatment
{
	GENERATED_BODY()

	// Optional default rule for all rect edges.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FBlockJsonSurfaceEdgeTreatmentRule Default;

	// Optional per-edge overrides keyed by:
	// "top", "right", "bottom", "left"
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FString, FBlockJsonSurfaceEdgeTreatmentRule> Override;
};

/*
 * To allow for future extention, the definitions of FBlockJsonGround and 
 * FBlockJsonReserve are intentionally retained (even though their 
 * fields are exactly identical).
 */

// Ground type definition
USTRUCT(BlueprintType)
struct PROCCITY_API FBlockJsonGround
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Role;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Shape;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FBlockJsonBounds2D Bounds;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FBlockJsonSurfaceEdgeTreatment EdgeTreatment;
};

// Reserve type definition
USTRUCT(BlueprintType)
struct PROCCITY_API FBlockJsonReserve
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Role;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Shape;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FBlockJsonBounds2D Bounds;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FBlockJsonSurfaceEdgeTreatment EdgeTreatment;
};

// Path related type definition
USTRUCT(BlueprintType)
struct PROCCITY_API FBlockJsonPathTreatment
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool Enabled = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Type;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<float> Origin;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RotationDeg = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TrimLength = 0.0f;
};

USTRUCT(BlueprintType)
struct PROCCITY_API FBlockJsonPath
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Role;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Shape;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<float> Start;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<float> End;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Width = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString HandednessHint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FBlockJsonPathTreatment StartTreatment;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FBlockJsonPathTreatment EndTreatment;
};

// Path junction type definition
USTRUCT(BlueprintType)
struct PROCCITY_API FBlockJsonPathJunction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Association;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Type;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Shape;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<float> Origin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MainPathWidth = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SecondaryPathWidth = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RotationDeg = 0.0f;
};


// Building footprint type definition
USTRUCT(BlueprintType)
struct PROCCITY_API FBlockJsonBuilding
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Role;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Shape;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ParcelId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FBlockJsonBounds2D Bounds;
};

// Parcel (lot) type definition
USTRUCT(BlueprintType)
struct PROCCITY_API FBlockJsonParcel
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Shape;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FBlockJsonBounds2D Bounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ZoneType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString FrontageSide;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bRearAccess = false;
};

// Point object type definition
USTRUCT(BlueprintType)
struct PROCCITY_API FBlockJsonPoint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Role;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<float> Position;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RotationDeg = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RandomYawMin = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RandomYawMax = 0.0f;
};

USTRUCT(BlueprintType)
struct PROCCITY_API FBlockJsonRoot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SchemaVersion;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Units;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ZoneType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FBlockJsonGround> Grounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FBlockJsonPath> Path;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FBlockJsonPathJunction> PathJunctions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FBlockJsonReserve> Reserves;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FBlockJsonBuilding> Buildings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FBlockJsonParcel> Parcels;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FBlockJsonPoint> Points;
};

// ------------------------------
// Import result
// ------------------------------

USTRUCT(BlueprintType)
struct PROCCITY_API FImportedBlockLayoutData
{
	GENERATED_BODY()
	
	// Currently only [[GeneratedLayout] is used in [BlockGeneratorActor]
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Block")
	FGeneratedBlockLayout GeneratedLayout;

	// Optional: imported building placeholders / footprints
	// You can replace this later with a dedicated type.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Block")
	TArray<FBox2D> ImportedBuildingFootprints;
};

// ------------------------------
// Importer
// ------------------------------

class PROCCITY_API FBlockLayoutJsonImporter
{
public:
	static bool ImportFromFile(
		const FString& FilePath,
		FImportedBlockLayoutData& OutData,
		FString& OutError
	);
	
	static bool ImportFromJsonString(
		const FString& JsonString,
		FImportedBlockLayoutData& OutData,
		FString& OutError
	);
	
private:
	static bool ConvertJsonRootToLayout(
		const FBlockJsonRoot& JsonRoot,
		FImportedBlockLayoutData& OutData,
		FString& OutError
	);
	
private:
	
	// -- Helper functions
	static bool ParseVector2D(const TArray<float>& InArray, FVector2D& OutValue);
	static bool ParseVector(const TArray<float>& InArray, FVector& OutValue);
	static bool ParseBox2D(const FBlockJsonBounds2D& InBounds, FBox2D& OutBox);
	
	static bool ParsePathTreatment(
		const FBlockJsonPathTreatment& InTreatment, 
		FPathTerminalTreatment& OutTreatment
	);
	
	template<typename TEnum>
	static TEnum ParseEnumByName(
		const FString& InValue, TEnum DefaultValue);
	
	static EBlockPathRole ParsePathRole(const FString& InRole);
	static EBlockPathJunctionType ParsePathJunctionType(const FString& InJunctionType);
	static EBlockReserveRole ParseReserveRole(const FString& InRole);
	static EBlockPointRole ParsePointRole(const FString& InRole);
	static EBlockGroundRole ParseGroundRole(const FString& InRole);
	static EBlockBuildingFeatureRole ParseBuildingRole(const FString& InRole);
	static EHandedness ParseHandednessHint(const FString& InHint);
	static ELotFrontageSide ParseFrontageSide(const FString& InSide);
	static EZoneType ParseZoneType(const FString& InZoneType);
	static EBlockFeatureGeometryType ParseGeometryShape(const FString& InShape);
	static EBlockPathTreatmentType ParsePathTreatmentType(const FString& InTreatmentType);
	
	static bool ParsePathWidthSpec(const float Width, EBlockPathWidthSpec& OutSpec);
	
	static bool ParseSurfaceEdgeTreatmentRule(
		const FBlockJsonSurfaceEdgeTreatmentRule& InRule,
		FSurfaceEdgeTreatmentRule& OutRule,
		FString& OutError
	);
	
	static ESurfaceEdgeTreatmentType 
		ParseSurfaceEdgeTreatmentType(const FString& InType);
	
	static bool TryParseRectSurfaceEdgeKey(
		const FString& InKey,
		ERectSurfaceEdge& OutEdge
	);
	
	static bool ParseSurfaceEdgeTreatment(
		const FBlockJsonSurfaceEdgeTreatment& InTreatment,
		FSurfaceEdgeTreatment& OutTreatment,
		FString& OutError
	);
	
	static bool ValidateUniqueId(
		const FString& InRawId,
		TSet<FName>& InOutSeenIds,
		const TCHAR* EntityType,
		FString& OutError
	);
};
