// Fill out your copyright notice in the Description page of Project Settings.

#include "ProcCity/Environment/Generator/BlockEnvironmentActor.h"

#include "Components/SceneComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

// Sets default values
ABlockEnvironmentActor::ABlockEnvironmentActor()
{
	PrimaryActorTick.bCanEverTick = false;
	
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("EnvironmentRoot"));
	Root->SetMobility(EComponentMobility::Static);
	SetRootComponent(Root);
}

void ABlockEnvironmentActor::SetResolvedEnvironmentPlan(
	const FResolvedBlockEnvironmentPlan& InPlan)
{
	ResolvedEnvironmentPlan = InPlan;
}

void ABlockEnvironmentActor::RegenerateEnvironment()
{
	ClearGeneratedEnvironment();
	GenerateAll();
}

void ABlockEnvironmentActor::ClearGeneratedEnvironment()
{
	for (UHISMComp* Component : GeneratedHISMComponents)
	{
		if (IsValid(Component))
		{
			Component->DestroyComponent();
		}
	}
	GeneratedHISMComponents.Reset();
	HISMComponentsMap.Reset();
}

void ABlockEnvironmentActor::GenerateAll()
{
	for (const FResolvedBlockEnvironmentInstance& Instance 
		: ResolvedEnvironmentPlan.Instances)
	{
		if (!Instance.bEnabled)
		{
			continue;
		}
		
		if (!ShouldGenerateInstanceType(Instance.InstanceType))
		{
			continue;
		}
		
		AddResolvedInstance(Instance);
	}
}

bool ABlockEnvironmentActor::ShouldGenerateInstanceType(
	const EBlockEnvironmentElementType InstanceType) const
{
	switch (InstanceType)
	{
	case EBlockEnvironmentElementType::Surface:
		return bGenerateSurfaces;
		
	case EBlockEnvironmentElementType::PathSegment:
	case EBlockEnvironmentElementType::PathTerminalTreatment:
		return bGeneratePathSegments;
		
	case EBlockEnvironmentElementType::PathJunction:
		return bGeneratePathJunction;

	case EBlockEnvironmentElementType::Prop:
		return bGenerateProps;
		
	case EBlockEnvironmentElementType::None:
	default:
		break;
	}
	
	return false;
}

FEnvironmentHISMKey ABlockEnvironmentActor::BuildHISMKey(
	const FResolvedBlockEnvironmentInstance& Instance)
{
	FEnvironmentHISMKey Key;
	Key.Mesh = Instance.Mesh;
	Key.Materials = Instance.MaterialPayload.Materials;
	Key.UsageTag = Instance.UsageTag;
	Key.NumCustomDataFloats = Instance.MaterialPayload.PerInstanceCustomData.Num();
	return Key;
}

ABlockEnvironmentActor::UHISMComp* 
	ABlockEnvironmentActor::GetOrCreateHISMComponent(
		const FEnvironmentHISMKey& Key,
		const FString& DebugNamePrefix
	)
{
	if (!Key.Mesh)
	{
		return nullptr;
	}
	
	if (TObjectPtr<UHISMComp>* Existing = HISMComponentsMap.Find(Key))
	{
		if (*Existing)
		{
			return *Existing;
		}
	}
	
	const int32 ComponentIndex = GeneratedHISMComponents.Num();
	const FName ComponentName(
		*FString::Printf(
			TEXT("%s_%s_%d"), 
			*DebugNamePrefix, 
			*Key.Mesh.GetName(), 
			ComponentIndex)	
		);
	
	UHISMComp* Component = 
		NewObject<UHISMComp>(this, ComponentName);
	
	if (!Component)
	{
		return nullptr;
	}

	Component->SetupAttachment(RootComponent);
	Component->SetMobility(EComponentMobility::Static);
	Component->SetStaticMesh(Key.Mesh);
	Component->NumCustomDataFloats = Key.NumCustomDataFloats;
	
	for (int32 MaterialIdx = 0; 
		MaterialIdx < Key.Materials.Num(); ++MaterialIdx)
	{
		if (Key.Materials[MaterialIdx])
		{
			Component->SetMaterial(MaterialIdx, Key.Materials[MaterialIdx]);
		}
	}
	
	Component->RegisterComponent();
	GeneratedHISMComponents.Add(Component);
	HISMComponentsMap.Add(Key, Component);
	
	return Component;
}

void ABlockEnvironmentActor::ApplyInstanceCustomData(
	UHISMComp* Component,
	const int32 InstanceIndex,
	const TArray<float>& PerInstanceCustomData
)
{
	if (!Component)
	{
		return;
	}
	
	if (PerInstanceCustomData.Num() == 0)
	{
		return;
	}
	
	for (int32 DataIdx = 0; 
		DataIdx < PerInstanceCustomData.Num(); ++DataIdx)
	{
		Component->SetCustomDataValue(
			InstanceIndex, 
			DataIdx, 
			PerInstanceCustomData[DataIdx], 
			false
		);
	}
	
	Component->MarkRenderStateDirty();
}

bool ABlockEnvironmentActor::AddResolvedInstance(
	const FResolvedBlockEnvironmentInstance& Instance
)
{
	if (!Instance.bEnabled || !Instance.Mesh)
	{
		return false;
	}
	
	const FEnvironmentHISMKey Key = BuildHISMKey(Instance);
	
	UHISMComp* Component = GetOrCreateHISMComponent(
		Key,
		Instance.UsageTag.IsNone()  
			? FString(TEXT("HISM_Environment"))  
			: Instance.UsageTag.ToString()
	);
	
	if (!Component)
	{
		return false;
	}
	
	const int32 InstanceIndex = 
		Component->AddInstance(Instance.RelativeTransform);
	
	if (InstanceIndex == INDEX_NONE)
	{
		return false;
	}
	
	ApplyInstanceCustomData(
		Component, 
		InstanceIndex, 
		Instance.MaterialPayload.PerInstanceCustomData
	);
	
	return true;
}



































