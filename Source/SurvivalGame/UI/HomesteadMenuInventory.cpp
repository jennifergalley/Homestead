#include "../HomesteadController.h"
#include "../HomesteadCharacter.h"
#include "../Simulation/HomesteadChests.h"
#include "../Simulation/HomesteadFood.h"
#include "../Simulation/HomesteadItems.h"
#include "../Simulation/HomesteadPail.h"
#include "../Simulation/HomesteadPackRow.h"

namespace
{
bool IsFood(Homestead::Item Item) { return Homestead::IsEdible(Item); }
FString FromUtf8(const char* Text) { return UTF8_TO_TCHAR(Text); }
bool WearableRow(const Homestead::State& State, const Homestead::WearableInstance& Instance, bool Storage, int Chest, FHomesteadRow& Row)
{
    const auto* Info = Homestead::GetWearableDefinition(Instance.definition);
    if (!Info) return false;
    Row = FHomesteadRow();
    Row.Id = static_cast<int>(Instance.definition); Row.SubjectId = Instance.id;
    Row.Subject = EHomesteadMenuSubject::Wearable;
    Row.ContainerId = Instance.owner == Homestead::WearableOwner::Chest ? Instance.chestId
        : Instance.owner == Homestead::WearableOwner::Equipped ? -1 : 0;
    Row.DestinationId = Storage ? (Row.ContainerId == 0 ? Chest : 0) : -1;
    Row.Quantity = 1;
    Row.Name = Row.Label = FromUtf8(Info->name);
    Row.Location = Row.ContainerId < 0 ? TEXT("Wearing") : Row.ContainerId == 0 ? TEXT("Carried")
        : FromUtf8(Homestead::Chests::DisplayName(State, Row.ContainerId).c_str());
    Row.Detail = FromUtf8(Homestead::WearableDescription(Instance.definition));
    if (Info->dyeable)
    {
        Row.Detail += TEXT("\nDye: ") + FromUtf8(Homestead::DyeName(Instance.dye));
        Row.Name += TEXT(" - ") + FromUtf8(Homestead::DyeName(Instance.dye));
        Row.Label = Row.Name;
        Row.IconTint = FLinearColor(0.20f, 0.27f, 0.115f) * HomesteadLook::TunicTint(Instance.dye);
    }
    Row.Detail += FString::Printf(TEXT("\nOwned item #%d\n"), Instance.id);
    const auto Has = [Info](Homestead::EquipmentSlot Slot) { return (Info->slots & (1u << static_cast<int>(Slot))) != 0; };
    if (Has(Homestead::EquipmentSlot::Torso) && Has(Homestead::EquipmentSlot::Legs)) Row.Detail += TEXT("Torso + legs");
    else if (Has(Homestead::EquipmentSlot::Torso)) Row.Detail += TEXT("Torso");
    else if (Has(Homestead::EquipmentSlot::Legs)) Row.Detail += TEXT("Legs");
    else if (Has(Homestead::EquipmentSlot::Outer)) Row.Detail += TEXT("Outer layer, over any shirt");
    else if (Has(Homestead::EquipmentSlot::Apron)) Row.Detail += TEXT("Apron layer (requires a top)");
    else Row.Detail += TEXT("Feet");
    Row.Action = Row.ContainerId < 0 ? TEXT("Unequip") : Row.ContainerId == 0 ? TEXT("Equip") : TEXT("Take to pack");
    Row.Icon = Instance.definition == Homestead::WearableDefinition::LeatherShoes ? FName(TEXT("leather-shoes"))
        : FName(UTF8_TO_TCHAR(Info->key));
    Row.CanStore = false;
    Row.CanTake = false;
    return true;
}
}

void AHomesteadController::MenuInventoryView(int32 View)
{
    MenuInventoryViewIndex = FMath::Clamp(View, 0, 2);
}

FString AHomesteadController::MenuInventorySummary() const
{
    if (ActiveChestId.IsSet())
        return FString::Printf(TEXT("%s: %d / %d  |  Pack: %d / %d"),
            *ChestDisplayName(ActiveChestId.GetValue()), Sim.ChestUsedCapacity(ActiveChestId.GetValue()), Homestead::ChestCapacity,
            Sim.UsedCapacity(), Sim.PackCapacity());
    if (MenuInventoryViewIndex == 2) return TEXT("Equipped clothing");
    return FString::Printf(TEXT("Your pack  |  %d / %d units"), Sim.UsedCapacity(), Sim.PackCapacity());
}

TArray<FHomesteadRow> AHomesteadController::MenuRows() const
{
    if (Page != 0)
    {
        auto Result = Rows();
        if (Page == 6)
            Result.RemoveAll([](const FHomesteadRow& Row) { return Row.Id == 4 || Row.Id == 5; });
        // Garment recipes needed the retired knife and fibre; the dressmaker (round 3) sells clothing.
        return Result;
    }

    TArray<FHomesteadRow> Result;
    const int Chest = ActiveChestId.Get(-1);
    const bool Storage = ActiveChestId.IsSet();
    const int Container = MenuInventoryViewIndex == 1 ? Chest : 0;
    if (!Storage && MenuInventoryViewIndex == 2)
    {
        for (const auto& Instance : State().wearables)
            if (Instance.owner == Homestead::WearableOwner::Equipped)
            {
                FHomesteadRow Row;
                if (WearableRow(State(), Instance, Storage, Chest, Row)) Result.Add(MoveTemp(Row));
            }
        return Result;
    }
    const auto AddContainer = [&](int32 CurrentContainer)
    {
        if (CurrentContainer < 0) return;
        const auto* Layout = Sim.GetLayout(CurrentContainer);
        if (!Layout) return;
        for (const auto& Entry : *Layout)
        {
            // Her pack's first row is the hotbar, shown as its own ten cells (MenuHotbarRow).
            if (CurrentContainer == 0 && Homestead::PackRowRules::CellOf(State().packRow, Entry) >= 0) continue;
            FHomesteadRow Row;
            if (MenuEntryRow(Entry, CurrentContainer, Row)) Result.Add(MoveTemp(Row));
        }
    };
    if (Storage) { AddContainer(Chest); AddContainer(0); }
    else AddContainer(Container);
    return Result;
}

bool AHomesteadController::MenuEntryRow(const Homestead::LayoutEntry& Entry, int32 CurrentContainer, FHomesteadRow& Row) const
{
    const int Chest = ActiveChestId.Get(-1);
    const bool Storage = ActiveChestId.IsSet();
    Row = FHomesteadRow();
    if (Entry.wearableId)
    {
        const auto* Instance = Sim.GetWearable(Entry.wearableId);
        if (!Instance || !WearableRow(State(), *Instance, Storage, Chest, Row)) return false;
    }
    else
    {
        // With her one pail carried, the pack's water shows on the pail instead (HomesteadPail.h).
        const auto Pail = Homestead::PresentPail(State());
        if (CurrentContainer == 0 && Entry.item == Homestead::Item::Water && Pail.hidePackWater) return false;
        Row.Id = static_cast<int>(Entry.item); Row.SubjectId = Entry.groupId;
        Row.Subject = EHomesteadMenuSubject::ItemGroup;
        Row.ContainerId = CurrentContainer;
        Row.DestinationId = Storage ? (CurrentContainer == 0 ? Chest : 0) : -1;
        Row.Quantity = Entry.quantity;
        Row.Name = Row.Label = FromUtf8(Homestead::ItemName(Entry.item));
        Row.Location = CurrentContainer == 0 ? FString(TEXT("Carried")) : ChestDisplayName(CurrentContainer);
        // Hover text: where and how many, what it is and (for food) what eating one now would do: its
        // Energy each, and for a Meal on the estate until when she'd be Well fed (Homestead::Food::
        // PackUseText; the book pauses the clock). Stats only, no how-to (Jenny 2026-09-30); the
        // internal stack id is not shown.
        FString Use = IsFood(Entry.item) ? FromUtf8(Homestead::Food::PackUseText(State(), Entry.item).c_str()) : FString();
        Use.RemoveFromEnd(TEXT(" Eat one from your pack."));
        Row.Detail = FString::Printf(TEXT("%s: %d\n\n%s"), *Row.Location, Entry.quantity,
            *FromUtf8(Homestead::ItemDescription(Entry.item))) + (Use.IsEmpty() ? FString() : TEXT("\n") + Use);
        Row.CanStore = false;
        Row.CanTake = false;
        Row.Action = CurrentContainer > 0 ? TEXT("Take to pack") : IsFood(Entry.item) ? TEXT("Eat 1") : FString();
        if (CurrentContainer == 0 && Entry.item == Homestead::Item::WateringCan && Pail.gauge)
        {
            // Shown in the footer as well as the tooltip: a controller has no hover.
            Row.Status = FromUtf8(Homestead::PailChargeLabel(Pail).c_str());
            Row.Detail += TEXT("\n") + Row.Status;
        }
    }
    if (CurrentContainer == 0)
    {
        Row.HotbarCell = Homestead::PackRowRules::CellOf(State().packRow, Entry);
        if (Row.HotbarCell >= 0)
        {
            Row.Location = FString::Printf(TEXT("Hotbar %d"), Homestead::PackRowRules::KeyNumber(Row.HotbarCell));
            Row.Detail = Row.Location + TEXT(" (the first row of your pack)\n") + Row.Detail;
        }
    }
    return true;
}

bool AHomesteadController::MenuHotbarRow(int32 Cell, FHomesteadRow& Out) const
{
    const auto* Entry = HotbarEntry(Cell);
    return Entry && MenuEntryRow(*Entry, 0, Out);
}

bool AHomesteadController::MenuItemAction(const FHomesteadRow& Row, EHomesteadItemAction Action,
    int32 Amount, uint64 ExpectedRevision)
{
    if (RejectPendingGroundSnapAction()) return false;
    if (bMenuSaveInProgress || IsFailed() || bTestResetRequired)
    { Notify(TEXT("This action is unavailable until you return to a playable world."), true); return false; }
    if (ExpectedRevision != Sim.GetRevision())
    { Notify(TEXT("Your inventory changed. Select the item again before confirming."), true); return false; }
    Homestead::Result Result{false, "That action is not available for this item."};
    if (Action == EHomesteadItemAction::Pin)
    {
        // "Move to hotbar" / "Move to pack": the hotbar is the first row of her pack.
        if (Row.HotbarCell >= 0) return MenuMoveHotbarToPack(Row.HotbarCell, nullptr);
        const int32 Free = FirstEmptyHotbarCell();
        if (Free == INDEX_NONE)
        { Notify(TEXT("The hotbar is full. Drag this onto a hotbar slot to swap them."), true); return false; }
        return MenuPlaceInHotbar(Row, Free);
    }
    if (Row.Subject == EHomesteadMenuSubject::GarmentRecipe || Action == EHomesteadItemAction::Equip
        || Action == EHomesteadItemAction::Unequip || Action == EHomesteadItemAction::Dye)
    {
        auto Transaction = [&](Homestead::Simulation& Target) -> Homestead::Result
        {
            if (Row.Subject == EHomesteadMenuSubject::GarmentRecipe)
                return Target.CraftGarment(static_cast<Homestead::WearableDefinition>(Row.SubjectId), PlayerPoint(), ExpectedRevision);
            if (Action == EHomesteadItemAction::Equip) return Target.EquipWearable(Row.SubjectId, ExpectedRevision);
            if (Action == EHomesteadItemAction::Unequip) return Target.UnequipWearable(Row.SubjectId, ExpectedRevision);
            const auto* Item = Target.GetWearable(Row.SubjectId);
            if (!Item) return {false, "That owned garment no longer exists."};
            // The dye chooser passes the chosen dye + 1; older callers without one step to the next.
            const int32 Dye = Amount >= 1 && Amount <= 4 ? Amount - 1 : (Item->dye + 1) % 4;
            return Target.RecolorWearable(Row.SubjectId, Dye, PlayerPoint(), ExpectedRevision);
        };
        Homestead::Simulation Candidate = Sim;
        const auto Proposed = Transaction(Candidate);
        if (!Proposed) { Notify(Proposed); return false; }
        auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
        FString Error;
        if (!Avatar || !Avatar->PrepareEquipment(Candidate.GetState(), Appearance, Error))
        {
            Notify(Error.IsEmpty() ? TEXT("The character is not ready to display clothing. Your possessions have not changed.")
                : Error + TEXT(" Your possessions have not changed."), true);
            return false;
        }
        const Homestead::Simulation Previous = Sim;
        Result = Transaction(Sim);
        if (!Result) { Avatar->ClearPreparedEquipment(); Notify(Result); return false; }
        if (!Avatar->ApplyPreparedEquipment(Error))
        {
            Sim = Previous;
            Avatar->ClearPreparedEquipment();
            Notify(TEXT("The prepared wardrobe could not be displayed; the transaction was canceled. ") + Error, true);
            return false;
        }
        Notify(Result);
        return true;
    }
    if (Row.Subject == EHomesteadMenuSubject::Wearable)
    {
        if (Action == EHomesteadItemAction::Transfer && Row.ContainerId >= 0 && Row.DestinationId >= 0)
            Result = Sim.MoveWearable(Row.SubjectId, Row.DestinationId, PlayerPoint(), ExpectedRevision);
        else if (Action == EHomesteadItemAction::Drop && Row.ContainerId == 0)
        {
            Homestead::Point DropPoint;
            if (!ResolveDropPoint(DropPoint))
                Result = {false, "There is no clear dry ground nearby for this garment."};
            else Result = Sim.DropWearable(Row.SubjectId, DropPoint, PlayerPoint(), ExpectedRevision);
        }
    }
    else if (Row.Subject == EHomesteadMenuSubject::ItemGroup)
    {
        if (Action == EHomesteadItemAction::Transfer)
        {
            const int Chest = Row.ContainerId > 0 ? Row.ContainerId : Row.DestinationId;
            Result = Sim.TransferGroup(Chest, Row.SubjectId, Amount, Row.ContainerId == 0, PlayerPoint(), ExpectedRevision);
        }
        else if (Action == EHomesteadItemAction::Split)
            Result = Sim.SplitGroup(Row.ContainerId, Row.SubjectId, Amount, PlayerPoint(), ExpectedRevision);
        else if (Action == EHomesteadItemAction::Merge)
            Result = Sim.MergeGroups(Row.ContainerId, Row.SubjectId, Amount, PlayerPoint(), ExpectedRevision);
        else if (Action == EHomesteadItemAction::Drop && Row.ContainerId == 0)
        {
            Homestead::Point DropPoint;
            if (!ResolveDropPoint(DropPoint))
                Result = {false, "There is no clear dry ground nearby for this item."};
            else Result = Sim.DropGroup(Row.SubjectId, Amount, DropPoint, PlayerPoint(), ExpectedRevision);
        }
        else if (Action == EHomesteadItemAction::Primary && Row.ContainerId == 0)
        {
            if (IsFood(static_cast<Homestead::Item>(Row.Id))) Result = Sim.EatGroup(Row.SubjectId, ExpectedRevision);
            else { Notify(Row.Detail); return true; }
        }
    }
    if (Action == EHomesteadItemAction::MoveEarlier || Action == EHomesteadItemAction::MoveLater)
    {
        const auto* Layout = Sim.GetLayout(Row.ContainerId);
        if (Layout)
        {
            for (int Index = 0; Index < static_cast<int>(Layout->size()); ++Index)
                if ((Row.Subject == EHomesteadMenuSubject::Wearable && (*Layout)[Index].wearableId == Row.SubjectId)
                    || (Row.Subject == EHomesteadMenuSubject::ItemGroup && (*Layout)[Index].groupId == Row.SubjectId))
                {
                    Result = Sim.ReorderEntry(Row.ContainerId, Index, Index + (Action == EHomesteadItemAction::MoveEarlier ? -1 : 1),
                        PlayerPoint(), ExpectedRevision);
                    break;
                }
        }
    }
    // A move between pack and chest shows in the book itself; only a refusal needs words.
    if (Action == EHomesteadItemAction::Transfer) NotifyResourceAction(Result, nullptr);
    else Notify(Result);
    return Result.ok;
}

bool AHomesteadController::MenuSplitHalf(const FHomesteadRow& Row)
{
    if (Row.Subject != EHomesteadMenuSubject::ItemGroup || Row.ContainerId < 0)
    { Notify(TEXT("Only ordinary item stacks can be split."), true); return false; }
    const auto Result = Sim.SplitHalf(Row.ContainerId, Row.SubjectId, PlayerPoint(), Sim.GetRevision());
    Notify(Result);
    return Result.ok;
}

bool AHomesteadController::MenuMoveWhole(const FHomesteadRow& Row)
{
    if (!ActiveChestId.IsSet())
    { Notify(TEXT("Open a storage chest to move things into it."), true); return false; }
    if (Row.ContainerId < 0)
    { Notify(TEXT("Unequip that garment before storing it."), true); return false; }
    FHomesteadRow Target = Row;
    Target.DestinationId = Row.ContainerId == 0 ? ActiveChestId.GetValue() : 0;
    if (Row.Subject == EHomesteadMenuSubject::Wearable)
        return MenuItemAction(Target, EHomesteadItemAction::Transfer, 1, Sim.GetRevision());
    if (Row.Subject != EHomesteadMenuSubject::ItemGroup) return false;
    const int32 Used = Row.ContainerId > 0 ? Sim.UsedCapacity() : Sim.ChestUsedCapacity(ActiveChestId.GetValue());
    const int32 Room = Used < 0 ? 0 : Homestead::ContainerCapacity(State(), Target.DestinationId) - Used;
    // Move what fits; with no room at all the transfer reports why.
    const int32 Count = Room > 0 ? FMath::Min(Row.Quantity, Room) : Row.Quantity;
    return MenuItemAction(Target, EHomesteadItemAction::Transfer, Count, Sim.GetRevision());
}

bool AHomesteadController::MenuWearableRow(int32 WearableId, FHomesteadRow& Out) const
{
    const auto* Instance = Sim.GetWearable(WearableId);
    return Instance && WearableRow(State(), *Instance, ActiveChestId.IsSet(), ActiveChestId.Get(-1), Out);
}

bool AHomesteadController::MenuSortPack()
{
    if (RejectPendingGroundSnapAction()) return false;
    const auto Result = Sim.SortPack(Sim.GetRevision());
    Notify(Result);
    return Result.ok;
}

bool AHomesteadController::MenuDrop(const FHomesteadRow& Source, const FHomesteadRow& Target,
    uint64 ExpectedRevision)
{
    if (RejectPendingGroundSnapAction()) return false;
    if (ExpectedRevision != Sim.GetRevision())
    { Notify(TEXT("Your inventory changed. Pick up the item again."), true); return false; }
    // The hotbar is the first row of her pack: into, within and out of it (HomesteadPackRow.h).
    if (Source.HotbarCell >= 0 && Target.HotbarCell >= 0) return MenuMoveHotbarSlot(Source.HotbarCell, Target.HotbarCell);
    if (Target.HotbarCell >= 0) return MenuPlaceInHotbar(Source, Target.HotbarCell);
    if (Source.HotbarCell >= 0) return MenuMoveHotbarToPack(Source.HotbarCell, &Target);
    if (Source.ContainerId != Target.ContainerId)
    {
        if (!ActiveChestId.IsSet()
            || (Source.ContainerId != 0 && Source.ContainerId != ActiveChestId.GetValue())
            || (Target.ContainerId != 0 && Target.ContainerId != ActiveChestId.GetValue()))
        { Notify(TEXT("Choose a valid destination in the opened chest or pack."), true); return false; }
        Homestead::Result Result{false, "That item cannot move between these containers."};
        if (Source.Subject == EHomesteadMenuSubject::ItemGroup)
            Result = Sim.TransferGroup(ActiveChestId.GetValue(), Source.SubjectId, Source.Quantity,
                Source.ContainerId == 0, PlayerPoint(), ExpectedRevision);
        else if (Source.Subject == EHomesteadMenuSubject::Wearable)
            Result = Sim.MoveWearable(Source.SubjectId, Target.ContainerId, PlayerPoint(), ExpectedRevision);
        NotifyResourceAction(Result, nullptr);
        return Result.ok;
    }
    if (Source.ContainerId < 0)
    { Notify(TEXT("Choose a valid destination in this container."), true); return false; }
    if (Source.Subject == EHomesteadMenuSubject::ItemGroup
        && Target.Subject == EHomesteadMenuSubject::ItemGroup
        && Source.Id == Target.Id && Source.SubjectId != Target.SubjectId)
    {
        const auto Result = Sim.MergeGroups(Source.ContainerId, Source.SubjectId,
            Target.SubjectId, PlayerPoint(), ExpectedRevision);
        Notify(Result);
        return Result.ok;
    }
    const auto* Layout = Sim.GetLayout(Source.ContainerId);
    if (!Layout) return false;
    const auto FindIndex = [Layout](const FHomesteadRow& Row)
    {
        for (int32 Index = 0; Index < static_cast<int32>(Layout->size()); ++Index)
            if ((Row.Subject == EHomesteadMenuSubject::ItemGroup
                    && (*Layout)[Index].groupId == Row.SubjectId)
                || (Row.Subject == EHomesteadMenuSubject::Wearable
                    && (*Layout)[Index].wearableId == Row.SubjectId))
                return Index;
        return static_cast<int32>(INDEX_NONE);
    };
    const int32 SourceIndex = FindIndex(Source);
    const int32 TargetIndex = FindIndex(Target);
    if (SourceIndex == INDEX_NONE || TargetIndex == INDEX_NONE || SourceIndex == TargetIndex)
        return false;
    const auto Result = Sim.ReorderEntry(Source.ContainerId, SourceIndex, TargetIndex,
        PlayerPoint(), ExpectedRevision);
    Notify(Result);
    return Result.ok;
}
