#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RhythmBattleConfig.generated.h"

UCLASS(BlueprintType)
class RHYTHMBATTLE_API URhythmBattleConfig : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double PerfectWindowSeconds = 0.045;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double GoodWindowSeconds = 0.100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double OKWindowSeconds = 0.160;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.01")) float BaseApproachDuration = 2.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.01")) float VisualSpeedMultiplier = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) float ScoreMultiplier = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.01")) float HitWindowMultiplier = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) float ComboMultiplier = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) int32 PerfectScore = 100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) int32 GoodScore = 70;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) int32 OKScore = 30;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) int32 MissPenalty = 0;
    // Positive latency is subtracted from the mapped input song timestamp.
    UPROPERTY(EditAnywhere, BlueprintReadWrite) double InputLatencyCompensation = 0.;
};
