#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"
#include "HomesteadMapGeometry.h"

class UHomesteadMapComponent;
struct FHomesteadMapViewState;

namespace HomesteadMenus
{
// The field book's Map tab: the whole baked map fitted to the page, parcels (the owned estate
// dashed, for-sale land hatched), named landmarks and a pulsing "you are here". Mouse drag pans and
// the wheel zooms; the menu forwards controller sticks, triggers and D-pad landmark steps.
class SHomesteadMapView : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SHomesteadMapView) {}
        SLATE_ARGUMENT(TWeakObjectPtr<UHomesteadMapComponent>, Map)
        SLATE_ATTRIBUTE(bool, UsesGamepad)
    SLATE_END_ARGS()

    void Construct(const FArguments& Args);
    // Controller analog input (left stick pans, right stick Y and the triggers zoom).
    void SetAnalog(const FKey& Key, float Value);
    // Moves the landmark focus toward a direction (screen, y down); false when nothing lies that way.
    bool Step(int32 Dx, int32 Dy);
    // Centres and zooms in on the focused landmark, or back out to the whole map.
    void ToggleZoomOnSelected();
    void ZoomBy(double Factor, FVector2D AnchorLocal);
    void PanPixels(FVector2D Delta);
    FString SelectedName() const;
    double PixelsPerCm() const;
    HomesteadMap::Vec Center() const;
    double FitPixelsPerCm() const;

    virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
    virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply OnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply OnMouseButtonDoubleClick(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FCursorReply OnCursorQuery(const FGeometry& Geometry, const FPointerEvent& Event) const override;

protected:
    virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(480, 320); }
    virtual bool ComputeVolatility() const override { return true; }
    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
    TWeakObjectPtr<UHomesteadMapComponent> Map;
    TAttribute<bool> UsesGamepad;
    // The painted size; the pan and zoom limits follow it.
    mutable FVector2D Size = FVector2D(480, 320);
    bool bDragging = false;
    FVector2D DragLast = FVector2D::ZeroVector;
    FVector2D DragStart = FVector2D::ZeroVector;
    struct FAxis { float Value = 0; double At = -1; };
    FAxis LeftX, LeftY, RightY, LeftTrigger, RightTrigger;
    double Time = 0;
    FHomesteadMapViewState& State() const;
    HomesteadMap::View MakeView() const;
    void Clamp() const;
    void EnsureState();
    void Reveal(int32 Index);
};
}
