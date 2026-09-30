// The field book's hotbar strip: the ten numbered slots shown whenever the inventory is open (on the
// Pack page under the grid, or under both grids with a chest open). She drops a pack stack on a slot
// or drags one slot onto another; the rules live in Simulation/HomesteadHotbarLayout.h and the
// controller applies them (HomesteadControllerHotbarEditor.cpp). Bindings only, never stock.
#include "SHomesteadMenuPrivate.h"
#include "../Simulation/HomesteadHotbarLayout.h"

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
        // Gold where it will go; a rust wash for something the hotbar won't take (the drop says why).
        const FHomesteadRow* Row = HeldHotbarSlot == INDEX_NONE ? HotbarCandidateRow() : nullptr;
        const bool Refused = Row && (Row->Subject != EHomesteadMenuSubject::ItemGroup || Row->ContainerId != 0 || Row->Id < 0
            || Row->Id >= static_cast<int32>(Homestead::Item::Count)
            || !AHomesteadController::CanPinToHotbar(static_cast<Homestead::Item>(Row->Id)));
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
    // The same ten numbered slots as the world hotbar, laid in the book so she can set them up:
    // drop a pack stack on a slot, or drag one slot onto another. A seed or food she has run out
    // of keeps its slot here (dimmed, "0") even though the world hotbar shows that slot empty.
    TSharedRef<SHorizontalBox> Strip = SNew(SHorizontalBox);
    HotbarCells.Reset();
    for (int32 Slot = 0; Slot < Homestead::HotbarSize; ++Slot)
    {
        const auto SlotInfo = [this, Slot]() { return BookHotbarSlot(Slot); };
        TSharedRef<SMenuButton> Button = SNew(SMenuButton).ButtonStyle(&MenuButtonStyle()).IsFocusable(true).ContentPadding(0)
            .ButtonColorAndOpacity_Lambda([this, Slot]() { return HotbarCellColor(Slot); })
            .ToolTipText(TAttribute<FText>::CreateLambda([SlotInfo, Slot]()
            {
                const auto Info = SlotInfo();
                const FString Key = UTF8_TO_TCHAR(Homestead::HotbarKeyLabel(Slot).c_str());
                if (!Info.Assigned)
                    return FText::FromString(FString::Printf(TEXT("Slot %s: empty. Drag a tool, food or seed here."), *Key));
                const FString Name = UTF8_TO_TCHAR(Homestead::ItemName(Info.Tool));
                return FText::FromString(Info.Available ? FString::Printf(TEXT("Slot %s: %s"), *Key, *Name)
                    : FString::Printf(TEXT("Slot %s: %s (none in your pack; it comes back when you have some)"), *Key, *Name));
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
                // Holding a pack stack ("Put on a hotbar slot"): a click places it.
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
                            if (IsHotbarDropTarget(Slot)) return PineInk;
                            return SlotInfo().Available ? MenuGold : Muted.CopyWithNewOpacity(0.45f);
                        })
                    ]
                ]
                + SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(5, 3, 0, 0)
                [
                    SNew(STextBlock).Text(FText::FromString(UTF8_TO_TCHAR(Homestead::HotbarKeyLabel(Slot).c_str())))
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
                    .ColorAndOpacity_Lambda([this, Slot]() { return FSlateColor(IsHotbarDropTarget(Slot) ? PineInk : Ink); })
                    .Visibility(EVisibility::HitTestInvisible)
                ]
                + SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0, 0, 5, 2)
                [
                    // How many of a pinned food or seed she carries; "0" when she has run out.
                    SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
                    .Text_Lambda([SlotInfo]()
                    {
                        const auto Info = SlotInfo();
                        return Info.Assigned && (Info.Food || Info.Seed) ? FText::AsNumber(Info.Count) : FText::GetEmpty();
                    })
                    .ColorAndOpacity_Lambda([this, Slot, SlotInfo]()
                    {
                        if (IsHotbarDropTarget(Slot)) return FSlateColor(PineInk);
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
    const auto Snapshot = Controller->HotbarSnapshot();
    if (!Snapshot.IsValidIndex(Slot) || !Snapshot[Slot].Assigned) return;
    CancelHotbarHolds();
    Region = ERegion::Hotbar;
    HotbarSelection = Slot;
    const auto Item = Snapshot[Slot].Tool;
    PopupTitle = FString::Printf(TEXT("Slot %s: %s"), UTF8_TO_TCHAR(Homestead::HotbarKeyLabel(Slot).c_str()),
        UTF8_TO_TCHAR(Homestead::ItemName(Item)));
    PopupOptions.Reset();
    // Clearing only unbinds the slot; what she carries stays in her pack.
    PopupOptions.Add({[]() { return FString(TEXT("Clear this slot")); },
        [this, Item]() { if (Controller.IsValid() && Controller->IsPinnedToHotbar(Item)) Controller->TogglePinnedToHotbar(Item); }, nullptr});
    PopupOptions.Add({[]() { return FString(TEXT("Cancel")); }, nullptr, nullptr});
    PopupAnchor = PopupAnchorFor(HotbarCells.IsValidIndex(Slot) ? HotbarCells[Slot] : nullptr, bPointer);
    SetDialog(EDialog::Context);
}
void SHomesteadMenu::EndHotbarPointerDrag()
{
    const bool WasDragging = bHotbarPointerDragging;
    const int32 From = HeldHotbarSlot;
    int32 To = INDEX_NONE;
    bool bOverSource = false;
    if (WasDragging && FSlateApplication::IsInitialized())
    {
        const FVector2D Position = FSlateApplication::Get().GetCursorPos();
        To = HotbarCellAt(Position);
        bOverSource = To == From;
    }
    bHotbarPointerDown = false;
    bHotbarPointerDragging = false;
    PointerHotbarTarget = INDEX_NONE;
    HeldHotbarSlot = INDEX_NONE;
    bSuppressHotbarClick = WasDragging && bOverSource;
    // Onto another slot it moves or swaps; released off the strip, nothing changes.
    if (WasDragging && To != INDEX_NONE && To != From && Controller.IsValid())
    {
        Controller->MenuMoveHotbarSlot(From, To);
        Region = ERegion::Hotbar;
        HotbarSelection = To;
        bFocusPending = true;
    }
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
    // Chest goods come to the pack first; say so rather than moving them implicitly.
    if (Row.Subject != EHomesteadMenuSubject::ItemGroup || Row.ContainerId != 0)
    {
        Controller->MenuAssignHotbarSlot(Row, FMath::Clamp(HotbarSelection, 0, Homestead::HotbarSize - 1));
        return;
    }
    CancelPointerItemDrag();
    CancelVirtualItemDrag();
    CancelHotbarHolds();
    HeldHotbarRow = Row;
    // Start on the slot it already has, else the first empty one, else her selected slot.
    const auto Item = static_cast<Homestead::Item>(Row.Id);
    const auto Snapshot = Controller->HotbarSnapshot();
    int32 Start = Snapshot.IndexOfByPredicate([Item](const FHomesteadHotbarSlot& Slot) { return Slot.Assigned && Slot.Tool == Item; });
    if (Start == INDEX_NONE) Start = Snapshot.IndexOfByPredicate([](const FHomesteadHotbarSlot& Slot) { return !Slot.Assigned; });
    if (Start == INDEX_NONE) Start = Controller->SelectedHotbarIndex();
    Region = ERegion::Hotbar;
    HotbarSelection = FMath::Clamp(Start, 0, Homestead::HotbarSize - 1);
    bFocusPending = true;
}
void SHomesteadMenu::ActivateHotbarSlot(int32 Slot)
{
    if (!Controller.IsValid() || Slot < 0 || Slot >= Homestead::HotbarSize) return;
    Region = ERegion::Hotbar;
    HotbarSelection = Slot;
    bFocusPending = true;
    // A pack stack picked up in the grid (A) or with "Put on a hotbar slot": place it here.
    if (bVirtualDraggingItem && Entries.IsValidIndex(VirtualDragSource))
    {
        const FHomesteadRow Row = Entries[VirtualDragSource];
        CancelVirtualItemDrag();
        Controller->MenuAssignHotbarSlot(Row, Slot);
        return;
    }
    if (HeldHotbarRow.IsSet())
    {
        const FHomesteadRow Row = HeldHotbarRow.GetValue();
        HeldHotbarRow.Reset();
        Controller->MenuAssignHotbarSlot(Row, Slot);
        return;
    }
    // A slot picked up with A: put it down here (moving, or swapping with what's here).
    if (HeldHotbarSlot != INDEX_NONE)
    {
        const int32 From = HeldHotbarSlot;
        HeldHotbarSlot = INDEX_NONE;
        if (From != Slot) Controller->MenuMoveHotbarSlot(From, Slot);
        return;
    }
    const auto Snapshot = Controller->HotbarSnapshot();
    if (Snapshot.IsValidIndex(Slot) && Snapshot[Slot].Assigned) HeldHotbarSlot = Slot;
}
FString SHomesteadMenu::HotbarHint() const
{
    if (!Controller.IsValid()) return {};
    const bool Pad = Controller->UsesGamepad();
    const FHomesteadRow* Held = HeldHotbarRow.IsSet() ? &HeldHotbarRow.GetValue()
        : bVirtualDraggingItem && Entries.IsValidIndex(VirtualDragSource) ? &Entries[VirtualDragSource] : nullptr;
    if (Held && Held->ContainerId != 0)
        return FString::Printf(TEXT("%s is in the chest: take it to your pack first, then put it on the hotbar\n%s"), *EntryName(*Held),
            Pad ? TEXT("B  cancel") : TEXT("Esc  cancel"));
    if (Held)
        return FString::Printf(TEXT("Choose a hotbar slot for %s\n%s"), *EntryName(*Held),
            Pad ? TEXT("D-pad  choose slot     A  put it here     B  cancel")
                : TEXT("Click a slot or press Enter to put it there     Esc  cancel"));
    const auto Snapshot = Controller->HotbarSnapshot();
    const auto Name = [&Snapshot](int32 Slot)
    {
        return Snapshot.IsValidIndex(Slot) && Snapshot[Slot].Assigned ? FString(UTF8_TO_TCHAR(Homestead::ItemName(Snapshot[Slot].Tool))) : FString(TEXT("Empty"));
    };
    if (HeldHotbarSlot != INDEX_NONE)
        return FString::Printf(TEXT("Moving %s from slot %s\n%s"), *Name(HeldHotbarSlot),
            UTF8_TO_TCHAR(Homestead::HotbarKeyLabel(HeldHotbarSlot).c_str()),
            Pad ? TEXT("D-pad  choose slot     A  put it here (swaps)     B  cancel")
                : TEXT("Enter  put it here (swaps)     Esc  cancel"));
    const FString Subject = FString::Printf(TEXT("Slot %s: %s\n"), UTF8_TO_TCHAR(Homestead::HotbarKeyLabel(HotbarSelection).c_str()), *Name(HotbarSelection));
    return Subject + (Pad ? TEXT("A  pick up and move     Y  clear slot     In the pack, Y  Put on a hotbar slot")
        : TEXT("Drag a pack stack onto a slot     Drag slot onto slot to swap     Right-click  clear"));
}
}
