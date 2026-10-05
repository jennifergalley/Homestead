#pragma once

#include "CoreMinimal.h"
#include "HomesteadFrameStyle.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"

namespace HomesteadMenus
{
// The field book's cell border (hairline, inner rule, optional corner brackets; geometry in
// HomesteadFrameStyle::ForEachCellRect) for tiles that are not SMenuButtons: empty pack cells and the
// shop's tabs, buttons and rows. Paint only, a fixed handful of boxes whatever it overlays.
class SHomesteadCellBorder : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SHomesteadCellBorder) : _Brackets(true) {}
        SLATE_ARGUMENT(bool, Brackets)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args)
    {
        bBrackets = Args._Brackets;
        SetVisibility(EVisibility::HitTestInvisible);
    }
    virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }
    virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
        FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle& WidgetStyle, bool) const override
    {
        const FVector2f Size = FVector2f(Geometry.GetLocalSize());
        const FSlateBrush* White = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
        const FLinearColor Tint = WidgetStyle.GetColorAndOpacityTint();
        HomesteadFrameStyle::ForEachCellRect(Size.X, Size.Y,
            [&](HomesteadFrameStyle::ECellPart Part, float X, float Y, float W, float H)
            {
                if (Part == HomesteadFrameStyle::ECellPart::Bracket && !bBrackets) return;
                FSlateDrawElement::MakeBox(Out, LayerId, Geometry.ToPaintGeometry(FVector2f(W, H),
                    FSlateLayoutTransform(FVector2f(X, Y))), White, ESlateDrawEffect::None,
                    HomesteadFrameStyle::CellColorOf(Part, false) * Tint);
            });
        return LayerId + 1;
    }

private:
    bool bBrackets = true;
};
}
