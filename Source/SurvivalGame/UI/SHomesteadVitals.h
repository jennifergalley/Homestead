#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class AHomesteadController;

namespace HomesteadMenus
{
// Her food, energy and purse, stacked under the calendar at the top-right of the HUD: an icon beside
// each bar, and the coin beside her balance, with no word labels. The last trade's change fades
// beside the balance.
class SHomesteadVitals : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SHomesteadVitals) {}
        SLATE_ARGUMENT(TWeakObjectPtr<AHomesteadController>, Controller)
    SLATE_END_ARGS()

    void Construct(const FArguments& Args);

    // Where the stack sits in the Canvas HUD's 1080-line logical units, for layout and feedback checks.
    // It matches the calendar panel's width and right edge (AHomesteadHUD::DrawHUD) and sits just below it.
    static FBox2D LogicalBox(float ViewWidth);
    // Top = the calendar's top (26) + height (100) + an 8-unit gap (HomesteadHudLayout in HomesteadHUD.h).
    static constexpr float Right = 30, Top = 134, RowHeight = 54, RowGap = 6, Width = 460;

private:
    virtual void Tick(const FGeometry& Geometry, double Time, float Delta) override;
    TSharedRef<SWidget> MeterRow(FName Icon, TFunction<double()> Value, FLinearColor Fill, int32 Meter, const TCHAR* Label);
    TSharedRef<SWidget> PurseRow();
    // A meal's gain on the food (0) or energy (1) bar: the bar fills from where it stood, and a
    // "+N Energy" popup rises and fades above its right end.
    struct FMealAnim { double From = 0, Gain = 0, StartedAt = -1000.0; };
    FMealAnim Meals[2];
    uint32 MealSerialSeen = 0;
    bool bMealPrimed = false;
    double Displayed(int32 Meter, double Actual) const;
    float PopupAlpha(int32 Meter) const;
    TWeakObjectPtr<AHomesteadController> Controller;
};
}
