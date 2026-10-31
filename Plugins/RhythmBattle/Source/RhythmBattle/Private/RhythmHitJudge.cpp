#include "RhythmHitJudge.h"
#include "RhythmBattleConfig.h"

ERhythmJudgment FRhythmHitJudge::Judge(double Error, const URhythmBattleConfig& Config, double Multiplier)
{
    if (!FMath::IsFinite(Error)) return ERhythmJudgment::Miss;
    const double Absolute = FMath::Abs(Error);
    Multiplier = FMath::Max(0., Multiplier);
    if (Absolute <= Config.PerfectWindowSeconds * Multiplier + 1.e-9) return ERhythmJudgment::Perfect;
    if (Absolute <= Config.GoodWindowSeconds * Multiplier + 1.e-9) return ERhythmJudgment::Good;
    if (Absolute <= Config.OKWindowSeconds * Multiplier + 1.e-9) return ERhythmJudgment::OK;
    return ERhythmJudgment::Miss;
}
int32 FRhythmHitJudge::FindClosestNote(const TArray<FRhythmRuntimeNote>& Notes, int32 Player, int32 Lane, double Time, double Window)
{
    int32 Best = INDEX_NONE;
    double Distance = Window + 1.e-9;
    for (int32 Index = 0; Index < Notes.Num(); ++Index)
    {
        const auto& Note = Notes[Index];
        if (Note.PlayerIndex != Player || Note.LaneId != Lane || Note.Judgment != ERhythmJudgment::None) continue;
        const double Error = FMath::Abs(Time - Note.HitTimeSeconds);
        if (Error <= Window + 1.e-9 && (Best == INDEX_NONE || Error < Distance)) { Distance = Error; Best = Index; }
    }
    return Best;
}
FRhythmHitResult FRhythmHitJudge::Resolve(TArray<FRhythmRuntimeNote>& Notes, int32 Index, double Time, const URhythmBattleConfig& Config, double Multiplier)
{
    FRhythmHitResult Result;
    if (!Notes.IsValidIndex(Index) || Notes[Index].Judgment != ERhythmJudgment::None) return Result;
    auto& Note = Notes[Index];
    Result.TimingErrorSeconds = Time - Note.HitTimeSeconds;
    Result.Judgment = Judge(Result.TimingErrorSeconds, Config, Multiplier);
    if (Result.Judgment == ERhythmJudgment::Miss) { Result.Judgment = ERhythmJudgment::None; return Result; }
    Result.NoteId = Note.NoteId; Result.PlayerIndex = Note.PlayerIndex; Result.LaneId = Note.LaneId;
    Note.Judgment = Result.Judgment; Note.State = ERhythmNoteState::Hit;
    return Result;
}
