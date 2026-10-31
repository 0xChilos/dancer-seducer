#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "../Data/DanceTypes.h"
#include "DanceBattleGameState.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnDanceStateChanged);

UCLASS()
class DANCER_API ADanceBattleGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category = "Dance")
    int32 PlayerScore = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Dance")
    int32 AIScore = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Dance")
    float RemainingTime = 60.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Dance")
    EDancePhase Phase = EDancePhase::Waiting;

    UPROPERTY(BlueprintReadOnly, Category = "Dance")
    FString Result;

    FOnDanceStateChanged OnChanged;

    UFUNCTION(BlueprintCallable, Category = "Dance")
    void ResetRound(float Duration);

    UFUNCTION(BlueprintCallable, Category = "Dance")
    void SetRemaining(float Seconds);

    UFUNCTION(BlueprintCallable, Category = "Dance")
    void ApplyJudgment(const FDanceNote& Note, const FDanceSettings& Settings);

    UFUNCTION(BlueprintCallable, Category = "Dance")
    void Finish();
};