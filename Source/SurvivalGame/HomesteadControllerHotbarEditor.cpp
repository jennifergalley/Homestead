// The field book's hotbar: the first row of her pack (Simulation/HomesteadPackRow.h, native-tested).
// She drags a pack or chest stack into a cell, one cell onto another, or a cell's stack back down into
// her pack; onto an empty cell it moves, onto the same item it merges, onto anything else the two
// swap. Stock only ever moves, all in one simulation transaction; this says why when it can't.
#include "HomesteadController.h"
#include "Simulation/HomesteadAudioLevels.h"

#include "Simulation/HomesteadPackRow.h"

bool AHomesteadController::MenuPlaceInHotbar(const FHomesteadRow& Row, int32 Cell)
{
    if (RejectPendingGroundSnapAction()) return false;
    if (Cell < 0 || Cell >= Homestead::PackRowSize)
    {
        Notify(TEXT("Choose one of the ten hotbar slots."), true);
        return false;
    }
    if (Row.HotbarCell == Cell) return true;
    if (Row.Subject == EHomesteadMenuSubject::EmptySlot) return false;
    Homestead::Result Result{false, "That can't go on the hotbar."};
    const bool Garment = Row.Subject == EHomesteadMenuSubject::Wearable;
    if (Row.Subject != EHomesteadMenuSubject::ItemGroup && !Garment)
        Result = {false, "That can't go on the hotbar."};
    else if (Row.ContainerId == 0)
        Result = Sim.MoveToPackRow(Garment ? 0 : Row.SubjectId, Garment ? Row.SubjectId : 0, Cell, Sim.GetRevision());
    else if (Row.ContainerId > 0 && !Garment)
        Result = Sim.TransferGroupToPackRow(Row.ContainerId, Row.SubjectId, Row.Quantity, Cell, PlayerPoint(), Sim.GetRevision());
    else if (Row.ContainerId > 0)
    {
        // A garment from the chest: into her pack, then into the cell. Each step is whole on its own;
        // if the second can't happen it simply waits in her pack.
        Result = Sim.MoveWearable(Row.SubjectId, 0, PlayerPoint(), Sim.GetRevision());
        if (Result) Result = Sim.MoveToPackRow(0, Row.SubjectId, Cell, Sim.GetRevision());
    }
    else Result = {false, "Take that garment off before putting it on the hotbar."};
    if (!Result) { Notify(Result); return false; }
    PlayEffect(UIClick, Homestead::AudioLevels::Gain::UIClickFaint);
    return true;
}

bool AHomesteadController::MenuMoveHotbarSlot(int32 From, int32 To)
{
    if (RejectPendingGroundSnapAction()) return false;
    const auto* Entry = HotbarEntry(From);
    // Picking up an empty cell is just nothing to move.
    if (!Entry) return false;
    if (From == To) return true;
    const auto Result = Sim.MoveToPackRow(Entry->wearableId ? 0 : Entry->groupId, Entry->wearableId, To, Sim.GetRevision());
    if (!Result) { Notify(Result); return false; }
    PlayEffect(UIClick, Homestead::AudioLevels::Gain::UIClickFaint);
    return true;
}

bool AHomesteadController::MenuMoveHotbarToPack(int32 Cell, const FHomesteadRow* Target)
{
    if (RejectPendingGroundSnapAction()) return false;
    if (!HotbarEntry(Cell)) return false;
    Homestead::Result Result;
    if (Target && Target->ContainerId > 0)
    {
        // Onto the open chest: store the whole stack (or garment) there.
        const auto* Entry = HotbarEntry(Cell);
        Result = Entry->wearableId
            ? Sim.MoveWearable(Entry->wearableId, Target->ContainerId, PlayerPoint(), Sim.GetRevision())
            : Sim.TransferGroup(Target->ContainerId, Entry->groupId, Entry->quantity, true, PlayerPoint(), Sim.GetRevision());
    }
    else if (Target && Target->ContainerId == 0 && Target->PackSlot != INDEX_NONE)
    {
        // Onto a square of her pack: exactly there, swapping with any occupant.
        const auto* Entry = HotbarEntry(Cell);
        Result = Sim.MoveToPackSlot(Entry->wearableId ? 0 : Entry->groupId, Entry->wearableId, Target->PackSlot, Sim.GetRevision());
    }
    else
    {
        const bool Garment = Target && Target->Subject == EHomesteadMenuSubject::Wearable;
        const bool Stack = Target && Target->Subject == EHomesteadMenuSubject::ItemGroup;
        Result = Sim.MoveFromPackRow(Cell, Stack ? Target->SubjectId : 0, Garment ? Target->SubjectId : 0, Sim.GetRevision());
    }
    if (!Result) { Notify(Result); return false; }
    PlayEffect(UIClick, Homestead::AudioLevels::Gain::UIClickFaint);
    return true;
}