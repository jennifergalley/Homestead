// Walking the public road between the manor and town (Map tab, and later the road signs): the
// preview from where she stands, and the walk itself. The whole walk is ordinary time in the
// Simulation (Simulation::WalkRoad): refused with nothing changed if she'd collapse or doze off on
// the way; otherwise the clock moves on and she's stood on the road at the far end.
#include "HomesteadController.h"

#include "HomesteadCharacter.h"
#include "Simulation/HomesteadTravel.h"

DEFINE_LOG_CATEGORY_STATIC(LogHomesteadTravel, Log, All);

bool AHomesteadController::CanSetOut() const
{
    return Cast<AHomesteadCharacter>(GetPawn()) && bWorldReady && !bPendingSpawn && !bPendingGroundSnap;
}

void AHomesteadController::TickTravelDiscovery()
{
    if (!CanSetOut() || !State().fixedEstate || IsFailed() || bBookOpen || bPlanning
        || HasNativeMenu() || IsNewGameSetup() || IsNamingSetup() || ShopScreen.IsValid()) return;
    const Homestead::Point At = PlayerPoint();
    for (int32 Index = 1; Index < Homestead::TravelDestinationCount; ++Index)
    {
        const auto Destination = static_cast<Homestead::TravelDestination>(Index);
        if (Homestead::IsTravelUnlocked(State(), Destination) || !Homestead::TravelVisitNear(At, Destination, Sim.Layout())) continue;
        const auto Result = Sim.DiscoverTravel(Destination, At);
        if (!Result.ok) { Notify(Result); return; }
        PendingTravelNotices.Add(Destination);
        UE_LOG(LogHomesteadTravel, Log, TEXT("%s"), UTF8_TO_TCHAR(Result.message.c_str()));
    }
    // Town and its store may be discovered together. Keep both notices instead of replacing one.
    while (ToastRemaining <= 0.0f && !PendingTravelNotices.IsEmpty())
    {
        const auto Destination = PendingTravelNotices[0];
        PendingTravelNotices.RemoveAt(0);
        if (Homestead::IsTravelUnlocked(State(), Destination))
            Notify(FString::Printf(TEXT("Fast travel unlocked: %s"),
                UTF8_TO_TCHAR(Homestead::TravelDestinationName(Destination))), false);
    }
}

Homestead::TravelPlan AHomesteadController::MenuPlanTravel(Homestead::TravelDestination Destination) const
{
    return Homestead::PlanTravel(State(), PlayerPoint(), Destination, Sim.Layout());
}

bool AHomesteadController::MenuTravel(Homestead::TravelDestination Destination, uint64 ExpectedRevision)
{
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    if (!Avatar || !CanSetOut())
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
    // The Estate's world actor is prepared here; the destination Landscape may still need to stream.
    if (!PrepareWorldAt(Plan.arrival)) return false;
    TUniquePtr<Homestead::Simulation> BeforeWalk = MakeUnique<Homestead::Simulation>(Sim);
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
    BeginGroundSnap(FVector(Plan.arrival.x, Plan.arrival.y, Plan.arrivalZ + 150.0));
    if (!bPendingGroundSnap)
    {
        Sim = MoveTemp(*BeforeWalk);
        RefreshRemaining = 0;
        return false;
    }
    GroundSnapTravelBefore = MoveTemp(BeforeWalk);
    const FRotator Facing(0, Plan.arrivalYaw, 0);
    Avatar->SetActorRotation(Facing);
    if (bBookOpen) CloseBook();
    SetControlRotation(FRotator(-12, Plan.arrivalYaw, 0));
    Avatar->SetRestingViewRotation(GetControlRotation());
    Notify(Result);
    return true;
}
