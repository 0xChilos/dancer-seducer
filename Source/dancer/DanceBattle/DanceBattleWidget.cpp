#include "DanceBattleWidget.h"
#include "DanceBattleGameMode.h"
#include "DanceBattleGameState.h"
#include "DanceRhythmComponent.h"


void UDanceBattleWidget::InitializeBattle(ADanceBattleGameMode* InBattle)
{
    if (!InBattle)
    { 
        return;
    }

    // Unbind previous delegates to prevent leaks/multiple calls
    // Note: In a real scenario, we'd need access to the specific delegate handles if they aren't multicast
    // But per instructions: RemoveAll(this) on the state/rhythm delegates
    
    if (Battle)
    {
        if (ADanceBattleGameState* State = Battle->GetGameState<ADanceBattleGameState>())
        {
            State->OnChanged.RemoveAll(this);
        }

        if (Battle->Rhythm)
        {
            Battle->Rhythm->OnAttempt.RemoveAll(this);
            Battle->Rhythm->OnJudged.RemoveAll(this);
        }
    }

    Battle = InBattle;
    ADanceBattleGameState* State = Battle->GetGameState<ADanceBattleGameState>();

    if (State)
    {
        State->OnChanged.AddUObject(this, &UDanceBattleWidget::RefreshState);
    }

    if (Battle->Rhythm)
    {
        Battle->Rhythm->OnAttempt.AddUObject(this, &UDanceBattleWidget::OnAttempt);
        Battle->Rhythm->OnJudged.AddUObject(this, &UDanceBattleWidget::OnJudged);
    }

    SetVisibility(ESlateVisibility::HitTestInvisible);
    RefreshState();
}

void UDanceBattleWidget::RefreshState()
{
    if (!Battle) return;
    ADanceBattleGameState* State = Battle->GetGameState<ADanceBattleGameState>();
    if (!State) return;

    // Cache Scores
    PlayerText = FString::FromInt(State->PlayerScore);
    AIText = FString::FromInt(State->AIScore);

    // Cache Timer (Ceil to avoid 0.999s showing as 0)
    TimerText = FString::FromInt(FMath::CeilToInt(State->RemainingTime));

    // Cache Result
    ResultText = (State->Phase == EDancePhase::Finished) ? State->Result : TEXT("");

    // Reset feedback if round restarted
    // Logic: Playing AND scores are 0 AND timer is at max duration
    if (State->Phase == EDancePhase::Playing && State->PlayerScore == 0 && State->AIScore == 0 && FMath::IsNearlyEqual(State->RemainingTime, Battle->Rhythm->Settings.RoundDuration))
    {
        PlayerFeedback = TEXT("");
        AIFeedback = TEXT("");
        PlayerFeedbackTime = -10.0f;
        AIFeedbackTime = -10.0f;
    }
}

void UDanceBattleWidget::OnAttempt(EDanceSide Side, EDanceDirection Direction, EDanceJudgment Judgment)
{
    if (!Battle) return;

    FString FeedbackStr;
    float CurrentTime = Battle->GetElapsedTime();

    switch (Judgment)
    {
        case EDanceJudgment::Perfect: FeedbackStr = TEXT("PERFECT"); break;
        case EDanceJudgment::Good:    FeedbackStr = TEXT("GOOD");    break;
        case EDanceJudgment::Miss:    FeedbackStr = TEXT("MISS");    break;
        default:                      FeedbackStr = TEXT("");        break;
    }

    if (Side == EDanceSide::Player)
    {
        PlayerFeedback = FeedbackStr;
        PlayerFeedbackTime = CurrentTime;
    }
    else
    {
        AIFeedback = FeedbackStr;
        AIFeedbackTime = CurrentTime;
    }
}

void UDanceBattleWidget::OnJudged(const FDanceNote& Note)
{
    // If the note was a miss, trigger the attempt logic to update feedback text/timestamp
    if (Note.Judgment == EDanceJudgment::Miss)
    {
        OnAttempt(Note.Side, Note.Direction, Note.Judgment);
    }
}

void UDanceBattleWidget::NativeDestruct()
{
    if (Battle)
    {
        if (ADanceBattleGameState* State = Battle->GetGameState<ADanceBattleGameState>())
        {
            State->OnChanged.RemoveAll(this);
        }

        if (Battle->Rhythm)
        {
            Battle->Rhythm->OnAttempt.RemoveAll(this);
            Battle->Rhythm->OnJudged.RemoveAll(this);
        }
    }

    Super::NativeDestruct();
}

