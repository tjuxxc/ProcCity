// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProcCity/Building/Types/BuildingEnums.h"

#include "BuildingMaterialStyleDataAsset.generated.h"

/**
 * This 
 */

class UMaterialInterface;


UCLASS(BlueprintType)
class PROCCITY_API UBuildingMaterialStyleDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Style")
	FName StyleId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Style")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Style", meta = (ClampMin = "1"))
	int32 Weight = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Materials")
	TObjectPtr<UMaterialInterface> WallMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Materials")
	TObjectPtr<UMaterialInterface> WindowFrameMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Materials")
	TObjectPtr<UMaterialInterface> GlassMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Materials")
	TObjectPtr<UMaterialInterface> WallBaseMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Materials")
	TObjectPtr<UMaterialInterface> DoorFrameMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Materials")
	TObjectPtr<UMaterialInterface> RoofMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Materials")
	TObjectPtr<UMaterialInterface> ParapetMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Materials")
	TObjectPtr<UMaterialInterface> AccentMaterial = nullptr;

public:
	UFUNCTION(BlueprintPure, Category = "Style")
	UMaterialInterface* GetMaterialByRole(EBuildingMaterialRole Role) const
	{
		switch (Role)
		{
		case EBuildingMaterialRole::Wall:         return WallMaterial;
		case EBuildingMaterialRole::WindowFrame:  return WindowFrameMaterial;
		case EBuildingMaterialRole::Glass:        return GlassMaterial;
		case EBuildingMaterialRole::WallBase:     return WallBaseMaterial;
		case EBuildingMaterialRole::DoorFrame:    return DoorFrameMaterial;
		case EBuildingMaterialRole::Roof:         return RoofMaterial;
		case EBuildingMaterialRole::Parapet:      return ParapetMaterial;
		case EBuildingMaterialRole::Accent:       return AccentMaterial;
		default:                                  return nullptr;
		}
	}

};
