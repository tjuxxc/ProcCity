// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ProcCityGeometry : ModuleRules
{
    public ProcCityGeometry(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",     
            "GeometryCore",    
            "GeometryAlgorithms", 
        });

        PrivateDependencyModuleNames.AddRange(new string[] { });

        // OptimizeCode = CodeOptimization.Always;  
    }
}