#include "DanceBattleWidget.h"
#include "../Gameplay/DanceBattleGameMode.h"
#include "../Rhythm/DanceRhythmComponent.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Math/UnrealMathUtility.h"

int32 UDanceBattleWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    int32 L = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled) + 1;
    if (!Battle || !Battle->Rhythm) return L;
    const UDanceRhythmComponent* Rhythm = Battle->Rhythm.Get();
    float Now = Battle->GetElapsedTime();
    float BeatInterval = Rhythm->Settings.BeatInterval;
    FVector2D Scale = AllottedGeometry.GetLocalSize() / FVector2D(1280, 720);
    float S = FMath::Min(Scale.X, Scale.Y);

    auto DrawText = [&](FVector2D pos, FString str, float size, FLinearColor color) {
        FSlateDrawElement::MakeText(OutDrawElements, L, AllottedGeometry.ToPaintGeometry(FVector2D(1, 1), FSlateLayoutTransform(pos * Scale)), str, FCoreStyle::GetDefaultFontStyle("Bold", FMath::Max(10, int32(size * S))), ESlateDrawEffect::None, color);
    };

    auto DrawLine = [&](const TArray<FVector2D>& points, FLinearColor color, float thickness) {
        TArray<FVector2D> Scaled; for (const auto& p : points) Scaled.Add(p * Scale);
        FSlateDrawElement::MakeLines(OutDrawElements, L, AllottedGeometry.ToPaintGeometry(), Scaled, ESlateDrawEffect::None, color, true, thickness * S);
    };

    auto DrawCircle = [&](FVector2D center, float radius, FLinearColor color) {
        TArray<FVector2D> pts; for (int i = 0; i <= 48; ++i) {
            float a = i * (2.0f * PI / 48.0f); pts.Add(center + FVector2D(FMath::Cos(a) * radius, FMath::Sin(a) * radius));
        }
        DrawLine(pts, color, 2.0f);
    };

    DrawText(FVector2D(130, 30), TEXT("PLAYER"), 32, FLinearColor::White);
    DrawText(FVector2D(930, 30), TEXT("AI"), 32, FLinearColor::White);
    DrawText(FVector2D(610, 30), TimerText, 32, FLinearColor::White);
    DrawText(FVector2D(500, 90), TEXT("SOLO DANCE BATTLE"), 20, FLinearColor::White);

    float Pulse = 62.0f + 4.0f * FMath::Cos((Now - Rhythm->Settings.TravelTime) * 2.0f * PI / FMath::Max(0.1f, BeatInterval));
    DrawCircle(FVector2D(300, 590), Pulse, FLinearColor(0.05f, 0.9f, 0.85f));
    DrawCircle(FVector2D(980, 590), Pulse, FLinearColor(0.9f, 0.2f, 0.5f));
    DrawText(FVector2D(270, 560), PlayerText, 48, FLinearColor::White);
    DrawText(FVector2D(950, 560), AIText, 48, FLinearColor::White);

    auto DrawLane = [&](float xBase, bool bIsPlayer) {
        for (int i = 0; i < 4; ++i) {
            float x = xBase + i * 75.0f; DrawCircle(FVector2D(x, 430), 22, FLinearColor(0.05f,0.9f,0.85f));
            FString Arr = (i == 0) ? TEXT("↑") : (i == 1) ? TEXT("↓") : (i == 2) ? TEXT("←") : TEXT("→");
            DrawText(FVector2D(x - 12, 412), Arr, 28, bIsPlayer ? FLinearColor::White : FLinearColor(0.9f, 0.2f, 0.5f));
        }
    };
    DrawLane(170, true); DrawLane(810, false);

    for (const auto& N : Rhythm->Notes) {
        if (N.Judgment == EDanceJudgment::Pending && Now >= N.SpawnTime && Now <= N.HitTime + Rhythm->Settings.GoodWindow) {
            float prog = FMath::Clamp((Now - N.SpawnTime) / FMath::Max(0.01f, Rhythm->Settings.TravelTime), 0.0f, 1.2f);
            float x = ((N.Side == EDanceSide::Player) ? 170.0f : 810.0f) + (static_cast<int32>(N.Direction)) * 75.0f;
            FString Arr = (static_cast<int32>(N.Direction) == 0) ? TEXT("↑") : (static_cast<int32>(N.Direction) == 1) ? TEXT("↓") : (static_cast<int32>(N.Direction) == 2) ? TEXT("←") : TEXT("→");
            DrawText(FVector2D(x - 12, 220 + 210 * prog - 18), Arr, 28, (N.Side == EDanceSide::Player) ? FLinearColor::White : FLinearColor(0.9f, 0.2f, 0.5f));
        }
    }

    auto DrawFB = [&](FVector2D pos, const FString& txt, float ts, bool bPlayer) {
        if (Now >= ts && Now - ts <= 0.65f) {
            FLinearColor c = (txt == "PERFECT") ? FLinearColor::Yellow : (txt == "GOOD") ? FLinearColor(0.05f, 0.9f, 0.85f) : FLinearColor::Red;
            DrawText(pos, txt, 32, c);
        }
    };
    DrawFB(FVector2D(240, 500), PlayerFeedback, PlayerFeedbackTime, true);
    DrawFB(FVector2D(900, 500), AIFeedback, AIFeedbackTime, false);

    if (!ResultText.IsEmpty()) {
        DrawText(FVector2D(510, 300), ResultText, 48, FLinearColor::White);
        DrawText(FVector2D(550, 370), TEXT("R TO RESTART"), 24, FLinearColor::White);
    }
    DrawText(FVector2D(240, 680), TEXT("ARROWS / WASD - MATCH THE NOTES     R - RESTART"), 16, FLinearColor::Gray);

    return L;
}