#pragma once

#include "CoreMinimal.h"
#include "RhythmBattleTypes.generated.h"

class URhythmEventAction;
class UInputAction;

UENUM(BlueprintType)
enum class ERhythmJudgment : uint8 { None, Perfect, Good, OK, Miss, Skipped };
UENUM(BlueprintType)
enum class ERhythmNoteState : uint8 { Upcoming, Active, Hit, Missed, Skipped };
UENUM(BlueprintType)
enum class ERhythmBattleState : uint8 { Stopped, Playing, Paused, Finished };

USTRUCT(BlueprintType)
struct RHYTHMBATTLE_API FRhythmLaneLayout
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 PlayerIndex = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 LaneId = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D Start = FVector2D(0.05, 0.7);
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D HitTarget = FVector2D(0.4, 0.7);
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FLinearColor Color = FLinearColor::Yellow;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1")) float DotRadius = 6.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bEnabled = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UInputAction> InputAction;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) float ApproachDurationOverride = 0.f;
};

USTRUCT(BlueprintType)
struct RHYTHMBATTLE_API FRhythmNoteDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 PlayerIndex = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 LaneId = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double Beat = 0.;
};

USTRUCT(BlueprintType)
struct RHYTHMBATTLE_API FRhythmRuntimeNote
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) int32 NoteId = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly) int32 PlayerIndex = 0;
    UPROPERTY(BlueprintReadOnly) int32 LaneId = 0;
    UPROPERTY(BlueprintReadOnly) double HitTimeSeconds = 0.;
    UPROPERTY(BlueprintReadOnly) float Progress = 0.f;
    UPROPERTY(BlueprintReadOnly) ERhythmJudgment Judgment = ERhythmJudgment::None;
    UPROPERTY(BlueprintReadOnly) ERhythmNoteState State = ERhythmNoteState::Upcoming;
    UPROPERTY(BlueprintReadOnly) bool bSpawned = false;
};

USTRUCT(BlueprintType)
struct RHYTHMBATTLE_API FRhythmPlayerScore
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) int32 PlayerIndex = 0;
    UPROPERTY(BlueprintReadOnly) int64 Score = 0;
    UPROPERTY(BlueprintReadOnly) int32 Combo = 0;
    UPROPERTY(BlueprintReadOnly) int32 MaxCombo = 0;
};

USTRUCT(BlueprintType)
struct RHYTHMBATTLE_API FRhythmHitResult
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) int32 NoteId = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly) int32 PlayerIndex = 0;
    UPROPERTY(BlueprintReadOnly) int32 LaneId = 0;
    UPROPERTY(BlueprintReadOnly) ERhythmJudgment Judgment = ERhythmJudgment::None;
    UPROPERTY(BlueprintReadOnly) double TimingErrorSeconds = 0.;
    UPROPERTY(BlueprintReadOnly) int64 Points = 0;
};

USTRUCT(BlueprintType)
struct RHYTHMBATTLE_API FRhythmEventCue
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName EventId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double Beat = 0.;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced) TObjectPtr<URhythmEventAction> Action;
};
