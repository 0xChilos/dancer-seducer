#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RhythmBattleTypes.h"
#include "RhythmModifierManager.h"
#include "RhythmClock.h"
#include "RhythmNoteScheduler.h"
#include "RhythmBattleComponent.generated.h"
class URhythmChart;
class URhythmBattleConfig;
class USoundBase;
class URhythmAudioSync;
class URhythmEventAction;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRhythmHitDelegate, const FRhythmHitResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRhythmScoreDelegate, const FRhythmPlayerScore&, Score);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRhythmReasonDelegate, FName, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRhythmStateDelegate);
UCLASS(ClassGroup=(Rhythm), meta=(BlueprintSpawnableComponent))
class RHYTHMBATTLE_API URhythmBattleComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    URhythmBattleComponent();
    UFUNCTION(BlueprintCallable, Category="Rhythm") bool StartBattle(URhythmChart* InChart, URhythmBattleConfig* InConfig, USoundBase* Music = nullptr);
    UFUNCTION(BlueprintCallable, Category="Rhythm") void StopBattle();
    UFUNCTION(BlueprintCallable, Category="Rhythm") void PauseRhythm(FName Reason);
    UFUNCTION(BlueprintCallable, Category="Rhythm") void ResumeRhythm(FName Reason);
    UFUNCTION(BlueprintCallable, Category="Rhythm") void StopSpawning(bool bStop = true);
    UFUNCTION(BlueprintCallable, Category="Rhythm") void SetVisualSpeed(float Multiplier);
    UFUNCTION(BlueprintCallable, Category="Rhythm") void SetScoreMultiplier(float Multiplier);
    UFUNCTION(BlueprintCallable, Category="Rhythm") void SetHitWindowMultiplier(float Multiplier);
    UFUNCTION(BlueprintCallable, Category="Rhythm") void EnableLane(int32 PlayerIndex, int32 LaneId, bool bEnabled);
    UFUNCTION(BlueprintCallable, Category="Rhythm") void SetLaneEnabled(int32 PlayerIndex, int32 LaneId, bool bEnabled) { EnableLane(PlayerIndex, LaneId, bEnabled); }
    UFUNCTION(BlueprintCallable, Category="Rhythm") void TriggerRhythmEvent(FName EventId, URhythmEventAction* Action = nullptr);
    UFUNCTION(BlueprintCallable, Category="Rhythm") bool AddTimedModifier(const FRhythmModifierSpec& Modifier);
    UFUNCTION(BlueprintCallable, Category="Rhythm") bool RemoveModifier(FName Id);
    UFUNCTION(BlueprintCallable, Category="Rhythm") FRhythmHitResult SubmitLaneInput(int32 PlayerIndex, int32 LaneId);
    UFUNCTION(BlueprintPure, Category="Rhythm") double GetSongTimeSeconds() const;
    UFUNCTION(BlueprintPure, Category="Rhythm") double GetDurationSeconds() const;
    UFUNCTION(BlueprintPure, Category="Rhythm") ERhythmBattleState GetBattleState() const { return State; }
    UFUNCTION(BlueprintPure, Category="Rhythm") TArray<FRhythmRuntimeNote> GetNoteSnapshot() const { return Scheduler.GetNotes(); }
    UFUNCTION(BlueprintPure, Category="Rhythm") TArray<FRhythmPlayerScore> GetScoreSnapshot() const { return Scores; }
    UFUNCTION(BlueprintPure, Category="Rhythm") TArray<FRhythmLaneLayout> GetLaneSnapshot() const { return Lanes; }
    UFUNCTION(BlueprintPure, Category="Rhythm") float GetEffectiveModifier(ERhythmModifierType Type, int32 PlayerIndex, int32 LaneId) const;
    UFUNCTION(BlueprintPure, Category="Rhythm") TArray<FRhythmModifierSpec> GetActiveModifiers() const { return Modifiers ? Modifiers->GetActiveModifiers() : TArray<FRhythmModifierSpec>(); }
    UPROPERTY(BlueprintAssignable) FRhythmHitDelegate OnNoteHit;
    UPROPERTY(BlueprintAssignable) FRhythmHitDelegate OnNoteMiss;
    UPROPERTY(BlueprintAssignable) FRhythmScoreDelegate OnScoreChanged;
    UPROPERTY(BlueprintAssignable) FRhythmScoreDelegate OnComboChanged;
    UPROPERTY(BlueprintAssignable) FRhythmReasonDelegate OnRhythmEventTriggered;
    UPROPERTY(BlueprintAssignable) FRhythmReasonDelegate OnRhythmPaused;
    UPROPERTY(BlueprintAssignable) FRhythmReasonDelegate OnRhythmResumed;
    UPROPERTY(BlueprintAssignable) FRhythmStateDelegate OnBattleFinished;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    // Deterministic C++ test seam; set only while stopped.
    void SetTimeProvider(FRhythmClock::FTimeProvider Provider);
    void AdvanceForTesting() { UpdateBattle(); }
private:
    void UpdateBattle();
    void ApplyResult(FRhythmHitResult& Result);
    bool IsLaneEnabled(int32 Player, int32 Lane) const;
    UPROPERTY(Transient) TObjectPtr<URhythmChart> Chart;
    UPROPERTY(Transient) TObjectPtr<URhythmBattleConfig> Config;
    UPROPERTY(Transient) TObjectPtr<URhythmModifierManager> Modifiers;
    UPROPERTY(Transient) TObjectPtr<URhythmAudioSync> AudioSync;
    UPROPERTY(Transient) TArray<TObjectPtr<URhythmEventAction>> RunningActions;
    FRhythmClock Clock;
    FRhythmNoteScheduler Scheduler;
    TArray<FRhythmPlayerScore> Scores;
    TArray<FRhythmLaneLayout> Lanes;
    TSet<FName> PauseReasons;
    TSet<int32> FiredEvents;
    ERhythmBattleState State = ERhythmBattleState::Stopped;
    bool bSpawning = true;
    float VisualSpeed = 1.f, ScoreMultiplier = 1.f, WindowMultiplier = 1.f;
    double LastVisualTime = 0.;
    uint64 Generation = 0;
};
