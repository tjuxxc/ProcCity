// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ProcCityGeometryTests : ModuleRules
{
    public ProcCityGeometryTests(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "ProcCityGeometry", "GeometryCore"
        });
    }
}

