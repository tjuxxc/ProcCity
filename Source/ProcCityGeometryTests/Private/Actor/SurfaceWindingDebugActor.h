#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "SurfaceWindingDebugActor.generated.h"

class UMaterialInterface;
class UProceduralMeshComponent;

/**
 * Temporary in-level visual verification for procedural surface triangle winding.
 *
 * Use a one-sided material:
 * - the surface at Z = 0 should be visible from above;
 * - the surface at Z = CeilingHeight should be visible from below.
 */

UCLASS()
class PROCCITYGEOMETRYTESTS_API ASurfaceWindingDebugActor : public AActor
{
	GENERATED_BODY()

public:
	ASurfaceWindingDebugActor();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UProceduralMeshComponent> PMC;

	/**
	 * Assign a material with Two Sided disabled.
	 * This is required to visually validate back-face culling/winding.
	 */
	UPROPERTY(EditAnywhere, Category = "Winding Debug")
	TObjectPtr<UMaterialInterface> DebugMaterialGround;
	
	UPROPERTY(EditAnywhere, Category = "Winding Debug")
	TObjectPtr<UMaterialInterface> DebugMaterialCeiling;
	
	UPROPERTY(EditAnywhere, Category = "Winding Debug", meta = (ClampMin = "1.0"))
	double CeilingHeight = 300.0;
};