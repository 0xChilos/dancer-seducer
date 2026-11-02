#include "DanceRhythmArenaGameMode.h"
#include "DanceBattle/Player/DancePlaceholderCharacter.h"
#include "RhythmBattleComponent.h"
#include "EngineUtils.h"

void ADanceRhythmArenaGameMode::BeginPlay()
{
    for (TActorIterator<ADancePlaceholderCharacter> It(GetWorld()); It; ++It)
    {
        if (It->Side == EDanceSide::Player) PlayerDancer = *It;
        else if (It->Side == EDanceSide::AI) AIDancer = *It;
    }
    if (PlayerDancer) PlayerDancer->Configure(EDanceSide::Player);
    if (AIDancer) AIDancer->Configure(EDanceSide::AI);
    Battle->OnNoteHit.AddDynamic(this, &ADanceRhythmArenaGameMode::HandleNote);
    Battle->OnNoteMiss.AddDynamic(this, &ADanceRhythmArenaGameMode::HandleNote);
    Super::BeginPlay();
}

void ADanceRhythmArenaGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Battle->OnNoteHit.RemoveDynamic(this, &ADanceRhythmArenaGameMode::HandleNote);
    Battle->OnNoteMiss.RemoveDynamic(this, &ADanceRhythmArenaGameMode::HandleNote);
    Super::EndPlay(EndPlayReason);
}

void ADanceRhythmArenaGameMode::HandleNote(const FRhythmHitResult& Result)
{
    ADancePlaceholderCharacter* Dancer = Result.PlayerIndex == 0 ? PlayerDancer.Get() :
        (Result.PlayerIndex == 1 ? AIDancer.Get() : nullptr);
    if (!IsValid(Dancer)) return;
    // Map 3 lanes to directions: lane 0=Up, lane 1=Left, lane 2=Down
    EDanceDirection Direction;
    if (Result.LaneId == 0) Direction = EDanceDirection::Up;
    else if (Result.LaneId == 1) Direction = EDanceDirection::Left;
    else Direction = EDanceDirection::Down;
    const EDanceJudgment Judgment = Result.Judgment == ERhythmJudgment::Perfect ? EDanceJudgment::Perfect :
        (Result.Judgment == ERhythmJudgment::Miss ? EDanceJudgment::Miss : EDanceJudgment::Good);
    Dancer->React(Direction, Judgment);
}
