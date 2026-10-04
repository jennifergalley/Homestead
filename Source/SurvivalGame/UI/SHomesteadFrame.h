#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

namespace HomesteadMenus
{
// The one ornate frame: a double rule, a small corner flourish and a faint paper grain, drawn in code
// over whatever it wraps (HUD plates, hotbar, shop and field book panels). It paints no fill; the
// panel's own backing shows through. Geometry is UI/HomesteadFrameStyle.h, colours HomesteadPalette.
// Painting is a fixed handful of boxes whatever the content, so it costs the same on every panel.
class SHomesteadFrame : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SHomesteadFrame) {}
        SLATE_DEFAULT_SLOT(FArguments, Content)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args);
    virtual int32 OnPaint(const FPaintArgs& Paint, const FGeometry& Geometry, const FSlateRect& CullingRect,
        FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override;
};
}
