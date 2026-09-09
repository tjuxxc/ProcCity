// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProcCity/Block/Types/BlockLayoutTypes.h"
#include "ProcCity/Building/Types/BuildingPlanTypes.h" 
#include "ProcCity/Block/Importer/BlockLayoutJsonImporter.h"

#include "BlockGeneratorActor.generated.h"

class USceneComponent;
// Building related forward declaration
class UBuildingArchetypeDataAsset;
class ABuildingGeneratorActor;
// Block env related forward declaration
class UBlockEnvironmentStyleDataAsset;
class ABlockEnvironmentActor;

// Editor-only visualization components
class UBoxComponent;
class UTextRenderComponent;

UCLASS(Blueprintable)
class PROCCITY_API
ABlockGeneratorActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABlockGeneratorActor();
	
public:
	virtual void OnConstruction(const FTransform& Transform) override;
	
	UFUNCTION(CallInEditor, Category = "ProcCity|Block")
	void RegenerateBlock();
	
	UFUNCTION(CallInEditor, Category = "ProcCity|Block")
	void ClearAll();
	
	UFUNCTION(CallInEditor, Category = "ProcCity|Block")
	void ClearManagedBuildingGenerator();
	
	UFUNCTION(CallInEditor, Category = "ProcCity|Block")
	void ClearManagedEnvironmentGenerator();
	
public:
#if WITH_EDITOR
	// Rebuild all debug visualization
	UFUNCTION(CallInEditor, Category = "ProcCity|Block|Debug")
	void RebuildDebugVisualization();
	
	// Clear all debug visualization
	UFUNCTION(CallInEditor, Category = "ProcCity|Block|Debug")
	void ClearDebugVisualization();
	
	UFUNCTION(CallInEditor, Category = "ProcCity|Block|Debug")
	void TestAny();
#endif

protected:
	UPROPERTY(VisibleAnywhere, Category="Components")
	TObjectPtr<USceneComponent> Root = nullptr;
	
	// Input parameters / data 
	UPROPERTY(EditAnywhere, Category = "Block|Buildings")
	TArray<TObjectPtr<UBuildingArchetypeDataAsset>> BuildingArchetypes;
	
	UPROPERTY(EditAnywhere, Category = "Block|Layout")
	FFilePath JsonLayoutFile;
	
	UPROPERTY(VisibleAnywhere, Transient, Category = "Block|Layout")
	FImportedBlockLayoutData ImportedLayoutData;
	
	// Block plan outcome 
	UPROPERTY(VisibleAnywhere, Transient, Category = "Block")
	FGeneratedBlockLayout GeneratedLayout;
	
	// TODO: Will be move to Building sub module in future
	// Generated building plan. 
	UPROPERTY(VisibleAnywhere, Transient, Category = "Block|Buildings")
	TArray<FBuildingPlan> GeneratedBuildingPlans;
	
	// -------------- Building generation parameters ------------
	
	// Building generation random seeds
	UPROPERTY(EditAnywhere, Category = "Block|Buildings|Random")
	int32 BuildingArchetypeSeed = 12345;
	
	UPROPERTY(EditAnywhere, Category = "Block|Buildings|Random")
	int32 BuildingPlanSeed = 50001;

	UPROPERTY(EditAnywhere, Category = "Block|Buildings|Random")
	int32 BuildingRuleSeed = 50011;

	UPROPERTY(EditAnywhere, Category = "Block|Buildings|Random")
	int32 BuildingModuleSeed = 50021;

	UPROPERTY(EditAnywhere, Category = "Block|Buildings|Random")
	int32 BuildingMaterialSeed = 50031;
	
	// If set true, buildings will be regenerated when calling RegenerateBlock()
	UPROPERTY(EditAnywhere, Category = "Block|Buildings")
	bool bAutoRegenerateBuildings = true;
	
	UPROPERTY(EditAnywhere, Category = "Block|Buildings")
	bool bAutoCreateBuildingGeneratorActor = true;
	
	// Actor used for building generation
	UPROPERTY(EditAnywhere, Category = "Block|Buildings")
	TSubclassOf<ABuildingGeneratorActor> BuildingGeneratorActorClass;
	
	// Currently managed BuildingGeneratorActor object
	UPROPERTY(VisibleAnywhere, Transient, Category = "Block|Buildings")
	TObjectPtr<ABuildingGeneratorActor> ManagedBuildingGeneratorActor = nullptr;
	
	// Position offset of [BuildingGeneratorActor] relative to [BlockGeneratorActor]
	UPROPERTY(EditAnywhere, Category = "Block|Buildings")
	FVector BuildingGeneratorActorOffset = FVector::ZeroVector;
	
	// If set ture, [BuildingGeneratorActor] rotates according to [BlockGeneratorActor]
	UPROPERTY(EditAnywhere, Category = "Block|Buildings")
	bool bAlignBuildingGeneratorActorRotationToBlock = true;
	
	// -------------- Block env generation parameters ------------
	
	// Block Env generation random seeds
	UPROPERTY(EditAnywhere, Category = "Block|Environment|Random")
	int32 EnvFeaturePostprocessSeed = 12345;
	
	UPROPERTY(EditAnywhere, Category = "Block|Environment|Random")
	bool bEnableEnvPropRandomYaw = true;
	
	UPROPERTY(EditAnywhere, Category = "Block|Environment")
	bool bAutoRegenerateEnvironment = true;
	
	UPROPERTY(EditAnywhere, Category = "Block|Environment")
	bool bAutoCreateEnvironmentActor = true;
	
	UPROPERTY(EditAnywhere, Category = "Block|Environment")
	TSubclassOf<ABlockEnvironmentActor> BlockEnvironmentActorClass;
	
	UPROPERTY(VisibleAnywhere, Transient, Category = "Block|Environment")
	TObjectPtr<ABlockEnvironmentActor> ManagedBlockEnvironmentActor = nullptr;
	
	UPROPERTY(EditAnywhere, Category = "Block|Environment")
	TObjectPtr<UBlockEnvironmentStyleDataAsset> EnvironmentStyle = nullptr;
	
	UPROPERTY(EditAnywhere, Category = "Block|Environment")
	FVector BlockEnvironmentActorOffset = FVector::ZeroVector;
	
	UPROPERTY(EditAnywhere, Category = "Block|Environment")
	bool bAlignEnvironmentActorRotationToBlock = true;
	
	// --------- Block visualization debug parameters ------------
	
	UPROPERTY(EditAnywhere, Category = "Block|Debug")
	bool bDrawSurfaceBounds = true;

	UPROPERTY(EditAnywhere, Category = "Block|Debug")
	bool bDrawLots = true;

	UPROPERTY(EditAnywhere, Category = "Block|Debug")
	bool bDrawLotLabels = true;
	
	UPROPERTY(EditAnywhere, Category = "Block|Debug")
	bool bDrawPaths = true;

	UPROPERTY(EditAnywhere, Category = "Block|Debug")
	bool bDrawPlacementPoints = true;	
	
	UPROPERTY(EditAnywhere, Category = "Block|Debug")
	bool bDrawPathJunctions = true;

	UPROPERTY(EditAnywhere, Category = "Block|Debug")
	bool bDrawBuildingBounds = true;
	
	UPROPERTY(EditAnywhere, Category = "Block|Debug", meta = (ClampMin = "0.0"))
	float DebugDrawDuration = 10.0f;

	UPROPERTY(EditAnywhere, Category = "Block|Debug", meta = (ClampMin = "0.0"))
	float LotLabelZOffset = 250.0f;
	
	UPROPERTY(EditAnywhere, Category = "Block|Debug", meta = (ClampMin = "1.0"))
	float LotLabelWorldSize = 96.0f;
	
	UPROPERTY(EditAnywhere, Category = "Block|Debug", meta = (ClampMin = "1.0"))
	float SurfaceVisualizationThickness = 20.0f;
	
	UPROPERTY(EditAnywhere, Category = "Block|Debug", meta = (ClampMin = "1.0"))
	float LotVisualizationThickness = 10.0f;
	
	UPROPERTY(EditAnywhere, Category = "Block|Debug")
	float LotVisualizationZOffset = 60.0f;
	
	UPROPERTY(EditAnywhere, Category = "Block|Debug", meta = (ClampMin = "1.0"))
	float PathVisualizationThickness = 8.0f;
	
	UPROPERTY(EditAnywhere, Category = "Block|Debug", meta = (ClampMin = "1.0"))
	float JunctionVisualizationExtent = 80.0f;
	
	UPROPERTY(EditAnywhere, Category = "Block|Debug", meta = (ClampMin = "1.0"))
	float PointVisualizationExtent = 80.0f;

private:
	// ---------------------- Generate block layout --------------
	
	void ClearGeneratedLayout();
	
	// Simple wrapper of [LoadBlockLayoutFromJson]
	// TODO: Other logic may be added future.
	void GenerateBlockLayout();
	
	bool LoadBlockLayoutFromJson(FString& OutError);
	
	// ---------------- Building related functions -----------------------
	void RegenerateBuildingsFromBlock();
	
	void GenerateBuildingPlans();
	void PushPlansToBuildingGenerator();
	
	// Generate Building plan for one lot
	bool GenerateBuildingPlanForLot(int32 LotIndex, FBuildingPlan& OutPlan) const;
	
	bool GetBuildingFeatureForLot(
			const FLotDefinition& Lot, 
			FBlockBuildingFeature& OutFeature
			) const;

	static bool DoesBuildingArchetypeFitBound(
		const UBuildingArchetypeDataAsset* Archetype,
		const FBlockBuildingFeature& BuildingFeature
	);
	
	// Select "Building" Archetypes for all lot by calling [PickArchetypeForLot]
	void ResolveBuildingArchetypesForLots();
	
	// Pick "Building" Archetype for one lot. 
	// Perhaps this function should be renamed in the future.
	UBuildingArchetypeDataAsset* PickArchetypeForLot(
		const FLotDefinition& Lot,
		int32 LotIndex
	) const;
	
	// Find "Building" Archetypes asset according to [ArchetypeId]
	UBuildingArchetypeDataAsset* FindBuildingArchetypeById(FName ArchetypeId) const;
	
	// Check or spawn the shared [ABuildingGeneratorActor] object
	ABuildingGeneratorActor* EnsureBuildingGeneratorActor();
	
	// --------------------- Env related functions -----------------------
	
	// Check or spawn the [ABlockEnvironmentActor] object
	ABlockEnvironmentActor* EnsureBlockEnvironmentActor();
	
	void PushEnvironmentPlanToActor();
	void RegenerateEnvironmentFromBlock();
	
	// ----------------------- Helper functions -----------------------------
	static FVector GetBox2DCenter3D(const FBox2D& Box, const float Z = 0.0f);
	static FVector GetBox2DExtent3D(const FBox2D& Box, const float ZExtent);
	static int32 GetLotStreetEdgeCount(const FLotDefinition& Lot);
	
	// Get the required building rotation in block local according to [Lot.FrontageSide]
	// The building front side in its local space is assumed to be South (Y-)
	static FRotator GetBuildingLocalRotationFromFrontage(const ELotFrontageSide FrontageSide);

#if WITH_EDITOR
private:
	void BuildDebugPathVisualization();
	void BuildDebugSurfaceAreaVisualization();
	void BuildDebugPlacementPointVisualization();
	void BuildDebugLotVisualization();
	void BuildDebugPathJunctionVisualization();
	void BuildDebugBuildingBoundsVisualization();
	
	// Build editor-only box
	UBoxComponent* CreateVisualizationBox(
		const FName Name,
		const FVector& RelativeLocation,
		const FVector& BoxExtent,
		const FColor& Color
	);
	
	// Build editor-only tex
	UTextRenderComponent* CreateVisualizationText(
		const FName Name,
		const FVector& RelativeLocation,
		const FString& Text,
		const FColor& Color
	);
#endif
	
#if WITH_EDITORONLY_DATA
private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UBoxComponent>> VisualizationBoxes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextRenderComponent>> VisualizationTexts;
#endif

};
