#pragma once

#include "CoreMinimal.h"
#include "RhythmDemoGameMode.h"
#include "DanceRhythmArenaGameMode.generated.h"

class ADancePlaceholderCharacter;

UCLASS()
class DANCER_API ADanceRhythmArenaGameMode : public ARhythmDemoGameMode
{
    GENERATED_BODY()
public:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UFUNCTION() void HandleNote(const FRhythmHitResult& Result);
    UPROPERTY() TObjectPtr<ADancePlaceholderCharacter> PlayerDancer;
    UPROPERTY() TObjectPtr<ADancePlaceholderCharacter> AIDancer;
};
