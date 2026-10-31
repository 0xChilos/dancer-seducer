#include "RhythmLaneWidget.h"
#include "HAL/PlatformTime.h"
#include "Rendering/DrawElements.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Widgets/SLeafWidget.h"

class SRhythmLaneSurface : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SRhythmLaneSurface) {} SLATE_END_ARGS()
    void Construct(const FArguments&, URhythmLaneWidget* InOwner) { Owner = InOwner; }
    virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(1280, 720); }
    virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&, FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool) const override
    { return Owner.IsValid() ? Owner->PaintLanes(Geometry, Elements, Layer, Style) : Layer; }
private:
    TWeakObjectPtr<URhythmLaneWidget> Owner;
};

URhythmLaneWidget::URhythmLaneWidget(const FObjectInitializer& Initializer) : Super(Initializer)
{
    SetVisibility(ESlateVisibility::HitTestInvisible);
    ForceVolatile(true); // Cosmetic feedback fades even when an external snapshot is unchanged.
    for (int32 Player = 0; Player < 2; ++Player)
        for (int32 Lane = 0; Lane < 2; ++Lane)
        {
            FRhythmLaneLayout& Layout = Lanes.AddDefaulted_GetRef();
            Layout.PlayerIndex = Player; Layout.LaneId = Lane;
            Layout.Start = FVector2D(Player == 0 ? .06 : .94, .60 + Lane * .25);
            Layout.HitTarget = FVector2D(Player == 0 ? .28 + Lane * .07 : .72 - Lane * .07, .69);
            Layout.Color = Lane == 0 ? FLinearColor(.95f, .92f, .35f) : FLinearColor(.95f, .65f, .85f);
        }
}

TSharedRef<SWidget> URhythmLaneWidget::RebuildWidget() { return SAssignNew(Surface, SRhythmLaneSurface, this); }
void URhythmLaneWidget::ReleaseSlateResources(bool bReleaseChildren) { Super::ReleaseSlateResources(bReleaseChildren); Surface.Reset(); }
void URhythmLaneWidget::SetLaneSnapshot(const TArray<FRhythmLaneLayout>& Layouts, const TArray<FRhythmRuntimeNote>& Notes)
{
    Lanes = Layouts; VisibleNotes = Notes;
    if (Surface) Surface->Invalidate(EInvalidateWidgetReason::Paint);
}
void URhythmLaneWidget::PushHitFeedback(const FRhythmHitResult& Result)
{
    const double Now = FPlatformTime::Seconds();
    Feedback.RemoveAll([Now](const FFeedback& Item) { return Item.ExpiresAt <= Now; });
    Feedback.Add({Result, Now + .45});
    if (Surface) Surface->Invalidate(EInvalidateWidgetReason::Paint);
}

int32 URhythmLaneWidget::PaintLanes(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style) const
{
    static const FSlateRoundedBoxBrush Circle(FLinearColor::White);
    const FVector2D Size = Geometry.GetLocalSize();
    const FLinearColor Tint = Style.GetColorAndOpacityTint();
    const auto Dot = [&](FVector2D Position, float Radius, FLinearColor Color, int32 Z)
    {
        FSlateDrawElement::MakeBox(Elements, Z, Geometry.ToPaintGeometry(FVector2D(Radius * 2), FSlateLayoutTransform(Position - FVector2D(Radius))), &Circle, ESlateDrawEffect::None, Color * Tint);
    };
    int32 Counts[2] = {0, 0};
    CachedGeometry.SetNum(Lanes.Num());
    int32 LaneIndex = -1;
    for (const FRhythmLaneLayout& Lane : Lanes)
    {
        ++LaneIndex;
        if (!Lane.bEnabled || Lane.PlayerIndex < 0 || Lane.PlayerIndex > 1 || Counts[Lane.PlayerIndex]++ >= 6) continue;
        const FVector2D Start = Lane.Start.ClampAxes(0., 1.) * Size;
        const FVector2D Target = Lane.HitTarget.ClampAxes(0., 1.) * Size;
        FCachedLaneGeometry& Cache = CachedGeometry[LaneIndex];
        if (Cache.Start != Start || Cache.Target != Target || Cache.Spacing != DotSpacing)
        {
            Cache.Start = Start; Cache.Target = Target; Cache.Spacing = DotSpacing;
            const int32 Steps = FMath::Clamp(FMath::FloorToInt((Target - Start).Size() / FMath::Max(8.f, DotSpacing)), 1, 256);
            Cache.Dots.Reset(Steps);
            for (int32 Index = 0; Index < Steps; ++Index) Cache.Dots.Add(FMath::Lerp(Start, Target, float(Index) / Steps));
        }
        for (const FVector2D& Position : Cache.Dots)
            Dot(Position, FMath::Clamp(Lane.DotRadius, 1.f, 24.f) * .45f, FLinearColor(1, 1, .85f, .7f), Layer);
        float Flash = 0.f;
        for (const FFeedback& Item : Feedback)
            if (Item.Result.PlayerIndex == Lane.PlayerIndex && Item.Result.LaneId == Lane.LaneId)
                Flash = FMath::Max(Flash, float(FMath::Clamp((Item.ExpiresAt - FPlatformTime::Seconds()) / .45, 0., 1.)));
        TArray<FVector2D>& Ring = Cache.Ring;
        Ring.SetNumUninitialized(33);
        const float Radius = FMath::Clamp(TargetRadius, 4.f, 100.f) + Flash * 4;
        for (int32 Index = 0; Index <= 32; ++Index)
        {
            const float Angle = Index * UE_TWO_PI / 32;
            Ring[Index] = Target + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius;
        }
        FSlateDrawElement::MakeLines(Elements, Layer + 1, Geometry.ToPaintGeometry(), Ring, ESlateDrawEffect::None, Lane.Color * Tint, true, 3.f + Flash * 3);
        for (const FRhythmRuntimeNote& Note : VisibleNotes)
            if (Note.bSpawned && Note.State == ERhythmNoteState::Active && Note.PlayerIndex == Lane.PlayerIndex && Note.LaneId == Lane.LaneId)
            {
                const FVector2D Position = FMath::Lerp(Start, Target, FMath::Clamp(Note.Progress, 0.f, 1.f));
                const float CoreRadius = FMath::Clamp(Lane.DotRadius, 1.f, 24.f);
                Dot(Position, CoreRadius * 1.5f, Lane.Color.CopyWithNewOpacity(.25f), Layer + 2);
                Dot(Position, CoreRadius, Lane.Color, Layer + 3);
            }
    }
    return Layer + 3;
}
