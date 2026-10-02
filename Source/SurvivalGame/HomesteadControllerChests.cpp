// Her storage chests in the field book (UI/SHomesteadMenu): auto-store onto matching stacks and the
// chest's name (Simulation/HomesteadChests.h).
#include "HomesteadController.h"
#include "HomesteadControllerText.h"

#include "Simulation/HomesteadChests.h"

using HomesteadControllerText::Text;

FString AHomesteadController::ChestDisplayName(int32 ChestId) const
{
    return Text(Homestead::Chests::DisplayName(State(), ChestId).c_str());
}

bool AHomesteadController::MenuStoreMatching()
{
    if (RejectPendingGroundSnapAction()) return false;
    if (!ActiveChestId.IsSet()) return false;
    const auto Result = Sim.StoreMatching(ActiveChestId.GetValue(), PlayerPoint(), Sim.GetRevision());
    Notify(Result);
    return Result.ok;
}

bool AHomesteadController::MenuRenameChest(const FString& Name)
{
    if (RejectPendingGroundSnapAction()) return false;
    if (!ActiveChestId.IsSet()) return false;
    const auto Result = Sim.RenameChest(ActiveChestId.GetValue(), TCHAR_TO_UTF8(*Name), PlayerPoint(), Sim.GetRevision());
    // The chest's title shows the new name; only a refusal needs saying.
    NotifyResourceAction(Result, nullptr);
    return Result.ok;
}
