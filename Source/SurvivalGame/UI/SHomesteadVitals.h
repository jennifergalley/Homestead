#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class AHomesteadController;

namespace HomesteadMenus
{
// Her food, energy and purse at the bottom-left of the HUD: an icon beside each bar, and the coin
// beside her balance, with no word labels. The last trade's change floats up beside the balance.
class SHomesteadVitals : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SHomesteadVitals) {}
        SLATE_ARGUMENT(TWeakObjectPtr<AHomesteadController>, Controller)
    SLATE_END_ARGS()

    void Construct(const FArguments& Args);

    // Where the stack sits in the Canvas HUD's 1080-line logical units, for feedback layout checks.
    static FBox2D LogicalBox(float ViewHeight);
    static constexpr float Left = 30, Bottom = 22, RowHeight = 38, RowGap = 4, Width = 214;

private:
    TSharedRef<SWidget> MeterRow(FName Icon, TFunction<double()> Value, FLinearColor Fill);
    TSharedRef<SWidget> PurseRow();
    TWeakObjectPtr<AHomesteadController> Controller;
};
}
