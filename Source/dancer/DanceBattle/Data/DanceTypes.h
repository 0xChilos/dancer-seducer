#pragma once

#include "CoreMinimal.h"
#include "DanceTypes.generated.h"

UENUM(BlueprintType)
enum class EDanceSide : uint8
{
    Player,
    AI
};

UENUM(BlueprintType)
enum class EDanceDirection : uint8
{
    Up,
    Down,
    Left,
    Right
};

UENUM(BlueprintType)
enum class EDanceJudgment : uint8
{
    Pending,
    Perfect,
    Good,
    Miss
};

UENUM(BlueprintType)
enum class EDancePhase : uint8
{
    Waiting,
    Playing,
    Finished
};

USTRUCT(BlueprintType)
struct DANCER_API FDanceNote
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 NoteId = 0;

    UPROPERTY(BlueprintReadOnly)
    EDanceSide Side = EDanceSide::Player;

    UPROPERTY(BlueprintReadOnly)
    EDanceDirection Direction = EDanceDirection::Up;

    UPROPERTY(BlueprintReadOnly)
    float SpawnTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float HitTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    EDanceJudgment Judgment = EDanceJudgment::Pending;

    UPROPERTY(BlueprintReadOnly)
    bool bSpawned = false;
};

USTRUCT(BlueprintType)
struct DANCER_API FDanceSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float RoundDuration = 60.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float BeatInterval = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float PerfectWindow = 0.06f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float GoodWindow = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TravelTime = 1.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 PerfectPoints = 20;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 GoodPoints = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Seed = 1337;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AIAccuracy = 0.7f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AITimingSpread = 0.20f;
};
