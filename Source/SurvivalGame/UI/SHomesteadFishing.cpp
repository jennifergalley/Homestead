#include "SHomesteadFishing.h"
#include "HomesteadUITheme.h"
#include "SHomesteadHudScale.h"
#include "../HomesteadController.h"
#include "../Simulation/HomesteadFishing.h"

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
HomesteadUITheme::FThemeColor Track(0.2f, 0.25f, 0.2f);
HomesteadUITheme::FThemeColor Band(FLinearColor(0.65f, 0.85f, 0.4f), FLinearColor(0.7f, 0.8f, 0.55f), FLinearColor(0.65f, 0.85f, 0.4f));
HomesteadUITheme::FThemeColor Marker(0.98f, 0.93f, 0.8f);

class SFishingMeter : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SFishingMeter) {}
        SLATE_ARGUMENT(TWeakObjectPtr<AHomesteadController>, Controller)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args) { Controller = Args._Controller; }
protected:
    virtual FVector2D ComputeDesiredSize(float) const override { return {SHomesteadFishing::Width - 32.0f, 22.0f}; }
    virtual bool ComputeVolatility() const override { return true; }
    virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
        FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool) const override
    {
        if (!Controller.IsValid()) return Layer;
        const auto& Cast = Controller->Simulation().FishingCast();
        const FVector2D Size = Geometry.GetLocalSize();
        auto Draw = [&](FVector2D At, FVector2D Extent, FLinearColor Tint)
        {
            FSlateDrawElement::MakeBox(Elements, ++Layer,
                Geometry.ToPaintGeometry(Extent, FSlateLayoutTransform(At)),
                FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")), ESlateDrawEffect::None,
                Tint * Style.GetColorAndOpacityTint());
        };
        Draw({0, 4}, {Size.X, 14}, Track);
        if (Cast.phase == Homestead::FishingPhase::Bite)
            Draw({0, 4}, {Size.X, 14}, Band);
        if (Cast.phase == Homestead::FishingPhase::Landing)
        {
            Draw({Size.X * Homestead::Fishing::LandingBandStart, 4},
                {Size.X * (Homestead::Fishing::LandingBandEnd - Homestead::Fishing::LandingBandStart), 14}, Band);
            Draw({Size.X * Homestead::Fishing::LandingBandStart, 0}, {2, Size.Y}, Ink);
            Draw({Size.X * Homestead::Fishing::LandingBandEnd - 2, 0}, {2, Size.Y}, Ink);
        }
        if (Cast.phase == Homestead::FishingPhase::Bite || Cast.phase == Homestead::FishingPhase::Landing)
        {
            constexpr double MarkerWidth = 8.0;
            const double MarkerX = FMath::Clamp(Size.X * Homestead::Fishing::Marker(Cast) - MarkerWidth * 0.5,
                0.0, FMath::Max(0.0, Size.X - MarkerWidth));
            Draw({MarkerX, 0}, {MarkerWidth, Size.Y}, Marker);
            Draw({MarkerX + 2, 2}, {MarkerWidth - 4, Size.Y - 4}, Paper);
        }
        return Layer;
    }
private:
    TWeakObjectPtr<AHomesteadController> Controller;
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
                        [ SNew(FishingStyle::SFishingMeter).Controller(Controller) ]
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
