#include "RhythmDemoActions.h"
#include "RhythmBattleComponent.h"

void URhythmModifierEventAction::Execute_Implementation(URhythmBattleComponent* Battle, FName EventId)
{
    if (Battle)
    {
        Battle->AddTimedModifier(Modifier);
    }
}

void URhythmPauseEventAction::Execute_Implementation(URhythmBattleComponent* Battle, FName EventId)
{
    if (Battle)
    {
        PauseReason = EventId;
        Battle->PauseRhythm(EventId);
    }
}

void URhythmPauseEventAction::Complete_Implementation(URhythmBattleComponent* Battle)
{
    if (Battle)
    {
        Battle->ResumeRhythm(PauseReason);
    }
}

void URhythmPauseEventAction::Cancel_Implementation(URhythmBattleComponent* Battle)
{
    if (Battle)
    {
        Battle->ResumeRhythm(PauseReason);
    }
}