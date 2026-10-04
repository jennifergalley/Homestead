#include "HomesteadController.h"
#include "HomesteadCharacter.h"
#include "Simulation/HomesteadAudioLevels.h"

bool AHomesteadController::MenuSetBackEquipment(bool bShown)
{
    if (RejectPendingGroundSnapAction()) return false;
    if (!bBookOpen || bMenuSaveInProgress || IsFailed())
    {
        Notify(TEXT("Back equipment is unavailable now."), true);
        return false;
    }
    const bool bCurrent = State().leatherBackpack && State().backpackShown;
    if (bCurrent == bShown) return true;
    const auto Result = Sim.SetBackpackShown(bShown);
    if (!Result) { Notify(Result); return false; }
    if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn()))
        Avatar->SetBackpackShown(State().leatherBackpack && State().backpackShown);
    RefreshMenuPortrait();
    PlayEffect(UIClick, Homestead::AudioLevels::Gain::UIClick);
    return true;
}
