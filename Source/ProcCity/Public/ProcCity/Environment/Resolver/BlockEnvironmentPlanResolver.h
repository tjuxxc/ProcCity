#pragma once

#include "CoreMinimal.h"
#include "ProcCity/Environment/Types/ResolvedBlockEnvironmentPlan.h"
#include "ProcCity/Material/Resolver/MaterialPayloadResolver.h"
#include "ProcCity/Material/Types/ResolvedMaterialContext.h"
#include "ProcCity/Environment/Types/SurfaceTreatmentTypes.h"

struct FGeneratedBlockLayout;
struct FPathTerminalTreatment;
struct FBlockPathFeature;
struct FBlockPathJunctionFeature;
struct FBlockPointFeature;
struct FBlockSurfaceFeature;

struct FBlockSurfaceModule;
struct FBlockPropModule;
struct FBlockPathSegmentModule;
struct FBlockPathJunctionModule;
struct FPathTerminalTreatmentModule;
struct FSurfaceEdgeStripModule;
struct FSurfaceCornerModule;

enum class EBlockEnvironmentElementType : uint8;
class UBlockEnvironmentStyleDataAsset;
struct FResolvedMaterialMetrics;
struct FResolvedMaterialContext;

/**
 * Resolves block layout semantic/environment features into a final
 * environment realization plan consumable by ABlockEnvironmentActor.
 */


struct PROCCITY_API FBlockEnvironmentPlanResolver
{
private:
	// Surface treatment helper structs
	struct FRectSurfaceEdgeCandidate
	{
		ERectSurfaceEdge WhichEdge = ERectSurfaceEdge::None;
		ESurfaceEdgeTreatmentType TreatmentType = ESurfaceEdgeTreatmentType::None;
		
		float Width = 0.0f;
		float Length = 0.0f;
		
		FVector LocalOrigin = FVector::ZeroVector;
		FRotator LocalRotation = FRotator::ZeroRotator;
	};

	struct FRectSurfaceCornerCandidate
	{
		// Which rect corner this candidate will finally be placed at.
		ERectSurfaceCorner WhichCorner = ERectSurfaceCorner::None;

		EBlockSurfaceCornerType CornerType = EBlockSurfaceCornerType::None;

		// Canonical, unrotated corner-module matching widths.
		// These are used for style/module lookup, not post-rotation world axes.
		float CanonicalHorizontalEdgeWidth = 0.0f;
		float CanonicalVerticalEdgeWidth = 0.0f;

		// How much this corner occupies along the canonical horizontal / vertical legs.
		// These hints are used to trim adjacent edge-strip candidates.
		float CanonicalHorizontalTrimBack = 0.0f;
		float CanonicalVerticalTrimBack = 0.0f;

		// Placement anchor in surface local space.
		FVector LocalOrigin = FVector::ZeroVector;

		// Rotation from canonical corner-module orientation to target rect corner.
		FRotator LocalRotation = FRotator::ZeroRotator;
	};
	
public:
	/**
	 * Resolve the given layout + style into a final block environment plan.
	 *
	 * Returns true if resolution succeeded at a coarse level.
	 * Partial outputs are allowed; invalid or unsupported features may simply
	 * produce no instances.
	 */
	static bool Resolve(
		const FGeneratedBlockLayout& Layout,
		const UBlockEnvironmentStyleDataAsset* Style,
		FResolvedBlockEnvironmentPlan& OutPlan
	);

private:
	
	// ----- Resolve individual feature groups -----
	static void ResolveSurfaceInstanceGroup(
			const FGeneratedBlockLayout& Layout,
			const UBlockEnvironmentStyleDataAsset* Style,
			FResolvedBlockEnvironmentPlan& OutPlan
		);
	
	static void ResolvePathInstanceGroup(
		const FGeneratedBlockLayout& Layout,
		const UBlockEnvironmentStyleDataAsset* Style,
		FResolvedBlockEnvironmentPlan& OutPlan
	);
	
	static void ResolvePathJunctionInstanceGroup(
		const FGeneratedBlockLayout& Layout,
		const UBlockEnvironmentStyleDataAsset* Style,
		FResolvedBlockEnvironmentPlan& OutPlan
	);

	static void ResolvePropInstanceGroup(
		const FGeneratedBlockLayout& Layout,
		const UBlockEnvironmentStyleDataAsset* Style,
		FResolvedBlockEnvironmentPlan& OutPlan
	);
	
	// Surface treatment helper functions
	static void BuildRectSurfaceCornerAndEdgeCandidates(
		const FBlockSurfaceFeature& SurfaceFeature,
		TArray<FRectSurfaceCornerCandidate>& OutCornerCandidates,
		TArray<FRectSurfaceEdgeCandidate>& OutEdgeCandidates
	);
	
	static void AddOneRectSurfaceCornerCandidate(
		const FBlockSurfaceFeature& SurfaceFeature,
		const ERectSurfaceCorner WhichCorner,
		TArray<FRectSurfaceCornerCandidate>& OutCornerCandidates
	);
	
	static void AddOneRectSurfaceEdgeCandidate(
		const FBlockSurfaceFeature& SurfaceFeature,
		const ERectSurfaceEdge WhichEdge,
		const TArray<FRectSurfaceCornerCandidate>& CornerCandidates,
		TArray<FRectSurfaceEdgeCandidate>& OutEdgeCandidates
	);
	
	static void ResolveRectSurfaceEdges(
		const FBlockSurfaceFeature& SurfaceFeature,
		const int32 SurfaceIdx,
		const UBlockEnvironmentStyleDataAsset* Style,
		const TArray<FRectSurfaceEdgeCandidate>& EdgeCandidates,
		FResolvedBlockEnvironmentPlan& OutPlan
	);
	
	static void ResolveRectSurfaceCorners(
		const FBlockSurfaceFeature& SurfaceFeature,
		int32 SurfaceIdx,
		const UBlockEnvironmentStyleDataAsset* Style,
		const TArray<FRectSurfaceCornerCandidate>& CornerCandidates,
		FResolvedBlockEnvironmentPlan& OutPlan
	);
	
	static const FRectSurfaceCornerCandidate* FindCornerCandidate(
			const TArray<FRectSurfaceCornerCandidate>& CornerCandidates,
			const ERectSurfaceCorner WhichCorner
		);
	
	static float GetTrimBackForEdgeFromCorner(
		const FRectSurfaceCornerCandidate& CornerCandidate,
		const ERectSurfaceEdge Edge
	);
	
	static bool GetTrimmedSurfaceRect(
		const FBlockSurfaceFeature& SurfaceFeature,
		FBox2D& OutTrimmedRect
	);
	
	// ----- Per-instance append helpers -----
	static bool AppendResolvedInstance(
		FResolvedBlockEnvironmentPlan& OutPlan,
		EBlockEnvironmentElementType InstanceType,
		FName UsageTag,
		UStaticMesh* Mesh,
		const FResolvedMaterialPayload& MaterialPayload,
		const FTransform& RelativeTransform,
		const FResolvedBlockEnvironmentSourceRef& SourceRef
	);
	
	// ----- Module/material resolve helpers -----
	template<typename TModule> 
	static bool ResolveInstance(
		const TModule& Module,
		EBlockEnvironmentElementType ElementType,
		FName UsageTag,
		const FTransform& RelativeTransform,
		const FResolvedBlockEnvironmentSourceRef& SourceRef,
		FResolvedBlockEnvironmentPlan& OutPlan
	)
	{
		if (!Module.bEnabled || !Module.Mesh)
		{
			return false;
		}
		
		// Convention:
		//   mesh local +Y corresponds to material U
		//   mesh local +X corresponds to material V
		const FVector Scale3D = RelativeTransform.GetScale3D();
		const FResolvedMaterialMetrics MaterialMetrics(
			/* U */ Scale3D.Y,
			/* V */ Scale3D.X);
		
		FResolvedMaterialContext MaterialContext;
		MaterialContext.RelativeTransform = RelativeTransform;
		MaterialContext.Metrics = MaterialMetrics;
		MaterialContext.ElementType = ElementType;
		MaterialContext.UsageTag = UsageTag;
		
		FResolvedMaterialPayload MaterialPayload;
		if (!FMaterialPayloadResolver::ResolveMaterialPayload(
			Module.MaterialSlots, 
			MaterialContext,
			MaterialPayload))
		{
			return false;
		}
	
		return AppendResolvedInstance(
			OutPlan,
			ElementType,
			UsageTag,
			Module.Mesh,
			MaterialPayload,
			RelativeTransform,
			SourceRef
		);
	}
	
	// ----- Transform helpers -----
	static bool BuildPathTreatmentTransform(
		const FPathTerminalTreatment& Treatment,
		const FPathTerminalTreatmentModule& Module,
		FTransform& OutRelativeTransform
	);

	static bool BuildPathSegmentTransform(
		const FBlockPathFeature& Feature,
		const FBlockPathSegmentModule& Module,
		FTransform& OutRelativeTransform,
		const bool bHasStartTreatment,
		const bool bHasEndTreatment
	);

	static bool BuildPathJunctionTransform(
		const FBlockPathJunctionFeature& Feature,
		const FBlockPathJunctionModule& Module,
		FTransform& OutRelativeTransform
	);

	static bool BuildPropTransform(
		const FBlockPointFeature& Feature,
		const FBlockPropModule& Module,
		FTransform& OutRelativeTransform
	);

	static bool BuildSurfaceTransform(
		const FBlockSurfaceFeature& Feature,
		const FBlockSurfaceModule& Module,
		FTransform& OutRelativeTransform
	);
	
	static bool BuildRectSurfaceEdgeTransform(
		const FRectSurfaceEdgeCandidate& Candidate,
		const FSurfaceEdgeStripModule& Module,
		FTransform& OutRelativeTransform
	);
	
	static bool BuildRectSurfaceCornerTransform(
		const FRectSurfaceCornerCandidate& Candidate,
		const FSurfaceCornerModule& Module,
		FTransform& OutRelativeTransform
	);
};