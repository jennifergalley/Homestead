// Walking the public road between the manor and town (Map tab, and later the road signs): the
// preview from where she stands, and the walk itself. The whole walk is ordinary time in the
// Simulation (Simulation::WalkRoad): refused with nothing changed if she'd collapse or doze off on
// the way; otherwise the clock moves on and she's stood on the road at the far end.
#include "HomesteadController.h"

#include "HomesteadCharacter.h"
#include "Simulation/HomesteadTravel.h"

DEFINE_LOG_CATEGORY_STATIC(LogHomesteadTravel, Log, All);

Homestead::TravelPlan AHomesteadController::MenuPlanTravel(Homestead::TravelDestination Destination) const
{
    return Homestead::PlanTravel(State(), PlayerPoint(), Destination, Sim.Layout());
}

bool AHomesteadController::MenuTravel(Homestead::TravelDestination Destination, uint64 ExpectedRevision)
{
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    if (!Avatar || !bWorldReady || bPendingSpawn || bPendingGroundSnap)
    {
        Notify(TEXT("You can't set out just now."), true);
        return false;
    }
    if (ExpectedRevision != Sim.GetRevision())
    {
        Notify(TEXT("Things changed while you were deciding. Choose the walk again."), true);
        return false;
    }
    const Homestead::Point From = PlayerPoint();
    const Homestead::TravelPlan Plan = Homestead::PlanTravel(State(), From, Destination, Sim.Layout());
    if (!Plan.ok)
    {
        Notify(UTF8_TO_TCHAR(Plan.error.c_str()), true);
        return false;
    }
    // The ground there must be ready before any time passes (the fixed estate is always ready).
    if (!PrepareWorldAt(Plan.arrival)) return false;
    const auto Result = Sim.WalkRoad(Destination, From);
    if (!Result.ok)
    {
        Notify(UTF8_TO_TCHAR(Result.message.c_str()), true);
        return false;
    }
    UE_LOG(LogHomesteadTravel, Log, TEXT("Walked %.0f m to %s in %.2f game hours; now hour %.2f."),
        Plan.totalMetres, UTF8_TO_TCHAR(Homestead::TravelDestinationName(Destination)), Plan.gameHours, State().hour);
    Avatar->CancelAction(true);
    Avatar->ResetSprint();
    // Stood on the road bed facing the way she walked; the ground snap holds her until the
    // destination's collision has streamed in.
    GroundSnapTarget = FVector(Plan.arrival.x, Plan.arrival.y, Plan.arrivalZ + 150.0);
    GroundSnapWait = 0;
    bPendingGroundSnap = true;
    const FRotator Facing(0, Plan.arrivalYaw, 0);
    Avatar->SetActorRotation(Facing);
    if (bBookOpen) CloseBook();
    SetControlRotation(FRotator(-12, Plan.arrivalYaw, 0));
    Avatar->SetRestingViewRotation(GetControlRotation());
    Notify(Result);
    return true;
}
