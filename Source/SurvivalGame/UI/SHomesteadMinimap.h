#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

class UHomesteadMapComponent;

namespace HomesteadMenus
{
// The round HUD minimap in the top-right corner. It fills the viewport but draws only its circle,
// laid out in the HUD's 1080-line units so it scales exactly like the Canvas HUD around it.
class SHomesteadMinimap : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SHomesteadMinimap) {}
        SLATE_ARGUMENT(TWeakObjectPtr<UHomesteadMapComponent>, Map)
    SLATE_END_ARGS()

    void Construct(const FArguments& Args);
    // The circle's centre and radius in this widget's local space (for tests and the HUD).
    bool CircleLocal(const FGeometry& Geometry, FVector2D& Center, float& Radius) const;

protected:
    virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }
    virtual bool ComputeVolatility() const override { return true; }
    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
    TWeakObjectPtr<UHomesteadMapComponent> Map;
};
}
