#include "HomesteadController.h"
#include "HomesteadControllerText.h"
#include "HomesteadCharacter.h"
#include "HomesteadWorld.h"
#include "HomesteadEstateTerrain.h"
#include "Simulation/HomesteadEstate.h"

#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

using HomesteadControllerText::Text;

Homestead::Point AHomesteadController::PlayerPoint() const
{
    const FVector Position = GetPawn() ? GetPawn()->GetActorLocation() : PendingLocation;
    return { Position.X, Position.Y };
}

float AHomesteadController::GroundHeight(float X, float Y) const
{
    return AHomesteadWorld::GroundHeight(X, Y, State().world);
}

void AHomesteadController::PrepareEstateSimulation(Homestead::Simulation& Target) const
{
    Target.SetLayout(Homestead::ProvisionalEstateLayout());
    Target.SetPlacements(Homestead::ProvisionalEstatePlacements());
    const TWeakObjectPtr<const AHomesteadController> Self(this);
    Target.SetWaterProbe([Self](Homestead::Point Position)
    {
        // Only fresh water counts for the pail; the sea is salt.
        return Self.IsValid() && Self->WaterEdgeDistance(Position, false) <= 120.0;
    });
}

void AHomesteadController::SetEstateSpawn()
{
    const Homestead::EstateLayout& Layout = Sim.Layout();
    const Homestead::Landmark* Spawn = Layout.FindLandmark(Homestead::Anchor::StandingRoomSpawn);
    const Homestead::Point At = Spawn ? Spawn->position : Homestead::Point{};
    PendingLocation = FVector(At.x, At.y, GroundHeight(At.x, At.y) + 100.0f);
    PendingRotation = FRotator(-12.0f, Spawn ? Spawn->yaw : 0.0f, 0.0f);
    LastSafeWorldPosition = PendingLocation;
    bFreshTerrainSpawn = true;
    EstateSpawnWait = 0;
}

bool AHomesteadController::SettleOnGround(FVector& Target, float& Waited, float DeltaSeconds, float HoldLimitSeconds, const TCHAR* Why)
{
    APawn* Avatar = GetPawn();
    if (!Avatar) return false;
    ACharacter* Body = Cast<ACharacter>(Avatar);
    UCapsuleComponent* Capsule = Body ? Body->GetCapsuleComponent() : nullptr;
    UCharacterMovementComponent* Movement = Body ? Body->GetCharacterMovement() : nullptr;
    const float HalfHeight = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 90.0f;
    const float Ground = GroundHeight(Target.X, Target.Y);
    // Trace for exactly what her capsule collides with, down to well below the terrain.
    FCollisionQueryParams Params(SCENE_QUERY_STAT(HomesteadSettleOnGround), false, Avatar);
    FCollisionResponseParams Responses;
    ECollisionChannel Channel = ECC_Pawn;
    if (Capsule)
    {
        Capsule->InitSweepCollisionParams(Params, Responses);
        Channel = Capsule->GetCollisionObjectType();
    }
    FHitResult Hit;
    const bool bFound = GetWorld()->LineTraceSingleByChannel(Hit, Target,
        FVector(Target.X, Target.Y, FMath::Min(Target.Z, Ground) - 500.0f), Channel, Params, Responses);
    if (!bFound && Waited < HoldLimitSeconds)
    {
        // Hold her where the terrain will be, not falling, until its collision arrives.
        if (Waited <= 0) UE_LOG(LogTemp, Display, TEXT("HOMESTEAD_GROUND_HOLD %s: no collision yet at (%.0f, %.0f); holding"), Why, Target.X, Target.Y);
        Waited += DeltaSeconds;
        if (Movement)
        {
            Movement->StopMovementImmediately();
            if (Movement->MovementMode != MOVE_None) Movement->DisableMovement();
        }
        Avatar->SetActorLocation(FVector(Target.X, Target.Y, FMath::Min(Target.Z, Ground + HalfHeight + 2.0f)),
            false, nullptr, ETeleportType::TeleportPhysics);
        return false;
    }
    const float Floor = bFound ? Hit.ImpactPoint.Z : Ground;
    if (!bFound)
        UE_LOG(LogTemp, Warning, TEXT("HOMESTEAD_GROUND_HOLD %s: gave up after %.1f s at (%.0f, %.0f); placing on the heightfield"), Why, Waited, Target.X, Target.Y);
    UE_LOG(LogTemp, Display, TEXT("HOMESTEAD_GROUND_SETTLE %s: held %.1f s; feet at %.0f (heightfield %.0f) at (%.0f, %.0f)"),
        Why, Waited, Floor, Ground, Target.X, Target.Y);
    Target.Z = Floor + HalfHeight + 2.0f;
    Waited = 0;
    if (Movement)
    {
        Movement->StopMovementImmediately();
        Movement->SetMovementMode(MOVE_Walking);
    }
    return true;
}

void AHomesteadController::HomesteadTeleport(float X, float Y, float Z)
{
    if (!GetPawn()) return;
    const bool bOnTerrain = Z <= -100000.0f;
    GroundSnapTarget = FVector(X, Y, bOnTerrain ? GroundHeight(X, Y) + 150.0f : Z + 100.0f);
    GroundSnapWait = 0;
    bPendingGroundSnap = true;
    if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn()))
    {
        Avatar->CancelAction(true);
        Avatar->ResetSprint();
    }
}

bool AHomesteadController::CollectPreparedBaselines(Homestead::Generation::ChunkCoord Chunk,
    std::array<const Homestead::Generation::ChunkBaseline*, 9>& Prepared) const
{
    bool bComplete = true;
    for (int32 Y = -1; Y <= 1; ++Y)
        for (int32 X = -1; X <= 1; ++X)
        {
            const int32 Index = (Y + 1) * 3 + X + 1;
            Prepared[Index] = Landscape ? Landscape->CachedBaselineFor(
                State().world, {Chunk.x + X, Chunk.y + Y}) : nullptr;
            bComplete &= Prepared[Index] != nullptr;
        }
    return bComplete;
}

bool AHomesteadController::PrepareWorldAt(Homestead::Point Position)
{
    if (State().fixedEstate)
    {
        // The Estate level streams itself; only the first publication needs the world actor.
        if (bWorldReady && Landscape && Landscape->IsPreparedFor(State())) return true;
        if (!Landscape || !Landscape->Refresh(Sim))
        {
            bWorldReady = false;
            Notify(TEXT("The estate could not be prepared. Movement stopped; existing saves are untouched."), true);
            return false;
        }
        bWorldReady = true;
        return true;
    }
    Homestead::Generation::ChunkCoord Chunk;
    if (!FMath::IsFinite(Position.x) || !FMath::IsFinite(Position.y)
        || FMath::Abs(Position.x) > Homestead::MaxWorldCoordinate
        || FMath::Abs(Position.y) > Homestead::MaxWorldCoordinate
        || Homestead::Generation::ChunkAt(FMath::FloorToInt64(Position.x), FMath::FloorToInt64(Position.y), Chunk)
            != Homestead::Generation::Status::Ok)
    {
        Notify(TEXT("This position exceeds the supported 10 km coordinate range. Your world changes are retained."), true);
        return false;
    }
    if (bWorldReady && State().activeChunk == Chunk && Landscape && Landscape->IsPreparedFor(State())) return true;
    const double PreparationStarted = FPlatformTime::Seconds();
    Homestead::Simulation Candidate = Sim;
    Homestead::PreparedWorldRegion Prepared;
    Prepared.world = State().world;
    CollectPreparedBaselines(Chunk, Prepared.chunks);
    const auto Result = Candidate.SetActiveWorldRegion(Position, &Prepared);
    if (!Result) { Notify(Result); return false; }
    LastRegionSimulationMilliseconds = (FPlatformTime::Seconds() - PreparationStarted) * 1000;
    const double PublicationStarted = FPlatformTime::Seconds();
    if (!Landscape || !Landscape->Refresh(Candidate))
    {
        if (Landscape) Landscape->CancelStagedResources();
        bWorldReady = false;
        Notify(TEXT("The next woodland region could not be prepared. Movement stopped; existing saves are untouched."), true);
        return false;
    }
    LastRegionWorldMilliseconds = (FPlatformTime::Seconds() - PublicationStarted) * 1000;
    Sim = MoveTemp(Candidate);
    bWorldReady = true;
    Focus = EFocus::None;
    FocusId = -1;
    RefreshRemaining = 0;
    return true;
}

bool AHomesteadController::HasHeroine() const
{
    const auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    return Avatar && Avatar->HasHeroine();
}
