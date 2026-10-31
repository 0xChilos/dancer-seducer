#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DanceTypes.h"
#include "DanceBattleGameMode.generated.h"

class UDanceRhythmComponent;
class UDanceAIOpponentComponent;
class ADancePlaceholderCharacter;
class ADanceBattleGameState;
class ADanceBattlePlayerController;

UCLASS()
class DANCER_API ADanceBattleGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ADanceBattleGameMode();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "Dance")
    void RestartMatch();

    UFUNCTION(BlueprintCallable, Category = "Dance")
    void HandleDirection(EDanceDirection Direction);

    UFUNCTION(BlueprintPure, Category = "Dance")
    float GetElapsedTime() const;

    UPROPERTY(VisibleAnywhere, Category = "Dance")
    TObjectPtr<UDanceRhythmComponent> Rhythm;

    UPROPERTY(VisibleAnywhere, Category = "Dance")
    TObjectPtr<UDanceAIOpponentComponent> AI;

    UPROPERTY(EditAnywhere, Category = "Dance")
    FDanceSettings Settings;

private:
    void HandleJudged(const FDanceNote& Note);
    void HandleAttempt(EDanceSide Side, EDanceDirection Direction, EDanceJudgment Judgment);

    UPROPERTY() TObjectPtr<ADancePlaceholderCharacter> PlayerDancer;
    UPROPERTY() TObjectPtr<ADancePlaceholderCharacter> AIDancer;
    
    float RoundStartTime;
};