#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DanceTypes.h"
#include "DancePlaceholderCharacter.generated.h"

UCLASS()
class DANCER_API ADancePlaceholderCharacter : public AActor
{
    GENERATED_BODY()

public:
    ADancePlaceholderCharacter();

    UPROPERTY(VisibleAnywhere)
    USceneComponent* Root;

    UPROPERTY(VisibleAnywhere)
    USceneComponent* DanceRoot;

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* Body;

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* Head;

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* LeftArm;

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* RightArm;

    UPROPERTY(EditAnywhere)
    EDanceSide Side = EDanceSide::Player;

    void Configure(EDanceSide InSide);
    void ResetDance();
    void React(EDanceDirection InDirection, EDanceJudgment InJudgment);

protected:
    virtual void Tick(float DeltaTime) override;

private:
    FTransform BaseBodyTransform;
    FTransform BaseHeadTransform;
    FTransform BaseLeftArmTransform;
    FTransform BaseRightArmTransform;

    float ReactionTimer = 0.0f;
    float ShakeTimer = 0.0f;

    EDanceDirection LastDirection = EDanceDirection::Up;
    EDanceJudgment LastJudgment = EDanceJudgment::Pending;
};