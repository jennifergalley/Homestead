#include "SHomesteadClock.h"

#include "../HomesteadController.h"
#include "../HomesteadHUD.h"
#include "HomesteadPalette.h"
#include "SHomesteadHudScale.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace HomesteadMenus
{
namespace ClockStyle
{
// Sized to the Canvas clock it replaces (42-unit line, 24-unit AM/PM) at the HUD's 1080-line scale.
constexpr int32 TimeSize = 30, MeridiemSize = 17;
// Lifts AM/PM onto the time's baseline.
constexpr float MeridiemBaseline = 5, MeridiemGap = 8;
}

void SHomesteadClock::Construct(const FArguments& Args)
{
    Controller = Args._Controller;
    SetVisibility(EVisibility::HitTestInvisible);
    ChildSlot
    .HAlign(HAlign_Right)
    .VAlign(VAlign_Top)
    [
        // One unit inside is one Canvas HUD unit, so the time sits on the calendar at any size.
        SNew(SHomesteadHudScale)
        [
            SNew(SBox).WidthOverride(PanelWidth + PanelRight)
            .Padding(TextLeft, HomesteadHudLayout::CalendarTop + TextTop, PanelRight, 0)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom)
                [
                    SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), ClockStyle::TimeSize))
                    .ColorAndOpacity(HomesteadPalette::Cream)
                    .Text_Lambda([this]() { return HourText(); })
                ]
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom)
                    .Padding(ClockStyle::MeridiemGap, 0, 0, ClockStyle::MeridiemBaseline)
                [
                    SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), ClockStyle::MeridiemSize))
                    .ColorAndOpacity(HomesteadPalette::Brass)
                    .Text_Lambda([this]() { return MeridiemText(); })
                ]
            ]
        ]
    ];
}

FText SHomesteadClock::HourText() const
{
    if (!Controller.IsValid()) return FText::GetEmpty();
    const double Hour = FMath::Fmod(Controller->State().hour, 24.0);
    const int32 H = FMath::FloorToInt(Hour);
    const int32 M = FMath::FloorToInt((Hour - H) * 60);
    return FText::FromString(FString::Printf(TEXT("%d:%02d"), H % 12 == 0 ? 12 : H % 12, M));
}

FText SHomesteadClock::MeridiemText() const
{
    if (!Controller.IsValid()) return FText::GetEmpty();
    return FText::FromString(FMath::Fmod(Controller->State().hour, 24.0) < 12.0 ? TEXT("AM") : TEXT("PM"));
}
}
