#include "ProcCity/Block/Importer/BlockLayoutJsonImporter.h"

#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "ProcCity/Environment/Utils/BlockEnvironmentUtils.h"
#include "ProcCity/Environment/Types/BlockEnvironmentFeatureTypes.h"


bool FBlockLayoutJsonImporter::ImportFromFile(
	const FString& FilePath,
	FImportedBlockLayoutData& OutData,
	FString& OutError
)
{
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
	{
		OutError = FString::Printf(TEXT("File to load file: %s"), *FilePath);
		return false;
	}
	
	return ImportFromJsonString(JsonString,OutData,OutError);
}

bool FBlockLayoutJsonImporter::ImportFromJsonString(
	const FString& JsonString,
	FImportedBlockLayoutData& OutData,
	FString& OutError
)
{
	FBlockJsonRoot JsonRoot;
	if (!FJsonObjectConverter::JsonObjectStringToUStruct<FBlockJsonRoot>(
		JsonString, &JsonRoot,0,0))
	{
		OutError = TEXT("Failed to deserialize JSON into FBlockJsonRoot.");
		return false;
	}
	
	return ConvertJsonRootToLayout(JsonRoot,OutData,OutError);
}

bool FBlockLayoutJsonImporter::ConvertJsonRootToLayout(
	const FBlockJsonRoot& JsonRoot,
	FImportedBlockLayoutData& OutData,
	FString& OutError
)
{
	OutData = FImportedBlockLayoutData();
	// Grounds -> Surface Feature
	{
		TSet<FName> SeenGroundIds;
		for (const FBlockJsonGround& Ground : JsonRoot.Grounds)
		{
			if (!ValidateUniqueId(Ground.Id, SeenGroundIds, 
					TEXT("Ground"), OutError))
			{
				return false;
			}
		
			FBox2D Box;
			if (!ParseBox2D(Ground.Bounds,Box))
			{
				OutError = FString::Printf(
					TEXT("Invalid ground bounds for %s"), *Ground.Id);
				return false;
			}
			
			FBlockSurfaceFeature SurfaceFeature;
			SurfaceFeature.SurfaceCategory = EBlockSurfaceCategory::Ground;
			SurfaceFeature.GroundRole = ParseGroundRole(Ground.Role);
			SurfaceFeature.GeometryType = ParseGeometryShape(Ground.Shape);
			SurfaceFeature.LocalRect = Box;
			
			// TODO: Current logic only supports Rect
			if (SurfaceFeature.GeometryType == EBlockFeatureGeometryType::Rect)
			{
				FSurfaceEdgeTreatment ParsedEdgeTreatment;
				if (!ParseSurfaceEdgeTreatment(Ground.EdgeTreatment, 
					ParsedEdgeTreatment, OutError))
				{
					OutError = FString::Printf(
						TEXT("Invalid edgeTreatment for ground '%s': %s"),
						*Ground.Id,
						*OutError
					);
					return false;
				}
				SurfaceFeature.EdgeTreatment = ParsedEdgeTreatment;
			}
			
			OutData.GeneratedLayout.SurfaceFeatures.Add(SurfaceFeature);
		}
	}
	
	// Path segments -> PathFeatures
	// TODO: The current logic only addresses straight path segment
	{
		TSet<FName> SeenPathIds;
		for (const FBlockJsonPath& Path : JsonRoot.Path)
		{
			if (!ValidateUniqueId(Path.Id, SeenPathIds, 
				TEXT("Path"), OutError))
			{
				return false;
			}
		
			FVector Start;
			FVector End;
			if (!ParseVector(Path.Start, Start) || 
				!ParseVector(Path.End, End))
			{
				OutError = FString::Printf(
					TEXT("Invalid path segment endpoints for %s"), 
					*Path.Id
				);
				return false;
			}
		
			FPathTerminalTreatment StartTreatment;
			FPathTerminalTreatment EndTreatment;
			if (!ParsePathTreatment(Path.StartTreatment, StartTreatment) || 
				!ParsePathTreatment(Path.EndTreatment, EndTreatment))
			{
				OutError = FString::Printf(
					TEXT("Invalid path start/end treatment for %s"), 
					*Path.Id
				);
				return false;
			}
		
			EBlockPathWidthSpec WidthSpec = EBlockPathWidthSpec::None;
			if (!ParsePathWidthSpec(Path.Width, WidthSpec))
			{
				OutError = FString::Printf(
					TEXT("Unsupported path width %.2f for path %s."),
					Path.Width,
					*Path.Id
				);
				return false;
			}
		
			FBlockPathFeature PathFeature;
			PathFeature.Role = ParsePathRole(Path.Role);
			PathFeature.GeometryType = ParseGeometryShape(Path.Shape);
			PathFeature.LocalStart = Start;
			PathFeature.LocalEnd = End;
			PathFeature.WidthSpec = WidthSpec;
			PathFeature.HandednessHint = ParseHandednessHint(Path.HandednessHint);
			PathFeature.StartTreatment = StartTreatment;
			PathFeature.EndTreatment = EndTreatment;
			OutData.GeneratedLayout.PathFeatures.Add(PathFeature);
		}
	}
	
	{
		// Junctions -> FBlockPathJunctionFeature + ImportedJunctionPlans
		// TODO: The current logic only addresses rect junction
		TSet<FName> SeenPathJunctionIds;
		for (const FBlockJsonPathJunction& Junction : JsonRoot.PathJunctions)
		{
			if (!ValidateUniqueId(Junction.Id, SeenPathJunctionIds, 
				TEXT("Path Junction"), OutError))
			{
				return false;
			}
		
			FVector LocalOrigin;
			if (!ParseVector(Junction.Origin, LocalOrigin))
			{
				OutError = FString::Printf(
					TEXT("Invalid origin for %s"), 
					*Junction.Id
				);
				return false;
			}
		
			EBlockPathWidthSpec PrimaryWidthSpec = EBlockPathWidthSpec::None;
		
			if (!ParsePathWidthSpec(Junction.MainPathWidth, PrimaryWidthSpec))
			{
				OutError = FString::Printf(
					TEXT("Unsupported main path width  %.2f for junction %s"), 
					Junction.MainPathWidth,
					*Junction.Id
				);
			}
		
			EBlockPathWidthSpec SecondaryWidthSpec = EBlockPathWidthSpec::None;
			if (!ParsePathWidthSpec(Junction.SecondaryPathWidth, SecondaryWidthSpec))
			{
				OutError = FString::Printf(
					TEXT("Unsupported secondary path width  %.2f for junction %s"),
					Junction.SecondaryPathWidth,
					*Junction.Id
				);
			}
		
			FBlockPathJunctionFeature PathJunctionFeature;
			PathJunctionFeature.JunctionType = ParsePathJunctionType(Junction.Type);
			PathJunctionFeature.Association = ParsePathRole(Junction.Association);
			PathJunctionFeature.GeometryType = ParseGeometryShape(Junction.Shape);
			PathJunctionFeature.LocalOrigin = LocalOrigin;
			PathJunctionFeature.LocalRotation = FRotator(
				0.0f, 
				Junction.RotationDeg, 
				0.0f
			);
		
			PathJunctionFeature.PrimaryWidthSpec = PrimaryWidthSpec;
			PathJunctionFeature.SecondaryWidthSpec = SecondaryWidthSpec;
		
			OutData.GeneratedLayout.PathJunctionFeatures.Add(PathJunctionFeature);
		}
	}
	
	// Reserves -> ReserveFeatures + ImportedReservePlans
	{
		// TODO: The current logic only addresses rect Reserve area
		TSet<FName> SeenReserveIds;
		for (const FBlockJsonReserve& Reserve : JsonRoot.Reserves)
		{
			if (!ValidateUniqueId(Reserve.Id, SeenReserveIds, 
				TEXT("Reserve"), OutError))
			{
				return false;
			}
		
			FBox2D Box;
			if (!ParseBox2D(Reserve.Bounds, Box))
			{
				OutError = FString::Printf(
					TEXT("Invalid reserve bounds for %s"), 
					*Reserve.Id
				);
				return false;
			}
		
			FBlockSurfaceFeature SurfaceFeature;
			SurfaceFeature.SurfaceCategory = EBlockSurfaceCategory::Reserve;
			SurfaceFeature.ReserveRole = ParseReserveRole(Reserve.Role);
			SurfaceFeature.GeometryType = ParseGeometryShape(Reserve.Shape);
			SurfaceFeature.LocalRect = Box;
			
			if (SurfaceFeature.GeometryType == EBlockFeatureGeometryType::Rect)
			{
				FSurfaceEdgeTreatment ParsedEdgeTreatment;
				if (!ParseSurfaceEdgeTreatment(Reserve.EdgeTreatment, ParsedEdgeTreatment, OutError))
				{
					OutError = FString::Printf(
						TEXT("Invalid edgeTreatment for reserve '%s': %s"),
						*Reserve.Id,
						*OutError
					);
					return false;
				}

				SurfaceFeature.EdgeTreatment = ParsedEdgeTreatment;
			}
			
			OutData.GeneratedLayout.SurfaceFeatures.Add(SurfaceFeature);
		}
	}
	
	// Points -> PointFeatures
	{
		for (const FBlockJsonPoint& Point : JsonRoot.Points)
		{
			FVector Position;
			if (!ParseVector(Point.Position, Position))
			{
				OutError = FString::Printf(
					TEXT("Invalid point position for %s"), 
					*Point.Id
				);
				return false;
			}
		
			FBlockPointFeature PointFeature;
			PointFeature.Role = ParsePointRole(Point.Role);
			PointFeature.GeometryType = EBlockFeatureGeometryType::Point;
			PointFeature.LocalOrigin = Position;
			PointFeature.LocalRotation = 
				FRotator(0.0f, Point.RotationDeg, 0.0f);
			PointFeature.RandomYawMin = Point.RandomYawMin;
			PointFeature.RandomYawMax = Point.RandomYawMax;
			OutData.GeneratedLayout.PointFeatures.Add(PointFeature);
		}
	}
	
	// Parcels -> Lots
	// TODO: The current logic only addresses rect lot
	TSet<FName> SeenParcelIds;
	for (const FBlockJsonParcel& Parcel : JsonRoot.Parcels)
	{
		if (!ValidateUniqueId(Parcel.Id, SeenParcelIds, 
			TEXT("Parcel"), OutError))
		{
			return false;
		}
		
		FBox2D Box;
		if (!ParseBox2D(Parcel.Bounds, Box))
		{
			OutError = FString::Printf(TEXT("Invalid parcel bounds for %s"), *Parcel.Id);
			return false;
		}
		
		FLotDefinition Lot;
		Lot.LotId = FName(*Parcel.Id);
		Lot.LocalRect = Box;
		Lot.LocalOrigin = FVector(Box.Min.X, Box.Min.Y, 0.0f);
		Lot.Width = Box.Max.X - Box.Min.X;
		Lot.Depth = Box.Max.Y - Box.Min.Y;
		Lot.ZoneType = ParseZoneType(Parcel.ZoneType);
		Lot.bHasRearAccess = Parcel.bRearAccess;
		
		OutData.GeneratedLayout.Lots.Add(Lot);
	}
	
	// Building -> BuildingFeatures
	// TODO: The current logic only addresses axis-aligned rect Building footprint
	TSet<FName> SeenBuildingIds;
	for (const FBlockJsonBuilding& Building : JsonRoot.Buildings)
	{
		if (!ValidateUniqueId(Building.Id, SeenBuildingIds, 
			TEXT("Building"), OutError))
		{
			return false;
		}
		
		const FString ParcelIdString = Building.ParcelId.TrimStartAndEnd();
		if (ParcelIdString.IsEmpty())
		{
			OutError = FString::Printf(
				TEXT("Building %s has empty parcelId."),
				*Building.Id
			);
			return false;
		}
		
		const FName ParcelIdName = FName(*ParcelIdString);
		if (!SeenParcelIds.Contains(ParcelIdName))
		{
			OutError = FString::Printf(
				TEXT("Building %s references unknown parcelId: %s"),
				*Building.Id,
				*Building.ParcelId
			);
			return false;
		}
		
		FBox2D Box;
		if (!ParseBox2D(Building.Bounds, Box))
		{
			OutError = FString::Printf(
				TEXT("Invalid building bounds for %s"), 
				*Building.Id
			);
			return false;
		}
		
		FBlockBuildingFeature BuildingFeature;
		BuildingFeature.Role = ParseBuildingRole(Building.Role);
		BuildingFeature.GeometryType = ParseGeometryShape(Building.Shape);
		BuildingFeature.ParcelId = FName(*Building.ParcelId);
		BuildingFeature.LocalRect = Box;
		
		OutData.GeneratedLayout.BuildingFeatures.Add(BuildingFeature);
	}
	return true;
}

bool FBlockLayoutJsonImporter::ParseVector2D(
	const TArray<float>& InArray, FVector2D& OutValue)
{
	if (InArray.Num() < 2)
	{
		return false;
	}
	
	OutValue = FVector2D(InArray[0], InArray[1]);
	return true;
}

bool FBlockLayoutJsonImporter::ParseVector(
	const TArray<float>& InArray, FVector& OutValue)
{
	if (InArray.Num() < 2)
	{
		return false;
	}
	
	const float Z = InArray.Num() >= 3 ? InArray[2] : 0.0f;
	OutValue = FVector(InArray[0], InArray[1], Z);
	return true;
}

bool FBlockLayoutJsonImporter::ParseBox2D(
	const FBlockJsonBounds2D& InBounds, FBox2D& OutBox)
{
	FVector2D Min;
	FVector2D Max;
	if (!ParseVector2D(InBounds.Min, Min) || 
		!ParseVector2D(InBounds.Max, Max))
	{
		return false;
	}
	
	OutBox = FBox2D(Min, Max);
	return true;
}

template<typename TEnum>
TEnum FBlockLayoutJsonImporter::ParseEnumByName(
	const FString& InValue, 
	TEnum DefaultValue
)
{
	const UEnum* EnumObj = StaticEnum<TEnum>();
	if (!EnumObj)
	{
		return DefaultValue;
	}
	
	const FString Normalized = InValue.TrimStartAndEnd();
	
	for (int32 Index = 0; Index < EnumObj->NumEnums(); ++Index)
	{
		// Skip hidden autogenerated _MAX if present
		if (EnumObj->HasMetaData(TEXT("Hidden"), Index))
		{
			continue;
		}
		
		const FString NameString = EnumObj->GetNameStringByIndex(Index);
		if (NameString.Equals(Normalized, ESearchCase::IgnoreCase))
		{
			return static_cast<TEnum>(EnumObj->GetValueByIndex(Index));
		}
	}
	return DefaultValue;
}

EBlockGroundRole
FBlockLayoutJsonImporter::ParseGroundRole(const FString& InRole)
{
	return ParseEnumByName<EBlockGroundRole>(
		InRole, 
		EBlockGroundRole::None
	);
}

EBlockFeatureGeometryType 
FBlockLayoutJsonImporter::ParseGeometryShape(const FString& InShape)
{
	return ParseEnumByName<EBlockFeatureGeometryType>(
		InShape,
		EBlockFeatureGeometryType::None
		);
}

EBlockPathRole
FBlockLayoutJsonImporter::ParsePathRole(const FString& InRole)
{
	return ParseEnumByName<EBlockPathRole>(
		InRole, 
		EBlockPathRole::None
		);
}

EBlockPathJunctionType
FBlockLayoutJsonImporter::ParsePathJunctionType(const FString& InJunctionType)
{
	return ParseEnumByName<EBlockPathJunctionType>(
		InJunctionType, 
		EBlockPathJunctionType::None
		);
}

EZoneType FBlockLayoutJsonImporter::ParseZoneType(const FString& InZoneType)
{
	return ParseEnumByName<EZoneType>(InZoneType, EZoneType::None);
}

EBlockReserveRole
FBlockLayoutJsonImporter::ParseReserveRole(const FString& InRole)
{
	return ParseEnumByName<EBlockReserveRole>(
		InRole, EBlockReserveRole::None
		);
}

EBlockPointRole 
FBlockLayoutJsonImporter::ParsePointRole(const FString& InRole)
{
	return ParseEnumByName<EBlockPointRole>(
		InRole, EBlockPointRole::None
		);
}

ELotFrontageSide 
FBlockLayoutJsonImporter::ParseFrontageSide(const FString& InSide)
{
	return ParseEnumByName<ELotFrontageSide>(
		InSide, 
		ELotFrontageSide::None
		);
}

EBlockPathTreatmentType 
FBlockLayoutJsonImporter::ParsePathTreatmentType(const FString& InTreatmentType)
{
	return ParseEnumByName<EBlockPathTreatmentType>(
		InTreatmentType, 
		EBlockPathTreatmentType::None
		);
}

EBlockBuildingFeatureRole 
FBlockLayoutJsonImporter::ParseBuildingRole(const FString& InRole)
{
	return ParseEnumByName<EBlockBuildingFeatureRole>(
		InRole, 
		EBlockBuildingFeatureRole::None
		);
}

EHandedness 
FBlockLayoutJsonImporter::ParseHandednessHint(const FString& InHint)
{
	return ParseEnumByName<EHandedness>(
		InHint, 
		EHandedness::None
		);
}

ESurfaceEdgeTreatmentType 
FBlockLayoutJsonImporter::ParseSurfaceEdgeTreatmentType(
	const FString& InType
)
{
	return ParseEnumByName<ESurfaceEdgeTreatmentType>(
		InType, 
		ESurfaceEdgeTreatmentType::None
	);
}



bool FBlockLayoutJsonImporter::ParsePathTreatment(
	const FBlockJsonPathTreatment& InTreatment,
	FPathTerminalTreatment& OutTreatment
)
{
	OutTreatment = FPathTerminalTreatment();
	
	OutTreatment.bEnabled = InTreatment.Enabled;
	OutTreatment.TreatmentType = ParsePathTreatmentType(InTreatment.Type);

	if (!ParseVector(InTreatment.Origin, OutTreatment.LocalOrigin))
	{
		return false;
	}
	
	OutTreatment.RotationDeg = InTreatment.RotationDeg;
	OutTreatment.TrimLength = InTreatment.TrimLength;
	
	return true;
}

bool FBlockLayoutJsonImporter::ParseSurfaceEdgeTreatmentRule(
	const FBlockJsonSurfaceEdgeTreatmentRule& InRule,
	FSurfaceEdgeTreatmentRule& OutRule,
	FString& OutError
)
{
	OutRule = FSurfaceEdgeTreatmentRule();
	OutRule.Type = ParseSurfaceEdgeTreatmentType(InRule.Type);
	
	if (InRule.Width < 0.0f)
	{
		OutError = FString::Printf(
			TEXT("Surface edge treatment width must be >= 0, got %.3f"),
			InRule.Width
		);
		return false;
	}
	
	OutRule.Width = InRule.Width;
	return true;
}

bool FBlockLayoutJsonImporter::TryParseRectSurfaceEdgeKey(
	const FString& InKey,
	ERectSurfaceEdge& OutEdge
)
{
	if (InKey.Equals(TEXT("top"), ESearchCase::IgnoreCase))
	{
		OutEdge = ERectSurfaceEdge::Top;
		return true;
	}
	
	if (InKey.Equals(TEXT("bottom"), ESearchCase::IgnoreCase))
	{
		OutEdge = ERectSurfaceEdge::Bottom;
		return true;
	}
	
	if (InKey.Equals(TEXT("left"), ESearchCase::IgnoreCase))
	{
		OutEdge = ERectSurfaceEdge::Left;
		return true;
	}
	
	if (InKey.Equals(TEXT("right"), ESearchCase::IgnoreCase))
	{
		OutEdge = ERectSurfaceEdge::Right;
		return true;
	}

	return false;
}

bool FBlockLayoutJsonImporter::ParseSurfaceEdgeTreatment(
	const FBlockJsonSurfaceEdgeTreatment& InTreatment,
	FSurfaceEdgeTreatment& OutTreatment,
	FString& OutError
)
{
	OutTreatment = FSurfaceEdgeTreatment();
	
	// Parse default rule
	FSurfaceEdgeTreatmentRule DefaultRule;
	if (!ParseSurfaceEdgeTreatmentRule(InTreatment.Default, 
		DefaultRule, OutError))
	{
		return false;
	}
	
	// Address case of no effective edge treatment
	const bool bHasNoEffectiveTreatment =
		(DefaultRule.Type == ESurfaceEdgeTreatmentType::None) &&
		FMath::IsNearlyZero(DefaultRule.Width) &&
		(InTreatment.Override.Num() == 0);

	if (bHasNoEffectiveTreatment)
	{
		OutTreatment.bEnabled = false;
		OutTreatment.EdgeRules.Empty();
		return true;
	}
	
	OutTreatment.bEnabled = true;
	
	
	// TODO: Current logic suppose rect surface
	OutTreatment.EdgeRules.Reserve(4);
	
	// Parse override rule
	auto AddResolvedRule = [&OutTreatment] 
		(const ERectSurfaceEdge Edge, const FSurfaceEdgeTreatmentRule& Rule)
		{
			FSurfaceEdgeTreatmentRule ResolvedRule = Rule;
			ResolvedRule.Edge = Edge;
			OutTreatment.EdgeRules.Add(ResolvedRule);
		};
	
	AddResolvedRule(ERectSurfaceEdge::Top, DefaultRule);
	AddResolvedRule(ERectSurfaceEdge::Bottom, DefaultRule);
	AddResolvedRule(ERectSurfaceEdge::Left, DefaultRule);
	AddResolvedRule(ERectSurfaceEdge::Right, DefaultRule);
		
	for (const TPair<FString, FBlockJsonSurfaceEdgeTreatmentRule>& 
		Pair : InTreatment.Override)
	{
		ERectSurfaceEdge Edge;
		if (!TryParseRectSurfaceEdgeKey(Pair.Key, Edge))
		{
			OutError = FString::Printf(
				TEXT("Unknown surface edge override key: %s"),
				*Pair.Key
			);
			return false;
		}
		
		FSurfaceEdgeTreatmentRule ParsedOverride;
		if (!ParseSurfaceEdgeTreatmentRule(Pair.Value, ParsedOverride, OutError))
		{
			OutError = FString::Printf(
				TEXT("Invalid override rule for edge '%s': %s"),
				*Pair.Key,
				*OutError
			);
			return false;
		}
		
		for (FSurfaceEdgeTreatmentRule& ExistingRule : OutTreatment.EdgeRules)
		{
			if (ExistingRule.Edge == Edge)
			{
				ExistingRule.Type = ParsedOverride.Type;
				ExistingRule.Width = ParsedOverride.Width;
				break;
			}
		}
	}
	
	return true;
}

bool FBlockLayoutJsonImporter::ParsePathWidthSpec(
	const float Width,
	EBlockPathWidthSpec& OutSpec
	)
{
	OutSpec = EBlockPathWidthSpec::None;
	
	if (!ProcCity::TryConvertWidthToPathWidthSpec(Width, OutSpec))
	{
		return false;
	}
	
	return true;
}



bool FBlockLayoutJsonImporter::ValidateUniqueId(
	const FString& InRawId,
	TSet<FName>& InOutSeenIds,
	const TCHAR* EntityType,
	FString& OutError
)
{
	const FString Trimmed = InRawId.TrimStartAndEnd();
	if (Trimmed.IsEmpty())
	{
		OutError = FString::Printf(TEXT("%s id is empty."), EntityType);
		return false;
	}
	
	const FName IdName(*Trimmed);
	if (IdName.IsNone())
	{
		OutError = FString::Printf(
			TEXT("%s id is invalid: %s"), 
			EntityType, 
			*InRawId
		);
		return false;
	}
	
	if (InOutSeenIds.Contains(IdName))
	{
		OutError = FString::Printf(
			TEXT("Duplicate %s id: %s"), 
			EntityType, 
			*InRawId
		);
		return false;
	}
	
	InOutSeenIds.Add(IdName);
	return true;
}























