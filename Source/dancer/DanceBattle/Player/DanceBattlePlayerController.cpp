#include "DanceBattlePlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputCoreTypes.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Camera/CameraActor.h"
#include "../UI/DanceBattleWidget.h"
#include "../Gameplay/DanceBattleGameMode.h"

void ADanceBattlePlayerController::BeginPlay() { Super::BeginPlay(); SetInputMode(FInputModeGameOnly()); bShowMouseCursor = false; for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It) { if (It->ActorHasTag(TEXT("DanceBattleCamera"))) { SetViewTarget(*It); break; } } HUD = CreateWidget<UDanceBattleWidget>(this, UDanceBattleWidget::StaticClass()); if (HUD) { HUD->AddToViewport(); HUD->InitializeBattle(Cast<ADanceBattleGameMode>(GetWorld()->GetAuthGameMode())); } }

void ADanceBattlePlayerController::SetupInputComponent() { Super::SetupInputComponent(); auto* E = Cast<UEnhancedInputComponent>(InputComponent); if (!E) return; DanceContext = NewObject<UInputMappingContext>(this); DanceActions.Reset(); for (int32 i = 0; i < 5; ++i) { UInputAction* IA = NewObject<UInputAction>(this); IA->ValueType = EInputActionValueType::Boolean; DanceActions.Add(IA); } DanceContext->MapKey(DanceActions[0], EKeys::Up); DanceContext->MapKey(DanceActions[0], EKeys::W); DanceContext->MapKey(DanceActions[1], EKeys::Down); DanceContext->MapKey(DanceActions[1], EKeys::S); DanceContext->MapKey(DanceActions[2], EKeys::Left); DanceContext->MapKey(DanceActions[2], EKeys::A); DanceContext->MapKey(DanceActions[3], EKeys::Right); DanceContext->MapKey(DanceActions[3], EKeys::D); DanceContext->MapKey(DanceActions[4], EKeys::R); E->BindAction(DanceActions[0], ETriggerEvent::Started, this, &ADanceBattlePlayerController::Up); E->BindAction(DanceActions[1], ETriggerEvent::Started, this, &ADanceBattlePlayerController::Down); E->BindAction(DanceActions[2], ETriggerEvent::Started, this, &ADanceBattlePlayerController::Left); E->BindAction(DanceActions[3], ETriggerEvent::Started, this, &ADanceBattlePlayerController::Right); E->BindAction(DanceActions[4], ETriggerEvent::Started, this, &ADanceBattlePlayerController::Restart); if (auto* LP = GetLocalPlayer()) { if (auto* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()) { Sub->AddMappingContext(DanceContext, 0); } } }

void ADanceBattlePlayerController::Up()
{
    HandleDirection(EDanceDirection::Up);
}

void ADanceBattlePlayerController::Down()
{
    HandleDirection(EDanceDirection::Down);
}

void ADanceBattlePlayerController::Left()
{
    HandleDirection(EDanceDirection::Left);
}

void ADanceBattlePlayerController::Right()
{
    HandleDirection(EDanceDirection::Right);
}

void ADanceBattlePlayerController::Restart()
{
    if (ADanceBattleGameMode* GM = Cast<ADanceBattleGameMode>(GetWorld()->GetAuthGameMode()))
    {
        GM->RestartMatch();
    }
}

void ADanceBattlePlayerController::HandleDirection(EDanceDirection Direction) { if (auto* GM = Cast<ADanceBattleGameMode>(GetWorld()->GetAuthGameMode())) { GM->HandleDirection(Direction); } }
