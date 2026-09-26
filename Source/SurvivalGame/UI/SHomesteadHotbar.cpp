#include "SHomesteadHotbar.h"

#include "../HomesteadController.h"
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
namespace
{
const FLinearColor Pine(0.055f, 0.09f, 0.075f, 0.96f);
const FLinearColor MutedPine(0.035f, 0.055f, 0.046f, 0.82f);
const FLinearColor Cream(0.93f, 0.93f, 0.84f, 1.0f);
const FLinearColor Gold(0.92f, 0.74f, 0.43f, 1.0f);
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
                return Weak.IsValid() && Weak->SelectedHotbarIndex() == Index ? Gold : Pine;
            })
            .Padding(3)
            [
                SAssignNew(Button, SButton)
                .ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>(TEXT("NoBorder")))
                .ContentPadding(0)
                .ButtonColorAndOpacity_Lambda([Weak = Controller, Index]()
                {
                    if (!Weak.IsValid()) return MutedPine;
                    const auto Snapshot = Weak->HotbarSnapshot();
                    return Snapshot.IsValidIndex(Index) && Snapshot[Index].Available
                        ? Pine : MutedPine;
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
                    SNew(SBox).WidthOverride(HotbarSlotSize).HeightOverride(HotbarSlotSize)
                    [
                        SNew(SOverlay)
                        + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
                        [
                            SNew(SHomesteadIcon)
                            .Kind_Lambda([Weak = Controller, Index]()
                            {
                                if (!Weak.IsValid()) return FName();
                                const auto Snapshot = Weak->HotbarSnapshot();
                                // Pinned food keeps its (faded) icon when the pack runs out.
                                return Snapshot.IsValidIndex(Index) && (Snapshot[Index].Available || Snapshot[Index].Food)
                                    ? Snapshot[Index].Icon : FName();
                            })
                            .Tint_Lambda([Weak = Controller, Index]()
                            {
                                if (!Weak.IsValid()) return FLinearColor(1, 1, 1, 0);
                                const auto Snapshot = Weak->HotbarSnapshot();
                                if (!Snapshot.IsValidIndex(Index)) return FLinearColor(1, 1, 1, 0);
                                return Snapshot[Index].Available ? Gold
                                    : Snapshot[Index].Food ? FLinearColor(Gold.R, Gold.G, Gold.B, 0.3f) : FLinearColor(1, 1, 1, 0);
                            })
                            .Visibility_Lambda([Weak = Controller, Index]()
                            {
                                if (!Weak.IsValid()) return EVisibility::Collapsed;
                                const auto Snapshot = Weak->HotbarSnapshot();
                                return Snapshot.IsValidIndex(Index) && (Snapshot[Index].Available || Snapshot[Index].Food)
                                    ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
                            })
                        ]
                        + SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom)
                        .Padding(0, 0, 5, 2)
                        [
                            // How many of a pinned food are left in the pack.
                            SNew(STextBlock)
                            .Text_Lambda([Weak = Controller, Index]()
                            {
                                if (!Weak.IsValid()) return FText::GetEmpty();
                                const auto Snapshot = Weak->HotbarSnapshot();
                                return Snapshot.IsValidIndex(Index) && Snapshot[Index].Food
                                    ? FText::AsNumber(Snapshot[Index].Count) : FText::GetEmpty();
                            })
                            .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 13))
                            .ColorAndOpacity(Cream)
                            .ShadowOffset(FVector2D(1, 1))
                            .ShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.85f))
                            .Visibility(EVisibility::HitTestInvisible)
                        ]
                        + SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top)
                        .Padding(5, 2, 0, 0)
                        [
                            SNew(STextBlock)
                            .Text(FText::FromString(Index == 9
                                ? TEXT("0") : FString::FromInt(Index + 1)))
                            .Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 13))
                            .ColorAndOpacity(Cream)
                            .ShadowOffset(FVector2D(1, 1))
                            .ShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.85f))
                        ]
                    ]
                ]
            ]
        ];
        SlotButtons.Add(Button);
    }
}
}
