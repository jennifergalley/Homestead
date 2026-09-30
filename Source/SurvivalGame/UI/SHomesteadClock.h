#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class AHomesteadController;

namespace HomesteadMenus
{
// The calendar's time of day ("7:40 PM") as native Slate text, laid over the Canvas calendar panel
// (AHomesteadHUD::DrawCalendar draws the panel, dial, season and weather around it). Slate renders
// the glyphs at their real size, where the Canvas scaled one bitmap font to 42 lines and blurred it.
class SHomesteadClock : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SHomesteadClock) {}
        SLATE_ARGUMENT(TWeakObjectPtr<AHomesteadController>, Controller)
    SLATE_END_ARGS()

    void Construct(const FArguments& Args);

    // Where the time's text starts on the calendar panel (Canvas HUD units from the panel's top-left),
    // and the panel's width and right margin (AHomesteadHUD::DrawHUD).
    static constexpr float TextLeft = 140, TextTop = 40, PanelWidth = 460, PanelRight = 30;

private:
    FText HourText() const;
    FText MeridiemText() const;
    TWeakObjectPtr<AHomesteadController> Controller;
};
}
