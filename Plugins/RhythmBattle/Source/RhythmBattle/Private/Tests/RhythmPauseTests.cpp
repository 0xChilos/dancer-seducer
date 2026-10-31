#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "RhythmBattleComponent.h"
#include "RhythmBattleConfig.h"
#include "RhythmChart.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRhythmPauseReasonsTest, "RhythmBattle.Runtime.PauseReasons", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRhythmPauseReasonsTest::RunTest(const FString&)
{
    double Now = 100.;
    auto* Battle = NewObject<URhythmBattleComponent>();
    auto* Chart = NewObject<URhythmChart>();
    auto* Config = NewObject<URhythmBattleConfig>();
    Chart->Lanes.AddDefaulted();
    FRhythmNoteDefinition Note; Note.Beat = 4.; Chart->Notes.Add(Note);
    Battle->SetTimeProvider([&Now]() { return Now; });
    TestTrue(TEXT("start"), Battle->StartBattle(Chart,Config));
    Now += 1.; Battle->PauseRhythm(TEXT("Animation")); Battle->PauseRhythm(TEXT("Animation")); Battle->PauseRhythm(TEXT("Menu"));
    Now += 40.;
    TestEqual(TEXT("pause freezes wall time"), Battle->GetSongTimeSeconds(), 1.);
    TestEqual(TEXT("paused input rejected"), Battle->SubmitLaneInput(0,0).Judgment, ERhythmJudgment::None);
    Battle->ResumeRhythm(TEXT("Animation"));
    TestEqual(TEXT("other reason holds pause"),Battle->GetBattleState(),ERhythmBattleState::Paused);
    Battle->ResumeRhythm(TEXT("Menu")); Now += 1.;
    TestEqual(TEXT("resume retains target"),Battle->SubmitLaneInput(0,0).Judgment,ERhythmJudgment::Perfect);
    TestEqual(TEXT("duplicate not scored"),Battle->SubmitLaneInput(0,0).Judgment,ERhythmJudgment::None);
    TestEqual(TEXT("single score"),Battle->GetScoreSnapshot()[0].Score,int64(100));
    Battle->StopBattle();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRhythmSpeedContinuityTest, "RhythmBattle.Runtime.SpeedContinuity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRhythmSpeedContinuityTest::RunTest(const FString&)
{
    double Now = 0.;
    auto* Battle = NewObject<URhythmBattleComponent>(); auto* Chart = NewObject<URhythmChart>(); auto* Config = NewObject<URhythmBattleConfig>();
    Chart->Lanes.AddDefaulted(); FRhythmNoteDefinition Note; Note.Beat = 4.; Chart->Notes.Add(Note);
    Battle->SetTimeProvider([&Now]() { return Now; }); Battle->StartBattle(Chart,Config);
    Now = 1.; Battle->AdvanceForTesting();
    const float Before = Battle->GetNoteSnapshot()[0].Progress;
    Battle->SetVisualSpeed(2.f); Battle->AdvanceForTesting();
    TestEqual(TEXT("speed change cannot teleport"),Battle->GetNoteSnapshot()[0].Progress,Before);
    Now = 1.5; Battle->AdvanceForTesting();
    TestTrue(TEXT("progress continues smoothly"),Battle->GetNoteSnapshot()[0].Progress > Before);
    TestEqual(TEXT("target time immutable"),Battle->GetNoteSnapshot()[0].HitTimeSeconds,2.);
    Now = 2.; TestEqual(TEXT("hit at original target"),Battle->SubmitLaneInput(0,0).Judgment,ERhythmJudgment::Perfect);
    Battle->StopBattle();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRhythmLatencyExpiryTest, "RhythmBattle.Runtime.CompensatedInputExpiry", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRhythmLatencyExpiryTest::RunTest(const FString&)
{
    double Now = 0.;
    auto* Battle = NewObject<URhythmBattleComponent>(); auto* Chart = NewObject<URhythmChart>(); auto* Config = NewObject<URhythmBattleConfig>();
    Chart->Lanes.AddDefaulted(); FRhythmNoteDefinition Note; Note.Beat = 4.; Chart->Notes.Add(Note);
    Config->InputLatencyCompensation = .2;
    Battle->SetTimeProvider([&Now]() { return Now; }); Battle->StartBattle(Chart,Config);
    Now = 2.2;
    TestEqual(TEXT("compensated input survives raw expiry"),Battle->SubmitLaneInput(0,0).Judgment,ERhythmJudgment::Perfect);
    Battle->StopBattle(); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRhythmFrameRateTest, "RhythmBattle.Runtime.FrameRateIndependence", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRhythmFrameRateTest::RunTest(const FString&)
{
    auto* Chart = NewObject<URhythmChart>(); auto* Config = NewObject<URhythmBattleConfig>();
    Chart->Lanes.AddDefaulted(); FRhythmNoteDefinition Note; Note.Beat = 4.; Chart->Notes.Add(Note);
    float Progress[2] = {}; int64 Scores[2] = {};
    for (int32 Run = 0; Run < 2; ++Run)
    {
        double Now = 0.; auto* Battle = NewObject<URhythmBattleComponent>();
        Battle->SetTimeProvider([&Now]() { return Now; }); Battle->StartBattle(Chart,Config);
        const int32 Fps = Run == 0 ? 60 : 120;
        for (int32 Frame = 1; Frame <= Fps*3/2; ++Frame)
        {
            Now = static_cast<double>(Frame)/Fps;
            if (Frame == Fps) Battle->SetVisualSpeed(1.35f);
            Battle->AdvanceForTesting();
        }
        Progress[Run] = Battle->GetNoteSnapshot()[0].Progress;
        Now = 2.04; TestEqual(TEXT("same input decision"),Battle->SubmitLaneInput(0,0).Judgment,ERhythmJudgment::Perfect);
        Scores[Run] = Battle->GetScoreSnapshot()[0].Score;
        Battle->StopBattle();
    }
    TestTrue(TEXT("60/120 progress within float integration tolerance"),FMath::IsNearlyEqual(Progress[0],Progress[1],.005f));
    TestEqual(TEXT("60/120 identical scores"),Scores[0],Scores[1]);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRhythmSuppressionTest, "RhythmBattle.Runtime.StopSpawningAndLaneRestore", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRhythmSuppressionTest::RunTest(const FString&)
{
    double Now = 0.; auto* Battle = NewObject<URhythmBattleComponent>(); auto* Chart = NewObject<URhythmChart>(); auto* Config = NewObject<URhythmBattleConfig>();
    Chart->Lanes.AddDefaulted();
    for (const double Beat : {4.,8.,12.}) { FRhythmNoteDefinition N; N.Beat = Beat; Chart->Notes.Add(N); }
    Battle->SetTimeProvider([&Now]() { return Now; }); Battle->StartBattle(Chart,Config);
    Battle->StopSpawning();
    Now = 1.; Battle->AdvanceForTesting();
    TestTrue(TEXT("existing visible note keeps moving"),Battle->GetNoteSnapshot()[0].Progress > 0.f);
    Now = 2.; TestEqual(TEXT("existing note remains hittable"),Battle->SubmitLaneInput(0,0).Judgment,ERhythmJudgment::Perfect);
    TestEqual(TEXT("future visual suppressed without miss"),Battle->GetNoteSnapshot()[1].Judgment,ERhythmJudgment::Skipped);
    Battle->StopSpawning(false);
    Battle->EnableLane(0,0,false);
    Now = 5.9; Battle->AdvanceForTesting();
    TestEqual(TEXT("disabled target skipped"),Battle->GetNoteSnapshot()[2].Judgment,ERhythmJudgment::Skipped);
    Battle->EnableLane(0,0,true);
    TestTrue(TEXT("lane restored in snapshot"),Battle->GetLaneSnapshot()[0].bEnabled);
    TestEqual(TEXT("skip preserves score"),Battle->GetScoreSnapshot()[0].Score,int64(100));
    TestEqual(TEXT("skip preserves combo"),Battle->GetScoreSnapshot()[0].Combo,1);
    Battle->StopBattle(); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRhythmTwelveLaneTest, "RhythmBattle.Runtime.TwelveLanesHundredNotes", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRhythmTwelveLaneTest::RunTest(const FString&)
{
    double Now = 0.; auto* Battle = NewObject<URhythmBattleComponent>(); auto* Chart = NewObject<URhythmChart>(); auto* Config = NewObject<URhythmBattleConfig>();
    for (int32 P = 0; P < 2; ++P) for (int32 L = 0; L < 6; ++L)
    { auto& Lane = Chart->Lanes.AddDefaulted_GetRef(); Lane.PlayerIndex = P; Lane.LaneId = L; }
    for (int32 I = 0; I < 100; ++I)
    { auto& N = Chart->Notes.AddDefaulted_GetRef(); N.PlayerIndex = (I%12)/6; N.LaneId = I%6; N.Beat = 2.+.01*I; }
    Battle->SetTimeProvider([&Now]() { return Now; }); TestTrue(TEXT("twelve lanes accepted"),Battle->StartBattle(Chart,Config));
    const auto Notes = Battle->GetNoteSnapshot();
    TestEqual(TEXT("hundred runtime notes"),Notes.Num(),100);
    int32 Visible = 0; for (const auto& N : Notes) Visible += N.bSpawned ? 1 : 0;
    TestEqual(TEXT("hundred simultaneous visible notes"),Visible,100);
    Now = 1.; TestEqual(TEXT("player zero hit"),Battle->SubmitLaneInput(0,0).Judgment,ERhythmJudgment::Perfect);
    TestEqual(TEXT("opponent untouched"),Battle->GetScoreSnapshot()[1].Score,int64(0));
    Battle->StopBattle(); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRhythmInvalidLaneTest, "RhythmBattle.Runtime.InvalidPlayer", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRhythmInvalidLaneTest::RunTest(const FString&)
{
    auto* Battle = NewObject<URhythmBattleComponent>();
    auto* Chart = NewObject<URhythmChart>();
    auto* Config = NewObject<URhythmBattleConfig>();
    Chart->Lanes.AddDefaulted_GetRef().PlayerIndex = -1;
    TestFalse(TEXT("negative player is rejected before key construction"), Battle->StartBattle(Chart, Config));
    return true;
}
#endif

