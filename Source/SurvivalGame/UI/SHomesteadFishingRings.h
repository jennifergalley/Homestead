#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

class AHomesteadController;

namespace HomesteadMenus
{
// Fishing cues drawn on the water around the float (harder-bob-fishing): one ring closes onto the
// float while the fish has it under and a click is due, and a faint ripple spreads from a nibble. World circles are projected through the player's camera, so they lie on
// the water in perspective. Paints only while a cast or its ripples are live.
class SHomesteadFishingRings : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SHomesteadFishingRings) {}
        SLATE_ARGUMENT(TWeakObjectPtr<AHomesteadController>, Controller)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args);
protected:
    virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }
    virtual bool ComputeVolatility() const override { return true; }
    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling,
        FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override;
private:
    struct FRipple
    {
        double Start = 0.0;
        FVector At = FVector::ZeroVector;
        float Reach = 0.0f, Life = 1.0f, Alpha = 1.0f;
    };
    void Spawn(double Now, const FVector& At, float Reach, float Life, float Alpha) const;
    TWeakObjectPtr<AHomesteadController> Controller;
    // Edge tracking across paints; Slate paints this leaf once per frame.
    mutable TArray<FRipple> Ripples;
    mutable FVector LastBob = FVector::ZeroVector;
    mutable float LastNibble = 0.0f;
};
}
