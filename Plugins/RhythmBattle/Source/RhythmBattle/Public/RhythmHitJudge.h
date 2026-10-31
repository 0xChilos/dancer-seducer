#pragma once
#include "CoreMinimal.h"
#include "RhythmBattleTypes.h"
class URhythmBattleConfig;

class RHYTHMBATTLE_API FRhythmHitJudge
{
public:
    static ERhythmJudgment Judge(double Error, const URhythmBattleConfig& Config, double WindowMultiplier = 1.);
    static int32 FindClosestNote(const TArray<FRhythmRuntimeNote>& Notes, int32 Player, int32 Lane, double InputSongTime, double MaxWindow);
    static FRhythmHitResult Resolve(TArray<FRhythmRuntimeNote>& Notes, int32 Index, double InputSongTime, const URhythmBattleConfig& Config, double WindowMultiplier = 1.);
};
