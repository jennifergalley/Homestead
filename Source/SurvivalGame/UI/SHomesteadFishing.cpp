#include "SHomesteadFishing.h"
#include "HomesteadUITheme.h"
#include "SHomesteadHudScale.h"
#include "../HomesteadController.h"
#include "../Simulation/HomesteadFishing.h"

#include "Brushes/SlateRoundedBoxBrush.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/Text/STextBlock.h"

namespace HomesteadMenus
{
namespace FishingStyle
{
HomesteadUITheme::FThemeColor Paper(0.055f, 0.09f, 0.075f, 0.96f);
HomesteadUITheme::FThemeColor Ink(0.92f, 0.86f, 0.7f);
HomesteadUITheme::FThemeColor Water(0.16f, 0.3f, 0.32f);
HomesteadUITheme::FThemeColor Ripple(0.55f, 0.72f, 0.7f);
HomesteadUITheme::FThemeColor FloatRed(0.86f, 0.2f, 0.12f);
HomesteadUITheme::FThemeColor FloatWhite(0.95f, 0.93f, 0.86f);
HomesteadUITheme::FThemeColor Line(0.85f, 0.8f, 0.62f, 0.8f);
HomesteadUITheme::FThemeColor Cue(1.0f, 0.82f, 0.25f);
HomesteadUITheme::FThemeColor Pip(0.65f, 0.85f, 0.4f);

// Bobber cue tuning (logical px / seconds): the ring closes from RingOuter to RingInner across the
// simulation's own reaction window, so its timing always matches Homestead::Fishing::Marker.
constexpr float SceneHeight = 96.0f, FloatRadius = 9.0f, RingOuter = 46.0f, RingInner = 12.0f;
constexpr float BobAmplitude = 2.5f, BobHz = 0.7f, BiteDip = 7.0f, TugHz = 6.0f, TugPx = 5.0f;
constexpr int32 RingSegments = 36;

// Bank-side view of the float: it bobs while she waits, dips hard with a "!" and a closing ring when
// a fish bites, vanishes under a tugging line while the fish runs, and pops the cue again when the
// strike is ready. No bar to read: you watch the float and react, like at a real bank.
class SFishingCue : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SFishingCue) {}
        SLATE_ARGUMENT(TWeakObjectPtr<AHomesteadController>, Controller)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args)
    {
        Controller = Args._Controller;
        Round = FSlateRoundedBoxBrush(FLinearColor::White, FloatRadius);
    }
protected:
    virtual FVector2D ComputeDesiredSize(float) const override { return {SHomesteadFishing::Width - 32.0f, SceneHeight}; }
    virtual bool ComputeVolatility() const override { return true; }
    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect&,
        FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool) const override
    {
        if (!Controller.IsValid()) return Layer;
        const auto& Cast = Controller->Simulation().FishingCast();
        const FLinearColor Tint = Style.GetColorAndOpacityTint();
        const FVector2D Size = Geometry.GetLocalSize();
        const float Time = static_cast<float>(Args.GetCurrentTime());
        auto Box = [&](FVector2D At, FVector2D Extent, FLinearColor Colour, const FSlateBrush* Brush)
        {
            FSlateDrawElement::MakeBox(Elements, ++Layer, Geometry.ToPaintGeometry(Extent, FSlateLayoutTransform(At)),
                Brush, ESlateDrawEffect::None, Colour * Tint);
        };
        const FSlateBrush* White = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
        auto Lines = [&](const TArray<FVector2D>& Points, FLinearColor Colour, float Thickness)
        {
            FSlateDrawElement::MakeLines(Elements, ++Layer, Geometry.ToPaintGeometry(), Points,
                ESlateDrawEffect::None, Colour * Tint, true, Thickness);
        };
        auto Ring = [&](FVector2D Centre, float Radius, FLinearColor Colour, float Thickness)
        {
            TArray<FVector2D> Points;
            Points.Reserve(RingSegments + 1);
            for (int32 I = 0; I <= RingSegments; ++I)
            {
                const float A = 2.0f * PI * I / RingSegments;
                Points.Add(Centre + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Radius);
            }
            Lines(Points, Colour, Thickness);
        };

        const bool bBite = Cast.phase == Homestead::FishingPhase::Bite;
        const bool bLanding = Cast.phase == Homestead::FishingPhase::Landing;
        const double Marker = Homestead::Fishing::Marker(Cast);
        const bool bStrike = bLanding && Marker > 0.0 && Marker < 1.0;
        const bool bRunning = bLanding && !bStrike;
        const float WaterY = Size.Y * 0.62f;
        const FVector2D FloatAt(Size.X * 0.5f, WaterY + (bBite ? BiteDip : BobAmplitude * FMath::Sin(Time * 2.0f * PI * BobHz)));

        Box({0, WaterY}, {Size.X, Size.Y - WaterY}, Water, White);
        for (int32 I = 0; I < 3; ++I)
        {
            const float Spread = 30.0f + 22.0f * I + 6.0f * FMath::Sin(Time * 1.3f + I);
            Lines({{FloatAt.X - Spread, WaterY + 4.0f + 5.0f * I}, {FloatAt.X - Spread * 0.4f, WaterY + 4.0f + 5.0f * I}}, Ripple, 1.5f);
            Lines({{FloatAt.X + Spread * 0.4f, WaterY + 4.0f + 5.0f * I}, {FloatAt.X + Spread, WaterY + 4.0f + 5.0f * I}}, Ripple, 1.5f);
        }

        // Line from her rod tip, off the top left of the scene, down to the float or the running fish.
        const FVector2D Tip(Size.X * 0.08f, 2.0f);
        const FVector2D Under = bRunning
            ? FVector2D(FloatAt.X + TugPx * FMath::Sin(Time * 2.0f * PI * TugHz), WaterY + 2.0f)
            : FloatAt - FVector2D(0, FloatRadius);
        Lines({Tip, FMath::Lerp(Tip, Under, 0.5f) + FVector2D(0, bLanding ? 0.0f : 10.0f), Under}, Line, 1.5f);
        if (!bLanding)
        {
            Box(FloatAt - FVector2D(FloatRadius, FloatRadius), {FloatRadius * 2, FloatRadius * 2}, FloatWhite, &Round);
            Box(FloatAt - FVector2D(FloatRadius, FloatRadius), {FloatRadius * 2, FloatRadius}, FloatRed, White);
            Box(FloatAt - FVector2D(1.5f, FloatRadius + 7.0f), {3.0f, 7.0f}, FloatRed, White);
        }
        if (bRunning)
            Ring({Under.X, WaterY + 2.0f}, 10.0f + 4.0f * FMath::Abs(FMath::Sin(Time * 2.0f * PI * TugHz)), Ripple, 2.0f);

        if (bBite || bStrike)
        {
            const FVector2D Centre = bBite ? FloatAt : FVector2D(Under.X, WaterY);
            const float Closing = FMath::Lerp(RingOuter, RingInner, static_cast<float>(Marker));
            Ring(Centre, Closing, Cue, 3.0f);
            Ring(Centre, RingInner, Cue.CopyWithNewOpacity(0.45f), 1.5f);
            // "!" above the float, popping in at the start of the window.
            const float Pop = FMath::Min(1.0f, static_cast<float>(Marker) * 8.0f + 0.4f);
            const FVector2D Mark(Centre.X + RingOuter * 0.7f, 6.0f);
            Box(Mark, FVector2D(5.0f, 20.0f * Pop), Cue, White);
            Box(Mark + FVector2D(0, 24.0f * Pop), FVector2D(5.0f, 5.0f), Cue, White);
        }

        if (bLanding)
            for (int32 I = 0; I < Homestead::Fishing::LandingBeats; ++I)
            {
                const FVector2D At(Size.X - 18.0f - 18.0f * (Homestead::Fishing::LandingBeats - 1 - I), 10.0f);
                if (I < Cast.landedBeats) Box(At - FVector2D(6, 6), {12, 12}, Pip, &Round);
                else Ring(At, 6.0f, Pip, 1.5f);
            }
        return Layer;
    }
private:
    TWeakObjectPtr<AHomesteadController> Controller;
    FSlateBrush Round;
};
}
FBox2D SHomesteadFishing::LogicalBox(float ViewWidth)
{
    return {{(ViewWidth - Width) * 0.5f, 1080.0f - Bottom - Height},
        {(ViewWidth + Width) * 0.5f, 1080.0f - Bottom}};
}

void SHomesteadFishing::Construct(const FArguments& Args)
{
    Controller = Args._Controller;
    ChildSlot.HAlign(HAlign_Center).VAlign(VAlign_Bottom)
    [
        SNew(SHomesteadHudScale)
        [
            SNew(SBox).Padding(0, 0, 0, Bottom)
            [
                SNew(SBox).WidthOverride(Width).HeightOverride(Height)
                [
                    SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
                    .BorderBackgroundColor(FishingStyle::Paper).Padding(16)
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot().AutoHeight()
                        [
                            SNew(STextBlock).Font(HomesteadUITheme::Font(TEXT("Bold"), 24))
                            .ColorAndOpacity(FishingStyle::Ink)
                            .Text_Lambda([this]()
                            {
                                return Controller.IsValid()
                                    ? FText::FromString(FString(UTF8_TO_TCHAR(Homestead::Fishing::WaterName(Controller->Simulation().FishingCast().water))) + TEXT(" fishing"))
                                    : FText::GetEmpty();
                            })
                        ]
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 6)
                        [
                            SNew(SBox).HeightOverride(54)
                            [
                                SNew(STextBlock).Font(HomesteadUITheme::KeyFont(TEXT("Regular"), 20))
                                .ColorAndOpacity(FishingStyle::Ink).AutoWrapText(true)
                                .Text_Lambda([this]() { return Controller.IsValid() ? FText::FromString(Controller->FishingPrompt()) : FText::GetEmpty(); })
                            ]
                        ]
                        + SVerticalBox::Slot().AutoHeight()
                        [ SNew(FishingStyle::SFishingCue).Controller(Controller) ]
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)
                        [
                            SNew(STextBlock).Font(HomesteadUITheme::KeyFont(TEXT("Regular"), 16))
                            .ColorAndOpacity(FishingStyle::Ink)
                            .Text_Lambda([this]()
                            {
                                return FText::FromString(Controller.IsValid() && Controller->UsesGamepad()
                                    ? TEXT("[B] Cancel   Moving or changing tool reels in")
                                    : TEXT("[Esc] Cancel   Moving or changing tool reels in"));
                            })
                        ]
                    ]
                ]
            ]
        ]
    ];
}
}
