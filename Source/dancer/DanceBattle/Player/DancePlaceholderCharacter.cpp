#include "DancePlaceholderCharacter.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "UObject/ConstructorHelpers.h"

ADancePlaceholderCharacter::ADancePlaceholderCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    DanceRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DanceRoot"));
    DanceRoot->SetupAttachment(Root);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    Body->SetupAttachment(DanceRoot);
    if (SphereMesh.Succeeded()) Body->SetStaticMesh(SphereMesh.Object);
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    Head = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Head"));
    Head->SetupAttachment(DanceRoot);
    if (SphereMesh.Succeeded()) Head->SetStaticMesh(SphereMesh.Object);
    Head->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    LeftArm = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftArm"));
    LeftArm->SetupAttachment(DanceRoot);
    if (CylinderMesh.Succeeded()) LeftArm->SetStaticMesh(CylinderMesh.Object);
    LeftArm->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    RightArm = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightArm"));
    RightArm->SetupAttachment(DanceRoot);
    if (CylinderMesh.Succeeded()) RightArm->SetStaticMesh(CylinderMesh.Object);
    RightArm->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    Side = EDanceSide::Player;
    Configure(Side);

    BaseBodyTransform = Body->GetRelativeTransform();
    BaseHeadTransform = Head->GetRelativeTransform();
    BaseLeftArmTransform = LeftArm->GetRelativeTransform();
    BaseRightArmTransform = RightArm->GetRelativeTransform();

    ReactionTimer = 0.0f;
    ShakeTimer = 0.0f;
}

void ADancePlaceholderCharacter::Configure(EDanceSide InSide)
{
    Side = InSide;

    FVector BodyScale = (Side == EDanceSide::Player) ? FVector(0.8f, 0.85f, 1.8f) : FVector(0.5f, 0.5f, 2.0f);
    Body->SetRelativeScale3D(BodyScale);
    Body->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));

    Head->SetRelativeScale3D(FVector(0.4f, 0.4f, 0.4f));
    Head->SetRelativeLocation(FVector(0.0f, 0.0f, 205.0f));

    LeftArm->SetRelativeScale3D(FVector(0.16f, 0.16f, 0.7f));
    LeftArm->SetRelativeLocation(FVector(0.0f, -60.0f, 125.0f));

    RightArm->SetRelativeScale3D(FVector(0.16f, 0.16f, 0.7f));
    RightArm->SetRelativeLocation(FVector(0.0f, 60.0f, 125.0f));
}

void ADancePlaceholderCharacter::React(EDanceDirection Direction, EDanceJudgment Judgment)
{
    ReactionTimer = 0.3f;
    LastDirection = Direction;
    LastJudgment = Judgment;
    if (Judgment == EDanceJudgment::Miss) ShakeTimer = 0.2f;
}

void ADancePlaceholderCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    float Time = GetWorld()->GetTimeSeconds();

    // Update Timers
    ReactionTimer = FMath::Max(0.0f, ReactionTimer - DeltaTime);
    ShakeTimer = FMath::Max(0.0f, ShakeTimer - DeltaTime);

    // Idle Calculations
    float IdleZ = FMath::Abs(FMath::Sin(Time * 4.0f)) * 10.0f;
    float IdleY = FMath::Sin(Time * 2.0f) * 5.0f;
    float IdleRoll = FMath::Sin(Time * 2.0f) * 4.0f;

    FVector TargetLocation = FVector(0.0f, IdleY, IdleZ);
    FRotator TargetRotation = FRotator(0.0f, 0.0f, IdleRoll);

    // Reaction Logic
    if (ReactionTimer > 0.0f)
    {
        float Bounce = FMath::Sin((1.0f - (ReactionTimer / 0.3f)) * PI) * 20.0f;
        
        if (LastDirection == EDanceDirection::Up || LastDirection == EDanceDirection::Down)
        {
            float PitchSign = (LastDirection == EDanceDirection::Up) ? 1.0f : -1.0f;
            TargetRotation.Pitch += PitchSign * Bounce * 0.5f;
        }
        else if (LastDirection == EDanceDirection::Left || LastDirection == EDanceDirection::Right)
        {
            float RollSign = (LastDirection == EDanceDirection::Left) ? 1.0f : -1.0f;
            TargetRotation.Roll += RollSign * Bounce * 0.5f;
        }
    }

    // Shake Logic
    if (ShakeTimer > 0.0f)
    {
        TargetLocation.Y += FMath::Sin(Time * 55.0f) * 10.0f;
    }

    // Apply to DanceRoot
    DanceRoot->SetRelativeLocation(TargetLocation);
    DanceRoot->SetRelativeRotation(TargetRotation);
}
void ADancePlaceholderCharacter::ResetDance() { ReactionTimer = 0; ShakeTimer = 0; DanceRoot->SetRelativeLocation(FVector::ZeroVector); DanceRoot->SetRelativeRotation(FRotator::ZeroRotator); }
