#include "RhythmDemoGameMode.h"
#include "RhythmBattleComponent.h"
#include "RhythmBattleConfig.h"
#include "RhythmChart.h"
#include "RhythmBattleWidget.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputCoreTypes.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Camera/CameraActor.h"
#include "HAL/PlatformTime.h"
#include "Sound/SoundWaveProcedural.h"

ARhythmDemoGameMode::ARhythmDemoGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
    PlayerControllerClass = ARhythmDemoPlayerController::StaticClass();
    DefaultPawnClass = nullptr;
    Battle = CreateDefaultSubobject<URhythmBattleComponent>(TEXT("RhythmBattle"));
}
void ARhythmDemoGameMode::BeginPlay()
{
    Super::BeginPlay();
    Battle->OnRhythmEventTriggered.AddDynamic(this, &ARhythmDemoGameMode::HandleEvent);
    Battle->OnScoreChanged.AddDynamic(this, &ARhythmDemoGameMode::HandleScore);
    RestartBattle();
}
void ARhythmDemoGameMode::RestartBattle()
{
    if (!DemoConfig) DemoConfig = NewObject<URhythmBattleConfig>(this);
    if (!DemoChart)
    {
        DemoChart = NewObject<URhythmChart>(this);
        // 3 lanes per side (6 total): upper (L0), middle (L1), lower (L2)
        // Left side (P0): W=upper, A=middle, S=lower
        // Right side (P1): Up=upper, Right=middle, Down=lower
        for (int32 P = 0; P < 2; ++P)
            for (int32 L = 0; L < 3; ++L)
            {
                FRhythmLaneLayout Lane;
                Lane.PlayerIndex = P; Lane.LaneId = L;
                const float Y = (P == 0 ? .54f : .60f) + L * .14f;
                Lane.Start = FVector2D(P == 0 ? .06f : .94f, Y);
                Lane.HitTarget = FVector2D(P == 0 ? .30f : .70f, Y);
                // Colors: upper=pink, middle=cyan, lower=green
                if (L == 0) Lane.Color = FLinearColor(1.f, .4f, .65f);
                else if (L == 1) Lane.Color = FLinearColor(.3f, 1.f, .9f);
                else Lane.Color = FLinearColor(.4f, 1.f, .5f);
                DemoChart->Lanes.Add(Lane);
            }
        for (int32 Beat = 4; Beat < 58; ++Beat)
            for (int32 P = 0; P < 2; ++P)
            { FRhythmNoteDefinition N; N.PlayerIndex=P; N.LaneId=(Beat+P)%3; N.Beat=Beat; DemoChart->Notes.Add(N); }
        for (const auto& Pair : TArray<TPair<FName,double>>{{TEXT("Taunt"),16.},{TEXT("SpeedBoost"),32.},{TEXT("LaneBreak"),48.}})
        { FRhythmEventCue Cue; Cue.EventId=Pair.Key; Cue.Beat=Pair.Value; DemoChart->Events.Add(Cue); }
    }
    bComboBoostUsed = false; TauntResumeAt = 0.;
    ActiveLanes.Reset();
    ActiveLanes.Add(0); // Start with upper lane for both players
    ActiveLanes.Add(0);
    USoundBase* Music = DemoMusic;
    if (!Music && FMath::IsFinite(DemoChart->DurationSeconds) && DemoChart->DurationSeconds > 0.0 &&
        DemoChart->DurationSeconds <= 120.0 && FMath::IsFinite(DemoChart->BPM) && DemoChart->BPM >= 1.0 && DemoChart->BPM <= 1000.0 &&
        FMath::IsFinite(DemoChart->SongOffsetSeconds) && FMath::Abs(DemoChart->SongOffsetSeconds) <= 120.0)
    {
        constexpr int32 SampleRate = 24000;
        USoundWaveProcedural* Click = NewObject<USoundWaveProcedural>(this);
        Click->SetSampleRate(SampleRate);
        Click->NumChannels = 1;
        Click->Duration = static_cast<float>(DemoChart->DurationSeconds);
        const int32 SampleCount = FMath::Max(0, FMath::RoundToInt(DemoChart->DurationSeconds * SampleRate));
        TArray<int16> Samples;
        Samples.SetNumZeroed(SampleCount);
        const double BeatSeconds = 60.0 / FMath::Max(DemoChart->BPM, 1.0);
        for (double BeatTime = DemoChart->SongOffsetSeconds + FMath::Max(0.0, FMath::CeilToDouble(-DemoChart->SongOffsetSeconds / BeatSeconds)) * BeatSeconds; BeatTime < DemoChart->DurationSeconds; BeatTime += BeatSeconds)
        {
            const int32 Start = FMath::RoundToInt(BeatTime * SampleRate);
            for (int32 I = 0; I < SampleRate / 25 && Start + I < SampleCount; ++I)
            {
                if (Start + I >= 0)
                {
                    const double Envelope = 1.0 - static_cast<double>(I) / (SampleRate / 25);
                    Samples[Start + I] = static_cast<int16>(12000.0 * Envelope * FMath::Sin(2.0 * PI * 880.0 * I / SampleRate));
                }
            }
        }
        Click->QueueAudio(reinterpret_cast<const uint8*>(Samples.GetData()), Samples.Num() * sizeof(int16));
        Music = Click;
    }
    Battle->StartBattle(DemoChart, DemoConfig, Music);
}
void ARhythmDemoGameMode::HandleEvent(FName Id)
{
    if (Id == TEXT("Taunt"))
    { Battle->PauseRhythm(Id); TauntResumeAt = FPlatformTime::Seconds()+1.5; }
    if (Id == TEXT("SpeedBoost") || Id == TEXT("LaneBreak"))
    {
        FRhythmModifierSpec M; M.Id=Id; M.bDurationInBeats=true;
        M.Type=Id == TEXT("SpeedBoost") ? ERhythmModifierType::VisualSpeed : ERhythmModifierType::LaneDisable;
        M.Value=Id == TEXT("SpeedBoost") ? 1.35f : 1.f;
        M.DurationSeconds=Id == TEXT("SpeedBoost") ? 8. : 2.;
        if (Id == TEXT("LaneBreak")) { M.Scope=ERhythmModifierScope::Lane; M.PlayerIndex=1; M.LaneId=0; }
        Battle->AddTimedModifier(M);
    }
}
void ARhythmDemoGameMode::HandleScore(const FRhythmPlayerScore& Score)
{
    if (Score.PlayerIndex == 0 && Score.Combo >= 20 && !bComboBoostUsed)
    {
        bComboBoostUsed=true;
        FRhythmModifierSpec M; M.Id=TEXT("ComboBoost"); M.Type=ERhythmModifierType::Score;
        M.Scope=ERhythmModifierScope::Player; M.PlayerIndex=0; M.Value=2; M.DurationSeconds=4; M.bDurationInBeats=true;
        Battle->AddTimedModifier(M);
    }
}
void ARhythmDemoGameMode::Tick(float Dt)
{
    Super::Tick(Dt);
    if (TauntResumeAt > 0. && FPlatformTime::Seconds() >= TauntResumeAt)
    { TauntResumeAt=0.; Battle->ResumeRhythm(TEXT("Taunt")); }
    if (!bOpponentAutoplay || Battle->GetBattleState()!=ERhythmBattleState::Playing) return;
    const double Now=Battle->GetSongTimeSeconds();
    for (const auto& Note : Battle->GetNoteSnapshot())
        if (Note.PlayerIndex==1 && Note.Judgment==ERhythmJudgment::None && Note.HitTimeSeconds<=Now)
        {
            SetActiveLane(1, Note.LaneId);
            Battle->SubmitLaneInput(1,Note.LaneId);
        }
}
void ARhythmDemoPlayerController::BeginPlay()
{
    Super::BeginPlay(); SetInputMode(FInputModeGameOnly());
    for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
        if (It->ActorHasTag(TEXT("DanceBattleCamera"))) { SetViewTarget(*It); break; }
    if (HUDClass) HUD=CreateWidget<URhythmBattleWidget>(this, HUDClass);
    if (HUD) { HUD->AddToViewport(); if (auto* GM=GetWorld()->GetAuthGameMode<ARhythmDemoGameMode>()) HUD->SetBattleComponent(GM->Battle); }
}
ARhythmDemoPlayerController::ARhythmDemoPlayerController()
{
    HUDClass = URhythmBattleWidget::StaticClass();
}
void ARhythmDemoPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    auto* Input=Cast<UEnhancedInputComponent>(InputComponent); if (!Input) return;
    Mapping=NewObject<UInputMappingContext>(this);
    const ARhythmDemoGameMode* GM = GetWorld()->GetAuthGameMode<ARhythmDemoGameMode>();
    // 6 lane keys: W/A/S (left) + Up/Right/Down (right) + R (restart) + P (pause)
    const FKey Keyboard[]={EKeys::W,EKeys::A,EKeys::S,EKeys::Up,EKeys::Right,EKeys::Down,EKeys::R,EKeys::P};
    const FKey Pad[]={EKeys::Gamepad_FaceButton_Left,EKeys::Gamepad_FaceButton_Bottom,EKeys::Gamepad_FaceButton_Right,EKeys::Gamepad_LeftShoulder,EKeys::Gamepad_RightShoulder,EKeys::Gamepad_RightTrigger,EKeys::Gamepad_Special_Right,EKeys::Gamepad_Special_Left};
    for (int32 I=0;I<8;++I)
    {
        const int32 PlayerIndex = I < 3 ? 0 : 1;
        const int32 Lane = I < 3 ? I : I - 3;
        bool bHasAuthoredAction = false;
        if (I < 6 && AuthoredMapping && GM && GM->DemoChart)
            for (const FRhythmLaneLayout& Layout : GM->DemoChart->Lanes)
                if (Layout.PlayerIndex == PlayerIndex && Layout.LaneId == Lane && Layout.InputAction)
                    bHasAuthoredAction = true;
        if (bHasAuthoredAction) continue;
        auto* Action=NewObject<UInputAction>(this); Action->ValueType=EInputActionValueType::Boolean; Actions.Add(Action);
        Mapping->MapKey(Action,Keyboard[I]); Mapping->MapKey(Action,Pad[I]);
        if (I<6)
        {
            // Lane keys: I=0,1,2 (W,A,S) -> Player 0, lanes 0,1,2
            // I=3,4,5 (Up,Right,Down) -> Player 1, lanes 0,1,2
            Input->BindAction(Action,ETriggerEvent::Started,this,&ARhythmDemoPlayerController::PressLane,PlayerIndex,Lane);
        }
        else if (I==6) Input->BindAction(Action,ETriggerEvent::Started,this,&ARhythmDemoPlayerController::Restart);
        else if (I==7) Input->BindAction(Action,ETriggerEvent::Started,this,&ARhythmDemoPlayerController::TogglePause);
    }
    if (AuthoredMapping)
    {
        if (GM && GM->DemoChart)
            for (const FRhythmLaneLayout& Lane : GM->DemoChart->Lanes)
                if (Lane.InputAction)
                    Input->BindAction(Lane.InputAction, ETriggerEvent::Started, this, &ARhythmDemoPlayerController::PressLane, Lane.PlayerIndex, Lane.LaneId);
    }
    if (GetLocalPlayer()) if (auto* Sub=GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
    {
        Sub->AddMappingContext(Mapping,10);
        if (AuthoredMapping) Sub->AddMappingContext(AuthoredMapping, 11);
    }
}
void ARhythmDemoPlayerController::PressLane(int32 PlayerIndex,int32 Lane)
{
    if (auto* GM=GetWorld()->GetAuthGameMode<ARhythmDemoGameMode>())
    {
        GM->SetActiveLane(PlayerIndex, Lane);
        GM->Battle->SubmitLaneInput(PlayerIndex, Lane);
    }
}

void ARhythmDemoGameMode::SetActiveLane(int32 PlayerIndex, int32 Lane)
{
    if (PlayerIndex >= 0 && PlayerIndex < ActiveLanes.Num() && Lane >= 0 && Lane < 3)
        ActiveLanes[PlayerIndex] = Lane;
}

int32 ARhythmDemoGameMode::GetActiveLane(int32 PlayerIndex) const
{
    if (PlayerIndex < 0 || PlayerIndex > 1 || PlayerIndex >= ActiveLanes.Num()) return 0;
    return ActiveLanes[PlayerIndex];
}
void ARhythmDemoPlayerController::Restart()
{ if (auto* GM=GetWorld()->GetAuthGameMode<ARhythmDemoGameMode>()) GM->RestartBattle(); }
void ARhythmDemoPlayerController::TogglePause()
{
    if (auto* GM=GetWorld()->GetAuthGameMode<ARhythmDemoGameMode>())
    { if (GM->Battle->GetBattleState()==ERhythmBattleState::Paused) GM->Battle->ResumeRhythm(TEXT("PlayerPause")); else GM->Battle->PauseRhythm(TEXT("PlayerPause")); }
}
