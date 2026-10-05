#include "HomesteadWorld.h"
#include "HomesteadWorldKeys.h"
#include "HomesteadWorldLog.h"
#include "HomesteadEstateTerrain.h"
#include "Simulation/HomesteadTreeFelling.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

// Felling the estate's decorative trees (openspec chop-any-tree). The simulation keeps which trees are down
// (State::felledTrees, keyed by trunk position); this file finds the tree in front of her, topples a copy
// of it, and keeps the estate's batches in step: the standing instance hidden, a stump and then a sapling
// in its place until it has grown back.
namespace HomesteadSceneryTrees
{
// The grid cell the trees are indexed by; the reach never spans more than a neighbouring cell.
constexpr float CellCm = 2000.0f;
// Her facing only breaks ties: a trunk this close counts whatever way she looks, cm.
constexpr float AlwaysInReachCm = 150.0f;
// Stumps scale to the trunk they replace: trunk radius times this over the stump mesh's own radius.
constexpr float StumpFlare = 1.15f;
constexpr float StumpScaleMin = 0.35f, StumpScaleMax = 1.6f;
constexpr const TCHAR* StumpPath = TEXT("/Game/SurvivalGame/Environment/Props/EstateTimber/SM_StumpLarge.SM_StumpLarge");
constexpr const TCHAR* SaplingPath = TEXT("/Game/Trials/WoodlandResources_20260921_01/Meshes/SM_FirSapling_a.SM_FirSapling_a");

FIntPoint CellOf(float X, float Y)
{
    return FIntPoint(FMath::FloorToInt32(X / CellCm), FMath::FloorToInt32(Y / CellCm));
}

uint64 InstanceKey(int32 Batch, int32 Index) { return (static_cast<uint64>(Batch) << 32) | static_cast<uint32>(Index); }
}

void AHomesteadWorld::RegisterSceneryTree(int32 Batch, int32 Index, float X, float Y, float TrunkRadius)
{
    FSceneryTreeRef Ref;
    Ref.Batch = Batch;
    Ref.Index = Index;
    Ref.X = X;
    Ref.Y = Y;
    Ref.TrunkRadius = TrunkRadius;
    SceneryTreeGrid.FindOrAdd(HomesteadSceneryTrees::CellOf(X, Y)).Add(Ref);
}

const AHomesteadWorld::FSceneryTreeRef* AHomesteadWorld::FindSceneryTreeAt(FVector2D Trunk) const
{
    const FIntPoint Cell = HomesteadSceneryTrees::CellOf(Trunk.X, Trunk.Y);
    for (int32 DX = -1; DX <= 1; ++DX)
        for (int32 DY = -1; DY <= 1; ++DY)
            if (const TArray<FSceneryTreeRef>* Found = SceneryTreeGrid.Find(Cell + FIntPoint(DX, DY)))
                for (const FSceneryTreeRef& Ref : *Found)
                    if (Homestead::TreeFelling::Cm(Ref.X) == Homestead::TreeFelling::Cm(Trunk.X)
                        && Homestead::TreeFelling::Cm(Ref.Y) == Homestead::TreeFelling::Cm(Trunk.Y))
                        return &Ref;
    return nullptr;
}

bool AHomesteadWorld::FindSceneryTreeNear(const Homestead::Simulation& Simulation, FVector2D From, FVector2D Facing,
    float Reach, FVector2D& Trunk, float& Radius) const
{
    if (!Simulation.GetState().fixedEstate || SceneryTreeGrid.IsEmpty()) return false;
    const FIntPoint Cell = HomesteadSceneryTrees::CellOf(From.X, From.Y);
    const bool bFacing = Facing.Normalize();
    float BestSquared = Reach * Reach;
    const FSceneryTreeRef* Best = nullptr;
    for (int32 DX = -1; DX <= 1; ++DX)
        for (int32 DY = -1; DY <= 1; ++DY)
        {
            const TArray<FSceneryTreeRef>* Found = SceneryTreeGrid.Find(Cell + FIntPoint(DX, DY));
            if (!Found) continue;
            for (const FSceneryTreeRef& Ref : *Found)
            {
                const FVector2D To(Ref.X - From.X, Ref.Y - From.Y);
                const float DistanceSquared = To.SizeSquared();
                if (DistanceSquared >= BestSquared) continue;
                if (!EstateScenery.IsValidIndex(Ref.Batch) || !EstateSceneryHidden.IsValidIndex(Ref.Batch)
                    || EstateSceneryHidden[Ref.Batch][Ref.Index]) continue;
                if (bFacing && DistanceSquared > FMath::Square(HomesteadSceneryTrees::AlwaysInReachCm)
                    && FVector2D::DotProduct(To.GetSafeNormal(), Facing) < 0.2f) continue;
                BestSquared = DistanceSquared;
                Best = &Ref;
            }
        }
    if (!Best) return false;
    Trunk = FVector2D(Best->X, Best->Y);
    Radius = Best->TrunkRadius;
    return true;
}

bool AHomesteadWorld::BeginFellingScenery(const Homestead::Simulation& Simulation, FVector2D Trunk)
{
    FinishFallingTree();
    const FSceneryTreeRef* Ref = FindSceneryTreeAt(Trunk);
    if (!Ref || !EstateScenery.IsValidIndex(Ref->Batch) || !EstateSceneryTransforms.IsValidIndex(Ref->Batch)
        || !EstateSceneryTransforms[Ref->Batch].IsValidIndex(Ref->Index) || !EstateScenery[Ref->Batch]
        || !EstateScenery[Ref->Batch]->GetStaticMesh()) return false;
    auto* Part = NewObject<UStaticMeshComponent>(this);
    Part->SetupAttachment(GetRootComponent());
    Part->SetMobility(EComponentMobility::Movable);
    Part->SetStaticMesh(EstateScenery[Ref->Batch]->GetStaticMesh());
    Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Part->SetGenerateOverlapEvents(false);
    Part->SetCanEverAffectNavigation(false);
    if (!ApplyCameraSafeFoliageMaterials(*Part))
    {
        Part->DestroyComponent();
        return false;
    }
    Part->SetRelativeTransform(EstateSceneryTransforms[Ref->Batch][Ref->Index]);
    Part->RegisterComponent();
    FallingParts.Add(Part);
    FallingRest.Add(Part->GetComponentTransform());
    // It topples about its trunk's foot, not the mesh origin, which sits off the trunk by the root flare.
    const FVector Foot = GetActorTransform().TransformPosition(
        FVector(Ref->X, Ref->Y, HomesteadEstateTerrain::Height(Ref->X, Ref->Y)));
    FallPivot = Foot;
    FallHeight = FMath::Max(100.0f, static_cast<float>(Part->Bounds.GetBox().Max.Z - Foot.Z));
    FallAngle = FallRate = FallLying = 0;
    FallBounces = 0;
    UpdateSceneryTreeStages(Simulation, true);
    return true;
}

void AHomesteadWorld::UpdateSceneryTreeStages(const Homestead::Simulation& Simulation, bool bForce)
{
    const Homestead::State& State = Simulation.GetState();
    if (!State.fixedEstate || SceneryTreeGrid.IsEmpty()) return;
    const uint64 Revision = Simulation.GetRevision();
    if (!bForce && Revision == SceneryStageRevision && State.hour < SceneryStageNextHour) return;
    SceneryStageRevision = Revision;
    SceneryStageNextHour = Homestead::TreeFelling::NextChangeHour(State);

    // What stands where: a signature of the felled set and the stage of each, so a clock tick that
    // changes nothing visible costs nothing more.
    uint64 Signature = HomesteadWorldKeys::Mix(HomesteadWorldKeys::Seed, State.felledTrees.size());
    for (const Homestead::FelledTree& Entry : State.felledTrees)
    {
        Signature = HomesteadWorldKeys::Mix(Signature, HomesteadWorldKeys::Pair(Entry.xCm, Entry.yCm));
        Signature = HomesteadWorldKeys::Mix(Signature,
            static_cast<uint64>(Homestead::TreeFelling::StageOf(Entry, State.hour)) + 1);
    }
    if (Signature == SceneryStageSignature && ScenerySaplings && SceneryStumps) return;
    SceneryStageSignature = Signature;

    TSet<uint64> NowFelled;
    TArray<FTransform> Stumps, Saplings;
    UStaticMesh* StumpMesh = LoadObject<UStaticMesh>(nullptr, HomesteadSceneryTrees::StumpPath);
    UStaticMesh* SaplingMesh = LoadObject<UStaticMesh>(nullptr, HomesteadSceneryTrees::SaplingPath);
    const float StumpRadius = StumpMesh ? FMath::Max(StumpMesh->GetBounds().BoxExtent.X, StumpMesh->GetBounds().BoxExtent.Y) : 0.0f;
    for (const Homestead::FelledTree& Entry : State.felledTrees)
    {
        const auto Stage = Homestead::TreeFelling::StageOf(Entry, State.hour);
        if (Stage == Homestead::TreeFelling::Stage::Standing) continue;
        const FSceneryTreeRef* Ref = FindSceneryTreeAt(FVector2D(Entry.xCm, Entry.yCm));
        if (!Ref) continue;
        NowFelled.Add(HomesteadSceneryTrees::InstanceKey(Ref->Batch, Ref->Index));
        const FVector Foot(Ref->X, Ref->Y, HomesteadEstateTerrain::Height(Ref->X, Ref->Y));
        // A spin that is the same every time the stage is drawn.
        const float Yaw = static_cast<float>(HomesteadWorldKeys::Mix(HomesteadWorldKeys::Seed,
            HomesteadWorldKeys::Pair(Entry.xCm, Entry.yCm)) % 360ull);
        if (Stage == Homestead::TreeFelling::Stage::Stump)
        {
            const float Scale = StumpRadius > 1.0f ? FMath::Clamp(
                Ref->TrunkRadius * HomesteadSceneryTrees::StumpFlare / StumpRadius,
                HomesteadSceneryTrees::StumpScaleMin, HomesteadSceneryTrees::StumpScaleMax) : 1.0f;
            Stumps.Add(FTransform(FRotator(0, Yaw, 0), Foot, FVector(Scale)));
        }
        else
        {
            Saplings.Add(FTransform(FRotator(0, Yaw, 0), Foot, FVector(1.0f)));
        }
    }

    // Standing instances: hide the newly felled, bring back the regrown.
    TSet<int32> Touched;
    const auto SetHidden = [this, &Touched](uint64 Key, bool bHide)
    {
        const int32 Batch = static_cast<int32>(Key >> 32), Index = static_cast<int32>(Key & 0xFFFFFFFFu);
        if (!EstateScenery.IsValidIndex(Batch) || !EstateScenery[Batch] || !EstateSceneryHidden.IsValidIndex(Batch)
            || !EstateSceneryTransforms[Batch].IsValidIndex(Index)) return;
        FTransform Shown = EstateSceneryTransforms[Batch][Index];
        if (bHide) Shown.SetScale3D(FVector(0.0001f));
        EstateSceneryHidden[Batch][Index] = bHide;
        EstateScenery[Batch]->UpdateInstanceTransform(Index, Shown, true, false, true);
        Touched.Add(Batch);
    };
    for (const uint64 Key : NowFelled)
        if (!FelledSceneryInstances.Contains(Key)) SetHidden(Key, true);
    for (const uint64 Key : FelledSceneryInstances)
        if (!NowFelled.Contains(Key)) SetHidden(Key, false);
    FelledSceneryInstances = MoveTemp(NowFelled);
    for (const int32 Batch : Touched) EstateScenery[Batch]->MarkRenderStateDirty();

    const auto Fill = [this](TObjectPtr<UHierarchicalInstancedStaticMeshComponent>& Component, UStaticMesh* Mesh,
        const TArray<FTransform>& Transforms, bool bFoliage)
    {
        if (!Mesh) return;
        if (!Component)
        {
            Component = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
            Component->SetupAttachment(GetRootComponent());
            Component->SetMobility(EComponentMobility::Static);
            Component->SetStaticMesh(Mesh);
            Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Component->SetCanEverAffectNavigation(false);
            Component->SetCastShadow(true);
            Component->ComponentTags.Add(TEXT("EstateScenery"));
            if (bFoliage) ApplyCameraSafeFoliageMaterials(*Component);
            Component->RegisterComponent();
        }
        Component->ClearInstances();
        if (Transforms.Num() > 0) Component->AddInstances(Transforms, false, true);
    };
    Fill(SceneryStumps, StumpMesh, Stumps, false);
    Fill(ScenerySaplings, SaplingMesh, Saplings, true);
    UE_LOG(LogHomesteadWorld, Display, TEXT("Felled estate trees: %d stumps, %d saplings."), Stumps.Num(), Saplings.Num());
}
