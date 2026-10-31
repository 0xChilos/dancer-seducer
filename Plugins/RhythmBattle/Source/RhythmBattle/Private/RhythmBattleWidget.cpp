#include "RhythmBattleWidget.h"
#include "RhythmBattleComponent.h"
#include "RhythmLaneWidget.h"
#include "HAL/PlatformTime.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SOverlay.h"

TSharedRef<SWidget> URhythmBattleWidget::RebuildWidget()
{
    ForceVolatile(true);
    SetVisibility(ESlateVisibility::HitTestInvisible);
    if (!LaneWidget) LaneWidget = NewObject<URhythmLaneWidget>(this);
    return SNew(SOverlay) + SOverlay::Slot()[LaneWidget->TakeWidget()];
}

void URhythmBattleWidget::SetBattleComponent(URhythmBattleComponent* Component)
{
    if (BattleComponent)
    {
        BattleComponent->OnNoteHit.RemoveDynamic(this, &URhythmBattleWidget::PushHitFeedback);
        BattleComponent->OnNoteMiss.RemoveDynamic(this, &URhythmBattleWidget::PushHitFeedback);
    }
    BattleComponent = Component;
    if (BattleComponent)
    {
        BattleComponent->OnNoteHit.AddUniqueDynamic(this, &URhythmBattleWidget::PushHitFeedback);
        BattleComponent->OnNoteMiss.AddUniqueDynamic(this, &URhythmBattleWidget::PushHitFeedback);
    }
}
void URhythmBattleWidget::NativeDestruct()
{
    SetBattleComponent(nullptr);
    Super::NativeDestruct();
}
void URhythmBattleWidget::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
    Super::NativeTick(Geometry, DeltaSeconds);
    if (BattleComponent)
        SetSnapshot(BattleComponent->GetLaneSnapshot(), BattleComponent->GetNoteSnapshot(), BattleComponent->GetScoreSnapshot(), BattleComponent->GetSongTimeSeconds(), BattleComponent->GetDurationSeconds(), BattleComponent->GetBattleState());
}
void URhythmBattleWidget::SetSnapshot(const TArray<FRhythmLaneLayout>& Lanes, const TArray<FRhythmRuntimeNote>& Notes, const TArray<FRhythmPlayerScore>& Scores, double SongTime, double Duration, ERhythmBattleState Phase)
{
    if (!LaneWidget) LaneWidget = NewObject<URhythmLaneWidget>(this);
    LaneWidget->SetLaneSnapshot(Lanes, Notes);
    PlayerScores = Scores; SongTimeSeconds = SongTime; DurationSeconds = Duration; BattlePhase = Phase;
    InvalidateLayoutAndVolatility();
}
void URhythmBattleWidget::PushHitFeedback(const FRhythmHitResult& Result)
{
    const double Now = FPlatformTime::Seconds();
    Feedback.RemoveAll([&](const FFeedback& Item) { return Item.ExpiresAt <= Now || Item.Result.PlayerIndex == Result.PlayerIndex; });
    Feedback.Add({Result, Now + .65});
    if (LaneWidget) LaneWidget->PushHitFeedback(Result);
    InvalidateLayoutAndVolatility();
}

int32 URhythmBattleWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect, FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const
{
    const int32 TopLayer = Super::NativePaint(Args, Geometry, CullingRect, Elements, Layer, Style, bParentEnabled) + 1;
    const FVector2D Size = Geometry.GetLocalSize();
    const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold", 20);
    const FLinearColor Tint = TextColor * Style.GetColorAndOpacityTint();
    const auto Text = [&](const FString& Value, FVector2D Position, FLinearColor Color)
    {
        FSlateDrawElement::MakeText(Elements, TopLayer, Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(Position)), Value, Font, ESlateDrawEffect::None, Color);
    };
    for (const FRhythmPlayerScore& Score : PlayerScores)
    {
        if (Score.PlayerIndex < 0 || Score.PlayerIndex > 1) continue;
        const FVector2D Position(Size.X * (Score.PlayerIndex == 0 ? .12 : .70), Size.Y * .43);
        Text(FString::Printf(TEXT("%lld"), static_cast<long long>(Score.Score)), Position, Tint);
        Text(FString::Printf(TEXT("Combo %d"), Score.Combo), Position + FVector2D(0, 30), Tint);
    }
    for (const FFeedback& Item : Feedback)
    {
        const float Alpha = FMath::Clamp(float((Item.ExpiresAt - FPlatformTime::Seconds()) / .65), 0.f, 1.f);
        if (Alpha <= 0 || Item.Result.PlayerIndex < 0 || Item.Result.PlayerIndex > 1) continue;
        const FString Judgment = StaticEnum<ERhythmJudgment>()->GetDisplayNameTextByValue(static_cast<int64>(Item.Result.Judgment)).ToString();
        const FString ScoreStr = Item.Result.Points >= 0 ? FString::Printf(TEXT("+%lld"), static_cast<long long>(Item.Result.Points)) : FString::Printf(TEXT("%lld"), static_cast<long long>(Item.Result.Points));
        Text(FString::Printf(TEXT("%s  %s"), *Judgment, *ScoreStr), FVector2D(Size.X * (Item.Result.PlayerIndex == 0 ? .12 : .70), Size.Y * .55), Tint.CopyWithNewOpacity(Tint.A * Alpha));
    }
    if (bShowProgress)
    {
        const float Progress = DurationSeconds > 0 ? FMath::Clamp(float(SongTimeSeconds / DurationSeconds), 0.f, 1.f) : 0.f;
        Text(StaticEnum<ERhythmBattleState>()->GetDisplayNameTextByValue(static_cast<int64>(BattlePhase)).ToString(), FVector2D(Size.X * .43, Size.Y * .06), Tint);
        const FSlateBrush* Brush = FCoreStyle::Get().GetBrush("WhiteBrush");
        const FVector2D BarSize(Size.X * .24, 5);
        const FVector2D BarPosition(Size.X * .38, Size.Y * .12);
        FSlateDrawElement::MakeBox(Elements, TopLayer, Geometry.ToPaintGeometry(BarSize, FSlateLayoutTransform(BarPosition)), Brush, ESlateDrawEffect::None, FLinearColor(1, 1, 1, .2f) * Tint);
        FSlateDrawElement::MakeBox(Elements, TopLayer + 1, Geometry.ToPaintGeometry(FVector2D(BarSize.X * Progress, BarSize.Y), FSlateLayoutTransform(BarPosition)), Brush, ESlateDrawEffect::None, Tint);
    }
    if (BattlePhase == ERhythmBattleState::Finished)
    {
        Text(TEXT("Finished - Press R to restart"), FVector2D(Size.X * .28f, Size.Y * .80f), Tint);
    }
    
    Text(TEXT("A / D"), FVector2D(Size.X * .08f, Size.Y * .90f), Tint.CopyWithNewOpacity(0.6f));
    Text(TEXT("J / L (autoplay)"), FVector2D(Size.X * .60f, Size.Y * .90f), Tint.CopyWithNewOpacity(0.6f));
    Text(TEXT("R - Restart | P - Pause"), FVector2D(Size.X * .32f, Size.Y * .96f), Tint.CopyWithNewOpacity(0.6f));

    return TopLayer + 1;
}
