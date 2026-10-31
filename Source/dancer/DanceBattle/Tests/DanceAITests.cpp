#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"
#include "../DanceRhythmComponent.h"
#include "../DanceAIOpponentComponent.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDanceAITest_Autonomous, "DanceBattle.AI.Autonomous", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDanceAITest_Autonomous::RunTest(const FString& Parameters)
{
    auto Simulate = [&](float Step) -> TArray<EDanceJudgment>
    {
        TStrongObjectPtr<UDanceRhythmComponent> R(NewObject<UDanceRhythmComponent>());
        TStrongObjectPtr<UDanceAIOpponentComponent> AI(NewObject<UDanceAIOpponentComponent>());

        R->Reset(FDanceSettings());
        AI->Reset(R.Get());

        int32 Iterations = FMath::CeilToInt(60.0f / Step);
        for (int32 i = 0; i <= Iterations; ++i)
        {
            float Time = FMath::Min(i * Step, 60.0f);
            AI->Advance(Time);
            R->Advance(Time);
        }

        R->Finish();
        AI->Cancel();

        TArray<EDanceJudgment> Results;
        for (const FDanceNote& Note : R->Notes) { if (Note.Side == EDanceSide::AI) Results.Add(Note.Judgment); }
        return Results;
    };

    // First Check: Step consistency and validity
    TArray<EDanceJudgment> Results1 = Simulate(1.0f / 60.0f);
    TArray<EDanceJudgment> Results2 = Simulate(0.37f);

    TestTrue("Chart should not be empty", Results1.Num() > 0);
    TestTrue("Results identical", Results1 == Results2);

    int32 Perfects = 0, Goods = 0, Misses = 0;
    for (EDanceJudgment J : Results1)
    {
        if (J == EDanceJudgment::Perfect) Perfects++;
        else if (J == EDanceJudgment::Good) Goods++;
        else if (J == EDanceJudgment::Miss) Misses++;
    }

    TestTrue("Must have Perfects", Perfects > 0);
    TestTrue("Must have Goods", Goods > 0);
    TestTrue("Must have Misses", Misses > 0);

    // Second Check: Stale generation safety
    TStrongObjectPtr<UDanceRhythmComponent> R2(NewObject<UDanceRhythmComponent>());
    TStrongObjectPtr<UDanceAIOpponentComponent> AI2(NewObject<UDanceAIOpponentComponent>());
    
    R2->Reset(FDanceSettings());
    AI2->Reset(R2.Get());
    int32 OldGen = R2->Generation;

    R2->Reset(FDanceSettings());
    // AI is NOT reset here, it holds a reference to the old rhythm state/generation
    AI2->Advance(60.0f);

    bool bAllPending = true;
    for (const FDanceNote& Note : R2->Notes)
    {
        if (Note.Judgment != EDanceJudgment::Pending)
        {
            bAllPending = false;
            break;
        }
    }
    TestTrue("AI notes must remain Pending when rhythm resets without AI reset", bAllPending);

    AI2->Cancel();
    AI2->Advance(60.0f); // Should do nothing safely

    R2->Reset(FDanceSettings());
    AI2->Reset(R2.Get());
    AI2->Advance(60.0f);

    TestTrue("New generation should be dispatched after proper reset", R2->Generation > OldGen);

    return true;
}

#endif
