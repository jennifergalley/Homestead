#pragma once

#include "CoreMinimal.h"
#include "Rendering/DrawElements.h"
#include "Widgets/SLeafWidget.h"

namespace HomesteadMenus
{
// A hand-inked tick or cross, drawn from a few strokes so it reads in any theme and font (the serif
// has no tick glyph). Used for a recipe requirement's met / missing mark.
class SHomesteadMark : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SHomesteadMark) : _Met(true), _Color(FLinearColor::Black) {}
        SLATE_ARGUMENT(bool, Met)
        SLATE_ATTRIBUTE(FSlateColor, Color)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args) { bMet = Args._Met; Color = Args._Color; }
    virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(Box, Box); }
    virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
        FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle& Style, bool) const override
    {
        const FVector2f S = FVector2f(Geometry.GetLocalSize());
        const FLinearColor Tint = Color.Get(FSlateColor(FLinearColor::Black)).GetSpecifiedColor() * Style.GetColorAndOpacityTint();
        const auto Stroke = [&](const TArray<FVector2f>& Points)
        {
            FSlateDrawElement::MakeLines(Out, LayerId, Geometry.ToPaintGeometry(), Points,
                ESlateDrawEffect::None, Tint, true, StrokeWidth);
        };
        if (bMet)
            Stroke({FVector2f(S.X * 0.14f, S.Y * 0.54f), FVector2f(S.X * 0.40f, S.Y * 0.80f),
                FVector2f(S.X * 0.88f, S.Y * 0.20f)});
        else
        {
            Stroke({FVector2f(S.X * 0.20f, S.Y * 0.22f), FVector2f(S.X * 0.80f, S.Y * 0.78f)});
            Stroke({FVector2f(S.X * 0.80f, S.Y * 0.22f), FVector2f(S.X * 0.20f, S.Y * 0.78f)});
        }
        return LayerId + 1;
    }
private:
    // Logical pixels: the mark's square and its pen width.
    static constexpr float Box = 22.0f;
    static constexpr float StrokeWidth = 2.6f;
    bool bMet = true;
    TAttribute<FSlateColor> Color;
};
}
