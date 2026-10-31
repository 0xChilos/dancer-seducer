using UnrealBuildTool;
using System.Collections.Generic;

public class dancerTarget : TargetRules
{
	public dancerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("dancer");
	}
}