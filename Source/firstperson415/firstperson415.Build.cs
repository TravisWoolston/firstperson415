// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class firstperson415 : ModuleRules
{
    public firstperson415(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "EnhancedInput",
            "AIModule",
            "StateTreeModule",
            "GameplayStateTreeModule",
            "UMG",
            "Slate",
            "Niagara",
            "ProceduralMeshComponent"
        });

        PrivateDependencyModuleNames.AddRange(new string[] { });

        PublicIncludePaths.AddRange(new string[] {
            "firstperson415",
            "firstperson415/Variant_Horror",
            "firstperson415/Variant_Horror/UI",
            "firstperson415/Variant_Shooter",
            "firstperson415/Variant_Shooter/AI",
            "firstperson415/Variant_Shooter/UI",
            "firstperson415/Variant_Shooter/Weapons"
        });

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
