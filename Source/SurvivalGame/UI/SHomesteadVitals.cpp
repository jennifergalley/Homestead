#include "SHomesteadVitals.h"
#include "HomesteadUITheme.h"

#include "../HomesteadController.h"
#include "../Simulation/HomesteadFood.h"
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
HomesteadUITheme::FThemeColor Backing(0.055f, 0.09f, 0.075f, 0.9f);
HomesteadUITheme::FThemeColor Track(0.2f, 0.25f, 0.2f, 1);
HomesteadUITheme::FThemeColor Gold(0.92f, 0.74f, 0.43f, 1);
HomesteadUITheme::FThemeColor Warning(1.0f, 0.67f, 0.48f, 1);
constexpr float IconSize = 40, IconGap = 14, SidePad = 12, BarHeight = 14;
constexpr float BarWidth = SHomesteadVitals::Width - SidePad * 2 - IconSize - IconGap;
// A meal's bar fill and "+N" popup (seconds; the popup rises this many units as it appears).
constexpr double MealFillSeconds = 0.6;
constexpr double MealPopupSeconds = 2.0;
constexpr double MealPopupFadeIn = 0.15;
constexpr double MealPopupFadeOut = 0.6;
constexpr float MealPopupRise = 6.0f;
HomesteadUITheme::FThemeColor Gain(0.72f, 0.90f, 0.56f, 1);
// The bars' fills: in parchment an olive ink for Energy and an ochre for the woodland's Food; the dark book keeps the classic fills.
HomesteadUITheme::FThemeColor EnergyFill(FLinearColor(0.66f, 0.76f, 0.52f, 1), FLinearColor(0.16f, 0.23f, 0.07f, 1), FLinearColor(0.66f, 0.76f, 0.52f, 1));
HomesteadUITheme::FThemeColor FoodFill(FLinearColor(0.77f, 0.66f, 0.37f, 1), FLinearColor(0.42f, 0.25f, 0.05f, 1), FLinearColor(0.77f, 0.66f, 0.37f, 1));
// Low and critical fills: amber, then a muted red (deep enough on parchment to read as a warning).
HomesteadUITheme::FThemeColor LowFill(FLinearColor(1.0f, 0.70f, 0.30f, 1), FLinearColor(0.64f, 0.38f, 0.04f, 1), FLinearColor(1.0f, 0.70f, 0.30f, 1));
HomesteadUITheme::FThemeColor CriticalFill(FLinearColor(0.88f, 0.40f, 0.34f, 1), FLinearColor(0.56f, 0.14f, 0.09f, 1), FLinearColor(0.88f, 0.40f, 0.34f, 1));
// The gentle pulse of a low bar: its opacity eases between these over one period (seconds).
constexpr float PulseMinOpacity = 0.6f;
constexpr double PulsePeriod = 1.8;
// The Well fed chip: a smaller pasty than the bars' icons, in the purse's warm gold.
constexpr float ChipIconSize = 30, ChipTextSize = 17;

const FSlateBrush* White() { return FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")); }
}

FBox2D SHomesteadVitals::LogicalBox(float ViewWidth, bool bFoodRow, bool bWellFedRow)
{
    const int32 Rows = bFoodRow ? 3 : 2;
    const float Height = RowHeight * Rows + RowGap * (Rows - 1) + (bWellFedRow ? RowGap + ChipHeight : 0.0f);
    return FBox2D(FVector2D(ViewWidth - Right - Width, Top), FVector2D(ViewWidth - Right, Top + Height));
}

bool SHomesteadVitals::ShowsFoodRow(const AHomesteadController& Controller) { return !Controller.State().fixedEstate; }

bool SHomesteadVitals::ShowsWellFed(const AHomesteadController& Controller)
{
    return Controller.State().fixedEstate && Controller.Simulation().IsWellFed();
}

FBox2D SHomesteadVitals::LogicalBox(float ViewWidth, const AHomesteadController& Controller)
{
    return LogicalBox(ViewWidth, ShowsFoodRow(Controller), ShowsWellFed(Controller));
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
                // The woodland's hunger; the estate has none (Homestead::Food), so the row goes.
                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, RowGap)
                [
                    SNew(SBox)
                    .Visibility_Lambda([this]() { return Controller.IsValid() && ShowsFoodRow(*Controller) ? EVisibility::Visible : EVisibility::Collapsed; })
                    [ MeterRow(FName(TEXT("bread")), Food, VitalsStyle::FoodFill, 0, TEXT("Food")) ]
                ]
                + SVerticalBox::Slot().AutoHeight()
                [ MeterRow(FName(TEXT("bed")), Energy, VitalsStyle::EnergyFill, 1, TEXT("Energy")) ]
                + SVerticalBox::Slot().AutoHeight().Padding(0, RowGap, 0, 0)
                [ PurseRow() ]
                + SVerticalBox::Slot().AutoHeight().Padding(0, RowGap, 0, 0)
                [ WellFedChip() ]
            ]
        ]
    ];
}

void SHomesteadVitals::Tick(const FGeometry& Geometry, double Time, float Delta)
{
    SCompoundWidget::Tick(Geometry, Time, Delta);
    if (!Controller.IsValid()) return;
    UpdateLowWarnings();
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

int32 SHomesteadVitals::WarningBand(double Value, int32 Previous)
{
    // Down at the lines, back up only a little above them.
    const double Low = Previous >= 1 ? LowAt + RearmMargin : LowAt;
    const double Critical = Previous >= 2 ? CriticalAt + RearmMargin : CriticalAt;
    return Value < Critical ? 2 : Value < Low ? 1 : 0;
}

const TCHAR* SHomesteadVitals::WarningText(int32 Meter, int32 Band)
{
    if (Band <= 0) return TEXT("");
    return Meter == 0 ? (Band >= 2 ? TEXT("Starving") : TEXT("Getting hungry"))
        : (Band >= 2 ? TEXT("Exhausted") : TEXT("Getting tired"));
}

void SHomesteadVitals::UpdateLowWarnings()
{
    const double Values[2] = {Controller->State().hunger, Controller->State().energy};
    const bool Shown[2] = {ShowsFoodRow(*Controller), true};
    for (int32 Meter = 0; Meter < 2; ++Meter)
    {
        const int32 Band = WarningBand(Values[Meter], WarnBand[Meter]);
        // Only a meter worn down across a line speaks, once: not the first look at a loaded game, not a
        // jump from loading or sleeping, not while she's failed or a menu has the screen.
        const bool bWornDown = bWarnPrimed && FMath::Abs(Values[Meter] - WarnLast[Meter]) < JumpLimit;
        if (bWornDown && Shown[Meter] && Band > WarnBand[Meter] && !Controller->IsFailed())
            Controller->PostHudNotice(WarningText(Meter, Band), Band >= 2);
        WarnBand[Meter] = Band;
        WarnLast[Meter] = Values[Meter];
    }
    bWarnPrimed = true;
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
    FSlateFontInfo PopupFont = HomesteadUITheme::Font(TEXT("Bold"), 17);
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
                            // Amber when low, a muted red when critical, pulsing gently while either.
                            SNew(SImage).Image(VitalsStyle::White())
                            .ColorAndOpacity_Lambda([Value, Fill]()
                            {
                                const double Now = Value();
                                if (Now >= LowAt) return FSlateColor(Fill);
                                const FLinearColor Warn = Now < CriticalAt ? VitalsStyle::CriticalFill : VitalsStyle::LowFill;
                                const double Phase = FSlateApplication::Get().GetCurrentTime() / VitalsStyle::PulsePeriod;
                                const float Ease = 0.5f + 0.5f * FMath::Cos(static_cast<float>(2.0 * PI * Phase));
                                return FSlateColor(Warn.CopyWithNewOpacity(FMath::Lerp(VitalsStyle::PulseMinOpacity, 1.0f, Ease)));
                            })
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
                .Font(HomesteadUITheme::Font(TEXT("Bold"), 20))
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
                .Font(HomesteadUITheme::Font(TEXT("Bold"), 17))
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

// Collapsed (and taking no room) unless she is Well fed on the estate; it sits under the purse so the
// bars and balance above it never move. Slate collapses the slot's gap along with it.
TSharedRef<SWidget> SHomesteadVitals::WellFedChip()
{
    return SNew(SBox).WidthOverride(Width).HeightOverride(ChipHeight)
    .Visibility_Lambda([this]() { return Controller.IsValid() && ShowsWellFed(*Controller) ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
    [
        SNew(SOverlay)
        + SOverlay::Slot()
        [ SNew(SImage).Image(VitalsStyle::White()).ColorAndOpacity(VitalsStyle::Backing) ]
        + SOverlay::Slot().Padding(VitalsStyle::SidePad, 0)
        [
            SNew(SHorizontalBox)
            // Centred in the same column as the bars' icons.
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
            [
                SNew(SBox).WidthOverride(VitalsStyle::IconSize).HAlign(HAlign_Center)
                [ SNew(SBox).WidthOverride(VitalsStyle::ChipIconSize).HeightOverride(VitalsStyle::ChipIconSize)[ SNew(SHomesteadIcon).Kind(FName(TEXT("pasty"))) ] ]
            ]
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(VitalsStyle::IconGap, 0, 0, 0)
            [
                SNew(STextBlock)
                .Font(HomesteadUITheme::Font(TEXT("Bold"), VitalsStyle::ChipTextSize))
                .ColorAndOpacity(VitalsStyle::Gold)
                .Text_Lambda([this]()
                {
                    return Controller.IsValid()
                        ? FText::FromString(UTF8_TO_TCHAR(Homestead::Food::WellFedBadge(Controller->State()).c_str())) : FText::GetEmpty();
                })
            ]
        ]
    ];
}
}