// Fill out your copyright notice in the Description page of Project Settings.

#include "ProcCity/Environment/DataAssets/BlockEnvironmentStyleDataAsset.h"
#include "ProcCity/Environment/Types/BlockEnvironmentFeatureTypes.h"

#if WITH_EDITOR
#include "ProcCity/Material/Validator/MaterialSlotBindingValidator.h"
#endif

namespace
{
	bool MatchesSurfaceSemantic(
		const FBlockSurfaceFeature& SurfaceFeature,
		const EBlockSurfaceCategory ModuleCategory,
		const EBlockFeatureGeometryType ModuleGeometryType,
		const EBlockGroundRole ModuleGroundRole,
		const EBlockReserveRole ModuleReserveRole
	)
	{
		if (SurfaceFeature.SurfaceCategory != ModuleCategory)
		{
			return false;
		}

		if (SurfaceFeature.GeometryType != ModuleGeometryType)
		{
			return false;
		}

		switch (SurfaceFeature.SurfaceCategory)
		{
		case EBlockSurfaceCategory::Ground:
			return SurfaceFeature.GroundRole == ModuleGroundRole;

		case EBlockSurfaceCategory::Reserve:
			return SurfaceFeature.ReserveRole == ModuleReserveRole;

		default:
			return false;
		}
	}
}

const FBlockSurfaceModule* UBlockEnvironmentStyleDataAsset::GetSurfaceModule(
	const FBlockSurfaceFeature& SurfaceFeature
) const
{
	for (const FBlockSurfaceModule& Module : SurfaceModules)
	{
		if (!Module.bEnabled)
		{
			continue;
		}
		
		// Must match category
		if (Module.SurfaceCategory != SurfaceFeature.SurfaceCategory)
		{
			continue;
		}
		
		// Must match Geometry Type
		if (Module.GeometryType != SurfaceFeature.GeometryType)
		{
			continue;
		}
		
		// Match semantic role according to category
		switch (SurfaceFeature.SurfaceCategory)
		{
		case EBlockSurfaceCategory::Ground:
			if (Module.GroundRole != SurfaceFeature.GroundRole)
			{
				continue;
			}
			break;
			
		case EBlockSurfaceCategory::Reserve:
			if (Module.ReserveRole != SurfaceFeature.ReserveRole)
			{
				continue;
			}
			break;
		
		case EBlockSurfaceCategory::None:
		default:
			continue;
		}
		
		return &Module;
	}
	return nullptr;
}

const FSurfaceEdgeStripModule* 
	UBlockEnvironmentStyleDataAsset::GetSurfaceEdgeModule(
		const FBlockSurfaceFeature& SurfaceFeature,
		const ESurfaceEdgeTreatmentType TreatmentType,
		const float Width,
		const float WidthTolerance
	) const
{
	// Current logic assume rect surface
	if (SurfaceFeature.GeometryType != EBlockFeatureGeometryType::Rect)
	{
		return nullptr;
	}
	
	// None should not request a dedicated edge module.
	if (TreatmentType == ESurfaceEdgeTreatmentType::None)
	{
		return nullptr;
	}
	
	for (const FSurfaceEdgeStripModule& Module : SurfaceEdgeModules)
	{
		if (!Module.bEnabled)
		{
			continue;
		}
		
		if (!MatchesSurfaceSemantic(
			SurfaceFeature,
			Module.TargetSurfaceCategory,
			Module.TargetSurfaceGeometryType,
			Module.TargetGroundRole,
			Module.TargetReserveRole
		))
		{
			continue;
		}
		
		if (Module.TreatmentType != TreatmentType)
		{
			continue;
		}
		
		if (!FMath::IsNearlyEqual(Module.Width, Width, WidthTolerance))
		{
			continue;
		}
		
		return &Module;
	}
	
	return nullptr;
}

const FSurfaceCornerModule* 
	UBlockEnvironmentStyleDataAsset::GetSurfaceCornerModule(
	const FBlockSurfaceFeature& SurfaceFeature,
	const EBlockSurfaceCornerType CornerType,
	const ERectSurfaceCorner WhichCorner,
	const float HorizontalWidth,
	const float VerticalWidth,
	const float WidthTolerance
) const
{
	// Current v1.0 only supports rect-surface corner lookup.
	if (SurfaceFeature.GeometryType != EBlockFeatureGeometryType::Rect)
	{
		return nullptr;
	}
	
	// None should not request a dedicated corner module.
	if (CornerType == EBlockSurfaceCornerType::None)
	{
		return nullptr;
	}
	
	auto MatchCommon = [&] (const FSurfaceCornerModule& Module) -> bool
	{
		if (!Module.bEnabled)
		{
			return false;
		}
		
		if (!MatchesSurfaceSemantic(
				SurfaceFeature,
				Module.TargetSurfaceCategory,
				Module.TargetSurfaceGeometryType,
				Module.TargetGroundRole,
				Module.TargetReserveRole))
		{
			return false;
		}
		
		if (Module.CornerType != CornerType)
		{
			return false;
		}
		
		if (!FMath::IsNearlyEqual(
			Module.HorizontalWidth, HorizontalWidth, WidthTolerance))
		{
			return false;
		}
		
		if (!FMath::IsNearlyEqual(
			Module.VerticalWidth, VerticalWidth, WidthTolerance))
		{
			return false;
		}
		
		return true;
	};
	
	// Pass 1: exact corner-specific match
	for (const FSurfaceCornerModule& Module : SurfaceCornerModules)
	{
		if (!MatchCommon(Module))
		{
			continue;
		}
		
		if (Module.WhichCorner != WhichCorner)
		{
			continue;
		}
		
		return &Module;
	}
	
	for (const FSurfaceCornerModule& Module : SurfaceCornerModules)
	{
		if (!MatchCommon(Module))
		{
			continue;
		}
		
		if (Module.WhichCorner == ERectSurfaceCorner::None)
		{
			return &Module;
		}
	}
	
	return nullptr;
}

const FBlockPropModule* UBlockEnvironmentStyleDataAsset::GetPropModule(
	const FBlockPointFeature& PointFeature
) const
{
	for (const FBlockPropModule& Module : PropModules)
	{
		if (!Module.bEnabled)
		{
			continue;
		}
		
		// TODO: This is not necessary in current version
		// Must match prop Geometry Type
		if (Module.GeometryType != PointFeature.GeometryType)
		{
			continue;
		}
		
		// Must match prop role
		if (Module.PropRole != PointFeature.Role)
		{
			continue;
		}
		
		return &Module;
	}
	return nullptr;
}


const FBlockPathSegmentModule* 
	UBlockEnvironmentStyleDataAsset::GetPathSegmentModule(
	const FBlockPathFeature& PathFeature
) const
{
	for (const FBlockPathSegmentModule& Module : PathSegmentModules)
	{
		if (!Module.bEnabled)
		{
			continue;
		}
		
		if (Module.PathRole != PathFeature.Role)
		{
			continue;
		}
		
		if (Module.GeometryType != PathFeature.GeometryType)
		{
			continue;
		}
		
		if (Module.WidthSpec != PathFeature.WidthSpec)
		{
			continue;
		}
		
		if (Module.Handedness != PathFeature.HandednessHint)
		{
			continue;
		}
		
		return &Module;
	}
	return nullptr;
}

const FBlockPathJunctionModule* 
	UBlockEnvironmentStyleDataAsset::GetPathJunctionModule(
		const FBlockPathJunctionFeature& PathJunctionFeature
	)
const
{
	for (const FBlockPathJunctionModule& Module : PathJunctionModules)
	{
		if (!Module.bEnabled)
		{
			continue;
		}
		
		if (Module.PathRole != PathJunctionFeature.Association)
		{
			continue;
		}
		
		if (Module.JunctionType != PathJunctionFeature.JunctionType)
		{
			continue;
		}
		
		if (Module.GeometryType != PathJunctionFeature.GeometryType)
		{
			continue;
		}
		
		if (Module.PrimaryWidthSpec != PathJunctionFeature.PrimaryWidthSpec)
		{
			continue;
		}
		
		if (Module.SecondaryWidthSpec != PathJunctionFeature.SecondaryWidthSpec)
		{
			continue;
		}
		
		return &Module;
	}
	return nullptr;
}

const FPathTerminalTreatmentModule* 
	UBlockEnvironmentStyleDataAsset::GetPathTerminalTreatmentModule(
		const TArray<FPathTerminalTreatmentModule>& SupportedModules,
		const FPathTerminalTreatment& Treatment,
		bool bStartTreatment,
		const EHandedness PathHandedness
	)
{
	for (const FPathTerminalTreatmentModule& Module : SupportedModules)
	{
		if (!Module.bEnabled)
		{
			continue;
		}
			
		if (Module.TreatmentType != Treatment.TreatmentType)
		{
			continue;
		}
		
		// TODO:
		// - If PathHandedness is None, the current logic may fail to find a correct module.
		// - Perhaps a new field is needed to indicate the spatial neighbouring semantic of a path. 
		if (PathHandedness == EHandedness::Default &&
			bStartTreatment &&
			Module.Handedness == EHandedness::Flipped)
		{
			continue;
		}
		
		if (PathHandedness == EHandedness::Default &&
			!bStartTreatment &&
			Module.Handedness == EHandedness::Default)
		{
			continue;
		}
			
		if (PathHandedness == EHandedness::Flipped &&
			bStartTreatment &&
			Module.Handedness == EHandedness::Default)
		{
			continue;
		}
			
		if (PathHandedness == EHandedness::Flipped &&
			!bStartTreatment &&
			Module.Handedness == EHandedness::Flipped)
		{
			continue;
		}
		
		return &Module;
	}
	return nullptr;
}

#if WITH_EDITOR

EDataValidationResult UBlockEnvironmentStyleDataAsset::IsDataValid(
	FDataValidationContext& Context
) const
{
	bool bHasErrors = false;
	// TODO: Current logic only validates modules' MaterialSlots
	//       The follow logic can be extracted as template.
	
	// Validate surface
	for (int32 ModuleIndex = 0; ModuleIndex < SurfaceModules.Num(); ++ModuleIndex)
	{
		const FString OwnerLabel = FString::Printf(
			TEXT("%s.SurfaceModules[%d]"),
			*GetName(),
			ModuleIndex
		);

		if (FMaterialSlotBindingValidator::ValidateMaterialSlots(
			SurfaceModules[ModuleIndex].MaterialSlots,
			OwnerLabel,
			Context) == EDataValidationResult::Invalid)
		{
			bHasErrors = true;
		}
	}

	// Validate path segment and terminal treatment
	for (int32 ModuleIndex = 0; ModuleIndex < PathSegmentModules.Num(); ++ModuleIndex)
	{
		// Check path segment
		{
			const FString OwnerLabel = FString::Printf(
				TEXT("%s.PathSegmentModules[%d]"),
				*GetName(),
				ModuleIndex
			);

			if (FMaterialSlotBindingValidator::ValidateMaterialSlots(
				PathSegmentModules[ModuleIndex].MaterialSlots,
				OwnerLabel,
				Context) == EDataValidationResult::Invalid)
			{
				bHasErrors = true;
			}
		}
		
		// Check terminal treatment
		{
			const TArray<FPathTerminalTreatmentModule>& TreatmentModules = 
				PathSegmentModules[ModuleIndex].SupportedTerminalTreatments;
		
			for (int32 TreatmentIndex = 0; 
				TreatmentIndex < TreatmentModules.Num(); ++TreatmentIndex)
			{
				const FString OwnerLabel = FString::Printf(
					TEXT("%s.PathSegmentModules[%d].TreatmentModules[%d]"),
					*GetName(),
					ModuleIndex,
					TreatmentIndex
				);
				
				if (FMaterialSlotBindingValidator::ValidateMaterialSlots(
					TreatmentModules[TreatmentIndex].MaterialSlots,
					OwnerLabel,
					Context) == EDataValidationResult::Invalid)
				{
					bHasErrors = true;
				}
			}
		}

	}

	// Validate path junction
	for (int32 ModuleIndex = 0; ModuleIndex < PathJunctionModules.Num(); ++ModuleIndex)
	{
		const FString OwnerLabel = FString::Printf(
			TEXT("%s.PathJunctionModules[%d]"),
			*GetName(),
			ModuleIndex
		);

		if (FMaterialSlotBindingValidator::ValidateMaterialSlots(
			PathJunctionModules[ModuleIndex].MaterialSlots,
			OwnerLabel,
			Context) == EDataValidationResult::Invalid)
		{
			bHasErrors = true;
		}
	}
	
	// Validate Prop
	for (int32 ModuleIndex = 0; ModuleIndex < PropModules.Num(); ++ModuleIndex)
	{
		const FString OwnerLabel = FString::Printf(
			TEXT("%s.PropModules[%d]"),
			*GetName(),
			ModuleIndex
		);

		if (FMaterialSlotBindingValidator::ValidateMaterialSlots(
			PropModules[ModuleIndex].MaterialSlots,
			OwnerLabel,
			Context) == EDataValidationResult::Invalid)
		{
			bHasErrors = true;
		}
	}

	return bHasErrors
		? EDataValidationResult::Invalid
		: EDataValidationResult::Valid;
}
#endif