#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RhythmEventAction.generated.h"
class URhythmBattleComponent;
UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class RHYTHMBATTLE_API URhythmEventAction : public UObject
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintNativeEvent, Category="Rhythm|Events") void Execute(URhythmBattleComponent* Battle, FName EventId);
    virtual void Execute_Implementation(URhythmBattleComponent* Battle, FName EventId);
    UFUNCTION(BlueprintNativeEvent, Category="Rhythm|Events") void Cancel(URhythmBattleComponent* Battle);
    virtual void Cancel_Implementation(URhythmBattleComponent* Battle);
    UFUNCTION(BlueprintNativeEvent, Category="Rhythm|Events") void Complete(URhythmBattleComponent* Battle);
    virtual void Complete_Implementation(URhythmBattleComponent* Battle);
};
