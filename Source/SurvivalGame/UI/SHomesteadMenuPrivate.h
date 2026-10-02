#pragma once

#include "SHomesteadMenu.h"
#include "HomesteadUITheme.h"
#include "HomesteadNoticeStyle.h"
#include "../Simulation/HomesteadChests.h"
#include "SHomesteadHudScale.h"
#include "SHomesteadIcon.h"
#include "SHomesteadMapView.h"
#include "SHomesteadArrival.h"
#include "../HomesteadMapComponent.h"

#include "Algo/Find.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/NavigationReply.h"
#include "Layout/WidgetPath.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"
#include "Types/NavigationMetaData.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace HomesteadMenus
{
namespace MenuDetail
{
class SMenuButton : public SButton
{
public:
    virtual FReply OnAnalogValueChanged(const FGeometry&, const FAnalogInputEvent&) override
    { return FReply::Unhandled(); }
    // Item tiles open their context menu on a right click.
    TFunction<void()> RightClick;
    virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override
    {
        if (RightClick && Event.GetEffectingButton() == EKeys::RightMouseButton) return FReply::Handled();
        return SButton::OnMouseButtonDown(Geometry, Event);
    }
    virtual FReply OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event) override
    {
        if (RightClick && Event.GetEffectingButton() == EKeys::RightMouseButton) { RightClick(); return FReply::Handled(); }
        return SButton::OnMouseButtonUp(Geometry, Event);
    }
};
class SMenuFocusAnchor : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SMenuFocusAnchor) {}
        SLATE_DEFAULT_SLOT(FArguments, Content)
        SLATE_EVENT(FSimpleDelegate, OnFocused)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args)
    {
        Focused = Args._OnFocused;
        ChildSlot
        [
            SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).Padding(2)
            .BorderBackgroundColor_Lambda([this]() { return HasKeyboardFocus()
                ? FLinearColor(0.92f, 0.74f, 0.43f, 1) : FLinearColor::Transparent; })
            [Args._Content.Widget]
        ];
    }

    virtual bool SupportsKeyboardFocus() const override { return true; }
    virtual FReply OnFocusReceived(const FGeometry&, const FFocusEvent&) override
    { Focused.ExecuteIfBound(); return FReply::Handled(); }
    virtual FReply OnAnalogValueChanged(const FGeometry&, const FAnalogInputEvent&) override
    { return FReply::Unhandled(); }
private:
    FSimpleDelegate Focused;
};
// A recipe square's craft progress: translucent white rising from the bottom with a bright edge,
// then a white flash as the cycle completes.
class SHomesteadCraftFill : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SHomesteadCraftFill) {}
        SLATE_ATTRIBUTE(float, Progress)
        SLATE_ATTRIBUTE(float, Flash)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args) { Progress = Args._Progress; Flash = Args._Flash; }
    virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }
    virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
        FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle& Style, bool) const override
    {
        const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
        const FVector2D Size = Geometry.GetLocalSize();
        const float Fill = FMath::Clamp(Progress.Get(0.0f), 0.0f, 1.0f);
        const float Bright = FMath::Clamp(Flash.Get(0.0f), 0.0f, 1.0f);
        const FLinearColor Tint = Style.GetColorAndOpacityTint();
        if (Fill > 0)
        {
            // Ease in so the first moments of a hold already show.
            const float Height = Size.Y * FMath::InterpEaseOut(0.0f, 1.0f, Fill, 1.6f);
            FSlateDrawElement::MakeBox(Out, LayerId, Geometry.ToPaintGeometry(FVector2D(Size.X, Height),
                FSlateLayoutTransform(FVector2D(0, Size.Y - Height))), White, ESlateDrawEffect::None,
                FLinearColor(1, 1, 1, 0.38f) * Tint);
            const float Edge = FMath::Min(3.0f, Height);
            FSlateDrawElement::MakeBox(Out, LayerId + 1, Geometry.ToPaintGeometry(FVector2D(Size.X, Edge),
                FSlateLayoutTransform(FVector2D(0, Size.Y - Height))), White, ESlateDrawEffect::None,
                FLinearColor(1, 1, 1, 0.9f) * Tint);
        }
        if (Bright > 0)
            FSlateDrawElement::MakeBox(Out, LayerId + 1, Geometry.ToPaintGeometry(), White, ESlateDrawEffect::None,
                FLinearColor(1, 1, 1, 0.75f * Bright * Bright) * Tint);
        return LayerId + 2;
    }
private:
    TAttribute<float> Progress;
    TAttribute<float> Flash;
};
inline HomesteadUITheme::FThemeColor Ink(0.93f, 0.93f, 0.84f);
inline HomesteadUITheme::FThemeColor Muted(0.71f, 0.77f, 0.69f);
inline HomesteadUITheme::FThemeColor MenuGold(0.92f, 0.74f, 0.43f);
// Panels are translucent so the book reads as laid over the living world.
inline HomesteadUITheme::FThemeColor MenuPine(0.025f, 0.05f, 0.038f, 0.6f);
inline HomesteadUITheme::FThemeColor PopupPine(0.025f, 0.05f, 0.038f, 0.88f);
inline HomesteadUITheme::FThemeColor PineInk(0.025f, 0.05f, 0.038f, 1.0f);
inline HomesteadUITheme::FThemeColor Selected(0.09f, 0.14f, 0.105f, 0.78f);
// The Appearance page's camera input (degrees per second, degrees per pixel dragged).
namespace MenuAppearanceInput
{
constexpr float KeyYawRate = 90.0f, KeyPitchRate = 45.0f;
constexpr float StickYawRate = 120.0f, StickPitchRate = 60.0f;
constexpr float StickDeadZone = 0.2f;
constexpr float DragYawPerPixel = 0.35f, DragPitchPerPixel = 0.2f;
}
// Names offered in the chest's naming dialog, so a controller player can name one without typing.
namespace MenuChestNames
{
inline const TCHAR* const Suggestions[] = {TEXT("Pantry"), TEXT("Tools"), TEXT("Seeds and garden"), TEXT("Timber and stone")};
}
// The dye chooser's colour chips (display colours for Homestead::DyeName's four plant dyes).
namespace MenuDyeStyle
{
constexpr int32 Count = 4;
inline FLinearColor Swatch(int32 Dye)
{
    static const FLinearColor Values[] = {
        FLinearColor(0.23f, 0.29f, 0.14f), FLinearColor(0.36f, 0.08f, 0.11f),
        FLinearColor(0.20f, 0.25f, 0.33f), FLinearColor(0.70f, 0.60f, 0.42f)};
    return Values[FMath::Clamp(Dye, 0, Count - 1)];
}
}
// The notice card over the book (logical book units and seconds): a small parchment slip with a
// double-ruled frame and Garamond ink, so it reads as a note laid on the book, not a dialog. Tuned
// so a one-line notice reads at a glance and is gone before it gets in the way; errors linger a little.
namespace MenuNoticeStyle
{
constexpr float MaxWidth = 700.0f;
constexpr float FontSize = 25.0f;
// Outer frame padding plus the inner rule's padding, each side of the text.
constexpr float TextInset = 2.0f * (5.0f + 22.0f);
constexpr float BottomInset = 28.0f;
constexpr float GapBelowTabs = 14.0f;
// Clearance kept between the card and the focused control.
constexpr float Clearance = 8.0f;
constexpr double Seconds = 2.6;
constexpr double ErrorSeconds = 3.8;
constexpr double FadeInSeconds = 0.14;
constexpr double FadeOutSeconds = 0.35;
constexpr float RiseDistance = 8.0f;
// Aged paper, iron-gall brown ink, and a rust ink for things that went wrong (shared with the HUD's
// world notices through HomesteadNoticeStyle).
constexpr FLinearColor Paper = HomesteadNoticeStyle::Paper;
constexpr FLinearColor InkBrown = HomesteadNoticeStyle::InkBrown;
constexpr FLinearColor RustInk = HomesteadNoticeStyle::RustInk;
inline const FSlateBrush& CardBrush()
{
    static const FSlateRoundedBoxBrush Brush(Paper, 6.0f, HomesteadNoticeStyle::Frame, HomesteadNoticeStyle::FrameWidth);
    return Brush;
}
inline const FSlateBrush& ErrorCardBrush()
{
    static const FSlateRoundedBoxBrush Brush(Paper, 6.0f, RustInk, HomesteadNoticeStyle::FrameWidth);
    return Brush;
}
// The inner rule of the double frame.
inline const FSlateBrush& RuleBrush()
{
    static const FSlateRoundedBoxBrush Brush(FLinearColor::Transparent, 3.0f,
        InkBrown.CopyWithNewOpacity(HomesteadNoticeStyle::RuleOpacity), HomesteadNoticeStyle::RuleWidth);
    return Brush;
}
inline const FSlateBrush& ShadowBrush()
{
    static const FSlateRoundedBoxBrush Brush(HomesteadNoticeStyle::Shadow, 8.0f);
    return Brush;
}
}
constexpr float ItemCellWidth = 76;
inline float LogicalBookWidth() { return static_cast<float>(HomesteadMenus::FullScreenLogicalSize().X); }
inline float LogicalBookHeight() { return static_cast<float>(HomesteadMenus::FullScreenLogicalSize().Y); }
inline float PortraitColumnWidth()
{
    return FMath::Min(420.0f, 240.0f + (LogicalBookWidth() - 1280.0f) * 0.14f);
}
constexpr float AppearancePanelWidth = 470.0f;
inline float DetailsColumnWidth()
{
    return FMath::Min(560.0f, 340.0f + (LogicalBookWidth() - 1280.0f) * 0.18f);
}
inline const FButtonStyle& MenuButtonStyle()
{
    static const FButtonStyle Style = FButtonStyle()
        .SetNormal(FSlateColorBrush(FLinearColor::White))
        .SetHovered(FSlateColorBrush(FLinearColor(1.12f, 1.12f, 1.12f, 1)))
        .SetPressed(FSlateColorBrush(FLinearColor(0.85f, 0.85f, 0.85f, 1)))
        .SetDisabled(FSlateColorBrush(FLinearColor(0.65f, 0.65f, 0.65f, 1)))
        .SetNormalPadding(FMargin(0)).SetPressedPadding(FMargin(0));
    return Style;
}
// A pale track with a large, easy-to-grab round handle.
inline const FSliderStyle& MenuSliderStyle()
{
    static const FSliderStyle Style = FSliderStyle()
        .SetNormalBarImage(FSlateRoundedBoxBrush(FLinearColor(0.93f, 0.93f, 0.88f, 1), 5.0f))
        .SetHoveredBarImage(FSlateRoundedBoxBrush(FLinearColor::White, 5.0f))
        .SetDisabledBarImage(FSlateRoundedBoxBrush(FLinearColor(0.6f, 0.6f, 0.58f, 0.6f), 5.0f))
        .SetNormalThumbImage(FSlateRoundedBoxBrush(MenuGold, 17.0f, PineInk, 2.0f, FVector2f(34, 34)))
        .SetHoveredThumbImage(FSlateRoundedBoxBrush(FLinearColor(1.0f, 0.84f, 0.55f), 17.0f, PineInk, 2.0f, FVector2f(38, 38)))
        .SetDisabledThumbImage(FSlateRoundedBoxBrush(Muted, 17.0f, PineInk, 2.0f, FVector2f(34, 34)))
        .SetBarThickness(10.0f);
    return Style;
}
inline bool IsAudioSetting(int32 Id) { return (Id >= 5 && Id <= 7) || Id == 16; }
inline int32 AudioSliderSlot(int32 Id) { return Id == 16 ? 3 : Id - 5; }
// Display colours for the Appearance swatches (hair colour, skin, eyes, tunic dye).
inline FLinearColor AppearanceSwatchColor(int32 Id, int32 Value)
{
    static const TCHAR* Hair[] = {TEXT("6B3E26"), TEXT("3B2A20"), TEXT("161312"), TEXT("A0522D"), TEXT("D8B878")};
    static const TCHAR* Skin[] = {TEXT("D9A98A"), TEXT("C68B66"), TEXT("6E4631"), TEXT("F2D5C2")};
    static const TCHAR* Eyes[] = {TEXT("5E8FB0"), TEXT("5A8A4E"), TEXT("8A6A3A"), TEXT("8C939A")};
    static const TCHAR* Tunic[] = {TEXT("6B7A45"), TEXT("7A2F45"), TEXT("566A80"), TEXT("CDBE9A")};
    const auto Pick = [Value](const TCHAR* const* Values, int32 Count)
    { return FLinearColor(FColor::FromHex(Values[FMath::Clamp(Value, 0, Count - 1)])); };
    switch (Id)
    {
    case 1: return Pick(Hair, UE_ARRAY_COUNT(Hair));
    case 2: return Pick(Skin, UE_ARRAY_COUNT(Skin));
    case 3: return Pick(Eyes, UE_ARRAY_COUNT(Eyes));
    case 4: return Pick(Tunic, UE_ARRAY_COUNT(Tunic));
    default: return FLinearColor::White;
    }
}
inline const TCHAR* Tabs[] = {TEXT("Inventory"), TEXT("Craft"), TEXT("Build"), TEXT("Guidebook"),
    TEXT("Settings"), TEXT("Credits"), TEXT("Appearance"), TEXT("Map")};
inline const TCHAR* TabIcons[] = {TEXT("pack"), TEXT("craft"), TEXT("build"), TEXT("guide"),
    TEXT("settings"), TEXT("credits"), TEXT("appearance"), TEXT("map")};
inline FName RequirementIcon(Homestead::Item Item)
{
    const int32 Index = static_cast<int32>(Item);
    if (Index < 0 || Index >= Homestead::ItemCount)
    {
        UE_LOG(LogTemp, Error, TEXT("Crafting requirement has no known item icon: %d"), Index);
        return NAME_None;
    }
    return FName(UTF8_TO_TCHAR(Homestead::ItemIcon(Item)));
}
inline const TCHAR* RecipeIcons[] = {TEXT("hatchet"), TEXT("digging-stick"), TEXT("scythe"), TEXT("billhook"),
    TEXT("pickaxe"), TEXT("roasted-roots"), TEXT("herbed-roots"), TEXT("firewood")};
inline const TCHAR* PieceIcons[] = {TEXT("foundation"), TEXT("wall"), TEXT("doorway"), TEXT("roof"),
    TEXT("fire"), TEXT("bed"), TEXT("chest")};
// The legacy apron still works but has no slot of its own here; it shows under the Top it ties over.
constexpr Homestead::EquipmentSlot VisibleEquipmentSlots[] = {
    Homestead::EquipmentSlot::Torso, Homestead::EquipmentSlot::Legs, Homestead::EquipmentSlot::Outer,
    Homestead::EquipmentSlot::Feet};
constexpr int32 VisibleEquipmentSlotCount = UE_ARRAY_COUNT(VisibleEquipmentSlots);
inline const TCHAR* EquipmentSlotNames[] = {TEXT("Top"), TEXT("Legs"), TEXT("Coat"), TEXT("Feet")};
inline const TCHAR* EquipmentSlotIcons[] = {TEXT("slot-torso"), TEXT("trousers"), TEXT("fur-coat"), TEXT("slot-feet")};
// The pages the tab bar, LB/RB and Ctrl+Tab cycle through. Page 3 (the old Guidebook) is retired but
// keeps its number, so the other page IDs (and anything that opens them) are unchanged.
constexpr int32 FieldBookPages[] = {0, 1, 2, 7, 6};

inline int32 ShiftFieldBookPage(int32 Page, int32 Direction)
{
    int32 Index = 0;
    for (int32 I = 0; I < UE_ARRAY_COUNT(FieldBookPages); ++I)
        if (FieldBookPages[I] == Page) { Index = I; break; }
    return FieldBookPages[(Index + UE_ARRAY_COUNT(FieldBookPages) + Direction) % UE_ARRAY_COUNT(FieldBookPages)];
}
}

using MenuDetail::SMenuButton;
using MenuDetail::SMenuFocusAnchor;
using MenuDetail::SHomesteadCraftFill;
using MenuDetail::Ink;
using MenuDetail::Muted;
using MenuDetail::MenuGold;
using MenuDetail::MenuPine;
using MenuDetail::PopupPine;
using MenuDetail::PineInk;
using MenuDetail::Selected;
namespace MenuNoticeStyle = MenuDetail::MenuNoticeStyle;
namespace MenuAppearanceInput = MenuDetail::MenuAppearanceInput;
namespace MenuChestNames = MenuDetail::MenuChestNames;
namespace MenuDyeStyle = MenuDetail::MenuDyeStyle;
using MenuDetail::ItemCellWidth;
using MenuDetail::LogicalBookWidth;
using MenuDetail::LogicalBookHeight;
using MenuDetail::PortraitColumnWidth;
using MenuDetail::AppearancePanelWidth;
using MenuDetail::DetailsColumnWidth;
using MenuDetail::MenuButtonStyle;
using MenuDetail::MenuSliderStyle;
using MenuDetail::IsAudioSetting;
using MenuDetail::AudioSliderSlot;
using MenuDetail::AppearanceSwatchColor;
using MenuDetail::Tabs;
using MenuDetail::TabIcons;
using MenuDetail::RequirementIcon;
using MenuDetail::RecipeIcons;
using MenuDetail::PieceIcons;
using MenuDetail::VisibleEquipmentSlots;
using MenuDetail::VisibleEquipmentSlotCount;
using MenuDetail::EquipmentSlotNames;
using MenuDetail::EquipmentSlotIcons;
using MenuDetail::FieldBookPages;
using MenuDetail::ShiftFieldBookPage;
}
