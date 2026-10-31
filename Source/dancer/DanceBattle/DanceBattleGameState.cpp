#include "DanceBattleGameState.h"
#include "Math/UnrealMathUtility.h"

void ADanceBattleGameState::ResetRound(float Duration)
{
    PlayerScore = 0;
    AIScore = 0;
    Result = TEXT("");
    RemainingTime = Duration;
    Phase = EDancePhase::Playing;
    OnChanged.Broadcast();
}

void ADanceBattleGameState::SetRemaining(float Seconds)
{
    float NewSeconds = FMath::Max(0.0f, Seconds);
    
    bool bIntegerChanged = FMath::CeilToInt(RemainingTime) != FMath::CeilToInt(NewSeconds);
    
    RemainingTime = NewSeconds;

    if (bIntegerChanged)
    {
        OnChanged.Broadcast();
    }
}

void ADanceBattleGameState::ApplyJudgment(const FDanceNote& Note, const FDanceSettings& Settings)
{
    if (Phase != EDancePhase::Playing)
    {
        return;
    }

    int32 Points = 0;
    if (Note.Judgment == EDanceJudgment::Perfect)
    {
        Points = Settings.PerfectPoints;
    }
    else if (Note.Judgment == EDanceJudgment::Good)
    {
        Points = Settings.GoodPoints;
    }

    if (Points <= 0)
    {
        return;
    }

    if (Note.Side == EDanceSide::Player)
    {
        PlayerScore += Points;
    }
    else
    {
        AIScore += Points;
    }

    OnChanged.Broadcast();
}

void ADanceBattleGameState::Finish()
{
    if (Phase == EDancePhase::Finished)
    {
        return;
    }

    Phase = EDancePhase::Finished;
    RemainingTime = 0.0f;

    if (PlayerScore > AIScore)
    {
        Result = TEXT("YOU WIN");
    }
    else if (AIScore > PlayerScore)
    {
        Result = TEXT("YOU LOSE");
    }
    else
    {
        Result = TEXT("TIE");
    }

    OnChanged.Broadcast();
}