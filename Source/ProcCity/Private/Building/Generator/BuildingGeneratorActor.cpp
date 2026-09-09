// Fill out your copyright notice in the Description page of Project Settings.


#include "ProcCity/Building/Generator/BuildingGeneratorActor.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

#include "ProcCity/Building/DataAssets/BuildingArchetypeDataAsset.h"
#include "ProcCity/Building/DataAssets/BuildingModuleSetDataAsset.h"
#include "ProcCity/Building/DataAssets/BuildingMaterialStyleDataAsset.h"
#include "ProcCity/Building/GenerationRules/BuildingRuleSet.h"
#include "ProcCity/Common/Math/RandomUtils.h"



// Sets default values
ABuildingGeneratorActor::ABuildingGeneratorActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("BuildingRoot"));
	Root->SetMobility(EComponentMobility::Static);
	SetRootComponent(Root);
	
}


void ABuildingGeneratorActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// Regenerate();
}


// Called when the game starts or when spawned
void ABuildingGeneratorActor::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void ABuildingGeneratorActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ABuildingGeneratorActor::Regenerate()
{
	ClearGenerated();
	BuildAllBuildings();
}

void ABuildingGeneratorActor::ClearGenerated()
{
	for (UHISMComp* Comp : HISMComponents)
	{
		if (IsValid(Comp))
		{
			Comp->DestroyComponent();
		}
	}

	HISMComponents.Reset();
	HISMMap.Reset();
}

// Main Building generation logics
void ABuildingGeneratorActor::BuildAllBuildings()
{
	if (bUseExternalPlans)
	{
		GenerateBuildingsFromExternalPlans();
	}
	else
	{
		GenerateBuildingsFromInternalPlans();
	}
}

// Generate from internal plans
void ABuildingGeneratorActor::GenerateBuildingsFromInternalPlans()
{
	if (BuildingCount <= 0 || BuildingsPerRow <= 0)
	{
		return;
	}
	
	TArray<FBuildingPlan> BuildingPlans;
	BuildingPlans.Reserve(BuildingCount);
	
	// Phase 1: Generate raw building plans in local space
	for (int32 BuildingIndex = 0; BuildingIndex < BuildingCount; ++BuildingIndex)
	{
		FBuildingPlan Plan;
		if (GenerateRawInternalBuildingPlan(BuildingIndex, Plan))
		{
			BuildingPlans.Add(MoveTemp(Plan));
		}
	}

	if (BuildingPlans.Num() <= 0)
	{
		return;
	}
	
	// --------------------------------------------------------------------
	// Phase 2: Row-based footprint-aware layout and generate final building plans
	//
	// Rule:
	// - Each building uses its own footprint width/depth
	// - BuildingSpacingX / Y are treated as extra gap between footprints
	// - BuildingsPerRow controls wrapping
	// --------------------------------------------------------------------
	
	const FVector BaseOrigin = GetActorLocation();

	float CurrentX = 0.0f;
	float CurrentY = 0.0f;
	float CurrentRowMaxDepth = 0.0f;

	for (int32 BuildingIndex = 0; BuildingIndex < BuildingPlans.Num(); ++BuildingIndex)
	{
		FBuildingPlan& Plan = BuildingPlans[BuildingIndex];

		FGeneratedBuildingDescription PreviewDescription;
		if (!GenerateBuildingDescriptionFromPlan(Plan, PreviewDescription))
		{
			continue;
		}
		
		const float FootprintWidth = GetDescriptionFootprintWidth(PreviewDescription);
		const float FootprintDepth = GetDescriptionFootprintDepth(PreviewDescription);

		if (BuildingIndex > 0 && (BuildingIndex % BuildingsPerRow) == 0)
		{
			CurrentX = 0.0f;
			CurrentY += CurrentRowMaxDepth + BuildingSpacingY;
			CurrentRowMaxDepth = 0.0f;
		}

		Plan.FinalOrigin = BaseOrigin + FVector(CurrentX, CurrentY, 0.0f);

		CurrentX += FootprintWidth + BuildingSpacingX;
		CurrentRowMaxDepth = FMath::Max(CurrentRowMaxDepth, FootprintDepth);
	}
	
	// Phase 3: Generate final description and instantiate
	for (FBuildingPlan& Plan : BuildingPlans)
	{
		FGeneratedBuildingDescription Description;
		if (!GenerateBuildingDescriptionFromPlan(Plan, Description))
		{
			continue;
		}

		FRandomStream ModuleRandom(Plan.ModuleSeed);
		FRandomStream MaterialRandom(Plan.MaterialSeed);
		
		// Building level world transform
		const FTransform BuildingWorldTransform(
			Plan.FinalRotation, Plan.FinalOrigin);
		
		InstantiateBuildingDescription(
			Plan.Archetype,
			Description,
			BuildingWorldTransform,
			ModuleRandom,
			MaterialRandom
		);
	}
}

// Generate from external plans
void ABuildingGeneratorActor::GenerateBuildingsFromExternalPlans()
{
	if (ExternalBuildingPlans.Num() <= 0)
	{
		return;
	}
	
	for (const FBuildingPlan& Plan : ExternalBuildingPlans)
	{
		if (!IsValid(Plan.Archetype))
		{
			continue;
		}
		
		FGeneratedBuildingDescription Description;
		if (!GenerateBuildingDescriptionFromPlan(Plan, Description))
		{
			continue;
		}
		
		FRandomStream ModuleRandom(Plan.ModuleSeed);
		FRandomStream MaterialRandom(Plan.MaterialSeed);
		
		// Building level world transform
		const FTransform BuildingWorldTransform(
			Plan.FinalRotation, Plan.FinalOrigin);
		
		InstantiateBuildingDescription(
			Plan.Archetype,
			Description,
			BuildingWorldTransform,
			ModuleRandom,
			MaterialRandom
		);
	}
}

void ABuildingGeneratorActor::SetUseExternalPlans(bool bInUseExternalPlans)
{
	bUseExternalPlans = bInUseExternalPlans;
}

void ABuildingGeneratorActor::SetExternalBuildingPlans(
	const TArray<FBuildingPlan>& InPlans
	)
{
	ExternalBuildingPlans = InPlans;
}

UBuildingArchetypeDataAsset* 
ABuildingGeneratorActor::PickArchetype(FRandomStream& RandomStream) const
{
	if (Archetypes.Num() <= 0)
	{
		return nullptr;
	}

	int32 TotalWeight = 0;
	for (const UBuildingArchetypeDataAsset* Archetype : Archetypes)
	{
		if (IsValid(Archetype) &&
			IsValid(Archetype->ModuleSet) &&
			Archetype->RuleSetClass != nullptr)
		{
			TotalWeight += FMath::Max(1, Archetype->Weight);
		}
	}

	if (TotalWeight <= 0)
	{
		return nullptr;
	}

	const int32 Pick = RandomStream.RandRange(1, TotalWeight);
	int32 Running = 0;

	for (UBuildingArchetypeDataAsset* Archetype : Archetypes)
	{
		if (!IsValid(Archetype) ||
			!IsValid(Archetype->ModuleSet) ||
			Archetype->RuleSetClass == nullptr)
		{
			continue;
		}

		Running += FMath::Max(1, Archetype->Weight);
		if (Pick <= Running)
		{
			return Archetype;
		}
	}

	return nullptr;
}

const UBuildingRuleSet* ABuildingGeneratorActor::CreateRuleObject(
	const UBuildingArchetypeDataAsset* Archetype
)
{
	if (!IsValid(Archetype) || Archetype->RuleSetClass == nullptr)
	{
		return nullptr;
	}

	//return NewObject<UBuildingRuleSet>(this, Archetype->RuleSetClass);
	return Archetype->RuleSetClass->GetDefaultObject<UBuildingRuleSet>();
}

bool ABuildingGeneratorActor::GenerateRawInternalBuildingPlan(
	int32 BuildingIndex,
	FBuildingPlan& OutPlan
) const
{
	OutPlan = FBuildingPlan();

	const int32 PerBuildingBaseSeed = ProcCity::DeriveSeed(MasterSeed, BuildingIndex);
	const int32 PerBuildingArchetypeSeed = ProcCity::DeriveSeed(ArchetypeSeed, BuildingIndex);
	const int32 PerBuildingRuleSeed = ProcCity::DeriveSeed(RuleSeed, BuildingIndex);
	const int32 PerBuildingModuleSeed = ProcCity::DeriveSeed(ModuleSeed, BuildingIndex);
	const int32 PerBuildingMaterialSeed = ProcCity::DeriveSeed(MaterialVariationSeed, BuildingIndex);

	FRandomStream ArchetypeRandom(PerBuildingArchetypeSeed);

	UBuildingArchetypeDataAsset* Archetype = PickArchetype(ArchetypeRandom);
	if (!IsValid(Archetype))
	{
		return false;
	}

	const UBuildingRuleSet* Rule = CreateRuleObject(Archetype);
	if (!IsValid(Rule))
	{
		return false;
	}
	
	OutPlan.Archetype = Archetype;
	OutPlan.BuildingIndex = BuildingIndex;
	OutPlan.FinalOrigin = FVector::ZeroVector;       
	// OutPlan.FinalRotation = FRotator::ZeroRotator;  
	
	const int32 PerBuildingYawSeed =
		ProcCity::DeriveSeed(BuildYawSeed, BuildingIndex);
	
	FRandomStream PerBuildingYawRandom(PerBuildingYawSeed);
	
	
	float YawVariation = PerBuildingYawRandom.FRand() * MaxYawVariation;
	
	OutPlan.FinalRotation = FRotator(0.0f, BaseBuildingYaw + YawVariation, 0.0f);
	OutPlan.BaseSeed = PerBuildingBaseSeed;
	OutPlan.ArchetypeSeed = PerBuildingArchetypeSeed;
	OutPlan.RuleSeed = PerBuildingRuleSeed;
	OutPlan.ModuleSeed = PerBuildingModuleSeed;
	OutPlan.MaterialSeed = PerBuildingMaterialSeed;
	
	return true;
}

bool ABuildingGeneratorActor::GenerateBuildingDescriptionFromPlan(
	const FBuildingPlan& Plan,
	FGeneratedBuildingDescription& OutDescription
)
{
	/*
	 Building description, include:
	 - building dimension (e.g. BaysX)
	 - module metrics (e.g. BaySize, Ground/UpperHeight, Thickness)
	 - StyleIndex
	 - module placements in building local space
	*/
	OutDescription = FGeneratedBuildingDescription();
	
	if (!IsValid(Plan.Archetype))
	{
		return false;
	}
	
	if (!IsValid(Plan.Archetype->ModuleSet))
	{
		return false;
	}
	
	const UBuildingRuleSet* Rule = CreateRuleObject(Plan.Archetype);
	if (!IsValid(Rule))
	{
		return false;
	}
	
	FBuildingGenerationInput Input;
	
	// Building local origin
	Input.Origin = FVector::ZeroVector; 
	
	Input.RuleSeed = Plan.RuleSeed;
	Input.MaterialSeed = Plan.MaterialSeed;
	Input.Metrics = Plan.Archetype->DefaultMetrics;
	
	Rule->GenerateBuilding(Plan.Archetype, Input, OutDescription);
	return true;
}


float ABuildingGeneratorActor::GetDescriptionFootprintWidth(
	const FGeneratedBuildingDescription& Description
)
{
	return Description.Dimensions.BaysX * Description.Metrics.BaySize;
}

float ABuildingGeneratorActor::GetDescriptionFootprintDepth(
	const FGeneratedBuildingDescription& Description
) 
{
	return Description.Dimensions.BaysY * Description.Metrics.BaySize;
}

void ABuildingGeneratorActor::InstantiateBuildingDescription(
	const UBuildingArchetypeDataAsset* Archetype,
	const FGeneratedBuildingDescription& Description,
	const FTransform& BuildingWorldTransform,
	FRandomStream& ModuleRandom,
	FRandomStream& MaterialRandom
)
{
	if (!IsValid(Archetype) || !IsValid(Archetype->ModuleSet))
	{
		return;
	}
	
	for (const FPlacedBuildingModule& Placement : Description.Placements)
	{
		// Select building module variant
		const FBuildingModuleVariant* Variant = ResolveVariantForPlacement(
			Archetype,
			Placement.Slot,
			ModuleRandom
		);

		if (!Variant || !IsValid(Variant->Mesh))
		{
			continue;
		}
		
		const FTransform WorldPlacement = 
			Placement.Transform * BuildingWorldTransform;
		
		AddInstance(
			Variant,
			Placement.Slot,
			Archetype->MaterialStyles,
			Placement.StyleIndex,
			WorldPlacement,
			MaterialRandom
		);
	}
}

const FBuildingModuleVariant* ABuildingGeneratorActor::ResolveVariantForPlacement(
	const UBuildingArchetypeDataAsset* Archetype,
	EBuildingSemanticSlot Slot,
	FRandomStream& ModuleRandom
)
{
	if (!IsValid(Archetype) || !IsValid(Archetype->ModuleSet))
	{
		return nullptr;
	}

	/**
	 RandomStream is passed for future extension. This implementation always selects
	 the first valid variant, but we want to have the option of adding more complex
	 selection logic.
	*/
	return Archetype->ModuleSet->PickVariant(Slot, ModuleRandom);
}


ABuildingGeneratorActor::UHISMComp* ABuildingGeneratorActor::GetOrCreateHISM(
	const FBuildingModuleVariant* Variant,
	EBuildingSemanticSlot Slot,
	const TArray<TObjectPtr<UBuildingMaterialStyleDataAsset>>& Styles,
	int32 StyleIndex
)
{
	if (!Variant || !IsValid(Variant->Mesh))
	{
		return nullptr;
	}

	UBuildingMaterialStyleDataAsset* StyleAsset = Styles.IsValidIndex(StyleIndex) 
		? Styles[StyleIndex] : nullptr;

	const FHISMKey Key{ Variant->Mesh, StyleAsset, Slot };

	if (TObjectPtr<UHISMComp>* Found = HISMMap.Find(Key))
	{
		return Found->Get();
	}

	const FString StyleName = IsValid(StyleAsset) ? StyleAsset->GetName() : TEXT("Default");

	const FString CompName = FString::Printf(
		TEXT("HISM_%s_%s_Slot_%d"),
		*Variant->Mesh->GetName(),
		*StyleName,
		static_cast<int32>(Slot)
	);

	UHISMComp* NewComp = NewObject<UHISMComp>(this, *CompName);
	if (!IsValid(NewComp))
	{
		return nullptr;
	}

	NewComp->SetupAttachment(RootComponent);
	NewComp->SetMobility(EComponentMobility::Static);
	NewComp->SetStaticMesh(Variant->Mesh);

	/*
	* Set up the custom data layout for the HISM component. 
	* 0 = DetailOffsetU
	* 1 = DetailOffsetV
	* 2 = DetailRotation
	* 3 = DetailVariation
	*/
	NewComp->NumCustomDataFloats = 4;
	NewComp->RegisterComponent();

	ApplyStyleToHISM(NewComp, Variant, Styles, StyleIndex);

	HISMComponents.Add(NewComp);
	HISMMap.Add(Key, NewComp);

	return NewComp;
}


void ABuildingGeneratorActor::ApplyStyleToHISM(
	UHISMComp* HISM,
	const FBuildingModuleVariant* Variant,
	const TArray<TObjectPtr<UBuildingMaterialStyleDataAsset>>& Styles,
	int32 StyleIndex
)
{
	if (!IsValid(HISM) || !Variant)
	{
		return;
	}

	if (!Styles.IsValidIndex(StyleIndex))
	{
		return;
	}

	const UBuildingMaterialStyleDataAsset* Style = Styles[StyleIndex];
	if (!IsValid(Style))
	{
		return;
	}

	if (!IsValid(Variant->Mesh))
	{
		return;
	}

	const int32 NumMeshSlots = Variant->Mesh->GetStaticMaterials().Num();
	const int32 NumSlotsToApply = FMath::Min(Variant->SlotRoles.Num(), NumMeshSlots);

	for (int32 SlotIndex = 0; SlotIndex < NumSlotsToApply; ++SlotIndex)
	{
		if (UMaterialInterface* Material = 
			Style->GetMaterialByRole(Variant->SlotRoles[SlotIndex]))
		{
			HISM->SetMaterial(SlotIndex, Material);
		}
	}
}


void ABuildingGeneratorActor::MakePerInstanceMaterialData(
	FRandomStream& RandomStream,
	float& OutDetailOffsetU,
	float& OutDetailOffsetV,
	float& OutDetailRotation,
	float& OutDetailVariation
) const
{
	if (!bEnablePerInstanceMaterialVariation)
	{
		OutDetailOffsetU = 0.0f;
		OutDetailOffsetV = 0.0f;
		OutDetailRotation = 0.0f;
		OutDetailVariation = 0.0f;
		return;
	}

	// Used for uv offset of scratches texture
	OutDetailOffsetU = RandomStream.FRandRange(0.0f, DetailUVOffsetMax);
	OutDetailOffsetV = RandomStream.FRandRange(0.0f, DetailUVOffsetMax);

	// Used for uv rotating the scratches texture
	if (bUseDiscreteQuarterTurns)
	{
		const int32 RotIndex = RandomStream.RandRange(0, 3);
		OutDetailRotation = static_cast<float>(RotIndex) * 0.25f;
	}
	else
	{
		OutDetailRotation = RandomStream.FRandRange(0.0f, 1.0f);
	}

	// Used for scratching strength variation?
	const float VariationMin = FMath::Min(DetailVariationMin, DetailVariationMax);
	const float VariationMax = FMath::Max(DetailVariationMin, DetailVariationMax);
	OutDetailVariation = RandomStream.FRandRange(VariationMin, VariationMax);
}


void ABuildingGeneratorActor::AddInstance(
	const FBuildingModuleVariant* Variant,
	EBuildingSemanticSlot Slot,
	const TArray<TObjectPtr<UBuildingMaterialStyleDataAsset>>& Styles,
	int32 StyleIndex,
	const FTransform& WorldTransform,
	FRandomStream& MaterialRandom
)
{
	if (!Variant)
	{
		return;
	}

	UHISMComp* HISM = GetOrCreateHISM(Variant, Slot, Styles, StyleIndex);
	if (!IsValid(HISM))
	{
		return;
	}

	float DetailOffsetU = 0.0f;
	float DetailOffsetV = 0.0f;
	float DetailRotation = 0.0f;
	float DetailVariation = 0.0f;

	MakePerInstanceMaterialData(
		MaterialRandom,
		DetailOffsetU,
		DetailOffsetV,
		DetailRotation,
		DetailVariation
	);

	const int32 InstanceIndex = HISM->AddInstance(WorldTransform, true);
	if (InstanceIndex == INDEX_NONE)
	{
		return;
	}

	HISM->SetCustomDataValue(InstanceIndex, 0, DetailOffsetU, false);
	HISM->SetCustomDataValue(InstanceIndex, 1, DetailOffsetV, false);
	HISM->SetCustomDataValue(InstanceIndex, 2, DetailRotation, false);
	HISM->SetCustomDataValue(InstanceIndex, 3, DetailVariation, true);
}




