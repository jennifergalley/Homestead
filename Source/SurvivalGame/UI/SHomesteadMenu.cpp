#include "SHomesteadMenu.h"
#include "SHomesteadMenuPrivate.h"

namespace HomesteadMenus
{

TSharedRef<SWidget> SHomesteadMenu::Text(const FString& Value, int32 Size) const
{
    return SNew(STextBlock).Text(FText::FromString(Value)).ColorAndOpacity(Ink)
        .Font(FCoreStyle::GetDefaultFontStyle("Regular", Size)).AutoWrapText(true);
}

void SHomesteadMenu::Construct(const FArguments& Args)
{
    Controller = Args._Controller;
    ChildSlot
    [
        SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
        .BorderBackgroundColor_Lambda([this]() { return FLinearColor(0.015f, 0.03f, 0.02f, SeenPage == 6 ? 0.0f : 0.28f); }).Padding(0)
        [
            SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
            [
                SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)
                .WidthOverride_Lambda([]() { return FOptionalSize(LogicalBookWidth()); })
                .HeightOverride_Lambda([]() { return FOptionalSize(LogicalBookHeight()); })
                [
                    SNew(SBox)
                    .WidthOverride_Lambda([]() { return FOptionalSize(LogicalBookWidth()); })
                    .HeightOverride_Lambda([]() { return FOptionalSize(LogicalBookHeight()); })
                    [
                        SAssignNew(BookOverlay, SOverlay)
                    + SOverlay::Slot().Padding(24, 16)
                    [
                        SAssignNew(Root, SVerticalBox)
                        + SVerticalBox::Slot().AutoHeight()
                        [
                            SAssignNew(TabBar, SHorizontalBox)
                        ]
                        // Notices float over the page in the card below (NoticeSlot), so nothing here moves.
                        + SVerticalBox::Slot().FillHeight(1)
                        [ SAssignNew(ContentHost, SBox).Clipping(EWidgetClipping::ClipToBounds) ]
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 0)
                        [
                            SNew(SBorder).Visibility(EVisibility::Collapsed)
                            .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                            .BorderBackgroundColor(MenuPine).Padding(12, 10)
                            [
                                SNew(STextBlock).AutoWrapText(true).ColorAndOpacity(MenuGold)
                                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 16))
                                .Text_Lambda([this]() { return FText::FromString(Footer()); })
                            ]
                        ]
                    ]
                        + SOverlay::Slot()[ SAssignNew(ModalHost, SBox).Visibility(EVisibility::Collapsed) ]
                        // Notices: a small card over the page that never takes focus, clicks or layout space.
                        + SOverlay::Slot().Expose(NoticeSlot).HAlign(HAlign_Center).VAlign(VAlign_Bottom)
                            .Padding(0, 0, 0, MenuNoticeStyle::BottomInset)
                        [
                            SAssignNew(NoticeCard, SBox).MaxDesiredWidth(MenuNoticeStyle::MaxWidth)
                            .Visibility_Lambda([this]() { return IsNoticeShowing() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
                            [
                                SNew(SBorder).BorderImage(&MenuNoticeStyle::ShadowBrush()).Padding(FMargin(0, 0, 0, 4))
                                [
                                    SNew(SBorder).Padding(5)
                                    .BorderImage_Lambda([this]() { return bNoticeError ? &MenuNoticeStyle::ErrorCardBrush() : &MenuNoticeStyle::CardBrush(); })
                                    [
                                        SNew(SBorder).BorderImage(&MenuNoticeStyle::RuleBrush()).Padding(FMargin(22, 9, 22, 11))
                                        .HAlign(HAlign_Center)
                                        [
                                            SNew(STextBlock).WrapTextAt(MenuNoticeStyle::MaxWidth - MenuNoticeStyle::TextInset)
                                            .WrappingPolicy(ETextWrappingPolicy::AllowPerCharacterWrapping)
                                            .Justification(ETextJustify::Center)
                                            .Font(DisplayFont(MenuNoticeStyle::FontSize))
                                            .ColorAndOpacity_Lambda([this]() { return FSlateColor(bNoticeError ? MenuNoticeStyle::RustInk : MenuNoticeStyle::InkBrown); })
                                            .Text_Lambda([this]() { return FText::FromString(NoticeText); })
                                        ]
                                    ]
                                ]
                            ]
                        ]
                    ]
                ]
            ]
        ]
    ];
    for (const int32 Page : FieldBookPages)
    {
        TabBar->AddSlot().FillWidth(1).Padding(3, 0)
        [
            RegisterButton(SNew(SMenuButton).ButtonStyle(&MenuButtonStyle()).IsFocusable(true).ContentPadding(FMargin(4, 7))
            .ButtonColorAndOpacity_Lambda([this, Page]() { return SeenPage == Page ? Selected
                : Region == ERegion::Tabs && FocusedTab == Page ? Selected : MenuPine; })
            .ToolTipText(FText::FromString(Tabs[Page]))
            .OnClicked_Lambda([this, Page]() { if (PointerAction() && Dialog == EDialog::None) ChangePage(Page); return FReply::Handled(); })
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
                [ SNew(SBox).WidthOverride(28).HeightOverride(28)[ SNew(SHomesteadIcon).Kind(FName(TabIcons[Page])) ] ]
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
                [
                    SNew(STextBlock).Text(FText::FromString(Tabs[Page]))
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
                    .ColorAndOpacity_Lambda([this, Page]() { return SeenPage == Page ? FSlateColor(MenuGold) : FSlateColor(Ink); })
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(8, 4, 8, 0)
                [
                    SNew(SBox).HeightOverride(3)
                    [ SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).Padding(0)
                        .BorderBackgroundColor_Lambda([this, Page]() { return SeenPage == Page ? MenuGold
                            : Region == ERegion::Tabs && FocusedTab == Page ? MenuGold : FLinearColor::Transparent; }) ]
                ]
            ], ERegion::Tabs, Page)
        ];
    }
    TabBar->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(10, 0, 0, 0)
    [
        SNew(SBox).WidthOverride(145)
        [
            SNew(STextBlock).AutoWrapText(true).ColorAndOpacity(Ink)
            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 15))
            .Text_Lambda([this]()
            {
                if (!Controller.IsValid()) return FText();
                const double Hour = FMath::Fmod(Controller->State().hour, 24.0);
                return FText::FromString(FString::Printf(TEXT("%s / Day %d\n%02d:%02d  %s"),
                    UTF8_TO_TCHAR(Controller->Simulation().SeasonName()), Controller->Simulation().DayNumber(),
                    FMath::FloorToInt(Hour), FMath::FloorToInt((Hour - FMath::FloorToInt(Hour)) * 60),
                    Controller->Simulation().IsRaining() ? TEXT("Rain") : TEXT("Clear")));
            })
        ]
    ];
    Refresh();
}

void SHomesteadMenu::Tick(const FGeometry& Geometry, double Time, float Delta)
{
    SCompoundWidget::Tick(Geometry, Time, Delta);
    if (!Controller.IsValid()) return;
    UpdateNotice();
    if (bPointerItemDown && PointerDragRevision != Controller->Simulation().GetRevision())
        CancelPointerItemDrag();
    if (bVirtualDraggingItem && VirtualDragRevision != Controller->Simulation().GetRevision())
        CancelVirtualItemDrag();
    if (CraftInput != ECraftInput::None)
    {
        if (Dialog != EDialog::None || SeenPage != 1 || Region != ERegion::Content
            || !Entries.IsValidIndex(ContentSelection)
            || Entries[ContentSelection].Subject != EHomesteadMenuSubject::Recipe
            || Entries[ContentSelection].SubjectId != CraftHoldRecipe)
        {
            StopCraftHold();
        }
        else
        {
            const auto Recipe = static_cast<Homestead::Recipe>(CraftHoldRecipe);
            const auto Assessment = Controller->Simulation().AssessRecipe(Recipe, Controller->PlayerPoint());
            if (!Assessment.craftable)
            {
                StopCraftHold();
            }
            else
            {
                CraftHoldElapsed += Delta;
                const float BeatTimes[] = {0.18f, 0.58f, 0.98f};
                while (CraftBeat < UE_ARRAY_COUNT(BeatTimes) && CraftHoldElapsed >= BeatTimes[CraftBeat])
                    Controller->MenuCraftBeat(CraftBeat++);
                if (CraftHoldElapsed >= CraftCycleSeconds)
                {
                    if (!Controller->MenuCraftRecipe(Recipe))
                    {
                        StopCraftHold();
                    }
                    else
                    {
                        CraftFlashRecipe = CraftHoldRecipe;
                        CraftFlashStart = FPlatformTime::Seconds();
                        CraftHoldElapsed = FMath::Fmod(CraftHoldElapsed, CraftCycleSeconds);
                        CraftBeat = 0;
                        Refresh();
                    }
                }
            }
        }
    }
    FString Next = FString::Printf(TEXT("%d:%d:%d:%d"), Controller->BookPage(), Controller->IsFailed() && !Controller->IsBookOpen(),
        Controller->InventoryView(), Controller->MenuPortraitBrush() != nullptr);
    for (const auto& Row : Controller->MenuRows())
        Next += FString::Printf(TEXT("|%s:%s:%s:%s:%d:%d:%d"), *RowKey(Row), *Row.Label, *Row.Detail, *Row.Action, Row.CanStore, Row.CanTake, Row.DestinationId);
    if (Next != Signature && AudioEditId < 0) { Signature = Next; Refresh(); }
    if (Controller->MenuNeedsTestReset() && !bResetPromptShown)
    { bResetPromptShown = true; SetDialog(EDialog::TestReset); }
    if (!Controller->MenuNeedsTestReset()) bResetPromptShown = false;
    if (Controller->UsesGamepad()) Hover = INDEX_NONE;
    if (bFocusPending) SynchronizeFocus();
    if (PendingDirection.Any())
    {
        const auto Direction = PendingDirection;
        PendingDirection = {};
        NavigateDirection(Direction);
    }
    const auto Direction = LeftStick.Poll(FPlatformTime::Seconds());
    if (Direction.Any() && Controller->UsesGamepad() && !bSaving) NavigateDirection(Direction);
}

void SHomesteadMenu::Refresh()
{
    if (!Controller.IsValid() || !ContentHost) return;
    CancelPointerItemDrag();
    CancelVirtualItemDrag();
    const int32 OldPage = SeenPage;
    const bool WasRecovery = bRecovery;
    bRecovery = Controller->IsFailed() && !Controller->IsBookOpen();
    const FString OldKey = Entries.IsValidIndex(ContentSelection) ? RowKey(Entries[ContentSelection]) : FString();
    SeenPage = Controller->BookPage();
    if (TabBar) TabBar->SetVisibility(SeenPage == 4 ? EVisibility::Collapsed : EVisibility::Visible);
    Controller->RefreshMenuPortrait();
    Entries.Reset(); RowIndices.Reset(); Cells.Reset();
    FocusTargets.RemoveAll([](const FFocusTarget& Target) { return Target.region != ERegion::Tabs; });
    const auto Rows = Controller->MenuRows();
    const auto LegacyRows = Controller->Rows();
    for (int32 Index = 0; Index < Rows.Num(); ++Index)
    {
        Entries.Add(Rows[Index]);
        RowIndices.Add(Rows[Index].Subject == EHomesteadMenuSubject::Legacy
            ? LegacyRows.IndexOfByPredicate([&](const FHomesteadRow& Row) { return Row.Id == Rows[Index].Id; }) : INDEX_NONE);
    }
    if (SeenPage == 4)
    {
        // Session rows first, then only the rows of the chosen Game / Sound / Video tab.
        static const int32 Order[] = {0, 1, 9, 2, 3, 4, 17, 12, 13, 15, 8, 14, 16, 5, 6, 7, 10, 11};
        TArray<FHomesteadRow> Sorted;
        TArray<int32> SortedIndices;
        const auto Take = [&](int32 Found)
        {
            Sorted.Add(Entries[Found]);
            SortedIndices.Add(RowIndices[Found]);
        };
        for (const int32 Id : Order)
        {
            const int32 Tab = SettingsTabOf(Id);
            if (Tab >= 0 && Tab != SettingsTab) continue;
            const int32 Found = Entries.IndexOfByPredicate([Id](const FHomesteadRow& Row) { return Row.Id == Id; });
            if (Found >= 0) Take(Found);
        }
        for (int32 Index = 0; Index < Entries.Num(); ++Index)
        {
            const int32 Id = Entries[Index].Id;
            const bool Listed = Algo::Find(Order, Id) != nullptr;
            if (!Listed && SettingsTabOf(Id) == SettingsTab) Take(Index);
        }
        Entries = MoveTemp(Sorted);
        RowIndices = MoveTemp(SortedIndices);
    }
    const FString DesiredKey = OldPage == SeenPage ? OldKey : RememberedKeys[SeenPage];
    int32 Match = Entries.IndexOfByPredicate([&](const FHomesteadRow& Row) { return RowKey(Row) == DesiredKey; });
    ContentSelection = Match >= 0 ? Match : FMath::Clamp(ContentSelection, 0, FMath::Max(0, Entries.Num() - 1));
    Hover = INDEX_NONE;
    if (OldPage != SeenPage || WasRecovery != bRecovery)
    {
        FocusedTab = SeenPage;
        Region = bRecovery ? ERegion::Recovery : SeenPage == 4 ? ERegion::Session : ERegion::Content;
        SessionSelection = 0;
        RecoverySelection = 0;
    }
    ContentHost->SetContent(BuildBody());
    Select(ContentSelection);
    bFocusPending = true;
}

}
