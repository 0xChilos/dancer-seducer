#include "RhythmScoreCalculator.h"
#include "RhythmBattleConfig.h"

void FRhythmScoreCalculator::Apply(FRhythmPlayerScore& Score, FRhythmHitResult& Result, const URhythmBattleConfig& Config, double Multiplier)
{
    int32 Base = 0;
    switch (Result.Judgment)
    {
    case ERhythmJudgment::Perfect: Base = Config.PerfectScore; break;
    case ERhythmJudgment::Good: Base = Config.GoodScore; break;
    case ERhythmJudgment::OK: Base = Config.OKScore; break;
    case ERhythmJudgment::Miss:
        Result.Points = -FMath::Min<int64>(Score.Score, FMath::Max(0, Config.MissPenalty));
        Score.Score += Result.Points; Score.Combo = 0; return;
    default: return;
    }
    const double ComboBonus = 1. + Score.Combo * FMath::Max(0.f, Config.ComboMultiplier);
    Result.Points = FMath::RoundToInt64(Base * FMath::Max(0., Multiplier) * ComboBonus);
    Score.Score += Result.Points;
    ++Score.Combo; Score.MaxCombo = FMath::Max(Score.MaxCombo, Score.Combo);
}
