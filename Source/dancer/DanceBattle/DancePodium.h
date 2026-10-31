#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "DancePodium.generated.h"

UCLASS()
class DANCER_API ADancePodium : public AActor
{
    GENERATED_BODY()

public:
    ADancePodium();

    UPROPERTY(VisibleAnywhere) USceneComponent* Root;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* Platform;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* Accent;

    void Pulse(float Strength);

private:
    FVector BaseAccentScale;
};
