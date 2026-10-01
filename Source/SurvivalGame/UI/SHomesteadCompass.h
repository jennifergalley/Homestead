#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

class UHomesteadMapComponent;

namespace HomesteadMenus
{
// The compass trial: a brass-ruled band at the top centre of the HUD that turns with the camera,
// showing N/E/S/W and the Map tab's landmarks hanging beneath it at their bearings. Like the minimap
// it fills the viewport, lays out in the Canvas HUD's 1080-line units and draws only its band; it
// reads the map component's per-frame snapshot, never the world.
class SHomesteadCompass : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SHomesteadCompass) {}
        SLATE_ARGUMENT(TWeakObjectPtr<UHomesteadMapComponent>, Map)
    SLATE_END_ARGS()

    void Construct(const FArguments& Args);

protected:
    virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }
    virtual bool ComputeVolatility() const override { return true; }
    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
    TWeakObjectPtr<UHomesteadMapComponent> Map;
};
}
