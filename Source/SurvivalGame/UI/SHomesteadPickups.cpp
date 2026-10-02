#include "SHomesteadPickups.h"
#include "HomesteadUITheme.h"

#include "HomesteadPalette.h"
#include "../HomesteadController.h"
#include "Simulation/HomesteadItems.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace HomesteadMenus
{
namespace PickupStyle
{
// Outline in HUD units, and its ink: dark enough to read over sky, grass or snow without a panel.
constexpr float Outline = 2.0f;
HomesteadUITheme::FThemeColor OutlineInk(0.025f, 0.05f, 0.038f, 0.9f);
}

void SHomesteadPickups::Construct(const FArguments& Args)
{
    Controller = Args._Controller;
    SetVisibility(EVisibility::HitTestInvisible);
}

int32 SHomesteadPickups::OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
    FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle&, bool) const
{
    if (!Controller.IsValid()) return LayerId;
    const auto& Lines = Controller->RecentPickups();
    FVector2D Pixel, ViewportPixels;
    if (Lines.IsEmpty() || !Controller->PickupAnchor(Pixel, ViewportPixels)) return LayerId;
    const FVector2D Local = Geometry.GetLocalSize();
    if (Local.X <= 1 || Local.Y <= 1) return LayerId;
    // One HUD unit, whatever the viewport's DPI scale: 1/1080 of its height.
    const float Unit = Local.Y / 1080.0f;
    const FVector2D Anchor = Pixel * (Local / ViewportPixels);
    FSlateFontInfo Font = HomesteadUITheme::Font("Bold", TextSize * Unit);
    Font.OutlineSettings.OutlineSize = FMath::Max(1, FMath::RoundToInt(PickupStyle::Outline * Unit));
    const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
    // Newest nearest her, older lines stacked above.
    for (int32 Row = 0; Row < Lines.Num(); ++Row)
    {
        const auto& Line = Lines[Lines.Num() - 1 - Row];
        const float In = FMath::Clamp(Line.Shown / HomesteadPickupTiming::FadeIn, 0.0f, 1.0f);
        const float OutFade = FMath::Clamp((HomesteadPickupTiming::Seconds - Line.Shown) / HomesteadPickupTiming::FadeOut, 0.0f, 1.0f);
        const float Alpha = In * OutFade;
        if (Alpha <= 0.0f) continue;
        FSlateFontInfo LineFont = Font;
        LineFont.OutlineSettings.OutlineColor = PickupStyle::OutlineInk.CopyWithNewOpacity(PickupStyle::OutlineInk.A * Alpha);
        const FString Amount = FString::Printf(TEXT("+%d "), Line.Amount);
        const FString Name = UTF8_TO_TCHAR(Homestead::ItemName(Line.Item));
        const FVector2D AmountSize = Measure->Measure(Amount, LineFont);
        const FVector2D NameSize = Measure->Measure(Name, LineFont);
        const float Width = AmountSize.X + NameSize.X;
        const float Lift = Rise * FMath::Clamp(Line.Shown / HomesteadPickupTiming::Seconds, 0.0f, 1.0f);
        FVector2D At(Anchor.X + BesideHer * Unit, Anchor.Y - (Row * LineStep + Lift) * Unit - AmountSize.Y * 0.5f);
        At.X = FMath::Clamp(At.X, SideMargin * Unit, FMath::Max(SideMargin * Unit, Local.X - Width - SideMargin * Unit));
        At.Y = FMath::Clamp(At.Y, TopMargin * Unit, FMath::Max(TopMargin * Unit, Local.Y - BottomMargin * Unit - AmountSize.Y));
        FSlateDrawElement::MakeText(Out, LayerId, Geometry.ToPaintGeometry(AmountSize, FSlateLayoutTransform(At)),
            Amount, LineFont, ESlateDrawEffect::None, HomesteadPalette::Brass.CopyWithNewOpacity(Alpha));
        FSlateDrawElement::MakeText(Out, LayerId, Geometry.ToPaintGeometry(NameSize, FSlateLayoutTransform(At + FVector2D(AmountSize.X, 0))),
            Name, LineFont, ESlateDrawEffect::None, HomesteadPalette::Cream.CopyWithNewOpacity(Alpha));
    }
    return LayerId;
}
}
