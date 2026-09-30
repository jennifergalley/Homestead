#include "SHomesteadVitals.h"

#include "../HomesteadController.h"
#include "../Simulation/HomesteadShops.h"
#include "SHomesteadHudScale.h"
#include "SHomesteadIcon.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace HomesteadMenus
{
namespace VitalsStyle
{
// The calendar panel's pine, so the stack reads as one column with it.
const FLinearColor Backing(0.055f, 0.09f, 0.075f, 0.9f);
const FLinearColor Track(0.2f, 0.25f, 0.2f, 1);
const FLinearColor Gold(0.92f, 0.74f, 0.43f, 1);
const FLinearColor Warning(1.0f, 0.67f, 0.48f, 1);
constexpr float IconSize = 40, IconGap = 14, SidePad = 12, BarHeight = 14;
constexpr float BarWidth = SHomesteadVitals::Width - SidePad * 2 - IconSize - IconGap;
// A meal's bar fill and "+N" popup (seconds; the popup rises this many units as it appears).
constexpr double MealFillSeconds = 0.6;
constexpr double MealPopupSeconds = 2.0;
constexpr double MealPopupFadeIn = 0.15;
constexpr double MealPopupFadeOut = 0.6;
constexpr float MealPopupRise = 6.0f;
const FLinearColor Gain(0.72f, 0.90f, 0.56f, 1);

const FSlateBrush* White() { return FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")); }
}

FBox2D SHomesteadVitals::LogicalBox(float ViewWidth)
{
    const float Height = RowHeight * 3 + RowGap * 2;
    return FBox2D(FVector2D(ViewWidth - Right - Width, Top), FVector2D(ViewWidth - Right, Top + Height));
}

void SHomesteadVitals::Construct(const FArguments& Args)
{
    Controller = Args._Controller;
    auto Food = [this]() { return Controller.IsValid() ? Controller->State().hunger : 0.0; };
    auto Energy = [this]() { return Controller.IsValid() ? Controller->State().energy : 0.0; };
    ChildSlot
    .HAlign(HAlign_Right)
    .VAlign(VAlign_Top)
    [
        // One unit inside is one Canvas HUD unit, so the stack lines up under the calendar at any size.
        SNew(SHomesteadHudScale)
        [
            SNew(SBox).Padding(0, Top, Right, 0)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [ MeterRow(FName(TEXT("bread")), Food, FLinearColor(0.77f, 0.66f, 0.37f, 1), 0, TEXT("Food")) ]
                + SVerticalBox::Slot().AutoHeight().Padding(0, RowGap, 0, 0)
                [ MeterRow(FName(TEXT("bed")), Energy, FLinearColor(0.66f, 0.76f, 0.52f, 1), 1, TEXT("Energy")) ]
                + SVerticalBox::Slot().AutoHeight().Padding(0, RowGap, 0, 0)
                [ PurseRow() ]
            ]
        ]
    ];
}

void SHomesteadVitals::Tick(const FGeometry& Geometry, double Time, float Delta)
{
    SCompoundWidget::Tick(Geometry, Time, Delta);
    if (!Controller.IsValid()) return;
    const auto& Meal = Controller->LastMealGain();
    // Meals eaten before this stack was built (it's rebuilt with the hotbar) don't replay.
    if (!bMealPrimed) { bMealPrimed = true; MealSerialSeen = Meal.Serial; return; }
    if (Meal.Serial == MealSerialSeen) return;
    MealSerialSeen = Meal.Serial;
    const double Now = FSlateApplication::Get().GetCurrentTime();
    const double Values[2] = {Controller->State().hunger, Controller->State().energy};
    const double Gains[2] = {Meal.Food, Meal.Energy};
    for (int32 Meter = 0; Meter < 2; ++Meter)
    {
        // A full bar gains nothing (or less than one point), and says nothing.
        if (Gains[Meter] < 0.5) continue;
        // Fill on from wherever the bar is showing now, so a second mouthful carries on smoothly, and
        // quick mouthfuls add up in one "+N" rather than replacing each other.
        const double Age = Now - Meals[Meter].StartedAt;
        const bool bShowing = Age >= 0 && Age < VitalsStyle::MealPopupSeconds;
        Meals[Meter].From = Displayed(Meter, Values[Meter] - Gains[Meter]);
        Meals[Meter].Gain = (bShowing ? Meals[Meter].Gain : 0.0) + Gains[Meter];
        Meals[Meter].StartedAt = Now;
    }
}

double SHomesteadVitals::Displayed(int32 Meter, double Actual) const
{
    const double Age = FSlateApplication::Get().GetCurrentTime() - Meals[Meter].StartedAt;
    if (Age >= VitalsStyle::MealFillSeconds || Age < 0) return Actual;
    const float Blend = FMath::InterpEaseOut(0.0f, 1.0f, static_cast<float>(Age / VitalsStyle::MealFillSeconds), 2.0f);
    return FMath::Lerp(Meals[Meter].From, Actual, static_cast<double>(Blend));
}

float SHomesteadVitals::PopupAlpha(int32 Meter) const
{
    const double Age = FSlateApplication::Get().GetCurrentTime() - Meals[Meter].StartedAt;
    if (Age < 0 || Age >= VitalsStyle::MealPopupSeconds) return 0.0f;
    const double In = FMath::Clamp(Age / VitalsStyle::MealPopupFadeIn, 0.0, 1.0);
    const double Out = FMath::Clamp((VitalsStyle::MealPopupSeconds - Age) / VitalsStyle::MealPopupFadeOut, 0.0, 1.0);
    return static_cast<float>(In * Out);
}

TSharedRef<SWidget> SHomesteadVitals::MeterRow(FName Icon, TFunction<double()> Value, FLinearColor Fill, int32 Meter, const TCHAR* Label)
{
    const FString Name(Label);
    FSlateFontInfo PopupFont = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 17);
    PopupFont.OutlineSettings.OutlineSize = 2;
    PopupFont.OutlineSettings.OutlineColor = FLinearColor(0.01f, 0.02f, 0.015f, 0.9f);
    return SNew(SBox).WidthOverride(Width).HeightOverride(RowHeight)
    [
        SNew(SOverlay)
        + SOverlay::Slot()
        [ SNew(SImage).Image(VitalsStyle::White()).ColorAndOpacity(VitalsStyle::Backing) ]
        + SOverlay::Slot().Padding(VitalsStyle::SidePad, 0)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
            [ SNew(SBox).WidthOverride(VitalsStyle::IconSize).HeightOverride(VitalsStyle::IconSize)[ SNew(SHomesteadIcon).Kind(Icon) ] ]
            + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).Padding(VitalsStyle::IconGap, 0, 0, 0)
            [
                SNew(SBox).WidthOverride(VitalsStyle::BarWidth).HeightOverride(VitalsStyle::BarHeight)
                [
                    SNew(SOverlay)
                    + SOverlay::Slot()
                    [ SNew(SImage).Image(VitalsStyle::White()).ColorAndOpacity(VitalsStyle::Track) ]
                    + SOverlay::Slot().HAlign(HAlign_Left)
                    [
                        SNew(SBox)
                        .WidthOverride_Lambda([this, Value, Meter]()
                        { return VitalsStyle::BarWidth * FMath::Clamp(static_cast<float>(Displayed(Meter, Value()) / 100.0), 0.0f, 1.0f); })
                        [
                            SNew(SImage).Image(VitalsStyle::White())
                            .ColorAndOpacity_Lambda([Value, Fill]() { return FSlateColor(Value() < 25 ? VitalsStyle::Warning : Fill); })
                        ]
                    ]
                ]
            ]
        ]
        // "+12 Energy" above the bar's right end, drawn over the backing so nothing moves.
        + SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0, 0, VitalsStyle::SidePad, 0)
        [
            SNew(STextBlock).Font(PopupFont)
            .Visibility_Lambda([this, Meter]() { return PopupAlpha(Meter) > 0 ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
            .ColorAndOpacity_Lambda([this, Meter]() { return FSlateColor(VitalsStyle::Gain.CopyWithNewOpacity(PopupAlpha(Meter))); })
            .RenderTransform_Lambda([this, Meter]()
            {
                const double Age = FSlateApplication::Get().GetCurrentTime() - Meals[Meter].StartedAt;
                const float In = FMath::Clamp(static_cast<float>(Age / VitalsStyle::MealPopupFadeIn), 0.0f, 1.0f);
                return TOptional<FSlateRenderTransform>(FSlateRenderTransform(FVector2f(0.0f, (1.0f - In) * VitalsStyle::MealPopupRise)));
            })
            .Text_Lambda([this, Meter, Name]()
            {
                const int32 Shown = FMath::RoundToInt(static_cast<float>(Meals[Meter].Gain));
                return FText::FromString(FString::Printf(TEXT("+%d %s"), Shown, *Name));
            })
        ]
    ];
}

TSharedRef<SWidget> SHomesteadVitals::PurseRow()
{
    return SNew(SBox).WidthOverride(Width).HeightOverride(RowHeight)
    [
        SNew(SOverlay)
        + SOverlay::Slot()
        [ SNew(SImage).Image(VitalsStyle::White()).ColorAndOpacity(VitalsStyle::Backing) ]
        + SOverlay::Slot().Padding(VitalsStyle::SidePad, 0)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
            [ SNew(SBox).WidthOverride(VitalsStyle::IconSize).HeightOverride(VitalsStyle::IconSize)[ SNew(SHomesteadIcon).Kind(FName(TEXT("coin"))) ] ]
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(VitalsStyle::IconGap, 0, 0, 0)
            [
                SNew(STextBlock)
                .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 20))
                .ColorAndOpacity(VitalsStyle::Gold)
                .Text_Lambda([this]()
                {
                    return Controller.IsValid()
                        ? FText::FromString(UTF8_TO_TCHAR(Homestead::FormatMoney(Controller->State().money).c_str())) : FText::GetEmpty();
                })
            ]
            // The last trade's signed change, fading beside the balance.
            + SHorizontalBox::Slot().FillWidth(1).HAlign(HAlign_Right).VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 17))
                .ColorAndOpacity_Lambda([this]()
                {
                    const float Alpha = Controller.IsValid() ? Controller->WalletDeltaAlpha() : 0.0f;
                    const bool bGain = Controller.IsValid() && Controller->WalletDelta() > 0;
                    const FLinearColor Color = bGain ? FLinearColor(0.72f, 0.90f, 0.56f, 1) : VitalsStyle::Warning;
                    return FSlateColor(FLinearColor(Color.R, Color.G, Color.B, Alpha));
                })
                .Text_Lambda([this]()
                {
                    if (!Controller.IsValid() || Controller->WalletDelta() == 0 || Controller->WalletDeltaAlpha() <= 0) return FText::GetEmpty();
                    return FText::FromString(UTF8_TO_TCHAR(Homestead::FormatMoneyDelta(Controller->WalletDelta()).c_str()));
                })
            ]
        ]
    ];
}
}
