#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RhythmModifierManager.generated.h"

UENUM(BlueprintType)
enum class ERhythmModifierType : uint8 { VisualSpeed, Score, HitWindow, LaneDisable };
UENUM(BlueprintType)
enum class ERhythmModifierScope : uint8 { Battle, Player, Lane };

/** Multipliers stack by multiplication. LaneDisable stacks by logical OR. Expiry uses song time. */
USTRUCT(BlueprintType)
struct RHYTHMBATTLE_API FRhythmModifierSpec
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Id;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) ERhythmModifierType Type = ERhythmModifierType::Score;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) ERhythmModifierScope Scope = ERhythmModifierScope::Battle;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 PlayerIndex = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 LaneId = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.01")) float Value = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) double DurationSeconds = 1.;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bDurationInBeats = false;
};

UCLASS(BlueprintType)
class RHYTHMBATTLE_API URhythmModifierManager : public UObject
{
    GENERATED_BODY()
public:
    bool Add(const FRhythmModifierSpec& Spec, double SongTime, double SecondsPerBeat);
    UFUNCTION(BlueprintCallable, Category="Rhythm|Modifiers") bool RemoveModifier(FName Id);
    void Expire(double SongTime);
    void Reset();
    float Evaluate(ERhythmModifierType Type, int32 Player, int32 Lane) const;
    UFUNCTION(BlueprintPure, Category="Rhythm|Modifiers") TArray<FRhythmModifierSpec> GetActiveModifiers() const;
private:
    UPROPERTY() TArray<FRhythmModifierSpec> Active;
    TMap<FName, double> Expiry;
};
