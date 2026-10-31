#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DanceTypes.h"
#include "DanceBattleWidget.generated.h"

class ADanceBattleGameMode;
class ADanceBattleGameState;
class UDanceRhythmComponent;

/**
 * UDanceBattleWidget handles the state caching and UI logic for the dance battle.
 * Drawing logic is deferred to NativePaint in a subsequent implementation.
 */
UCLASS()
class DANCER_API UDanceBattleWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Initializes the widget with the game mode and binds to game state delegates */
    UFUNCTION(BlueprintCallable, Category = "DanceBattle")
    void InitializeBattle(ADanceBattleGameMode* InBattle);

protected:
    virtual void NativeDestruct() override;

    /** Standard Slate override for custom drawing (Implementation in next task) */
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippedGeometry, FSlateWindowElementList& OutList, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
    /** Updates the cached FString values from the GameState */
    void RefreshState();

    /** Called when a player/AI makes a rhythmic attempt */
    void OnAttempt(EDanceSide Side, EDanceDirection Direction, EDanceJudgment Judgment);

    /** Called when a note is judged (handles Miss logic) */
    void OnJudged(const FDanceNote& Note);

    UPROPERTY()
    TObjectPtr<ADanceBattleGameMode> Battle;

    // Cached UI Strings
    FString PlayerText;
    FString AIText;
    FString TimerText;
    FString ResultText;
    FString PlayerFeedback;
    FString AIFeedback;

    // Timestamps for feedback expiration (Domain: Elapsed Round Time)
    float PlayerFeedbackTime = -10.0f;
    float AIFeedbackTime = -10.0f;
};
