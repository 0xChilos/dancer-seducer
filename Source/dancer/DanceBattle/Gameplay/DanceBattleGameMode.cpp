#include "DanceBattleGameMode.h"
#include "DanceBattleGameState.h"
#include "../Player/DanceBattlePlayerController.h"
#include "../Rhythm/DanceRhythmComponent.h"
#include "../AI/DanceAIOpponentComponent.h"
#include "../Player/DancePlaceholderCharacter.h"
#include "EngineUtils.h"
#include "Engine/World.h"

ADanceBattleGameMode::ADanceBattleGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
    GameStateClass = ADanceBattleGameState::StaticClass();
    PlayerControllerClass = ADanceBattlePlayerController::StaticClass();
    DefaultPawnClass = nullptr;

    Rhythm = CreateDefaultSubobject<UDanceRhythmComponent>(TEXT("Rhythm"));
    AI = CreateDefaultSubobject<UDanceAIOpponentComponent>(TEXT("AI"));
}

void ADanceBattleGameMode::BeginPlay()
{
    Super::BeginPlay();

    for (TActorIterator<ADancePlaceholderCharacter> It(GetWorld()); It; ++It)
    {
        ADancePlaceholderCharacter* Dancer = *It;
        if (Dancer->Side == EDanceSide::Player) PlayerDancer = Dancer;
        else if (Dancer->Side == EDanceSide::AI) AIDancer = Dancer;
    }

    if (!PlayerDancer) 
    {
        FActorSpawnParameters Params; 
        PlayerDancer = GetWorld()->SpawnActor<ADancePlaceholderCharacter>(ADancePlaceholderCharacter::StaticClass(), FVector(0, -230, 30), FRotator::ZeroRotator, Params);
        
    }
    if (!AIDancer) 
    {
        FActorSpawnParameters Params; 
        AIDancer = GetWorld()->SpawnActor<ADancePlaceholderCharacter>(ADancePlaceholderCharacter::StaticClass(), FVector(0, 230, 30), FRotator::ZeroRotator, Params);
        
    }

    if (PlayerDancer) PlayerDancer->Configure(EDanceSide::Player); if (AIDancer) AIDancer->Configure(EDanceSide::AI);
    Rhythm->OnJudged.AddUObject(this, &ADanceBattleGameMode::HandleJudged);
    Rhythm->OnAttempt.AddUObject(this, &ADanceBattleGameMode::HandleAttempt);

    RestartMatch();
}

void ADanceBattleGameMode::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    ADanceBattleGameState* GS = GetGameState<ADanceBattleGameState>();
    if (!GS || GS->Phase != EDancePhase::Playing) return;

    float Time = GetElapsedTime();
    AI->Advance(Time);
    Rhythm->Advance(Time);

    GS->SetRemaining(Rhythm->Settings.RoundDuration - Time);

    if (Time >= Rhythm->Settings.RoundDuration)
    {
        AI->Cancel();
        Rhythm->Finish();
        GS->Finish();
    }
}

void ADanceBattleGameMode::RestartMatch() { AI->Cancel(); if(PlayerDancer) PlayerDancer->ResetDance(); if(AIDancer) AIDancer->ResetDance(); Rhythm->Reset(Settings); AI->Reset(Rhythm); RoundStartTime=GetWorld()->GetTimeSeconds(); if (ADanceBattleGameState* GS=GetGameState<ADanceBattleGameState>()) GS->ResetRound(Rhythm->Settings.RoundDuration); Rhythm->Advance(0.0f); }

float ADanceBattleGameMode::GetElapsedTime() const { return FMath::Clamp(GetWorld()->GetTimeSeconds()-RoundStartTime,0.0f,Rhythm->Settings.RoundDuration); }

void ADanceBattleGameMode::HandleDirection(EDanceDirection Direction)
{
    ADanceBattleGameState* GS = GetGameState<ADanceBattleGameState>();
    if (GS && GS->Phase == EDancePhase::Playing)
    {
        float Time = GetElapsedTime();
        AI->Advance(Time);
        Rhythm->Advance(Time);
        Rhythm->Attempt(EDanceSide::Player, Direction, Time);
    }
}

void ADanceBattleGameMode::HandleJudged(const FDanceNote& Note)
{
    ADanceBattleGameState* GS = GetGameState<ADanceBattleGameState>();
    if (GS) GS->ApplyJudgment(Note, Rhythm->Settings);

    if (Note.Judgment == EDanceJudgment::Miss)
    {
        if (Note.Side == ::EDanceSide::Player && PlayerDancer) PlayerDancer->React(Note.Direction, EDanceJudgment::Miss);
        else if (Note.Side == ::EDanceSide::AI && AIDancer) AIDancer->React(Note.Direction, EDanceJudgment::Miss);
    }
}

void ADanceBattleGameMode::HandleAttempt(EDanceSide Side, EDanceDirection Direction, EDanceJudgment Judgment)
{
    if (Side == EDanceSide::Player && PlayerDancer) PlayerDancer->React(Direction, Judgment);
    else if (Side == EDanceSide::AI && AIDancer) AIDancer->React(Direction, Judgment);
}