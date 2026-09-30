// The field book's hotbar row: her pack's first row (Simulation/HomesteadPackRow.h), ten keyed cells
// holding real stacks, shown whenever the inventory is open (heading the Pack page, or the pack
// column with a chest open). She drags a pack or chest stack into a cell, a cell onto another, or a
// cell back down into her pack or the chest: onto an empty place it moves, onto the same item it
// merges, onto anything else the two swap (HomesteadControllerHotbarEditor.cpp).
#include "SHomesteadMenuPrivate.h"
#include "../Simulation/HomesteadHotbarLayout.h"
#include "../Simulation/HomesteadPackRow.h"

namespace HomesteadMenus
{
namespace MenuHotbarStyle
{
// Slightly smaller than a pack tile so ten fit under the pack grid.
constexpr float SlotSize = 58;
}
const FHomesteadRow* SHomesteadMenu::HotbarCandidateRow() const
{
    const FHomesteadRow* Row = HeldHotbarRow.IsSet() ? &HeldHotbarRow.GetValue()
        : bPointerDraggingItem && Entries.IsValidIndex(PointerDragSource) ? &Entries[PointerDragSource]
        : bVirtualDraggingItem && Entries.IsValidIndex(VirtualDragSource) ? &Entries[VirtualDragSource] : nullptr;
    return Row && (Row->Subject == EHomesteadMenuSubject::ItemGroup || Row->Subject == EHomesteadMenuSubject::Wearable) ? Row : nullptr;
}
int32 SHomesteadMenu::HotbarCellAt(FVector2D Position) const
{
    for (int32 Slot = 0; Slot < HotbarCells.Num(); ++Slot)
        if (HotbarCells[Slot] && HotbarCells[Slot]->GetCachedGeometry().IsUnderLocation(Position)) return Slot;
    return INDEX_NONE;
}
bool SHomesteadMenu::IsHotbarDropTarget(int32 Slot) const
{
    if (PointerHotbarTarget != INDEX_NONE) return Slot == PointerHotbarTarget;
    // Keyboard / controller: the focused slot is where A would put what she holds.
    const bool Holding = HeldHotbarRow.IsSet() || HeldHotbarSlot != INDEX_NONE || bVirtualDraggingItem;
    return Holding && Region == ERegion::Hotbar && Slot == HotbarSelection && Slot != HeldHotbarSlot;
}
FLinearColor SHomesteadMenu::HotbarCellColor(int32 Slot) const
{
    if (IsHotbarDropTarget(Slot))
    {
        // Gold where it will go; a rust wash for something worn, which has to come off first.
        const FHomesteadRow* Row = HeldHotbarSlot == INDEX_NONE ? HotbarCandidateRow() : nullptr;
        const bool Refused = Row && Row->ContainerId < 0;
        return Refused ? FLinearColor(0.42f, 0.16f, 0.08f, 0.9f) : MenuGold;
    }
    if (Slot == HeldHotbarSlot) return FLinearColor(0.045f, 0.055f, 0.05f, 0.72f);
    return Region == ERegion::Hotbar && Slot == HotbarSelection ? Selected : FLinearColor(0.055f, 0.09f, 0.075f, 0.5f);
}
FHomesteadHotbarSlot SHomesteadMenu::BookHotbarSlot(int32 Slot) const
{
    if (HotbarSnapshotFrame != GFrameCounter)
    {
        HotbarSnapshotCache = Controller.IsValid() ? Controller->HotbarSnapshot() : TArray<FHomesteadHotbarSlot>();
        HotbarSnapshotFrame = GFrameCounter;
    }
    return HotbarSnapshotCache.IsValidIndex(Slot) ? HotbarSnapshotCache[Slot] : FHomesteadHotbarSlot();
}
TSharedRef<SWidget> SHomesteadMenu::BuildBookHotbar()
{
    // The same ten numbered cells as the world hotbar: the first row of her pack, holding the very
    // stacks the world hotbar shows. A stack used up leaves its cell empty.
    TSharedRef<SHorizontalBox> Strip = SNew(SHorizontalBox);
    HotbarCells.Reset();
    for (int32 Slot = 0; Slot < Homestead::HotbarSize; ++Slot)
    {
        const auto SlotInfo = [this, Slot]() { return BookHotbarSlot(Slot); };
        TSharedRef<SMenuButton> Button = SNew(SMenuButton).ButtonStyle(&MenuButtonStyle()).IsFocusable(true).ContentPadding(0)
            .ButtonColorAndOpacity_Lambda([this, Slot]() { return HotbarCellColor(Slot); })
            .ToolTipText(TAttribute<FText>::CreateLambda([this, Slot]()
            {
                const FString Key = UTF8_TO_TCHAR(Homestead::HotbarKeyLabel(Slot).c_str());
                FHomesteadRow Held;
                if (!Controller.IsValid() || !Controller->MenuHotbarRow(Slot, Held))
                    return FText::FromString(FString::Printf(TEXT("Slot %s: empty. Drag anything from your pack or the chest here."), *Key));
                return FText::FromString(Held.Quantity > 1 ? FString::Printf(TEXT("Slot %s: %s  x%d"), *Key, *Held.Name, Held.Quantity)
                    : FString::Printf(TEXT("Slot %s: %s"), *Key, *Held.Name));
            }))
            .OnPressed_Lambda([this, Slot, SlotInfo]()
            {
                // Pressing a used slot may start dragging it onto another.
                if (!PointerAction() || Dialog != EDialog::None || HeldHotbarRow.IsSet() || !SlotInfo().Assigned) return;
                CancelVirtualItemDrag();
                HeldHotbarSlot = Slot;
                HotbarDragStart = FSlateApplication::Get().GetCursorPos();
                bHotbarPointerDown = true;
                bHotbarPointerDragging = false;
            })
            .OnReleased_Lambda([this]() { if (bHotbarPointerDown) EndHotbarPointerDrag(); })
            .OnClicked_Lambda([this, Slot]()
            {
                if (!PointerAction() || Dialog != EDialog::None) return FReply::Handled();
                if (bSuppressHotbarClick) { bSuppressHotbarClick = false; return FReply::Handled(); }
                Region = ERegion::Hotbar;
                HotbarSelection = Slot;
                // Holding a stack ("Move to a hotbar slot"): a click places it.
                if (HeldHotbarRow.IsSet()) ActivateHotbarSlot(Slot);
                bFocusPending = true;
                return FReply::Handled();
            });
        Button->RightClick = [this, Slot]() { if (PointerAction() && Dialog == EDialog::None) OpenHotbarSlotMenu(Slot, true); };
        Button->SetContent(
            SNew(SBox).WidthOverride(MenuHotbarStyle::SlotSize).HeightOverride(MenuHotbarStyle::SlotSize)
            [
                SNew(SOverlay)
                + SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Fill)
                [
                    // The same thin ink outline and pale fill as an empty pack tile, so an empty cell
                    // reads as a slot of her pack (the fill steps aside for focus and drop colours).
                    SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).Padding(1.5f)
                    .BorderBackgroundColor(FLinearColor(Ink.R, Ink.G, Ink.B, 0.2f)).Visibility(EVisibility::HitTestInvisible)
                    [
                        SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                        .BorderBackgroundColor_Lambda([this, Slot]()
                        {
                            const bool Lit = IsHotbarDropTarget(Slot) || Slot == HeldHotbarSlot
                                || (Region == ERegion::Hotbar && Slot == HotbarSelection);
                            return Lit ? FLinearColor::Transparent : FLinearColor(0.2f, 0.3f, 0.24f, 0.25f);
                        })
                    ]
                ]
                + SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Fill)
                [
                    // Her selected slot keeps the world hotbar's gold frame.
                    SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).Padding(2)
                    .BorderBackgroundColor_Lambda([this, Slot]()
                    {
                        return Controller.IsValid() && Controller->SelectedHotbarIndex() == Slot ? MenuGold : FLinearColor::Transparent;
                    })
                    [ SNew(SBox) ]
                ]
                + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
                [
                    SNew(SBox).WidthOverride(40).HeightOverride(40)
                    .Visibility_Lambda([SlotInfo]() { return SlotInfo().Assigned ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
                    [
                        SNew(SHomesteadIcon)
                        .Kind_Lambda([SlotInfo]() { return SlotInfo().Icon; })
                        .Tint_Lambda([this, Slot, SlotInfo]()
                        {
                            // Pine on the gold of her selected cell, as on the world hotbar.
                            if (IsHotbarDropTarget(Slot) || (Controller.IsValid() && Controller->SelectedHotbarIndex() == Slot)) return PineInk;
                            return SlotInfo().Available ? MenuGold : Muted.CopyWithNewOpacity(0.45f);
                        })
                    ]
                ]
                + SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(5, 3, 0, 0)
                [
                    SNew(STextBlock).Text(FText::FromString(UTF8_TO_TCHAR(Homestead::HotbarKeyLabel(Slot).c_str())))
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
                    .ColorAndOpacity_Lambda([this, Slot]() { return FSlateColor(IsHotbarDropTarget(Slot) || (Controller.IsValid() && Controller->SelectedHotbarIndex() == Slot) ? PineInk : Ink); })
                    .Visibility(EVisibility::HitTestInvisible)
                ]
                + SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0, 0, 5, 2)
                [
                    // The stack's count (tools and garments are single).
                    SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
                    .Text_Lambda([SlotInfo]()
                    {
                        const auto Info = SlotInfo();
                        return Info.Assigned && (Info.Food || Info.Seed || Info.Material) ? FText::AsNumber(Info.Count) : FText::GetEmpty();
                    })
                    .ColorAndOpacity_Lambda([this, Slot, SlotInfo]()
                    {
                        if (IsHotbarDropTarget(Slot) || (Controller.IsValid() && Controller->SelectedHotbarIndex() == Slot)) return FSlateColor(PineInk);
                        return FSlateColor(SlotInfo().Available ? Ink : Muted);
                    })
                    .Visibility(EVisibility::HitTestInvisible)
                ]
            ]);
        HotbarCells.Add(Button);
        Strip->AddSlot().AutoWidth().Padding(3, 0)[ RegisterButton(Button, ERegion::Hotbar, Slot) ];
    }
    return Strip;
}
void SHomesteadMenu::OpenHotbarSlotMenu(int32 Slot, bool bPointer)
{
    if (!Controller.IsValid() || Dialog != EDialog::None) return;
    FHomesteadRow Row;
    if (!Controller->MenuHotbarRow(Slot, Row)) return;
    CancelHotbarHolds();
    Region = ERegion::Hotbar;
    HotbarSelection = Slot;
    // A cell holds a real stack, so it offers what that stack does anywhere in her pack (eat, drop,
    // split, store, "Move into the pack").
    OpenItemContextMenuFor(Row, PopupAnchorFor(HotbarCells.IsValidIndex(Slot) ? HotbarCells[Slot] : nullptr, bPointer));
}
void SHomesteadMenu::EndHotbarPointerDrag()
{
    const bool WasDragging = bHotbarPointerDragging;
    const int32 From = HeldHotbarSlot;
    int32 To = INDEX_NONE;
    int32 Onto = INDEX_NONE;
    bool bOverSource = false, bOverPack = false, bOverChest = false;
    if (WasDragging && FSlateApplication::IsInitialized())
    {
        const FVector2D Position = FSlateApplication::Get().GetCursorPos();
        To = HotbarCellAt(Position);
        bOverSource = To == From;
        if (To == INDEX_NONE)
        {
            for (int32 Index = 0; Index < Cells.Num() && Onto == INDEX_NONE; ++Index)
                if (Cells[Index] && Entries.IsValidIndex(Index) && Cells[Index]->GetCachedGeometry().IsUnderLocation(Position)) Onto = Index;
            bOverPack = PackDropArea && PackDropArea->GetCachedGeometry().IsUnderLocation(Position);
            bOverChest = ChestDropArea && ChestDropArea->GetCachedGeometry().IsUnderLocation(Position);
        }
    }
    bHotbarPointerDown = false;
    bHotbarPointerDragging = false;
    PointerHotbarTarget = INDEX_NONE;
    HeldHotbarSlot = INDEX_NONE;
    bSuppressHotbarClick = WasDragging && bOverSource;
    if (!WasDragging || !Controller.IsValid()) return;
    // Onto another cell it moves, merges or swaps; onto a stack below it merges or swaps; onto the
    // rest of her pack it goes to its end; onto the chest it is stored. Anywhere else, nothing.
    if (To != INDEX_NONE && To != From)
    {
        Controller->MenuMoveHotbarSlot(From, To);
        Region = ERegion::Hotbar;
        HotbarSelection = To;
        bFocusPending = true;
    }
    else if (Onto != INDEX_NONE) Controller->MenuMoveHotbarToPack(From, &Entries[Onto]);
    else if (bOverChest && Controller->ActiveStorageChest().IsSet())
    {
        FHomesteadRow Chest;
        Chest.ContainerId = Controller->ActiveStorageChest().GetValue();
        Controller->MenuMoveHotbarToPack(From, &Chest);
    }
    else if (bOverPack) Controller->MenuMoveHotbarToPack(From, nullptr);
}
void SHomesteadMenu::CancelHotbarHolds()
{
    HeldHotbarRow.Reset();
    HeldHotbarSlot = INDEX_NONE;
    bHotbarPointerDown = false;
    bHotbarPointerDragging = false;
    PointerHotbarTarget = INDEX_NONE;
    bSuppressHotbarClick = false;
}
void SHomesteadMenu::BeginPlacingOnHotbar(const FHomesteadRow& Row)
{
    if (!Controller.IsValid() || SeenPage != 0) return;
    if (Row.ContainerId < 0 || (Row.Subject != EHomesteadMenuSubject::ItemGroup && Row.Subject != EHomesteadMenuSubject::Wearable))
    {
        // Worn: the controller refuses it with the reason.
        Controller->MenuPlaceInHotbar(Row, FMath::Clamp(HotbarSelection, 0, Homestead::PackRowSize - 1));
        return;
    }
    CancelPointerItemDrag();
    CancelVirtualItemDrag();
    CancelHotbarHolds();
    HeldHotbarRow = Row;
    // Start on the first empty cell, else her selected one.
    const int32 Free = Controller->FirstEmptyHotbarCell();
    Region = ERegion::Hotbar;
    HotbarSelection = FMath::Clamp(Free != INDEX_NONE ? Free : Controller->SelectedHotbarIndex(), 0, Homestead::PackRowSize - 1);
    bFocusPending = true;
}
void SHomesteadMenu::ActivateHotbarSlot(int32 Slot)
{
    if (!Controller.IsValid() || Slot < 0 || Slot >= Homestead::PackRowSize) return;
    Region = ERegion::Hotbar;
    HotbarSelection = Slot;
    bFocusPending = true;
    // A stack picked up in a grid (A) or with "Move to a hotbar slot": place it here.
    if (bVirtualDraggingItem && Entries.IsValidIndex(VirtualDragSource))
    {
        const FHomesteadRow Row = Entries[VirtualDragSource];
        CancelVirtualItemDrag();
        Controller->MenuPlaceInHotbar(Row, Slot);
        return;
    }
    if (HeldHotbarRow.IsSet())
    {
        const FHomesteadRow Row = HeldHotbarRow.GetValue();
        HeldHotbarRow.Reset();
        Controller->MenuPlaceInHotbar(Row, Slot);
        return;
    }
    // A cell picked up with A: put it down here (moving, merging, or swapping with what's here).
    if (HeldHotbarSlot != INDEX_NONE)
    {
        const int32 From = HeldHotbarSlot;
        HeldHotbarSlot = INDEX_NONE;
        if (From != Slot) Controller->MenuMoveHotbarSlot(From, Slot);
        return;
    }
    if (Controller->HotbarEntry(Slot)) HeldHotbarSlot = Slot;
}
FString SHomesteadMenu::HotbarHint() const
{
    if (!Controller.IsValid()) return {};
    const bool Pad = Controller->UsesGamepad();
    const FHomesteadRow* Held = HeldHotbarRow.IsSet() ? &HeldHotbarRow.GetValue()
        : bVirtualDraggingItem && Entries.IsValidIndex(VirtualDragSource) ? &Entries[VirtualDragSource] : nullptr;
    if (Held)
        return FString::Printf(TEXT("Choose a hotbar slot for %s\n%s"), *EntryName(*Held),
            Pad ? TEXT("D-pad  choose slot     A  put it here (swaps)     B  cancel")
                : TEXT("Click a slot or press Enter to put it there (swaps)     Esc  cancel"));
    const auto Name = [this](int32 Slot)
    {
        FHomesteadRow Row;
        return Controller->MenuHotbarRow(Slot, Row) ? Row.Name : FString(TEXT("Empty"));
    };
    if (HeldHotbarSlot != INDEX_NONE)
        return FString::Printf(TEXT("Moving %s from slot %s\n%s"), *Name(HeldHotbarSlot),
            UTF8_TO_TCHAR(Homestead::HotbarKeyLabel(HeldHotbarSlot).c_str()),
            Pad ? TEXT("D-pad  choose slot     A  put it here (swaps)     B  cancel")
                : TEXT("Enter  put it here (swaps)     Esc  cancel"));
    const FString Subject = FString::Printf(TEXT("Slot %s: %s  (the first row of your pack)\n"),
        UTF8_TO_TCHAR(Homestead::HotbarKeyLabel(HotbarSelection).c_str()), *Name(HotbarSelection));
    return Subject + (Pad ? TEXT("A  pick up and move     Y  options     In the pack, Y  Move to a hotbar slot")
        : TEXT("Drag stacks in, out and between slots     Right-click  options"));
}
}