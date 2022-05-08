// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MonteCarloATT2 : ModuleRules
{
	public MonteCarloATT2(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });
	}
}
