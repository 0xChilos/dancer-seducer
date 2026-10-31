#include "DanceRhythmComponent.h"
#include "Math/UnrealMathUtility.h"

void UDanceRhythmComponent::Reset(const FDanceSettings& InSettings)
{
	Generation++;
	Elapsed = 0.0f;
	bRunning = false;
	Notes.Empty();
	Settings = InSettings;
	Settings.RoundDuration = FMath::Max(Settings.RoundDuration, 1.0f);
	Settings.BeatInterval = FMath::Max(Settings.BeatInterval, 0.1f);
	Settings.PerfectWindow = FMath::Max(Settings.PerfectWindow, 0.01f);
	Settings.GoodWindow = FMath::Max(Settings.GoodWindow, Settings.PerfectWindow + 0.01f);
	Settings.TravelTime = FMath::Max(Settings.TravelTime, 0.0f);
	GenerateChart();
	bRunning = true;
}

void UDanceRhythmComponent::GenerateChart()
{
	FRandomStream Stream(Settings.Seed);
	int32 NoteIdCounter = 0;
	float CurrentBeatTime = Settings.TravelTime;
	const float LastValidHitTime = Settings.RoundDuration - Settings.GoodWindow;

	while (CurrentBeatTime <= LastValidHitTime)
	{
		for (int32 i = 0; i < 2; ++i)
		{
			EDanceSide Side = (i == 0) ? EDanceSide::Player : EDanceSide::AI;
			EDanceDirection Dir = (EDanceDirection)Stream.RandRange(0, 3);
			FDanceNote Note;
			Note.NoteId = NoteIdCounter++;
			Note.Side = Side;
			Note.Direction = Dir;
			Note.HitTime = CurrentBeatTime;
			Note.SpawnTime = FMath::Max(0.0f, Note.HitTime - Settings.TravelTime);
			Note.Judgment = EDanceJudgment::Pending;
			Note.bSpawned = false;
			Notes.Add(Note);
		}
		CurrentBeatTime += Settings.BeatInterval;
	}
}

void UDanceRhythmComponent::Advance(float NewElapsed)
{
	if (!bRunning) return;

	Elapsed = FMath::Max(Elapsed, NewElapsed);
	int32 CurrentGen = Generation;

	TArray<FDanceNote> SpawnEvents;
	TArray<FDanceNote> JudgeEvents;

	for (FDanceNote& Note : Notes)
	{
		if (!Note.bSpawned && Elapsed >= Note.SpawnTime)
		{
			Note.bSpawned = true;
			SpawnEvents.Add(Note);
		}

		if (Note.Judgment == EDanceJudgment::Pending && (Elapsed - Note.HitTime) > Settings.GoodWindow + 1e-5f)
		{
			Note.Judgment = EDanceJudgment::Miss;
			JudgeEvents.Add(Note);
		}
	}

	for (auto& N : SpawnEvents) {
    OnSpawned.Broadcast(N);
    if (CurrentGen != Generation) return;
}
for (auto& N : JudgeEvents) {
    OnJudged.Broadcast(N);
    if (CurrentGen != Generation) return;
}

	if (CurrentGen != Generation) return;
}

EDanceJudgment UDanceRhythmComponent::Attempt(EDanceSide Side, EDanceDirection Direction, float AtTime, int32 ExpectedNoteId)
{
	if (!bRunning) return EDanceJudgment::Miss;
	int32 CurrentGen = Generation;

	int32 BestMatchIdx = INDEX_NONE;
	float MinMatchDist = TNumericLimits<float>::Max();
	int32 BestAnyIdx = INDEX_NONE;
	float MinAnyDist = TNumericLimits<float>::Max();

	for (int32 i = 0; i < Notes.Num(); ++i)
	{
		FDanceNote& Note = Notes[i];
		if (Note.Judgment != EDanceJudgment::Pending || Note.Side != Side) continue;

		float Dist = FMath::Abs(AtTime - Note.HitTime);
		if (Dist > Settings.GoodWindow + 1e-5f) continue;

		if (ExpectedNoteId != INDEX_NONE && Note.NoteId != ExpectedNoteId) continue;

		if (Note.Direction == Direction)
		{
			if (Dist < MinMatchDist)
			{
				MinMatchDist = Dist;
				BestMatchIdx = i;
			}
		}

		if (Dist < MinAnyDist)
		{
			MinAnyDist = Dist;
			BestAnyIdx = i;
		}
	}

	int32 TargetIdx = (BestMatchIdx != INDEX_NONE) ? BestMatchIdx : BestAnyIdx;
	EDanceJudgment Result = EDanceJudgment::Miss;

	if (TargetIdx != INDEX_NONE)
	{
		FDanceNote TargetCopy = Notes[TargetIdx];
		if (TargetCopy.Direction == Direction)
		{
			Result = (MinMatchDist <= Settings.PerfectWindow + 1e-5f) ? EDanceJudgment::Perfect : EDanceJudgment::Good;
		}
		Notes[TargetIdx].Judgment = Result;
		TargetCopy.Judgment = Result;
		OnJudged.Broadcast(TargetCopy);
		if (CurrentGen != Generation) return Result;
	}

	OnAttempt.Broadcast(Side, Direction, Result);
	if (CurrentGen != Generation) return EDanceJudgment::Pending;

	return Result;
}

void UDanceRhythmComponent::Finish()
{
	bRunning = false;
	int32 CurrentGen = Generation;
	TArray<FDanceNote> PendingNotes;

	for (FDanceNote& Note : Notes)
	{
		if (Note.Judgment == EDanceJudgment::Pending)
		{
			Note.Judgment = EDanceJudgment::Miss;
			PendingNotes.Add(Note);
		}
	}

	for (auto& N : PendingNotes) {
    OnJudged.Broadcast(N);
    if (CurrentGen != Generation) return;
}
	if (CurrentGen != Generation) return;
}