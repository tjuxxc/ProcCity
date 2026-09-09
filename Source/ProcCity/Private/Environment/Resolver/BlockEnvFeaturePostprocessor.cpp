#include "ProcCity/Environment/Resolver/BlockEnvFeaturePostprocessor.h"

#include "ProcCity/Block/Types/BlockLayoutTypes.h"
#include "ProcCity/Environment/Types/BlockEnvironmentFeatureTypes.h"
#include "ProcCity/Common/Math/RandomUtils.h"


namespace
{
	static constexpr int32 PointRandomYawSeedSalt = 101;
}


int32 FBlockEnvFeaturePostprocessSettings::GetEffectivePointRandomYawSeed() const
{
	if (bOverridePointRandomYawSeed)
	{
		return PointRandomYawSeed;
	}

	return ProcCity::DeriveSeed(MasterSeed, PointRandomYawSeedSalt);
}

bool FBlockEnvFeaturePostprocessor::PostProcess(
	const FGeneratedBlockLayout& InLayout,
	FGeneratedBlockLayout& OutLayout
) const
{
	OutLayout = InLayout;
	
	if (!Settings.bEnabled)
	{
		return true;
	}
	
	PostProcessPointFeatures(OutLayout);
	return true;
}

void FBlockEnvFeaturePostprocessor::PostProcessPointFeatures(
	FGeneratedBlockLayout& InOutLayout
) const
{
	if (!Settings.bEnablePointRandomYaw)
	{
		return;
	}
	
	ApplyPointFeatureRandomYaw(
		InOutLayout,
		Settings.GetEffectivePointRandomYawSeed()
	);
}

void FBlockEnvFeaturePostprocessor::ApplyPointFeatureRandomYaw(
	FGeneratedBlockLayout& InOutLayout,
	const int32 Seed
)
{
	for (int32 PointIdx = 0; PointIdx < InOutLayout.PointFeatures.Num(); 
		++PointIdx)
	{
		FBlockPointFeature& Point = InOutLayout.PointFeatures[PointIdx];
		
		if (Point.RandomYawMax <= Point.RandomYawMin)
		{
			continue;
		}
		
		const int32 PerPointSeed = 
			ProcCity::DeriveSeed(Seed, PointIdx);
		FRandomStream RotationRandom(PerPointSeed);
		
		const float RandomYaw = RotationRandom.FRandRange(
			Point.RandomYawMin,
			Point.RandomYawMax
		);
		
		Point.LocalRotation.Yaw += RandomYaw;
	}
}
