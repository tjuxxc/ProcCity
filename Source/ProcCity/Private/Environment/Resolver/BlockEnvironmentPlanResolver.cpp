#include "ProcCity/Environment/Resolver/BlockEnvironmentPlanResolver.h"

#include "ProcCity/Block/Types/BlockLayoutTypes.h"
#include "ProcCity/Environment/Types/BlockEnvironmentFeatureTypes.h"
#include "ProcCity/Environment/DataAssets/BlockEnvironmentStyleDataAsset.h"
#include "ProcCity/Environment/Types/BlockEnvironmentModuleTypes.h"
#include "ProcCity/Environment/Types/SurfaceTreatmentTypes.h"
#include "ProcCity/Common/Geometry/PositionUtils.h"


// Surface Corner Canonical Mapping helpers
namespace
{
	struct FCanonicalCornerEdgeMapping
	{
		ERectSurfaceEdge CanonicalHorizontalEdge = ERectSurfaceEdge::Top;
		ERectSurfaceEdge CanonicalVerticalEdge = ERectSurfaceEdge::Left;
	};
	
	struct FRectEdgeEndpointCorners
	{
		ERectSurfaceCorner FirstCorner = ERectSurfaceCorner::None;
		ERectSurfaceCorner SecondCorner = ERectSurfaceCorner::None;
	};
	
	const FCanonicalCornerEdgeMapping& GetCanonicalCornerEdgeMapping(
		const ERectSurfaceCorner Corner
	)
	{
		// Canonical module orientation:
		// - canonical horizontal leg = local X+
		// - canonical vertical leg   = local Y+
		//
		// Mapping answers:
		// when that canonical module is rotated/placed onto a target rect corner,
		// which rect edges do the canonical horizontal / vertical legs align to?
		
		static constexpr FCanonicalCornerEdgeMapping TopLeftMapping
		{
			ERectSurfaceEdge::Top,
			ERectSurfaceEdge::Left
		};

		static constexpr FCanonicalCornerEdgeMapping TopRightMapping
		{
			ERectSurfaceEdge::Right,
			ERectSurfaceEdge::Top
		};

		static constexpr FCanonicalCornerEdgeMapping BottomRightMapping
		{
			ERectSurfaceEdge::Bottom,
			ERectSurfaceEdge::Right
		};

		static constexpr FCanonicalCornerEdgeMapping BottomLeftMapping
		{
			ERectSurfaceEdge::Left,
			ERectSurfaceEdge::Bottom
		};

		static constexpr FCanonicalCornerEdgeMapping DefaultMapping
		{
			ERectSurfaceEdge::Top,
			ERectSurfaceEdge::Left
		};
		
		switch (Corner)
		{
		case ERectSurfaceCorner::TopLeft:
			return TopLeftMapping;

		case ERectSurfaceCorner::TopRight:
			return TopRightMapping;

		case ERectSurfaceCorner::BottomRight:
			return BottomRightMapping;

		case ERectSurfaceCorner::BottomLeft:
			return BottomLeftMapping;

		default:
			return DefaultMapping;
		}
	}
	
	const FRectEdgeEndpointCorners& GetRectEdgeEndpointCorners(
		const ERectSurfaceEdge Edge
	)
	{
		static constexpr FRectEdgeEndpointCorners TopCorners
		{
			ERectSurfaceCorner::TopLeft,
			ERectSurfaceCorner::TopRight
		};

		static constexpr FRectEdgeEndpointCorners RightCorners
		{
			ERectSurfaceCorner::TopRight,
			ERectSurfaceCorner::BottomRight
		};

		static constexpr FRectEdgeEndpointCorners BottomCorners
		{
			ERectSurfaceCorner::BottomLeft,
			ERectSurfaceCorner::BottomRight
		};

		static constexpr FRectEdgeEndpointCorners LeftCorners
		{
			ERectSurfaceCorner::TopLeft,
			ERectSurfaceCorner::BottomLeft
		};

		static constexpr FRectEdgeEndpointCorners DefaultCorners
		{
			ERectSurfaceCorner::None,
			ERectSurfaceCorner::None
		};
		
		switch (Edge)
		{
		case ERectSurfaceEdge::Top:
			return TopCorners;

		case ERectSurfaceEdge::Right:
			return RightCorners;

		case ERectSurfaceEdge::Bottom:
			return BottomCorners;

		case ERectSurfaceEdge::Left:
			return LeftCorners;

		default:
			return DefaultCorners;
		}
	}
	
	bool HasEdgeStrip(const FSurfaceEdgeTreatmentRule* Rule)
	{
		return Rule 
			&& Rule->Type != ESurfaceEdgeTreatmentType::None 
			&& Rule->Width > 0.0f;
	}
	
	const FSurfaceEdgeTreatmentRule* FindSurfaceEdgeRule(
		const TArray<FSurfaceEdgeTreatmentRule>& Rules,
		const ERectSurfaceEdge WhichEdge
	)
	{
		for (const FSurfaceEdgeTreatmentRule& Rule : Rules)
		{
			if (Rule.Edge == WhichEdge)
			{
				return &Rule;
			}
		}
		
		return nullptr;
	}
}

bool FBlockEnvironmentPlanResolver::Resolve(
	const FGeneratedBlockLayout& Layout,
	const UBlockEnvironmentStyleDataAsset* Style,
	FResolvedBlockEnvironmentPlan& OutPlan
)
{
	OutPlan.Reset();
	
	if (!Style)
	{
		return false;
	}
	
	ResolveSurfaceInstanceGroup(Layout, Style, OutPlan);
	ResolvePathInstanceGroup(Layout, Style, OutPlan);
	ResolvePathJunctionInstanceGroup(Layout, Style, OutPlan);
	ResolvePropInstanceGroup(Layout, Style, OutPlan);
	
	return true;
}

void FBlockEnvironmentPlanResolver::ResolveSurfaceInstanceGroup(
	const FGeneratedBlockLayout& Layout,
	const UBlockEnvironmentStyleDataAsset* Style,
	FResolvedBlockEnvironmentPlan& OutPlan
)
{
	for (int32 SurfaceIdx = 0; 
		SurfaceIdx < Layout.SurfaceFeatures.Num(); ++SurfaceIdx)
	{
		const FBlockSurfaceFeature& SurfaceFeature = 
			Layout.SurfaceFeatures[SurfaceIdx];
		
		if (SurfaceFeature.SurfaceCategory == EBlockSurfaceCategory::None)
		{
			continue;
		}
		
		if (SurfaceFeature.SurfaceCategory == EBlockSurfaceCategory::Ground
			&& SurfaceFeature.GroundRole == EBlockGroundRole::None)
		{
			continue;
		}
		
		if (SurfaceFeature.SurfaceCategory == EBlockSurfaceCategory::Reserve
			&& SurfaceFeature.ReserveRole == EBlockReserveRole::None)
		{
			continue;
		}
		
		const FBlockSurfaceModule* Module = Style ? 
			Style->GetSurfaceModule(SurfaceFeature) : nullptr;
		
		if (!Module || !Module->bEnabled || !Module->Mesh)
		{
			continue;
		}
		
		// Source record for debug
		FResolvedBlockEnvironmentSourceRef SurfaceSourceRef;
		SurfaceSourceRef.SourceIndex = SurfaceIdx;
		SurfaceSourceRef.SourceCategory = TEXT("SurfaceFeature");
		
		
		FTransform RelativeTransform = FTransform::Identity;
		if (!BuildSurfaceTransform(SurfaceFeature, *Module, 
			RelativeTransform))
		{
			continue;
		}
		
		// Resolve surface interior
		ResolveInstance(
			*Module,
			EBlockEnvironmentElementType::Surface,
			FName(TEXT("HISM_Surface")),
			RelativeTransform,
			SurfaceSourceRef,
			OutPlan
		);
		
		// Resolve surface edges/corners candidates
		TArray<FRectSurfaceCornerCandidate> CornerCandidates;
		TArray<FRectSurfaceEdgeCandidate> EdgeCandidates;
		BuildRectSurfaceCornerAndEdgeCandidates(
			SurfaceFeature, 
			CornerCandidates, 
			EdgeCandidates
		);
		
		
		if (EdgeCandidates.Num() > 0)
		{
			// UE_LOG(LogTemp, Warning, TEXT("Has edge candidates"));
			ResolveRectSurfaceEdges(SurfaceFeature, 
				SurfaceIdx, Style, EdgeCandidates, OutPlan);
		}
		
		if (CornerCandidates.Num() > 0)
		{
			// UE_LOG(LogTemp, Warning, TEXT("Has corner candidates"));
			ResolveRectSurfaceCorners(SurfaceFeature, 
				SurfaceIdx, Style, CornerCandidates, OutPlan);
		}
	}
}

void FBlockEnvironmentPlanResolver::ResolvePathJunctionInstanceGroup(
	const FGeneratedBlockLayout& Layout,
	const UBlockEnvironmentStyleDataAsset* Style,
	FResolvedBlockEnvironmentPlan& OutPlan
)
{
	for (int32 JunctionIdx = 0; 
		JunctionIdx < Layout.PathJunctionFeatures.Num(); ++JunctionIdx)
	{
		const FBlockPathJunctionFeature& JunctionFeature = 
			Layout.PathJunctionFeatures[JunctionIdx];
		
		if (JunctionFeature.JunctionType == EBlockPathJunctionType::None)
		{
			continue;
		}
		
		const FBlockPathJunctionModule* Module = Style ? 
			Style->GetPathJunctionModule(JunctionFeature) : nullptr;
		
		if (!Module || !Module->bEnabled || !Module->Mesh)
		{
			continue;
		}
		
		// Source record for debug
		FResolvedBlockEnvironmentSourceRef JunctionSourceRef;
		JunctionSourceRef.SourceIndex = JunctionIdx;
		JunctionSourceRef.SourceCategory = TEXT("JunctionFeature");
		
		FTransform RelativeTransform = FTransform::Identity;
		if (!BuildPathJunctionTransform(JunctionFeature, *Module, 
			RelativeTransform))
		{
			continue;
		}
		
		// Resolve path junction instance
		ResolveInstance(
			*Module,
			EBlockEnvironmentElementType::PathJunction,
			FName(TEXT("HISM_PathJunction")),
			RelativeTransform,
			JunctionSourceRef,
			OutPlan
		);
	}
}

void FBlockEnvironmentPlanResolver::ResolvePropInstanceGroup(
	const FGeneratedBlockLayout& Layout,
	const UBlockEnvironmentStyleDataAsset* Style,
	FResolvedBlockEnvironmentPlan& OutPlan
)
{
	for (int32 PropIdx = 0; 
		PropIdx < Layout.PointFeatures.Num(); ++PropIdx)
	{
		const FBlockPointFeature& PropFeature = Layout.PointFeatures[PropIdx]; 
		
		if (PropFeature.Role == EBlockPointRole::None)
		{
			continue;
		}
		
		const FBlockPropModule* Module = Style ? 
			Style->GetPropModule(PropFeature) : nullptr;
		
		if (!Module || !Module->bEnabled || !Module->Mesh)
		{
			continue;
		}
		
		// Source record for debug
		FResolvedBlockEnvironmentSourceRef PropSourceRef;
		PropSourceRef.SourceIndex = PropIdx;
		PropSourceRef.SourceCategory = TEXT("PointFeature");
		
		
		FTransform RelativeTransform = FTransform::Identity;
		if (!BuildPropTransform(PropFeature, *Module, RelativeTransform))
		{
			continue;
		}
		
		ResolveInstance(
			*Module,
			EBlockEnvironmentElementType::Prop,
			FName(TEXT("HISM_Prop")),
			RelativeTransform,
			PropSourceRef,
			OutPlan
		);
	}
}

void FBlockEnvironmentPlanResolver::ResolvePathInstanceGroup(
	const FGeneratedBlockLayout& Layout,
	const UBlockEnvironmentStyleDataAsset* Style,
	FResolvedBlockEnvironmentPlan& OutPlan
)
{
	for (int32 PathIdx = 0; PathIdx < Layout.PathFeatures.Num(); ++PathIdx)
	{
		const FBlockPathFeature& PathFeature = Layout.PathFeatures[PathIdx];
		
		if (PathFeature.Role == EBlockPathRole::None)
		{
			continue;
		}
		
		const FBlockPathSegmentModule* SegmentModule  = Style ?
			Style->GetPathSegmentModule(PathFeature) : nullptr;
		
		if (!SegmentModule || !SegmentModule->bEnabled || !SegmentModule->Mesh)
		{
			continue;
		}
		
		// Source record for debug
		FResolvedBlockEnvironmentSourceRef PathSourceRef;
		PathSourceRef.SourceIndex = PathIdx;
		PathSourceRef.SourceCategory = TEXT("PathFeature");
		
		bool bHasStartTreatment = false;
		bool bHasEndTreatment = false;
		
		// Resolve start treatment instance
		if (PathFeature.StartTreatment.bEnabled &&
			PathFeature.StartTreatment.TreatmentType != EBlockPathTreatmentType::None)
		{
			const FPathTerminalTreatmentModule* StartTreatmentModule =
				UBlockEnvironmentStyleDataAsset::GetPathTerminalTreatmentModule(
					SegmentModule->SupportedTerminalTreatments,
					PathFeature.StartTreatment,
					true,
					PathFeature.HandednessHint
				);
			
			FTransform RelativeTransform = FTransform::Identity;
			if (StartTreatmentModule && StartTreatmentModule->bEnabled
				&& StartTreatmentModule->Mesh
				&& BuildPathTreatmentTransform(
					PathFeature.StartTreatment,
					*StartTreatmentModule, RelativeTransform)
				)
			{
				bHasStartTreatment = ResolveInstance(
					*StartTreatmentModule,
					EBlockEnvironmentElementType::PathTerminalTreatment,
					FName(TEXT("HISM_PathTerminalTreatment_Start")),
					RelativeTransform,
					PathSourceRef,
					OutPlan
				);
			}
		}
		
		// Resolve end treatment instance
		if (PathFeature.EndTreatment.bEnabled &&
			PathFeature.EndTreatment.TreatmentType != EBlockPathTreatmentType::None)
		{
			const FPathTerminalTreatmentModule* EndTreatmentModule =
				UBlockEnvironmentStyleDataAsset::GetPathTerminalTreatmentModule(
					SegmentModule->SupportedTerminalTreatments,
					PathFeature.EndTreatment,
					false,
					PathFeature.HandednessHint
				);
			
			FTransform RelativeTransform = FTransform::Identity;
			if (EndTreatmentModule && EndTreatmentModule->bEnabled &&
				EndTreatmentModule->Mesh
				&& BuildPathTreatmentTransform(
					PathFeature.EndTreatment,
					*EndTreatmentModule, RelativeTransform)
				)
			{
				bHasEndTreatment = ResolveInstance(
					*EndTreatmentModule,
					EBlockEnvironmentElementType::PathTerminalTreatment,
					FName(TEXT("HISM_PathTerminalTreatment_End")),
					RelativeTransform,
					PathSourceRef,
					OutPlan
				);
			}
		}
		
		// Resolve main path segment body
		FTransform RelativeTransform = FTransform::Identity;
		if (BuildPathSegmentTransform(PathFeature, *SegmentModule,
			RelativeTransform, bHasStartTreatment, bHasEndTreatment))
		{
			ResolveInstance(
				*SegmentModule,
				EBlockEnvironmentElementType::PathSegment,
				FName(TEXT("HISM_PathSegment")),
				RelativeTransform,
				PathSourceRef,
				OutPlan
			);
		}
	}
}

bool FBlockEnvironmentPlanResolver::AppendResolvedInstance(
	FResolvedBlockEnvironmentPlan& OutPlan,
	const EBlockEnvironmentElementType InstanceType,
	const FName UsageTag,
	UStaticMesh* Mesh,
	const FResolvedMaterialPayload& MaterialPayload,
	const FTransform& RelativeTransform,
	const FResolvedBlockEnvironmentSourceRef& SourceRef
)
{
	if (!Mesh)
	{
		return false;
	}
	
	FResolvedBlockEnvironmentInstance& Instance = 
		OutPlan.Instances.AddDefaulted_GetRef();
	
	Instance.InstanceType = InstanceType;
	Instance.UsageTag = UsageTag;
	Instance.Mesh = Mesh;
	Instance.MaterialPayload = MaterialPayload;
	Instance.RelativeTransform = RelativeTransform;
	Instance.bEnabled = true;
	Instance.SourceRef = SourceRef;

	return true;
}


bool FBlockEnvironmentPlanResolver::BuildPathSegmentTransform(
	const FBlockPathFeature& Feature,
	const FBlockPathSegmentModule& Module,
	FTransform& OutRelativeTransform,
	const bool bHasStartTreatment,
	const bool bHasEndTreatment
)
{
	
	if (Feature.GeometryType != EBlockFeatureGeometryType::StraightSegment)
	{
		return false;
	}
	
	// TODO: Current logic only support 2D straight segment
	//       and assumes Start.Z = End.Z
	FVector Start = Feature.LocalStart;
	FVector End = Feature.LocalEnd;
	const FVector Direction = (End - Start).GetSafeNormal();
	
	if (Direction.IsNearlyZero())
	{
		return false;
	}
	
	if (bHasStartTreatment)
	{
		Start += Direction * Feature.StartTreatment.TrimLength;
	}

	if (bHasEndTreatment)
	{
		End -= Direction * Feature.EndTreatment.TrimLength;
	}
	
	// Compute length before adding join overlap
	const float LengthBeforeOffset = FVector::Dist2D(Start, End);
	
	// Add small join overlap.
	Start -= Direction * Module.JoinOverlap;
	End += Direction * Module.JoinOverlap;
	
	const FVector Mid = (Start + End) * 0.5f;
	const FVector RelativeLocation =
		Mid +
		FVector(0.0f, 0.0f, Module.SegmentZOffset) -
		Module.SegmentPivotOffset;
	
	const float Length = FVector::Dist2D(Start, End);
	
	if (Length <= KINDA_SMALL_NUMBER)
	{
		return false;
	}
	
	FVector RelativeScale;
	if (Module.bScalable)
	{
		if (Module.NominalLength <= KINDA_SMALL_NUMBER)
		{
			return false;
		}
		
		RelativeScale = FVector(
			1.0f,
			Length / Module.NominalLength,
			1.0f);
		
	}
	else
	{
		RelativeScale = FVector(
			1.0f,
			Length / LengthBeforeOffset,
			1.0f);
	}
	
	// Assumes the mesh extends along +Y in local space.
	const float YawDeg = FMath::RadiansToDegrees(
		FMath::Atan2(Direction.Y, Direction.X)) - 90.0f;
	
	OutRelativeTransform = FTransform(
		FRotator(0.0f, YawDeg, 0.0f),
		RelativeLocation,
		RelativeScale
	);
	
	return true;
}

bool FBlockEnvironmentPlanResolver::BuildPathTreatmentTransform(
	const FPathTerminalTreatment& Treatment,
	const FPathTerminalTreatmentModule& Module,
	FTransform& OutRelativeTransform
)
{
	const FVector RelativeLocation =
		Treatment.LocalOrigin +
		FVector(0.0f, 0.0f, Module.ZOffset) -
		Module.PivotOffset;
	
	OutRelativeTransform = FTransform(
		FRotator(0.0f, Treatment.RotationDeg, 0.0f),
		RelativeLocation,
		FVector(1.0f)
	);
	
	return true;
}

bool FBlockEnvironmentPlanResolver::BuildPathJunctionTransform(
	const FBlockPathJunctionFeature& Feature,
	const FBlockPathJunctionModule& Module,
	FTransform& OutRelativeTransform
)
{
	const FVector RelativeLocation = 
		Feature.LocalOrigin 
		+ FVector(0.0f, 0.0f, Module.ZOffset)
		- Module.PivotOffset;
	
	OutRelativeTransform = FTransform(
		Feature.LocalRotation, 
		RelativeLocation, 
		FVector(1.0f)
	);
	
	return true;
}

bool FBlockEnvironmentPlanResolver::BuildPropTransform(
	const FBlockPointFeature& Feature,
	const FBlockPropModule& Module,
	FTransform& OutRelativeTransform
)
{
	const FVector RelativeLocation = 
		Feature.LocalOrigin 
		+ FVector(0.0f, 0.0f, Module.ZOffset)
		- Module.PivotOffset;
	
	OutRelativeTransform = FTransform(
		Feature.LocalRotation, 
		RelativeLocation, 
		FVector(1.0f)
	);
	
	return true;
}

bool FBlockEnvironmentPlanResolver::BuildSurfaceTransform(
	const FBlockSurfaceFeature& Feature,
	const FBlockSurfaceModule& Module,
	FTransform& OutRelativeTransform
)
{
	// TODO: Current logic only supports rect surface
	if (Feature.GeometryType != EBlockFeatureGeometryType::Rect)
	{
		return false;
	}
	
	if (!Feature.LocalRect.bIsValid)
	{
		return false;
	}
	
	FBox2D EffectiveRect = Feature.LocalRect;
	// If edge strips exist, shrink the interior surface rect accordingly.
	FBox2D TrimmedRect;
	if (GetTrimmedSurfaceRect(Feature, TrimmedRect))
	{
		EffectiveRect = TrimmedRect;
	}

	// The module origin is assumed at its bottom center.
	const FVector RelativeLocation =
		ProcCity::GetRectCenter3D(EffectiveRect, Module.ZOffset) 
		- Module.PivotOffset;
	
	// If not allow scale, use module's default size.
	FVector RelativeScale = FVector(1.0f);
	if (Module.bScalable)
	{
		// Example: UE default cube has size (100, 100, 100) cm
		const FVector NominalSize = Module.NominalSize;
		if (NominalSize.X <= KINDA_SMALL_NUMBER ||
			NominalSize.Y <= KINDA_SMALL_NUMBER)
		{
			return false;
		}
		
		const FVector2D Size2D = EffectiveRect.GetSize();
		// Assume only X, Y are allowed to be scaled
		RelativeScale = FVector(
			Size2D.X / NominalSize.X,
			Size2D.Y / NominalSize.Y,
			1.0f
		);
	}
	
	// TODO: Rotation logic is not considered yet.
	OutRelativeTransform = FTransform(
		FRotator::ZeroRotator,
		RelativeLocation,
		RelativeScale
	);
	
	return true;
}

bool FBlockEnvironmentPlanResolver::BuildRectSurfaceEdgeTransform(
	const FRectSurfaceEdgeCandidate& Candidate,
	const FSurfaceEdgeStripModule& Module,
	FTransform& OutRelativeTransform
)
{
	if (!Module.Mesh)
	{
		return false;
	}
		
	const float NominalLength = Module.NominalLength;
	if (NominalLength <= KINDA_SMALL_NUMBER)
	{
		return false;
	}
		
	if (Candidate.Length <= KINDA_SMALL_NUMBER 
		|| Candidate.Width <= KINDA_SMALL_NUMBER)
	{
		return false;
	}
		
	const FVector RelativeLocation = Candidate.LocalOrigin 
		- Module.PivotOffset 
		+ FVector(0.0f, 0.0f, Module.ZOffset);

	const FVector RelativeScale = FVector(
		1.0f, 
		Candidate.Length / NominalLength, 
		1.0f
	);
		
	OutRelativeTransform = FTransform(
		Candidate.LocalRotation,
		RelativeLocation,
		RelativeScale
	);
		
	return true;
}

bool FBlockEnvironmentPlanResolver::BuildRectSurfaceCornerTransform(
		const FRectSurfaceCornerCandidate& Candidate,
		const FSurfaceCornerModule& Module,
		FTransform& OutRelativeTransform
	)
{
	if (!Module.Mesh)
	{
		return false;
	}
	
	const FVector RelativeLocation = Candidate.LocalOrigin 
		- Module.PivotOffset 
		+ FVector(0.0f, 0.0f, Module.ZOffset);
	
	OutRelativeTransform = FTransform(
		Candidate.LocalRotation,
		RelativeLocation,
		FVector(1.0f)
	);
	
	return true;
}

void FBlockEnvironmentPlanResolver::BuildRectSurfaceCornerAndEdgeCandidates(
	const FBlockSurfaceFeature& SurfaceFeature,
	TArray<FRectSurfaceCornerCandidate>& OutCornerCandidates,
	TArray<FRectSurfaceEdgeCandidate>& OutEdgeCandidates
)
{
	OutCornerCandidates.Reset();
	OutEdgeCandidates.Reset();
	
	if (!SurfaceFeature.HasValidEdgeTreatment())
	{
		return;
	}
	
	const FVector2D& Min = SurfaceFeature.LocalRect.Min;
	const FVector2D& Max = SurfaceFeature.LocalRect.Max;

	const float RectWidth = Max.X - Min.X;
	const float RectHeight = Max.Y - Min.Y;
	
	if (RectWidth <= KINDA_SMALL_NUMBER || RectHeight <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	
	// Add corner candidates one by one
	AddOneRectSurfaceCornerCandidate(
		SurfaceFeature,
		ERectSurfaceCorner::TopLeft,
		OutCornerCandidates);

	AddOneRectSurfaceCornerCandidate(
		SurfaceFeature,
		ERectSurfaceCorner::TopRight,
		OutCornerCandidates);

	AddOneRectSurfaceCornerCandidate(
		SurfaceFeature,
		ERectSurfaceCorner::BottomRight,
		OutCornerCandidates);

	AddOneRectSurfaceCornerCandidate(
		SurfaceFeature,
		ERectSurfaceCorner::BottomLeft,
		OutCornerCandidates);

	AddOneRectSurfaceEdgeCandidate(
		SurfaceFeature,
		ERectSurfaceEdge::Top,
		OutCornerCandidates,
		OutEdgeCandidates);
	
	// Add edge candidates one by one
	AddOneRectSurfaceEdgeCandidate(
		SurfaceFeature,
		ERectSurfaceEdge::Bottom,
		OutCornerCandidates,
		OutEdgeCandidates);

	AddOneRectSurfaceEdgeCandidate(
		SurfaceFeature,
		ERectSurfaceEdge::Left,
		OutCornerCandidates,
		OutEdgeCandidates);

	AddOneRectSurfaceEdgeCandidate(
		SurfaceFeature,
		ERectSurfaceEdge::Right,
		OutCornerCandidates,
		OutEdgeCandidates);
}

void FBlockEnvironmentPlanResolver::AddOneRectSurfaceCornerCandidate(
	const FBlockSurfaceFeature& SurfaceFeature,
	const ERectSurfaceCorner WhichCorner,
	TArray<FRectSurfaceCornerCandidate>& OutCornerCandidates
)
{
	if (WhichCorner == ERectSurfaceCorner::None)
	{
		return;
	}
	
	// Get Canonical edge rules of a specific corner
	const FCanonicalCornerEdgeMapping& Mapping =
		GetCanonicalCornerEdgeMapping(WhichCorner);
	
	const FSurfaceEdgeTreatmentRule* CanonicalHorizontalRule =
		FindSurfaceEdgeRule(
			SurfaceFeature.EdgeTreatment.EdgeRules,
			Mapping.CanonicalHorizontalEdge);

	const FSurfaceEdgeTreatmentRule* CanonicalVerticalRule =
		FindSurfaceEdgeRule(
			SurfaceFeature.EdgeTreatment.EdgeRules,
			Mapping.CanonicalVerticalEdge);
	
	const bool bHasCanonicalHorizontalEdge = HasEdgeStrip(CanonicalHorizontalRule);
	const bool bHasCanonicalVerticalEdge = HasEdgeStrip(CanonicalVerticalRule);
	
	if (!bHasCanonicalHorizontalEdge && !bHasCanonicalVerticalEdge)
	{
		return;
	}
	
	const float CanonicalHorizontalEdgeWidth =
		bHasCanonicalHorizontalEdge ? CanonicalHorizontalRule->Width : 0.0f;

	const float CanonicalVerticalEdgeWidth =
		bHasCanonicalVerticalEdge ? CanonicalVerticalRule->Width : 0.0f;
	
	const FVector2D& Min = SurfaceFeature.LocalRect.Min;
	const FVector2D& Max = SurfaceFeature.LocalRect.Max;
	
	// Build corner candidate
	FRectSurfaceCornerCandidate Candidate;
	Candidate.WhichCorner = WhichCorner;
	
	// ** Important **:
	// Canonical widths/trimbacks are defined in the unrotated corner-module local space.
	Candidate.CanonicalHorizontalEdgeWidth = CanonicalHorizontalEdgeWidth;
	Candidate.CanonicalVerticalEdgeWidth = CanonicalVerticalEdgeWidth;
	
	// Compute placement origin and rotation.
	switch (WhichCorner)
	{
	case ERectSurfaceCorner::TopLeft:
		Candidate.LocalOrigin = 
			FVector(Max.X, Max.Y, 0.0f);
		Candidate.LocalRotation = 
			FRotator(0.0f, 180.0f, 0.0f);
		break;

	case ERectSurfaceCorner::TopRight:
		Candidate.LocalOrigin = 
			FVector(Min.X, Max.Y, 0.0f);
		Candidate.LocalRotation = 
			FRotator(0.0f, -90.0f, 0.0f);
		break;

	case ERectSurfaceCorner::BottomRight:
		Candidate.LocalOrigin = 
			FVector(Min.X, Min.Y, 0.0f);
		Candidate.LocalRotation = 
			FRotator(0.0f, 0.0f, 0.0f);
		break;

	case ERectSurfaceCorner::BottomLeft:
		Candidate.LocalOrigin = 
			FVector(Max.X, Min.Y, 0.0f);
		Candidate.LocalRotation = 
			FRotator(0.0f, 90.0f, 0.0f);
		break;
		
	default:
		return;
	}

	// ** Important **:
	// Canonical widths/trimbacks are defined in the unrotated corner-module local space.
	if (bHasCanonicalHorizontalEdge && bHasCanonicalVerticalEdge)
	{
		Candidate.CornerType = 
			EBlockSurfaceCornerType::DoubleEdgeStripCorner;
		
		// v1.0 rule:
		// trim-back along one canonical leg equals the width of the adjacent leg.
		Candidate.CanonicalHorizontalTrimBack = CanonicalVerticalEdgeWidth;
		Candidate.CanonicalVerticalTrimBack = CanonicalHorizontalEdgeWidth;

		OutCornerCandidates.Add(Candidate);
		return;
	}
	
	Candidate.CornerType = 
		EBlockSurfaceCornerType::SingleEdgeStripEndCap;
	
	// v1.0 rule:
	// For [SingleEdgeStripEndCap], assume trim-back along one canonical leg 
	// equals the width of this leg
	if (bHasCanonicalHorizontalEdge)
	{
		Candidate.CanonicalHorizontalTrimBack = CanonicalHorizontalEdgeWidth;
		Candidate.CanonicalVerticalTrimBack = 0.0f;
	}
	else
	{
		Candidate.CanonicalHorizontalTrimBack = 0.0f;
		Candidate.CanonicalVerticalTrimBack = CanonicalVerticalEdgeWidth;
	}
	
	OutCornerCandidates.Add(Candidate);
	return;
}

void FBlockEnvironmentPlanResolver::AddOneRectSurfaceEdgeCandidate(
	const FBlockSurfaceFeature& SurfaceFeature,
	const ERectSurfaceEdge WhichEdge,
	const TArray<FRectSurfaceCornerCandidate>& CornerCandidates,
	TArray<FRectSurfaceEdgeCandidate>& OutEdgeCandidates
)
{
	const FSurfaceEdgeTreatmentRule* EdgeRule =
		FindSurfaceEdgeRule(SurfaceFeature.EdgeTreatment.EdgeRules, WhichEdge);
	
	if (!HasEdgeStrip(EdgeRule))
	{
		return;
	}
	
	const FVector2D& Min = SurfaceFeature.LocalRect.Min;
	const FVector2D& Max = SurfaceFeature.LocalRect.Max;
	const FVector2D Center = (Min + Max) * 0.5f;

	const float RectWidth = Max.X - Min.X;
	const float RectHeight = Max.Y - Min.Y;
	
	const FRectEdgeEndpointCorners& Endpoints =
		GetRectEdgeEndpointCorners(WhichEdge);
	
	// Here [FindCornerCandidate] must ensure the first corner at left or above of the second.
	const FRectSurfaceCornerCandidate* FirstCorner =
		FindCornerCandidate(CornerCandidates, Endpoints.FirstCorner);
	
	const FRectSurfaceCornerCandidate* SecondCorner =
		FindCornerCandidate(CornerCandidates, Endpoints.SecondCorner);
	
	// Get trim back length due to corner
	const float FirstInset = FirstCorner ?
		GetTrimBackForEdgeFromCorner(*FirstCorner, WhichEdge) : 0.0f;
	
	const float SecondInset = SecondCorner ?
		GetTrimBackForEdgeFromCorner(*SecondCorner, WhichEdge) : 0.0f;
	
	FRectSurfaceEdgeCandidate Candidate;
	Candidate.WhichEdge = WhichEdge;
	Candidate.TreatmentType = EdgeRule->Type;
	Candidate.Width = EdgeRule->Width;
	
	switch (WhichEdge)
	{
	case ERectSurfaceEdge::Top:
		Candidate.Length = 
			FMath::Max(0.0f, RectWidth - FirstInset - SecondInset);
		
		Candidate.LocalOrigin = FVector(
			Center.X - FirstInset * 0.5f + SecondInset * 0.5f,
			Max.Y - Candidate.Width * 0.5f,
			0.0f);
		Candidate.LocalRotation = FRotator(0.0f, -90.0f, 0.0f);
		break;

	case ERectSurfaceEdge::Bottom:
		Candidate.Length = FMath::Max(0.0f, RectWidth - FirstInset - SecondInset);
		Candidate.LocalOrigin = FVector(
			Center.X - FirstInset * 0.5f + SecondInset * 0.5f,
			Min.Y + Candidate.Width * 0.5f,
			0.0f);
		Candidate.LocalRotation = FRotator(0.0f, 90.0f, 0.0f);
		break;

	case ERectSurfaceEdge::Left:
		Candidate.Length = FMath::Max(0.0f, RectHeight - FirstInset - SecondInset);
		Candidate.LocalOrigin = FVector(
			Max.X - Candidate.Width * 0.5f,
			Center.Y - SecondInset * 0.5f + FirstInset * 0.5f,
			0.0f);
		Candidate.LocalRotation = FRotator(0.0f, 180.0f, 0.0f);
		break;

	case ERectSurfaceEdge::Right:
		Candidate.Length = FMath::Max(0.0f, RectHeight - FirstInset - SecondInset);
		Candidate.LocalOrigin = FVector(
			Min.X + Candidate.Width * 0.5f,
			Center.Y - SecondInset * 0.5f + FirstInset * 0.5f,
			0.0f);
		Candidate.LocalRotation = FRotator::ZeroRotator;
		break;

	default:
		return;
	}
	
	if (Candidate.Length <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	
	OutEdgeCandidates.Add(Candidate);
}

void FBlockEnvironmentPlanResolver::ResolveRectSurfaceEdges(
	const FBlockSurfaceFeature& SurfaceFeature,
	const int32 SurfaceIdx,
	const UBlockEnvironmentStyleDataAsset* Style,
	const TArray<FRectSurfaceEdgeCandidate>& EdgeCandidates,
	FResolvedBlockEnvironmentPlan& OutPlan
)
{
	if (!Style)
	{
		return;
	}
	
	for (const FRectSurfaceEdgeCandidate& Candidate : EdgeCandidates)
	{
		if (Candidate.TreatmentType == ESurfaceEdgeTreatmentType::None)
		{
			continue;
		}
			
		if (Candidate.Width <= KINDA_SMALL_NUMBER ||
			Candidate.Length <= KINDA_SMALL_NUMBER)
		{
			continue;
		}
		
		const FSurfaceEdgeStripModule* EdgeModule = Style->GetSurfaceEdgeModule(
			SurfaceFeature,
			Candidate.TreatmentType,
			Candidate.Width
		);
			
		if (!EdgeModule || !EdgeModule->bEnabled || !EdgeModule->Mesh)
		{
			continue;
		}
		
		// UE_LOG(LogTemp, Warning, TEXT("Has valid module"));
			
		FTransform RelativeTransform = FTransform::Identity;
		if (!BuildRectSurfaceEdgeTransform(Candidate, *EdgeModule, 
			RelativeTransform))
		{
			continue;
		}
			
		FResolvedBlockEnvironmentSourceRef EdgeSourceRef;
		EdgeSourceRef.SourceIndex = SurfaceIdx;
		EdgeSourceRef.SourceCategory = TEXT("SurfaceFeature.Edge");
			
		ResolveInstance(
			*EdgeModule,
			EBlockEnvironmentElementType::Surface,
			FName(TEXT("HISM_SurfaceEdge")),
			RelativeTransform,
			EdgeSourceRef,
			OutPlan
		);
	}
}

void FBlockEnvironmentPlanResolver::ResolveRectSurfaceCorners(
		const FBlockSurfaceFeature& SurfaceFeature,
		int32 SurfaceIdx,
		const UBlockEnvironmentStyleDataAsset* Style,
		const TArray<FRectSurfaceCornerCandidate>& CornerCandidates,
		FResolvedBlockEnvironmentPlan& OutPlan
	)
{
	if (!Style)
	{
		return;
	}
	
	for (const FRectSurfaceCornerCandidate& Candidate : CornerCandidates)
	{
		if (Candidate.CornerType == EBlockSurfaceCornerType::None)
		{
			continue;
		}
		
		const FSurfaceCornerModule* CornerModule = 
			Style->GetSurfaceCornerModule(
				SurfaceFeature,
				Candidate.CornerType,
				Candidate.WhichCorner,
				Candidate.CanonicalHorizontalEdgeWidth,
				Candidate.CanonicalVerticalEdgeWidth
			);
		
		if (!CornerModule || !CornerModule->bEnabled || !CornerModule->Mesh)
		{
			FString CornerTypeName = TEXT("Not Single");
			if (Candidate.CornerType == EBlockSurfaceCornerType::SingleEdgeStripEndCap)
			{
				CornerTypeName = TEXT("SingleEdgeStripEndCap");
			}
			
			UE_LOG(LogTemp, Warning, 
				TEXT("Fail to get corner module. Corner Type: %s. H: %.3f. V: %.3f"), 
				*CornerTypeName, Candidate.CanonicalHorizontalEdgeWidth, Candidate.CanonicalVerticalEdgeWidth);
			continue;
		}
		
		FTransform RelativeTransform = FTransform::Identity;
		if (!BuildRectSurfaceCornerTransform(Candidate, 
			*CornerModule, RelativeTransform))
		{
			continue;
		}
		
		FResolvedBlockEnvironmentSourceRef CornerSourceRef;
		CornerSourceRef.SourceIndex = SurfaceIdx;
		CornerSourceRef.SourceCategory = TEXT("SurfaceFeature.Corner");
		
		ResolveInstance(
			*CornerModule,
			EBlockEnvironmentElementType::Surface,
			FName(TEXT("HISM_SurfaceCorner")),
			RelativeTransform,
			CornerSourceRef,
			OutPlan
		);
	}
}

const FBlockEnvironmentPlanResolver::FRectSurfaceCornerCandidate* 
	FBlockEnvironmentPlanResolver::FindCornerCandidate(
		const TArray<FRectSurfaceCornerCandidate>& CornerCandidates,
		const ERectSurfaceCorner WhichCorner
	)
{
	for (const FRectSurfaceCornerCandidate& Candidate : CornerCandidates)
	{
		if (Candidate.WhichCorner == WhichCorner)
		{
			return &Candidate;
		}
	}
	return nullptr;
}

float FBlockEnvironmentPlanResolver::GetTrimBackForEdgeFromCorner(
	const FRectSurfaceCornerCandidate& CornerCandidate,
	const ERectSurfaceEdge Edge
)
{
	const FCanonicalCornerEdgeMapping& Mapping =
		GetCanonicalCornerEdgeMapping(CornerCandidate.WhichCorner);
		
	if (Edge == Mapping.CanonicalHorizontalEdge)
	{
		return CornerCandidate.CanonicalHorizontalTrimBack;
	}
		
	if (Edge == Mapping.CanonicalVerticalEdge)
	{
		return CornerCandidate.CanonicalVerticalTrimBack;
	}
		
	return 0.0f;
}

bool FBlockEnvironmentPlanResolver::GetTrimmedSurfaceRect(
	const FBlockSurfaceFeature& SurfaceFeature,
	FBox2D& OutTrimmedRect
)
{
	// Get the surface interior rect after removing edge treatments
	
	OutTrimmedRect = FBox2D(EForceInit::ForceInit);
	
	if (SurfaceFeature.GeometryType != EBlockFeatureGeometryType::Rect)
	{
		return false;
	}
	
	if (!SurfaceFeature.LocalRect.bIsValid)
	{
		return false;
	}
	
	const FVector2D& SourceMin = SurfaceFeature.LocalRect.Min;
	const FVector2D& SourceMax = SurfaceFeature.LocalRect.Max;
	
	float TopInset = 0.0f;
	float RightInset = 0.0f;
	float BottomInset = 0.0f;
	float LeftInset = 0.0f;

	if (SurfaceFeature.HasValidEdgeTreatment())
	{
		const FSurfaceEdgeTreatmentRule* TopRule = FindSurfaceEdgeRule(
			SurfaceFeature.EdgeTreatment.EdgeRules,
			ERectSurfaceEdge::Top);

		const FSurfaceEdgeTreatmentRule* RightRule = FindSurfaceEdgeRule(
			SurfaceFeature.EdgeTreatment.EdgeRules,
			ERectSurfaceEdge::Right);

		const FSurfaceEdgeTreatmentRule* BottomRule = FindSurfaceEdgeRule(
			SurfaceFeature.EdgeTreatment.EdgeRules,
			ERectSurfaceEdge::Bottom);

		const FSurfaceEdgeTreatmentRule* LeftRule = FindSurfaceEdgeRule(
			SurfaceFeature.EdgeTreatment.EdgeRules,
			ERectSurfaceEdge::Left);

		TopInset = HasEdgeStrip(TopRule) ? TopRule->Width : 0.0f;
		RightInset = HasEdgeStrip(RightRule) ? RightRule->Width : 0.0f;
		BottomInset = HasEdgeStrip(BottomRule) ? BottomRule->Width : 0.0f;
		LeftInset = HasEdgeStrip(LeftRule) ? LeftRule->Width : 0.0f;
	}
	
	const FVector2D TrimmedMin(
		SourceMin.X + RightInset,
		SourceMin.Y + BottomInset
	);
	
	const FVector2D TrimmedMax(
		SourceMax.X - LeftInset,
		SourceMax.Y - TopInset
	);
	
	if (TrimmedMax.X - TrimmedMin.X <= KINDA_SMALL_NUMBER ||
		TrimmedMax.Y - TrimmedMin.Y <= KINDA_SMALL_NUMBER)
	{
		return false;
	}
	
	OutTrimmedRect = FBox2D(TrimmedMin, TrimmedMax);
	return true;
}