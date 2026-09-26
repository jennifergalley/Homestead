#include "../HomesteadController.h"
#include "../HomesteadCharacter.h"

namespace
{
bool IsFood(Homestead::Item Item)
{
    return Item == Homestead::Item::Berries || Item == Homestead::Item::RoastedRoots || Item == Homestead::Item::HerbedRoots;
}
FString FromUtf8(const char* Text) { return UTF8_TO_TCHAR(Text); }
}

void AHomesteadController::MenuInventoryView(int32 View)
{
    MenuInventoryViewIndex = FMath::Clamp(View, 0, 2);
}

FString AHomesteadController::MenuInventorySummary() const
{
    if (ActiveChestId.IsSet())
        return FString::Printf(TEXT("Chest %d: %d / 120  |  Pack: %d / 120"),
            ActiveChestId.GetValue(), Sim.ChestUsedCapacity(ActiveChestId.GetValue()), Sim.UsedCapacity());
    if (MenuInventoryViewIndex == 2) return TEXT("Equipped clothing");
    return FString::Printf(TEXT("Your pack  |  %d / 120 units"), Sim.UsedCapacity());
}

TArray<FHomesteadRow> AHomesteadController::MenuRows() const
{
    if (Page != 0)
    {
        auto Result = Rows();
        if (Page == 6)
            Result.RemoveAll([](const FHomesteadRow& Row) { return Row.Id == 4 || Row.Id == 5; });
        if (Page == 1)
        {
            for (int Index = 0; Index < static_cast<int>(Homestead::WearableDefinition::Count); ++Index)
            {
                const auto Definition = static_cast<Homestead::WearableDefinition>(Index);
                const auto* Info = Homestead::GetWearableDefinition(Definition);
                if (!Info || Info->fiberCost <= 0) continue;
                FHomesteadRow Row;
                Row.Id = Index; Row.SubjectId = Index; Row.Subject = EHomesteadMenuSubject::GarmentRecipe;
                Row.Name = Row.Label = FromUtf8(Info->name);
                Row.Detail = TEXT("Needs: ") + FromUtf8(Homestead::GarmentRequirements(Definition));
                Row.Action = TEXT("Craft clothing");
                Row.Icon = FName(UTF8_TO_TCHAR(Info->key));
                Result.Add(MoveTemp(Row));
            }
        }
        return Result;
    }

    TArray<FHomesteadRow> Result;
    const int Chest = ActiveChestId.Get(-1);
    const bool Storage = ActiveChestId.IsSet();
    const int Container = MenuInventoryViewIndex == 1 ? Chest : 0;
    auto AddWearable = [&](const Homestead::WearableInstance& Instance)
    {
        const auto* Info = Homestead::GetWearableDefinition(Instance.definition);
        if (!Info) return;
        FHomesteadRow Row;
        Row.Id = static_cast<int>(Instance.definition); Row.SubjectId = Instance.id;
        Row.Subject = EHomesteadMenuSubject::Wearable;
        Row.ContainerId = Instance.owner == Homestead::WearableOwner::Chest ? Instance.chestId
            : Instance.owner == Homestead::WearableOwner::Equipped ? -1 : 0;
        Row.DestinationId = Storage ? (Row.ContainerId == 0 ? Chest : 0) : -1;
        Row.Quantity = 1;
        Row.Name = Row.Label = FromUtf8(Info->name);
        Row.Location = Row.ContainerId < 0 ? TEXT("Wearing") : Row.ContainerId == 0 ? TEXT("Carried")
            : FString::Printf(TEXT("Chest %d"), Row.ContainerId);
        Row.Detail = FromUtf8(Homestead::WearableDescription(Instance.definition));
        if (Info->dyeable)
        {
            Row.Detail += TEXT("\nDye: ") + FromUtf8(Homestead::DyeName(Instance.dye));
            Row.Name += TEXT(" - ") + FromUtf8(Homestead::DyeName(Instance.dye));
            Row.Label = Row.Name;
            Row.IconTint = FLinearColor(0.20f, 0.27f, 0.115f) * HomesteadLook::TunicTint(Instance.dye);
        }
        Row.Detail += FString::Printf(TEXT("\nOwned item #%d\n"), Instance.id);
        if (Info->slots & (1u << static_cast<int>(Homestead::EquipmentSlot::Torso))) Row.Detail += TEXT("Torso + legs");
        else if (Info->slots & (1u << static_cast<int>(Homestead::EquipmentSlot::Apron))) Row.Detail += TEXT("Apron layer (requires tunic)");
        else Row.Detail += TEXT("Feet");
        Row.Action = Row.ContainerId < 0 ? TEXT("Unequip") : Row.ContainerId == 0 ? TEXT("Equip") : TEXT("Take to pack");
        Row.Icon = Instance.definition == Homestead::WearableDefinition::LeatherShoes ? FName(TEXT("leather-shoes"))
            : FName(UTF8_TO_TCHAR(Info->key));
        Row.CanStore = false;
        Row.CanTake = false;
        Result.Add(MoveTemp(Row));
    };
    if (!Storage && MenuInventoryViewIndex == 2)
    {
        for (const auto& Instance : State().wearables)
            if (Instance.owner == Homestead::WearableOwner::Equipped) AddWearable(Instance);
        return Result;
    }
    const auto AddContainer = [&](int32 CurrentContainer)
    {
        if (CurrentContainer < 0) return;
        const auto* Layout = Sim.GetLayout(CurrentContainer);
        if (!Layout) return;
        for (const auto& Entry : *Layout)
        {
            if (Entry.wearableId)
            {
                if (const auto* Instance = Sim.GetWearable(Entry.wearableId)) AddWearable(*Instance);
                continue;
            }
            FHomesteadRow Row;
            Row.Id = static_cast<int>(Entry.item); Row.SubjectId = Entry.groupId;
            Row.Subject = EHomesteadMenuSubject::ItemGroup;
            Row.ContainerId = CurrentContainer;
            Row.DestinationId = Storage ? (CurrentContainer == 0 ? Chest : 0) : -1;
            Row.Quantity = Entry.quantity;
            Row.Name = Row.Label = FromUtf8(Homestead::ItemName(Entry.item));
            Row.Location = CurrentContainer == 0 ? TEXT("Carried") : FString::Printf(TEXT("Chest %d"), CurrentContainer);
            Row.Detail = FString::Printf(TEXT("%s: %d\nStack #%d\n\n%s"), *Row.Location, Entry.quantity, Entry.groupId,
                IsFood(Entry.item) ? TEXT("Food. Eat one from your pack.") : TEXT("Used in the world or in recipes."));
            Row.CanStore = false;
            Row.CanTake = false;
            Row.Action = CurrentContainer > 0 ? TEXT("Take to pack") : IsFood(Entry.item) ? TEXT("Eat 1") : FString();
            Result.Add(MoveTemp(Row));
        }
    };
    if (Storage) { AddContainer(Chest); AddContainer(0); }
    else AddContainer(Container);
    return Result;
}

bool AHomesteadController::MenuItemAction(const FHomesteadRow& Row, EHomesteadItemAction Action,
    int32 Amount, uint64 ExpectedRevision)
{
    if (bMenuSaveInProgress || IsFailed() || bTestResetRequired)
    { Notify(TEXT("This action is unavailable until you return to a playable world."), true); return false; }
    if (ExpectedRevision != Sim.GetRevision())
    { Notify(TEXT("Your inventory changed. Select the item again before confirming."), true); return false; }
    Homestead::Result Result{false, "That action is not available for this item."};
    if (Action == EHomesteadItemAction::Pin && Row.Subject == EHomesteadMenuSubject::ItemGroup)
        return Row.Id >= 0 && Row.Id < static_cast<int32>(Homestead::Item::Count)
            && TogglePinnedToHotbar(static_cast<Homestead::Item>(Row.Id));
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
            return Target.RecolorWearable(Row.SubjectId, (Item->dye + 1) % 4, PlayerPoint(), ExpectedRevision);
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
    Notify(Result);
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
    const int32 Room = Used < 0 ? 0 : Homestead::InventoryCapacity - Used;
    // Move what fits; with no room at all the transfer reports why.
    const int32 Count = Room > 0 ? FMath::Min(Row.Quantity, Room) : Row.Quantity;
    return MenuItemAction(Target, EHomesteadItemAction::Transfer, Count, Sim.GetRevision());
}

bool AHomesteadController::MenuSortPack()
{
    const auto Result = Sim.SortPack(Sim.GetRevision());
    Notify(Result);
    return Result.ok;
}

bool AHomesteadController::MenuDrop(const FHomesteadRow& Source, const FHomesteadRow& Target,
    uint64 ExpectedRevision)
{
    if (ExpectedRevision != Sim.GetRevision())
    { Notify(TEXT("Your inventory changed. Pick up the item again."), true); return false; }
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
        Notify(Result);
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
