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

namespace
{
const FLinearColor Ink(0.93f, 0.93f, 0.84f);
const FLinearColor Muted(0.71f, 0.77f, 0.69f);
const FLinearColor Gold(0.92f, 0.74f, 0.43f);
const FLinearColor Pine(0.025f, 0.05f, 0.038f, 0.97f);
const FLinearColor Selected(0.19f, 0.25f, 0.14f);
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
    Refresh();
}

void SHomesteadMenu::Tick(const FGeometry& Geometry, double Time, float Delta)
{
    SCompoundWidget::Tick(Geometry, Time, Delta);
    if (!Controller.IsValid()) return;
    FString Next = FString::Printf(TEXT("%d:%d"), Controller->BookPage(), Controller->IsFailed() && !Controller->IsBookOpen());
    for (const auto& Row : Controller->Rows())
        Next += FString::Printf(TEXT("|%d:%s:%s:%s:%d:%d"), Row.Id, *Row.Label, *Row.Detail, *Row.Action, Row.CanStore, Row.CanTake);
    if (Next != Signature) { Signature = Next; Refresh(); }
    if (Controller->UsesGamepad()) Hover = INDEX_NONE;
}

int32 SHomesteadMenu::Columns() const { return SeenPage <= 2 ? 6 : SeenPage == 4 || SeenPage == 6 ? 2 : 1; }

void SHomesteadMenu::Refresh()
{
    if (!Controller.IsValid() || !ContentHost) return;
    const int32 OldPage = SeenPage;
    const bool WasRecovery = bRecovery;
    bRecovery = Controller->IsFailed() && !Controller->IsBookOpen();
    const int32 OldId = Entries.IsValidIndex(ContentSelection) ? Entries[ContentSelection].Id : -1;
    SeenPage = Controller->BookPage();
    Entries.Reset(); RowIndices.Reset(); Cells.Reset();
    const auto Rows = Controller->Rows();
    for (int32 Index = 0; Index < Rows.Num(); ++Index)
    {
        if (SeenPage == 4 && Rows[Index].Id == 9) continue; // The exit is pinned, never in scrolled controls.
        Entries.Add(Rows[Index]); RowIndices.Add(Index);
    }
    const int32 DesiredId = OldPage == SeenPage ? OldId : RememberedIds[SeenPage];
    int32 Match = Entries.IndexOfByPredicate([DesiredId](const FHomesteadRow& Row) { return Row.Id == DesiredId; });
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
            [ Text(SeenPage == 0 ? FString::Printf(TEXT("Pack %d / %d units"), Controller->Simulation().UsedCapacity(), Homestead::InventoryCapacity)
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
    Body->AddSlot().FillHeight(1)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().FillWidth(0.68f).Padding(0, 0, 16, 0)
        [
            SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Pine).Padding(12)
            [
                SAssignNew(Scroll, SScrollBox)
                + SScrollBox::Slot()
                [ SAssignNew(Grid, SUniformGridPanel).SlotPadding(FMargin(4)) ]
            ]
        ]
        + SHorizontalBox::Slot().FillWidth(0.32f)
        [
            SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).Padding(2)
            .BorderBackgroundColor_Lambda([this]() { return Region == ERegion::Details ? Gold : Pine; })
            [
                SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Pine).Padding(16)
                [ BuildDetails() ]
            ]
        ]
    ];
    if (Entries.IsEmpty())
        Grid->AddSlot(0, 0)[ Text(TEXT("Your pack is empty.\nGather supplies or take an item from a nearby chest.")) ];
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
            [ SNew(SBox).WidthOverride(48).HeightOverride(48)[ SNew(SHomesteadIcon).Kind(EntryIcon(Row)) ] ];
        }
        Contents->AddSlot().AutoHeight()[ Text(Name, SeenPage <= 2 ? 16 : 18) ];
        if (SeenPage == 0)
        {
            const auto Item = static_cast<Homestead::Item>(Row.Id);
            Contents->AddSlot().AutoHeight()[ Text(FString::Printf(TEXT("Carried %d"), Controller->Simulation().Count(Item)), 15) ];
        }
        Cell = SNew(SBox).MinDesiredWidth(SeenPage <= 2 ? 100 : SeenPage == 4 || SeenPage == 6 ? 330 : 670)
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
    Actions = {0};
    if (SeenPage == 0) { Actions.Add(1); Actions.Add(2); }
    for (int32 Index = 0; Index < Actions.Num(); ++Index)
    {
        const int32 Action = Actions[Index];
        Box->AddSlot().AutoHeight().Padding(0, 8, 0, 0)
        [
            SNew(SButton).IsFocusable(false).ContentPadding(12)
            .ButtonColorAndOpacity_Lambda([this, Index]() { return Region == ERegion::Actions && ActionSelection == Index ? Gold : Selected; })
            .OnClicked_Lambda([this, Action]() { if (PointerAction()) RunAction(Action); return FReply::Handled(); })
            [
                SNew(STextBlock).AutoWrapText(true)
                .ColorAndOpacity_Lambda([this, Index]() { return Region == ERegion::Actions && ActionSelection == Index ? FSlateColor(Pine) : FSlateColor(Ink); })
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 17))
                .Text_Lambda([this, Action]()
                {
                    if (Action == 1) return FText::FromString(TEXT("Store 1 in nearby chest"));
                    if (Action == 2) return FText::FromString(TEXT("Take 1 from nearby chest"));
                    const int32 Index = DetailIndex();
                    if (!Entries.IsValidIndex(Index)) return FText::FromString(TEXT("No item selected"));
                    const auto& Row = Entries[Index];
                    return FText::FromString(SeenPage == 0 ? (Row.Action.IsEmpty() ? TEXT("Inspect item") : Row.Action)
                        : SeenPage == 1 ? TEXT("Craft") : SeenPage == 2 ? TEXT("Plan placement")
                        : SeenPage == 3 || SeenPage == 5 ? TEXT("Read") : TEXT("Change / activate"));
                })
            ]
        ];
    }
    return Result;
}

FString SHomesteadMenu::EntryName(const FHomesteadRow& Row) const
{
    return SeenPage == 0 && Row.Id >= 0 && Row.Id < Homestead::ItemCount
        ? FString(UTF8_TO_TCHAR(Homestead::ItemName(static_cast<Homestead::Item>(Row.Id)))) : Row.Label;
}
FName SHomesteadMenu::EntryIcon(const FHomesteadRow& Row) const
{
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
    FString Detail = Row.Label + TEXT("\n\n") + Row.Detail;
    if (SeenPage == 0)
    {
        if (!Row.CanStore) Detail += TEXT("\n\nStore unavailable: carry this item and stand near a chest.");
        if (!Row.CanTake) Detail += TEXT("\n\nTake unavailable: no stock in nearby storage.");
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
    return Controller->UsesGamepad()
        ? TEXT("LB / RB  Tabs    LT / RT  Regions    D-pad  Select    A  Activate    X  Store    Y  Take    B  Back")
        : TEXT("Ctrl+Tab  Tabs    Tab  Regions    Arrows  Select    Enter  Activate    F  Store    G  Take    Esc  Back");
}
FLinearColor SHomesteadMenu::CellColor(int32 Index) const
{
    return Index == ContentSelection ? Selected
        : Index == Hover ? Selected : FLinearColor(0.055f, 0.09f, 0.075f);
}
void SHomesteadMenu::Select(int32 Index)
{
    ContentSelection = FMath::Clamp(Index, 0, FMath::Max(0, Entries.Num() - 1));
    if (Entries.IsValidIndex(ContentSelection))
    {
        RememberedIds[SeenPage] = Entries[ContentSelection].Id;
        Controller->MenuSelect(RowIndices[ContentSelection]);
        if (Scroll && Cells.IsValidIndex(ContentSelection))
            Scroll->ScrollDescendantIntoView(Cells[ContentSelection], false, EDescendantScrollDestination::IntoView);
    }
}
bool SHomesteadMenu::PointerAction()
{
    return Controller.IsValid() && Controller->MenuAcceptsPhysicalInput();
}
void SHomesteadMenu::RunAction(int32 Action)
{
    if (!Controller.IsValid() || Dialog != EDialog::None || bSaving) return;
    if (Hover != INDEX_NONE) Select(Hover);
    if (!Entries.IsValidIndex(ContentSelection)) return;
    Controller->MenuSelect(RowIndices[ContentSelection]);
    if (Action == 1) Controller->MenuStore();
    else if (Action == 2) Controller->MenuTake();
    else if (SeenPage == 4 && Entries[ContentSelection].Id == 8) SetDialog(EDialog::Restart);
    else Controller->MenuActivate();
}
void SHomesteadMenu::Activate()
{
    if (Dialog != EDialog::None) { DialogAction(DialogSelection); return; }
    if (Region == ERegion::Tabs) ChangePage(FocusedTab);
    else if (Region == ERegion::Session) { if (SessionSelection == 0) Back(); else RequestExit(); }
    else if (Region == ERegion::Actions) RunAction(Actions.IsValidIndex(ActionSelection) ? Actions[ActionSelection] : 0);
    else { Region = ERegion::Actions; ActionSelection = 0; }
}
void SHomesteadMenu::ChangePage(int32 Page)
{
    if (!Controller.IsValid() || Dialog != EDialog::None) return;
    Controller->MenuPage(Page);
    Refresh();
}
void SHomesteadMenu::CycleRegion(int32 Direction)
{
    TArray<ERegion> Regions = {ERegion::Tabs};
    if (SeenPage == 4) Regions.Add(ERegion::Session);
    Regions.Add(ERegion::Content); Regions.Add(ERegion::Details); Regions.Add(ERegion::Actions);
    Region = Regions[HomesteadMenuNavigation::Cycle(Regions.IndexOfByKey(Region), Regions.Num(), Direction)];
    Hover = INDEX_NONE;
}
bool SHomesteadMenu::HandleKey(FKey Key, EInputEvent Event, float Amount)
{
    if (Key == EKeys::LeftControl || Key == EKeys::RightControl) bControl = Event != IE_Released;
    if (Key == EKeys::LeftShift || Key == EKeys::RightShift) bShift = Event != IE_Released;
    if (Event == IE_Axis)
    {
        if (FMath::Abs(Amount) < 0.55f || FPlatformTime::Seconds() < NextAxisMove) return true;
        if (Key == EKeys::Gamepad_LeftX) Key = Amount > 0 ? EKeys::Right : EKeys::Left;
        else if (Key == EKeys::Gamepad_LeftY) Key = Amount > 0 ? EKeys::Up : EKeys::Down;
        else return true;
        NextAxisMove = FPlatformTime::Seconds() + 0.18;
        Event = IE_Pressed;
    }
    if (Event != IE_Pressed || bSaving) return true;
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
        if (Dx || Dy) { DialogSelection = HomesteadMenuNavigation::Cycle(DialogSelection, DialogCount(), Dx ? Dx : Dy); }
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
    if (Key == EKeys::F || Key == EKeys::Gamepad_FaceButton_Left) { if (SeenPage == 0) RunAction(1); return true; }
    if (Key == EKeys::G || Key == EKeys::Gamepad_FaceButton_Top) { if (SeenPage == 0) RunAction(2); return true; }
    if (Dx || Dy)
    {
        Hover = INDEX_NONE;
        if (Region == ERegion::Tabs) FocusedTab = HomesteadMenuNavigation::Cycle(FocusedTab, 7, Dx ? Dx : Dy);
        else if (Region == ERegion::Session) SessionSelection = FMath::Clamp(SessionSelection + (Dx ? Dx : Dy), 0, 1);
        else if (Region == ERegion::Details && DetailsScroll)
            DetailsScroll->SetScrollOffset(FMath::Max(0.0f, DetailsScroll->GetScrollOffset() + (Dy ? Dy : Dx) * 48));
        else if (Region == ERegion::Actions) ActionSelection = FMath::Clamp(ActionSelection + (Dy ? Dy : Dx), 0, Actions.Num() - 1);
        else Select(HomesteadMenuNavigation::Step(ContentSelection, Entries.Num(), Columns(), Dx, Dy));
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
void SHomesteadMenu::RequestExit()
{
    if (Controller.IsValid() && !bSaving) SetDialog(Controller->IsFailed() ? EDialog::Unsaved : EDialog::Exit);
}
void SHomesteadMenu::ShowSaveFailure(const FString& Error)
{
    bSaving = false; DialogError = Error; SetDialog(EDialog::SaveFailed);
}
void SHomesteadMenu::SetDialog(EDialog Value)
{
    Dialog = Value; DialogSelection = 0;
    BuildDialog();
}
int32 SHomesteadMenu::DialogCount() const { return Dialog == EDialog::Exit || Dialog == EDialog::SaveFailed ? 3 : 2; }
void SHomesteadMenu::BuildDialog()
{
    if (Dialog == EDialog::None) { ModalHost->SetVisibility(EVisibility::Collapsed); return; }
    FString Title, Description;
    TArray<FString> Labels;
    if (Dialog == EDialog::Exit)
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
    else if (Dialog == EDialog::Unsaved)
    {
        Title = TEXT("Quit without saving?");
        Description = TEXT("Progress since the last successful save will be lost.\n\n") + Controller->MenuSaveStatus();
        Labels = {TEXT("Cancel"), TEXT("Quit without saving")};
    }
    else
    {
        Title = TEXT("Start a new clearing?");
        Description = TEXT("This replaces the current test session with a fresh clearing. Unsaved progress will be lost.");
        Labels = {TEXT("Cancel"), TEXT("Start a new clearing")};
    }
    TSharedPtr<SVerticalBox> Choices;
    ModalHost->SetVisibility(EVisibility::Visible);
    ModalHost->SetContent(
        SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
        .BorderBackgroundColor(FLinearColor(0.01f, 0.025f, 0.015f, 0.96f)).Padding(170, 110)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 16)[ Text(Title, 30) ]
            + SVerticalBox::Slot().FillHeight(1)
            [ SNew(SScrollBox) + SScrollBox::Slot()[ Text(Description, 20) ] ]
            + SVerticalBox::Slot().AutoHeight()[ SAssignNew(Choices, SVerticalBox) ]
        ]);
    for (int32 Index = 0; Index < Labels.Num(); ++Index)
        Choices->AddSlot().AutoHeight().Padding(0, 6)
        [ MakeButton(Labels[Index], [this, Index]() { DialogAction(Index); },
            TAttribute<FSlateColor>::CreateLambda([this, Index]() { return DialogSelection == Index ? Gold : Selected; })) ];
}
void SHomesteadMenu::DialogAction(int32 Index)
{
    if (!Controller.IsValid() || bSaving) return;
    if (Index == 0) { SetDialog(EDialog::None); return; }
    if ((Dialog == EDialog::Exit || Dialog == EDialog::SaveFailed) && Index == 2) { SetDialog(EDialog::Unsaved); return; }
    if (Dialog == EDialog::Unsaved) Controller->MenuQuitWithoutSaving();
    else if (Dialog == EDialog::Restart) { SetDialog(EDialog::None); Controller->MenuRestart(); }
    else if (Dialog == EDialog::Exit || Dialog == EDialog::SaveFailed)
    {
        bSaving = true;
        Controller->MenuSaveAndQuit();
    }
}
