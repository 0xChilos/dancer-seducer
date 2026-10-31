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
        for (int32 P = 0; P < 2; ++P)
            for (int32 L = 0; L < 2; ++L)
            {
                FRhythmLaneLayout Lane;
                Lane.PlayerIndex = P; Lane.LaneId = L;
                const float X = P == 0 ? .25f : .75f;
                Lane.Start = FVector2D(X + (L == 0 ? -.17f : .17f), .85f);
                Lane.HitTarget = FVector2D(X + (L == 0 ? -.055f : .055f), .65f);
                Lane.Color = L == 0 ? FLinearColor(1.f,.4f,.65f) : FLinearColor(.9f,1.f,.3f);
                DemoChart->Lanes.Add(Lane);
            }
        for (int32 Beat = 4; Beat < 58; ++Beat)
            for (int32 P = 0; P < 2; ++P)
            { FRhythmNoteDefinition N; N.PlayerIndex=P; N.LaneId=(Beat+P)%2; N.Beat=Beat; DemoChart->Notes.Add(N); }
        for (const auto& Pair : TArray<TPair<FName,double>>{{TEXT("Taunt"),16.},{TEXT("SpeedBoost"),32.},{TEXT("LaneBreak"),48.}})
        { FRhythmEventCue Cue; Cue.EventId=Pair.Key; Cue.Beat=Pair.Value; DemoChart->Events.Add(Cue); }
    }
    bComboBoostUsed = false; TauntResumeAt = 0.;
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
            Battle->SubmitLaneInput(1,Note.LaneId);
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
    const FKey Keyboard[]={EKeys::A,EKeys::D,EKeys::J,EKeys::L,EKeys::R,EKeys::P};
    const FKey Pad[]={EKeys::Gamepad_FaceButton_Left,EKeys::Gamepad_FaceButton_Bottom,EKeys::Gamepad_LeftShoulder,EKeys::Gamepad_RightShoulder,EKeys::Gamepad_Special_Right,EKeys::Gamepad_Special_Left};
    for (int32 I=0;I<6;++I)
    {
        auto* Action=NewObject<UInputAction>(this); Action->ValueType=EInputActionValueType::Boolean; Actions.Add(Action);
        Mapping->MapKey(Action,Keyboard[I]); Mapping->MapKey(Action,Pad[I]);
        if (I<4 && !AuthoredMapping) Input->BindAction(Action,ETriggerEvent::Started,this,&ARhythmDemoPlayerController::PressLane,I/2,I%2);
        else if (I==4) Input->BindAction(Action,ETriggerEvent::Started,this,&ARhythmDemoPlayerController::Restart);
        else if (I==5) Input->BindAction(Action,ETriggerEvent::Started,this,&ARhythmDemoPlayerController::TogglePause);
    }
    if (AuthoredMapping)
    {
        if (auto* GM = GetWorld()->GetAuthGameMode<ARhythmDemoGameMode>())
            if (GM->DemoChart)
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
{ if (auto* GM=GetWorld()->GetAuthGameMode<ARhythmDemoGameMode>()) GM->Battle->SubmitLaneInput(PlayerIndex,Lane); }
void ARhythmDemoPlayerController::Restart()
{ if (auto* GM=GetWorld()->GetAuthGameMode<ARhythmDemoGameMode>()) GM->RestartBattle(); }
void ARhythmDemoPlayerController::TogglePause()
{
    if (auto* GM=GetWorld()->GetAuthGameMode<ARhythmDemoGameMode>())
    { if (GM->Battle->GetBattleState()==ERhythmBattleState::Paused) GM->Battle->ResumeRhythm(TEXT("PlayerPause")); else GM->Battle->PauseRhythm(TEXT("PlayerPause")); }
}
