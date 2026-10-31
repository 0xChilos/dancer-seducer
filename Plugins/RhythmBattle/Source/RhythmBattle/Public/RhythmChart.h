#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RhythmBattleTypes.h"
#include "RhythmChart.generated.h"

UCLASS(BlueprintType)
class RHYTHMBATTLE_API URhythmChart : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1")) double BPM = 120.;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) double SongOffsetSeconds = 0.;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double DurationSeconds = 30.;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1")) int32 BeatsPerBar = 4;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FRhythmNoteDefinition> Notes;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FRhythmLaneLayout> Lanes;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FRhythmEventCue> Events;
    UFUNCTION(BlueprintPure) double BeatToSeconds(double Beat) const { return SongOffsetSeconds + Beat * 60. / FMath::Max(BPM, 1.); }
};
