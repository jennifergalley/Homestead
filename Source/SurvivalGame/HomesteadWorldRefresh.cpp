#include "HomesteadWorld.h"
#include "HomesteadWorldCommon.h"
#include "HomesteadWorldKeys.h"
#include "HomesteadWorldLog.h"
#include "HomesteadWorldVisualHelpers.h"
#include "HomesteadGrassField.h"
#include "Simulation/HomesteadOvergrowth.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

using HomesteadWorldCommon::ProfileChunkPublishing;
using HomesteadWorldVisualHelpers::RemoveMissing;
using HomesteadWorldVisualHelpers::Stage;

void AHomesteadWorld::ClearVisual(FHomesteadWorldVisual& Visual)
{
    for (USceneComponent* Component : Visual.Components)
    {
        if (IsValid(Component))
        {
            Component->DestroyComponent();
        }
    }
    Visual.Components.Reset();
    Visual.Signature.Reset();
    Visual.bShownCleared = false;
}

void AHomesteadWorld::CancelStagedResources()
{
    for (auto& Entry : StagedResourceVisuals) ClearVisual(Entry.Value);
    for (auto& Entry : StagedResourceProduceVisuals) ClearVisual(Entry.Value);
    for (auto& Entry : StagedCoverChunks) ClearVisual(Entry.Value.Cover);
    StagedResourceVisuals.Reset();
    StagedResourceProduceVisuals.Reset();
    StagedCoverChunks.Reset();
    StagedResourceNodes.clear();
    StagedCoverKeys.Reset();
    StagedResourceCursor = 0;
    StagedCoverCursor = 0;
    StagingFrameMaximumMilliseconds = 0;
    bStagingResourceWindow = false;
    bStagingResourceBuild = false;
}

bool AHomesteadWorld::StageAdjacentResources(const Homestead::Simulation& Destination,
    uint64 SourceRevision)
{
    const auto& State = Destination.GetState();
    if (!bInitialized || !bTerrainReady || Descriptor.seed != State.world.seed
        || Descriptor.generationVersion != State.world.generationVersion
        || FMath::Abs(State.activeChunk.x - PreparedChunk.x)
            + FMath::Abs(State.activeChunk.y - PreparedChunk.y) != 1)
    {
        UE_LOG(LogHomesteadWorld, Error, TEXT("Adjacent resource staging requires a prepared neighboring world."));
        CancelStagedResources();
        return false;
    }
    if (!bStagingResourceWindow || StagedChunk != State.activeChunk
        || StagedWorld.seed != State.world.seed
        || StagedWorld.generationVersion != State.world.generationVersion
        || StagedSourceRevision != SourceRevision)
    {
        CancelStagedResources();
        bStagingResourceWindow = true;
        StagedChunk = State.activeChunk;
        StagedWorld = State.world;
        StagedSourceRevision = SourceRevision;
        for (const auto& Node : State.resources)
            if (!Node.cleared && Node.kind != Homestead::ResourceKind::ForestTree
                && !ResourceVisuals.Contains(Node.id))
                StagedResourceNodes.push_back(Node);
        for (int32 Y = -2; Y <= 2; ++Y)
            for (int32 X = -2; X <= 2; ++X)
            {
                const FIntPoint Key(State.activeChunk.x + X, State.activeChunk.y + Y);
                if (!TerrainChunks.Contains(Key))
                    StagedCoverKeys.Add(Key);
            }
    }

    const double Started = FPlatformTime::Seconds();
    while (StagedResourceCursor < static_cast<int32>(StagedResourceNodes.size())
        && (FPlatformTime::Seconds() - Started) < 0.004)
    {
        const auto& Node = StagedResourceNodes[StagedResourceCursor++];
        const FString Signature = HomesteadWorldKeys::ResourceSignature(Node.kind, Node.position, Node.cleared);
        bStagingResourceBuild = true;
        auto& Base = StagedResourceVisuals.FindOrAdd(Node.id);
        BuildResource(Base, Node, false);
        Base.Signature = Signature;
        Base.bShownCleared = Node.cleared;
        auto& Produce = StagedResourceProduceVisuals.FindOrAdd(Node.id);
        const bool bReady = Node.readyAtHour <= State.hour;
        if (bReady) BuildResource(Produce, Node, true);
        Produce.Signature = Signature + (bReady ? TEXT(":ready") : TEXT(":harvested"));
        bStagingResourceBuild = false;
        if (bVisualBuildFailed)
        {
            UE_LOG(LogHomesteadWorld, Error,
                TEXT("Adjacent resource %d could not be staged; previous world retained."), Node.id);
            CancelStagedResources();
            return false;
        }
    }
    if (StagedResourceCursor == static_cast<int32>(StagedResourceNodes.size())
        && StagedCoverCursor < StagedCoverKeys.Num()
        && (FPlatformTime::Seconds() - Started) < 0.004)
    {
        const FIntPoint Key = StagedCoverKeys[StagedCoverCursor];
        bool bNeighborsCached = true;
        for (int32 DY = -1; DY <= 1; ++DY)
            for (int32 DX = -1; DX <= 1; ++DX)
                bNeighborsCached &= CachedBaselineFor(State.world,
                    {Key.X + DX, Key.Y + DY}) != nullptr;
        if (bNeighborsCached)
        {
            if (!BuildDecorations(Destination, &Key))
            {
                UE_LOG(LogHomesteadWorld, Error,
                    TEXT("Adjacent cover %d,%d could not be staged; previous world retained."),
                    Key.X, Key.Y);
                CancelStagedResources();
                return false;
            }
            ++StagedCoverCursor;
        }
    }
    StagingFrameMaximumMilliseconds = FMath::Max(StagingFrameMaximumMilliseconds,
        (FPlatformTime::Seconds() - Started) * 1000);
    return true;
}

void AHomesteadWorld::UpdateEstateGrass(const Homestead::State& State)
{
    if (!State.fixedEstate)
    {
        if (EstateGrass) EstateGrass->Clear();
        return;
    }
    if (!EstateGrass)
    {
        EstateGrass = NewObject<UHomesteadGrassField>(this, TEXT("EstateGrass"));
        EstateGrass->SetupAttachment(GetRootComponent());
        EstateGrass->RegisterComponent();
    }
    const APlayerController* Viewer = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    if (Viewer && Viewer->PlayerCameraManager)
        EstateGrass->Update(State, Viewer->PlayerCameraManager->GetCameraLocation());
}

uint64 AHomesteadWorld::RefreshInputsKey(const Homestead::Simulation& Simulation) const
{
    // Everything Refresh's layout strings and per-object signatures read, in integers. Readiness,
    // plot stages and fire fuel change as game time passes, without a revision bump.
    const Homestead::State& State = Simulation.GetState();
    uint64 Key = HomesteadWorldKeys::Mix(HomesteadWorldKeys::Seed, Simulation.GetRevision());
    Key = HomesteadWorldKeys::Mix(Key, State.world.seed);
    Key = HomesteadWorldKeys::Mix(Key, State.world.generationVersion);
    Key = HomesteadWorldKeys::Mix(Key, HomesteadWorldKeys::Pair(State.activeChunk.x, State.activeChunk.y));
    Key = HomesteadWorldKeys::Mix(Key, State.fixedEstate ? 1 : 0);
    Key = HomesteadWorldKeys::Mix(Key, HomesteadWorldKeys::Pair(HeldProduceId, HeldPlotId));
    Key = HomesteadWorldKeys::Mix(Key, HomesteadWorldKeys::Pair(HeldHarvestPlotId, static_cast<int32>(HeldHarvestKind)));
    Key = HomesteadWorldKeys::Mix(Key, bHeldPlotHidden ? 1 : 0);
    Key = HomesteadWorldKeys::Mix(Key, 0x3ull + State.resources.size());
    for (const auto& Node : State.resources)
    {
        Key = HomesteadWorldKeys::Mix(Key, HomesteadWorldKeys::Pair(Node.id, static_cast<int32>(Node.kind)));
        Key = HomesteadWorldKeys::Mix(Key, HomesteadWorldKeys::Bits(Node.position.x));
        Key = HomesteadWorldKeys::Mix(Key, HomesteadWorldKeys::Bits(Node.position.y));
        Key = HomesteadWorldKeys::Mix(Key, HomesteadWorldKeys::Pair(Node.key.chunk.x, Node.key.chunk.y));
        Key = HomesteadWorldKeys::Mix(Key, (static_cast<uint64>(Node.key.localId) << 2)
            | (Node.cleared ? 2u : 0u) | (Node.readyAtHour <= State.hour ? 1u : 0u));
    }
    Key = HomesteadWorldKeys::Mix(Key, 0x5ull + State.structures.size());
    for (const auto& Structure : State.structures)
    {
        Key = HomesteadWorldKeys::Mix(Key, HomesteadWorldKeys::Pair(Structure.id, static_cast<int32>(Structure.kind)));
        Key = HomesteadWorldKeys::Mix(Key, HomesteadWorldKeys::Pair(Structure.buildingId, Structure.rotation));
        Key = HomesteadWorldKeys::Mix(Key, HomesteadWorldKeys::Pair(Structure.cellX, Structure.cellY));
        Key = HomesteadWorldKeys::Mix(Key, HomesteadWorldKeys::Pair(static_cast<int32>(Structure.skin), Structure.fuelHours > 0 ? 1 : 0));
    }
    Key = HomesteadWorldKeys::Mix(Key, 0x7ull + State.plots.size());
    for (const auto& Plot : State.plots)
    {
        Key = HomesteadWorldKeys::Mix(Key, HomesteadWorldKeys::Pair(Plot.id, static_cast<int32>(Plot.kind)));
        Key = HomesteadWorldKeys::Mix(Key, HomesteadWorldKeys::Pair(Plot.cellX, Plot.cellY));
        Key = HomesteadWorldKeys::Mix(Key, HomesteadWorldKeys::Pair(Stage(Plot.growth, 24), Stage(Plot.moisture, 5)));
        Key = HomesteadWorldKeys::Mix(Key, HomesteadWorldKeys::Pair(Stage(Plot.weeds, 8) * 2 + (Plot.planted ? 1 : 0),
            static_cast<int32>(Homestead::StageOf(Plot))));
    }
    Key = HomesteadWorldKeys::Mix(Key, 0xDull + State.worldDrops.size());
    for (const auto& Drop : State.worldDrops)
    {
        Key = HomesteadWorldKeys::Mix(Key, HomesteadWorldKeys::Pair(Drop.id, static_cast<int32>(Drop.item)));
        Key = HomesteadWorldKeys::Mix(Key, HomesteadWorldKeys::Bits(Drop.position.x));
        Key = HomesteadWorldKeys::Mix(Key, HomesteadWorldKeys::Bits(Drop.position.y));
        Key = HomesteadWorldKeys::Mix(Key, HomesteadWorldKeys::Pair(Drop.quantity, Drop.wearableId));
    }
    Key = HomesteadWorldKeys::Mix(Key, 0xEull + State.resourceEdits.size());
    for (const auto& Edit : State.resourceEdits)
    {
        Key = HomesteadWorldKeys::Mix(Key, HomesteadWorldKeys::Pair(Edit.key.chunk.x, Edit.key.chunk.y));
        Key = HomesteadWorldKeys::Mix(Key, (static_cast<uint64>(Edit.key.localId) << 1) | (Edit.cleared ? 1u : 0u));
    }
    return Key;
}

bool AHomesteadWorld::Refresh(const Homestead::Simulation& Simulation)
{
    const double RefreshStarted = FPlatformTime::Seconds();
    const auto& State = Simulation.GetState();
    if (!bInitialized)
    {
        return Initialize(Simulation);
    }
    const bool Transition = !IsPreparedFor(State);
    const bool WorldChanged = Descriptor.seed != State.world.seed || Descriptor.generationVersion != State.world.generationVersion;
    if (WorldChanged) CancelStagedResources();
    if (!BuildTerrain(State)) return false;
    bVisualBuildFailed = false;
    if (Transition) LastTransitionStagedCoverChunks = 0;
    if (WorldChanged)
    {
        for (auto& Entry : ResourceVisuals) ClearVisual(Entry.Value);
        for (auto& Entry : ResourceProduceVisuals) ClearVisual(Entry.Value);
        for (auto& Entry : StructureVisuals) ClearVisual(Entry.Value);
        for (auto& Entry : PlotVisuals) ClearVisual(Entry.Value);
        for (auto& Entry : DropVisuals) ClearVisual(Entry.Value);
        DropVisuals.Reset();
        ClearOuterTreeBatches();
        ClearActiveTreeBatches();
        ClearVisual(Preview);
    }
    const uint64 Inputs = RefreshInputsKey(Simulation);
    if (!Transition && !WorldChanged && bRefreshInputsKnown && Inputs == LastRefreshInputs)
    {
        // Nothing the visuals are built from has changed: keep them and do only the per-refresh work.
        UpdateEstateGrass(State);
        UpdateSceneryTreeStages(Simulation);
        bLampDropLit = Simulation.LampOil() > 0.0;
        UpdateLighting(State);
        LastRefreshMilliseconds = (FPlatformTime::Seconds() - RefreshStarted) * 1000;
        return true;
    }
    bRefreshInputsKnown = false;
    uint64 LayoutKey = HomesteadWorldKeys::Mix(HomesteadWorldKeys::Seed, State.world.seed);
    LayoutKey = HomesteadWorldKeys::Mix(LayoutKey, State.world.generationVersion);
    LayoutKey = HomesteadWorldKeys::Mix(LayoutKey, HomesteadWorldKeys::Pair(State.activeChunk.x, State.activeChunk.y));
    for (const auto& Node : State.resources)
    {
        LayoutKey = HomesteadWorldKeys::Mix(LayoutKey, static_cast<uint64>(Node.id));
        LayoutKey = HomesteadWorldKeys::Mix(LayoutKey, HomesteadWorldKeys::Bits(Node.position.x));
        LayoutKey = HomesteadWorldKeys::Mix(LayoutKey, HomesteadWorldKeys::Bits(Node.position.y));
        LayoutKey = HomesteadWorldKeys::Mix(LayoutKey, Node.cleared ? 1 : 0);
    }
    LayoutKey = HomesteadWorldKeys::Mix(LayoutKey, 0x5ull + State.structures.size());
    for (const auto& Structure : State.structures)
    {
        LayoutKey = HomesteadWorldKeys::Mix(LayoutKey, static_cast<uint64>(Structure.buildingId));
        LayoutKey = HomesteadWorldKeys::Mix(LayoutKey, HomesteadWorldKeys::Pair(Structure.cellX, Structure.cellY));
        LayoutKey = HomesteadWorldKeys::Mix(LayoutKey, HomesteadWorldKeys::Pair(static_cast<int32>(Structure.kind), Structure.rotation));
    }
    LayoutKey = HomesteadWorldKeys::Mix(LayoutKey, 0x7ull + State.plots.size());
    for (const auto& Plot : State.plots)
        LayoutKey = HomesteadWorldKeys::Mix(LayoutKey, HomesteadWorldKeys::Pair(Plot.cellX, Plot.cellY));
    LayoutKey = HomesteadWorldKeys::Mix(LayoutKey, 0xDull + State.worldDrops.size());
    for (const auto& Drop : State.worldDrops)
    {
        LayoutKey = HomesteadWorldKeys::Mix(LayoutKey, static_cast<uint64>(Drop.id));
        LayoutKey = HomesteadWorldKeys::Mix(LayoutKey, HomesteadWorldKeys::Bits(Drop.position.x));
        LayoutKey = HomesteadWorldKeys::Mix(LayoutKey, HomesteadWorldKeys::Bits(Drop.position.y));
    }
    FString Layout = FString::Printf(TEXT("%016llx"), static_cast<unsigned long long>(LayoutKey));
    if (ResourceLayoutSignature != Layout)
    {
        if (!State.fixedEstate && !BuildDecorations(Simulation)) return false;
        ResourceLayoutSignature = MoveTemp(Layout);
    }
    if (State.fixedEstate) ClearEstateSceneryUnderPieces(Simulation, LayoutKey);
    UpdateSceneryTreeStages(Simulation);
    UpdateEstateGrass(State);
    FString OuterLayout = FString::Printf(TEXT("%llu:%u:%d,%d;"),
        static_cast<unsigned long long>(State.world.seed), State.world.generationVersion,
        State.activeChunk.x, State.activeChunk.y);
    TArray<FString> ClearedOuterTreeEdits;
    for (const auto& Edit : State.resourceEdits)
    {
        const int32 DeltaX = FMath::Abs(Edit.key.chunk.x - State.activeChunk.x);
        const int32 DeltaY = FMath::Abs(Edit.key.chunk.y - State.activeChunk.y);
        if (!Edit.cleared || DeltaX > 2 || DeltaY > 2 || (DeltaX <= 1 && DeltaY <= 1))
            continue;
        Homestead::Generation::GeneratedEntity Entity;
        const auto Status = Homestead::Generation::FindEntity(State.world, Edit.key, Entity);
        if (Status != Homestead::Generation::Status::Ok)
        {
            UE_LOG(LogHomesteadWorld, Error, TEXT("Generated resource edit key cannot resolve: %s"),
                UTF8_TO_TCHAR(Homestead::Generation::StatusMessage(Status)));
            return false;
        }
        if (Entity.kind == Homestead::Generation::EntityKind::ForestTree)
            ClearedOuterTreeEdits.Add(FString::Printf(TEXT("%d:%d:%u;"),
                Edit.key.chunk.x, Edit.key.chunk.y, Edit.key.localId));
    }
    ClearedOuterTreeEdits.Sort();
    OuterLayout += FString::Join(ClearedOuterTreeEdits, TEXT(""));
    if (State.fixedEstate)
    {
        ClearOuterTreeBatches();
        OuterTreeLayoutSignature = OuterLayout;
    }
    else if (OuterTreeLayoutSignature != OuterLayout)
    {
        if (!RebuildOuterTreeBatches(Simulation)) return false;
        OuterTreeLayoutSignature = MoveTemp(OuterLayout);
    }
    TArray<FString> ActiveTreeRows;
    for (const auto& Node : State.resources)
        if (Node.kind == Homestead::ResourceKind::ForestTree)
            ActiveTreeRows.Add(FString::Printf(TEXT("%d:%d:%u:%d:%d;"),
                Node.key.chunk.x, Node.key.chunk.y, Node.key.localId, Node.id, Node.cleared));
    ActiveTreeRows.Sort();
    const FString ActiveLayout = FString::Printf(TEXT("%llu:%u:%d,%d;"),
        static_cast<unsigned long long>(State.world.seed), State.world.generationVersion,
        State.activeChunk.x, State.activeChunk.y) + FString::Join(ActiveTreeRows, TEXT(""));
    if (ActiveTreeLayoutSignature != ActiveLayout)
    {
        if (!RebuildActiveTreeBatches(Simulation)) return false;
        ActiveTreeLayoutSignature = ActiveLayout;
    }

    const bool bUseStagedResources = Transition && bStagingResourceWindow
        && StagedChunk == State.activeChunk && StagedWorld.seed == State.world.seed
        && StagedWorld.generationVersion == State.world.generationVersion
        && Simulation.GetRevision() == StagedSourceRevision + 1;
    auto AdoptStaged = [&](TMap<int32, FHomesteadWorldVisual>& Live,
        TMap<int32, FHomesteadWorldVisual>& Staged, int32 Id, const FString& Signature)
    {
        auto* Prepared = bUseStagedResources ? Staged.Find(Id) : nullptr;
        if (!Prepared || Prepared->Signature != Signature) return;
        for (USceneComponent* Component : Prepared->Components)
            if (!IsValid(Component) || !Component->IsRegistered())
            {
                UE_LOG(LogHomesteadWorld, Error, TEXT("Staged resource %d has an unregistered component."), Id);
                bVisualBuildFailed = true;
                return;
            }
        FHomesteadWorldVisual& Visual = Live.FindOrAdd(Id);
        ClearVisual(Visual);
        Visual = MoveTemp(*Prepared);
        Staged.Remove(Id);
        ++LastTransitionStagedResourceVisuals;
        for (USceneComponent* Component : Visual.Components)
        {
            Component->SetVisibility(true);
            Component->SetHiddenInGame(false);
        }
    };
    const double ResourceStarted = FPlatformTime::Seconds();
    if (Transition)
    {
        LastTransitionStagedResourceVisuals = 0;
        LastTransitionStagingFrameMilliseconds = StagingFrameMaximumMilliseconds;
    }
    RemoveMissing(ResourceVisuals, State.resources);
    RemoveMissing(ResourceProduceVisuals, State.resources);
    for (const auto& Node : State.resources)
    {
        if (Node.kind == Homestead::ResourceKind::ForestTree)
        {
            if (auto* Visual = ResourceVisuals.Find(Node.id)) ClearVisual(*Visual);
            ResourceVisuals.Remove(Node.id);
            if (auto* Produce = ResourceProduceVisuals.Find(Node.id)) ClearVisual(*Produce);
            ResourceProduceVisuals.Remove(Node.id);
            continue;
        }
        const bool bReady = Node.readyAtHour <= State.hour || Node.id == HeldProduceId;
        // A fallen bough or salvage pile she is still kneeling at stays until she has lifted from it.
        Homestead::ResourceNode Shown = Node;
        Shown.cleared = Node.cleared && Node.id != HeldProduceId;
        const FString Signature = HomesteadWorldKeys::ResourceSignature(Node.kind, Node.position, Shown.cleared);
        AdoptStaged(ResourceVisuals, StagedResourceVisuals, Node.id, Signature);
        if (bVisualBuildFailed) return false;
        FHomesteadWorldVisual& Visual = ResourceVisuals.FindOrAdd(Node.id);
        if (Visual.Signature != Signature)
        {
            // Cleared in play (not loaded or streamed in cleared): it pops away instead of vanishing.
            if (Shown.cleared && !Transition && Homestead::IsOvergrowth(Node.kind) && !Visual.Signature.IsEmpty() && !Visual.bShownCleared)
                StartClearPop(Visual, Node);
            ClearVisual(Visual);
            BuildResource(Visual, Shown, false);
            if (bVisualBuildFailed) return false;
            Visual.Signature = Signature;
            Visual.bShownCleared = Shown.cleared;
        }
        const FString ProduceSignature = Signature + (bReady ? TEXT(":ready") : TEXT(":harvested"));
        AdoptStaged(ResourceProduceVisuals, StagedResourceProduceVisuals, Node.id, ProduceSignature);
        if (bVisualBuildFailed) return false;
        FHomesteadWorldVisual& Produce = ResourceProduceVisuals.FindOrAdd(Node.id);
        if (Produce.Signature != ProduceSignature)
        {
            ClearVisual(Produce);
            if (bReady)
            {
                BuildResource(Produce, Shown, true);
                if (bVisualBuildFailed) return false;
            }
            Produce.Signature = ProduceSignature;
        }
        if (Node.kind == Homestead::ResourceKind::DeerRemains)
            for (USceneComponent* Bone : Visual.Components)
                if (Bone && Bone->IsVisible() == bReady) Bone->SetVisibility(!bReady);
    }
    if (Transition) CancelStagedResources();

    const double StructuresStarted = FPlatformTime::Seconds();
    std::vector<Homestead::Structure> NearStructures;
    std::vector<Homestead::Plot> NearPlots;
    std::vector<Homestead::WorldDrop> NearDrops;
    auto Near = [&](Homestead::Point Center)
    {
        if (State.fixedEstate) return true;
        return FMath::Abs(Center.x - (State.activeChunk.x + 0.5) * Homestead::Generation::ChunkSizeCm) <= 6000
            && FMath::Abs(Center.y - (State.activeChunk.y + 0.5) * Homestead::Generation::ChunkSizeCm) <= 6000;
    };
    for (const auto& Structure : State.structures)
        if (Near(Homestead::StructureCenter(State, Structure))) NearStructures.push_back(Structure);
    for (const auto& Plot : State.plots)
        if (Near(Homestead::PlotCenter(Plot))) NearPlots.push_back(Plot);
    const double ChunkCenterX = (State.activeChunk.x + 0.5) * Homestead::Generation::ChunkSizeCm;
    const double ChunkCenterY = (State.activeChunk.y + 0.5) * Homestead::Generation::ChunkSizeCm;
    for (const auto& Drop : State.worldDrops)
        if (State.fixedEstate || FMath::Abs(Drop.position.x - ChunkCenterX) <= 6000
            && FMath::Abs(Drop.position.y - ChunkCenterY) <= 6000)
            NearDrops.push_back(Drop);
    RemoveMissing(StructureVisuals, NearStructures);
    FoundationCells.Reset();
    for (const auto& Structure : State.structures)
        if (Structure.kind == Homestead::Piece::Foundation)
            FoundationCells.Add(FIntVector(Structure.cellX, Structure.cellY, Structure.buildingId));
    for (const auto& Structure : NearStructures)
    {
        const bool bOnFoundation = FoundationCells.Contains(FIntVector(Structure.cellX, Structure.cellY, Structure.buildingId));
        const FString Signature = FString::Printf(TEXT("%d:%d:%d:%d:%d:%d:%d:%d"),
            static_cast<int>(Structure.kind), Structure.buildingId, Structure.cellX, Structure.cellY, Structure.rotation,
            Structure.fuelHours > 0, bOnFoundation, static_cast<int>(Structure.skin));
        FHomesteadWorldVisual& Visual = StructureVisuals.FindOrAdd(Structure.id);
        if (Visual.Signature != Signature)
        {
            ClearVisual(Visual);
            const Homestead::Building* Frame = Homestead::FindBuilding(State, Structure.buildingId);
            BuildStructure(Visual, Structure, Frame ? *Frame : Homestead::Building{}, bOnFoundation, false);
            Visual.Signature = Signature;
        }
    }

    RemoveMissing(PlotVisuals, NearPlots);
    for (const auto& Plot : NearPlots)
    {
        // Just harvested: the ripe plant stays until her hands lift the produce.
        const bool bHarvestHeld = Plot.id == HeldHarvestPlotId;
        const bool bShownPlanted = (Plot.planted || bHarvestHeld) && Plot.id != HeldPlotId;
        const bool bSquareHidden = bHeldPlotHidden && Plot.id == HeldPlotId;
        const FString Signature = FString::Printf(TEXT("%d:%d:%d:%d:%d:%d:%d:%d:%d:%d"),
            Plot.cellX, Plot.cellY, bShownPlanted, bSquareHidden, Stage(Plot.growth, 24),
            Stage(Plot.moisture, 5), Stage(Plot.weeds, 8), static_cast<int>(Plot.kind), bHarvestHeld,
            static_cast<int>(Homestead::StageOf(Plot)));
        FHomesteadWorldVisual& Visual = PlotVisuals.FindOrAdd(Plot.id);
        if (Visual.Signature != Signature)
        {
            ClearVisual(Visual);
            Homestead::Plot Shown = Plot;
            Shown.planted = bShownPlanted;
            if (bHarvestHeld)
            {
                Shown.kind = HeldHarvestKind;
                Shown.growth = 1.0;
            }
            if (!bSquareHidden) BuildPlot(Visual, Shown);
            Visual.Signature = Signature;
        }
    }
    bLampDropLit = Simulation.LampOil() > 0.0;
    RemoveMissing(DropVisuals, NearDrops);
    for (const auto& Drop : NearDrops)
    {
        const FString Signature = FString::Printf(TEXT("%d:%.3f:%.3f:%d:%d:%d"),
            Drop.id, Drop.position.x, Drop.position.y, static_cast<int>(Drop.item),
            Drop.quantity, Drop.wearableId);
        FHomesteadWorldVisual& Visual = DropVisuals.FindOrAdd(Drop.id);
        if (Visual.Signature != Signature)
        {
            ClearVisual(Visual);
            BuildDrop(Visual, Drop);
            Visual.Signature = Signature;
        }
    }
    const double LightingStarted = FPlatformTime::Seconds();
    LastRefreshInputs = Inputs;
    bRefreshInputsKnown = true;
    UpdateLighting(State);
    LastRefreshMilliseconds = (FPlatformTime::Seconds() - RefreshStarted) * 1000;
    if (Transition)
    {
        LastTransitionRefreshMilliseconds = LastRefreshMilliseconds;
        LastTransitionTerrainMilliseconds = LastTerrainPrepareMilliseconds;
        LastTransitionCoverMilliseconds = LastCoverPrepareMilliseconds;
        LastTransitionOuterTreeMilliseconds = LastOuterTreePrepareMilliseconds;
        LastTransitionActiveTreeMilliseconds = LastActiveTreePrepareMilliseconds;
        if (ProfileChunkPublishing())
        {
            LastTransitionPublishingProfile = TerrainProfile + CoverProfile + OuterTreeProfile + ActiveTreeProfile;
            LastTransitionPublishingProfile += FString::Printf(
                TEXT("CHUNK_STAGE other_layout_ms=%.3f resource_visuals_ms=%.3f structures_drops_ms=%.3f lighting_ms=%.3f\n"),
                (ResourceStarted - RefreshStarted) * 1000
                    - LastTransitionTerrainMilliseconds - LastTransitionCoverMilliseconds
                    - LastTransitionOuterTreeMilliseconds - LastTransitionActiveTreeMilliseconds,
                (StructuresStarted - ResourceStarted) * 1000,
                (LightingStarted - StructuresStarted) * 1000,
                (FPlatformTime::Seconds() - LightingStarted) * 1000);
        }
    }
    if (LastRefreshMilliseconds > 25.0)
        UE_LOG(LogHomesteadWorld, Display,
            TEXT("Woodland refresh timing: center=%d,%d total_ms=%.3f terrain_ms=%.3f cover_ms=%.3f outer_trees_ms=%.3f active_trees_ms=%.3f baseline_hits=%llu baseline_misses=%llu"),
            State.activeChunk.x, State.activeChunk.y, LastRefreshMilliseconds,
            LastTerrainPrepareMilliseconds, LastCoverPrepareMilliseconds,
            LastOuterTreePrepareMilliseconds, LastActiveTreePrepareMilliseconds,
            static_cast<unsigned long long>(ChunkBaselineCacheHits),
            static_cast<unsigned long long>(ChunkBaselineCacheMisses));
    return true;
}
