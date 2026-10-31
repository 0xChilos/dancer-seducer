#pragma once
#include "CoreMinimal.h"
#include "RhythmBattleTypes.h"
class URhythmBattleConfig;
class RHYTHMBATTLE_API FRhythmScoreCalculator
{
public:
    static void Apply(FRhythmPlayerScore& Score, FRhythmHitResult& Result, const URhythmBattleConfig& Config, double ScoreMultiplier = 1.);
};
