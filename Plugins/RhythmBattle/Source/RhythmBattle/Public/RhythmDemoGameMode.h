#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "RhythmBattleTypes.h"
#include "RhythmDemoGameMode.generated.h"
class URhythmBattleComponent;
class URhythmChart;
class URhythmBattleConfig;
class URhythmBattleWidget;
class USoundBase;
class UInputAction;
class UInputMappingContext;

/** Optional demo mode; existing project game modes are unaffected. */
UCLASS()
class RHYTHMBATTLE_API ARhythmDemoGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ARhythmDemoGameMode();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    UFUNCTION(BlueprintCallable) void RestartBattle();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<URhythmBattleComponent> Battle;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<URhythmChart> DemoChart;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<URhythmBattleConfig> DemoConfig;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<USoundBase> DemoMusic;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bOpponentAutoplay = true;
private:
    UFUNCTION() void HandleEvent(FName Id);
    UFUNCTION() void HandleScore(const struct FRhythmPlayerScore& Score);
    bool bComboBoostUsed = false;
    double TauntResumeAt = 0.;
};

UCLASS()
class RHYTHMBATTLE_API ARhythmDemoPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    ARhythmDemoPlayerController();
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
private:
    void PressLane(int32 Player, int32 Lane);
    void Restart();
    void TogglePause();
    UPROPERTY(EditAnywhere, Category="Rhythm") TObjectPtr<UInputMappingContext> AuthoredMapping;
    UPROPERTY() TObjectPtr<UInputMappingContext> Mapping;
    UPROPERTY() TArray<TObjectPtr<UInputAction>> Actions;
    UPROPERTY(EditAnywhere, Category="Rhythm") TSubclassOf<URhythmBattleWidget> HUDClass;
    UPROPERTY() TObjectPtr<URhythmBattleWidget> HUD;
};
