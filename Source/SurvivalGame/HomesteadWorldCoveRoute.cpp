// The cove route's granite steps, landings, kerbs, oak rails and fingerposts (add-cove-route-kit), placed at
// runtime from Homestead::EstateCoveRouteKit() (the route Scripts/Terrain/cove_route.py designed and cut into
// the heightfield). Props' meshes import to /Game/SurvivalGame/Environment/Props/<Recipe>/ with the kit's
// pivots, so placing them is only transforms: treads, slabs and kerbs as instanced batches (their collision is
// in the meshes; kerbs refuse step-up), each rail bay its own component (a mirrored bay scales Y by -1), and a
// pawn-only box along every rail bay, since the rails carry no collision. Nothing here is game state.
#include "HomesteadWorld.h"
#include "HomesteadWorldLog.h"
#include "Simulation/HomesteadCoveRouteKit.h"

#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"

namespace HomesteadCoveRouteLook
{
struct FKitMesh
{
    const TCHAR* Folder;
    const TCHAR* Name;
};

// In Homestead::CoveKitPiece order (cove_steps.py, cove_kerb.py, cove_handrail.py, fingerpost.py).
const FKitMesh KitMeshes[] = {
    {TEXT("CoveSteps"), TEXT("SM_CoveStep_A")},
    {TEXT("CoveSteps"), TEXT("SM_CoveStep_B")},
    {TEXT("CoveSteps"), TEXT("SM_CoveStep_C")},
    {TEXT("CoveSteps"), TEXT("SM_CoveLandingSlab")},
    {TEXT("CoveKerb"), TEXT("SM_CoveKerb_Straight")},
    {TEXT("CoveRail"), TEXT("SM_CoveRail_Level")},
    {TEXT("CoveRail"), TEXT("SM_CoveRail_Rake26")},
    {TEXT("CoveRail"), TEXT("SM_CoveRail_Rake28")},
    {TEXT("CoveRail"), TEXT("SM_CoveRail_Rake30")},
    {TEXT("Fingerpost"), TEXT("SM_Fingerpost_ToTheCove")},
    {TEXT("CoveRail"), TEXT("SM_CoveRail_EndPost")},
    {TEXT("CoveSteps"), TEXT("SM_CoveLandingSlab75")},
    {TEXT("CoveSteps"), TEXT("SM_CoveLandingWedge")},
};
static_assert(UE_ARRAY_COUNT(KitMeshes) == static_cast<int32>(Homestead::CoveKitPiece::Count), "a mesh per kit piece");

bool IsRail(Homestead::CoveKitPiece Piece)
{
    return Piece == Homestead::CoveKitPiece::RailLevel || Piece == Homestead::CoveKitPiece::Rail26
        || Piece == Homestead::CoveKitPiece::Rail28 || Piece == Homestead::CoveKitPiece::Rail30;
}
}

void AHomesteadWorld::BuildCoveRoute()
{
    using namespace HomesteadCoveRouteLook;
    if (bCoveRouteBuilt) return;
    bCoveRouteBuilt = true;
    const Homestead::CoveKitLayout& Kit = Homestead::EstateCoveRouteKit();
    if (Kit.pieces.empty()) return;

    // Each of Props' groups (steps, kerbs, rails, fingerposts) goes down whole or not at all, independently of
    // the others (Homestead::CoveKitPlaceableGroups): the steps don't wait on a parked fingerpost.
    UStaticMesh* Meshes[UE_ARRAY_COUNT(KitMeshes)] = {};
    Homestead::CoveKitMeshesLoaded Loaded{};
    TArray<FString> Missing;
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(KitMeshes); ++Index)
    {
        const FString Path = FString::Printf(TEXT("/Game/SurvivalGame/Environment/Props/%s/%s.%s"),
            KitMeshes[Index].Folder, KitMeshes[Index].Name, KitMeshes[Index].Name);
        Meshes[Index] = LoadObject<UStaticMesh>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
        Loaded[Index] = Meshes[Index] != nullptr;
        if (!Meshes[Index]) Missing.Add(KitMeshes[Index].Name);
    }
    const Homestead::CoveKitGroupsPlaced Groups = Homestead::CoveKitPlaceableGroups(Loaded);
    TArray<FString> Skipped;
    for (int32 Group = 0; Group < static_cast<int32>(Homestead::CoveKitGroup::Count); ++Group)
        if (!Groups[Group]) Skipped.Add(UTF8_TO_TCHAR(Homestead::CoveKitGroupName(static_cast<Homestead::CoveKitGroup>(Group))));
    if (!Skipped.IsEmpty())
        UE_LOG(LogHomesteadWorld, Log, TEXT("Cove route: not placing %s (missing %s)."),
            *FString::Join(Skipped, TEXT(", ")), *FString::Join(Missing, TEXT(", ")));
    if (Skipped.Num() == static_cast<int32>(Homestead::CoveKitGroup::Count)) return;

    auto Transform = [](const Homestead::CoveKitPlacement& Piece)
    {
        return FTransform(FRotator(0.0f, static_cast<float>(Piece.yaw), 0.0f),
            FVector(Piece.position.x, Piece.position.y, Piece.z),
            FVector(Piece.scaleX, Piece.scaleY, Piece.scaleZ));
    };
    TMap<int32, TObjectPtr<UInstancedStaticMeshComponent>> Batches;
    int32 Placed = 0;
    int32 PlacedOf[static_cast<int32>(Homestead::CoveKitPiece::Count)] = {};
    int32 Blockers = 0;
    for (const Homestead::CoveKitPlacement& Piece : Kit.pieces)
    {
        const int32 Index = static_cast<int32>(Piece.piece);
        UStaticMesh* Mesh = Meshes[Index];
        if (!Groups[static_cast<int32>(Homestead::CoveKitGroupOf(Piece.piece))]) continue;
        ++PlacedOf[Index];
        // A mirrored wedge (a left-hand turn) has negative Y scale too.
        if (IsRail(Piece.piece) || Piece.piece == Homestead::CoveKitPiece::Fingerpost || Piece.piece == Homestead::CoveKitPiece::RailEndPost
            || Piece.piece == Homestead::CoveKitPiece::LandingWedge)
        {
            // Rails and fingerposts one component each: a mirrored rail bay's negative scale flips its winding,
            // which a component handles and an instance batch doesn't.
            UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);
            Part->SetupAttachment(GetRootComponent());
            Part->SetMobility(EComponentMobility::Movable);
            Part->SetStaticMesh(Mesh);
            Part->SetWorldTransform(Transform(Piece));
            if (IsRail(Piece.piece) || Piece.piece == Homestead::CoveKitPiece::RailEndPost)
            {
                Part->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
                Part->SetCanEverAffectNavigation(false);
            }
            else if (Piece.piece == Homestead::CoveKitPiece::LandingWedge)
            {
                Part->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
                Part->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
            }
            else
            {
                Part->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
                Part->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
                Part->CanCharacterStepUpOn = ECB_No;
            }
            Part->SetGenerateOverlapEvents(false);
            Part->RegisterComponent();
            CoveRouteVisual.Components.Add(Part);
            ++Placed;
            continue;
        }
        TObjectPtr<UInstancedStaticMeshComponent>& Batch = Batches.FindOrAdd(Index);
        if (!Batch)
        {
            Batch = NewObject<UInstancedStaticMeshComponent>(this, NAME_None, RF_Transient);
            Batch->SetupAttachment(GetRootComponent());
            Batch->SetMobility(EComponentMobility::Movable);
            Batch->SetStaticMesh(Mesh);
            // Treads and landings are walked on (a box per tread in the mesh); kerbs stop her without being
            // stepped onto; the camera passes through both.
            Batch->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
            Batch->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
            if (Piece.piece == Homestead::CoveKitPiece::Kerb) Batch->CanCharacterStepUpOn = ECB_No;
            Batch->SetGenerateOverlapEvents(false);
            Batch->RegisterComponent();
            CoveRouteVisual.Components.Add(Batch);
        }
        Batch->AddInstance(Transform(Piece), /*bWorldSpace=*/true);
        ++Placed;
    }
    for (const Homestead::CoveKitBlocker& Edge : Kit.blockers)
    {
        // The pawn blockers stand behind the rails: with no rails, no invisible walls.
        if (!Groups[static_cast<int32>(Homestead::CoveKitGroup::Rails)]) break;
        UBoxComponent* Blocker = NewObject<UBoxComponent>(this, NAME_None, RF_Transient);
        Blocker->SetupAttachment(GetRootComponent());
        Blocker->SetMobility(EComponentMobility::Movable);
        Blocker->SetBoxExtent(FVector(Edge.halfLength, Edge.halfThickness, Edge.halfHeight), false);
        Blocker->SetWorldTransform(FTransform(FRotator(0.0f, static_cast<float>(Edge.yaw), 0.0f),
            FVector(Edge.centre.x, Edge.centre.y, Edge.z)));
        Blocker->SetCollisionResponseToAllChannels(ECR_Ignore);
        Blocker->SetCollisionObjectType(ECC_WorldStatic);
        Blocker->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Blocker->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
        Blocker->CanCharacterStepUpOn = ECB_No;
        Blocker->SetGenerateOverlapEvents(false);
        Blocker->SetCanEverAffectNavigation(false);
        Blocker->SetHiddenInGame(true);
        Blocker->RegisterComponent();
        CoveRouteVisual.Components.Add(Blocker);
        ++Blockers;
    }
    // What actually went down (a group left out counts nothing).
    const auto Of = [&PlacedOf](Homestead::CoveKitPiece Piece) { return PlacedOf[static_cast<int32>(Piece)]; };
    UE_LOG(LogHomesteadWorld, Log, TEXT("Cove route: %d kit pieces (%d treads, %d landing slabs, %d kerbs, %d rail bays), %d rail blockers."),
        Placed, Of(Homestead::CoveKitPiece::StepA) + Of(Homestead::CoveKitPiece::StepB) + Of(Homestead::CoveKitPiece::StepC),
        Of(Homestead::CoveKitPiece::LandingSlab) + Of(Homestead::CoveKitPiece::LandingSlab75), Of(Homestead::CoveKitPiece::Kerb),
        Of(Homestead::CoveKitPiece::RailLevel) + Of(Homestead::CoveKitPiece::Rail26) + Of(Homestead::CoveKitPiece::Rail28)
            + Of(Homestead::CoveKitPiece::Rail30), Blockers);
}
