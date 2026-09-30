// The fingerposts on the public road (HomesteadRoadSign): she walks up to one and it offers the
// Map tab's walk, through the same confirm popup and Simulation::WalkRoad transaction.
#include "HomesteadController.h"

#include "HomesteadRoadSign.h"
#include "Simulation/HomesteadEstatePublicRoad.h"
#include "Simulation/HomesteadTravel.h"
#include "UI/SHomesteadMenu.h"

#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogHomesteadRoadSign, Log, All);

void AHomesteadController::TickRoadSigns()
{
    // Only the fixed estate has the road; spawned once, from the road data.
    const bool bWanted = bWorldReady && State().fixedEstate;
    if (!bWanted)
    {
        for (const auto& Sign : RoadSigns) if (Sign) Sign->Destroy();
        RoadSigns.Reset();
        return;
    }
    if (!RoadSigns.IsEmpty() || !GetWorld()) return;
    for (const Homestead::PublicRoadSign& Sign : Homestead::EstatePublicRoad().signs)
    {
        const FString Words = UTF8_TO_TCHAR(Homestead::RoadSignLabel(Sign.name).c_str());
        if (Words.IsEmpty()) continue;
        FActorSpawnParameters Parameters;
        Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto* Actor = GetWorld()->SpawnActor<AHomesteadRoadSign>(Parameters);
        if (!Actor) continue;
        Actor->Place(UTF8_TO_TCHAR(Sign.name.c_str()), FVector(Sign.position.x, Sign.position.y, Sign.z), static_cast<float>(Sign.yaw), Words);
        RoadSigns.Add(Actor);
        UE_LOG(LogHomesteadRoadSign, Display, TEXT("ROAD_SIGN %s at (%.0f, %.0f) standIn=%d"),
            UTF8_TO_TCHAR(Sign.name.c_str()), Sign.position.x, Sign.position.y, Actor->IsStandIn() ? 1 : 0);
    }
}

void AHomesteadController::ConsiderRoadSignFocus(TFunctionRef<void(EFocus, int32, Homestead::Point)> Consider) const
{
    if (!State().fixedEstate) return;
    const auto& Signs = Homestead::EstatePublicRoad().signs;
    if (const Homestead::PublicRoadSign* Sign = Homestead::RoadSignNear(PlayerPoint()))
        Consider(EFocus::RoadSign, static_cast<int32>(Sign - Signs.data()), Sign->position);
}

FString AHomesteadController::RoadSignTitle() const
{
    const auto& Signs = Homestead::EstatePublicRoad().signs;
    if (!(FocusId >= 0 && FocusId < static_cast<int32>(Signs.size()))) return TEXT("Road sign");
    return FString(TEXT("Road sign  |  ")) + UTF8_TO_TCHAR(Homestead::RoadSignLabel(Signs[FocusId].name).c_str());
}

FString AHomesteadController::RoadSignActions() const
{
    const auto& Signs = Homestead::EstatePublicRoad().signs;
    if (!(FocusId >= 0 && FocusId < static_cast<int32>(Signs.size()))) return FString();
    const FString A = bGamepad ? TEXT("[A]") : TEXT("[E]");
    const auto Destinations = Homestead::RoadSignDestinations(Signs[FocusId].name);
    if (Destinations.size() != 1) return A + TEXT(" Choose a way");
    const Homestead::TravelPlan Plan = MenuPlanTravel(Destinations[0]);
    if (!Plan.ok) return UTF8_TO_TCHAR(Plan.error.c_str());
    return FString::Printf(TEXT("%s Walk to %s (about %s)"), *A, UTF8_TO_TCHAR(Homestead::TravelDestinationName(Destinations[0])),
        UTF8_TO_TCHAR(Homestead::FormatWalkDuration(Plan.gameHours).c_str()));
}

void AHomesteadController::InteractWithRoadSign()
{
    const auto& Signs = Homestead::EstatePublicRoad().signs;
    if (!(FocusId >= 0 && FocusId < static_cast<int32>(Signs.size()))) return;
    TArray<Homestead::TravelDestination> Destinations;
    for (const auto Destination : Homestead::RoadSignDestinations(Signs[FocusId].name)) Destinations.Add(Destination);
    if (Destinations.IsEmpty()) return;
    // A single way she can't walk refuses with the reason (MenuTravel's notice) and changes nothing:
    // no book, no pause.
    if (Destinations.Num() == 1 && (!CanSetOut() || !MenuPlanTravel(Destinations[0]).ok))
    {
        MenuTravel(Destinations[0], Sim.GetRevision());
        return;
    }
    // The same confirm the Map tab asks, over the field book's Map page (paused while she decides).
    OpenBook(7);
    if (NativeMenu) NativeMenu->OpenSignTravelPrompt(UTF8_TO_TCHAR(Homestead::RoadSignLabel(Signs[FocusId].name).c_str()), Destinations);
    // Nothing to decide after all: don't leave her in a book she didn't ask for.
    if (!NativeMenu || !NativeMenu->IsTravelPromptOpen()) CloseBook();
}
