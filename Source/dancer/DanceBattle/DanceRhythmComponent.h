#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DanceTypes.h"
#include "DanceRhythmComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnNoteSpawned, const FDanceNote&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnNoteJudged, const FDanceNote&);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnDanceAttempt, EDanceSide, EDanceDirection, EDanceJudgment);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DANCER_API UDanceRhythmComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dance")
	FDanceSettings Settings;

	UPROPERTY(BlueprintReadOnly, Category = "Dance")
	TArray<FDanceNote> Notes;

	UPROPERTY(BlueprintReadOnly, Category = "Dance")
	int32 Generation = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dance")
	float Elapsed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Dance")
	bool bRunning = false;

	FOnNoteSpawned OnSpawned;
	FOnNoteJudged OnJudged;
	FOnDanceAttempt OnAttempt;

	void Reset(const FDanceSettings& InSettings);
	void Advance(float NewElapsed);
	EDanceJudgment Attempt(EDanceSide Side, EDanceDirection Direction, float AtTime, int32 ExpectedNoteId = INDEX_NONE);
	void Finish();

private:
	void GenerateChart();
};