#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DanceRhythmComponent.h"
#include "DanceAIOpponentComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DANCER_API UDanceAIOpponentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Initializes the AI with the rhythm component and pre-calculates AI actions based on the current generation. */
	void Reset(UDanceRhythmComponent* InRhythm);

	/** Processes due AI actions based on the new elapsed time. Should be called BEFORE Rhythm->Advance(). */
	void Advance(float NewElapsed);

	/** Clears the rhythm reference and the action queue. */
	void Cancel();

private:
	struct FAction
	{
		int32 NoteId;
		EDanceDirection Direction;
		float Time;
	};

	UPROPERTY()
	TObjectPtr<UDanceRhythmComponent> Rhythm;

	TArray<FAction> Actions;
	int32 NextAction = 0;
	int32 Generation = 0;
};
