using UnrealBuildTool;

public class RhythmBattle : ModuleRules
{
    public RhythmBattle(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.NoPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "UMG", "SlateCore", "EnhancedInput", "GameplayTags" });
        PrivateDependencyModuleNames.AddRange(new[] { "Slate", "AudioMixer", "InputCore" });
    }
}
