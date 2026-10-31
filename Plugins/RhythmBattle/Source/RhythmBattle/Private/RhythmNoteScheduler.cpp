#include "RhythmNoteScheduler.h"
#include "RhythmChart.h"

void FRhythmNoteScheduler::Initialize(const URhythmChart& Chart)
{
    Reset();
    Notes.Reserve(Chart.Notes.Num());
    for (int32 Id = 0; Id < Chart.Notes.Num(); ++Id)
    {
        const auto& Def = Chart.Notes[Id];
        FRhythmRuntimeNote& Note = Notes.AddDefaulted_GetRef();
        Note.NoteId = Id; Note.PlayerIndex = Def.PlayerIndex; Note.LaneId = Def.LaneId;
        Note.HitTimeSeconds = Chart.BeatToSeconds(Def.Beat);
    }
    Notes.StableSort([](const auto& A, const auto& B) { return A.HitTimeSeconds < B.HitTimeSeconds; });
}
void FRhythmNoteScheduler::Update(double SongTime, double ApproachDuration, bool bSpawn)
{
    ApproachDuration = FMath::Max(ApproachDuration, 0.001);
    for (int32 Index = FirstUnresolved; Index < Notes.Num(); ++Index)
    {
        auto& Note = Notes[Index];
        if (Note.HitTimeSeconds > SongTime + ApproachDuration) break;
        if (Note.Judgment != ERhythmJudgment::None) continue;
        if (bSpawn) { Note.bSpawned = true; Note.State = ERhythmNoteState::Active; }
        Note.Progress = FMath::Clamp(1. - (Note.HitTimeSeconds - SongTime) / ApproachDuration, 0., 1.);
    }
}
TArray<int32> FRhythmNoteScheduler::ExpireMisses(double SongTime, double Window)
{
    TArray<int32> Expired;
    while (FirstUnresolved < Notes.Num())
    {
        auto& Note = Notes[FirstUnresolved];
        if (Note.Judgment == ERhythmJudgment::None)
        {
            if (SongTime <= Note.HitTimeSeconds + Window) break;
            Note.Judgment = ERhythmJudgment::Miss; Note.State = ERhythmNoteState::Missed;
            Expired.Add(FirstUnresolved);
        }
        ++FirstUnresolved;
    }
    return Expired;
}
void FRhythmNoteScheduler::SkipLane(int32 Player, int32 Lane, double SongTime, double Window)
{
    for (auto& Note : Notes)
    {
        if (Note.PlayerIndex == Player && Note.LaneId == Lane && Note.Judgment == ERhythmJudgment::None && Note.HitTimeSeconds <= SongTime + Window)
        { Note.Judgment = ERhythmJudgment::Skipped; Note.State = ERhythmNoteState::Skipped; }
    }
}
