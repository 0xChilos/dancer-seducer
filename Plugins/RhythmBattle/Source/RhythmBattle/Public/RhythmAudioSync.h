#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Sound/QuartzQuantizationUtilities.h"
#include "RhythmAudioSync.generated.h"
class UAudioComponent;
class UQuartzClockHandle;
class USoundBase;

class URhythmAudioSync;
UCLASS(Transient)
class RHYTHMBATTLE_API URhythmAudioSyncCallback : public UObject
{
    GENERATED_BODY()
public:
    TWeakObjectPtr<URhythmAudioSync> Owner;
    uint64 Session = 0;
    UFUNCTION() void OnCommand(EQuartzCommandDelegateSubType Type, FName Name);
};

// Owns one quantized music playback and its corresponding Quartz transport.
UCLASS(Transient)
class RHYTHMBATTLE_API URhythmAudioSync : public UObject
{
    GENERATED_BODY()
public:
    bool Start(UObject* Context, USoundBase* Sound, double BPM);
    void Pause();
    void Resume();
    void Stop();
    double GetSongTimeSeconds() const;
    bool IsReady() const { return bReady; }
    bool HasFailed() const;
    void OnCommand(uint64 Session, EQuartzCommandDelegateSubType Type);
private:
    bool QueuePlayback();
    void ClearPlayback();
    UPROPERTY(Transient) TObjectPtr<UObject> WorldContext;
    UPROPERTY(Transient) TObjectPtr<USoundBase> Music;
    UPROPERTY(Transient) TObjectPtr<URhythmAudioSyncCallback> Callback;
    UPROPERTY(Transient) TObjectPtr<UAudioComponent> Audio;
    UPROPERTY(Transient) TObjectPtr<UQuartzClockHandle> Clock;
    bool bReady = false, bFailed = false, bPaused = false, bPendingRestart = false;
    uint64 Session = 0;
    double Tempo = 0.;
    double PendingSince = 0., PendingElapsed = 0., FrozenSongTime = 0.;
};
