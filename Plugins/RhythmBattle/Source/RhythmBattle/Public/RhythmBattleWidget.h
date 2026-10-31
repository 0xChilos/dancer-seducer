#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RhythmBattleTypes.h"
#include "RhythmBattleWidget.generated.h"

class URhythmBattleComponent;
class URhythmLaneWidget;

/** Screen-space HUD. Owns presentation snapshots only; the component owns timing and scoring. */
UCLASS(BlueprintType, Blueprintable)
class RHYTHMBATTLE_API URhythmBattleWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm") bool bShowProgress = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm") FLinearColor TextColor = FLinearColor::White;
    UFUNCTION(BlueprintCallable, Category="Rhythm") void SetBattleComponent(URhythmBattleComponent* Component);
    UFUNCTION(BlueprintCallable, Category="Rhythm") void SetSnapshot(const TArray<FRhythmLaneLayout>& Lanes, const TArray<FRhythmRuntimeNote>& Notes, const TArray<FRhythmPlayerScore>& Scores, double SongTime, double Duration, ERhythmBattleState Phase);
    UFUNCTION(BlueprintCallable, Category="Rhythm") void PushHitFeedback(const FRhythmHitResult& Result);
    UFUNCTION(BlueprintPure, Category="Rhythm") URhythmLaneWidget* GetLaneWidget() const { return LaneWidget; }
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
    virtual void NativeDestruct() override;
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect, FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override;
private:
    UPROPERTY(Transient) TObjectPtr<URhythmLaneWidget> LaneWidget;
    UPROPERTY(Transient) TObjectPtr<URhythmBattleComponent> BattleComponent;
    TArray<FRhythmPlayerScore> PlayerScores;
    struct FFeedback { FRhythmHitResult Result; double ExpiresAt = 0.; };
    TArray<FFeedback> Feedback;
    double SongTimeSeconds = 0.;
    double DurationSeconds = 0.;
    ERhythmBattleState BattlePhase = ERhythmBattleState::Stopped;
};
