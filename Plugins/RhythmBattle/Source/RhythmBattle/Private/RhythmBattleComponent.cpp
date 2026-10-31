#include "RhythmBattleComponent.h"
#include "RhythmChart.h"
#include "RhythmBattleConfig.h"
#include "RhythmEventAction.h"
#include "RhythmHitJudge.h"
#include "RhythmScoreCalculator.h"
#include "RhythmAudioSync.h"
#include "Sound/SoundBase.h"

URhythmBattleComponent::URhythmBattleComponent() { PrimaryComponentTick.bCanEverTick = true; }
void URhythmBattleComponent::SetTimeProvider(FRhythmClock::FTimeProvider Provider)
{ if (State == ERhythmBattleState::Stopped) Clock = FRhythmClock(MoveTemp(Provider)); }
double URhythmBattleComponent::GetSongTimeSeconds() const { return AudioSync && AudioSync->IsReady() ? AudioSync->GetSongTimeSeconds() : Clock.GetSongTimeSeconds(); }
double URhythmBattleComponent::GetDurationSeconds() const { return Chart ? Chart->DurationSeconds : 0.; }
bool URhythmBattleComponent::StartBattle(URhythmChart* InChart, URhythmBattleConfig* InConfig, USoundBase* Music)
{
    if (!InChart || !InConfig || !FMath::IsFinite(InChart->BPM) || InChart->BPM <= 0. ||
        !FMath::IsFinite(InChart->DurationSeconds) || InChart->DurationSeconds <= 0. ||
        !FMath::IsFinite(InChart->SongOffsetSeconds) ||
        !FMath::IsFinite(InConfig->InputLatencyCompensation) ||
        !FMath::IsFinite(InConfig->BaseApproachDuration) || InConfig->BaseApproachDuration <= 0.f ||
        !FMath::IsFinite(InConfig->VisualSpeedMultiplier) || InConfig->VisualSpeedMultiplier <= 0.f ||
        !FMath::IsFinite(InConfig->ScoreMultiplier) || InConfig->ScoreMultiplier < 0.f ||
        !FMath::IsFinite(InConfig->HitWindowMultiplier) || InConfig->HitWindowMultiplier <= 0.f ||
        !FMath::IsFinite(InConfig->PerfectWindowSeconds) || InConfig->PerfectWindowSeconds < 0. ||
        !FMath::IsFinite(InConfig->GoodWindowSeconds) || InConfig->GoodWindowSeconds < InConfig->PerfectWindowSeconds ||
        !FMath::IsFinite(InConfig->OKWindowSeconds) || InConfig->OKWindowSeconds < InConfig->GoodWindowSeconds) return false;
    TSet<int64> Keys;
    TMap<int32, int32> Counts;
    for (const auto& Lane : InChart->Lanes)
    {
        if (Lane.PlayerIndex < 0 || Lane.PlayerIndex > 1 || Lane.LaneId < 0) return false;
        const int64 Key = (static_cast<int64>(Lane.PlayerIndex) << 32) | static_cast<uint32>(Lane.LaneId);
        if (Keys.Contains(Key) || ++Counts.FindOrAdd(Lane.PlayerIndex) > 6) return false;
        Keys.Add(Key);
    }
    for (const auto& N : InChart->Notes)
        if (N.PlayerIndex < 0 || N.PlayerIndex > 1 || N.LaneId < 0 || !FMath::IsFinite(N.Beat) || N.Beat < 0. || InChart->BeatToSeconds(N.Beat) > InChart->DurationSeconds || !Keys.Contains((static_cast<int64>(N.PlayerIndex) << 32) | static_cast<uint32>(N.LaneId))) return false;
    for (const auto& Cue : InChart->Events)
        if (!FMath::IsFinite(Cue.Beat) || Cue.Beat < 0.) return false;
    StopBattle();
    Chart = InChart; Config = InConfig; Lanes = Chart->Lanes;
    Modifiers = NewObject<URhythmModifierManager>(this);
    Scores.SetNum(2); Scores[0] = FRhythmPlayerScore(); Scores[1] = FRhythmPlayerScore(); Scores[1].PlayerIndex = 1;
    VisualSpeed = FMath::Max(.01f, Config->VisualSpeedMultiplier);
    ScoreMultiplier = FMath::Max(0.f, Config->ScoreMultiplier);
    WindowMultiplier = FMath::Max(.01f, Config->HitWindowMultiplier);
    bSpawning = true; FiredEvents.Reset(); Scheduler.Initialize(*Chart);
    Clock.Start(); LastVisualTime = 0.; State = ERhythmBattleState::Playing;
    if (Music && GetWorld())
    {
        AudioSync = NewObject<URhythmAudioSync>(this);
        if (AudioSync->Start(this, Music, Chart->BPM)) Clock.Pause();
        else AudioSync = nullptr;
    }
    UpdateBattle();
    return true;
}
void URhythmBattleComponent::StopBattle()
{
    ++Generation;
    State = ERhythmBattleState::Stopped; Clock.Stop();
    if (AudioSync) { AudioSync->Stop(); AudioSync = nullptr; }
    const auto Actions = RunningActions; RunningActions.Reset();
    PauseReasons.Reset(); Scheduler.Reset(); FiredEvents.Reset();
    if (Modifiers) Modifiers->Reset();
    for (const auto& Action : Actions) if (Action) Action->Cancel(this);
}
void URhythmBattleComponent::EndPlay(const EEndPlayReason::Type Reason) { StopBattle(); Super::EndPlay(Reason); }
void URhythmBattleComponent::PauseRhythm(FName Reason)
{
    if (Reason.IsNone() || (State != ERhythmBattleState::Playing && State != ERhythmBattleState::Paused)) return;
    PauseReasons.Add(Reason);
    if (State == ERhythmBattleState::Playing)
    { Clock.Pause(); State = ERhythmBattleState::Paused; if (AudioSync) AudioSync->Pause(); OnRhythmPaused.Broadcast(Reason); }
}
void URhythmBattleComponent::ResumeRhythm(FName Reason)
{
    if (State != ERhythmBattleState::Paused || !PauseReasons.Remove(Reason) || !PauseReasons.IsEmpty()) return;
    if (!AudioSync || AudioSync->IsReady() || AudioSync->HasFailed()) Clock.Resume(); State = ERhythmBattleState::Playing; if (AudioSync) AudioSync->Resume(); OnRhythmResumed.Broadcast(Reason);
}
void URhythmBattleComponent::StopSpawning(bool bStop) { bSpawning = !bStop; }
void URhythmBattleComponent::SetVisualSpeed(float V) { if (FMath::IsFinite(V) && V > 0.f) VisualSpeed = V; }
void URhythmBattleComponent::SetScoreMultiplier(float V) { if (FMath::IsFinite(V) && V >= 0.f) ScoreMultiplier = V; }
void URhythmBattleComponent::SetHitWindowMultiplier(float V) { if (FMath::IsFinite(V) && V > 0.f) WindowMultiplier = V; }
float URhythmBattleComponent::GetEffectiveModifier(ERhythmModifierType Type, int32 P, int32 L) const
{
    const float Base = Type == ERhythmModifierType::VisualSpeed ? VisualSpeed : Type == ERhythmModifierType::Score ? ScoreMultiplier : Type == ERhythmModifierType::HitWindow ? WindowMultiplier : 1.f;
    return Base * (Modifiers ? Modifiers->Evaluate(Type, P, L) : Type == ERhythmModifierType::LaneDisable ? 0.f : 1.f);
}
bool URhythmBattleComponent::IsLaneEnabled(int32 P, int32 L) const
{
    const auto* Lane = Lanes.FindByPredicate([P,L](const auto& X) { return X.PlayerIndex == P && X.LaneId == L; });
    return Lane && Lane->bEnabled && GetEffectiveModifier(ERhythmModifierType::LaneDisable, P, L) == 0.f;
}
void URhythmBattleComponent::EnableLane(int32 P, int32 L, bool bEnabled)
{
    for (auto& Lane : Lanes) if (Lane.PlayerIndex == P && Lane.LaneId == L) Lane.bEnabled = bEnabled;
    if (!bEnabled && Config) Scheduler.SkipLane(P, L, GetSongTimeSeconds(), Config->OKWindowSeconds * GetEffectiveModifier(ERhythmModifierType::HitWindow,P,L));
}
bool URhythmBattleComponent::AddTimedModifier(const FRhythmModifierSpec& S)
{ return Modifiers && Chart && Modifiers->Add(S, GetSongTimeSeconds(), 60. / Chart->BPM); }
bool URhythmBattleComponent::RemoveModifier(FName Id) { return Modifiers && Modifiers->RemoveModifier(Id); }
void URhythmBattleComponent::TriggerRhythmEvent(FName Id, URhythmEventAction* Action)
{
    const uint64 Before = Generation;
    OnRhythmEventTriggered.Broadcast(Id);
    if (Before != Generation || State == ERhythmBattleState::Stopped || State == ERhythmBattleState::Finished) return;
    if (Action) { auto* Instance = DuplicateObject<URhythmEventAction>(Action, this); RunningActions.Add(Instance); Instance->Execute(this, Id); }
}
void URhythmBattleComponent::ApplyResult(FRhythmHitResult& R)
{
    if (!Scores.IsValidIndex(R.PlayerIndex) || !Config) return;
    FRhythmScoreCalculator::Apply(Scores[R.PlayerIndex], R, *Config, GetEffectiveModifier(ERhythmModifierType::Score,R.PlayerIndex,R.LaneId));
    const FRhythmPlayerScore Score = Scores[R.PlayerIndex];
    const uint64 Before = Generation;
    if (R.Judgment == ERhythmJudgment::Miss) OnNoteMiss.Broadcast(R); else OnNoteHit.Broadcast(R);
    if (Before != Generation) return;
    OnScoreChanged.Broadcast(Score);
    if (Before == Generation) OnComboChanged.Broadcast(Score);
}
FRhythmHitResult URhythmBattleComponent::SubmitLaneInput(int32 P, int32 L)
{
    FRhythmHitResult R;
    if (State != ERhythmBattleState::Playing || !Config) return R;
    const uint64 Before = Generation;
    UpdateBattle();
    if (Before != Generation || State != ERhythmBattleState::Playing || (AudioSync && !AudioSync->IsReady()) || !IsLaneEnabled(P,L)) return R;
    const double Time = GetSongTimeSeconds() - Config->InputLatencyCompensation;
    const float Window = GetEffectiveModifier(ERhythmModifierType::HitWindow,P,L);
    const int32 Index = FRhythmHitJudge::FindClosestNote(Scheduler.GetNotes(),P,L,Time,Config->OKWindowSeconds*Window);
    R = FRhythmHitJudge::Resolve(Scheduler.GetNotes(),Index,Time,*Config,Window);
    if (R.NoteId != INDEX_NONE) ApplyResult(R);
    return R;
}
void URhythmBattleComponent::TickComponent(float Dt, ELevelTick T, FActorComponentTickFunction* F)
{ Super::TickComponent(Dt,T,F); UpdateBattle(); }
void URhythmBattleComponent::UpdateBattle()
{
    if (State != ERhythmBattleState::Playing || !Chart || !Config) return;
    if (AudioSync && AudioSync->HasFailed())
    {
        const double FallbackTime = AudioSync->GetSongTimeSeconds();
        AudioSync->Stop(); AudioSync = nullptr; Clock.Start(FallbackTime);
    }
    else if (AudioSync && !AudioSync->IsReady()) return;
    else if (AudioSync && Clock.IsPaused()) Clock.Resume();
    const double Time = GetSongTimeSeconds();
    const uint64 Before = Generation;
    Modifiers->Expire(Time);
    // Mark before invoking Blueprint: callbacks may pause, stop, or restart the battle.
    const auto Cues = Chart->Events;
    for (int32 I = 0; I < Cues.Num(); ++I)
        if (!FiredEvents.Contains(I) && Chart->BeatToSeconds(Cues[I].Beat) <= Time)
        { FiredEvents.Add(I); TriggerRhythmEvent(Cues[I].EventId,Cues[I].Action); if (Before != Generation || State != ERhythmBattleState::Playing) return; }
    const double Dt = FMath::Max(0., Time - LastVisualTime);
    LastVisualTime = Time;
    TArray<FRhythmHitResult> Misses;
    for (auto& N : Scheduler.GetNotes())
    {
        if (N.Judgment != ERhythmJudgment::None) continue;
        const double Window = Config->OKWindowSeconds * GetEffectiveModifier(ERhythmModifierType::HitWindow,N.PlayerIndex,N.LaneId);
        if (!IsLaneEnabled(N.PlayerIndex,N.LaneId) && N.HitTimeSeconds <= Time + Window)
        { N.Judgment = ERhythmJudgment::Skipped; N.State = ERhythmNoteState::Skipped; continue; }
        const auto* Lane = Lanes.FindByPredicate([&N](const auto& X) { return X.PlayerIndex == N.PlayerIndex && X.LaneId == N.LaneId; });
        const double Duration = FMath::Max(.01f,Lane && Lane->ApproachDurationOverride > 0.f ? Lane->ApproachDurationOverride : Config->BaseApproachDuration);
        const double Speed = GetEffectiveModifier(ERhythmModifierType::VisualSpeed,N.PlayerIndex,N.LaneId);
        if (!N.bSpawned && !bSpawning && Time >= N.HitTimeSeconds-Duration/Speed)
        { N.Judgment = ERhythmJudgment::Skipped; N.State = ERhythmNoteState::Skipped; continue; }
        double VisualDt = Dt;
        if (!N.bSpawned && bSpawning && Time >= N.HitTimeSeconds-Duration/Speed)
        { N.bSpawned = true; N.State = ERhythmNoteState::Active; N.Progress = 0.f; VisualDt = FMath::Min(Dt,FMath::Max(0.,Time-(N.HitTimeSeconds-Duration/Speed))); }
        if (N.bSpawned)
        {
            // Continuous remaining-distance integration. A speed change changes velocity only;
            // the authored target time is immutable and always reaches progress 1 at that time.
            const double RemainingBefore = N.HitTimeSeconds - (Time-VisualDt);
            N.Progress = Time >= N.HitTimeSeconds ? 1.f : FMath::Clamp(1.-(1.-N.Progress)*FMath::Pow(FMath::Max(0.,(N.HitTimeSeconds-Time)/FMath::Max(.000001,RemainingBefore)),Speed),0.,1.);
        }
        if (Time > N.HitTimeSeconds + Window + FMath::Max(0.,Config->InputLatencyCompensation))
        {
            N.Judgment = ERhythmJudgment::Miss; N.State = ERhythmNoteState::Missed;
            FRhythmHitResult& R = Misses.AddDefaulted_GetRef(); R.NoteId = N.NoteId; R.PlayerIndex = N.PlayerIndex; R.LaneId = N.LaneId; R.Judgment = ERhythmJudgment::Miss; R.TimingErrorSeconds = Time-N.HitTimeSeconds;
        }
    }
    for (auto& R : Misses) { ApplyResult(R); if (Before != Generation || State != ERhythmBattleState::Playing) return; }
    if (Time > Chart->DurationSeconds)
    {
        bool bUnresolved = false;
        for (const auto& N : Scheduler.GetNotes()) bUnresolved |= N.Judgment == ERhythmJudgment::None;
        if (!bUnresolved)
        {
            Clock.Pause(); State = ERhythmBattleState::Finished;
            if (AudioSync) AudioSync->Pause();
            Modifiers->Reset(); PauseReasons.Reset();
            const auto Actions = RunningActions; RunningActions.Reset();
            for (const auto& Action : Actions)
            {
                if (Action) Action->Complete(this);
                if (Before != Generation) return;
            }
            OnBattleFinished.Broadcast();
        }
    }
}
