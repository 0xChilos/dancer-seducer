#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "../Data/DanceTypes.h"
#include "DanceBattlePlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UDanceBattleWidget;
class ADanceBattleGameMode;

UCLASS()
class DANCER_API ADanceBattlePlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DanceBattle")
    TObjectPtr<UInputMappingContext> DanceContext;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DanceBattle")
    TArray<TObjectPtr<UInputAction>> DanceActions;

    UPROPERTY(BlueprintReadOnly, Category = "DanceBattle")
    TObjectPtr<UDanceBattleWidget> HUD;

private:
    void Up();
    void Down();
    void Left();
    void Right();
    void Restart();

    void HandleDirection(EDanceDirection Direction);
};