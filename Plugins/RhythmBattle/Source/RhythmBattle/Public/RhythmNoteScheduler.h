#pragma once
#include "CoreMinimal.h"
#include "RhythmBattleTypes.h"
class URhythmChart;

class RHYTHMBATTLE_API FRhythmNoteScheduler
{
public:
    void Initialize(const URhythmChart& Chart);
    void Reset() { Notes.Reset(); FirstUnresolved = 0; }
    TArray<FRhythmRuntimeNote>& GetNotes() { return Notes; }
    const TArray<FRhythmRuntimeNote>& GetNotes() const { return Notes; }
    void Update(double SongTime, double ApproachDuration, bool bSpawn = true);
    TArray<int32> ExpireMisses(double SongTime, double Window);
    void SkipLane(int32 Player, int32 Lane, double SongTime, double Window);
private:
    TArray<FRhythmRuntimeNote> Notes;
    int32 FirstUnresolved = 0;
};
