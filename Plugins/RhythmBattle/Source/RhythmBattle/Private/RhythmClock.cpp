#include "RhythmClock.h"
#include "HAL/PlatformTime.h"

FRhythmClock::FRhythmClock(FTimeProvider InProvider) : Provider(MoveTemp(InProvider))
{
    if (!Provider) Provider = [] { return FPlatformTime::Seconds(); };
}
void FRhythmClock::Start(double SongTime) { Origin = Provider() - SongTime; FrozenSongTime = SongTime; bRunning = true; bPaused = false; }
void FRhythmClock::Stop() { FrozenSongTime = 0.; bRunning = false; bPaused = false; }
void FRhythmClock::Pause() { if (bRunning && !bPaused) { FrozenSongTime = GetSongTimeSeconds(); bPaused = true; } }
void FRhythmClock::Resume() { if (bRunning && bPaused) { Origin = Provider() - FrozenSongTime; bPaused = false; } }
double FRhythmClock::GetSongTimeSeconds() const { return MapTimestampToSongTime(Provider()); }
double FRhythmClock::MapTimestampToSongTime(double Timestamp) const { return bRunning && !bPaused ? Timestamp - Origin : FrozenSongTime; }
