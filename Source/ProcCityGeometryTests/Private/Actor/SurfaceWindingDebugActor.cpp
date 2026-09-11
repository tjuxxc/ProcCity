#include "SurfaceWindingDebugActor.h"
#include "ProceduralMeshComponent.h"

#include "ProcCityGeometry//Polygon2D.h"
#include "ProcCityGeometry/PolygonMeshAdapter.h"

ASurfaceWindingDebugActor::ASurfaceWindingDebugActor()
{
	PrimaryActorTick.bCanEverTick = false;

	PMC = CreateDefaultSubobject<UProceduralMeshComponent>(
		TEXT("ProceduralMeshComponent"));

	SetRootComponent(PMC);
}

void ASurfaceWindingDebugActor::BeginPlay()
{
	Super::BeginPlay();
	
	// Keep coordinates actor-local, so placing the Actor in 
	// the level is intuitive.
	const FPolygon2D Poly = FPolygon2D::MakeRect(
		FVector2D::ZeroVector,
		FVector2D(500.0, 300.0),
		25.0);
	
	// Section 0: expected to be visible ONLY from above.
	ProcCityGeometry::FSurfaceMeshData Ground;
	if (!BuildSurfaceMesh(Poly, 100.0, 
		ProcCityGeometry::ESurfaceFacing::Up, 100.0, Ground))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to build Up winding-debug surface."));
		return;
	}
	
	// Section 1: expected to be visible ONLY from below.
	ProcCityGeometry::FSurfaceMeshData Ceiling;
	if (!BuildSurfaceMesh(Poly, 500.0, 
		ProcCityGeometry::ESurfaceFacing::Down, 100.0, Ceiling))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to build Down winding-debug surface."));
		return;
	}
	
	// This overload accepts FVector2D UVs. Empty UVs are sufficient for winding checks.
	const TArray<FVector2D> EmptyUVs;
	const TArray<FLinearColor> EmptyColors;
	const TArray<FProcMeshTangent> EmptyTangents;
	
	PMC->CreateMeshSection_LinearColor(
		0, 
		Ground.Positions,
		Ground.Indices,
		Ground.Normals,
		EmptyUVs,
		EmptyColors,
		EmptyTangents,
		false);
	
	PMC->CreateMeshSection_LinearColor(
		1,
		Ceiling.Positions,
		Ceiling.Indices,
		Ceiling.Normals,
		EmptyUVs,
		EmptyColors,
		EmptyTangents,
		false);
	
	if (DebugMaterialGround != nullptr || DebugMaterialCeiling != nullptr)
	{
		PMC->SetMaterial(0, DebugMaterialGround);
		PMC->SetMaterial(1, DebugMaterialCeiling);
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("No DebugMaterial assigned. Use a material with Two Sided disabled "
				 "before relying on this actor for winding validation."));
	}
	
	UE_LOG(LogTemp, Display,
		TEXT("Created winding test: Up at Z=0; Down at Z=%f."),
		CeilingHeight);
}