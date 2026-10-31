#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "RhythmBattleTypes.h"
#include "RhythmLaneWidget.generated.h"

/** One Slate surface for all lanes. Progress is supplied by gameplay, never judged here. */
UCLASS(BlueprintType)
class RHYTHMBATTLE_API URhythmLaneWidget : public UWidget
{
    GENERATED_BODY()
public:
    URhythmLaneWidget(const FObjectInitializer& ObjectInitializer);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm") TArray<FRhythmLaneLayout> Lanes;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm", meta=(ClampMin="8")) float DotSpacing = 28.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rhythm", meta=(ClampMin="4")) float TargetRadius = 22.f;
    UFUNCTION(BlueprintCallable, Category="Rhythm") void SetLaneSnapshot(const TArray<FRhythmLaneLayout>& Layouts, const TArray<FRhythmRuntimeNote>& Notes);
    UFUNCTION(BlueprintCallable, Category="Rhythm") void PushHitFeedback(const FRhythmHitResult& Result);
    int32 PaintLanes(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style) const;
    virtual void ReleaseSlateResources(bool bReleaseChildren) override;
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
private:
    TArray<FRhythmRuntimeNote> VisibleNotes;
    struct FFeedback { FRhythmHitResult Result; double ExpiresAt = 0.; };
    TArray<FFeedback> Feedback;
    struct FCachedLaneGeometry
    {
        FVector2D Start = FVector2D(-1, -1);
        FVector2D Target = FVector2D(-1, -1);
        float Spacing = -1;
        TArray<FVector2D> Dots;
        TArray<FVector2D> Ring;
    };
    mutable TArray<FCachedLaneGeometry> CachedGeometry;
    TSharedPtr<class SRhythmLaneSurface> Surface;
};
