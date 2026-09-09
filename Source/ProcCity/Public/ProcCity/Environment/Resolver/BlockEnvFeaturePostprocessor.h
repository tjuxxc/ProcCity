#pragma once

#include "CoreMinimal.h"

struct FGeneratedBlockLayout;

struct PROCCITY_API FBlockEnvFeaturePostprocessSettings
{
public:
	// Master switch
	bool bEnabled = true;

	// Global deterministic seed used to derive per-feature-category seeds.
	int32 MasterSeed = 12345;

	// ---------------- Point / Prop-like feature options ----------------

	// Whether to apply random yaw for point features when requested by 
	// feature data.
	bool bEnablePointRandomYaw = false;

	// If true, use PointRandomYawSeed directly.
	// Otherwise, derive it from MasterSeed.
	bool bOverridePointRandomYawSeed = false;

	int32 PointRandomYawSeed = 0;

public:
	int32 GetEffectivePointRandomYawSeed() const;
};


struct PROCCITY_API FBlockEnvFeaturePostprocessor
{
public:
	explicit FBlockEnvFeaturePostprocessor(
		const FBlockEnvFeaturePostprocessSettings& InSettings)
		: Settings(InSettings)
	{
	}

	bool PostProcess(
		const FGeneratedBlockLayout& InLayout,
		FGeneratedBlockLayout& OutLayout
	) const;

private:
	void PostProcessPointFeatures(
		FGeneratedBlockLayout& InOutLayout
	) const;

	static void ApplyPointFeatureRandomYaw(
		FGeneratedBlockLayout& InOutLayout,
		int32 Seed
	);

private:
	FBlockEnvFeaturePostprocessSettings Settings;
};