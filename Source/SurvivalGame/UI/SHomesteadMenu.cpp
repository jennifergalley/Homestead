#include "SHomesteadMenu.h"
#include "SHomesteadIcon.h"
#include "HomesteadMenuNavigation.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Brushes/SlateColorBrush.h"
#include "Input/NavigationReply.h"
#include "Types/NavigationMetaData.h"
#include "Layout/WidgetPath.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"

namespace HomesteadMenus
{
namespace
{
class SMenuButton : public SButton
{
public:
    virtual FReply OnAnalogValueChanged(const FGeometry&, const FAnalogInputEvent&) override
    { return FReply::Unhandled(); }
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
const FLinearColor Ink(0.93f, 0.93f, 0.84f);
const FLinearColor Muted(0.71f, 0.77f, 0.69f);
const FLinearColor MenuGold(0.92f, 0.74f, 0.43f);
const FLinearColor MenuPine(0.025f, 0.05f, 0.038f, 0.97f);
const FLinearColor Selected(0.09f, 0.14f, 0.105f);
constexpr float ItemCellWidth = 76;
float LogicalBookWidth()
{
    const FViewport* Viewport = GEngine && GEngine->GameViewport
        ? GEngine->GameViewport->Viewport : nullptr;
    return Viewport ? FMath::Max(1280.0f,
        Viewport->GetSizeXY().X * (1280.0f / 1920.0f)) : 1280.0f;
}
float LogicalBookHeight()
{
    const FViewport* Viewport = GEngine && GEngine->GameViewport
        ? GEngine->GameViewport->Viewport : nullptr;
    return Viewport ? FMath::Max(720.0f,
        Viewport->GetSizeXY().Y * (720.0f / 1080.0f)) : 720.0f;
}
float PortraitColumnWidth()
{
    return FMath::Min(420.0f, 240.0f + (LogicalBookWidth() - 1280.0f) * 0.14f);
}
float DetailsColumnWidth()
{
    return FMath::Min(560.0f, 340.0f + (LogicalBookWidth() - 1280.0f) * 0.18f);
}
const FButtonStyle& MenuButtonStyle()
{
    static const FButtonStyle Style = FButtonStyle()
        .SetNormal(FSlateColorBrush(FLinearColor::White))
        .SetHovered(FSlateColorBrush(FLinearColor(1.12f, 1.12f, 1.12f, 1)))
        .SetPressed(FSlateColorBrush(FLinearColor(0.85f, 0.85f, 0.85f, 1)))
        .SetDisabled(FSlateColorBrush(FLinearColor(0.65f, 0.65f, 0.65f, 1)))
        .SetNormalPadding(FMargin(0)).SetPressedPadding(FMargin(0));
    return Style;
}
const TCHAR* Tabs[] = {TEXT("Inventory"), TEXT("Craft"), TEXT("Build"), TEXT("Guidebook"),
    TEXT("Settings"), TEXT("Credits"), TEXT("Appearance")};
const TCHAR* TabIcons[] = {TEXT("pack"), TEXT("craft"), TEXT("build"), TEXT("guide"),
    TEXT("settings"), TEXT("credits"), TEXT("appearance")};
const TCHAR* ItemIcons[] = {TEXT("knife"), TEXT("branch"), TEXT("stone"), TEXT("fiber"),
    TEXT("berries"), TEXT("roots"), TEXT("flowers"), TEXT("seeds"), TEXT("hatchet"),
    TEXT("digging-stick"), TEXT("watering-can"), TEXT("water"), TEXT("roasted-roots"), TEXT("herbed-roots"),
    TEXT("timber"), TEXT("firewood")};
FName RequirementIcon(Homestead::Item Item)
{
    const int32 Index = static_cast<int32>(Item);
    if (Index < 0 || Index >= UE_ARRAY_COUNT(ItemIcons))
    {
        UE_LOG(LogTemp, Error, TEXT("Crafting requirement has no known item icon: %d"), Index);
        return NAME_None;
    }
    return FName(ItemIcons[Index]);
}
const TCHAR* RecipeIcons[] = {TEXT("hatchet"), TEXT("digging-stick"), TEXT("watering-can"),
    TEXT("roasted-roots"), TEXT("herbed-roots"), TEXT("firewood")};
const TCHAR* PieceIcons[] = {TEXT("foundation"), TEXT("wall"), TEXT("doorway"), TEXT("roof"),
    TEXT("fire"), TEXT("bed"), TEXT("chest")};
constexpr Homestead::EquipmentSlot VisibleEquipmentSlots[] = {
    Homestead::EquipmentSlot::Torso, Homestead::EquipmentSlot::Apron, Homestead::EquipmentSlot::Feet};
constexpr int32 FieldBookPages[] = {0, 1, 2, 3, 6};

int32 ShiftFieldBookPage(int32 Page, int32 Direction)
{
    int32 Index = 0;
    for (int32 I = 0; I < UE_ARRAY_COUNT(FieldBookPages); ++I)
        if (FieldBookPages[I] == Page) { Index = I; break; }
    return FieldBookPages[(Index + UE_ARRAY_COUNT(FieldBookPages) + Direction) % UE_ARRAY_COUNT(FieldBookPages)];
}
}
TSharedRef<SWidget> SHomesteadMenu::Text(const FString& Value, int32 Size) const
{
    return SNew(STextBlock).Text(FText::FromString(Value)).ColorAndOpacity(Ink)
        .Font(FCoreStyle::GetDefaultFontStyle("Regular", Size)).AutoWrapText(true);
}

TSharedRef<SButton> SHomesteadMenu::MakeButton(const FString& Label, TFunction<void()> Action,
    TAttribute<FSlateColor> Color, const FString& AccessibleLabel, FMargin Padding)
{
    return SNew(SMenuButton).ButtonStyle(&MenuButtonStyle()).IsFocusable(true).ContentPadding(Padding)
        .ButtonColorAndOpacity(Color).ToolTipText(FText::FromString(AccessibleLabel.IsEmpty() ? Label : AccessibleLabel))
        .OnClicked_Lambda([this, Action]() { if (PointerAction()) Action(); return FReply::Handled(); })
        [
            SNew(STextBlock).Text(FText::FromString(Label)).AutoWrapText(true)
            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 17))
            .ColorAndOpacity_Lambda([Color]() { return Color.Get().GetSpecifiedColor() == MenuGold ? FSlateColor(MenuPine) : FSlateColor(Ink); })
        ];
}

TSharedRef<SButton> SHomesteadMenu::RegisterButton(TSharedRef<SButton> Button, ERegion TargetRegion, int32 Index)
{
    Button->SetOnFocusReceived(FSimpleDelegate::CreateLambda([this, TargetRegion, Index]() { AdoptFocus(TargetRegion, Index); }));
    FocusTargets.Add({TargetRegion, Index, Button});
    return Button;
}

TSharedRef<SWidget> SHomesteadMenu::FocusAnchor(TSharedRef<SWidget> Content, ERegion TargetRegion, int32 Index)
{
    auto Anchor = SNew(SMenuFocusAnchor)
        .OnFocused_Lambda([this, TargetRegion, Index]() { AdoptFocus(TargetRegion, Index); })[Content];
    if (TargetRegion == ERegion::Portrait && Index == -1)
    {
        // The tall image overlaps header rows; its right neighbor is remembered content.
        auto Navigation = MakeShared<FNavigationMetaData>();
        Navigation->SetNavigationCustom(EUINavigation::Right, EUINavigationRule::Custom,
            FNavigationDelegate::CreateLambda([this](EUINavigation) -> TSharedPtr<SWidget>
            {
                if (!Controller.IsValid() || !Controller->IsBookOpen() || Dialog != EDialog::None || bSaving) return nullptr;
                const int32 Remembered = Entries.IsEmpty() ? -1 : ContentSelection;
                for (const auto& Target : FocusTargets)
                    if (Target.region == ERegion::Content && Target.index == Remembered
                        && IsTargetAvailable(Target.region, Target.index))
                    {
                        const auto Widget = Target.widget.Pin();
                        if (Widget && Widget->SupportsKeyboardFocus()) return Widget;
                    }
                return nullptr;
            }));
        Anchor->AddMetadata(Navigation);
    }
    FocusTargets.Add({TargetRegion, Index, Anchor});
    return Anchor;
}

void SHomesteadMenu::AdoptFocus(ERegion TargetRegion, int32 Index)
{
    if (bSynchronizingFocus || Dialog != EDialog::None) return;
    const bool ChangedSubject = TargetRegion == ERegion::Content && (Region != TargetRegion || ContentSelection != Index);
    Region = TargetRegion;
    Hover = INDEX_NONE;
    switch (TargetRegion)
    {
    case ERegion::Tabs: FocusedTab = Index; break;
    case ERegion::Session: SessionSelection = Index; break;
    case ERegion::Inventory: InventorySelection = Index; break;
    case ERegion::Portrait: PortraitSelection = Index; break;
    case ERegion::Equipment: EquipmentSelection = Index; break;
    case ERegion::Actions: ActionSelection = Index; ScrollActionIntoView(); break;
    case ERegion::Recovery: RecoverySelection = Index; break;
    case ERegion::Details: DetailsSelection = Index; break;
    case ERegion::Content: if (ChangedSubject && Index >= 0) Select(Index, Index == ContentSelection); break;
    default: break;
    }
}

TSharedPtr<SWidget> SHomesteadMenu::FocusWidget() const
{
    if (Dialog != EDialog::None)
        return DialogSelection < 0 ? AmountControl : DialogButtons.IsValidIndex(DialogSelection) ? DialogButtons[DialogSelection] : nullptr;
    int32 Index = 0;
    switch (Region)
    {
    case ERegion::Tabs: Index = FocusedTab; break;
    case ERegion::Session: Index = SessionSelection; break;
    case ERegion::Inventory: Index = InventorySelection; break;
    case ERegion::Portrait: Index = PortraitSelection; break;
    case ERegion::Content: Index = Entries.IsEmpty() ? -1 : ContentSelection; break;
    case ERegion::Equipment: Index = EquipmentSelection; break;
    case ERegion::Actions: Index = ActionSelection; break;
    case ERegion::Recovery: Index = RecoverySelection; break;
    case ERegion::Details: Index = DetailsSelection; break;
    default: break;
    }
    for (const auto& Target : FocusTargets)
        if (Target.region == Region && Target.index == Index) return Target.widget.Pin();
    return nullptr;
}

void SHomesteadMenu::SynchronizeFocus()
{
    auto Target = FocusWidget();
    if (!Target || !Target->IsEnabled() || !Target->GetVisibility().IsVisible())
    {
        if (Dialog != EDialog::None)
        {
            DialogSelection = 0;
            bEditingAmount = false;
            Target = DialogButtons.IsEmpty() ? nullptr : DialogButtons[0];
        }
        else
        {
            for (const auto& Candidate : FocusTargets)
                if (IsTargetAvailable(Candidate.region, Candidate.index))
                {
                    const auto NewTarget = Candidate.widget.Pin();
                    const auto NewRegion = Candidate.region;
                    const int32 NewIndex = Candidate.index;
                    AdoptFocus(NewRegion, NewIndex);
                    Target = NewTarget;
                    break;
                }
        }
    }
    if (!Target) return;
    TGuardValue<bool> Guard(bSynchronizingFocus, true);
    FSlateApplication::Get().SetKeyboardFocus(Target, EFocusCause::Navigation);
    bFocusPending = FSlateApplication::Get().GetKeyboardFocusedWidget() != Target;
    if (Dialog != EDialog::None && DialogScroll) DialogScroll->ScrollDescendantIntoView(Target, false);
    ScrollActionIntoView();
}

bool SHomesteadMenu::IsTargetAvailable(ERegion TargetRegion, int32 Index) const
{
    for (const auto& Target : FocusTargets)
        if (Target.region == TargetRegion && Target.index == Index)
        {
            const auto Widget = Target.widget.Pin();
            return Widget && Widget->IsEnabled() && Widget->GetVisibility().IsVisible();
        }
    return false;
}

bool SHomesteadMenu::HasSynchronizedFocus() const
{
    return FocusWidget().IsValid() && FSlateApplication::Get().GetKeyboardFocusedWidget() == FocusWidget();
}

bool SHomesteadMenu::IsFocusedControlVisible() const
{
    const auto Target = FocusWidget();
    if (!Target) return false;
    const auto Container = Dialog != EDialog::None ? StaticCastSharedPtr<SWidget>(DialogScroll)
        : Region == ERegion::Content ? StaticCastSharedPtr<SWidget>(Scroll)
        : Region == ERegion::Actions ? StaticCastSharedPtr<SWidget>(DetailsScroll) : TSharedPtr<SWidget>();
    const FGeometry Bounds = Container ? Container->GetCachedGeometry() : GetCachedGeometry();
    const auto Geometry = Target->GetCachedGeometry();
    const FVector2D Min = Bounds.GetAbsolutePosition(), Max = Min + Bounds.GetAbsoluteSize();
    const FVector2D A = Geometry.GetAbsolutePosition(), B = A + Geometry.GetAbsoluteSize();
    return A.X >= Min.X - 1 && A.Y >= Min.Y - 1 && B.X <= Max.X + 1 && B.Y <= Max.Y + 1;
}

FString SHomesteadMenu::GetFocusedRegionName() const
{
    if (Dialog != EDialog::None) return bEditingAmount ? TEXT("AmountEdit") : TEXT("Dialog");
    const TCHAR* Names[] = {TEXT("Tabs"), TEXT("Session"), TEXT("Inventory"), TEXT("Portrait"),
        TEXT("Content"), TEXT("Equipment"), TEXT("Details"), TEXT("Actions"), TEXT("Recovery")};
    return Names[static_cast<int32>(Region)];
}

void SHomesteadMenu::Construct(const FArguments& Args)
{
    Controller = Args._Controller;
    ChildSlot
    [
        SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
        .BorderBackgroundColor(FLinearColor(0.015f, 0.03f, 0.02f, 0.78f)).Padding(0)
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
                            .Visibility_Lambda([this]() { return Controller.IsValid() && !Controller->Toast().IsEmpty()
                                ? EVisibility::Visible : EVisibility::Collapsed; })
                            [
                                SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                                .BorderBackgroundColor(MenuPine).Padding(12, 6)
                                [
                                    SNew(SScrollBox)
                                    + SScrollBox::Slot()
                                    [
                                        SNew(STextBlock).AutoWrapText(true)
                                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 17))
                                        .ColorAndOpacity_Lambda([this]() { return Controller.IsValid() && Controller->ToastIsError()
                                            ? FSlateColor(FLinearColor(1, 0.67f, 0.48f)) : FSlateColor(Muted); })
                                        .Text_Lambda([this]() { return FText::FromString(Controller.IsValid()
                                            ? Controller->Toast()
                                            : TEXT("Menu unavailable.")); })
                                    ]
                                ]
                            ]
                        ]
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

bool SHomesteadMenu::StartCraftHold(ECraftInput Input)
{
    StopCraftHold();
    if (!Controller.IsValid() || Dialog != EDialog::None || bSaving || SeenPage != 1
        || Region != ERegion::Content || !Entries.IsValidIndex(ContentSelection))
        return false;
    const auto& Row = Entries[ContentSelection];
    if (Row.Subject != EHomesteadMenuSubject::Recipe || !Row.HasRecipeState
        || !Row.RecipeState.craftable)
        return false;
    CraftHoldRecipe = Row.SubjectId;
    CraftHoldElapsed = 0;
    CraftBeat = 0;
    CraftInput = Input;
    return true;
}

void SHomesteadMenu::StopCraftHold()
{
    CraftHoldRecipe = INDEX_NONE;
    CraftHoldElapsed = 0;
    CraftBeat = 0;
    CraftInput = ECraftInput::None;
}

bool SHomesteadMenu::IsHoldingRecipe(int32 Recipe) const
{
    return CraftInput != ECraftInput::None && CraftHoldRecipe == Recipe;
}

int32 SHomesteadMenu::Columns() const
{
    const bool Expanded = LogicalBookWidth() >= 1800;
    return SeenPage == 0 ? (Expanded ? 12 : 7)
        : SeenPage <= 2 ? (Expanded ? 10 : 6) : SeenPage == 4 || SeenPage == 6 ? 2 : 1;
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

TSharedRef<SWidget> SHomesteadMenu::BuildBody()
{
    if (bRecovery)
    {
        return SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(24)[ Text(TEXT("Time to try again"), 32) ]
            + SVerticalBox::Slot().FillHeight(1).Padding(24)
            [ Text(TEXT("You ran out of warmth, food, or energy.\n\nReturn to a recovery checkpoint, or open Settings to quit. No failed state will replace your usable checkpoint."), 23) ]
            + SVerticalBox::Slot().AutoHeight().Padding(24, 8)
            [ RegisterButton(MakeButton(TEXT("Retry checkpoint  [A / Enter]"), [this]() { Controller->MenuRetry(); }), ERegion::Recovery, 0) ]
            + SVerticalBox::Slot().AutoHeight().Padding(24, 8)
            [ RegisterButton(MakeButton(TEXT("Settings / Quit  [Y / G]"), [this]() { ChangePage(4); }), ERegion::Recovery, 1) ];
    }
    TSharedPtr<SVerticalBox> Body;
    TSharedPtr<SUniformGridPanel> Grid;
    TSharedPtr<SUniformGridPanel> ChestGrid;
    TSharedPtr<SUniformGridPanel> PackGrid;
    auto Result = SNew(SVerticalBox)
        + SVerticalBox::Slot().FillHeight(1)
        [
            SAssignNew(Body, SVerticalBox)
        ];
    const auto BuildSettings = [this]() -> TSharedRef<SWidget>
    {
        AudioSliders.Init(nullptr, 3);
        TSharedPtr<SVerticalBox> RowsBox;
        auto Result = SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
            [
                RegisterButton(MakeButton(Controller->IsFailed() ? TEXT("Return to recovery") : TEXT("Resume"),
                    [this]() { Back(); }, TAttribute<FSlateColor>::CreateLambda([this]()
                        { return Region == ERegion::Session ? MenuGold : MenuPine; })), ERegion::Session, 0)
            ]
            + SVerticalBox::Slot().FillHeight(1)
            [
                SAssignNew(Scroll, SScrollBox).Clipping(EWidgetClipping::ClipToBounds)
                + SScrollBox::Slot()
                [
                    SAssignNew(RowsBox, SVerticalBox)
                ]
            ];

        const auto OptionButton = [this](const FString& Label, bool SelectedOption, TFunction<void()> Action)
        {
            return SNew(SMenuButton).ButtonStyle(&MenuButtonStyle()).IsFocusable(false).ContentPadding(FMargin(10, 6))
                .ButtonColorAndOpacity(SelectedOption ? MenuGold : Selected)
                .OnClicked_Lambda([this, Action]() { if (PointerAction()) Action(); return FReply::Handled(); })
                [
                    SNew(STextBlock).Text(FText::FromString(Label))
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 15))
                    .ColorAndOpacity(SelectedOption ? FSlateColor(MenuPine) : FSlateColor(Ink))
                ];
        };

        for (int32 Index = 0; Index < Entries.Num(); ++Index)
        {
            const FHomesteadRow& Row = Entries[Index];
            TAttribute<FText> RowLabel = FText::FromString(Row.Label);
            if (Row.Id >= 5 && Row.Id <= 7)
            {
                const int32 AudioId = Row.Id;
                const FString Prefix = Row.Id == 5 ? TEXT("Music volume")
                    : Row.Id == 6 ? TEXT("Ambience volume") : TEXT("Effects volume");
                RowLabel = TAttribute<FText>::CreateLambda([this, AudioId, Prefix]()
                {
                    return FText::FromString(FString::Printf(TEXT("%s: %d%%"), *Prefix,
                        FMath::RoundToInt(Controller->MenuAudioVolume(AudioId) * 100)));
                });
            }
            TSharedPtr<SVerticalBox> RowContent;
            auto Content = SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(STextBlock).Text(RowLabel)
                    .ColorAndOpacity(Row.Id == 13 && !Controller->IsAutosaveEnabled() ? Muted : Ink)
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 3, 0, 0)
                [
                    SNew(STextBlock).Text(FText::FromString(Row.Detail)).ColorAndOpacity(Muted)
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 14)).AutoWrapText(true)
                ];
            RowContent = Content;

            if (Row.Id == 2)
            {
                TSharedPtr<SHorizontalBox> Choices;
                RowContent->AddSlot().AutoHeight().Padding(0, 8, 0, 0)[SAssignNew(Choices, SHorizontalBox)];
                const double Current = Controller->State().dayMinutes;
                const TPair<const TCHAR*, double> Options[] = {
                    {TEXT("Leisurely"), 120}, {TEXT("Balanced"), 60}, {TEXT("Fast"), 30}};
                for (const auto& Option : Options)
                    Choices->AddSlot().AutoWidth().Padding(0, 0, 6, 0)
                    [OptionButton(Option.Key, FMath::IsNearlyEqual(Current, Option.Value),
                        [this, Value = Option.Value]() { Controller->MenuSetGameSpeed(Value); Refresh(); })];
            }
            else if (Row.Id >= 5 && Row.Id <= 7)
            {
                const int32 AudioId = Row.Id;
                RowContent->AddSlot().AutoHeight().Padding(0, 8, 0, 0)
                [
                    SNew(SBox).WidthOverride_Lambda([]()
                    {
                        return FOptionalSize(FMath::Min(LogicalBookWidth() - 80.0f, 1280.0f));
                    })
                    [
                        SAssignNew(AudioSliders[AudioId - 5], SSlider)
                        .Value_Lambda([this, AudioId]() { return Controller->MenuAudioVolume(AudioId); })
                        .OnMouseCaptureBegin_Lambda([this, AudioId, Index]()
                        {
                            AudioEditId = AudioId;
                            AudioEditStart = Controller->MenuAudioVolume(AudioId);
                            Region = ERegion::Content;
                            Select(Index);
                        })
                        .OnValueChanged_Lambda([this, AudioId](float Value)
                        {
                            Controller->MenuPreviewAudioVolume(AudioId, Value);
                        })
                        .OnMouseCaptureEnd_Lambda([this, AudioId]()
                        {
                            const float Current = Controller->MenuAudioVolume(AudioId);
                            Controller->MenuCommitAudioVolume(AudioId, Current, AudioEditStart);
                            AudioEditId = -1;
                            Refresh();
                        })
                    ]
                ];
            }
            else if (Row.Id == 12)
            {
                TSharedPtr<SHorizontalBox> Choices;
                RowContent->AddSlot().AutoHeight().Padding(0, 8, 0, 0)[SAssignNew(Choices, SHorizontalBox)];
                Choices->AddSlot().AutoWidth().Padding(0, 0, 6, 0)
                [OptionButton(TEXT("On"), Controller->IsAutosaveEnabled(),
                    [this]() { Controller->MenuSetAutosaveEnabled(true); Refresh(); })];
                Choices->AddSlot().AutoWidth()
                [OptionButton(TEXT("Off"), !Controller->IsAutosaveEnabled(),
                    [this]() { Controller->MenuSetAutosaveEnabled(false); Refresh(); })];
            }
            else if (Row.Id == 13)
            {
                TSharedPtr<SHorizontalBox> Choices;
                RowContent->AddSlot().AutoHeight().Padding(0, 8, 0, 0)
                [
                    SAssignNew(Choices, SHorizontalBox)
                    .IsEnabled(Controller->IsAutosaveEnabled())
                ];
                for (const int32 Minutes : {5, 10, 20, 30})
                    Choices->AddSlot().AutoWidth().Padding(0, 0, 6, 0)
                    [OptionButton(FString::Printf(TEXT("%d min"), Minutes), Controller->AutosaveIntervalMinutes() == Minutes,
                        [this, Minutes]() { Controller->MenuSetAutosaveInterval(Minutes); Refresh(); })];
            }

            TSharedRef<SWidget> RowWidget = Row.Id == 14
                ? StaticCastSharedRef<SWidget>(SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                    .BorderBackgroundColor(MenuPine).Padding(14)[RowContent.ToSharedRef()])
                : FocusAnchor(
                    SNew(SMenuButton).ButtonStyle(&MenuButtonStyle()).IsFocusable(false).ContentPadding(14)
                    .ButtonColorAndOpacity_Lambda([this, Index]() { return ContentSelection == Index ? Selected : MenuPine; })
                    .OnClicked_Lambda([this, Index, Id = Row.Id]()
                    {
                        if (PointerAction())
                        {
                            Region = ERegion::Content;
                            Select(Index);
                            if (Id != 2 && (Id < 5 || Id > 7) && Id != 12 && Id != 13)
                                RunAction(EHomesteadItemAction::Primary);
                        }
                        return FReply::Handled();
                    })[RowContent.ToSharedRef()],
                    ERegion::Content, Index);
            Cells.Add(RowWidget);
            RowsBox->AddSlot().AutoHeight().Padding(0, 0, 0, 7)[RowWidget];
        }
        return Result;
    };
    if (SeenPage == 4) return BuildSettings();
    const bool Storage = SeenPage == 0 && Controller->ActiveStorageChest().IsSet();
    TSharedPtr<SHorizontalBox> ColumnsBox;
    Body->AddSlot().FillHeight(1)[ SAssignNew(ColumnsBox, SHorizontalBox) ];
    if ((SeenPage == 0 || SeenPage == 6) && !Storage && Controller->MenuPortraitBrush())
    {
        ColumnsBox->AddSlot().AutoWidth().Padding(0, 0, 12, 0)
        [
            SNew(SBox).WidthOverride(PortraitColumnWidth()).Clipping(EWidgetClipping::ClipToBounds)
            [
                SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).Padding(2)
                .BorderBackgroundColor_Lambda([this]() { return Region == ERegion::Portrait ? MenuGold : MenuPine; })
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().FillHeight(1)
                    [
                        FocusAnchor(SNew(SScaleBox).Stretch(EStretch::ScaleToFit).HAlign(HAlign_Center).VAlign(VAlign_Center)
                        [ SNew(SImage).Image_Lambda([this]() { return Controller.IsValid() ? Controller->MenuPortraitBrush() : nullptr; }) ], ERegion::Portrait, -1)
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(8)
                    [
                        SNew(STextBlock).AutoWrapText(true).ColorAndOpacity(Ink)
                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 16))
                        .Text_Lambda([this]() { return FText::FromString(Controller->MenuPortraitStatus()); })
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 8)
                    [
                        SNew(SBox).HeightOverride(44)
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 4, 0)
                            [ RegisterButton(MakeButton(TEXT("<"), [this]() { Controller->OrbitMenuPortrait(-20); },
                                TAttribute<FSlateColor>::CreateLambda([this]() { return Region == ERegion::Portrait && PortraitSelection == 0 ? MenuGold : MenuPine; }),
                                TEXT("Turn character left"), FMargin(8)), ERegion::Portrait, 0) ]
                            + SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 4, 0)
                            [ RegisterButton(MakeButton(TEXT(">"), [this]() { Controller->OrbitMenuPortrait(20); },
                                TAttribute<FSlateColor>::CreateLambda([this]() { return Region == ERegion::Portrait && PortraitSelection == 1 ? MenuGold : MenuPine; }),
                                TEXT("Turn character right"), FMargin(8)), ERegion::Portrait, 1) ]
                            + SHorizontalBox::Slot().FillWidth(1)
                            [ RegisterButton(MakeButton(TEXT("Zoom"), [this]() { Controller->ZoomMenuPortrait(); },
                                TAttribute<FSlateColor>::CreateLambda([this]() { return Region == ERegion::Portrait && PortraitSelection == 2 ? MenuGold : MenuPine; }),
                                TEXT("Toggle close-up and full-body view"), FMargin(8)), ERegion::Portrait, 2) ]
                        ]
                    ]
                ]
            ]
        ];
    }
    TSharedPtr<SVerticalBox> InventoryColumn;
    ColumnsBox->AddSlot().FillWidth(1).Padding(0, 0, 16, 0)
        [
            SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(MenuPine).Padding(12)
            .Clipping(EWidgetClipping::ClipToBounds)
            [
                SAssignNew(InventoryColumn, SVerticalBox)
            ]
        ];
    if (SeenPage == 0 && !Storage)
    {
        InventoryColumn->AddSlot().AutoHeight().Padding(4, 0, 4, 6)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
            [
                SNew(SBox).WidthOverride(42).HeightOverride(42)
                [
                    SNew(SMenuButton).ButtonStyle(&MenuButtonStyle()).IsFocusable(false)
                    .ContentPadding(5).ButtonColorAndOpacity(Selected)
                    .ToolTipText(FText::FromString(TEXT("Sort pack")))
                    .OnClicked_Lambda([this]()
                    {
                        if (PointerAction() && Controller->MenuSortPack()) Refresh();
                        return FReply::Handled();
                    })
                    [ SNew(SHomesteadIcon).Kind(FName(TEXT("sort"))).Tint(MenuGold) ]
                ]
            ]
            + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
            [ Text(Controller->BookTitle(), 26) ]
        ];
    }
    else
        InventoryColumn->AddSlot().AutoHeight().Padding(4, 0, 4, 6)
        [ Text(SeenPage <= 2 ? Controller->BookTitle() : Tabs[SeenPage], 26) ];
    const FString Summary = SeenPage == 0 ? Controller->MenuInventorySummary() : Controller->BookSummary();
    if (!Summary.IsEmpty())
        InventoryColumn->AddSlot().AutoHeight().Padding(4, 0, 4, 12)[ Text(Summary, 17) ];
    if (Storage)
    {
        TSharedPtr<SVerticalBox> ChestColumn;
        TSharedPtr<SVerticalBox> PackColumn;
        InventoryColumn->AddSlot().FillHeight(1)
        [
            SAssignNew(Scroll, SScrollBox).Clipping(EWidgetClipping::ClipToBounds)
            + SScrollBox::Slot()
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 8, 0)
                [
                    SAssignNew(ChestColumn, SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight().Padding(4, 0, 4, 6)
                    [ Text(TEXT("Chest"), 20) ]
                    + SVerticalBox::Slot().AutoHeight()
                    [ SAssignNew(ChestGrid, SUniformGridPanel).SlotPadding(FMargin(3)) ]
                ]
                + SHorizontalBox::Slot().FillWidth(1).Padding(8, 0, 0, 0)
                [
                    SAssignNew(PackColumn, SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight().Padding(4, 0, 4, 6)
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
                        [ Text(TEXT("Pack"), 20) ]
                        + SHorizontalBox::Slot().AutoWidth()
                        [
                            SNew(SBox).WidthOverride(34).HeightOverride(34)
                            [
                                SNew(SMenuButton).ButtonStyle(&MenuButtonStyle()).IsFocusable(false)
                                .ContentPadding(4).ButtonColorAndOpacity(Selected)
                                .ToolTipText(FText::FromString(TEXT("Sort pack")))
                                .OnClicked_Lambda([this]()
                                {
                                    if (PointerAction() && Controller->MenuSortPack()) Refresh();
                                    return FReply::Handled();
                                })
                                [ SNew(SHomesteadIcon).Kind(FName(TEXT("sort"))).Tint(MenuGold) ]
                            ]
                        ]
                    ]
                    + SVerticalBox::Slot().AutoHeight()
                    [ SAssignNew(PackGrid, SUniformGridPanel).SlotPadding(FMargin(3)) ]
                ]
            ]
        ];
    }
    else
    {
        InventoryColumn->AddSlot().FillHeight(1)
        [
            SAssignNew(Scroll, SScrollBox).Clipping(EWidgetClipping::ClipToBounds)
            + SScrollBox::Slot().HAlign(SeenPage <= 2 ? HAlign_Left : HAlign_Fill)
            [ SAssignNew(Grid, SUniformGridPanel).SlotPadding(FMargin(4)) ]
        ];
    }
    if (SeenPage == 0 && !Storage)
    {
        TSharedPtr<SHorizontalBox> EquipmentBar;
        InventoryColumn->AddSlot().AutoHeight().Padding(0, 8, 0, 4)[ Text(TEXT("Equipped slots"), 16) ];
        InventoryColumn->AddSlot().AutoHeight()[ SAssignNew(EquipmentBar, SHorizontalBox) ];
        for (int32 Index = 0; Index < 3; ++Index)
        {
            const TCHAR* SlotIcons[] = {TEXT("slot-torso"), TEXT("slot-apron"), TEXT("slot-feet")};
            EquipmentBar->AddSlot().FillWidth(1).Padding(3, 0)
            [
                RegisterButton(SNew(SMenuButton).ButtonStyle(&MenuButtonStyle()).IsFocusable(true).ContentPadding(8)
                    .ButtonColorAndOpacity_Lambda([this, Index]()
                        { return Region == ERegion::Equipment && EquipmentSelection == Index ? MenuGold : Selected; })
                    .OnClicked_Lambda([this, Index]()
                        { if (PointerAction()) FocusEquipment(Index); return FReply::Handled(); })
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
                        [ SNew(SBox).WidthOverride(36).HeightOverride(36)
                            [ SNew(SHomesteadIcon).Kind(FName(SlotIcons[Index])) ] ]
                        + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
                        [ Text(EquipmentLabel(Index), 15) ]
                    ], ERegion::Equipment, Index)
            ];
        }
    }
    ColumnsBox->AddSlot().AutoWidth()
        [
            SNew(SBox).WidthOverride(DetailsColumnWidth()).Clipping(EWidgetClipping::ClipToBounds)
            [
                SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).Padding(2)
                .BorderBackgroundColor_Lambda([this]() { return Region == ERegion::Details ? MenuGold : MenuPine; })
                [
                    SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(MenuPine).Padding(16)
                    [ SAssignNew(DetailsHost, SBox)[ BuildDetails() ] ]
                ]
            ]
        ];
    if (Entries.IsEmpty() && Grid)
        Grid->AddSlot(0, 0)[ FocusAnchor(SNew(SBox).WidthOverride_Lambda([this]()
            { return FMath::Max(ItemCellWidth, Scroll->GetCachedGeometry().GetLocalSize().X > 0
                ? static_cast<float>(Scroll->GetCachedGeometry().GetLocalSize().X) - 24.0f : 480.0f); })
            [Text(SeenPage == 0 && Controller->InventoryView() == 1
            ? TEXT("This chest is empty.")
            : SeenPage == 0 && Controller->InventoryView() == 2 ? TEXT("No removable clothing is equipped.")
            : TEXT("Your pack is empty."))], ERegion::Content, -1) ];
    int32 ChestCell = 0;
    int32 PackCell = 0;
    for (int32 Index = 0; Index < Entries.Num(); ++Index)
    {
        const auto& Row = Entries[Index];
        const FString Name = EntryName(Row);
        TSharedPtr<SWidget> Cell;
        TSharedPtr<SVerticalBox> Contents;
        auto Button = RegisterButton(SNew(SMenuButton).ButtonStyle(&MenuButtonStyle()).IsFocusable(true).ContentPadding(7)
            .ButtonColorAndOpacity_Lambda([this, Index]() { return CellColor(Index); })
            .ToolTipText(FText::FromString(Name + TEXT("\n") + Row.Detail))
            .OnHovered_Lambda([this, Index]() { if (Controller.IsValid() && !Controller->UsesGamepad()) Hover = Index; })
            .OnUnhovered_Lambda([this, Index]() { if (Hover == Index) Hover = INDEX_NONE; })
            .OnPressed_Lambda([this, Index]()
            {
                if (PointerAction() && Entries.IsValidIndex(Index)
                    && Entries[Index].Subject == EHomesteadMenuSubject::Recipe)
                {
                    StopCraftHold();
                    Region = ERegion::Content;
                    Select(Index);
                    StartCraftHold(ECraftInput::Pointer);
                }
                else if (PointerAction() && SeenPage == 0 && Entries.IsValidIndex(Index)
                    && (Entries[Index].Subject == EHomesteadMenuSubject::ItemGroup
                        || Entries[Index].Subject == EHomesteadMenuSubject::Wearable))
                {
                    CancelVirtualItemDrag();
                    Region = ERegion::Content;
                    Select(Index);
                    BeginPointerItemDrag(Index);
                }
            })
            .OnReleased_Lambda([this]()
            {
                if (CraftInput == ECraftInput::Pointer) StopCraftHold();
                if (bPointerItemDown) EndPointerItemDrag();
            })
            .OnClicked_Lambda([this, Index]()
            {
                if (PointerAction() && Dialog == EDialog::None)
                {
                    if (bSuppressItemClick) { bSuppressItemClick = false; return FReply::Handled(); }
                    Region = ERegion::Content;
                    Select(Index);
                    if (SeenPage == 0 && bControl) SplitSelectedHalf();
                    if (Entries.IsValidIndex(ContentSelection)
                        && IsDirectCameraSetting(Entries[ContentSelection]))
                        RunAction(EHomesteadItemAction::Primary);
                }
                return FReply::Handled();
            })
            [ SAssignNew(Contents, SVerticalBox) ], ERegion::Content, Index);
        if (SeenPage == 0)
        {
            Contents->AddSlot().AutoHeight().HAlign(HAlign_Center)
            [
                SNew(SBox).WidthOverride(58).HeightOverride(58)
                [
                    SNew(SOverlay)
                    + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
                    [ SNew(SBox).WidthOverride(48).HeightOverride(48)
                        [ SNew(SHomesteadIcon).Kind(EntryIcon(Row)).Tint(Row.IconTint) ] ]
                    + SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom)
                    [
                        SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                        .BorderBackgroundColor(MenuPine).Padding(FMargin(4, 1))
                        [
                            SNew(STextBlock).Text(FText::AsNumber(FMath::Max(1, Row.Quantity)))
                            .ColorAndOpacity(Ink)
                            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 15))
                        ]
                    ]
                ]
            ];
        }
        else if (SeenPage <= 2)
        {
            if (Row.Subject == EHomesteadMenuSubject::Recipe)
            {
                Contents->AddSlot().AutoHeight().HAlign(HAlign_Center)
                [
                    SNew(SBox).WidthOverride(58).HeightOverride(58)
                    [
                        SNew(SOverlay)
                        + SOverlay::Slot()
                        [
                            SNew(SHomesteadIcon).Kind(EntryIcon(Row))
                            .Tint(MenuGold).Desaturation(1.0f)
                        ]
                        + SOverlay::Slot().VAlign(VAlign_Bottom)
                        [
                            SNew(SBox).WidthOverride(58)
                            .HeightOverride_Lambda([this, Recipe = Row.SubjectId]()
                            {
                                return IsHoldingRecipe(Recipe)
                                    ? 58.0f * FMath::Clamp(GetCraftProgress(), 0.0f, 1.0f) : 58.0f;
                            })
                            .Clipping(EWidgetClipping::ClipToBounds)
                            [
                                SNew(SBox).WidthOverride(58).HeightOverride(58).VAlign(VAlign_Bottom)
                                [
                                    SNew(SHomesteadIcon).Kind(EntryIcon(Row)).Tint(MenuGold)
                                    .Desaturation(Row.RecipeState.craftable ? 0.0f : 1.0f)
                                ]
                            ]
                        ]
                    ]
                ];
            }
            else
            {
                Contents->AddSlot().AutoHeight().HAlign(HAlign_Center)
                [ SNew(SBox).WidthOverride(48).HeightOverride(48)[ SNew(SHomesteadIcon).Kind(EntryIcon(Row)).Tint(Row.IconTint) ] ];
            }
        }
        if (SeenPage > 1)
            Contents->AddSlot().AutoHeight()[ Text(Name, SeenPage <= 2 ? 16 : 18) ];
        Cell = SNew(SBox).WidthOverride(SeenPage <= 2 ? FOptionalSize(ItemCellWidth) : FOptionalSize())
            .MinDesiredWidth(SeenPage <= 2 ? ItemCellWidth : SeenPage == 4 ? 330 : SeenPage == 6 ? 250 : 670)
            .MinDesiredHeight(SeenPage == 0 ? 76 : SeenPage == 1 ? 96 : SeenPage <= 2 ? 144 : 72)
            [
                SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).Padding(2)
                .BorderBackgroundColor_Lambda([this, Index]() { return Index == ContentSelection ? MenuGold : FLinearColor::Transparent; })
                [ Button ]
            ];
        Cells.Add(Cell);
        if (Storage)
        {
            const bool InChest = Row.ContainerId > 0;
            const int32 CellIndex = InChest ? ChestCell++ : PackCell++;
            auto TargetGrid = InChest ? ChestGrid : PackGrid;
            TargetGrid->AddSlot(CellIndex % 4, CellIndex / 4)[ Cell.ToSharedRef() ];
        }
        else Grid->AddSlot(Index % Columns(), Index / Columns())[ Cell.ToSharedRef() ];
    }
    return Result;
}

TSharedRef<SWidget> SHomesteadMenu::BuildDetails()
{
    FocusTargets.RemoveAll([](const FFocusTarget& Target) { return Target.region == ERegion::Details || Target.region == ERegion::Actions; });
    TSharedPtr<SVerticalBox> DetailsContent;
    auto Result = SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
        [
            FocusAnchor(SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 12, 0)
            [
                SNew(SBox).WidthOverride(48).HeightOverride(48)
                [
                    SNew(SHomesteadIcon).Kind_Lambda([this]()
                    {
                        const int32 Index = DetailIndex();
                        return Entries.IsValidIndex(Index) ? EntryIcon(Entries[Index]) : FName(TEXT("pack"));
                    })
                    .Tint_Lambda([this]()
                    {
                        const int32 Index = DetailIndex();
                        return Entries.IsValidIndex(Index) ? Entries[Index].IconTint : MenuGold;
                    })
                ]
            ]
            + SHorizontalBox::Slot().FillWidth(1)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(STextBlock).WrapTextAt(DetailsColumnWidth() - 100).ColorAndOpacity(Ink)
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 22))
                    .Text_Lambda([this]()
                    {
                        const int32 Index = DetailIndex();
                        return FText::FromString(Entries.IsValidIndex(Index) ? EntryName(Entries[Index]) : TEXT("Details"));
                    })
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
                [
                    SNew(STextBlock).WrapTextAt(DetailsColumnWidth() - 100).ColorAndOpacity(Muted)
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 17))
                    .Text_Lambda([this]()
                    {
                        const int32 Index = DetailIndex();
                        if (!Entries.IsValidIndex(Index)) return FText();
                        const auto& Row = Entries[Index];
                        return FText::FromString(Row.Quantity > 0 ? FString::Printf(TEXT("%s: %d"), *Row.Location, Row.Quantity) : Row.Location);
                    })
                ]
            ], ERegion::Details, 0)
        ]
        + SVerticalBox::Slot().FillHeight(1)
        [
            SAssignNew(DetailsScroll, SScrollBox).Clipping(EWidgetClipping::ClipToBounds)
            + SScrollBox::Slot()
            [
                SAssignNew(DetailsContent, SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 16)
                [
                    FocusAnchor(SNew(STextBlock).WrapTextAt(DetailsColumnWidth() - 56).ColorAndOpacity(Ink)
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 18))
                    .Text_Lambda([this]() { return FText::FromString(DetailsBodyText()); }), ERegion::Details, 1)
                ]
            ]
        ];
    RequirementHints.Reset();
    if (Entries.IsValidIndex(ContentSelection)
        && Entries[ContentSelection].Subject == EHomesteadMenuSubject::Recipe
        && Entries[ContentSelection].HasRecipeState)
    {
        const auto& Assessment = Entries[ContentSelection].RecipeState;
        const auto AddRequirement = [this, &DetailsContent](FName Icon, const FString& Label,
            const FString& Status, const FString& Source, bool Met)
        {
            const int32 FocusIndex = RequirementHints.Num() + 2;
            RequirementHints.Add(Met ? FString() : Source);
            const FString FullDetail = FString::Printf(TEXT("%s: %s. %s. %s"),
                *Label, *Status, Met ? TEXT("Ready") : TEXT("Missing"),
                Met ? TEXT("Requirement met") : Source.IsEmpty()
                    ? TEXT("No other requirement") : *Source);
            auto Box = SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                .BorderBackgroundColor(Met ? FLinearColor(0.075f, 0.16f, 0.10f, 1)
                    : FLinearColor(0.24f, 0.075f, 0.055f, 1))
                .Padding(FMargin(8, 5)).ToolTipText(FText::FromString(FullDetail))
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight()
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 9, 0)
                        [ SNew(SBox).WidthOverride(28).HeightOverride(28)
                            [ SNew(SHomesteadIcon).Kind(Icon).Tint(Met ? MenuGold : Ink) ] ]
                        + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
                        [ SNew(STextBlock).Text(FText::FromString(Label)).ColorAndOpacity(Ink)
                            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 16)) ]
                        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(5, 0)
                        [ SNew(STextBlock).Text(FText::FromString(Status)).ColorAndOpacity(Ink)
                            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 16)) ]
                        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                        [ SNew(STextBlock).Text(FText::FromString(Met ? TEXT("[+]") : TEXT("[-]")))
                            .ColorAndOpacity(Met ? MenuGold : FLinearColor(1.0f, 0.66f, 0.52f))
                            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 16)) ]
                    ]
                    + SVerticalBox::Slot().AutoHeight()
                    [
                        SNew(STextBlock).Text(FText::FromString(Source)).ColorAndOpacity(Muted)
                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
                        .Visibility_Lambda([this, FocusIndex, Met, Source]()
                        {
                            return !Met && !Source.IsEmpty() && Region == ERegion::Details
                                && DetailsSelection == FocusIndex
                                ? EVisibility::Visible : EVisibility::Collapsed;
                        })
                    ]
                ];
            DetailsContent->AddSlot().AutoHeight().Padding(0, 0, 0, 5)
            [FocusAnchor(Box, ERegion::Details, FocusIndex)];
        };
        for (const auto& Ingredient : Assessment.ingredients)
        {
            AddRequirement(RequirementIcon(Ingredient.item),
                UTF8_TO_TCHAR(Homestead::ItemName(Ingredient.item)),
                FString::Printf(TEXT("%d / %d"), Ingredient.have, Ingredient.need),
                UTF8_TO_TCHAR(Ingredient.source), Ingredient.met);
        }
        if (Assessment.retainedTool != Homestead::Item::Count)
        {
            const int32 Have = Controller.IsValid()
                ? Controller->Simulation().Count(Assessment.retainedTool) : 0;
            AddRequirement(RequirementIcon(Assessment.retainedTool),
                FString::Printf(TEXT("%s (kept)"),
                    UTF8_TO_TCHAR(Homestead::ItemName(Assessment.retainedTool))),
                FString::Printf(TEXT("%d / 1"), Have), TEXT("Required tool; not consumed"),
                Assessment.retainedToolMet);
        }
        if (Assessment.stationRequired)
            AddRequirement(FName(TEXT("fire")), TEXT("Cookfire"),
                Assessment.stationMet ? TEXT("Ready") : TEXT("Missing"),
                TEXT("Fueled cookfire nearby"), Assessment.stationMet);
        if (!Assessment.capacityMet)
            AddRequirement(FName(TEXT("pack")), TEXT("Pack space"), TEXT("Full"),
                TEXT("Make room for the crafted output"), false);
    }
    Actions.Reset();
    ActionButtons.Reset();
    if (Entries.IsValidIndex(ContentSelection))
    {
        const auto& Row = Entries[ContentSelection];
        if (Row.Subject == EHomesteadMenuSubject::Wearable)
        {
            if (Row.ContainerId < 0) Actions.Add(EHomesteadItemAction::Unequip);
            else if (Row.ContainerId == 0)
            {
                Actions.Add(EHomesteadItemAction::Equip);
                Actions.Add(EHomesteadItemAction::Drop);
            }
            const auto* Info = Homestead::GetWearableDefinition(static_cast<Homestead::WearableDefinition>(Row.Id));
            if (Info && Info->dyeable) Actions.Add(EHomesteadItemAction::Dye);
        }
        else if (Row.Subject == EHomesteadMenuSubject::ItemGroup)
        {
            if (Row.ContainerId == 0)
            {
                Actions.Add(EHomesteadItemAction::Primary);
                Actions.Add(EHomesteadItemAction::Drop);
            }
        }
        else if (Row.Subject != EHomesteadMenuSubject::Recipe && SeenPage != 3
            && SeenPage != 5 && !IsDirectCameraSetting(Row))
            Actions.Add(EHomesteadItemAction::Primary);
    }
    ActionSelection = FMath::Clamp(ActionSelection, 0, FMath::Max(0, Actions.Num() - 1));
    if (Actions.IsEmpty() && Region == ERegion::Actions) Region = ERegion::Details;
    TSharedPtr<SUniformGridPanel> ActionGrid;
    DetailsContent->AddSlot().AutoHeight().Padding(0, 4, 0, 0)
    [ SAssignNew(ActionGrid, SUniformGridPanel).SlotPadding(FMargin(3)) ];
    for (int32 Index = 0; Index < Actions.Num(); ++Index)
    {
        const auto Action = Actions[Index];
        auto ActionButton = RegisterButton(SNew(SMenuButton).ButtonStyle(&MenuButtonStyle()).IsFocusable(true).ContentPadding(8)
            .ButtonColorAndOpacity_Lambda([this, Index]() { return Region == ERegion::Actions && ActionSelection == Index ? MenuGold : Selected; })
            .OnClicked_Lambda([this, Action]() { if (PointerAction()) RunAction(Action); return FReply::Handled(); })
            [
                SNew(STextBlock).WrapTextAt(120)
                .ColorAndOpacity_Lambda([this, Index]() { return Region == ERegion::Actions && ActionSelection == Index ? FSlateColor(MenuPine) : FSlateColor(Ink); })
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 16))
                .Text_Lambda([this, Action]()
                {
                    return FText::FromString(ActionLabel(Action));
                })
            ], ERegion::Actions, Index);
        TSharedRef<SWidget> Button = SNew(SBox).WidthOverride(136).MinDesiredHeight(44)[ActionButton];
        ActionButtons.Add(Button);
        ActionGrid->AddSlot(Index % 2, Index / 2)[ Button ];
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
bool SHomesteadMenu::IsDirectCameraSetting(const FHomesteadRow& Row) const
{
    return SeenPage == 4 && Row.Subject == EHomesteadMenuSubject::Legacy
        && (Row.Id == 3 || Row.Id == 4);
}
int32 SHomesteadMenu::DetailIndex() const { return Hover != INDEX_NONE ? Hover : ContentSelection; }
FString SHomesteadMenu::GetFocusedRequirementHint() const
{
    const int32 Index = DetailsSelection - 2;
    return Region == ERegion::Details && RequirementHints.IsValidIndex(Index)
        ? RequirementHints[Index] : FString();
}
FString SHomesteadMenu::DetailsText() const
{
    const int32 Index = DetailIndex();
    if (!Entries.IsValidIndex(Index)) return DetailsBodyText();
    const auto& Row = Entries[Index];
    if (Row.Subject == EHomesteadMenuSubject::Recipe && Row.HasRecipeState)
    {
        FString Result = Row.Label + TEXT("\n") + Row.Location + TEXT("\n") + Row.Detail;
        for (const auto& Ingredient : Row.RecipeState.ingredients)
            Result += FString::Printf(TEXT("\n%s: Have %d / Need %d (%s)"),
                UTF8_TO_TCHAR(Homestead::ItemName(Ingredient.item)), Ingredient.have,
                Ingredient.need, UTF8_TO_TCHAR(Ingredient.source));
        if (Row.RecipeState.retainedTool != Homestead::Item::Count)
            Result += FString::Printf(TEXT("\n%s (kept): %s"),
                UTF8_TO_TCHAR(Homestead::ItemName(Row.RecipeState.retainedTool)),
                Row.RecipeState.retainedToolMet ? TEXT("Ready") : TEXT("Missing"));
        if (Row.RecipeState.stationRequired)
            Result += FString::Printf(TEXT("\nFueled cookfire nearby: %s"),
                Row.RecipeState.stationMet ? TEXT("Ready") : TEXT("Not nearby"));
        Result += FString::Printf(TEXT("\nPack space: %s"),
            Row.RecipeState.capacityMet ? TEXT("Available") : TEXT("Full"));
        return Result;
    }
    return Row.Label + TEXT("\n") + Row.Location + TEXT("\n\n") + DetailsBodyText();
}
FString SHomesteadMenu::DetailsBodyText() const
{
    const int32 Index = DetailIndex();
    if (!Entries.IsValidIndex(Index)) return TEXT("Select an item to see its details.\n\nNothing here is a recipe output you already own.");
    const auto& Row = Entries[Index];
    if (Row.Subject == EHomesteadMenuSubject::Recipe) return FString();
    FString Detail = Row.Detail;
    if (SeenPage == 4 && Controller.IsValid() && Controller->IsFailed())
        Detail += TEXT("\n\nRecovery: saving a failed state is disabled. Retry a checkpoint or quit explicitly.");
    return Detail;
}
void SHomesteadMenu::ScrollActionIntoView()
{
    if (Region == ERegion::Actions && DetailsScroll && ActionButtons.IsValidIndex(ActionSelection))
        DetailsScroll->ScrollDescendantIntoView(ActionButtons[ActionSelection], false, EDescendantScrollDestination::IntoView);
}
FString SHomesteadMenu::Footer() const
{
    if (!Controller.IsValid()) return {};
    if (bRecovery) return Controller->UsesGamepad() ? TEXT("D-pad / Left stick  Select    A  Activate    Y  Settings / Quit")
        : TEXT("Arrows  Select    Enter  Activate    G  Settings / Quit");
    const bool Pad = Controller->UsesGamepad();
    FString Hint = Pad ? TEXT("D-pad / Left stick  Move between sections    A  Activate    LB/RB  Tabs")
        : TEXT("Arrows  Move between sections    Enter  Activate    Ctrl+Tab  Tabs");
    if (Region == ERegion::Portrait) Hint += Pad ? TEXT("    Right stick  Turn    R3  Zoom") : TEXT("    Z  Zoom");
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
    case EHomesteadItemAction::Drop: return TEXT("Drop...");
    default: return Row.Action.IsEmpty() ? TEXT("Change / activate") : Row.Action;
    }
}

TSharedPtr<SWidget> SHomesteadMenu::GetAudioSliderWidget(int32 AudioId) const
{
    return SeenPage == 4 && AudioSliders.IsValidIndex(AudioId - 5)
        ? StaticCastSharedPtr<SWidget>(AudioSliders[AudioId - 5]) : nullptr;
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
        if (Entry >= 0) { Select(Entry); Region = ERegion::Actions; ActionSelection = 0; ScrollActionIntoView(); }
    }
}
FLinearColor SHomesteadMenu::CellColor(int32 Index) const
{
    if (bVirtualDraggingItem && Index == VirtualDragSource)
        return FLinearColor(0.045f, 0.055f, 0.05f, 0.72f);
    if (bVirtualDraggingItem && Index == ContentSelection)
        return MenuGold;
    if (bPointerDraggingItem && Index == PointerDragSource)
        return FLinearColor(0.045f, 0.055f, 0.05f, 0.72f);
    if (bPointerDraggingItem && Index == PointerDragTarget)
        return MenuGold;
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
        ScrollActionIntoView();
    }
}
void SHomesteadMenu::SplitSelectedHalf()
{
    if (!Controller.IsValid() || Dialog != EDialog::None || SeenPage != 0
        || Region != ERegion::Content || !Entries.IsValidIndex(ContentSelection))
        return;
    const auto Row = Entries[ContentSelection];
    if (Controller->MenuSplitHalf(Row)) Refresh();
}
void SHomesteadMenu::BeginPointerItemDrag(int32 Index)
{
    CancelPointerItemDrag();
    if (!Entries.IsValidIndex(Index)) return;
    PointerDragSource = Index;
    PointerDragStart = FSlateApplication::Get().GetCursorPos();
    PointerDragRevision = Controller->Simulation().GetRevision();
    bPointerItemDown = true;
}
void SHomesteadMenu::EndPointerItemDrag()
{
    const bool WasDragging = bPointerDraggingItem;
    const int32 Source = PointerDragSource;
    int32 Target = INDEX_NONE;
    if (WasDragging && FSlateApplication::IsInitialized())
    {
        const FVector2D Position = FSlateApplication::Get().GetCursorPos();
        for (int32 Index = 0; Index < Cells.Num(); ++Index)
            if (Index != Source && Cells[Index]
                && Cells[Index]->GetCachedGeometry().IsUnderLocation(Position))
            { Target = Index; break; }
    }
    bPointerItemDown = false;
    bPointerDraggingItem = false;
    PointerDragSource = INDEX_NONE;
    PointerDragTarget = INDEX_NONE;
    bSuppressItemClick = WasDragging;
    if (WasDragging && Entries.IsValidIndex(Source) && Entries.IsValidIndex(Target)
        && Source != Target)
        Controller->MenuDrop(Entries[Source], Entries[Target], PointerDragRevision);
}
void SHomesteadMenu::CancelPointerItemDrag()
{
    bPointerItemDown = false;
    bPointerDraggingItem = false;
    PointerDragSource = INDEX_NONE;
    PointerDragTarget = INDEX_NONE;
    bSuppressItemClick = false;
}
void SHomesteadMenu::BeginOrCommitVirtualItemDrag()
{
    if (!Controller.IsValid() || Dialog != EDialog::None || SeenPage != 0
        || Region != ERegion::Content || !Entries.IsValidIndex(ContentSelection))
        return;
    const auto& Row = Entries[ContentSelection];
    if (Row.Subject != EHomesteadMenuSubject::ItemGroup
        && Row.Subject != EHomesteadMenuSubject::Wearable)
        return;
    if (!bVirtualDraggingItem)
    {
        CancelPointerItemDrag();
        VirtualDragSource = ContentSelection;
        VirtualDragRevision = Controller->Simulation().GetRevision();
        bVirtualDraggingItem = true;
        return;
    }
    const int32 Source = VirtualDragSource;
    const int32 Target = ContentSelection;
    const uint64 Revision = VirtualDragRevision;
    CancelVirtualItemDrag();
    if (Entries.IsValidIndex(Source) && Entries.IsValidIndex(Target) && Source != Target)
        Controller->MenuDrop(Entries[Source], Entries[Target], Revision);
}
void SHomesteadMenu::CancelVirtualItemDrag()
{
    VirtualDragSource = INDEX_NONE;
    VirtualDragRevision = 0;
    bVirtualDraggingItem = false;
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
        if ((Action == EHomesteadItemAction::Transfer || Action == EHomesteadItemAction::Split
            || Action == EHomesteadItemAction::Drop)
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
        if (Action == EHomesteadItemAction::Drop && Row.Subject == EHomesteadMenuSubject::Wearable)
        { SetDialog(EDialog::DropWearable); return; }
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
    if (Dialog == EDialog::Amount && DialogSelection < 0)
    { bEditingAmount = !bEditingAmount; BuildDialog(); return; }
    if (Dialog != EDialog::None) { DialogAction(DialogSelection); return; }
    if (Region == ERegion::Tabs) ChangePage(FocusedTab);
    else if (Region == ERegion::Inventory) ChangeInventoryView(InventorySelection);
    else if (Region == ERegion::Equipment) FocusEquipment(EquipmentSelection);
    else if (Region == ERegion::Session) { if (SessionSelection == 0) Back(); else RequestExit(); }
    else if (Region == ERegion::Recovery) { if (RecoverySelection == 0) Controller->MenuRetry(); else ChangePage(4); }
    else if (Region == ERegion::Portrait)
    {
        if (PortraitSelection < 0) { PortraitSelection = 0; bFocusPending = true; SynchronizeFocus(); }
        else if (PortraitSelection < 2) Controller->OrbitMenuPortrait(PortraitSelection == 0 ? -20 : 20);
        else Controller->ZoomMenuPortrait();
    }
    else if (Region == ERegion::Actions && Actions.IsValidIndex(ActionSelection)) RunAction(Actions[ActionSelection]);
    else if (Region == ERegion::Content && Entries.IsValidIndex(ContentSelection)
        && SeenPage == 4)
        RunAction(EHomesteadItemAction::Primary);
    else
    {
        Region = Actions.IsEmpty() ? ERegion::Details : ERegion::Actions;
        ActionSelection = 0; bFocusPending = true; SynchronizeFocus();
    }
}
void SHomesteadMenu::ChangePage(int32 Page)
{
    if (!Controller.IsValid() || Dialog != EDialog::None) return;
    CancelPointerItemDrag();
    CancelVirtualItemDrag();
    StopCraftHold();
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
    bFocusPending = true;
    return true;
}
bool SHomesteadMenu::FocusSubject(EHomesteadMenuSubject Subject, int32 SubjectId, int32 ContainerId)
{
    const int32 Index = Entries.IndexOfByPredicate([=](const FHomesteadRow& Row)
        { return Row.Subject == Subject && Row.SubjectId == SubjectId && Row.ContainerId == ContainerId; });
    if (Index < 0) return false;
    Select(Index);
    Region = ERegion::Content;
    bFocusPending = true;
    SynchronizeFocus();
    return true;
}
bool SHomesteadMenu::FocusItemAction(EHomesteadItemAction Action)
{
    const int32 Index = Actions.IndexOfByKey(Action);
    if (Index < 0) return false;
    Region = ERegion::Actions;
    ActionSelection = Index;
    bFocusPending = true;
    SynchronizeFocus();
    return true;
}
void SHomesteadMenu::CycleRegion(int32 Direction)
{
    TArray<ERegion> Regions = {ERegion::Tabs};
    if (SeenPage == 4) Regions.Add(ERegion::Session);
    if (SeenPage == 4)
    {
        Regions.Add(ERegion::Content);
        Region = Regions[HomesteadMenuNavigation::Cycle(Regions.IndexOfByKey(Region), Regions.Num(), Direction)];
        Hover = INDEX_NONE;
        bFocusPending = true;
        SynchronizeFocus();
        return;
    }
    if (Controller->MenuPortraitBrush()) Regions.Add(ERegion::Portrait);
    Regions.Add(ERegion::Content);
    if (SeenPage == 0) Regions.Add(ERegion::Equipment);
    Regions.Add(ERegion::Details);
    if (!Actions.IsEmpty()) Regions.Add(ERegion::Actions);
    Region = Regions[HomesteadMenuNavigation::Cycle(Regions.IndexOfByKey(Region), Regions.Num(), Direction)];
    Hover = INDEX_NONE;
    ScrollActionIntoView();
    bFocusPending = true;
    SynchronizeFocus();
}

bool SHomesteadMenu::MoveWithin(int32& Index, int32 Count, int32 ColumnCount,
    HomesteadMenuNavigation::Direction Direction, int32 Desired)
{
    int32 Candidate = Index;
    for (int32 Attempt = 0; Attempt < Count; ++Attempt)
    {
        const auto Result = HomesteadMenuNavigation::Move(Candidate, Count, ColumnCount, Direction,
            Desired < 0 ? Candidate % FMath::Max(1, ColumnCount) : Desired);
        if (Result.boundary || Result.index < 0) return false;
        Candidate = Result.index;
        if (IsTargetAvailable(Region, Candidate)) { Index = Candidate; return true; }
    }
    return false;
}

void SHomesteadMenu::NavigateSpatial(HomesteadMenuNavigation::Direction Direction)
{
    const auto Target = FocusWidget();
    if (!Target || Target->GetCachedGeometry().GetLocalSize().IsNearlyZero())
    {
        bFocusPending = true;
        PendingDirection = Direction;
        return;
    }
    SynchronizeFocus();
    FWidgetPath Path;
    auto& Slate = FSlateApplication::Get();
    if (!Slate.GeneratePathToWidgetUnchecked(Target.ToSharedRef(), Path)) return;
    const EUINavigation Navigation = Direction.x < 0 ? EUINavigation::Left : Direction.x > 0 ? EUINavigation::Right
        : Direction.y < 0 ? EUINavigation::Up : EUINavigation::Down;
    Slate.ProcessReply(Path, FReply::Handled().SetNavigation(Navigation, ENavigationGenesis::User), nullptr, nullptr, 0);
}

void SHomesteadMenu::NavigateDirection(HomesteadMenuNavigation::Direction Direction)
{
    if (!Direction.Any() || bSaving) return;
    Hover = INDEX_NONE;
    if (Dialog != EDialog::None) { NavigateDialog(Direction); return; }
    bool Moved = false;
    switch (Region)
    {
    case ERegion::Content:
    {
        if (SeenPage == 4 && Direction.x && Entries.IsValidIndex(ContentSelection))
        {
            Controller->MenuAdjustSetting(Entries[ContentSelection].Id, Direction.x);
            Refresh();
            return;
        }
        if (SeenPage == 0 && Controller->ActiveStorageChest().IsSet()
            && Entries.IsValidIndex(ContentSelection))
        {
            const int32 CurrentContainer = Entries[ContentSelection].ContainerId;
            TArray<int32> CurrentGrid;
            TArray<int32> OtherGrid;
            for (int32 Index = 0; Index < Entries.Num(); ++Index)
            {
                if (Entries[Index].ContainerId == CurrentContainer) CurrentGrid.Add(Index);
                else OtherGrid.Add(Index);
            }
            const int32 Local = CurrentGrid.IndexOfByKey(ContentSelection);
            if (Direction.x && !OtherGrid.IsEmpty())
            {
                const bool MoveToPack = Direction.x > 0 && CurrentContainer > 0;
                const bool MoveToChest = Direction.x < 0 && CurrentContainer == 0;
                if (MoveToPack || MoveToChest)
                {
                    Select(OtherGrid[FMath::Clamp(Local, 0, OtherGrid.Num() - 1)]);
                    Moved = true;
                    break;
                }
            }
            if (Direction.y && Local >= 0)
            {
                const auto LocalMove = HomesteadMenuNavigation::Move(
                    Local, CurrentGrid.Num(), 4, Direction, Local % 4);
                if (!LocalMove.boundary && LocalMove.index >= 0)
                {
                    Select(CurrentGrid[LocalMove.index], true);
                    Moved = true;
                    break;
                }
            }
        }
        int32 Next = ContentSelection;
        if (MoveWithin(Next, Entries.Num(), Columns(), Direction, DesiredColumn))
        { Select(Next, Direction.y != 0); Moved = true; }
        break;
    }
    case ERegion::Tabs:
        if (Direction.x) { FocusedTab = ShiftFieldBookPage(FocusedTab, Direction.x); Moved = true; }
        break;
    case ERegion::Session: Moved = MoveWithin(SessionSelection, 2, 2, Direction); break;
    case ERegion::Inventory: Moved = MoveWithin(InventorySelection, 3, 3, Direction); break;
    case ERegion::Equipment: Moved = MoveWithin(EquipmentSelection, 3, 3, Direction); break;
    case ERegion::Portrait:
        if (PortraitSelection >= 0) Moved = MoveWithin(PortraitSelection, 3, 3, Direction);
        break;
    case ERegion::Recovery: Moved = MoveWithin(RecoverySelection, 2, 1, Direction); break;
    case ERegion::Actions:
        Moved = MoveWithin(ActionSelection, Actions.Num(), 2, Direction);
        if (!Moved && Direction.y < 0 && ActionSelection < 2)
        {
            Region = ERegion::Details;
            DetailsSelection = 1;
            if (DetailsScroll) DetailsScroll->ScrollToStart();
            Moved = true;
        }
        break;
    case ERegion::Details:
        if (SeenPage == 1 && Direction.y && !RequirementHints.IsEmpty())
        {
            const int32 Next = DetailsSelection + Direction.y;
            if (Next >= 1 && Next < RequirementHints.Num() + 2)
            {
                DetailsSelection = Next;
                for (const auto& Target : FocusTargets)
                    if (Target.region == ERegion::Details && Target.index == Next)
                        if (const auto Widget = Target.widget.Pin())
                            if (DetailsScroll)
                                DetailsScroll->ScrollDescendantIntoView(Widget, false);
                Moved = true;
                break;
            }
        }
        if (Direction.y && DetailsScroll)
        {
            const float Before = DetailsScroll->GetScrollOffset();
            const float End = DetailsScroll->GetScrollOffsetOfEnd();
            if (Direction.y > 0 && !ActionButtons.IsEmpty())
            {
                const auto View = DetailsScroll->GetCachedGeometry();
                const auto First = ActionButtons[0]->GetCachedGeometry();
                const bool ActionVisible = First.GetAbsolutePosition().Y < View.GetAbsolutePosition().Y + View.GetAbsoluteSize().Y
                    && First.GetAbsolutePosition().Y + First.GetAbsoluteSize().Y > View.GetAbsolutePosition().Y;
                if (ActionVisible || Before >= End - 1)
                { Region = ERegion::Actions; ActionSelection = 0; Moved = true; }
            }
            if (!Moved)
            {
                const float Next = FMath::Clamp(Before + Direction.y * 48, 0.0f, End);
                if (!FMath::IsNearlyEqual(Before, Next))
                { DetailsScroll->SetScrollOffset(Next); return; }
            }
        }
        break;
    }
    if (Moved)
    {
        bFocusPending = true;
        SynchronizeFocus();
    }
    else NavigateSpatial(Direction);
}

void SHomesteadMenu::NavigateDialog(HomesteadMenuNavigation::Direction Direction)
{
    if (Dialog == EDialog::Amount && bEditingAmount)
    {
        if (Direction.x) Amount = FMath::Clamp(Amount + Direction.x, 1, MaximumAmount);
        else { bEditingAmount = false; DialogSelection = 0; }
        BuildDialog();
        return;
    }
    const int32 Delta = Direction.y ? Direction.y : Direction.x;
    if (!Delta) return;
    if (Dialog == EDialog::Amount && DialogSelection == 0 && Direction.y < 0) DialogSelection = -1;
    else if (DialogSelection < 0) DialogSelection = 0;
    else DialogSelection = FMath::Clamp(DialogSelection + Delta, 0, DialogCount() - 1);
    if (DialogScroll && DialogButtons.IsValidIndex(DialogSelection))
        DialogScroll->ScrollDescendantIntoView(DialogButtons[DialogSelection], false);
    bFocusPending = true;
    SynchronizeFocus();
}

FNavigationReply SHomesteadMenu::OnNavigation(const FGeometry&, const FNavigationEvent&)
{
    return FNavigationReply::Stop();
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
        if (Key == EKeys::Gamepad_LeftX || Key == EKeys::Gamepad_LeftY)
        {
            const double Now = FPlatformTime::Seconds();
            if (Key == EKeys::Gamepad_LeftX) LeftStick.Sample(true, InputAmount, Now);
            else LeftStick.Sample(false, -InputAmount, Now);
            const auto Direction = LeftStick.Poll(Now);
            if (Direction.Any() && Controller->UsesGamepad() && !bSaving) NavigateDirection(Direction);
        }
        return true;
    }
    if (Event == IE_Repeat && (Key == EKeys::Left || Key == EKeys::Right || Key == EKeys::Up || Key == EKeys::Down
        || Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_DPad_Down))
        Event = IE_Pressed;
    const bool ActivateKey = Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::E
        || Key == EKeys::Gamepad_FaceButton_Bottom || Key == EKeys::LeftMouseButton;
    if (Event == IE_Released && ActivateKey)
    {
        const bool MatchingInput = (CraftInput == ECraftInput::Pointer && Key == EKeys::LeftMouseButton)
            || (CraftInput == ECraftInput::Controller && Key == EKeys::Gamepad_FaceButton_Bottom)
            || (CraftInput == ECraftInput::Keyboard
                && (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::E));
        if (MatchingInput) StopCraftHold();
        return true;
    }
    if (Event != IE_Pressed || bSaving) return true;
    if (!Key.IsMouseButton()) Hover = INDEX_NONE;
    if (Region == ERegion::Portrait && Dialog == EDialog::None
        && (Key == EKeys::Z || Key == EKeys::Gamepad_RightThumbstick))
    { Controller->ZoomMenuPortrait(); return true; }
    if (bRecovery && Dialog == EDialog::None)
    {
        if (Key == EKeys::G || Key == EKeys::Gamepad_FaceButton_Top || Key == EKeys::Escape
            || Key == EKeys::Gamepad_Special_Right || Key == EKeys::Gamepad_FaceButton_Right) ChangePage(4);
        if (Key == EKeys::G || Key == EKeys::Gamepad_FaceButton_Top || Key == EKeys::Escape
            || Key == EKeys::Gamepad_Special_Right || Key == EKeys::Gamepad_FaceButton_Right) return true;
    }
    if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right || Key == EKeys::I || Key == EKeys::Gamepad_Special_Right) { Back(); return true; }
    if (ActivateKey)
    {
        if (SeenPage == 0 && bControl && Region == ERegion::Content
            && Entries.IsValidIndex(ContentSelection)
            && Entries[ContentSelection].Subject == EHomesteadMenuSubject::ItemGroup)
        {
            SplitSelectedHalf();
            return true;
        }
        const ECraftInput Input = Key == EKeys::LeftMouseButton ? ECraftInput::Pointer
            : Key == EKeys::Gamepad_FaceButton_Bottom ? ECraftInput::Controller : ECraftInput::Keyboard;
        if (SeenPage == 1 && Region == ERegion::Content && Entries.IsValidIndex(ContentSelection)
            && Entries[ContentSelection].Subject == EHomesteadMenuSubject::Recipe)
            StartCraftHold(Input);
        else if (SeenPage == 0 && Region == ERegion::Content
            && Entries.IsValidIndex(ContentSelection))
            BeginOrCommitVirtualItemDrag();
        else
            Activate();
        return true;
    }
    int32 Dx = Key == EKeys::Right || Key == EKeys::Gamepad_DPad_Right ? 1 : Key == EKeys::Left || Key == EKeys::Gamepad_DPad_Left ? -1 : 0;
    int32 Dy = Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Down ? 1 : Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up ? -1 : 0;
    if (Dialog != EDialog::None)
    {
        if (Dialog == EDialog::Amount)
        {
            const int32 Delta = Key == EKeys::Gamepad_RightShoulder ? 10 : Key == EKeys::Gamepad_LeftShoulder ? -10 : 0;
            if (Delta && bEditingAmount) { Amount = FMath::Clamp(Amount + Delta, 1, MaximumAmount); BuildDialog(); return true; }
        }
        if (Dx || Dy) NavigateDialog({Dx, Dy});
        if (Key == EKeys::Tab) NavigateDialog({0, bShift ? -1 : 1});
        return true;
    }
    if (Key == EKeys::Gamepad_LeftShoulder) { ChangePage(ShiftFieldBookPage(SeenPage, -1)); return true; }
    if (Key == EKeys::Gamepad_RightShoulder) { ChangePage(ShiftFieldBookPage(SeenPage, 1)); return true; }
    if (Key == EKeys::Tab)
    {
        if (bControl) ChangePage(ShiftFieldBookPage(SeenPage, bShift ? -1 : 1));
        else CycleRegion(bShift ? -1 : 1);
        return true;
    }
    if (Key == EKeys::Gamepad_LeftTrigger) { CycleRegion(-1); return true; }
    if (Key == EKeys::Gamepad_RightTrigger) { CycleRegion(1); return true; }
    if (SeenPage == 0 && Key == EKeys::S)
    {
        if (Controller->MenuSortPack()) Refresh();
        return true;
    }
    if (Key == EKeys::Gamepad_FaceButton_Left)
    {
        if (SeenPage == 0 && GetSelectedSubject())
            SplitSelectedHalf();
        return true;
    }
    if (Key == EKeys::G || Key == EKeys::Gamepad_FaceButton_Top)
    {
        if (SeenPage == 0 && Key == EKeys::Gamepad_FaceButton_Top)
        {
            if (Controller->MenuSortPack()) Refresh();
        }
        return true;
    }
    if (Dx || Dy) { LeftStick.Reset(); NavigateDirection({Dx, Dy}); }
    return true;
}
FReply SHomesteadMenu::OnKeyDown(const FGeometry&, const FKeyEvent& Event)
{
    return FReply::Handled();
}
FReply SHomesteadMenu::OnPreviewKeyDown(const FGeometry&, const FKeyEvent& Event)
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
    PointerItemDragMove(Event.GetScreenSpacePosition());
    return FReply::Handled();
}
void SHomesteadMenu::PointerItemDragMove(FVector2D Position)
{
    if (bPointerItemDown && Entries.IsValidIndex(PointerDragSource))
    {
        if (!bPointerDraggingItem
            && FVector2D::Distance(Position, PointerDragStart) >= 7.0f)
            bPointerDraggingItem = true;
        if (bPointerDraggingItem)
        {
            PointerDragTarget = INDEX_NONE;
            for (int32 Index = 0; Index < Cells.Num(); ++Index)
                if (Index != PointerDragSource && Cells[Index]
                    && Cells[Index]->GetCachedGeometry().IsUnderLocation(Position))
                { PointerDragTarget = Index; break; }
            if (Scroll)
            {
                const auto Bounds = Scroll->GetCachedGeometry();
                const float Top = Bounds.GetAbsolutePosition().Y;
                const float Bottom = Top + Bounds.GetAbsoluteSize().Y;
                float Delta = 0.0f;
                if (Position.Y < Top + 36.0f) Delta = -18.0f;
                else if (Position.Y > Bottom - 36.0f) Delta = 18.0f;
                if (Delta != 0.0f)
                    Scroll->SetScrollOffset(FMath::Clamp(Scroll->GetScrollOffset() + Delta,
                        0.0f, Scroll->GetScrollOffsetOfEnd()));
            }
        }
    }
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
    if (bVirtualDraggingItem) { CancelVirtualItemDrag(); return; }
    CancelPointerItemDrag();
    StopCraftHold();
    if (Dialog == EDialog::Amount && bEditingAmount) { bEditingAmount = false; BuildDialog(); return; }
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
    if (Controller.IsValid() && !bSaving) { DialogError.Reset(); SetDialog(EDialog::Exit); }
}
void SHomesteadMenu::ShowSaveFailure(const FString& Error)
{
    bSaving = false;
    DialogError = Error;
    if (Dialog == EDialog::Exit) { BuildDialog(); bFocusPending = true; }
    else SetDialog(EDialog::SaveFailed);
}
void SHomesteadMenu::ShowGraphicsSaveFailure(const FString& Error)
{
    bSaving = false; DialogError = Error; SetDialog(EDialog::GraphicsFailed);
}
void SHomesteadMenu::SetDialog(EDialog Value)
{
    CancelPointerItemDrag();
    CancelVirtualItemDrag();
    StopCraftHold();
    Dialog = Value; DialogSelection = 0;
    bEditingAmount = false;
    LeftStick.Reset();
    PendingDirection = {};
    if (Root) Root->SetEnabled(Value == EDialog::None);
    BuildDialog();
    bFocusPending = true;
}
int32 SHomesteadMenu::DialogCount() const
{
    if (Dialog == EDialog::Amount) return 4;
    if (Dialog == EDialog::Merge) return MergeTargets.Num() + 1;
    if (Dialog == EDialog::Exit) return 2;
    return Dialog == EDialog::SaveFailed || Dialog == EDialog::GraphicsFailed ? 3 : 2;
}
void SHomesteadMenu::BuildDialog()
{
    if (Dialog == EDialog::None) { ModalHost->SetVisibility(EVisibility::Collapsed); AmountControl.Reset(); return; }
    FString Title, Description;
    TArray<FString> Labels;
    if (Dialog == EDialog::Amount)
    {
        Title = PendingAction == EHomesteadItemAction::Split ? TEXT("Split stack")
            : PendingAction == EHomesteadItemAction::Drop ? TEXT("Drop items") : TEXT("Transfer items");
        const FString Destination = PendingAction == EHomesteadItemAction::Split ? TEXT("A new stack in the same container")
            : PendingAction == EHomesteadItemAction::Drop ? TEXT("Nearby ground")
            : PendingRow.ContainerId > 0 ? TEXT("Your pack") : FString::Printf(TEXT("Chest %d"), PendingRow.DestinationId);
        Description = FString::Printf(TEXT("%s\nFrom: %s\nTo: %s\n\nAmount: %d / %d\nChoose Amount and activate to edit. While editing: Left/Right one, LB/RB ten; Back finishes editing."),
            *PendingRow.Name, *PendingRow.Location, *Destination, Amount, MaximumAmount);
        Labels = {TEXT("Cancel"), TEXT("Confirm"), TEXT("One"), TEXT("All available")};
    }
    else if (Dialog == EDialog::DropWearable)
    {
        Title = TEXT("Drop garment");
        Description = FString::Printf(TEXT("%s\n\nPlace this garment on clear ground nearby?"),
            *PendingRow.Name);
        Labels = {TEXT("Cancel"), TEXT("Drop")};
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
        Title = TEXT("Quit game");
        Description = (DialogError.IsEmpty() ? FString() : DialogError + TEXT("\n\n"))
            + Controller->MenuSaveStatus();
        Labels = {TEXT("Save & Quit"), TEXT("Quit without Saving")};
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
        Labels = {TEXT("Stay in Settings"), TEXT("Start a new test woodland")};
    }
    else
    {
        Title = TEXT("Start a new woodland?");
        Description = TEXT("This replaces the current test session with a new woodland seed. Unsaved progress will be lost.");
        Labels = {TEXT("Cancel"), TEXT("Start a new woodland")};
    }
    TSharedPtr<SVerticalBox> Choices;
    DialogButtons.Reset();
    AmountControl.Reset();
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
    if (Dialog == EDialog::Amount)
    {
        auto Editor = MakeButton(FString::Printf(TEXT("Amount: %d%s"), Amount,
            bEditingAmount ? TEXT("  (editing)") : TEXT("  - activate to edit")),
            [this]() { DialogSelection = -1; bEditingAmount = true; BuildDialog(); },
            TAttribute<FSlateColor>::CreateLambda([this]() { return DialogSelection < 0 ? MenuGold : Selected; }));
        Editor->SetOnFocusReceived(FSimpleDelegate::CreateLambda([this]()
            { if (!bSynchronizingFocus) DialogSelection = -1; }));
        AmountControl = Editor;
        Choices->AddSlot().AutoHeight().Padding(0, 6)[Editor];
    }
    for (int32 Index = 0; Index < Labels.Num(); ++Index)
    {
        auto Button = MakeButton(Labels[Index], [this, Index]() { DialogAction(Index); },
            TAttribute<FSlateColor>::CreateLambda([this, Index]() { return DialogSelection == Index ? MenuGold : Selected; }));
        Button->SetOnFocusReceived(FSimpleDelegate::CreateLambda([this, Index]()
            { if (!bSynchronizingFocus) { DialogSelection = Index; bEditingAmount = false; } }));
        DialogButtons.Add(Button);
        Choices->AddSlot().AutoHeight().Padding(0, 6)[ Button ];
    }
    bFocusPending = true;
}
void SHomesteadMenu::DialogAction(int32 Index)
{
    if (!Controller.IsValid() || bSaving) return;
    if (Dialog == EDialog::Exit)
    {
        if (Index == 0) { bSaving = true; Controller->MenuSaveAndQuit(); }
        else if (Index == 1) Controller->MenuQuitWithoutSaving();
        return;
    }
    if (Index == 0)
    {
        SetDialog(EDialog::None);
        return;
    }
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
    if (Dialog == EDialog::DropWearable)
    {
        Controller->MenuItemAction(PendingRow, PendingAction, 1, PendingRevision);
        SetDialog(EDialog::None); Refresh(); return;
    }
    if (Dialog == EDialog::SaveFailed && Index == 2) { Controller->MenuQuitWithoutSaving(); return; }
    if (Dialog == EDialog::GraphicsFailed && Index == 2) { Controller->MenuQuitWithoutSaving(); return; }
    if (Dialog == EDialog::Unsaved) Controller->MenuQuitWithoutSaving();
    else if (Dialog == EDialog::Restart || Dialog == EDialog::TestReset) { SetDialog(EDialog::None); Controller->MenuRestart(); }
    else if (Dialog == EDialog::SaveFailed || Dialog == EDialog::GraphicsFailed)
    {
        bSaving = true;
        Controller->MenuSaveAndQuit();
    }
}
}
