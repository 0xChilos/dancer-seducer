#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "RhythmBattleConfig.h"
#include "RhythmChart.h"
#include "RhythmClock.h"
#include "RhythmHitJudge.h"
#include "RhythmNoteScheduler.h"
#include "RhythmScoreCalculator.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRhythmJudgeBoundaryTest, "RhythmBattle.Core.JudgmentAndScore", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRhythmJudgeBoundaryTest::RunTest(const FString& Parameters)
{
    const auto* Config = GetDefault<URhythmBattleConfig>();
    for (double Sign : { -1., 1. })
    {
        TestEqual(TEXT("45 ms inclusive"), FRhythmHitJudge::Judge(Sign * .045, *Config), ERhythmJudgment::Perfect);
        TestEqual(TEXT("Beyond 45 ms"), FRhythmHitJudge::Judge(Sign * .04501, *Config), ERhythmJudgment::Good);
        TestEqual(TEXT("100 ms inclusive"), FRhythmHitJudge::Judge(Sign * .100, *Config), ERhythmJudgment::Good);
        TestEqual(TEXT("Beyond 100 ms"), FRhythmHitJudge::Judge(Sign * .10001, *Config), ERhythmJudgment::OK);
        TestEqual(TEXT("160 ms inclusive"), FRhythmHitJudge::Judge(Sign * .160, *Config), ERhythmJudgment::OK);
        TestEqual(TEXT("Beyond 160 ms"), FRhythmHitJudge::Judge(Sign * .16001, *Config), ERhythmJudgment::Miss);
    }
    TArray<FRhythmRuntimeNote> Notes;
    for (int32 Id = 0; Id < 2; ++Id)
    {
        auto& Note = Notes.AddDefaulted_GetRef(); Note.NoteId = Id; Note.HitTimeSeconds = 1. + Id * .1;
    }
    int32 Index = FRhythmHitJudge::FindClosestNote(Notes, 0, 0, 1.09, .16);
    TestEqual(TEXT("Closest candidate"), Index, 1);
    auto Hit = FRhythmHitJudge::Resolve(Notes, Index, 1.09, *Config);
    TestTrue(TEXT("Early error signed"), Hit.TimingErrorSeconds < 0.);
    TestEqual(TEXT("Duplicate cannot resolve"), FRhythmHitJudge::Resolve(Notes, Index, 1.09, *Config).Judgment, ERhythmJudgment::None);
    TestEqual(TEXT("Other player isolated"), FRhythmHitJudge::FindClosestNote(Notes, 1, 0, 1., .16), INDEX_NONE);
    FRhythmPlayerScore Score;
    FRhythmScoreCalculator::Apply(Score, Hit, *Config);
    TestEqual(TEXT("Perfect score"), Score.Score, int64(100));
    TestEqual(TEXT("Combo increment"), Score.Combo, 1);
    Hit.Judgment = ERhythmJudgment::Miss;
    FRhythmScoreCalculator::Apply(Score, Hit, *Config);
    TestEqual(TEXT("Miss resets combo"), Score.Combo, 0);
    TestEqual(TEXT("Max combo retained"), Score.MaxCombo, 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRhythmClockSchedulerTest, "RhythmBattle.Core.ClockAndScheduler", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRhythmClockSchedulerTest::RunTest(const FString& Parameters)
{
    double Now = 100.;
    FRhythmClock Clock([&Now] { return Now; });
    Clock.Start(); Now += 1.;
    TestEqual(TEXT("Monotonic elapsed time"), Clock.GetSongTimeSeconds(), 1.);
    Clock.Pause(); Clock.Pause(); Now += 25.;
    TestEqual(TEXT("Pause freezes song time"), Clock.GetSongTimeSeconds(), 1.);
    Clock.Resume(); Clock.Resume(); Now += .5;
    TestEqual(TEXT("Resume removes paused duration"), Clock.GetSongTimeSeconds(), 1.5);
    TestEqual(TEXT("Input timestamp alignment"), Clock.MapTimestampToSongTime(Now - .1), 1.4, 1.e-8);
    auto* Chart = NewObject<URhythmChart>();
    for (int32 Id = 0; Id < 4; ++Id) { auto& Note = Chart->Notes.AddDefaulted_GetRef(); Note.Beat = Id; }
    FRhythmNoteScheduler Scheduler;
    Scheduler.Initialize(*Chart);
    TestEqual(TEXT("120 BPM interval"), Scheduler.GetNotes()[3].HitTimeSeconds, 1.5);
    Scheduler.Update(0., 1.);
    TestTrue(TEXT("Within lookahead spawned"), Scheduler.GetNotes()[2].bSpawned);
    TestFalse(TEXT("Outside lookahead unspawned"), Scheduler.GetNotes()[3].bSpawned);
    TestEqual(TEXT("No miss at inclusive boundary"), Scheduler.ExpireMisses(.16, .16).Num(), 0);
    TestEqual(TEXT("Miss expires once"), Scheduler.ExpireMisses(.161, .16).Num(), 1);
    TestEqual(TEXT("Miss cannot expire twice"), Scheduler.ExpireMisses(.161, .16).Num(), 0);
    Scheduler.SkipLane(0, 0, .5, .16);
    TestEqual(TEXT("Disabled lane skips upcoming target"), Scheduler.GetNotes()[1].Judgment, ERhythmJudgment::Skipped);
    return true;
}
#endif
