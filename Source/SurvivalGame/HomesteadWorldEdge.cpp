// The playable area's edge (shrink-estate-map decision 7): invisible pawn-only walls along the four sides of the
// layout's PlayableBounds rectangle, standing behind dense wood, hedge or sea, and the shut field gate with its
// milestone where the public road ends inside the sheet (AHomesteadRoadEndGate). The rectangle and the gate's
// place come from the layout (Simulation/HomesteadPlayableBounds.h), never from numbers here. Nothing is game state.
#include "HomesteadWorld.h"
#include "HomesteadWorldLog.h"
#include "HomesteadRoadEndGate.h"
#include "Simulation/HomesteadEstate.h"
#include "Simulation/HomesteadEstatePublicRoad.h"
#include "Simulation/HomesteadPlayableBounds.h"

#include "Components/BoxComponent.h"
#include "Engine/World.h"

namespace HomesteadWorldEdgeStyle
{
// estate_layout.json seaLevel is 0 cm; the walls reach well below the sea floor at the edge and far above any
// hill, so nothing she can stand on gets round them.
constexpr double SeaLevelCm = 0.0;
constexpr double WallBottomCm = SeaLevelCm - 6000.0;
constexpr double WallTopCm = SeaLevelCm + 40000.0;
constexpr double WallHalfThickCm = 50.0;
}

void AHomesteadWorld::BuildWorldEdge()
{
    using namespace HomesteadWorldEdgeStyle;
    if (bWorldEdgeBuilt) return;
    bWorldEdgeBuilt = true;
    const Homestead::EstateLayout& Layout = Homestead::ProvisionalEstateLayout();

    const Homestead::PlayableRect Rect = Homestead::EstatePlayableRect(Layout);
    int32 Walls = 0;
    if (Rect.valid)
    {
        // Each wall sits just outside its side and overlaps its neighbours at the corners.
        const double HalfZ = (WallTopCm - WallBottomCm) * 0.5, MidZ = (WallTopCm + WallBottomCm) * 0.5;
        const double MidX = (Rect.minX + Rect.maxX) * 0.5, MidY = (Rect.minY + Rect.maxY) * 0.5;
        const double HalfX = (Rect.maxX - Rect.minX) * 0.5 + 2.0 * WallHalfThickCm;
        const double HalfY = (Rect.maxY - Rect.minY) * 0.5 + 2.0 * WallHalfThickCm;
        const FVector Sides[4][2] = {
            {FVector(Rect.maxX + WallHalfThickCm, MidY, MidZ), FVector(WallHalfThickCm, HalfY, HalfZ)}, // north
            {FVector(Rect.minX - WallHalfThickCm, MidY, MidZ), FVector(WallHalfThickCm, HalfY, HalfZ)}, // south
            {FVector(MidX, Rect.maxY + WallHalfThickCm, MidZ), FVector(HalfX, WallHalfThickCm, HalfZ)}, // east
            {FVector(MidX, Rect.minY - WallHalfThickCm, MidZ), FVector(HalfX, WallHalfThickCm, HalfZ)}, // west
        };
        for (const FVector (&Side)[2] : Sides)
        {
            UBoxComponent* Wall = NewObject<UBoxComponent>(this, NAME_None, RF_Transient);
            Wall->SetupAttachment(GetRootComponent());
            Wall->SetMobility(EComponentMobility::Movable);
            Wall->SetBoxExtent(Side[1], false);
            Wall->SetWorldLocationAndRotation(Side[0], FRotator::ZeroRotator);
            Wall->SetCollisionObjectType(ECC_WorldStatic);
            Wall->SetCollisionResponseToAllChannels(ECR_Ignore);
            Wall->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
            Wall->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            Wall->CanCharacterStepUpOn = ECB_No;
            Wall->SetGenerateOverlapEvents(false);
            Wall->SetCanEverAffectNavigation(false);
            Wall->SetHiddenInGame(true);
            Wall->SetVisibility(false);
            Wall->RegisterComponent();
            WorldEdgeVisual.Components.Add(Wall);
            ++Walls;
        }
    }
    else UE_LOG(LogHomesteadWorld, Warning, TEXT("World edge: the layout has no %s rectangle; no edge walls."),
        UTF8_TO_TCHAR(Homestead::Anchor::PlayableBounds));

    const Homestead::RoadEndGate Gate = Homestead::EstateRoadEndGate(Layout, Homestead::EstatePublicRoad().points);
    if (Gate.valid && GetWorld())
    {
        FActorSpawnParameters Parameters;
        Parameters.Owner = this;
        Parameters.ObjectFlags |= RF_Transient;
        Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        RoadEndGate = GetWorld()->SpawnActor<AHomesteadRoadEndGate>(Parameters);
        if (RoadEndGate)
            RoadEndGate->Place(FVector2D(Gate.position.x, Gate.position.y), static_cast<float>(Gate.yaw));
    }
    UE_LOG(LogHomesteadWorld, Log, TEXT("World edge: %d walls round (%.0f..%.0f, %.0f..%.0f); road-end gate %s."), Walls,
        Rect.minX, Rect.maxX, Rect.minY, Rect.maxY,
        !RoadEndGate ? TEXT("not placed") : Gate.fromLandmark ? TEXT("at RoadEndGate") : TEXT("at the road's end"));
}
