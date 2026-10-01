#include "SHomesteadMenuPrivate.h"
#include "HomesteadUITheme.h"

namespace HomesteadMenus
{
void SHomesteadMenu::ChangeInventoryView(int32 View)
{
    if (Dialog != EDialog::None || !Controller.IsValid()) return;
    InventorySelection = FMath::Clamp(View, 0, 2);
    Controller->MenuInventoryView(InventorySelection);
    ContentSelection = 0; Hover = INDEX_NONE;
    Region = ERegion::Content;
    Refresh();
}

void SHomesteadMenu::FocusEquipment(int32 Index, bool bPointer)
{
    if (!Controller.IsValid() || Dialog != EDialog::None || Index < 0 || Index >= VisibleEquipmentSlotCount) return;
    const auto Slot = VisibleEquipmentSlots[Index];
    const int32 Id = Controller->State().equipment[static_cast<int32>(Slot)];
    EquipmentSelection = Index;
    TSharedPtr<SWidget> Anchor;
    for (const auto& Target : FocusTargets)
        if (Target.region == ERegion::Equipment && Target.index == Index) Anchor = Target.widget.Pin();
    FHomesteadRow Worn;
    if (Id && Controller->MenuWearableRow(Id, Worn))
    {
        OpenItemContextMenuFor(Worn, PopupAnchorFor(Anchor, bPointer));
        return;
    }
    // An empty slot offers the carried garments that fit it.
    PopupOptions.Reset();
    PopupTitle = FString(EquipmentSlotNames[Index]) + TEXT(": empty");
    for (const auto& Instance : Controller->State().wearables)
    {
        const auto* Info = Homestead::GetWearableDefinition(Instance.definition);
        FHomesteadRow Row;
        if (Instance.owner != Homestead::WearableOwner::Carried || !Info
            || !(Info->slots & (1u << static_cast<int>(Slot))) || !Controller->MenuWearableRow(Instance.id, Row))
            continue;
        PopupOptions.Add({[Label = TEXT("Wear ") + Row.Name]() { return Label; },
            [this, Row]() { Controller->MenuItemAction(Row, EHomesteadItemAction::Equip, 1, Controller->Simulation().GetRevision()); },
            nullptr, EHomesteadItemAction::Equip});
    }
    if (PopupOptions.IsEmpty())
        PopupOptions.Add({[]() { return FString(TEXT("Nothing carried fits here")); }, nullptr, []() { return false; }, {}});
    PopupOptions.Add({[]() { return FString(TEXT("Cancel")); }, nullptr, nullptr, {}});
    PopupAnchor = PopupAnchorFor(Anchor, bPointer);
    SetDialog(EDialog::Context);
}

FLinearColor SHomesteadMenu::CellColor(int32 Index) const
{
    if (bVirtualDraggingItem && Index == VirtualDragSource)
        return HomesteadUITheme::Themed(FLinearColor(0.045f, 0.055f, 0.05f, 0.72f));
    if (bVirtualDraggingItem && Index == ContentSelection)
        return MenuGold;
    if (bPointerDraggingItem && Index == PointerDragSource)
        return HomesteadUITheme::Themed(FLinearColor(0.045f, 0.055f, 0.05f, 0.72f));
    if (bPointerDraggingItem && Index == PointerDragTarget)
        return MenuGold;
    return Index == ContentSelection ? Selected
        : Index == Hover ? Selected : HomesteadUITheme::Themed(FLinearColor(0.055f, 0.09f, 0.075f, 0.5f));
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
        {
            // A tile built this frame has no layout yet; scrolling to it now would leave the scroll offset
            // NaN. Its scroll waits for the first layout (Tick).
            if (Cells[ContentSelection]->GetCachedGeometry().GetLocalSize().X > 0)
                Scroll->ScrollDescendantIntoView(Cells[ContentSelection], false, EDescendantScrollDestination::IntoView);
            else bScrollSelectionPending = true;
        }
        if (DetailsHost) DetailsHost->SetContent(BuildDetails());
        else ComputeActions();
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

void SHomesteadMenu::QuickMove(int32 Index)
{
    if (!Controller.IsValid() || Dialog != EDialog::None || !Entries.IsValidIndex(Index)) return;
    const FHomesteadRow Row = Entries[Index];
    bool Changed = false;
    if (Controller->ActiveStorageChest().IsSet() || Row.ContainerId != 0)
        Changed = Controller->MenuMoveWhole(Row);
    else if (Row.Subject == EHomesteadMenuSubject::Wearable)
        Changed = Controller->MenuItemAction(Row, EHomesteadItemAction::Equip, 1, Controller->Simulation().GetRevision());
    // Otherwise a pack stack hops between the hotbar row and the rest of her pack.
    else Changed = Controller->MenuItemAction(Row, EHomesteadItemAction::Pin, 1, Controller->Simulation().GetRevision());
    if (Changed) Refresh();
}

FVector2D SHomesteadMenu::PopupAnchorFor(const TSharedPtr<SWidget>& Widget, bool bPointer) const
{
    if ((bPointer || !Widget) && FSlateApplication::IsInitialized()) return FSlateApplication::Get().GetCursorPos();
    if (!Widget) return FVector2D::ZeroVector;
    const FGeometry Geometry = Widget->GetCachedGeometry();
    return FVector2D(Geometry.GetAbsolutePosition()) + FVector2D(Geometry.GetAbsoluteSize()) * 0.6f;
}

void SHomesteadMenu::OpenDyeChooser(int32 Choice)
{
    if (!Controller.IsValid()) return;
    DyeChoice = FMath::Clamp(Choice, 0, MenuDyeStyle::Count - 1);
    PopupOptions.Reset();
    PopupTitle = FString::Printf(TEXT("Dye the %s"), *FString(UTF8_TO_TCHAR(Homestead::GetWearableDefinition(
        static_cast<Homestead::WearableDefinition>(DyeRow.Id))->name)).ToLower());
    for (int32 Dye = 0; Dye < MenuDyeStyle::Count; ++Dye)
        PopupOptions.Add({[this, Dye]()
            {
                return FString(UTF8_TO_TCHAR(Homestead::DyeName(Dye)))
                    + (Dye == DyeOriginal ? TEXT("  (as it is)") : Dye == DyeChoice ? TEXT("  (chosen)") : TEXT(""));
            },
            [this, Dye]() { OpenDyeChooser(Dye); }, nullptr, {}, MenuDyeStyle::Swatch(Dye)});
    // Nothing is spent and nothing changes until Apply.
    PopupOptions.Add({[this]() { return FString::Printf(TEXT("Apply %s"), UTF8_TO_TCHAR(Homestead::DyeName(DyeChoice))); },
        [this]()
        {
            bDyeChooser = false;
            DyePreviewed = INDEX_NONE;
            if (!Controller->MenuItemAction(DyeRow, EHomesteadItemAction::Dye, DyeChoice + 1, DyeRevision))
                Controller->MenuEndDyePreview();
        },
        [this]() { return DyeChoice != DyeOriginal; }});
    PopupOptions.Add({[]() { return FString(TEXT("Cancel")); }, nullptr, nullptr});
    bDyeChooser = true;
    SetDialog(EDialog::Context);
    // After choosing a dye the next press applies it; otherwise start on her current dye.
    DialogSelection = DyeChoice != DyeOriginal ? MenuDyeStyle::Count : DyeChoice;
    bFocusPending = true;
}

void SHomesteadMenu::EndDyePreview()
{
    bDyeChooser = false;
    if (DyePreviewed == INDEX_NONE) return;
    DyePreviewed = INDEX_NONE;
    if (Controller.IsValid()) Controller->MenuEndDyePreview();
}

bool SHomesteadMenu::BuildItemOptions(const FHomesteadRow& Row)
{
    if (!Controller.IsValid()) return false;
    if (Row.Subject != EHomesteadMenuSubject::ItemGroup && Row.Subject != EHomesteadMenuSubject::Wearable) return false;
    PopupOptions.Reset();
    bDyeChooser = false;
    const auto Add = [this](const FString& Label, TFunction<void()> Run, TOptional<EHomesteadItemAction> Action = {})
    {
        PopupOptions.Add({[Label]() { return Label; }, MoveTemp(Run), nullptr, Action});
    };
    const auto Act = [this, Row](EHomesteadItemAction Action, int32 Count) -> TFunction<void()>
    {
        return [this, Row, Action, Count]()
            { Controller->MenuItemAction(Row, Action, Count, Controller->Simulation().GetRevision()); };
    };
    const TFunction<void()> Move = [this, Row]() { Controller->MenuMoveWhole(Row); };
    // The hotbar is her pack's first row (Simulation/HomesteadPackRow.h): a stack moves up into it
    // or back down into the rest of the pack.
    const auto HotbarOptions = [&]()
    {
        if (Row.HotbarCell >= 0)
        {
            Add(TEXT("Move into the pack"), Act(EHomesteadItemAction::Pin, 1), EHomesteadItemAction::Pin);
            Add(TEXT("Move to another hotbar slot..."), [this, Cell = Row.HotbarCell]()
            {
                CancelHotbarHolds();
                HeldHotbarSlot = Cell;
                Region = ERegion::Hotbar;
                HotbarSelection = Cell;
                bFocusPending = true;
            });
        }
        else
        {
            Add(TEXT("Move to the hotbar"), Act(EHomesteadItemAction::Pin, 1), EHomesteadItemAction::Pin);
            Add(TEXT("Move to a hotbar slot..."), [this, Row]() { BeginPlacingOnHotbar(Row); });
        }
    };
    const bool Storage = Controller->ActiveStorageChest().IsSet();
    PopupTitle = Row.Name + (Row.Quantity > 1 ? FString::Printf(TEXT("  x%d"), Row.Quantity) : FString());
    if (Row.Subject == EHomesteadMenuSubject::ItemGroup)
    {
        const bool Known = Row.Id >= 0 && Row.Id < static_cast<int32>(Homestead::Item::Count);
        const auto Item = static_cast<Homestead::Item>(Row.Id);
        if (Row.ContainerId == 0)
        {
            if (Known && Homestead::IsEdible(Item))
                Add(TEXT("Eat 1"), Act(EHomesteadItemAction::Primary, 1), EHomesteadItemAction::Primary);
            if (Known && (Item == Homestead::Item::OilFlask || Item == Homestead::Item::OilLamp))
                Add(TEXT("Fill the lamp"), [this]() { Controller->MenuRefillLamp(); });
            HotbarOptions();
            if (Storage) Add(FString::Printf(TEXT("Move to chest %d"), Row.DestinationId), Move, EHomesteadItemAction::Transfer);
            Add(Row.Quantity > 1 ? TEXT("Drop 1") : TEXT("Drop"), Act(EHomesteadItemAction::Drop, 1), EHomesteadItemAction::Drop);
            if (Row.Quantity > 1)
            {
                Add(FString::Printf(TEXT("Drop all %d"), Row.Quantity), Act(EHomesteadItemAction::Drop, Row.Quantity));
                Add(TEXT("Drop or split some..."), [this, Row]() { OpenQuantityPrompt(Row); }, EHomesteadItemAction::Split);
            }
        }
        else if (Row.ContainerId > 0)
        {
            Add(TEXT("Take to pack"), Move, EHomesteadItemAction::Transfer);
            Add(TEXT("Take to a hotbar slot..."), [this, Row]() { BeginPlacingOnHotbar(Row); });
            if (Row.Quantity > 1)
                Add(TEXT("Take or split some..."), [this, Row]() { OpenQuantityPrompt(Row); }, EHomesteadItemAction::Split);
        }
    }
    else
    {
        if (Row.ContainerId < 0) Add(TEXT("Unequip to pack"), Act(EHomesteadItemAction::Unequip, 1), EHomesteadItemAction::Unequip);
        else if (Row.ContainerId == 0)
        {
            Add(TEXT("Equip"), Act(EHomesteadItemAction::Equip, 1), EHomesteadItemAction::Equip);
            HotbarOptions();
            if (Storage) Add(FString::Printf(TEXT("Move to chest %d"), Row.DestinationId), Move, EHomesteadItemAction::Transfer);
            // A garment is one owned thing, so dropping it asks first.
            Add(TEXT("Drop..."), [this, Row]()
            {
                PendingRow = Row; PendingAction = EHomesteadItemAction::Drop;
                PendingRevision = Controller->Simulation().GetRevision();
                SetDialog(EDialog::DropWearable);
            }, EHomesteadItemAction::Drop);
        }
        else Add(TEXT("Take to pack"), Move, EHomesteadItemAction::Transfer);
        const auto* Info = Homestead::GetWearableDefinition(static_cast<Homestead::WearableDefinition>(Row.Id));
        if (Info && Info->dyeable)
            Add(TEXT("Change dye..."), [this, Row]()
            {
                const auto* Owned = Controller->Simulation().GetWearable(Row.SubjectId);
                if (!Owned) return;
                DyeRow = Row;
                DyeRevision = Controller->Simulation().GetRevision();
                DyeOriginal = Owned->dye;
                OpenDyeChooser(Owned->dye);
            }, EHomesteadItemAction::Dye);
    }
    if (SeenPage == 0 && Row.ContainerId >= 0)
        Add(TEXT("Sort pack"), [this]() { Controller->MenuSortPack(); });
    if (SeenPage == 0 && Controller->ActiveStorageChest().IsSet())
    {
        Add(TEXT("Store matching stacks (T)"), [this]() { Controller->MenuStoreMatching(); });
        Add(TEXT("Name this chest..."), [this]() { OpenRenameChest(); });
    }
    Add(TEXT("Cancel"), nullptr);
    return true;
}

void SHomesteadMenu::OpenItemContextMenuFor(const FHomesteadRow& Row, FVector2D Anchor)
{
    if (!Controller.IsValid() || Dialog != EDialog::None || SeenPage != 0) return;
    CancelPointerItemDrag();
    if (!BuildItemOptions(Row)) return;
    PopupAnchor = Anchor;
    SetDialog(EDialog::Context);
}

void SHomesteadMenu::OpenItemContextMenu(int32 Index, bool bPointer)
{
    if (!Controller.IsValid() || Dialog != EDialog::None || SeenPage != 0 || !Entries.IsValidIndex(Index)) return;
    CancelPointerItemDrag();
    Region = ERegion::Content;
    Select(Index);
    OpenItemContextMenuFor(Entries[Index], PopupAnchorFor(Cells.IsValidIndex(Index) ? Cells[Index] : nullptr, bPointer));
}

void SHomesteadMenu::OpenQuantityPrompt(const FHomesteadRow& Row)
{
    if (!Controller.IsValid() || Row.Subject != EHomesteadMenuSubject::ItemGroup || Row.ContainerId < 0) return;
    if (Row.Quantity < 2)
    {
        OpenItemContextMenu(Entries.IndexOfByPredicate([&Row](const FHomesteadRow& Entry)
            { return Entry.Subject == Row.Subject && Entry.SubjectId == Row.SubjectId; }));
        return;
    }
    CancelPointerItemDrag();
    PendingRow = Row;
    PendingRevision = Controller->Simulation().GetRevision();
    MaximumAmount = Row.Quantity;
    Amount = FMath::Max(1, Row.Quantity / 2);
    PopupTitle = Row.Name + FString::Printf(TEXT("  x%d"), Row.Quantity);
    PopupOptions.Reset();
    const auto Run = [this](EHomesteadItemAction Action)
    {
        return [this, Action]()
            { Controller->MenuItemAction(PendingRow, Action, Amount, Controller->Simulation().GetRevision()); };
    };
    const TFunction<bool()> CanSplit = [this]() { return Amount < MaximumAmount; };
    if (Row.ContainerId == 0)
    {
        PopupOptions.Add({[this]() { return FString::Printf(TEXT("Split off %d"), Amount); }, Run(EHomesteadItemAction::Split), CanSplit});
        PopupOptions.Add({[this]() { return FString::Printf(TEXT("Drop %d"), Amount); }, Run(EHomesteadItemAction::Drop), nullptr});
        if (Controller->ActiveStorageChest().IsSet())
            PopupOptions.Add({[this]() { return FString::Printf(TEXT("Move %d to chest"), Amount); }, Run(EHomesteadItemAction::Transfer), nullptr});
    }
    else
    {
        PopupOptions.Add({[this]() { return FString::Printf(TEXT("Take %d to pack"), Amount); }, Run(EHomesteadItemAction::Transfer), nullptr});
        PopupOptions.Add({[this]() { return FString::Printf(TEXT("Split off %d"), Amount); }, Run(EHomesteadItemAction::Split), CanSplit});
    }
    PopupOptions.Add({[]() { return FString(TEXT("Cancel")); }, nullptr, nullptr});
    if (Dialog == EDialog::None && !bKeepPopupAnchor) PopupAnchor = FSlateApplication::Get().GetCursorPos();
    SetDialog(EDialog::Quantity);
}

void SHomesteadMenu::AdjustQuantity(int32 Delta)
{
    if (Dialog != EDialog::Quantity) return;
    Amount = FMath::Clamp(Amount + Delta, 1, MaximumAmount);
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
    const int32 HotbarTarget = PointerHotbarTarget;
    int32 Target = INDEX_NONE;
    bool bOverSource = false;
    if (WasDragging && FSlateApplication::IsInitialized())
    {
        const FVector2D Position = FSlateApplication::Get().GetCursorPos();
        for (int32 Index = 0; Index < Cells.Num(); ++Index)
            if (Index != Source && Cells[Index]
                && Cells[Index]->GetCachedGeometry().IsUnderLocation(Position))
            { Target = Index; break; }
        bOverSource = Cells.IsValidIndex(Source) && Cells[Source] && Cells[Source]->GetCachedGeometry().IsUnderLocation(Position);
    }
    bPointerItemDown = false;
    bPointerDraggingItem = false;
    PointerDragSource = INDEX_NONE;
    PointerDragTarget = INDEX_NONE;
    PointerHotbarTarget = INDEX_NONE;
    // Only a release back over the tile she picked up clicks it; swallow that one click. Released
    // anywhere else, no click follows, so the next real click mustn't be eaten.
    bSuppressItemClick = WasDragging && bOverSource;
    if (WasDragging && Entries.IsValidIndex(Source) && Entries.IsValidIndex(Target)
        && Source != Target)
        Controller->MenuDrop(Entries[Source], Entries[Target], PointerDragRevision);
    // Dropped on a hotbar cell: the stack itself moves into her pack's first row (from a chest in
    // one step), merging or swapping with what is there. Anywhere else, nothing.
    else if (WasDragging && Entries.IsValidIndex(Source) && Target == INDEX_NONE && HotbarCells.IsValidIndex(HotbarTarget))
        Controller->MenuPlaceInHotbar(Entries[Source], HotbarTarget);
}

void SHomesteadMenu::CancelPointerItemDrag()
{
    bPointerItemDown = false;
    bPointerDraggingItem = false;
    PointerDragSource = INDEX_NONE;
    PointerDragTarget = INDEX_NONE;
    PointerHotbarTarget = INDEX_NONE;
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

void SHomesteadMenu::PointerItemDragMove(FVector2D Position)
{
    if (bHotbarPointerDown && HotbarCells.IsValidIndex(HeldHotbarSlot))
    {
        if (!bHotbarPointerDragging && FVector2D::Distance(Position, HotbarDragStart) >= 7.0f)
            bHotbarPointerDragging = true;
        if (bHotbarPointerDragging)
        {
            const int32 Over = HotbarCellAt(Position);
            PointerHotbarTarget = Over != HeldHotbarSlot ? Over : INDEX_NONE;
        }
        return;
    }
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
            // Off the grid, a pack stack can go onto one of the book's hotbar slots.
            PointerHotbarTarget = PointerDragTarget == INDEX_NONE && HotbarCandidateRow() ? HotbarCellAt(Position) : INDEX_NONE;
            if (Scroll && PointerHotbarTarget == INDEX_NONE)
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
}
