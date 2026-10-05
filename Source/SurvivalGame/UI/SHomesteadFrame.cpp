#include "SHomesteadFrame.h"
#include "HomesteadFrameStyle.h"

#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace HomesteadMenus
{
void SHomesteadFrame::Construct(const FArguments& Args)
{
    ChildSlot[ Args._Content.Widget ];
    bGrand = Args._Grand;
}

int32 SHomesteadFrame::OnPaint(const FPaintArgs& Paint, const FGeometry& Geometry, const FSlateRect& CullingRect,
    FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
    const int32 ChildLayer = SCompoundWidget::OnPaint(Paint, Geometry, CullingRect, Out, LayerId, Style, bParentEnabled);
    const FSlateBrush* White = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
    const FVector2f Size = FVector2f(Geometry.GetLocalSize());
    const FLinearColor Tint = Style.GetColorAndOpacityTint();
    const auto Draw = [&](HomesteadFrameStyle::EPart Part, float X, float Y, float W, float H)
    {
        FSlateDrawElement::MakeBox(Out, ChildLayer, Geometry.ToPaintGeometry(FVector2f(W, H),
            FSlateLayoutTransform(FVector2f(X, Y))), White, ESlateDrawEffect::None,
            HomesteadFrameStyle::ColorOf(Part) * Tint);
    };
    if (bGrand) HomesteadFrameStyle::ForEachGrandRect(Size.X, Size.Y, Draw);
    else HomesteadFrameStyle::ForEachRect(Size.X, Size.Y, Draw);
    return ChildLayer + 1;
}
}
