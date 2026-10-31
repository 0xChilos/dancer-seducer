#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "RhythmModifierManager.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRhythmScopedModifiersTest, "RhythmBattle.Modifiers.ScopeStackExpiry", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRhythmScopedModifiersTest::RunTest(const FString&)
{
    auto* M = NewObject<URhythmModifierManager>();
    FRhythmModifierSpec S; S.Id = TEXT("Global"); S.Value = 2.f; S.DurationSeconds = 4.; S.bDurationInBeats = true;
    TestTrue(TEXT("add global"),M->Add(S,0.,.5));
    S.Id = TEXT("Player"); S.Scope = ERhythmModifierScope::Player; S.PlayerIndex = 1; S.Value = 3.f; S.DurationSeconds = 8.; S.bDurationInBeats = false; M->Add(S,0.,.5);
    TestEqual(TEXT("global only on player zero"),M->Evaluate(ERhythmModifierType::Score,0,0),2.f);
    TestEqual(TEXT("multiplicative player scope"),M->Evaluate(ERhythmModifierType::Score,1,0),6.f);
    S.Id = TEXT("Lane"); S.Scope = ERhythmModifierScope::Lane; S.Type = ERhythmModifierType::LaneDisable; S.PlayerIndex = 1; S.LaneId = 2; M->Add(S,0.,.5);
    TestEqual(TEXT("lane disabled"),M->Evaluate(ERhythmModifierType::LaneDisable,1,2),1.f);
    TestEqual(TEXT("other lane enabled"),M->Evaluate(ERhythmModifierType::LaneDisable,1,1),0.f);
    M->Expire(2.);
    TestEqual(TEXT("beats expire at precise boundary"),M->Evaluate(ERhythmModifierType::Score,1,0),3.f);
    M->RemoveModifier(TEXT("Player"));
    TestEqual(TEXT("remove restores baseline"),M->Evaluate(ERhythmModifierType::Score,1,0),1.f);
    M->Expire(8.);
    TestEqual(TEXT("disable expiry restores lane"),M->Evaluate(ERhythmModifierType::LaneDisable,1,2),0.f);
    S.Id = NAME_None; TestFalse(TEXT("anonymous modifier rejected"),M->Add(S,0.,.5));
    return true;
}
#endif
