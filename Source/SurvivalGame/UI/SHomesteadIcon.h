#pragma once

#include "CoreMinimal.h"
#include "HomesteadPalette.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SLeafWidget.h"

namespace HomesteadIcons
{
class SURVIVALGAME_API SHomesteadIcon : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SHomesteadIcon)
        : _Kind(FName(TEXT("pack")))
        , _Tint(HomesteadPalette::Brass)
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
        Unknown, Pack, Sort, Craft, Build, Guide, Settings, Appearance, Map,
        Knife, Branch, Stone, Fiber, Berries, Roots, Flowers, Seeds, Hatchet,
        DiggingStick, WateringCan, Water, RoastedRoots, HerbedRoots, Timber, Firewood, Machete,
        Foundation, Wall, Doorway, Roof, Fire, Bed, Chest,
        LinenTunic, LinenApron, LeatherShoes, WovenFootwraps,
        Fur, LinenShirt, LongLinenShirt, Trousers, FurCoat, FurBoots, WovenSandals, TurnShoes,
        SlotTorso, SlotApron, SlotFeet,
        Pasty, Bread, Cheese, Twine, Coin, Shop,
        Scythe, Billhook, Pickaxe, RustedAxeHead, RustedHoeBlade, RustedScytheBlade, RustedBillhookHead,
        RustedPickHead, Hay, Weeds, BrambleCanes, Kindling, ScrapIron, ScrapLead,
        Primroses, Bluebells, WildDaffodils, WildGarlic, OilLamp, OilFlask, PouchArrows, FishingPole, Fish
    };
    TAttribute<FName> Kind{FName(TEXT("pack"))};
    TAttribute<FLinearColor> Tint{HomesteadPalette::Brass};
    TAttribute<float> Desaturation{0.0f};
};
}
using SHomesteadIcon = HomesteadIcons::SHomesteadIcon;
