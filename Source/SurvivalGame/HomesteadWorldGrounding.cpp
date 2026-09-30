// Soft ground cover drawn right on the soil (Jenny's playtest: some weeds floated above the ground).
// The runtime heightfield (HomesteadEstateTerrain) is the Estate Landscape's source of truth, but the
// Landscape she walks on can differ from it in places (Scripts/Terrain/README.md: regrades applied
// only to their own rectangle, strips at some tile edges), and a clump placed on the height at its
// centre hovers on its downhill side. So a weed, nettle or tall-grass clump sits on the lowest drawn
// ground under its footprint: the Landscape's own collision when that's loaded, else the heightfield,
// and it settles onto the Landscape once its collision streams in.
#include "HomesteadWorld.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogHomesteadGrounding, Log, All);

namespace HomesteadGrounding
{
// Samples across the clump: its centre and four points at this fraction of its half extents.
constexpr float FootprintReach = 0.7f;
// It may sink this far below the ground at its centre (uphill side into the soil), no further, so a
// clump on a steep bank never buries its foliage.
constexpr float MaxSinkCm = 10.0f;
// How far above and below the heightfield to look for the Landscape's surface.
constexpr float TraceReachCm = 1500.0f;
constexpr float RetrySeconds = 1.0f;
constexpr int32 RetriesPerTick = 48;
// About ten minutes of retries; the Landscape round her is always loaded long before that.
constexpr int32 MaxTries = 600;

bool IsLandscapeCollision(const UPrimitiveComponent* Component)
{
    // LandscapeHeightfieldCollisionComponent / LandscapeMeshCollisionComponent, without a Landscape
    // module dependency.
    return Component && Component->GetClass()->GetName().StartsWith(TEXT("Landscape"));
}
}

bool AHomesteadWorld::IsSoilGrounded(Homestead::ResourceKind Kind)
{
    return Kind == Homestead::ResourceKind::Weeds || Kind == Homestead::ResourceKind::Nettles
        || Kind == Homestead::ResourceKind::TallGrass;
}

float AHomesteadWorld::SoilHeight(FVector2D Centre, FVector2D Half, float Yaw, bool& bOnLandscape) const
{
    namespace G = HomesteadGrounding;
    bOnLandscape = true;
    UWorld* World = GetWorld();
    const FRotator Turn(0, Yaw, 0);
    const FVector2D Offsets[] = {{0, 0}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
    float Lowest = TNumericLimits<float>::Max();
    float CentreZ = 0.0f;
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Offsets); ++Index)
    {
        const FVector Local(Offsets[Index].X * Half.X * G::FootprintReach, Offsets[Index].Y * Half.Y * G::FootprintReach, 0);
        const FVector Point = FVector(Centre, 0) + Turn.RotateVector(Local);
        float Z = GroundHeight(Point.X, Point.Y);
        bool bTraced = false;
        if (World && FMath::IsFinite(Z))
        {
            TArray<FHitResult> Hits;
            FCollisionQueryParams Query(SCENE_QUERY_STAT(HomesteadSoilHeight), true);
            World->LineTraceMultiByObjectType(Hits, FVector(Point.X, Point.Y, Z + G::TraceReachCm),
                FVector(Point.X, Point.Y, Z - G::TraceReachCm), FCollisionObjectQueryParams(ECC_WorldStatic), Query);
            float Best = TNumericLimits<float>::Max();
            for (const FHitResult& Hit : Hits)
                if (G::IsLandscapeCollision(Hit.GetComponent())
                    && FMath::Abs(Hit.ImpactPoint.Z - Z) < FMath::Abs(Best - Z))
                {
                    Best = static_cast<float>(Hit.ImpactPoint.Z);
                    bTraced = true;
                }
            if (bTraced) Z = Best;
        }
        bOnLandscape = bOnLandscape && bTraced;
        if (Index == 0) CentreZ = Z;
        if (FMath::IsFinite(Z)) Lowest = FMath::Min(Lowest, Z);
    }
    if (!FMath::IsFinite(CentreZ)) return CentreZ;
    return FMath::Max(Lowest, CentreZ - G::MaxSinkCm);
}

void AHomesteadWorld::QueueSoilGrounding(USceneComponent* Part, FVector2D Centre, FVector2D Half, float Yaw, float PlacedOn)
{
    if (!Part) return;
    FPendingSoil& Pending = PendingSoil.AddDefaulted_GetRef();
    Pending.Part = Part;
    Pending.Centre = Centre;
    Pending.Half = Half;
    Pending.Yaw = Yaw;
    Pending.PlacedOn = PlacedOn;
}

void AHomesteadWorld::UpdateSoilGrounding(float DeltaSeconds)
{
    namespace G = HomesteadGrounding;
    if (PendingSoil.IsEmpty()) return;
    SoilRetryTimer -= DeltaSeconds;
    if (SoilRetryTimer > 0.0f) return;
    SoilRetryTimer = G::RetrySeconds;
    // A bounded slice per retry, walking round the list so every clump gets its turn.
    int32 Checked = 0;
    while (Checked < G::RetriesPerTick && !PendingSoil.IsEmpty())
    {
        const int32 Index = SoilRetryCursor % PendingSoil.Num();
        FPendingSoil& Pending = PendingSoil[Index];
        USceneComponent* Part = Pending.Part.Get();
        ++Checked;
        if (!Part) { PendingSoil.RemoveAtSwap(Index); continue; }
        bool bOnLandscape = false;
        const float SoilZ = SoilHeight(Pending.Centre, Pending.Half, Pending.Yaw, bOnLandscape);
        if (bOnLandscape && FMath::IsFinite(SoilZ))
        {
            if (!FMath::IsNearlyEqual(SoilZ, Pending.PlacedOn, 0.5f))
                Part->AddWorldOffset(FVector(0, 0, SoilZ - Pending.PlacedOn));
            PendingSoil.RemoveAtSwap(Index);
        }
        else if (++Pending.Tries >= G::MaxTries)
        {
            UE_LOG(LogHomesteadGrounding, Verbose, TEXT("No Landscape under ground cover at (%.0f, %.0f); keeping the heightfield."),
                Pending.Centre.X, Pending.Centre.Y);
            PendingSoil.RemoveAtSwap(Index);
        }
        else ++SoilRetryCursor;
    }
}
