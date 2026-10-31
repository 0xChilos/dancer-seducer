#include "RhythmAudioSync.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Quartz/AudioMixerClockHandle.h"
#include "Quartz/QuartzSubsystem.h"
#include "Sound/SoundBase.h"
#include "HAL/PlatformTime.h"

bool URhythmAudioSync::Start(UObject* Context, USoundBase* Sound, double BPM)
{
    Stop();
    if (!Context || !Context->GetWorld() || !Sound || !FMath::IsFinite(BPM) || BPM <= 0.) return false;
    WorldContext = Context; Music = Sound; Tempo = BPM;
    if (!QueuePlayback()) { Stop(); return false; }
    return true;
}
bool URhythmAudioSync::QueuePlayback()
{
    UObject* Context = WorldContext;
    if (!Context || !Context->GetWorld() || !Music) return false;
    UQuartzSubsystem* Quartz = UQuartzSubsystem::Get(Context->GetWorld());
    if (!Quartz || !Quartz->IsQuartzEnabled()) return false;
    const FName ClockName(*FString::Printf(TEXT("Rhythm_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits)));
    Clock = Quartz->CreateNewClock(Context, ClockName, FQuartzClockSettings());
    Audio = UGameplayStatics::CreateSound2D(Context, Music, 1.f, 1.f, 0.f, nullptr, false, false);
    if (!Clock || !Audio) { ClearPlayback(); return false; }
    UQuartzClockHandle* Handle = Clock;
    FQuartzQuantizationBoundary Boundary(EQuartzCommandQuantization::Beat);
    // Reset and resume on the audio thread when the play command is queued.
    // This makes the first boundary and transport zero the same sample.
    Boundary.bResetClockOnQueued = true;
    Boundary.bResumeClockOnQueued = true;
    FOnQuartzCommandEventBP Delegate;
    Callback = NewObject<URhythmAudioSyncCallback>(this);
    Callback->Owner = this; Callback->Session = ++Session;
    Delegate.BindDynamic(Callback, &URhythmAudioSyncCallback::OnCommand);
    PendingSince = FPlatformTime::Seconds();
    Audio->PlayQuantized(Context, Handle, Boundary, Delegate);
    FQuartzQuantizationBoundary TempoBoundary(EQuartzCommandQuantization::Tick);
    Clock->SetBeatsPerMinute(Context, TempoBoundary, FOnQuartzCommandEventBP(), Handle, static_cast<float>(Tempo));
    return true;
}
void URhythmAudioSyncCallback::OnCommand(EQuartzCommandDelegateSubType Type, FName)
{
    if (URhythmAudioSync* Sync = Owner.Get()) Sync->OnCommand(Session, Type);
}
void URhythmAudioSync::OnCommand(uint64 InSession, EQuartzCommandDelegateSubType Type)
{
    if (InSession != Session) return;
    if (Type == EQuartzCommandDelegateSubType::CommandOnStarted)
    {
        bReady = true;
        // A queued command may resume the transport after a pause request.
        if (bPaused)
        {
            if (Audio) Audio->SetPaused(true);
            if (Clock && WorldContext) { UQuartzClockHandle* Handle = Clock; Clock->PauseClock(WorldContext, Handle); }
        }
    }
    else if (Type == EQuartzCommandDelegateSubType::CommandOnFailedToQueue || Type == EQuartzCommandDelegateSubType::CommandOnCanceled) bFailed = true;
}
bool URhythmAudioSync::HasFailed() const
{
    // A missing audio callback must never leave gameplay frozen indefinitely.
    return bFailed || (!bReady && !bPaused && PendingElapsed + FPlatformTime::Seconds() - PendingSince > 5.);
}
double URhythmAudioSync::GetSongTimeSeconds() const
{
    if (bPaused) return FrozenSongTime;
    return bReady && Clock && WorldContext ? FMath::Max(0.f, Clock->GetEstimatedRunTime(WorldContext)) : 0.;
}
void URhythmAudioSync::Pause()
{
    if (bPaused) return;
    FrozenSongTime = GetSongTimeSeconds();
    if (!bReady)
    {
        bPaused = true; bPendingRestart = true;
        ClearPlayback();
        return;
    }
    bPaused = true;
    if (Audio) Audio->SetPaused(true);
    if (Clock && WorldContext) { UQuartzClockHandle* Handle = Clock; Clock->PauseClock(WorldContext, Handle); }
}
void URhythmAudioSync::Resume()
{
    if (!bPaused) return;
    bPaused = false; PendingSince = FPlatformTime::Seconds();
    if (bPendingRestart)
    {
        bPendingRestart = false; PendingElapsed = 0.;
        if (!QueuePlayback()) bFailed = true;
        return;
    }
    if (Clock && WorldContext) { UQuartzClockHandle* Handle = Clock; Clock->ResumeClock(WorldContext, Handle); }
    if (Audio) Audio->SetPaused(false);
}
void URhythmAudioSync::ClearPlayback()
{
    ++Session; Callback = nullptr;
    if (Audio) { Audio->Stop(); Audio->DestroyComponent(); Audio = nullptr; }
    if (Clock && WorldContext && WorldContext->GetWorld())
    {
        UQuartzClockHandle* Handle = Clock;
        Clock->StopClock(WorldContext, true, Handle);
        if (UQuartzSubsystem* Quartz = UQuartzSubsystem::Get(WorldContext->GetWorld())) Quartz->DeleteClockByHandle(WorldContext, Handle);
    }
    Clock = nullptr;
}
void URhythmAudioSync::Stop()
{
    ClearPlayback();
    WorldContext = nullptr; Music = nullptr; Tempo = 0.;
    bReady = bFailed = bPaused = bPendingRestart = false; PendingSince = PendingElapsed = FrozenSongTime = 0.;
}

