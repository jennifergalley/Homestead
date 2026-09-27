#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SLeafWidget.h"

namespace HomesteadIcons
{
class SURVIVALGAME_API SHomesteadIcon : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SHomesteadIcon)
        : _Kind(FName(TEXT("pack")))
        , _Tint(FLinearColor(0.92f, 0.74f, 0.43f, 1.0f))
        , _Desaturation(0.0f)
    {}
        SLATE_ATTRIBUTE(FName, Kind)
        SLATE_ATTRIBUTE(FLinearColor, Tint)
        SLATE_ATTRIBUTE(float, Desaturation)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

protected:
    virtual bool ComputeVolatility() const override;
    virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
        int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
    enum class EKind : uint8
    {
        Unknown, Pack, Sort, Craft, Build, Guide, Settings, Appearance,
        Knife, Branch, Stone, Fiber, Berries, Roots, Flowers, Seeds, Hatchet,
        DiggingStick, WateringCan, Water, RoastedRoots, HerbedRoots, Timber, Firewood, Machete,
        Foundation, Wall, Doorway, Roof, Fire, Bed, Chest,
        LinenTunic, LinenApron, LeatherShoes, WovenFootwraps,
        Fur, LinenShirt, LongLinenShirt, Trousers, FurCoat, FurBoots, WovenSandals, TurnShoes,
        SlotTorso, SlotApron, SlotFeet
    };
    TAttribute<FName> Kind{FName(TEXT("pack"))};
    TAttribute<FLinearColor> Tint{FLinearColor(0.92f, 0.74f, 0.43f, 1.0f)};
    TAttribute<float> Desaturation{0.0f};
};
}
using SHomesteadIcon = HomesteadIcons::SHomesteadIcon;
