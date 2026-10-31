#include "DanceAIOpponentComponent.h"

void UDanceAIOpponentComponent::Reset(UDanceRhythmComponent* InRhythm)
{
	Rhythm = InRhythm;
	Actions.Empty();
	NextAction = 0;

	if (!Rhythm)
	{
		return;
	}

	Generation = Rhythm->Generation;
	FRandomStream Stream(Rhythm->Settings.Seed + 997);

	// Pre-calculate one action for every note assigned to the AI
	for (const FDanceNote& Note : Rhythm->Notes)
	{
		if (Note.Side == EDanceSide::AI)
		{
			float Offset = 0.0f;
			const float Accuracy = FMath::Clamp(Rhythm->Settings.AIAccuracy, 0.0f, 1.0f);
			const float GoodWin = Rhythm->Settings.GoodWindow;
			const float Spread = Rhythm->Settings.AITimingSpread;

			// Consistent probability mechanism:
			// With probability AIAccuracy, sample uniform offset [-GoodWindow, GoodWindow]
			// Else, sample signed magnitude GoodWindow + 0.01 + random spread
			if (Stream.FRand() < Accuracy)
			{
				// Uniformly distributed within the "Good" window
				Offset = Stream.FRandRange(-GoodWin, GoodWin);
			}
			else
			{
				// Outside the window: GoodWindow + 0.01 + [0, max(.01, Spread)]
				float Magnitude = GoodWin + 0.01f + Stream.FRand() * FMath::Max(0.01f, Spread);
				Offset = (Stream.FRand() < 0.5f) ? -Magnitude : Magnitude;
			}

			Actions.Add(FAction{
				Note.NoteId,
				Note.Direction,
				Note.HitTime + Offset
			});
		}
	}

	// Sort actions by scheduled time to ensure sequential processing
	Actions.Sort([](const FAction& A, const FAction& B) {
		return A.Time < B.Time;
	});
}

void UDanceAIOpponentComponent::Advance(float NewElapsed) {
 if (!Rhythm || !Rhythm->bRunning || Rhythm->Generation != Generation) return;
 const int32 EntryGeneration = Generation;
 while (NextAction < Actions.Num() && Actions[NextAction].Time <= NewElapsed) {
  const FAction Action = Actions[NextAction++];
  Rhythm->Attempt(EDanceSide::AI, Action.Direction, Action.Time, Action.NoteId);
  if (!Rhythm || Generation != EntryGeneration || Rhythm->Generation != EntryGeneration || !Rhythm->bRunning) return;
 }
}

void UDanceAIOpponentComponent::Cancel()
{
	Rhythm = nullptr;
	Actions.Empty();
	NextAction = 0;
	Generation = 0;
}
