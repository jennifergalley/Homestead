#include "SHomesteadHotbar.h"
#include "HomesteadUITheme.h"
#include "Widgets/Notifications/SProgressBar.h"

#include "../HomesteadController.h"
#include "HomesteadPalette.h"
#include "SHomesteadIcon.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace HomesteadMenus
{
namespace HotbarStyle
{
HomesteadUITheme::FThemeColor Pine(0.055f, 0.09f, 0.075f, 0.96f);
HomesteadUITheme::FThemeColor MutedPine(0.035f, 0.055f, 0.046f, 0.82f);
const FLinearColor& Cream = HomesteadPalette::Cream;
const FLinearColor& Gold = HomesteadPalette::Brass;
constexpr float HotbarSlotSize = 64.0f;
}

void SHomesteadHotbar::Construct(const FArguments& Args)
{
    Controller = Args._Controller;
    TSharedPtr<SHorizontalBox> Slots;
    ChildSlot
    [
        SAssignNew(Slots, SHorizontalBox)
    ];

    for (int32 Index = 0; Index < 10; ++Index)
    {
        TSharedPtr<SButton> Button;
        Slots->AddSlot().AutoWidth().Padding(3, 0)
        [
            SNew(SBorder)
            .BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
            .BorderBackgroundColor_Lambda([Weak = Controller, Index]()
            {
                return Weak.IsValid() && Weak->SelectedHotbarIndex() == Index ? HotbarStyle::Gold : HotbarStyle::Pine;
            })
            .Padding(3)
            [
                SAssignNew(Button, SButton)
                .ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>(TEXT("NoBorder")))
                .ContentPadding(0)
                .ButtonColorAndOpacity_Lambda([Weak = Controller, Index]()
                {
                    if (!Weak.IsValid()) return HotbarStyle::MutedPine;
                    const auto Snapshot = Weak->HotbarSnapshot();
                    return Snapshot.IsValidIndex(Index) && Snapshot[Index].Available
                        ? HotbarStyle::Pine : HotbarStyle::MutedPine;
                })
                .OnClicked_Lambda([Weak = Controller, Index]()
                {
                    if (Weak.IsValid()) Weak->SelectHotbarSlot(Index);
                    return FReply::Handled();
                })
                .OnHovered_Lambda([Weak = Controller, Index]()
                {
                    if (Weak.IsValid()) Weak->HoverHotbarSlot(Index);
                })
                .OnUnhovered_Lambda([Weak = Controller]()
                {
                    if (Weak.IsValid()) Weak->HoverHotbarSlot(INDEX_NONE);
                })
                [
                    // Genre-typical 1080p slot size (Stardew and Valheim use roughly 64-70 px).
                    SNew(SBox).WidthOverride(HotbarStyle::HotbarSlotSize).HeightOverride(HotbarStyle::HotbarSlotSize)
                    [
                        SNew(SOverlay)
                        + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
                        [
                            SNew(SHomesteadIcon)
                            .Kind_Lambda([Weak = Controller, Index]()
                            {
                                if (!Weak.IsValid()) return FName();
                                const auto Snapshot = Weak->HotbarSnapshot();
                                // The cell's stack (the hotbar is the first row of her pack).
                                return Snapshot.IsValidIndex(Index) && Snapshot[Index].Available ? Snapshot[Index].Icon : FName();
                            })
                            .Tint_Lambda([Weak = Controller, Index]()
                            {
                                if (!Weak.IsValid()) return FLinearColor(1, 1, 1, 0);
                                const auto Snapshot = Weak->HotbarSnapshot();
                                if (!Snapshot.IsValidIndex(Index)) return FLinearColor(1, 1, 1, 0);
                                return Snapshot[Index].Available ? HotbarStyle::Gold : FLinearColor(1, 1, 1, 0);
                            })
                            .Visibility_Lambda([Weak = Controller, Index]()
                            {
                                if (!Weak.IsValid()) return EVisibility::Collapsed;
                                const auto Snapshot = Weak->HotbarSnapshot();
                                return Snapshot.IsValidIndex(Index) && Snapshot[Index].Available
                                    ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
                            })
                        ]
                        + SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom)
                        .Padding(0, 0, 5, 2)
                        [
                            // The stack's count (tools and garments are single).
                            SNew(STextBlock)
                            .Text_Lambda([Weak = Controller, Index]()
                            {
                                if (!Weak.IsValid()) return FText::GetEmpty();
                                const auto Snapshot = Weak->HotbarSnapshot();
                                return Snapshot.IsValidIndex(Index) && (Snapshot[Index].Food || Snapshot[Index].Seed || Snapshot[Index].Material) && Snapshot[Index].Available
                                    ? FText::AsNumber(Snapshot[Index].Count) : FText::GetEmpty();
                            })
                            .Font(HomesteadUITheme::Font(TEXT("Bold"), 13))
                            .ColorAndOpacity(HotbarStyle::Cream)
                            .ShadowOffset(FVector2D(1, 1))
                            .ShadowColorAndOpacity(HomesteadUITheme::Themed(FLinearColor(0, 0, 0, 0.85f)))
                            .Visibility(EVisibility::HitTestInvisible)
                        ]
                        + SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top)
                        .Padding(0, 1, 5, 0)
                        [
                            // A seed slot that can switch to other seed in her pack (the seed pouch).
                            SNew(SBox).WidthOverride(12).HeightOverride(12)
                            .Visibility_Lambda([Weak = Controller, Index]()
                            {
                                if (!Weak.IsValid()) return EVisibility::Collapsed;
                                const auto Snapshot = Weak->HotbarSnapshot();
                                return Snapshot.IsValidIndex(Index) && Snapshot[Index].Pouch
                                    ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
                            })
                            [
                                SNew(SHomesteadIcon).Kind(FName(TEXT("pouch-arrows")))
                                .Tint_Lambda([]()
                                {
                                    // Brass on the slot's face, selected or not (the selection is the frame round it).
                                    return FLinearColor(HotbarStyle::Gold);
                                })
                            ]
                        ]
                        + SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Bottom)
                        .Padding(6, 0, 6, 3)
                        [
                            // The lamp's oil: a thin amber bar that shortens as it burns, red when low.
                            SNew(SBox).HeightOverride(4)
                            .Visibility_Lambda([Weak = Controller, Index]()
                            {
                                if (!Weak.IsValid()) return EVisibility::Collapsed;
                                const auto Snapshot = Weak->HotbarSnapshot();
                                return Snapshot.IsValidIndex(Index) && Snapshot[Index].Fill >= 0.0f ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
                            })
                            [
                                SNew(SBorder)
                                .BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
                                .BorderBackgroundColor(HomesteadUITheme::Themed(FLinearColor(0.02f, 0.03f, 0.025f, 0.85f)))
                                .Padding(0)
                                [
                                    SNew(SProgressBar)
                                    .Percent_Lambda([Weak = Controller, Index]()
                                    {
                                        if (!Weak.IsValid()) return TOptional<float>(0.0f);
                                        const auto Snapshot = Weak->HotbarSnapshot();
                                        return TOptional<float>(Snapshot.IsValidIndex(Index) ? FMath::Clamp(Snapshot[Index].Fill, 0.0f, 1.0f) : 0.0f);
                                    })
                                    .FillColorAndOpacity_Lambda([Weak = Controller, Index]()
                                    {
                                        const auto Snapshot = Weak.IsValid() ? Weak->HotbarSnapshot() : TArray<FHomesteadHotbarSlot>();
                                        const bool bLow = Snapshot.IsValidIndex(Index) && Snapshot[Index].Fill < 1.0f / 6.0f;
                                        const bool bWater = Snapshot.IsValidIndex(Index) && Snapshot[Index].Tool == Homestead::Item::WateringCan;
                                        if (bWater) return FSlateColor(bLow ? FLinearColor(0.9f, 0.32f, 0.2f) : FLinearColor(0.36f, 0.66f, 0.92f));
                                        return FSlateColor(bLow ? FLinearColor(0.9f, 0.32f, 0.2f) : FLinearColor(0.98f, 0.68f, 0.28f));
                                    })
                                    .BackgroundImage(FCoreStyle::Get().GetBrush(TEXT("NoBrush")))
                                    .FillImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
                                ]
                            ]
                        ]
                        + SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top)
                        .Padding(5, 2, 0, 0)
                        [
                            SNew(STextBlock)
                            .Text(FText::FromString(Index == 9
                                ? TEXT("0") : FString::FromInt(Index + 1)))
                            .Font(HomesteadUITheme::KeyFont(TEXT("Bold"), 13))
                            .ColorAndOpacity(HotbarStyle::Cream)
                            .ShadowOffset(FVector2D(1, 1))
                            .ShadowColorAndOpacity(HomesteadUITheme::Themed(FLinearColor(0, 0, 0, 0.85f)))
                        ]
                    ]
                ]
            ]
        ];
        SlotButtons.Add(Button);
    }
}
}
