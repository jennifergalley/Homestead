#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class AHomesteadController;

namespace HomesteadMenus
{
// Her energy and purse, stacked under the calendar at the top-right of the HUD: an icon beside the
// bar, and the coin beside her balance. The last trade's change fades beside the balance. On the
// estate there is no hunger, so the single Energy bar stands alone, and while she is Well fed a small
// "Well fed until 2:30 PM · work costs 15% less" chip sits under the purse (so nothing above it moves when it comes and
// goes). The seeded woodland keeps its Food bar above Energy.
class SHomesteadVitals : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SHomesteadVitals) {}
        SLATE_ARGUMENT(TWeakObjectPtr<AHomesteadController>, Controller)
    SLATE_END_ARGS()

    void Construct(const FArguments& Args);

    // Where the stack sits in the Canvas HUD's 1080-line logical units, for layout and feedback checks.
    // It matches the calendar panel's width and right edge (AHomesteadHUD::DrawHUD) and sits just below it.
    static FBox2D LogicalBox(float ViewWidth, bool bFoodRow, bool bWellFedRow);
    // The same for her current state: the Food row only off the estate, the chip only while Well fed.
    static FBox2D LogicalBox(float ViewWidth, const AHomesteadController& Controller);
    static bool ShowsFoodRow(const AHomesteadController& Controller);
    static bool ShowsWellFed(const AHomesteadController& Controller);
    // Low-meter warnings (Jenny 2026-09-30: she starved "with zero warning"). A meter is low below
    // LowAt and critical below CriticalAt: its bar turns amber, then a muted red, and pulses gently, and
    // a short notice marks each crossing ("Getting tired", "Exhausted"; "Getting hungry", "Starving" for
    // the woodland's Food). The band only drops back once the meter is RearmMargin above the line, so a
    // value hovering at 25 doesn't repeat itself.
    static constexpr double LowAt = 25, CriticalAt = 10, RearmMargin = 5;
    // A meter that moves this much in one tick was loaded, slept or eaten up, not worn down: re-arm quietly.
    static constexpr double JumpLimit = 20;
    // 0 fine, 1 low, 2 critical, given the band it showed last.
    static int32 WarningBand(double Value, int32 Previous);
    static const TCHAR* WarningText(int32 Meter, int32 Band);
    // Top = the calendar's top (26) + height (100) + an 8-unit gap (HomesteadHudLayout in HomesteadHUD.h).
    // ChipHeight fits two wrapped lines (the longest badge, "Well fed until 12:45 PM - work costs
    // 15% less", overflows the chip's single-line width; see WellFedChip's WrapTextAt).
    static constexpr float Right = 30, Top = 134, RowHeight = 54, RowGap = 6, Width = 460, ChipHeight = 60;

private:
    virtual void Tick(const FGeometry& Geometry, double Time, float Delta) override;
    TSharedRef<SWidget> MeterRow(FName Icon, TFunction<double()> Value, FLinearColor Fill, int32 Meter, const TCHAR* Label);
    TSharedRef<SWidget> PurseRow();
    TSharedRef<SWidget> WellFedChip();
    // A meal's gain on the food (0) or energy (1) bar: the bar fills from where it stood, and a
    // "+N Energy" popup rises and fades above its right end.
    struct FMealAnim { double From = 0, Gain = 0, StartedAt = -1000.0; };
    FMealAnim Meals[2];
    uint32 MealSerialSeen = 0;
    bool bMealPrimed = false;
    double Displayed(int32 Meter, double Actual) const;
    float PopupAlpha(int32 Meter) const;
    // The band each meter (0 food, 1 energy) showed last and its value, primed on the first tick so a
    // rebuilt HUD or a freshly loaded game says nothing.
    int32 WarnBand[2] = {0, 0};
    double WarnLast[2] = {0, 0};
    bool bWarnPrimed = false;
    void UpdateLowWarnings();
    TWeakObjectPtr<AHomesteadController> Controller;
};
}
