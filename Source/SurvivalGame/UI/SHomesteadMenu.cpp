#include "SHomesteadMenu.h"
#include "SHomesteadIcon.h"
#include "HomesteadMenuNavigation.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"

namespace HomesteadMenus
{
namespace
{
const FLinearColor Ink(0.93f, 0.93f, 0.84f);
const FLinearColor Muted(0.71f, 0.77f, 0.69f);
const FLinearColor Gold(0.92f, 0.74f, 0.43f);
const FLinearColor Pine(0.025f, 0.05f, 0.038f, 0.97f);
const FLinearColor Selected(0.09f, 0.14f, 0.105f);
const TCHAR* Tabs[] = {TEXT("Inventory"), TEXT("Craft"), TEXT("Build"), TEXT("Guidebook"),
    TEXT("Settings"), TEXT("Credits"), TEXT("Appearance")};
const TCHAR* TabIcons[] = {TEXT("pack"), TEXT("craft"), TEXT("build"), TEXT("guide"),
    TEXT("settings"), TEXT("credits"), TEXT("appearance")};
const TCHAR* ItemIcons[] = {TEXT("knife"), TEXT("branch"), TEXT("stone"), TEXT("fiber"),
    TEXT("berries"), TEXT("roots"), TEXT("flowers"), TEXT("seeds"), TEXT("hatchet"),
    TEXT("digging-stick"), TEXT("watering-can"), TEXT("water"), TEXT("roasted-roots"), TEXT("herbed-roots")};
const TCHAR* RecipeIcons[] = {TEXT("hatchet"), TEXT("digging-stick"), TEXT("watering-can"),
    TEXT("roasted-roots"), TEXT("herbed-roots")};
const TCHAR* PieceIcons[] = {TEXT("foundation"), TEXT("wall"), TEXT("doorway"), TEXT("roof"),
    TEXT("fire"), TEXT("bed"), TEXT("chest")};
constexpr Homestead::EquipmentSlot VisibleEquipmentSlots[] = {
    Homestead::EquipmentSlot::Torso, Homestead::EquipmentSlot::Apron, Homestead::EquipmentSlot::Feet};
}

TSharedRef<SWidget> SHomesteadMenu::Text(const FString& Value, int32 Size) const
{
    return SNew(STextBlock).Text(FText::FromString(Value)).ColorAndOpacity(Ink)
        .Font(FCoreStyle::GetDefaultFontStyle("Regular", Size)).AutoWrapText(true);
}

TSharedRef<SWidget> SHomesteadMenu::MakeButton(const FString& Label, TFunction<void()> Action,
    TAttribute<FSlateColor> Color)
{
    return SNew(SButton).IsFocusable(false).ContentPadding(FMargin(14, 10))
        .ButtonColorAndOpacity(Color).ToolTipText(FText::FromString(Label))
        .OnClicked_Lambda([this, Action]() { if (PointerAction()) Action(); return FReply::Handled(); })
        [
            SNew(STextBlock).Text(FText::FromString(Label)).AutoWrapText(true)
            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 17))
            .ColorAndOpacity_Lambda([Color]() { return Color.Get().GetSpecifiedColor() == Gold ? FSlateColor(Pine) : FSlateColor(Ink); })
        ];
}

void SHomesteadMenu::Construct(const FArguments& Args)
{
    Controller = Args._Controller;
    TSharedPtr<SHorizontalBox> TabBar;
    ChildSlot
    [
        SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
        .BorderBackgroundColor(FLinearColor(0.015f, 0.03f, 0.02f, 0.78f)).Padding(0)
        [
            SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
            [
                SNew(SBox).WidthOverride(1280).HeightOverride(720)
                [
                    SNew(SOverlay)
                    + SOverlay::Slot().Padding(24, 16)
                    [
                        SAssignNew(Root, SVerticalBox)
                        + SVerticalBox::Slot().AutoHeight()
                        [
                            SAssignNew(TabBar, SHorizontalBox)
                        ]
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 10)
                        [
                            SNew(SBox).HeightOverride(62)
                            [
                                SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                                .BorderBackgroundColor(Pine).Padding(12, 6)
                                [
                                    SNew(SScrollBox)
                                    + SScrollBox::Slot()
                                    [
                                        SNew(STextBlock).AutoWrapText(true)
                                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 17))
                                        .ColorAndOpacity_Lambda([this]() { return Controller.IsValid() && Controller->ToastIsError()
                                            ? FSlateColor(FLinearColor(1, 0.67f, 0.48f)) : FSlateColor(Muted); })
                                        .Text_Lambda([this]() { return FText::FromString(Controller.IsValid()
                                            ? (Controller->Toast().IsEmpty() ? TEXT("Time paused  |  Choose a tab to manage your homestead.") : Controller->Toast())
                                            : TEXT("Menu unavailable.")); })
                                    ]
                                ]
                            ]
                        ]
                        + SVerticalBox::Slot().FillHeight(1)[ SAssignNew(ContentHost, SBox) ]
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 0)
                        [
                            SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                            .BorderBackgroundColor(Pine).Padding(12, 10)
                            [
                                SNew(STextBlock).AutoWrapText(true).ColorAndOpacity(Gold)
                                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 16))
                                .Text_Lambda([this]() { return FText::FromString(Footer()); })
                            ]
                        ]
                    ]
                    + SOverlay::Slot()[ SAssignNew(ModalHost, SBox).Visibility(EVisibility::Collapsed) ]
                ]
            ]
        ]
    ];
    for (int32 Page = 0; Page < 7; ++Page)
    {
        TabBar->AddSlot().FillWidth(1).Padding(3, 0)
        [
            SNew(SButton).IsFocusable(false).ContentPadding(FMargin(4, 7))
            .ButtonColorAndOpacity_Lambda([this, Page]() { return SeenPage == Page ? Selected
                : Region == ERegion::Tabs && FocusedTab == Page ? Selected : Pine; })
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
                    .ColorAndOpacity_Lambda([this, Page]() { return SeenPage == Page ? FSlateColor(Gold) : FSlateColor(Ink); })
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(8, 4, 8, 0)
                [
                    SNew(SBox).HeightOverride(3)
                    [ SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).Padding(0)
                        .BorderBackgroundColor_Lambda([this, Page]() { return SeenPage == Page ? Gold
                            : Region == ERegion::Tabs && FocusedTab == Page ? Gold : FLinearColor::Transparent; }) ]
                ]
            ]
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
    FString Next = FString::Printf(TEXT("%d:%d:%d:%d"), Controller->BookPage(), Controller->IsFailed() && !Controller->IsBookOpen(),
        Controller->InventoryView(), Controller->MenuPortraitBrush() != nullptr);
    for (const auto& Row : Controller->MenuRows())
        Next += FString::Printf(TEXT("|%s:%s:%s:%s:%d:%d:%d"), *RowKey(Row), *Row.Label, *Row.Detail, *Row.Action, Row.CanStore, Row.CanTake, Row.DestinationId);
    if (Next != Signature) { Signature = Next; Refresh(); }
    if (Controller->MenuNeedsTestReset() && !bResetPromptShown)
    { bResetPromptShown = true; SetDialog(EDialog::TestReset); }
    if (!Controller->MenuNeedsTestReset()) bResetPromptShown = false;
    if (Controller->UsesGamepad()) Hover = INDEX_NONE;
}

int32 SHomesteadMenu::Columns() const
{
    return SeenPage == 0 && Controller.IsValid() && Controller->MenuPortraitBrush() ? 5
        : SeenPage <= 2 ? 6 : SeenPage == 4 || SeenPage == 6 ? 2 : 1;
}

void SHomesteadMenu::Refresh()
{
    if (!Controller.IsValid() || !ContentHost) return;
    const int32 OldPage = SeenPage;
    const bool WasRecovery = bRecovery;
    bRecovery = Controller->IsFailed() && !Controller->IsBookOpen();
    const FString OldKey = Entries.IsValidIndex(ContentSelection) ? RowKey(Entries[ContentSelection]) : FString();
    SeenPage = Controller->BookPage();
    Controller->RefreshMenuPortrait();
    Entries.Reset(); RowIndices.Reset(); Cells.Reset();
    const auto Rows = Controller->MenuRows();
    const auto LegacyRows = Controller->Rows();
    for (int32 Index = 0; Index < Rows.Num(); ++Index)
    {
        if (SeenPage == 4 && Rows[Index].Id == 9) continue; // The exit is pinned, never in scrolled controls.
        Entries.Add(Rows[Index]);
        RowIndices.Add(Rows[Index].Subject == EHomesteadMenuSubject::Legacy
            ? LegacyRows.IndexOfByPredicate([&](const FHomesteadRow& Row) { return Row.Id == Rows[Index].Id; }) : INDEX_NONE);
    }
    const FString DesiredKey = OldPage == SeenPage ? OldKey : RememberedKeys[SeenPage];
    int32 Match = Entries.IndexOfByPredicate([&](const FHomesteadRow& Row) { return RowKey(Row) == DesiredKey; });
    ContentSelection = Match >= 0 ? Match : FMath::Clamp(ContentSelection, 0, FMath::Max(0, Entries.Num() - 1));
    Hover = INDEX_NONE;
    if (OldPage != SeenPage || WasRecovery != bRecovery)
    {
        FocusedTab = SeenPage;
        Region = SeenPage == 4 ? ERegion::Session : ERegion::Content;
        SessionSelection = 0;
    }
    ContentHost->SetContent(BuildBody());
    Select(ContentSelection);
}

TSharedRef<SWidget> SHomesteadMenu::BuildBody()
{
    if (bRecovery)
    {
        return SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(24)[ Text(TEXT("Time to try again"), 32) ]
            + SVerticalBox::Slot().FillHeight(1).Padding(24)
            [ Text(TEXT("You ran out of warmth, food, or energy.\n\nReturn to a recovery checkpoint, or open Settings to quit. No failed state will replace your usable checkpoint."), 23) ]
            + SVerticalBox::Slot().AutoHeight().Padding(24, 8)
            [ MakeButton(TEXT("Retry checkpoint  [A / Enter]"), [this]() { Controller->MenuRetry(); }) ]
            + SVerticalBox::Slot().AutoHeight().Padding(24, 8)
            [ MakeButton(TEXT("Settings / Quit  [Y / G]"), [this]() { ChangePage(4); }) ];
    }
    TSharedPtr<SVerticalBox> Body;
    TSharedPtr<SUniformGridPanel> Grid;
    auto Result = SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1)
            [ Text(SeenPage <= 2 ? Controller->BookTitle() : Tabs[SeenPage], 28) ]
            + SHorizontalBox::Slot().AutoWidth()
            [ Text(SeenPage == 0 ? Controller->MenuInventorySummary()
                : Controller->PreviewLabel(), 17) ]
        ]
        + SVerticalBox::Slot().FillHeight(1)
        [
            SAssignNew(Body, SVerticalBox)
        ];
    if (SeenPage == 4)
    {
        Body->AddSlot().AutoHeight().Padding(0, 0, 0, 12)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 8, 0)
            [ MakeButton(Controller->IsFailed() ? TEXT("Return to recovery") : TEXT("Resume"),
                [this]() { Back(); }, TAttribute<FSlateColor>::CreateLambda([this]() { return Region == ERegion::Session && SessionSelection == 0 ? Gold : Pine; })) ]
            + SHorizontalBox::Slot().FillWidth(1)
            [ MakeButton(Controller->IsFailed() ? TEXT("Quit to desktop") : TEXT("Save and quit to desktop"),
                [this]() { RequestExit(); }, TAttribute<FSlateColor>::CreateLambda([this]() { return Region == ERegion::Session && SessionSelection == 1 ? Gold : Pine; })) ]
        ];
    }
    if (SeenPage == 0)
    {
        TSharedPtr<SHorizontalBox> Views;
        Body->AddSlot().AutoHeight().Padding(0, 0, 0, 12)[ SAssignNew(Views, SHorizontalBox) ];
        const TCHAR* Names[] = {TEXT("Carried"), TEXT("Nearby chest"), TEXT("Wearing")};
        for (int32 View = 0; View < 3; ++View)
            Views->AddSlot().FillWidth(1).Padding(0, 0, 8, 0)
            [ MakeButton(Names[View], [this, View]() { ChangeInventoryView(View); },
                TAttribute<FSlateColor>::CreateLambda([this, View]()
                { return (Region == ERegion::Inventory ? InventorySelection == View : Controller->InventoryView() == View) ? Gold : Pine; })) ];
    }
    TSharedPtr<SHorizontalBox> ColumnsBox;
    Body->AddSlot().FillHeight(1)[ SAssignNew(ColumnsBox, SHorizontalBox) ];
    if ((SeenPage == 0 || SeenPage == 6) && Controller->MenuPortraitBrush())
    {
        ColumnsBox->AddSlot().AutoWidth().Padding(0, 0, 12, 0)
        [
            SNew(SBox).WidthOverride(190)
            [
                SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).Padding(2)
                .BorderBackgroundColor_Lambda([this]() { return Region == ERegion::Portrait ? Gold : Pine; })
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().FillHeight(1)
                    [
                        SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
                        [ SNew(SImage).Image_Lambda([this]() { return Controller.IsValid() ? Controller->MenuPortraitBrush() : nullptr; }) ]
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(8)
                    [
                        SNew(STextBlock).AutoWrapText(true).ColorAndOpacity(Ink)
                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 16))
                        .Text_Lambda([this]() { return FText::FromString(Controller->MenuPortraitStatus()); })
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(4)
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().FillWidth(1)[ MakeButton(TEXT("< Turn"), [this]() { Controller->OrbitMenuPortrait(-20); }) ]
                        + SHorizontalBox::Slot().FillWidth(1)[ MakeButton(TEXT("Turn >"), [this]() { Controller->OrbitMenuPortrait(20); }) ]
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(4)
                    [ MakeButton(TEXT("Close-up / full body"), [this]() { Controller->ZoomMenuPortrait(); }) ]
                ]
            ]
        ];
    }
    TSharedPtr<SVerticalBox> InventoryColumn;
    ColumnsBox->AddSlot().FillWidth(0.68f).Padding(0, 0, 16, 0)
        [
            SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Pine).Padding(12)
            [
                SAssignNew(InventoryColumn, SVerticalBox)
                + SVerticalBox::Slot().FillHeight(1)
                [
                    SAssignNew(Scroll, SScrollBox)
                    + SScrollBox::Slot()
                    [ SAssignNew(Grid, SUniformGridPanel).SlotPadding(FMargin(4)) ]
                ]
            ]
        ];
    if (SeenPage == 0)
    {
        TSharedPtr<SHorizontalBox> EquipmentBar;
        InventoryColumn->AddSlot().AutoHeight().Padding(0, 8, 0, 4)[ Text(TEXT("Equipped slots"), 16) ];
        InventoryColumn->AddSlot().AutoHeight()[ SAssignNew(EquipmentBar, SHorizontalBox) ];
        for (int32 Index = 0; Index < 3; ++Index)
            EquipmentBar->AddSlot().FillWidth(1).Padding(3, 0)
            [
                MakeButton(EquipmentLabel(Index), [this, Index]() { FocusEquipment(Index); },
                    TAttribute<FSlateColor>::CreateLambda([this, Index]()
                        { return Region == ERegion::Equipment && EquipmentSelection == Index ? Gold : Selected; }))
            ];
    }
    ColumnsBox->AddSlot().FillWidth(0.32f)
        [
            SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).Padding(2)
            .BorderBackgroundColor_Lambda([this]() { return Region == ERegion::Details ? Gold : Pine; })
            [
                SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Pine).Padding(16)
                [ SAssignNew(DetailsHost, SBox)[ BuildDetails() ] ]
            ]
        ];
    if (Entries.IsEmpty())
        Grid->AddSlot(0, 0)[ Text(SeenPage == 0 && Controller->InventoryView() == 1
            ? TEXT("No items in reachable storage.\nStand near a chest to manage its contents.")
            : SeenPage == 0 && Controller->InventoryView() == 2 ? TEXT("No removable clothing is equipped.")
            : TEXT("Your pack is empty.\nGather supplies or take an item from a nearby chest.")) ];
    for (int32 Index = 0; Index < Entries.Num(); ++Index)
    {
        const auto& Row = Entries[Index];
        const FString Name = EntryName(Row);
        TSharedPtr<SWidget> Cell;
        TSharedPtr<SVerticalBox> Contents;
        auto Button = SNew(SButton).IsFocusable(false).ContentPadding(7)
            .ButtonColorAndOpacity_Lambda([this, Index]() { return CellColor(Index); })
            .ToolTipText(FText::FromString(Name + TEXT("\n") + Row.Detail))
            .OnHovered_Lambda([this, Index]() { if (Controller.IsValid() && !Controller->UsesGamepad()) Hover = Index; })
            .OnUnhovered_Lambda([this, Index]() { if (Hover == Index) Hover = INDEX_NONE; })
            .OnClicked_Lambda([this, Index]()
            {
                if (PointerAction() && Dialog == EDialog::None) { Region = ERegion::Content; Select(Index); }
                return FReply::Handled();
            })
            [ SAssignNew(Contents, SVerticalBox) ];
        if (SeenPage <= 2)
        {
            Contents->AddSlot().AutoHeight().HAlign(HAlign_Center)
            [ SNew(SBox).WidthOverride(48).HeightOverride(48)[ SNew(SHomesteadIcon).Kind(EntryIcon(Row)).Tint(Row.IconTint) ] ];
        }
        Contents->AddSlot().AutoHeight()[ Text(Name, SeenPage <= 2 ? 16 : 18) ];
        if (SeenPage == 0)
        {
            Contents->AddSlot().AutoHeight()[ Text(FString::Printf(TEXT("%s %d"), *Row.Location, Row.Quantity), 15) ];
        }
        Cell = SNew(SBox).MinDesiredWidth(SeenPage <= 2 ? 100 : SeenPage == 4 ? 330 : SeenPage == 6 ? 270 : 670)
            .MinDesiredHeight(SeenPage <= 2 ? 116 : 72)
            [
                SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).Padding(2)
                .BorderBackgroundColor_Lambda([this, Index]() { return Index == ContentSelection ? Gold : FLinearColor::Transparent; })
                [ Button ]
            ];
        Cells.Add(Cell);
        Grid->AddSlot(Index % Columns(), Index / Columns())[ Cell.ToSharedRef() ];
    }
    return Result;
}

TSharedRef<SWidget> SHomesteadMenu::BuildDetails()
{
    TSharedPtr<SVerticalBox> Box;
    auto Result = SAssignNew(Box, SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(0, 0, 0, 12)
        [
            SNew(SBox).WidthOverride(72).HeightOverride(72)
            [
                SNew(SHomesteadIcon).Kind_Lambda([this]()
                {
                    const int32 Index = DetailIndex();
                    return Entries.IsValidIndex(Index) ? EntryIcon(Entries[Index]) : FName(TEXT("pack"));
                })
                .Tint_Lambda([this]()
                {
                    const int32 Index = DetailIndex();
                    return Entries.IsValidIndex(Index) ? Entries[Index].IconTint : Gold;
                })
            ]
        ]
        + SVerticalBox::Slot().FillHeight(1)
        [
            SAssignNew(DetailsScroll, SScrollBox)
            + SScrollBox::Slot()
            [
                SNew(STextBlock).AutoWrapText(true).ColorAndOpacity(Ink)
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 19))
                .Text_Lambda([this]() { return FText::FromString(DetailsText()); })
            ]
        ];
    Actions.Reset();
    if (Entries.IsValidIndex(ContentSelection))
    {
        const auto& Row = Entries[ContentSelection];
        if (Row.Subject == EHomesteadMenuSubject::Wearable)
        {
            Actions.Add(Row.ContainerId < 0 ? EHomesteadItemAction::Unequip : Row.ContainerId == 0
                ? EHomesteadItemAction::Equip : EHomesteadItemAction::Transfer);
            const auto* Info = Homestead::GetWearableDefinition(static_cast<Homestead::WearableDefinition>(Row.Id));
            if (Info && Info->dyeable) Actions.Add(EHomesteadItemAction::Dye);
            if (Row.ContainerId == 0) Actions.Add(EHomesteadItemAction::Transfer);
        }
        else if (Row.Subject == EHomesteadMenuSubject::ItemGroup)
        {
            Actions.Add(Row.ContainerId == 0 ? EHomesteadItemAction::Primary : EHomesteadItemAction::Transfer);
            if (Row.ContainerId == 0) Actions.Add(EHomesteadItemAction::Transfer);
            if (Row.Quantity > 1) Actions.Add(EHomesteadItemAction::Split);
            Actions.Add(EHomesteadItemAction::Merge);
        }
        else Actions.Add(EHomesteadItemAction::Primary);
        if (SeenPage == 0 && Row.ContainerId >= 0)
        { Actions.Add(EHomesteadItemAction::MoveEarlier); Actions.Add(EHomesteadItemAction::MoveLater); }
    }
    ActionSelection = FMath::Clamp(ActionSelection, 0, FMath::Max(0, Actions.Num() - 1));
    TSharedPtr<SUniformGridPanel> ActionGrid;
    Box->AddSlot().AutoHeight().Padding(0, 4, 0, 0)
    [ SAssignNew(ActionGrid, SUniformGridPanel).SlotPadding(FMargin(3)) ];
    for (int32 Index = 0; Index < Actions.Num(); ++Index)
    {
        const auto Action = Actions[Index];
        ActionGrid->AddSlot(Index % 2, Index / 2)
        [
            SNew(SButton).IsFocusable(false).ContentPadding(8)
            .ButtonColorAndOpacity_Lambda([this, Index]() { return Region == ERegion::Actions && ActionSelection == Index ? Gold : Selected; })
            .OnClicked_Lambda([this, Action]() { if (PointerAction()) RunAction(Action); return FReply::Handled(); })
            [
                SNew(STextBlock).AutoWrapText(true)
                .ColorAndOpacity_Lambda([this, Index]() { return Region == ERegion::Actions && ActionSelection == Index ? FSlateColor(Pine) : FSlateColor(Ink); })
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 16))
                .Text_Lambda([this, Action]()
                {
                    return FText::FromString(ActionLabel(Action));
                })
            ]
        ];
    }
    return Result;
}

FString SHomesteadMenu::EntryName(const FHomesteadRow& Row) const
{
    if (!Row.Name.IsEmpty()) return Row.Name;
    return SeenPage == 0 && Row.Id >= 0 && Row.Id < Homestead::ItemCount
        ? FString(UTF8_TO_TCHAR(Homestead::ItemName(static_cast<Homestead::Item>(Row.Id)))) : Row.Label;
}
FName SHomesteadMenu::EntryIcon(const FHomesteadRow& Row) const
{
    if (!Row.Icon.IsNone()) return Row.Icon;
    if (SeenPage == 0 && Row.Id >= 0 && Row.Id < UE_ARRAY_COUNT(ItemIcons)) return FName(ItemIcons[Row.Id]);
    if (SeenPage == 1 && Row.Id >= 0 && Row.Id < UE_ARRAY_COUNT(RecipeIcons)) return FName(RecipeIcons[Row.Id]);
    if (SeenPage == 2 && Row.Id >= 0 && Row.Id < UE_ARRAY_COUNT(PieceIcons)) return FName(PieceIcons[Row.Id]);
    return FName(TabIcons[FMath::Clamp(SeenPage, 0, 6)]);
}
int32 SHomesteadMenu::DetailIndex() const { return Hover != INDEX_NONE ? Hover : ContentSelection; }
FString SHomesteadMenu::DetailsText() const
{
    const int32 Index = DetailIndex();
    if (!Entries.IsValidIndex(Index)) return TEXT("Select an item to see its details.\n\nNothing here is a recipe output you already own.");
    const auto& Row = Entries[Index];
    FString Detail = Row.Label + TEXT("\n") + Row.Location + TEXT("\n\n") + Row.Detail;
    if (SeenPage == 0)
    {
        if (Row.ContainerId == 0 && !Row.CanStore) Detail += TEXT("\n\nStand near a chest to store this item.");
    }
    if (SeenPage == 4 && Controller.IsValid() && Controller->IsFailed())
        Detail += TEXT("\n\nRecovery: saving a failed state is disabled. Retry a checkpoint or quit explicitly.");
    return Detail;
}
FString SHomesteadMenu::Footer() const
{
    if (!Controller.IsValid()) return {};
    if (bRecovery) return Controller->UsesGamepad() ? TEXT("A  Retry checkpoint    Y  Settings / Quit")
        : TEXT("Enter  Retry checkpoint    G  Settings / Quit");
    const bool Pad = Controller->UsesGamepad();
    FString Hint = Pad ? TEXT("LB/RB  Tabs    LT/RT  Regions    D-pad  Navigate    A  Activate")
        : TEXT("Ctrl+Tab  Tabs    Tab  Regions    Arrows  Navigate    Enter  Activate");
    if (SeenPage == 0 && Entries.IsValidIndex(ContentSelection))
    {
        const auto& Row = Entries[ContentSelection];
        if (Row.CanStore || Row.CanTake) Hint += Pad ? TEXT("    X  Transfer") : TEXT("    F  Transfer");
        if (Row.Subject == EHomesteadMenuSubject::ItemGroup && Row.Quantity > 1)
            Hint += Pad ? TEXT("    Y  Split") : TEXT("    G  Split");
    }
    if (Region == ERegion::Portrait) Hint += Pad ? TEXT("    Right stick  Turn    R3  Zoom") : TEXT("    Left/Right  Turn    Z  Zoom");
    if (Region == ERegion::Details) Hint += TEXT("    Up/Down  Scroll details");
    return Hint + (Pad ? TEXT("    B  Back") : TEXT("    Esc  Back"));
}
FString SHomesteadMenu::RowKey(const FHomesteadRow& Row) const
{
    return FString::Printf(TEXT("%d:%d:%d"), static_cast<int>(Row.Subject), Row.ContainerId,
        Row.Subject == EHomesteadMenuSubject::Legacy ? Row.Id : Row.SubjectId);
}
FString SHomesteadMenu::ActionLabel(EHomesteadItemAction Action) const
{
    if (!Entries.IsValidIndex(ContentSelection)) return TEXT("No item selected");
    const auto& Row = Entries[ContentSelection];
    switch (Action)
    {
    case EHomesteadItemAction::Transfer: return Row.ContainerId > 0 ? TEXT("Take to pack...")
        : Row.DestinationId < 0 ? TEXT("Store (no nearby chest)") : FString::Printf(TEXT("Store in chest %d..."), Row.DestinationId);
    case EHomesteadItemAction::Split: return TEXT("Split stack...");
    case EHomesteadItemAction::Merge: return TEXT("Merge with stack...");
    case EHomesteadItemAction::MoveEarlier: return TEXT("Move earlier in grid");
    case EHomesteadItemAction::MoveLater: return TEXT("Move later in grid");
    case EHomesteadItemAction::Equip: return TEXT("Equip");
    case EHomesteadItemAction::Unequip: return TEXT("Unequip to pack");
    case EHomesteadItemAction::Dye: return TEXT("Change dye");
    default: return Row.Action.IsEmpty() ? (SeenPage == 3 || SeenPage == 5 ? TEXT("Read") : TEXT("Change / activate")) : Row.Action;
    }
}
void SHomesteadMenu::ChangeInventoryView(int32 View)
{
    if (Dialog != EDialog::None || !Controller.IsValid()) return;
    InventorySelection = FMath::Clamp(View, 0, 2);
    Controller->MenuInventoryView(InventorySelection);
    ContentSelection = 0; Hover = INDEX_NONE;
    Region = ERegion::Content;
    Refresh();
}
FString SHomesteadMenu::EquipmentLabel(int32 Index) const
{
    const TCHAR* Names[] = {TEXT("Torso + legs"), TEXT("Apron"), TEXT("Feet")};
    if (!Controller.IsValid() || Index < 0 || Index >= 3) return {};
    const int32 Id = Controller->State().equipment[static_cast<int32>(VisibleEquipmentSlots[Index])];
    const auto* Item = Controller->Simulation().GetWearable(Id);
    return FString(Names[Index]) + TEXT("\n") + (Item
        ? FString(UTF8_TO_TCHAR(Homestead::WearableName(Item->definition))) : TEXT("Empty - choose from pack"));
}
void SHomesteadMenu::FocusEquipment(int32 Index)
{
    if (!Controller.IsValid() || Dialog != EDialog::None || Index < 0 || Index >= 3) return;
    const int32 Id = Controller->State().equipment[static_cast<int32>(VisibleEquipmentSlots[Index])];
    EquipmentSelection = Index;
    ChangeInventoryView(Id ? 2 : 0);
    if (Id)
    {
        const int32 Entry = Entries.IndexOfByPredicate([Id](const FHomesteadRow& Row)
            { return Row.Subject == EHomesteadMenuSubject::Wearable && Row.SubjectId == Id; });
        if (Entry >= 0) { Select(Entry); Region = ERegion::Actions; ActionSelection = 0; }
    }
}
FLinearColor SHomesteadMenu::CellColor(int32 Index) const
{
    return Index == ContentSelection ? Selected
        : Index == Hover ? Selected : FLinearColor(0.055f, 0.09f, 0.075f);
}
void SHomesteadMenu::Select(int32 Index, bool KeepDesiredColumn)
{
    ContentSelection = FMath::Clamp(Index, 0, FMath::Max(0, Entries.Num() - 1));
    if (!KeepDesiredColumn) DesiredColumn = ContentSelection % Columns();
    if (Entries.IsValidIndex(ContentSelection))
    {
        RememberedKeys[SeenPage] = RowKey(Entries[ContentSelection]);
        if (RowIndices[ContentSelection] != INDEX_NONE) Controller->MenuSelect(RowIndices[ContentSelection]);
        if (Scroll && Cells.IsValidIndex(ContentSelection))
            Scroll->ScrollDescendantIntoView(Cells[ContentSelection], false, EDescendantScrollDestination::IntoView);
        if (DetailsHost) DetailsHost->SetContent(BuildDetails());
    }
}
bool SHomesteadMenu::PointerAction()
{
    return Controller.IsValid() && Controller->MenuAcceptsPhysicalInput();
}
void SHomesteadMenu::RunAction(EHomesteadItemAction Action)
{
    if (!Controller.IsValid() || Dialog != EDialog::None || bSaving) return;
    if (!Entries.IsValidIndex(ContentSelection)) return;
    const auto Row = Entries[ContentSelection];
    if (Row.Subject != EHomesteadMenuSubject::Legacy)
    {
        PendingRow = Row; PendingAction = Action; PendingRevision = Controller->Simulation().GetRevision();
        if ((Action == EHomesteadItemAction::Transfer || Action == EHomesteadItemAction::Split)
            && Row.Subject == EHomesteadMenuSubject::ItemGroup)
        {
            Amount = 1; MaximumAmount = Row.Quantity - (Action == EHomesteadItemAction::Split ? 1 : 0);
            if (Action == EHomesteadItemAction::Transfer)
            {
                const int32 Used = Row.ContainerId > 0 ? Controller->Simulation().UsedCapacity()
                    : Controller->Simulation().ChestUsedCapacity(Row.DestinationId);
                MaximumAmount = Used < 0 ? 0 : FMath::Min(MaximumAmount, Homestead::InventoryCapacity - Used);
            }
            if (MaximumAmount < 1)
            { Controller->MenuItemAction(Row, Action, 1, PendingRevision); return; }
            SetDialog(EDialog::Amount); return;
        }
        if (Action == EHomesteadItemAction::Merge)
        {
            MergeTargets.Reset();
            for (const auto& Target : Entries)
                if (Target.Subject == EHomesteadMenuSubject::ItemGroup && Target.Id == Row.Id && Target.ContainerId == Row.ContainerId
                    && Target.SubjectId != Row.SubjectId) MergeTargets.Add(Target.SubjectId);
            SetDialog(EDialog::Merge); return;
        }
        Controller->MenuItemAction(Row, Action, 1, PendingRevision);
        Refresh();
        return;
    }
    Controller->MenuSelect(RowIndices[ContentSelection]);
    if (SeenPage == 4 && Row.Id == 8) SetDialog(EDialog::Restart);
    else Controller->MenuActivate();
}
void SHomesteadMenu::Activate()
{
    if (Dialog != EDialog::None) { DialogAction(DialogSelection); return; }
    if (Region == ERegion::Tabs) ChangePage(FocusedTab);
    else if (Region == ERegion::Inventory) ChangeInventoryView(InventorySelection);
    else if (Region == ERegion::Equipment) FocusEquipment(EquipmentSelection);
    else if (Region == ERegion::Session) { if (SessionSelection == 0) Back(); else RequestExit(); }
    else if (Region == ERegion::Actions && Actions.IsValidIndex(ActionSelection)) RunAction(Actions[ActionSelection]);
    else { Region = ERegion::Actions; ActionSelection = 0; }
}
void SHomesteadMenu::ChangePage(int32 Page)
{
    if (!Controller.IsValid() || Dialog != EDialog::None) return;
    Controller->MenuPage(Page);
    Refresh();
}
bool SHomesteadMenu::FocusLegacySubject(int32 Id)
{
    const int32 Index = Entries.IndexOfByPredicate([Id](const FHomesteadRow& Row)
        { return Row.Subject == EHomesteadMenuSubject::Legacy && Row.Id == Id; });
    if (Index < 0) return false;
    Region = ERegion::Content;
    Hover = INDEX_NONE;
    Select(Index);
    return true;
}
void SHomesteadMenu::CycleRegion(int32 Direction)
{
    TArray<ERegion> Regions = {ERegion::Tabs};
    if (SeenPage == 4) Regions.Add(ERegion::Session);
    if (SeenPage == 0) Regions.Add(ERegion::Inventory);
    if (Controller->MenuPortraitBrush()) Regions.Add(ERegion::Portrait);
    Regions.Add(ERegion::Content);
    if (SeenPage == 0) Regions.Add(ERegion::Equipment);
    Regions.Add(ERegion::Details); Regions.Add(ERegion::Actions);
    Region = Regions[HomesteadMenuNavigation::Cycle(Regions.IndexOfByKey(Region), Regions.Num(), Direction)];
    Hover = INDEX_NONE;
}
bool SHomesteadMenu::HandleKey(FKey Key, EInputEvent Event, float InputAmount)
{
    if (Key == EKeys::LeftControl || Key == EKeys::RightControl) bControl = Event != IE_Released;
    if (Key == EKeys::LeftShift || Key == EKeys::RightShift) bShift = Event != IE_Released;
    if (Event == IE_Axis)
    {
        if (Region == ERegion::Portrait && Dialog == EDialog::None && Key == EKeys::Gamepad_RightX)
        {
            if (FMath::Abs(InputAmount) > 0.3f) Controller->OrbitMenuPortrait(InputAmount * 1.5f);
            return true;
        }
        if (FMath::Abs(InputAmount) < 0.55f || FPlatformTime::Seconds() < NextAxisMove) return true;
        if (Key == EKeys::Gamepad_LeftX) Key = InputAmount > 0 ? EKeys::Right : EKeys::Left;
        else if (Key == EKeys::Gamepad_LeftY) Key = InputAmount > 0 ? EKeys::Up : EKeys::Down;
        else return true;
        NextAxisMove = FPlatformTime::Seconds() + 0.18;
        Event = IE_Pressed;
    }
    if (Event == IE_Repeat && (Key == EKeys::Left || Key == EKeys::Right || Key == EKeys::Up || Key == EKeys::Down
        || Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_DPad_Down))
        Event = IE_Pressed;
    if (Event != IE_Pressed || bSaving) return true;
    if (!Key.IsMouseButton()) Hover = INDEX_NONE;
    if (Region == ERegion::Portrait && Dialog == EDialog::None
        && (Key == EKeys::Z || Key == EKeys::Gamepad_RightThumbstick))
    { Controller->ZoomMenuPortrait(); return true; }
    if (bRecovery)
    {
        if (Key == EKeys::Enter || Key == EKeys::E || Key == EKeys::Gamepad_FaceButton_Bottom) Controller->MenuRetry();
        else if (Key == EKeys::G || Key == EKeys::Gamepad_FaceButton_Top || Key == EKeys::Escape
            || Key == EKeys::Gamepad_Special_Right || Key == EKeys::Gamepad_FaceButton_Right) ChangePage(4);
        return true;
    }
    if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right || Key == EKeys::I || Key == EKeys::Gamepad_Special_Right) { Back(); return true; }
    if (Key == EKeys::Enter || Key == EKeys::E || Key == EKeys::Gamepad_FaceButton_Bottom) { Activate(); return true; }
    int32 Dx = Key == EKeys::Right || Key == EKeys::Gamepad_DPad_Right ? 1 : Key == EKeys::Left || Key == EKeys::Gamepad_DPad_Left ? -1 : 0;
    int32 Dy = Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Down ? 1 : Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up ? -1 : 0;
    if (Dialog != EDialog::None)
    {
        if (Dialog == EDialog::Amount)
        {
            const int32 Delta = Dx ? Dx : Key == EKeys::Gamepad_RightShoulder ? 10 : Key == EKeys::Gamepad_LeftShoulder ? -10 : 0;
            if (Delta) { Amount = FMath::Clamp(Amount + Delta, 1, MaximumAmount); BuildDialog(); return true; }
        }
        if (Dx || Dy)
        {
            DialogSelection = HomesteadMenuNavigation::Cycle(DialogSelection, DialogCount(), Dx ? Dx : Dy);
            if (DialogScroll && DialogButtons.IsValidIndex(DialogSelection))
                DialogScroll->ScrollDescendantIntoView(DialogButtons[DialogSelection], false);
        }
        return true;
    }
    if (Key == EKeys::Gamepad_LeftShoulder) { ChangePage((SeenPage + 6) % 7); return true; }
    if (Key == EKeys::Gamepad_RightShoulder) { ChangePage((SeenPage + 1) % 7); return true; }
    if (Key == EKeys::Tab)
    {
        if (bControl) ChangePage((SeenPage + (bShift ? 6 : 1)) % 7);
        else CycleRegion(bShift ? -1 : 1);
        return true;
    }
    if (Key == EKeys::Gamepad_LeftTrigger) { CycleRegion(-1); return true; }
    if (Key == EKeys::Gamepad_RightTrigger) { CycleRegion(1); return true; }
    if (Key == EKeys::F || Key == EKeys::Gamepad_FaceButton_Left) { if (SeenPage == 0) RunAction(EHomesteadItemAction::Transfer); return true; }
    if (Key == EKeys::G || Key == EKeys::Gamepad_FaceButton_Top) { if (SeenPage == 0) RunAction(EHomesteadItemAction::Split); return true; }
    if (Dx || Dy)
    {
        Hover = INDEX_NONE;
        if (Region == ERegion::Tabs) FocusedTab = HomesteadMenuNavigation::Cycle(FocusedTab, 7, Dx ? Dx : Dy);
        else if (Region == ERegion::Inventory) InventorySelection = FMath::Clamp(InventorySelection + (Dx ? Dx : Dy), 0, 2);
        else if (Region == ERegion::Equipment) EquipmentSelection = FMath::Clamp(EquipmentSelection + (Dx ? Dx : Dy), 0, 2);
        else if (Region == ERegion::Portrait) Controller->OrbitMenuPortrait((Dx ? Dx : Dy) * 15);
        else if (Region == ERegion::Session) SessionSelection = FMath::Clamp(SessionSelection + (Dx ? Dx : Dy), 0, 1);
        else if (Region == ERegion::Details && DetailsScroll)
            DetailsScroll->SetScrollOffset(FMath::Max(0.0f, DetailsScroll->GetScrollOffset() + (Dy ? Dy : Dx) * 48));
        else if (Region == ERegion::Actions)
            ActionSelection = FMath::Max(0, HomesteadMenuNavigation::Step(ActionSelection, Actions.Num(), 2, Dx, Dy));
        else
        {
            int32 Next = HomesteadMenuNavigation::Step(ContentSelection, Entries.Num(), Columns(), Dx, Dy);
            if (Dy && Next >= 0 && Next / Columns() != ContentSelection / Columns())
                Next = FMath::Min(Next / Columns() * Columns() + DesiredColumn, Entries.Num() - 1);
            Select(Next, Dy != 0);
        }
    }
    return true;
}
FReply SHomesteadMenu::OnKeyDown(const FGeometry&, const FKeyEvent& Event)
{
    bControl = Event.IsControlDown(); bShift = Event.IsShiftDown();
    if (Controller.IsValid()) Controller->MenuPhysicalInput(Event.GetKey(), Event.IsRepeat() ? IE_Repeat : IE_Pressed);
    return FReply::Handled();
}
FReply SHomesteadMenu::OnKeyUp(const FGeometry&, const FKeyEvent& Event)
{
    bControl = Event.IsControlDown(); bShift = Event.IsShiftDown();
    if (Controller.IsValid()) Controller->MenuPhysicalInput(Event.GetKey(), IE_Released, 0);
    return FReply::Handled();
}
FReply SHomesteadMenu::OnAnalogValueChanged(const FGeometry&, const FAnalogInputEvent& Event)
{
    if (Controller.IsValid()) Controller->MenuPhysicalInput(Event.GetKey(), IE_Axis, Event.GetAnalogValue());
    return FReply::Handled();
}
FReply SHomesteadMenu::OnMouseMove(const FGeometry&, const FPointerEvent& Event)
{
    return FReply::Handled();
}
FReply SHomesteadMenu::OnMouseWheel(const FGeometry&, const FPointerEvent& Event)
{
    if (Controller.IsValid() && Controller->MenuAcceptsPhysicalInput() && Scroll)
        Scroll->SetScrollOffset(FMath::Max(0.0f, Scroll->GetScrollOffset() - Event.GetWheelDelta() * 80));
    return FReply::Handled();
}
void SHomesteadMenu::Back()
{
    if (bSaving) return;
    if (Dialog != EDialog::None) { SetDialog(EDialog::None); return; }
    if (Controller.IsValid()) Controller->MenuBack();
}
bool SHomesteadMenu::PrepareQuickAction()
{
    if (bSaving) return false;
    if (Dialog == EDialog::Amount || Dialog == EDialog::Merge) SetDialog(EDialog::None);
    return Dialog == EDialog::None;
}
void SHomesteadMenu::RequestExit()
{
    if (Controller.IsValid() && !bSaving) SetDialog(Controller->IsFailed() ? EDialog::Unsaved : EDialog::Exit);
}
void SHomesteadMenu::ShowSaveFailure(const FString& Error)
{
    bSaving = false; DialogError = Error; SetDialog(EDialog::SaveFailed);
}
void SHomesteadMenu::ShowGraphicsSaveFailure(const FString& Error)
{
    bSaving = false; DialogError = Error; SetDialog(EDialog::GraphicsFailed);
}
void SHomesteadMenu::SetDialog(EDialog Value)
{
    Dialog = Value; DialogSelection = 0;
    BuildDialog();
}
int32 SHomesteadMenu::DialogCount() const
{
    if (Dialog == EDialog::Amount) return 4;
    if (Dialog == EDialog::Merge) return MergeTargets.Num() + 1;
    return Dialog == EDialog::Exit || Dialog == EDialog::SaveFailed || Dialog == EDialog::GraphicsFailed ? 3 : 2;
}
void SHomesteadMenu::BuildDialog()
{
    if (Dialog == EDialog::None) { ModalHost->SetVisibility(EVisibility::Collapsed); return; }
    FString Title, Description;
    TArray<FString> Labels;
    if (Dialog == EDialog::Amount)
    {
        Title = PendingAction == EHomesteadItemAction::Split ? TEXT("Split stack") : TEXT("Transfer items");
        const FString Destination = PendingAction == EHomesteadItemAction::Split ? TEXT("A new stack in the same container")
            : PendingRow.ContainerId > 0 ? TEXT("Your pack") : FString::Printf(TEXT("Chest %d"), PendingRow.DestinationId);
        Description = FString::Printf(TEXT("%s\nFrom: %s\nTo: %s\n\nAmount: %d / %d\nLeft / Right: one   LB / RB: ten\nUp / Down: choose a button"),
            *PendingRow.Name, *PendingRow.Location, *Destination, Amount, MaximumAmount);
        Labels = {TEXT("Cancel"), TEXT("Confirm"), TEXT("One"), TEXT("All available")};
    }
    else if (Dialog == EDialog::Merge)
    {
        Title = TEXT("Merge stacks");
        Description = MergeTargets.IsEmpty() ? TEXT("No other matching stack exists in this container.")
            : TEXT("Choose the destination stack. Quantities are combined; nothing changes until you confirm a destination.");
        Labels.Add(TEXT("Cancel"));
        for (int32 Target : MergeTargets)
        {
            const auto* Row = Entries.FindByPredicate([Target](const FHomesteadRow& Entry)
                { return Entry.Subject == EHomesteadMenuSubject::ItemGroup && Entry.SubjectId == Target; });
            Labels.Add(FString::Printf(TEXT("Stack #%d  (%d)"), Target, Row ? Row->Quantity : 0));
        }
    }
    else if (Dialog == EDialog::Exit)
    {
        Title = TEXT("Save and quit?");
        Description = TEXT("Save this homestead before closing the game.\n\n") + Controller->MenuSaveStatus();
        Labels = {TEXT("Stay in Settings"), TEXT("Save and quit"), TEXT("Quit without saving...")};
    }
    else if (Dialog == EDialog::SaveFailed)
    {
        Title = TEXT("Your progress was not saved");
        Description = DialogError + TEXT("\n\nThe game is still paused. Retry or choose what to do next.");
        Labels = {TEXT("Return to Settings"), TEXT("Retry save and quit"), TEXT("Quit without saving...")};
    }
    else if (Dialog == EDialog::GraphicsFailed)
    {
        Title = TEXT("World saved; video preference not confirmed");
        Description = DialogError + TEXT("\n\nYour homestead was saved successfully. You can retry the preference or explicitly leave it unsaved.");
        Labels = {TEXT("Return to Settings"), TEXT("Retry and quit"), TEXT("Quit with video preference unverified")};
    }
    else if (Dialog == EDialog::Unsaved)
    {
        Title = TEXT("Quit without saving?");
        Description = TEXT("Progress since the last successful save will be lost.\n\n") + Controller->MenuSaveStatus();
        Labels = {TEXT("Cancel"), TEXT("Quit without saving")};
    }
    else if (Dialog == EDialog::TestReset)
    {
        Title = TEXT("This test save could not be loaded");
        Description = Controller->MenuLoadProblem() + TEXT("\n\nA reset starts a fresh test world. It is not a migration of the old progress.");
        Labels = {TEXT("Stay in Settings"), TEXT("Start a new test clearing")};
    }
    else
    {
        Title = TEXT("Start a new clearing?");
        Description = TEXT("This replaces the current test session with a fresh clearing. Unsaved progress will be lost.");
        Labels = {TEXT("Cancel"), TEXT("Start a new clearing")};
    }
    TSharedPtr<SVerticalBox> Choices;
    DialogButtons.Reset();
    ModalHost->SetVisibility(EVisibility::Visible);
    ModalHost->SetContent(
        SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
        .BorderBackgroundColor(FLinearColor(0.01f, 0.025f, 0.015f, 0.96f)).Padding(170, 110)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 16)[ Text(Title, 30) ]
            + SVerticalBox::Slot().FillHeight(1)
            [ SNew(SScrollBox) + SScrollBox::Slot()[ Text(Description, 20) ] ]
            + SVerticalBox::Slot().AutoHeight()
            [
                SNew(SBox).MaxDesiredHeight(230)
                [ SAssignNew(DialogScroll, SScrollBox) + SScrollBox::Slot()[ SAssignNew(Choices, SVerticalBox) ] ]
            ]
        ]);
    for (int32 Index = 0; Index < Labels.Num(); ++Index)
    {
        auto Button = MakeButton(Labels[Index], [this, Index]() { DialogAction(Index); },
            TAttribute<FSlateColor>::CreateLambda([this, Index]() { return DialogSelection == Index ? Gold : Selected; }));
        DialogButtons.Add(Button);
        Choices->AddSlot().AutoHeight().Padding(0, 6)[ Button ];
    }
}
void SHomesteadMenu::DialogAction(int32 Index)
{
    if (!Controller.IsValid() || bSaving) return;
    if (Index == 0) { SetDialog(EDialog::None); return; }
    if (Dialog == EDialog::Amount)
    {
        if (Index == 2) { Amount = 1; BuildDialog(); return; }
        if (Index == 3) { Amount = MaximumAmount; BuildDialog(); return; }
        Controller->MenuItemAction(PendingRow, PendingAction, Amount, PendingRevision);
        SetDialog(EDialog::None); Refresh(); return;
    }
    if (Dialog == EDialog::Merge)
    {
        if (MergeTargets.IsValidIndex(Index - 1))
            Controller->MenuItemAction(PendingRow, PendingAction, MergeTargets[Index - 1], PendingRevision);
        SetDialog(EDialog::None); Refresh(); return;
    }
    if ((Dialog == EDialog::Exit || Dialog == EDialog::SaveFailed) && Index == 2) { SetDialog(EDialog::Unsaved); return; }
    if (Dialog == EDialog::GraphicsFailed && Index == 2) { Controller->MenuQuitWithoutSaving(); return; }
    if (Dialog == EDialog::Unsaved) Controller->MenuQuitWithoutSaving();
    else if (Dialog == EDialog::Restart || Dialog == EDialog::TestReset) { SetDialog(EDialog::None); Controller->MenuRestart(); }
    else if (Dialog == EDialog::Exit || Dialog == EDialog::SaveFailed || Dialog == EDialog::GraphicsFailed)
    {
        bSaving = true;
        Controller->MenuSaveAndQuit();
    }
}
}
