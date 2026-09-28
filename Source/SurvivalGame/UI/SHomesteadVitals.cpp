#include "SHomesteadVitals.h"

#include "../HomesteadController.h"
#include "../Simulation/HomesteadShops.h"
#include "SHomesteadIcon.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace HomesteadMenus
{
namespace
{
const FLinearColor Backing(0.025f, 0.045f, 0.035f, 0.78f);
const FLinearColor Track(0.2f, 0.25f, 0.2f, 1);
const FLinearColor Gold(0.92f, 0.74f, 0.43f, 1);
const FLinearColor Warning(1.0f, 0.67f, 0.48f, 1);
constexpr float IconSize = 30, BarWidth = 150, BarHeight = 8;

const FSlateBrush* White() { return FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")); }
}

FBox2D SHomesteadVitals::LogicalBox(float ViewHeight)
{
    const float Height = RowHeight * 3 + RowGap * 2;
    return FBox2D(FVector2D(Left, ViewHeight - Bottom - Height), FVector2D(Left + Width, ViewHeight - Bottom));
}

void SHomesteadVitals::Construct(const FArguments& Args)
{
    Controller = Args._Controller;
    auto Food = [this]() { return Controller.IsValid() ? Controller->State().hunger : 0.0; };
    auto Energy = [this]() { return Controller.IsValid() ? Controller->State().energy : 0.0; };
    ChildSlot
    .HAlign(HAlign_Left)
    .VAlign(VAlign_Bottom)
    .Padding(Left, 0, 0, Bottom)
    [
        // Scales exactly as the hotbar does, so the two stay in proportion.
        SNew(SScaleBox).Stretch(EStretch::UserSpecified)
        .UserSpecifiedScale_Lambda([]()
        {
            const FViewport* Viewport = GEngine && GEngine->GameViewport ? GEngine->GameViewport->Viewport : nullptr;
            return Viewport ? FMath::Min(1.0f, 1080.0f / FMath::Max(720, Viewport->GetSizeXY().Y)) : 1.0f;
        })
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()
            [ MeterRow(FName(TEXT("bread")), Food, FLinearColor(0.77f, 0.66f, 0.37f, 1)) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, RowGap, 0, 0)
            [ MeterRow(FName(TEXT("bed")), Energy, FLinearColor(0.66f, 0.76f, 0.52f, 1)) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, RowGap, 0, 0)
            [ PurseRow() ]
        ]
    ];
}

TSharedRef<SWidget> SHomesteadVitals::MeterRow(FName Icon, TFunction<double()> Value, FLinearColor Fill)
{
    return SNew(SBox).WidthOverride(Width).HeightOverride(RowHeight)
    [
        SNew(SOverlay)
        + SOverlay::Slot()
        [ SNew(SImage).Image(White()).ColorAndOpacity(Backing) ]
        + SOverlay::Slot().Padding(6, 0, 12, 0)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
            [ SNew(SBox).WidthOverride(IconSize).HeightOverride(IconSize)[ SNew(SHomesteadIcon).Kind(Icon) ] ]
            + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).Padding(10, 0, 0, 0)
            [
                SNew(SBox).WidthOverride(BarWidth).HeightOverride(BarHeight)
                [
                    SNew(SOverlay)
                    + SOverlay::Slot()
                    [ SNew(SImage).Image(White()).ColorAndOpacity(Track) ]
                    + SOverlay::Slot().HAlign(HAlign_Left)
                    [
                        SNew(SBox)
                        .WidthOverride_Lambda([Value]() { return BarWidth * FMath::Clamp(static_cast<float>(Value() / 100.0), 0.0f, 1.0f); })
                        [
                            SNew(SImage).Image(White())
                            .ColorAndOpacity_Lambda([Value, Fill]() { return FSlateColor(Value() < 25 ? Warning : Fill); })
                        ]
                    ]
                ]
            ]
        ]
    ];
}

TSharedRef<SWidget> SHomesteadVitals::PurseRow()
{
    return SNew(SBox).WidthOverride(Width).HeightOverride(RowHeight)
    [
        SNew(SOverlay)
        + SOverlay::Slot()
        [ SNew(SImage).Image(White()).ColorAndOpacity(Backing) ]
        + SOverlay::Slot().Padding(6, 0, 12, 0)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
            [ SNew(SBox).WidthOverride(IconSize).HeightOverride(IconSize)[ SNew(SHomesteadIcon).Kind(FName(TEXT("coin"))) ] ]
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10, 0, 0, 0)
            [
                SNew(STextBlock)
                .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 15))
                .ColorAndOpacity(Gold)
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
                .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 13))
                .ColorAndOpacity_Lambda([this]()
                {
                    const float Alpha = Controller.IsValid() ? Controller->WalletDeltaAlpha() : 0.0f;
                    const bool bGain = Controller.IsValid() && Controller->WalletDelta() > 0;
                    const FLinearColor Color = bGain ? FLinearColor(0.72f, 0.90f, 0.56f, 1) : Warning;
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
