// Copyright (c) 2025 Maurel Sagbo

using UnrealBuildTool;

public class Portal : ModuleRules
{
	public Portal(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "PhysicsCore", "ProceduralMeshComponent" });
	}
}
