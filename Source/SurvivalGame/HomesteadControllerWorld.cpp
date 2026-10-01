#include "HomesteadController.h"
#include "HomesteadControllerText.h"
#include "HomesteadCharacter.h"
#include "HomesteadWorld.h"
#include "HomesteadEstateTerrain.h"
#include "Simulation/HomesteadEstate.h"
#include "Simulation/HomesteadEstatePublicRoad.h"

#include "Engine/World.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/WorldPartitionStreamingSourceComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/PlatformTime.h"

using HomesteadControllerText::Text;

DEFINE_LOG_CATEGORY_STATIC(LogHomesteadGroundSnap, Log, All);

namespace HomesteadGroundSnap
{
constexpr double ReportIntervalSeconds = 10.0;
// Reach the neighbouring Landscape proxy at a cell edge without requesting the whole Estate.
constexpr float SourceRadiusCm = 18000.0f;

bool HasLandscapeCollision(UWorld* World, const FVector& Start, const FVector& End, const APawn* Avatar)
{
    TArray<FHitResult> Hits;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(HomesteadLandscapeGround), false, Avatar);
    World->LineTraceMultiByObjectType(Hits, Start, End, FCollisionObjectQueryParams(ECC_WorldStatic), Params);
    for (const FHitResult& Hit : Hits)
        if (const UPrimitiveComponent* Component = Hit.GetComponent();
            Component && Component->GetClass()->GetName().StartsWith(TEXT("Landscape"))
                && Component->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block)
            return true;
    return false;
}
}

namespace HomesteadSettleTuning
{
constexpr float BridgeSettleMarginCm = 30.0f;     // past her capsule: the deck counts as underfoot this far out
constexpr float BridgeSettleHoldSeconds = 5.0f;   // wait this long for the bridge's walking slab before giving up on it
}

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
    const float Radius = Capsule ? Capsule->GetScaledCapsuleRadius() : 34.0f;
    // Over the road bridge (a save from the old ford, or a teleport onto it) the probe starts above the deck's
    // walking slab and she stands on it: from under the slab the trace would miss it and stand her in it.
    const Homestead::PublicRoadBridge& Deck = Homestead::EstatePublicRoad().deck;
    const bool bOnBridge = bEstateMap && Deck.Covers({Target.X, Target.Y}, Radius + HomesteadSettleTuning::BridgeSettleMarginCm);
    if (bOnBridge)
    {
        const Homestead::Point On = Deck.OntoDeck({Target.X, Target.Y}, Radius + 5.0f);
        Target.X = static_cast<float>(On.x);
        Target.Y = static_cast<float>(On.y);
        Target.Z = static_cast<float>(Deck.ProbeStartZ(On, Target.Z, HalfHeight + HomesteadSettleTuning::BridgeSettleMarginCm, 0.0));
    }
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
    const FVector End(Target.X, Target.Y, FMath::Min(Target.Z, Ground) - 500.0f);
    bool bFound = GetWorld()->LineTraceSingleByChannel(Hit, Target, End, Channel, Params, Responses);
    // A pending snap never accepts the riverbed as the bridge deck; rollback is safer.
    if (bOnBridge && bFound && Hit.ImpactPoint.Z < Deck.deckZ - 10.0
        && (bPendingGroundSnap || Waited < HomesteadSettleTuning::BridgeSettleHoldSeconds)) bFound = false;
    const bool bLandscapeReady = !bEstateMap || HomesteadGroundSnap::HasLandscapeCollision(GetWorld(), Target, End, Avatar);
    if ((!bFound || !bLandscapeReady) && Waited < HoldLimitSeconds)
    {
        const double Now = FPlatformTime::Seconds();
        if (Waited <= 0 || (bPendingGroundSnap && Now - GroundSnapLastReportAt >= HomesteadGroundSnap::ReportIntervalSeconds))
        {
            UE_LOG(LogHomesteadGroundSnap, Display,
                TEXT("HOMESTEAD_GROUND_HOLD %s: floor=%d landscape=%d source=%d elapsed=%.1f s at (%.0f, %.0f)"),
                Why, bFound, bLandscapeReady,
                GroundSnapStreamingSource ? GroundSnapStreamingSource->IsStreamingCompleted() : -1,
                bPendingGroundSnap ? Now - GroundSnapStartedAt : static_cast<double>(Waited), Target.X, Target.Y);
            GroundSnapLastReportAt = Now;
        }
        Waited += DeltaSeconds;
        if (Movement)
        {
            Movement->StopMovementImmediately();
            if (Movement->MovementMode != MOVE_None) Movement->DisableMovement();
        }
        const float HoldZ = bOnBridge ? Target.Z : FMath::Min(Target.Z, Ground + HalfHeight + 2.0f);
        const FVector HoldAt = bPendingGroundSnap ? GroundSnapSafePosition
            : FVector(Target.X, Target.Y, HoldZ);
        Avatar->SetActorLocation(HoldAt,
            false, nullptr, ETeleportType::TeleportPhysics);
        return false;
    }
    const float Floor = bFound ? Hit.ImpactPoint.Z : bOnBridge ? static_cast<float>(Deck.deckZ) : Ground;
    if (!bFound || !bLandscapeReady)
    {
        if (bPendingGroundSnap && bEstateMap) return false;
        UE_LOG(LogHomesteadGroundSnap, Warning,
            TEXT("HOMESTEAD_GROUND_HOLD %s: gave up after %.1f game s at (%.0f, %.0f); placing on the %s"),
            Why, Waited, Target.X, Target.Y, bOnBridge ? TEXT("bridge deck") : TEXT("heightfield"));
    }
    UE_LOG(LogHomesteadGroundSnap, Display, TEXT("HOMESTEAD_GROUND_SETTLE %s: held %.1f s; feet at %.0f (heightfield %.0f) at (%.0f, %.0f)"),
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

void AHomesteadController::EndGroundSnap()
{
    bPendingGroundSnap = false;
    GroundSnapWait = 0;
    GroundSnapStartedAt = 0;
    GroundSnapLastReportAt = 0;
    GroundSnapLastInputNoticeAt = -1000.0;
    GroundSnapTravelBefore.Reset();
    GroundSnapStreamingSource = nullptr;
    if (IsValid(GroundSnapStreamingActor))
    {
        GroundSnapStreamingActor->Destroy();
    }
    GroundSnapStreamingActor = nullptr;
}

bool AHomesteadController::RejectPendingGroundSnapAction()
{
    if (!bPendingGroundSnap) return false;
    const double Now = FPlatformTime::Seconds();
    if (Now - GroundSnapLastInputNoticeAt >= 2.0)
    {
        GroundSnapLastInputNoticeAt = Now;
        Notify(TEXT("Still finding your footing. Wait a moment."), true);
    }
    return true;
}

void AHomesteadController::BeginGroundSnap(FVector Target)
{
    if (bPendingGroundSnap)
    {
        UE_LOG(LogHomesteadGroundSnap, Warning, TEXT("Refusing another ground snap while one is pending."));
        Notify(TEXT("Wait until she is safely on the ground before setting out again."), true);
        return;
    }
    if (Target.ContainsNaN())
    {
        UE_LOG(LogHomesteadGroundSnap, Error, TEXT("Refusing ground snap to a non-finite destination."));
        Notify(TEXT("The destination is invalid. You are still where you started."), true);
        return;
    }
    const FVector SafePosition = LastSafeWorldPosition;
    if (bBookOpen) CloseBook();
    if (ShopScreen.IsValid()) CloseShopScreen();
    EndGroundSnap();
    GroundSnapTarget = Target;
    GroundSnapSafePosition = SafePosition;
    GroundSnapSafeRotation = GetControlRotation();
    GroundSnapSafeActorRotation = GetPawn() ? GetPawn()->GetActorRotation() : FRotator::ZeroRotator;
    GroundSnapStartedAt = FPlatformTime::Seconds();
    bPendingGroundSnap = true;
    bControlDown = false;
    bSprintShiftDown = false;
    bSprintShiftModifier = false;
    FlushPressedKeys();
    if (!bEstateMap) return;

    if (APawn* Avatar = GetPawn())
    {
        Avatar->SetActorLocation(SafePosition, false, nullptr, ETeleportType::TeleportPhysics);
        if (AHomesteadCharacter* Heroine = Cast<AHomesteadCharacter>(Avatar))
            Heroine->ResetSprint();
        if (ACharacter* Body = Cast<ACharacter>(Avatar))
        {
            Body->GetCharacterMovement()->StopMovementImmediately();
            Body->GetCharacterMovement()->DisableMovement();
        }
    }

    FActorSpawnParameters Spawn;
    Spawn.ObjectFlags |= RF_Transient;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AActor* SourceActor = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), Target, FRotator::ZeroRotator, Spawn);
    if (!SourceActor)
    {
        UE_LOG(LogHomesteadGroundSnap, Error, TEXT("Could not request World Partition streaming at (%.0f, %.0f)."), Target.X, Target.Y);
        Notify(TEXT("The destination could not be prepared. Your position will be restored."), true);
        return;
    }
    GroundSnapStreamingActor = SourceActor;
    USceneComponent* Anchor = NewObject<USceneComponent>(SourceActor);
    SourceActor->AddInstanceComponent(Anchor);
    SourceActor->SetRootComponent(Anchor);
    Anchor->RegisterComponent();
    SourceActor->SetActorLocation(Target);
    UWorldPartitionStreamingSourceComponent* Source = NewObject<UWorldPartitionStreamingSourceComponent>(SourceActor);
    Source->Priority = EStreamingSourcePriority::Highest;
    Source->TargetState = EStreamingSourceTargetState::Activated;
    FStreamingSourceShape Shape;
    Shape.bUseGridLoadingRange = false;
    Shape.Radius = HomesteadGroundSnap::SourceRadiusCm;
    Source->Shapes.Add(Shape);
    SourceActor->AddInstanceComponent(Source);
    Source->RegisterComponent();
    GroundSnapStreamingSource = Source;
}

void AHomesteadController::AbortGroundSnap()
{
    UE_LOG(LogHomesteadGroundSnap, Error, TEXT("HOMESTEAD_GROUND_TIMEOUT: no landscape collision after %.1f real s at (%.0f, %.0f); restoring (%.0f, %.0f)."),
        FPlatformTime::Seconds() - GroundSnapStartedAt, GroundSnapTarget.X, GroundSnapTarget.Y,
        GroundSnapSafePosition.X, GroundSnapSafePosition.Y);
    const FVector SafePosition = GroundSnapSafePosition;
    const FRotator SafeRotation = GroundSnapSafeRotation;
    const FRotator SafeActorRotation = GroundSnapSafeActorRotation;
    TUniquePtr<Homestead::Simulation> BeforeTravel = MoveTemp(GroundSnapTravelBefore);
    EndGroundSnap();
    FString EquipmentError;
    bool bEquipmentRestored = true;
    if (BeforeTravel)
    {
        Sim = MoveTemp(*BeforeTravel);
        RefreshRemaining = 0;
        if (AHomesteadCharacter* Heroine = Cast<AHomesteadCharacter>(GetPawn()))
            if (!Heroine->PrepareEquipment(State(), Appearance, EquipmentError)
                || !Heroine->ApplyPreparedEquipment(EquipmentError))
            {
                Heroine->ClearPreparedEquipment();
                bEquipmentRestored = false;
            }
    }
    if (!bEquipmentRestored && EquipmentError.IsEmpty())
        EquipmentError = TEXT("The owned clothing could not be displayed.");
    if (APawn* Avatar = GetPawn())
    {
        Avatar->SetActorLocation(SafePosition, false, nullptr, ETeleportType::TeleportPhysics);
        Avatar->SetActorRotation(SafeActorRotation);
        if (ACharacter* Body = Cast<ACharacter>(Avatar))
        {
            Body->GetCharacterMovement()->StopMovementImmediately();
            Body->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        }
        if (AHomesteadCharacter* Heroine = Cast<AHomesteadCharacter>(Avatar))
        {
            Heroine->SetRestingViewRotation(SafeRotation);
            Heroine->SnapCamera();
        }
    }
    SetControlRotation(SafeRotation);
    LastStepPosition = SafePosition;
    LastSafeWorldPosition = SafePosition;
    StepDistance = 0;
    Notify(!bEquipmentRestored
        ? TEXT("Your walk was canceled, but her clothing could not be restored: ") + EquipmentError
        : BeforeTravel
            ? TEXT("The ground there did not load. Your walk was canceled without spending time.")
            : TEXT("The ground there did not load. You are back where you started."), true);
}

void AHomesteadController::HomesteadTeleport(float X, float Y, float Z)
{
    if (!GetPawn()) return;
    if (bPendingGroundSnap)
    {
        UE_LOG(LogHomesteadGroundSnap, Warning, TEXT("Refusing teleport while the previous destination is still streaming."));
        Notify(TEXT("Wait until she is safely on the ground before teleporting again."), true);
        return;
    }
    const bool bOnTerrain = Z <= -100000.0f;
    BeginGroundSnap(FVector(X, Y, bOnTerrain ? GroundHeight(X, Y) + 150.0f : Z + 100.0f));
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
