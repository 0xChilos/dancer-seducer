#include "Misc/AutomationTest.h"
#include "../DanceRhythmComponent.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDanceRhythmTimingTest, "DanceBattle.Rhythm.Timing", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDanceRhythmTimingTest::RunTest(const FString& Parameters)
{
    TStrongObjectPtr<UDanceRhythmComponent> R(NewObject<UDanceRhythmComponent>());
    FDanceSettings S;
    S.RoundDuration = 10.0f;
    R->Reset(S);

    FDanceNote N = R->Notes[0];

    struct FTimingCase {
        float Offset;
        EDanceJudgment ExpectedJudgment;
    };

    TArray<FTimingCase> Cases = {
        { -0.06f, EDanceJudgment::Perfect },
        { 0.06f,  EDanceJudgment::Perfect },
        { -0.15f, EDanceJudgment::Good },
        { 0.15f,  EDanceJudgment::Good },
        { -0.151f, EDanceJudgment::Miss },
        { 0.151f,  EDanceJudgment::Miss }
    };

    for (const auto& Case : Cases)
    {
        R->Reset(S);
        EDanceJudgment Result = R->Attempt(EDanceSide::Player, N.Direction, N.HitTime + Case.Offset, N.NoteId);
        TestTrue(FString::Printf(TEXT("Attempt Result Match: %f"), Case.Offset), Result == Case.ExpectedJudgment);
        
        if (Case.Offset <= -0.151f || Case.Offset >= 0.151f)
        {
            TestTrue("Out of window remains Pending", R->Notes[0].Judgment == EDanceJudgment::Pending);
        }
    }

    R->Reset(S);
    int32 Count = 0;
    R->OnJudged.AddLambda([&Count](const FDanceNote& Note) { Count++; });
    R->Attempt(EDanceSide::Player, N.Direction, N.HitTime, N.NoteId);
    R->Attempt(EDanceSide::Player, N.Direction, N.HitTime, N.NoteId);
    TestEqual(TEXT("Repeated note judged once"), Count, 1);
    R->OnJudged.Clear();

    R->Reset(S);
    EDanceDirection wrongDir = static_cast<EDanceDirection>((static_cast<int32>(N.Direction) + 1) % 4);
    TestTrue("Wrong direction results in Miss", R->Attempt(EDanceSide::Player, wrongDir, N.HitTime, N.NoteId) == EDanceJudgment::Miss);
    TestTrue("Note judgment updated to Miss", R->Notes[0].Judgment == EDanceJudgment::Miss);

    R->Reset(S);
    R->Advance(N.HitTime + S.GoodWindow + 0.01f);
    TestTrue("Note missed after window expires", R->Notes[0].Judgment == EDanceJudgment::Miss);

    R->Reset(S);
    TestTrue("Note is pending after reset", R->Notes[0].Judgment == EDanceJudgment::Pending);

    R->Finish();
    TestFalse("Component is not running after finish", R->bRunning);

    return true;
}
#endif
