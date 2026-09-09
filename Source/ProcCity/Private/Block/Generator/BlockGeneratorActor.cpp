// Fill out your copyright notice in the Description page of Project Settings.

#include "ProcCity/Block/Generator/BlockGeneratorActor.h"

#include "ProcCity/Building/DataAssets/BuildingArchetypeDataAsset.h" 
#include "ProcCity/Building/Generator/BuildingGeneratorActor.h"     
#include "ProcCity/Environment/Types/BlockEnvironmentFeatureTypes.h"
#include "ProcCity/Environment/Utils/BlockEnvironmentUtils.h"
#include "ProcCity/Environment/Generator/BlockEnvironmentActor.h"
#include "ProcCity/Environment/Types/ResolvedBlockEnvironmentPlan.h"
#include "ProcCity/Environment/Resolver/BlockEnvironmentPlanResolver.h"
#include "ProcCity/Environment/Resolver/BlockEnvFeaturePostprocessor.h"
#include "ProcCity/Common/Math/RandomUtils.h"

#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"

namespace
{
	FColor GetPathColor(const EBlockPathRole Role)
	{
		switch (Role)
		{
		case EBlockPathRole::Alley:
			return FColor(80, 160, 255);
		case EBlockPathRole::ServiceLane:
			return FColor(180, 80, 255);
		case EBlockPathRole::PedestrianPath:
			return FColor(200, 240, 255);
		default:
			return FColor::White;
		}
	}
	
	FColor GetPointColor(const EBlockPointRole Role)
	{
		switch (Role)
		{
		case EBlockPointRole::Tree:
			return FColor::Green;
		case EBlockPointRole::Light:
			return FColor(255, 255, 120);
		case EBlockPointRole::Bench:
			return FColor(180, 140, 80);
		case EBlockPointRole::Utility:
			return FColor::Red;
		case EBlockPointRole::StreetFurniture:
			return FColor::White;
		default:
			return FColor::White;
		}
	}
	
	const TCHAR* FrontageSideToString(const ELotFrontageSide Side)
	{
		switch (Side)
		{
		case ELotFrontageSide::South: 
			return TEXT("South");
		case ELotFrontageSide::North: 
			return TEXT("North");
		case ELotFrontageSide::West: 
			return TEXT("West");
		case ELotFrontageSide::East: 
			return TEXT("East");
		case ELotFrontageSide::InteriorLane: 
			return TEXT("InteriorLane");
		default: 
			return TEXT("None");
		}
	}
}

// Sets default values
ABlockGeneratorActor::ABlockGeneratorActor()
{
 	// Set this actor to call Tick() every frame. You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("BlockRoot"));
	SetRootComponent(Root);
}

void ABlockGeneratorActor::OnConstruction(const FTransform& Transform)
{
	// May be removed in future
	
	Super::OnConstruction(Transform);
	
	// Intentionally disabled to avoid expensive regeneration on every edit change.
	// RegenerateBlock();
}

void ABlockGeneratorActor::RegenerateBlock()
{
	GenerateBlockLayout();
	ResolveBuildingArchetypesForLots();
	
	if (bAutoRegenerateEnvironment)
	{
		RegenerateEnvironmentFromBlock();
	}
	
	if (bAutoRegenerateBuildings)
	{
		RegenerateBuildingsFromBlock();
	}
}

void ABlockGeneratorActor::ClearAll()
{
	ClearDebugVisualization();
	
	ClearGeneratedLayout();
	ImportedLayoutData = FImportedBlockLayoutData();
	
	ClearManagedBuildingGenerator();
	ClearManagedEnvironmentGenerator();
}

void ABlockGeneratorActor::RegenerateBuildingsFromBlock()
{
	GenerateBuildingPlans();
	PushPlansToBuildingGenerator();
}

void ABlockGeneratorActor::ClearManagedBuildingGenerator()
{
	if (IsValid(ManagedBuildingGeneratorActor))
	{
		ManagedBuildingGeneratorActor->Destroy();
		ManagedBuildingGeneratorActor = nullptr;
	}
	GeneratedBuildingPlans.Reset();
}

void ABlockGeneratorActor::ClearManagedEnvironmentGenerator()
{
	if (IsValid(ManagedBlockEnvironmentActor))
	{
		ManagedBlockEnvironmentActor->Destroy();
		ManagedBlockEnvironmentActor = nullptr;
	}
}

void ABlockGeneratorActor::GenerateBlockLayout()
{

	FString Error;
	if (!LoadBlockLayoutFromJson(Error))
	{
		UE_LOG(LogTemp, Warning, 
		TEXT("Failed to load block layout: %s"), *Error);
	}
	
	return;
}

bool ABlockGeneratorActor::LoadBlockLayoutFromJson(FString& OutError)
{
	ClearGeneratedLayout();
	
	ImportedLayoutData = FImportedBlockLayoutData();
	
	FString FilePath = JsonLayoutFile.FilePath.TrimStartAndEnd();
	if (FilePath.IsEmpty())
	{
		OutError = FString(TEXT("File path is empty."));
		return false;
	}
	
	// If a relative path is provided, resolve it against the project directory.
	if (!FPaths::FileExists(FilePath))
	{
		const FString ResolvedFilePath = FPaths::ConvertRelativePathToFull(
			FPaths::ProjectDir(), 
			FilePath	
		);
		
		if (!FPaths::FileExists(ResolvedFilePath))
		{
			FilePath = ResolvedFilePath;
		}
	}
	
	if (!FPaths::FileExists(FilePath))
	{
		OutError = FString::Printf(
			TEXT("JSON layout file does not exist: %s"),
			*JsonLayoutFile.FilePath
		);
		return false;
	}
	
	if (!FBlockLayoutJsonImporter::ImportFromFile(
		FilePath, ImportedLayoutData, OutError))
	{
		return false;
	}
	
	GeneratedLayout = ImportedLayoutData.GeneratedLayout;
	return true;
}


void ABlockGeneratorActor::ClearGeneratedLayout()
{
	GeneratedLayout = FGeneratedBlockLayout();
}


// Fill the [SelectedArchetypeId] field of lot.
void ABlockGeneratorActor::ResolveBuildingArchetypesForLots()
{
	for (int32 LotIndex = 0; LotIndex < GeneratedLayout.Lots.Num(); ++LotIndex)
	{
		FLotDefinition& Lot = GeneratedLayout.Lots[LotIndex];
		
		UBuildingArchetypeDataAsset* 
			SelectedArchetype = PickArchetypeForLot(Lot, LotIndex);
		
		Lot.SelectedArchetypeId = IsValid(SelectedArchetype) 
			? SelectedArchetype->ArchetypeId : NAME_None;
	}
}

// Invoking by [ResolveBuildingArchetypesForLots]
UBuildingArchetypeDataAsset* ABlockGeneratorActor::PickArchetypeForLot(
	const FLotDefinition& Lot,
	int32 LotIndex
) const
{
	TArray<UBuildingArchetypeDataAsset*> Candidates;
	int32 TotalWeight = 0;
	
	FBlockBuildingFeature BuildingFeature;
	if (!GetBuildingFeatureForLot(Lot, BuildingFeature))
	{
		return nullptr;
	}
	
	for (UBuildingArchetypeDataAsset* Archetype : BuildingArchetypes)
	{
		if (!IsValid(Archetype))
		{
			continue;
		}
		
		if (!DoesBuildingArchetypeFitBound(Archetype, BuildingFeature))
		{
			UE_LOG(LogTemp, Warning, TEXT("Building Archetypes does not fit in bounds"));
			continue;
		}

		TotalWeight += FMath::Max(1, Archetype->Weight);
		Candidates.Add(Archetype);
	}
	
	if (Candidates.Num() <= 0 || TotalWeight <= 0)
	{
		return nullptr;
	}
	
	FRandomStream ArchetypesRandom(BuildingArchetypeSeed + LotIndex * 7919);
	const int32 Pick = ArchetypesRandom.RandRange(1, TotalWeight);
	
	int32 Running = 0;
	for (UBuildingArchetypeDataAsset* Candidate : Candidates)
	{
		Running += FMath::Max(1, Candidate->Weight);
		if (Pick <= Running)
		{
			return Candidate;
		}
	}
	return Candidates[0];
}

// Invoking by [PickArchetypeForLot]
bool ABlockGeneratorActor::GetBuildingFeatureForLot(
	const FLotDefinition& Lot,
	FBlockBuildingFeature& OutFeature
) const
{
	OutFeature = FBlockBuildingFeature();
	
	// Use BuildingFeature from external JSON (already stored in GeneratedLayout)
	if (Lot.LotId.IsNone())
	{
		return false;
	}
	
	for (const FBlockBuildingFeature& BuildingFeature 
		: GeneratedLayout.BuildingFeatures)
	{
		if (BuildingFeature.ParcelId == Lot.LotId)
		{
			OutFeature = BuildingFeature;
			return true;
		}
	}
	return false;
}

bool ABlockGeneratorActor::DoesBuildingArchetypeFitBound(
	const UBuildingArchetypeDataAsset* Archetype,
	const FBlockBuildingFeature& BuildingFeature
)
{
	if (!IsValid(Archetype))
	{
		return false;
	}
	
	// TODO: Current version only supports rect bound fit check
	if (BuildingFeature.GeometryType != EBlockFeatureGeometryType::Rect)
	{
		return false;
	}
	
	const FVector2D Size = BuildingFeature.LocalRect.GetSize();
	if (Size.X < Archetype->MinWidth || Size.Y < Archetype->MinDepth)
	{
		return false;
	}
	
	return true;
}


UBuildingArchetypeDataAsset* 
	ABlockGeneratorActor::FindBuildingArchetypeById(FName ArchetypeId) const
{
	if (ArchetypeId.IsNone())
	{
		return nullptr;
	}
	
	for (UBuildingArchetypeDataAsset* Archetype : BuildingArchetypes)
	{
		if (IsValid(Archetype) && Archetype->ArchetypeId == ArchetypeId)
		{
			return Archetype;
		}
	}
	return nullptr;
}

void ABlockGeneratorActor::GenerateBuildingPlans()
{
	GeneratedBuildingPlans.Reset();
	GeneratedBuildingPlans.Reserve(GeneratedLayout.Lots.Num());
	
	for (int32 LotIndex = 0; LotIndex < GeneratedLayout.Lots.Num(); ++LotIndex)
	{
		FBuildingPlan Plan;
		if (GenerateBuildingPlanForLot(LotIndex, Plan))
		{
			GeneratedBuildingPlans.Add(Plan);
		}
	}
}

bool ABlockGeneratorActor::GenerateBuildingPlanForLot(
	int32 LotIndex, FBuildingPlan& OutPlan
	) const
{
	OutPlan = FBuildingPlan();
	
	if (!GeneratedLayout.Lots.IsValidIndex(LotIndex))
	{
		return false;
	}
	
	const FLotDefinition& Lot = GeneratedLayout.Lots[LotIndex];
	UBuildingArchetypeDataAsset* Archetype = 
		FindBuildingArchetypeById(Lot.SelectedArchetypeId);
	if (!IsValid(Archetype))
	{
		return false;
	}
	
	// Obtain building center
	FBlockBuildingFeature BuildingFeature;
	if (!GetBuildingFeatureForLot(Lot, BuildingFeature))
	{
		return false;
	}
	
	const FVector2D BuildingCenter2D = 
		(BuildingFeature.LocalRect.Min + BuildingFeature.LocalRect.Max) * 0.5f;
	
	// TODO: Some offset Z may be needed here.
	const FVector BuildingCenterLocal = 
		FVector(BuildingCenter2D.X, BuildingCenter2D.Y, 0.0f);
	
	// Transform to world space
	const FVector BuildingCenterWorld = 
		GetActorTransform().TransformPosition(BuildingCenterLocal);
	
	// Building world rotation is aligned with block rotation
	// Get the required building rotation in block local according to [Lot.FrontageSide]
	// The building front side in its local space is assumed to be South (Y-)
	const FRotator BuildingLocalRotation = 
		GetBuildingLocalRotationFromFrontage(Lot.FrontageSide);
	const FQuat BuildingLocalQuat = BuildingLocalRotation.Quaternion();
	
	const FQuat BlockWorldQuat = GetActorQuat();
	const FQuat BuildingWorldQuat = BlockWorldQuat * BuildingLocalQuat;
	const FRotator BuildingWorldRotation = BuildingWorldQuat.Rotator();
	
	OutPlan.Archetype = Archetype;
	OutPlan.BuildingIndex = LotIndex;
	OutPlan.FinalOrigin = BuildingCenterWorld;
	OutPlan.FinalRotation = BuildingWorldRotation;
	
	OutPlan.BaseSeed = ProcCity::DeriveSeed(BuildingPlanSeed, LotIndex);
	OutPlan.ArchetypeSeed = ProcCity::DeriveSeed(BuildingArchetypeSeed, LotIndex);
	OutPlan.RuleSeed = ProcCity::DeriveSeed(BuildingRuleSeed, LotIndex);
	OutPlan.ModuleSeed = ProcCity::DeriveSeed(BuildingModuleSeed, LotIndex);
	OutPlan.MaterialSeed = ProcCity::DeriveSeed(BuildingMaterialSeed, LotIndex);
	
	return true;
}

ABuildingGeneratorActor* ABlockGeneratorActor::EnsureBuildingGeneratorActor()
{
	if (IsValid(ManagedBuildingGeneratorActor))
	{
		return ManagedBuildingGeneratorActor;
	}
	
	if (!bAutoCreateBuildingGeneratorActor)
	{
		return nullptr;
	}
	
	if (!BuildingGeneratorActorClass)
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	
	// Get the spawn location of building generator actor 
	const FVector SpawnLocation = 
		GetActorTransform().TransformPosition(BuildingGeneratorActorOffset);
	
	// Get the spawn rotation of building generator actor
	const FRotator SpawnRotation =
		bAlignBuildingGeneratorActorRotationToBlock
			? GetActorRotation()
			: FRotator::ZeroRotator;
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = 
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	ManagedBuildingGeneratorActor = World->SpawnActor<ABuildingGeneratorActor>(
		BuildingGeneratorActorClass,
		SpawnLocation,
		SpawnRotation,
		SpawnParams
	);
	
	return ManagedBuildingGeneratorActor;
}

void ABlockGeneratorActor::PushPlansToBuildingGenerator()
{
	ABuildingGeneratorActor* BuildingGenActor = EnsureBuildingGeneratorActor();
	if (!IsValid(BuildingGenActor))
	{
		return;
	}
	
	BuildingGenActor->SetActorLocation(
		GetActorTransform().TransformPosition(BuildingGeneratorActorOffset)
		);
	
	BuildingGenActor->SetActorRotation(
		bAlignBuildingGeneratorActorRotationToBlock
		? GetActorRotation()
		: FRotator::ZeroRotator
	);
	
	BuildingGenActor->Modify();
	BuildingGenActor->SetUseExternalPlans(true);
	BuildingGenActor->SetExternalBuildingPlans(GeneratedBuildingPlans);
	
	// Generated buildings will be cleared in [Regenerate()] before regenerate
	BuildingGenActor->Regenerate();
}

ABlockEnvironmentActor* ABlockGeneratorActor::EnsureBlockEnvironmentActor()
{
	if (IsValid(ManagedBlockEnvironmentActor))
	{
		return ManagedBlockEnvironmentActor;
	}
	
	if (!bAutoCreateEnvironmentActor)
	{
		return nullptr;
	}
	
	if (!BlockEnvironmentActorClass)
	{
		return nullptr;
	}
	
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	
	const FVector SpawnLocation = 
		GetActorTransform().TransformPosition(BlockEnvironmentActorOffset);
	
	const FRotator SpawnRotation = bAlignEnvironmentActorRotationToBlock 
		? GetActorRotation() : FRotator::ZeroRotator;
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	ManagedBlockEnvironmentActor = World->SpawnActor<ABlockEnvironmentActor>(
		BlockEnvironmentActorClass,
		SpawnLocation,
		SpawnRotation,
		SpawnParams
	);
	
	return ManagedBlockEnvironmentActor;
}

void ABlockGeneratorActor::PushEnvironmentPlanToActor()
{
	ABlockEnvironmentActor* EnvironmentActor = EnsureBlockEnvironmentActor();
	if (!IsValid(EnvironmentActor))
	{
		return;
	}
	
	EnvironmentActor->SetActorLocation(
		GetActorTransform().TransformPosition(BlockEnvironmentActorOffset)	
	);
	
	EnvironmentActor->SetActorRotation(
		bAlignEnvironmentActorRotationToBlock 
		? GetActorRotation() 
		: FRotator::ZeroRotator
	);
	
	// Apply Env feature postprocessing, e.g., per Prop yaw random
	FBlockEnvFeaturePostprocessSettings Setting;
	Setting.MasterSeed = EnvFeaturePostprocessSeed;
	Setting.bEnablePointRandomYaw = bEnableEnvPropRandomYaw;
	
	FBlockEnvFeaturePostprocessor Postprocessor(Setting);
	FGeneratedBlockLayout ProcessedLayout;
	
	if (!Postprocessor.PostProcess(
		GeneratedLayout, 
		ProcessedLayout))
	{
		return;
	}
	
	FResolvedBlockEnvironmentPlan ResolvedPlan;
	// Do not update env if [Resolve] fails
	if (!FBlockEnvironmentPlanResolver::Resolve(
		ProcessedLayout, EnvironmentStyle, ResolvedPlan))
	{
		return;
	}
	
	// [EnvironmentActor->RegenerateEnvironment()] is responsible to clear the 
	// generated Env before regenerating.
	EnvironmentActor->SetResolvedEnvironmentPlan(ResolvedPlan);
	EnvironmentActor->RegenerateEnvironment();
}

void ABlockGeneratorActor::RegenerateEnvironmentFromBlock()
{
	PushEnvironmentPlanToActor();
}


FVector ABlockGeneratorActor::GetBox2DCenter3D(const FBox2D& Box, const float Z)
{
	const FVector2D Center2D = (Box.Min + Box.Max) * 0.5f;
	return FVector(Center2D.X, Center2D.Y, Z);
}

FVector ABlockGeneratorActor::GetBox2DExtent3D(const FBox2D& Box, const float ZExtent)
{
	const FVector2D Extent2D = (Box.Max - Box.Min) * 0.5f;
	return FVector(Extent2D.X, Extent2D.Y, ZExtent);
}

int32 ABlockGeneratorActor::GetLotStreetEdgeCount(const FLotDefinition& Lot)
{
	return static_cast<int32>(Lot.bHasWestStreet) + 
		static_cast<int32>(Lot.bHasEastStreet) + 
			static_cast<int32>(Lot.bHasSouthStreet) + 
				static_cast<int32>(Lot.bHasNorthStreet);
}

FRotator ABlockGeneratorActor::
GetBuildingLocalRotationFromFrontage(const ELotFrontageSide FrontageSide)
{
	switch (FrontageSide)
	{
	case ELotFrontageSide::West:
		return FRotator(0.0f, 90.0f, 0.0f);
		
	case ELotFrontageSide::East:
		return FRotator(0.0f, -90.0f, 0.0f);
		
	case ELotFrontageSide::North:
		return FRotator(0.0f, 180.0f, 0.0f);
		
	case ELotFrontageSide::South:
		return FRotator::ZeroRotator;
		
	case ELotFrontageSide::InteriorLane:
		return FRotator::ZeroRotator;
		
	case ELotFrontageSide::None:
	default:
		return FRotator::ZeroRotator;
	}
}


#if WITH_EDITOR
void ABlockGeneratorActor::RebuildDebugVisualization()
{
	ClearDebugVisualization();
	
	if (bDrawSurfaceBounds)
	{
		BuildDebugSurfaceAreaVisualization();
	}

	if (bDrawPaths)
	{
		BuildDebugPathVisualization();
	}
	
	if (bDrawPathJunctions)
	{
		BuildDebugPathJunctionVisualization();
	}

	if (bDrawPlacementPoints)
	{
		BuildDebugPlacementPointVisualization();
	}
	
	if (bDrawLots)
	{
		BuildDebugLotVisualization();
	}
	
	if (bDrawBuildingBounds)
	{
		BuildDebugBuildingBoundsVisualization();
	}
}

void ABlockGeneratorActor::ClearDebugVisualization()
{
#if WITH_EDITORONLY_DATA
	for (UBoxComponent* Box : VisualizationBoxes)
	{
		if (IsValid(Box))
		{
			Box->DestroyComponent();
		}
	}
	VisualizationBoxes.Reset();
	
	for (UTextRenderComponent* TextComp : VisualizationTexts)
	{
		if (IsValid(TextComp))
		{
			TextComp->DestroyComponent();
		}
	}
	VisualizationTexts.Reset();
#endif
}

UBoxComponent* ABlockGeneratorActor::CreateVisualizationBox(
	const FName Name,
	const FVector& RelativeLocation,
	const FVector& BoxExtent,
	const FColor& Color
)
{
#if WITH_EDITORONLY_DATA
	UBoxComponent* Box = NewObject<UBoxComponent>(
		this, 
		Name, 
		RF_Transactional | RF_TextExportTransient);
	if (!Box)
	{
		return nullptr;
	}
	
	Box->SetupAttachment(RootComponent);
	Box->SetRelativeLocation(RelativeLocation);
	Box->SetBoxExtent(BoxExtent);
	Box->ShapeColor = Color;
	Box->SetHiddenInGame(true);
	
	Box->SetIsVisualizationComponent(true);
	Box->RegisterComponent();
	
	VisualizationBoxes.Add(Box);
	return Box;
#else
	return nullptr;
#endif
}

UTextRenderComponent* ABlockGeneratorActor::CreateVisualizationText(
	const FName Name,
	const FVector& RelativeLocation,
	const FString& Text,
	const FColor& Color
)
{
#if WITH_EDITORONLY_DATA
	UTextRenderComponent* TextComp = NewObject<UTextRenderComponent>(
		this, 
		Name, 
		RF_Transactional | RF_TextExportTransient);
	if (!TextComp)
	{
		return nullptr;
	}
	
	TextComp->SetupAttachment(RootComponent);
	TextComp->SetRelativeLocation(RelativeLocation);
	TextComp->SetText(FText::FromString(Text));
	TextComp->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	TextComp->SetWorldSize(LotLabelWorldSize);
	TextComp->SetTextRenderColor(Color);
	TextComp->SetHiddenInGame(true);
	
	TextComp->SetIsVisualizationComponent(true);
	TextComp->bAlwaysRenderAsText = true;
	
	TextComp->SetRelativeRotation(FRotator(90.0f, -90.0f, 0.0f));
	TextComp->RegisterComponent();
	
	VisualizationTexts.Add(TextComp);
	return TextComp;
	
#else
	return nullptr;
#endif
}

void ABlockGeneratorActor::BuildDebugPathVisualization()
{
	for (int32 Index = 0; Index < GeneratedLayout.PathFeatures.Num(); ++Index)
	{
		const FBlockPathFeature& Path = GeneratedLayout.PathFeatures[Index];
		
		const FVector Center = (Path.LocalStart + Path.LocalEnd) * 0.5f;
		const FVector Delta = Path.LocalEnd - Path.LocalStart;
		const float Length = Delta.Size2D();
		
		if (Length <= KINDA_SMALL_NUMBER)
		{
			continue;
		}
		
		const bool bMostlyVertical = FMath::Abs(Delta.Y) >= FMath::Abs(Delta.X);
		
		// [Important Note:]
		// The current logic can only handle vertical (i.e., path extends 
		// along Y+) and horizontal (i.e., path extends along X+) path
		
		float PathWidth;
		ProcCity::TryGetPathWidthFromWidthSpec(Path.WidthSpec, PathWidth);
		
		FVector Extent;
		if (bMostlyVertical)
		{
			Extent = FVector(
				PathWidth * 0.5f, 
				Length * 0.5f, 
				PathVisualizationThickness
				);
		}
		else
		{
			Extent = FVector(
				Length * 0.5f, 
				PathWidth * 0.5f, 
				PathVisualizationThickness
				);
		}
		
		CreateVisualizationBox(
			*FString::Printf(TEXT("Viz_Path_%d"), Index),
			Center + FVector(0.0f, 0.0f, 10.0f),
			Extent,
			GetPathColor(Path.Role)
			);
	}
}

void ABlockGeneratorActor::BuildDebugPathJunctionVisualization()
{
	for (int32 Index = 0; Index < GeneratedLayout.PathJunctionFeatures.Num(); ++Index)
	{
		const FBlockPathJunctionFeature& Junction = 
			GeneratedLayout.PathJunctionFeatures[Index];
		
		// Draw Junction as a small box, similar to point objects.
		
		CreateVisualizationBox(
			*FString::Printf(TEXT("Viz_PathJunction_%d"), Index),
			Junction.LocalOrigin + FVector(0.0f, 0.0f, 40.0f),
			FVector(
				JunctionVisualizationExtent, 
				JunctionVisualizationExtent, 
				JunctionVisualizationExtent
				),
				FColor::Red
		);
	}
}

void ABlockGeneratorActor::BuildDebugSurfaceAreaVisualization()
{
	for (int32 Index = 0; Index < GeneratedLayout.SurfaceFeatures.Num(); ++Index)
	{
		const FBlockSurfaceFeature& Surface = GeneratedLayout.SurfaceFeatures[Index];
		
		if (!Surface.LocalRect.bIsValid)
		{
			continue;
		}
		
		FString CategoryString;
		switch (Surface.SurfaceCategory)
		{
		case EBlockSurfaceCategory::Ground:
			CategoryString = TEXT("Ground");
		case EBlockSurfaceCategory::Reserve:
			CategoryString = TEXT("Reserve");
		case EBlockSurfaceCategory::None:
		default:
			break;
		}

		CreateVisualizationBox(
			*FString::Printf(TEXT("Viz_%s_%d"), *CategoryString, Index),
			GetBox2DCenter3D(Surface.LocalRect, 20.0f),
			GetBox2DExtent3D(Surface.LocalRect, SurfaceVisualizationThickness),
			FColor::Green
		);
	}
}

void ABlockGeneratorActor::BuildDebugPlacementPointVisualization()
{
	for (int32 Index = 0; Index < GeneratedLayout.PointFeatures.Num(); ++Index)
	{
		const FBlockPointFeature& Point = GeneratedLayout.PointFeatures[Index];
		const FColor Color = GetPointColor(Point.Role);
		
		CreateVisualizationBox(
			*FString::Printf(TEXT("Viz_Point_%d"), Index),
			Point.LocalOrigin + FVector(0.0f, 0.0f, 40.0f),
			FVector(
				PointVisualizationExtent, 
				PointVisualizationExtent, 
				PointVisualizationExtent
				),
			Color
		);
		
		CreateVisualizationText(
			*FString::Printf(TEXT("Viz_PointText_%d"), Index),
			Point.LocalOrigin + FVector(0.0f, 0.0f, 180.0f),
			UEnum::GetValueAsString(Point.Role),
			Color
		);
	}
}

void ABlockGeneratorActor::BuildDebugLotVisualization()
{
	for (int32 Index = 0; Index < GeneratedLayout.Lots.Num(); ++Index)
	{
		const FLotDefinition& Lot = GeneratedLayout.Lots[Index];

		const FVector LotCenterLocal = GetBox2DCenter3D(
			Lot.LocalRect, 
			LotVisualizationZOffset
			);
		
		const FVector LotExtent = 
			GetBox2DExtent3D(Lot.LocalRect, LotVisualizationThickness) - 
			FVector(5.0f, 5.0f, 0.0f);;
		
		FColor LotColor = FColor::White;
		switch (Lot.PositionType)
		{
		case ELotPositionType::Corner:
			LotColor = FColor(255, 220, 0);
			break;

		case ELotPositionType::Edge:
			LotColor = FColor(255, 120, 0);
			break;

		case ELotPositionType::Interior:
		default:
			LotColor = FColor(255, 2, 255);
			break;
		}
		
		// Create lot box
		CreateVisualizationBox(
			*FString::Printf(TEXT("Viz_LotBox_%d"), Index),
			LotCenterLocal,
			LotExtent,
			LotColor
		);
		
		// Create lot text
		if (bDrawLotLabels)
		{
			const FString Label = FString::Printf(
				TEXT("Lot %d\n%.0f x %.0f\nFrontage: %s\nRearAccess: %s\n%s"),
				Index,
				Lot.Width,
				Lot.Depth,
				FrontageSideToString(Lot.FrontageSide),
				Lot.bHasRearAccess ? TEXT("Yes") : TEXT("No"),
				*Lot.SelectedArchetypeId.ToString()
			);

			CreateVisualizationText(
				*FString::Printf(TEXT("Viz_LotText_%d"), Index),
				LotCenterLocal + FVector(0.0f, 0.0f, LotLabelZOffset),
				Label,
				LotColor
			);
		}
	}
}

void ABlockGeneratorActor::BuildDebugBuildingBoundsVisualization()
{
	for (int32 Index = 0; Index < GeneratedLayout.BuildingFeatures.Num(); ++Index)
	{
		const FBlockBuildingFeature& 
			Building = GeneratedLayout.BuildingFeatures[Index];
		
		if (Building.GeometryType == EBlockFeatureGeometryType::Rect 
			&& Building.LocalRect.bIsValid)
		{
			CreateVisualizationBox(
				*FString::Printf(TEXT("Viz_BuildingFootprint_%d"), Index),
				GetBox2DCenter3D(
					Building.LocalRect, 
					LotVisualizationZOffset + 20.0f
					),
				GetBox2DExtent3D(Building.LocalRect, LotVisualizationThickness),
				FColor(180, 80, 80)
			);
		}
	}
}

#endif

void ABlockGeneratorActor::TestAny()
{
	const FTransform Placement(
		FRotator::ZeroRotator,
		FVector(100.0f, 0.0f, 0.0f),
		FVector(1.0f));
	
	const FTransform BuildingWorld(
		FRotator(0.0f, 90.0f, 0.0f),
		FVector(1000.0f, 0.0f, 0.0f),
		FVector(1.0f)
	);
	
	const FTransform A = Placement * BuildingWorld;
	const FTransform B = BuildingWorld * Placement;
	
	UE_LOG(
		LogTemp, 
		Warning, 
		TEXT("A Location: %s Rot: %s"), 
		*A.GetLocation().ToString(),
		*A.Rotator().ToString()
		);
	
	UE_LOG(
		LogTemp, 
		Warning, 
		TEXT("B Location: %s Rot: %s"), 
		*B.GetLocation().ToString(),
		*B.Rotator().ToString()
		);
}
