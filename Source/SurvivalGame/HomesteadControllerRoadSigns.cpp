// The fingerposts on the public road (HomesteadRoadSign): solid wooden signs that point the way to the village and
// the manor. They only point; walking there is the Map tab's.
#include "HomesteadController.h"

#include "HomesteadRoadSign.h"
#include "Simulation/HomesteadEstatePublicRoad.h"
#include "Simulation/HomesteadTravel.h"

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
        Actor->Place(UTF8_TO_TCHAR(Sign.name.c_str()), FVector(Sign.position.x, Sign.position.y, Sign.z), static_cast<float>(Homestead::RoadSignArmYaw(Sign)), Words);
        RoadSigns.Add(Actor);
        UE_LOG(LogHomesteadRoadSign, Display, TEXT("ROAD_SIGN %s at (%.0f, %.0f) standIn=%d"),
            UTF8_TO_TCHAR(Sign.name.c_str()), Sign.position.x, Sign.position.y, Actor->IsStandIn() ? 1 : 0);
    }
}
