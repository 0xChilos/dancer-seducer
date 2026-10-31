#pragma once

#include "CoreMinimal.h"
#include "RhythmEventAction.h"
#include "RhythmModifierManager.h"
#include "RhythmDemoActions.generated.h"

UCLASS(Blueprintable, EditInlineNew)
class RHYTHMBATTLE_API URhythmModifierEventAction : public URhythmEventAction
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm")
    FRhythmModifierSpec Modifier;

    virtual void Execute_Implementation(class URhythmBattleComponent* Battle, FName EventId) override;
};

UCLASS(Blueprintable, EditInlineNew)
class RHYTHMBATTLE_API URhythmPauseEventAction : public URhythmEventAction
{
    GENERATED_BODY()

public:
    virtual void Execute_Implementation(class URhythmBattleComponent* Battle, FName EventId) override;
    virtual void Complete_Implementation(class URhythmBattleComponent* Battle) override;
    virtual void Cancel_Implementation(class URhythmBattleComponent* Battle) override;

private:
    UPROPERTY()
    FName PauseReason;
};
