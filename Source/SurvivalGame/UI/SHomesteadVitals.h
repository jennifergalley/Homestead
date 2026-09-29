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
    TSharedRef<SWidget> MeterRow(FName Icon, TFunction<double()> Value, FLinearColor Fill);
    TSharedRef<SWidget> PurseRow();
    TWeakObjectPtr<AHomesteadController> Controller;
};
}
