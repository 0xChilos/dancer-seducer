#include "DancePodium.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "UObject/ConstructorHelpers.h"

ADancePodium::ADancePodium()
{
    PrimaryActorTick.bCanEverTick = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

    Platform = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Platform"));
    Platform->SetupAttachment(Root);
    if (CubeMesh.Succeeded()) Platform->SetStaticMesh(CubeMesh.Object);
    Platform->SetRelativeLocation(FVector(0, 0, -15));
    Platform->SetRelativeScale3D(FVector(2.8f, 2.8f, 0.3f));

    Accent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Accent"));
    Accent->SetupAttachment(Root);
    if (CylinderMesh.Succeeded()) Accent->SetStaticMesh(CylinderMesh.Object);
    Accent->SetRelativeLocation(FVector(0, 0, 3));
    Accent->SetRelativeScale3D(FVector(2.6f, 2.6f, 0.04f));
    Accent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    BaseAccentScale = Accent->GetRelativeScale3D();
}

void ADancePodium::Pulse(float Strength)
{
    Accent->SetRelativeScale3D(BaseAccentScale * Strength);
}
