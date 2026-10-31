#pragma once
#include "CoreMinimal.h"

// Monotonic wall time is injected to permit audio onset alignment and deterministic tests.
class RHYTHMBATTLE_API FRhythmClock
{
public:
    using FTimeProvider = TFunction<double()>;
    explicit FRhythmClock(FTimeProvider InProvider = {});
    void Start(double SongTime = 0.);
    void Stop();
    void Pause();
    void Resume();
    double GetSongTimeSeconds() const;
    double MapTimestampToSongTime(double MonotonicTimestamp) const;
    bool IsPaused() const { return bPaused; }
private:
    FTimeProvider Provider;
    double Origin = 0.;
    double FrozenSongTime = 0.;
    bool bRunning = false;
    bool bPaused = false;
};
