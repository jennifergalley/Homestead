#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

class AHomesteadController;

// How long one "+3 Berries" line stays up, in real seconds, and its fades: long enough to read a
// quick "+1 Berries" without lingering. A matching gain while it shows adds to it and holds it.
namespace HomesteadPickupTiming
{
inline constexpr float Seconds = 2.6f;
inline constexpr float FadeIn = 0.15f;
inline constexpr float FadeOut = 0.6f;
// At most this many lines at once; the oldest goes first.
inline constexpr int32 MaxLines = 4;
}

namespace HomesteadMenus
{
// What she has just gained ("+3 Berries"), floated just right of her in brass and cream with a dark
// outline and no panel, rising a little as it fades. Reads AHomesteadController::RecentPickups;
// the controller decides what counts as a gain (HomesteadControllerPickups.cpp).
class SHomesteadPickups : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SHomesteadPickups) {}
        SLATE_ARGUMENT(TWeakObjectPtr<AHomesteadController>, Controller)
    SLATE_END_ARGS()

    void Construct(const FArguments& Args);

    // In 1080-line HUD units (scaled with the viewport's height): the text, the gap from her, the
    // line spacing, how far a line rises over its life, and the margins it keeps from the edges and
    // from the calendar/vitals above and the hotbar below.
    static constexpr float TextSize = 20.0f;
    static constexpr float BesideHer = 70.0f;
    static constexpr float LineStep = 32.0f;
    static constexpr float Rise = 18.0f;
    static constexpr float SideMargin = 24.0f;
    static constexpr float TopMargin = 170.0f;
    static constexpr float BottomMargin = 150.0f;

protected:
    virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(1, 1); }
    virtual bool ComputeVolatility() const override { return true; }
    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
    TWeakObjectPtr<AHomesteadController> Controller;
};
}
