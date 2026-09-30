#include "SHomesteadMenuPrivate.h"
#include "../Simulation/HomesteadHotbarLayout.h"

namespace HomesteadMenus
{
TSharedRef<SButton> SHomesteadMenu::MakeButton(const FString& Label, TFunction<void()> Action,
    TAttribute<FSlateColor> Color, const FString& AccessibleLabel, FMargin Padding)
{
    return SNew(SMenuButton).ButtonStyle(&MenuButtonStyle()).IsFocusable(true).ContentPadding(Padding)
        .ButtonColorAndOpacity(Color).ToolTipText(FText::FromString(AccessibleLabel.IsEmpty() ? Label : AccessibleLabel))
        .OnClicked_Lambda([this, Action]() { if (PointerAction()) Action(); return FReply::Handled(); })
        [
            SNew(STextBlock).Text(FText::FromString(Label)).AutoWrapText(true)
            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 17))
            .ColorAndOpacity_Lambda([Color]() { return Color.Get().GetSpecifiedColor() == MenuGold ? FSlateColor(PineInk) : FSlateColor(Ink); })
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
    case ERegion::Hotbar: HotbarSelection = Index; break;
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
    case ERegion::Hotbar: Index = HotbarSelection; break;
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
        TEXT("Content"), TEXT("Equipment"), TEXT("Details"), TEXT("Actions"), TEXT("Recovery"), TEXT("Hotbar")};
    return Names[static_cast<int32>(Region)];
}

void SHomesteadMenu::ScrollActionIntoView()
{
    if (Region == ERegion::Actions && DetailsScroll && ActionButtons.IsValidIndex(ActionSelection))
        DetailsScroll->ScrollDescendantIntoView(ActionButtons[ActionSelection], false, EDescendantScrollDestination::IntoView);
}

int32 SHomesteadMenu::SettingsTabOf(int32 SettingId)
{
    switch (SettingId)
    {
    case 0: case 1: case 9: return -1;
    case 5: case 6: case 7: case 16: return 1;
    case 10: case 11: return 2;
    default: return 0;
    }
}

void SHomesteadMenu::SetSettingsTab(int32 Tab)
{
    Tab = FMath::Clamp(Tab, 0, 2);
    if (Tab == SettingsTab || Dialog != EDialog::None) return;
    SettingsTab = Tab;
    if (Scroll) Scroll->ScrollToStart();
    Refresh();
}

int32 SHomesteadMenu::SettingsTopCount() const
{
    int32 Count = 0;
    while (SeenPage == 4 && Entries.IsValidIndex(Count) && SettingsTabOf(Entries[Count].Id) < 0) ++Count;
    return Count;
}

bool SHomesteadMenu::FocusLegacySubject(int32 Id)
{
    if (SeenPage == 4 && SettingsTabOf(Id) >= 0 && SettingsTabOf(Id) != SettingsTab)
    {
        SettingsTab = SettingsTabOf(Id);
        Refresh();
    }
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
    if (SeenPage == 0)
    {
        // Pack and chest actions live in the item menu: open it on the selected tile with that action chosen.
        if (!Entries.IsValidIndex(ContentSelection)) return false;
        if (Dialog != EDialog::None) SetDialog(EDialog::None);
        if (!BuildItemOptions(Entries[ContentSelection])) return false;
        const int32 Option = PopupOptions.IndexOfByPredicate([Action](const FPopupOption& Candidate)
            { return Candidate.Action.IsSet() && Candidate.Action.GetValue() == Action; });
        if (Option < 0) { PopupOptions.Reset(); return false; }
        Region = ERegion::Content;
        PopupAnchor = PopupAnchorFor(Cells.IsValidIndex(ContentSelection) ? Cells[ContentSelection] : nullptr, false);
        SetDialog(EDialog::Context);
        DialogSelection = Option;
        bFocusPending = true;
        SynchronizeFocus();
        return true;
    }
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
    if (Controller->MenuPortraitBrush() && SeenPage == 0) Regions.Add(ERegion::Portrait);
    Regions.Add(ERegion::Content);
    if (SeenPage == 0 && !HotbarCells.IsEmpty()) Regions.Add(ERegion::Hotbar);
    if (SeenPage == 0) Regions.Add(ERegion::Equipment);
    if (SeenPage != 0 && SeenPage != 6 && SeenPage != 7) Regions.Add(ERegion::Details);
    if (!Actions.IsEmpty() && SeenPage != 0 && SeenPage != 6) Regions.Add(ERegion::Actions);
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
        if (SeenPage == 6 && Direction.x && Entries.IsValidIndex(ContentSelection))
        {
            Controller->MenuStepAppearance(Entries[ContentSelection].Id, Direction.x);
            Refresh();
            return;
        }
        if (SeenPage == 4 && Entries.IsValidIndex(ContentSelection))
        {
            // Save / Load / Quit form one row above the Game / Sound / Video tabs; the tab's
            // settings follow as a single column.
            const int32 Top = SettingsTopCount();
            if (ContentSelection < Top)
            {
                if (Direction.x)
                {
                    const int32 Next = FMath::Clamp(ContentSelection + Direction.x, 0, Top - 1);
                    if (Next != ContentSelection) { Select(Next); Moved = true; }
                }
                else
                {
                    Region = ERegion::Session;
                    SessionSelection = Direction.y < 0 ? 0 : SettingsTab + 1;
                    Moved = true;
                }
                break;
            }
            if (Direction.x)
            {
                Controller->MenuAdjustSetting(Entries[ContentSelection].Id, Direction.x);
                Refresh();
                return;
            }
            if (Direction.y < 0 && ContentSelection == Top)
            {
                Region = ERegion::Session;
                SessionSelection = SettingsTab + 1;
                Moved = true;
                break;
            }
            if (Entries.IsValidIndex(ContentSelection + Direction.y))
            {
                Select(ContentSelection + Direction.y);
                Moved = true;
            }
            break;
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
                    Local, CurrentGrid.Num(), StorageColumns(), Direction, Local % StorageColumns());
                if (!LocalMove.boundary && LocalMove.index >= 0)
                {
                    Select(CurrentGrid[LocalMove.index], true);
                    Moved = true;
                    break;
                }
                // Below the last row of either grid: the hotbar strip that runs under both.
                if (Direction.y > 0 && !HotbarCells.IsEmpty())
                {
                    Region = ERegion::Hotbar;
                    Moved = true;
                    break;
                }
            }
        }
        int32 Next = ContentSelection;
        if (MoveWithin(Next, Entries.Num(), Columns(), Direction, DesiredColumn))
        { Select(Next, Direction.y != 0); Moved = true; }
        else if (SeenPage == 0 && Direction.y < 0 && !HotbarCells.IsEmpty() && !Controller->ActiveStorageChest().IsSet())
        {
            // Above the pack grid's first row: the hotbar, her pack's first row (her selection in
            // the grid is kept).
            Region = ERegion::Hotbar;
            Moved = true;
        }
        else if (SeenPage == 0 && Direction.x < 0 && Controller->MenuPortraitBrush() && !Controller->ActiveStorageChest().IsSet())
        {
            // The pack's left edge: her portrait beside it, whatever row she is on (spatial
            // navigation can miss the tall image from rows it doesn't overlap).
            Region = ERegion::Portrait;
            PortraitSelection = -1;
            Moved = true;
        }
        break;
    }
    case ERegion::Tabs:
        if (Direction.x) { FocusedTab = ShiftFieldBookPage(FocusedTab, Direction.x); Moved = true; }
        break;
    case ERegion::Session:
    {
        const int32 Top = SettingsTopCount();
        if (SeenPage != 4) { Moved = MoveWithin(SessionSelection, 2, 2, Direction); break; }
        if (SessionSelection == 0)
        {
            if (Direction.y > 0 && Top > 0) { Region = ERegion::Content; Select(0); Moved = true; }
            break;
        }
        if (Direction.x)
        {
            const int32 Tab = FMath::Clamp(SessionSelection - 1 + Direction.x, 0, 2);
            if (Tab != SessionSelection - 1) { SessionSelection = Tab + 1; SetSettingsTab(Tab); Moved = true; }
        }
        else if (Direction.y < 0)
        {
            Region = Top > 0 ? ERegion::Content : ERegion::Session;
            if (Top > 0) Select(FMath::Min(SettingsTab, Top - 1)); else SessionSelection = 0;
            Moved = true;
        }
        else if (Direction.y > 0 && Entries.Num() > Top) { Region = ERegion::Content; Select(Top); Moved = true; }
        break;
    }
    case ERegion::Inventory: Moved = MoveWithin(InventorySelection, 3, 3, Direction); break;
    case ERegion::Equipment: Moved = MoveWithin(EquipmentSelection, VisibleEquipmentSlotCount, VisibleEquipmentSlotCount, Direction); break;
    case ERegion::Hotbar:
        if (Direction.x)
        {
            Moved = MoveWithin(HotbarSelection, Homestead::HotbarSize, Homestead::HotbarSize, Direction);
            // The strip's ends are its edges: don't let spatial navigation carry focus (and whatever
            // she is holding) off into the grid above.
            if (!Moved) return;
        }
        // Back to the grid tile she left (pack or chest), not wherever is nearest: up from the row
        // under both grids with a chest open, down from the row heading the pack page.
        else if (Direction.y == (Controller->ActiveStorageChest().IsSet() ? -1 : 1) && !Entries.IsEmpty())
        { Region = ERegion::Content; Select(ContentSelection, true); Moved = true; }
        break;
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
}
