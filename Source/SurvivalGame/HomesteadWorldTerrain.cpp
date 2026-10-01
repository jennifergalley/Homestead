#include "HomesteadWorld.h"
#include "HomesteadWorldCommon.h"
#include "HomesteadWorldLog.h"
#include "HomesteadWorldLook.h"
#include "HomesteadEstateTerrain.h"

#include "Async/Async.h"
#include "Async/ParallelFor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "ProceduralMeshComponent.h"
#include "PhysicsEngine/BodySetup.h"

using HomesteadWorldCommon::ProfileChunkPublishing;
using HomesteadWorldLook::Meadow;

bool AHomesteadWorld::GetChunkBaseline(Homestead::Generation::WorldDescriptor World,
    Homestead::Generation::ChunkCoord Chunk, Homestead::Generation::ChunkBaseline& Baseline)
{
    const bool DisableCache = FParse::Param(FCommandLine::Get(), TEXT("HomesteadDisableChunkPreparation"));
    if (!DisableCache)
        if (const auto Found = ChunkBaselineCache.find(Chunk); Found != ChunkBaselineCache.end())
        {
            Baseline = Found->second;
            ++ChunkBaselineCacheHits;
            return true;
        }

    ++ChunkBaselineCacheMisses;
    const auto Status = Homestead::Generation::GenerateChunk(World, Chunk, Baseline);
    if (Status != Homestead::Generation::Status::Ok) return false;
    if (!DisableCache) ChunkBaselineCache[Chunk] = Baseline;
    return true;
}

const Homestead::Generation::ChunkBaseline* AHomesteadWorld::CachedBaselineFor(
    Homestead::Generation::WorldDescriptor World, Homestead::Generation::ChunkCoord Chunk) const
{
    if (FParse::Param(FCommandLine::Get(), TEXT("HomesteadDisableChunkPreparation"))
        || !bTerrainReady || Descriptor.seed != World.seed
        || Descriptor.generationVersion != World.generationVersion)
        return nullptr;
    const auto Found = ChunkBaselineCache.find(Chunk);
    return Found == ChunkBaselineCache.end() ? nullptr : &Found->second;
}

void AHomesteadWorld::QueueChunkBaselineBuild(Homestead::Generation::WorldDescriptor World,
    Homestead::Generation::ChunkCoord Center)
{
    if (FParse::Param(FCommandLine::Get(), TEXT("HomesteadDisableChunkPreparation"))
        || ChunkBaselineBuild) return;
    std::vector<Homestead::Generation::ChunkCoord> Missing;
    for (int Y = -4; Y <= 4; ++Y)
        for (int X = -4; X <= 4; ++X)
        {
            const Homestead::Generation::ChunkCoord Chunk{Center.x + X, Center.y + Y};
            if (ChunkBaselineCache.find(Chunk) == ChunkBaselineCache.end()) Missing.push_back(Chunk);
        }
    if (Missing.empty()) return;
    ChunkBaselineBuildCount += Missing.size();
    ChunkBaselineBuild = MakeUnique<TFuture<FHomesteadChunkBaselineBuild>>(
        Async(EAsyncExecution::ThreadPool, [World, Missing = std::move(Missing)]()
        {
            FHomesteadChunkBaselineBuild Build;
            Build.World = World;
            Build.Chunks.reserve(Missing.size());
            for (const auto Chunk : Missing)
            {
                Homestead::Generation::ChunkBaseline Baseline;
                if (Homestead::Generation::GenerateChunk(World, Chunk, Baseline)
                    == Homestead::Generation::Status::Ok)
                    Build.Chunks.push_back(MoveTemp(Baseline));
            }
            return Build;
        }));
}

float AHomesteadWorld::GroundHeight(float X, float Y, Homestead::Generation::WorldDescriptor World)
{
    if (HomesteadEstateTerrain::IsActive())
        return HomesteadEstateTerrain::Height(X, Y);
    namespace Gen = Homestead::Generation;
    if (!FMath::IsFinite(X) || !FMath::IsFinite(Y)
        || FMath::Abs(X) > Homestead::MaxWorldCoordinate + Gen::ChunkSizeCm * 4
        || FMath::Abs(Y) > Homestead::MaxWorldCoordinate + Gen::ChunkSizeCm * 4)
    {
        UE_LOG(LogHomesteadWorld, Error, TEXT("Non-finite or unsupported runtime terrain coordinate."));
        return std::numeric_limits<float>::quiet_NaN();
    }
    // Match the colliding mesh's triangles, rather than a smoother surface above/below it.
    const int64 X0 = FMath::FloorToInt64(X / Gen::TerrainSpacingCm) * Gen::TerrainSpacingCm;
    const int64 Y0 = FMath::FloorToInt64(Y / Gen::TerrainSpacingCm) * Gen::TerrainSpacingCm;
    const double U = (X - X0) / Gen::TerrainSpacingCm;
    const double V = (Y - Y0) / Gen::TerrainSpacingCm;
    Gen::TerrainSample A, B, C, D;
    Gen::TerrainSample* Samples[] = {&A, &B, &C, &D};
    for (int Index = 0; Index < 4; ++Index)
    {
        const auto Status = Gen::SampleTerrain(World, X0 + (Index % 2) * Gen::TerrainSpacingCm,
            Y0 + (Index / 2) * Gen::TerrainSpacingCm, *Samples[Index]);
        if (Status != Gen::Status::Ok)
        {
            UE_LOG(LogHomesteadWorld, Error, TEXT("Terrain height rejected: %s"), UTF8_TO_TCHAR(Gen::StatusMessage(Status)));
            return std::numeric_limits<float>::quiet_NaN();
        }
    }
    return U + V <= 1 ? A.heightCm + U * (B.heightCm - A.heightCm) + V * (C.heightCm - A.heightCm)
        : D.heightCm + (1 - U) * (C.heightCm - D.heightCm) + (1 - V) * (B.heightCm - D.heightCm);
}

float AHomesteadWorld::GroundHeight(float X, float Y) const
{
    return GroundHeight(X, Y, Descriptor);
}

FVector AHomesteadWorld::AtGround(float X, float Y, float Offset) const
{
    return FVector(X, Y, GroundHeight(X, Y) + Offset);
}

float AHomesteadWorld::CachedGroundHeight(float X, float Y) const
{
    if (HomesteadEstateTerrain::IsActive())
        return HomesteadEstateTerrain::Height(X, Y);
    namespace Gen = Homestead::Generation;
    if (!FMath::IsFinite(X) || !FMath::IsFinite(Y))
        return GroundHeight(X, Y);
    const int64 X0 = FMath::FloorToInt64(X / Gen::TerrainSpacingCm) * Gen::TerrainSpacingCm;
    const int64 Y0 = FMath::FloorToInt64(Y / Gen::TerrainSpacingCm) * Gen::TerrainSpacingCm;
    Gen::ChunkCoord Chunk;
    if (Gen::ChunkAt(X0, Y0, Chunk) != Gen::Status::Ok)
        return GroundHeight(X, Y);
    const auto Found = ChunkBaselineCache.find(Chunk);
    if (Found == ChunkBaselineCache.end())
        return GroundHeight(X, Y);

    const int32 Column = static_cast<int32>((X0 - static_cast<int64>(Chunk.x) * Gen::ChunkSizeCm)
        / Gen::TerrainSpacingCm);
    const int32 Row = static_cast<int32>((Y0 - static_cast<int64>(Chunk.y) * Gen::ChunkSizeCm)
        / Gen::TerrainSpacingCm);
    const auto& Samples = Found->second.terrain;
    const double A = Samples[Row * Gen::TerrainVerticesPerSide + Column].heightCm;
    const double B = Samples[Row * Gen::TerrainVerticesPerSide + Column + 1].heightCm;
    const double C = Samples[(Row + 1) * Gen::TerrainVerticesPerSide + Column].heightCm;
    const double D = Samples[(Row + 1) * Gen::TerrainVerticesPerSide + Column + 1].heightCm;
    const double U = (X - X0) / Gen::TerrainSpacingCm;
    const double V = (Y - Y0) / Gen::TerrainSpacingCm;
    return U + V <= 1 ? A + U * (B - A) + V * (C - A)
        : D + (1 - U) * (C - D) + (1 - V) * (B - D);
}

float AHomesteadWorld::StructureBase(Homestead::Point Center, double Yaw) const
{
    float Height = GroundHeight(Center.x, Center.y);
    for (int X : {-1, 1})
    {
        for (int Y : {-1, 1})
        {
            const auto Corner = Homestead::RotateYaw({X * 150.0, Y * 150.0}, Yaw);
            Height = FMath::Max(Height, GroundHeight(Center.x + Corner.x, Center.y + Corner.y));
        }
    }
    return Height + 16.0f;
}

bool AHomesteadWorld::BuildTerrain(const Homestead::State& State)
{
    if (State.fixedEstate)
    {
        // The Estate level's Landscape is the ground; no generated chunks, creek or regional water.
        if (!bFixedEstate)
        {
            for (auto& Entry : TerrainChunks)
            {
                if (Entry.Value.Terrain) Entry.Value.Terrain->DestroyComponent();
                if (Entry.Value.Water) Entry.Value.Water->DestroyComponent();
                ClearVisual(Entry.Value.Cover);
            }
            TerrainChunks.Reset();
            ClearRegionalWater();
            ChunkBaselineCache.clear();
            Ground = nullptr;
            bFixedEstate = true;
        }
        if (!HomesteadEstateTerrain::IsActive())
        {
            UE_LOG(LogHomesteadWorld, Error, TEXT("A fixed-estate game needs the Estate map's heightfield."));
            return false;
        }
        Descriptor = State.world;
        PreparedChunk = State.activeChunk;
        bTerrainReady = true;
        const bool bScenery = BuildEstateScenery();
        BuildRoadBridge();
        BuildCoveRoute();
        return bScenery;
    }
    if (bFixedEstate)
    {
        bFixedEstate = false;
        bTerrainReady = false;
    }
    if (IsPreparedFor(State)) return true;
    TerrainChunkProfile.Reset();
    namespace Gen = Homestead::Generation;
    const double Started = FPlatformTime::Seconds();
    const bool SameWorld = bTerrainReady && Descriptor.seed == State.world.seed
        && Descriptor.generationVersion == State.world.generationVersion;
    if (!SameWorld)
    {
        ClearRegionalWater();
        ChunkBaselineCache.clear();
    }
    TMap<FIntPoint, FHomesteadTerrainChunk> Prepared;
    double OldChunkTeardownMilliseconds = 0;
    double CollisionSwitchMilliseconds = 0;
    for (int Y = -2; Y <= 2; ++Y)
        for (int X = -2; X <= 2; ++X)
        {
            const FIntPoint Key(State.activeChunk.x + X, State.activeChunk.y + Y);
            if (SameWorld && TerrainChunks.Contains(Key)) continue;
            Gen::ChunkBaseline Baseline;
            const bool Generated = GetChunkBaseline(State.world, {Key.X, Key.Y}, Baseline);
            TObjectPtr<UProceduralMeshComponent> Water;
            auto* Mesh = Generated
                ? BuildTerrainChunk(Baseline, State.world, FMath::Abs(X) <= 1 && FMath::Abs(Y) <= 1, Water) : nullptr;
            if (!Mesh)
            {
                for (auto& Entry : Prepared)
                {
                    Entry.Value.Terrain->DestroyComponent();
                    if (Entry.Value.Water) Entry.Value.Water->DestroyComponent();
                ClearVisual(Entry.Value.Cover);
                }
                UE_LOG(LogHomesteadWorld, Error, TEXT("Chunk %d,%d preparation failed; previous terrain retained."),
                    Key.X, Key.Y);
                return false;
            }
            FHomesteadTerrainChunk Chunk;
            Chunk.Terrain = Mesh;
            Chunk.Water = Water;
            Chunk.bCollision = FMath::Abs(X) <= 1 && FMath::Abs(Y) <= 1;
            Prepared.Add(Key, MoveTemp(Chunk));
        }
    // Every destination tile exists and its synchronous collision cook has completed.
    for (auto It = TerrainChunks.CreateIterator(); It; ++It)
    {
        if (!SameWorld || FMath::Abs(It.Key().X - State.activeChunk.x) > 2
            || FMath::Abs(It.Key().Y - State.activeChunk.y) > 2)
        {
            const double TeardownStarted = FPlatformTime::Seconds();
            ClearVisual(It.Value().Cover);
            It.Value().Terrain->DestroyComponent();
            if (It.Value().Water) It.Value().Water->DestroyComponent();
            It.RemoveCurrent();
            OldChunkTeardownMilliseconds += (FPlatformTime::Seconds() - TeardownStarted) * 1000;
        }
    }
    for (auto& Entry : Prepared) TerrainChunks.Add(Entry.Key, MoveTemp(Entry.Value));
    for (auto& Entry : TerrainChunks)
    {
        const bool Colliding = FMath::Abs(Entry.Key.X - State.activeChunk.x) <= 1
            && FMath::Abs(Entry.Key.Y - State.activeChunk.y) <= 1;
        const double CollisionStarted = FPlatformTime::Seconds();
        Entry.Value.Terrain->SetCollisionEnabled(Colliding ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
        CollisionSwitchMilliseconds += (FPlatformTime::Seconds() - CollisionStarted) * 1000;
        Entry.Value.bCollision = Colliding;
    }
    Descriptor = State.world;
    PreparedChunk = State.activeChunk;
    bTerrainReady = true;
    Ground = TerrainChunks.FindChecked(FIntPoint(PreparedChunk.x, PreparedChunk.y)).Terrain;
    std::vector<Homestead::RegionalGeneration::RegionCoord> LoadedRegions;
    for (const auto& Entry : TerrainChunks)
    {
        const int64 OriginX = static_cast<int64>(Entry.Key.X) * Gen::ChunkSizeCm;
        const int64 OriginY = static_cast<int64>(Entry.Key.Y) * Gen::ChunkSizeCm;
        for (const int64 X : {OriginX, OriginX + Gen::ChunkSizeCm - 1})
            for (const int64 Y : {OriginY, OriginY + Gen::ChunkSizeCm - 1})
            {
                Homestead::RegionalGeneration::RegionCoord Region;
                const auto RegionStatus = Homestead::RegionalGeneration::RegionAtCm(X, Y, Region);
                if (RegionStatus != Homestead::RegionalGeneration::Status::Ok)
                {
                    UE_LOG(LogHomesteadWorld, Error, TEXT("Loaded regional identity failed: %s"),
                        UTF8_TO_TCHAR(Homestead::RegionalGeneration::StatusMessage(RegionStatus)));
                    return false;
                }
                if (std::find(LoadedRegions.begin(), LoadedRegions.end(), Region) == LoadedRegions.end())
                    LoadedRegions.push_back(Region);
            }
    }
    if (!RefreshRegionalDescriptors(State.world, LoadedRegions)) return false;
    if (!RebuildRegionalWater()) return false;
    LastTerrainPrepareMilliseconds = (FPlatformTime::Seconds() - Started) * 1000;
    if (ProfileChunkPublishing())
        TerrainProfile = TerrainChunkProfile + FString::Printf(
            TEXT("CHUNK_STAGE terrain center=%d,%d old_chunk_teardown_ms=%.3f existing_collision_switch_ms=%.3f total_ms=%.3f\n"),
            State.activeChunk.x, State.activeChunk.y, OldChunkTeardownMilliseconds,
            CollisionSwitchMilliseconds, LastTerrainPrepareMilliseconds);
    QueueChunkBaselineBuild(State.world, State.activeChunk);
    UE_LOG(LogHomesteadWorld, Display, TEXT("Generated terrain: seed=%llu version=%u center=%d,%d tiles=%d colliding=9 vertices_per_tile=625 prepare_ms=%.3f baseline_hits=%llu baseline_misses=%llu baseline_async_builds=%llu"),
        static_cast<unsigned long long>(Descriptor.seed), Descriptor.generationVersion,
        PreparedChunk.x, PreparedChunk.y, TerrainChunks.Num(), LastTerrainPrepareMilliseconds,
        static_cast<unsigned long long>(ChunkBaselineCacheHits),
        static_cast<unsigned long long>(ChunkBaselineCacheMisses),
        static_cast<unsigned long long>(ChunkBaselineBuildCount));
    return true;
}

UProceduralMeshComponent* AHomesteadWorld::BuildTerrainChunk(
    const Homestead::Generation::ChunkBaseline& Baseline, Homestead::Generation::WorldDescriptor World, bool bCollision,
    TObjectPtr<UProceduralMeshComponent>& OutWater)
{
    namespace Gen = Homestead::Generation;
    OutWater = nullptr;
    const double Started = FPlatformTime::Seconds();
    auto* Mesh = NewObject<UProceduralMeshComponent>(this);
    Mesh->SetupAttachment(GetRootComponent());
    Mesh->SetMobility(EComponentMobility::Static);
    Mesh->bUseAsyncCooking = false;
    Mesh->bUseComplexAsSimpleCollision = true;
    Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
    Mesh->SetGenerateOverlapEvents(false);
    Mesh->RegisterComponent();
    const double Registered = FPlatformTime::Seconds();

    constexpr int Cells = Gen::TerrainCellsPerChunk;
    constexpr float Spacing = Gen::TerrainSpacingCm;
    const double OriginX = static_cast<int64>(Baseline.chunk.x) * Gen::ChunkSizeCm;
    const double OriginY = static_cast<int64>(Baseline.chunk.y) * Gen::ChunkSizeCm;
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UV;
    TArray<FLinearColor> Colors;
    TArray<FProcMeshTangent> Tangents;
    Vertices.Reserve((Cells + 1) * (Cells + 1));
    for (int Y = 0; Y <= Cells; ++Y)
    {
        for (int X = 0; X <= Cells; ++X)
        {
            const double PX = OriginX + X * Spacing;
            const double PY = OriginY + Y * Spacing;
            const auto& Sample = Baseline.terrain[Y * (Cells + 1) + X];
            Vertices.Add(FVector(PX, PY, Sample.heightCm));
            Normals.Add(FVector(Sample.normalX, Sample.normalY, Sample.normalZ));
            UV.Add(FVector2D(PX / 300.0f, PY / 300.0f));
            Colors.Add(FLinearColor(Gen::CreekGroundBlendWeight(World, PX, PY), 0, 0, 1));
            Tangents.Add(FProcMeshTangent(FVector(Sample.normalZ, 0, -Sample.normalX).GetSafeNormal(), false));
            if (X < Cells && Y < Cells)
            {
                const int A = Y * (Cells + 1) + X;
                Triangles.Append({A, A + Cells + 1, A + 1, A + 1, A + Cells + 1, A + Cells + 2});
            }
        }
    }
    const double SectionStarted = FPlatformTime::Seconds();
    Mesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UV, Colors, Tangents, true);
    const double SectionPublished = FPlatformTime::Seconds();
    Mesh->SetMaterial(0, GroundMaterial ? GroundMaterial.Get() : Material(Meadow));
    if (!Mesh->GetBodySetup() || !Mesh->GetBodySetup()->bCreatedPhysicsMeshes
        || Mesh->GetBodySetup()->bFailedToCreatePhysicsMeshes)
    {
        UE_LOG(LogHomesteadWorld, Error, TEXT("Generated terrain collision cook did not produce physics meshes."));
        Mesh->DestroyComponent();
        return nullptr;
    }

    const double WaterStarted = FPlatformTime::Seconds();
    // The colliding terrain carries the muddy bank blend and the channel; the creek surface is its
    // own Single Layer Water, shadowless mesh. It spans the whole channel at the water level, and each
    // vertex's colour records how deep the water stands over the rendered bed (the triangulated
    // terrain, not the finer generator profile) so the material turns the shallows clear and fades
    // the shoreline exactly where the surface meets the bank.
    auto RenderedHeight = [&](double PX, double PY)
    {
        const double LX = FMath::Clamp((PX - OriginX) / Spacing, 0.0, Cells - 1e-6);
        const double LY = FMath::Clamp((PY - OriginY) / Spacing, 0.0, Cells - 1e-6);
        const int IX = FMath::FloorToInt(LX);
        const int IY = FMath::FloorToInt(LY);
        const double FX = LX - IX;
        const double FY = LY - IY;
        auto H = [&](int X, int Y) { return Baseline.terrain[Y * (Cells + 1) + X].heightCm; };
        // Quads split along the (X+1, Y) - (X, Y+1) diagonal, matching the terrain triangles.
        return FX + FY <= 1.0
            ? H(IX, IY) + FX * (H(IX + 1, IY) - H(IX, IY)) + FY * (H(IX, IY + 1) - H(IX, IY))
            : H(IX + 1, IY + 1) + (1 - FX) * (H(IX, IY + 1) - H(IX + 1, IY + 1))
                + (1 - FY) * (H(IX + 1, IY) - H(IX + 1, IY + 1));
    };
    auto CreekSurface = [&]()
    {
        Vertices.Reset();
        Triangles.Reset();
        Normals.Reset();
        UV.Reset();
        Colors.Reset();
        Tangents.Reset();
        TArray<float> Depths;
        constexpr int Rows = Cells * 2;
        constexpr int Columns = CreekSurfaceColumns;
        for (int Y = 0; Y <= Rows; ++Y)
        {
            const double PY = OriginY + Y * Spacing * 0.5;
            const double Center = Homestead::StreamX(PY);
            Gen::TerrainSample Sample;
            const auto Status = Gen::SampleTerrain(World, FMath::RoundToInt64(Center), FMath::RoundToInt64(PY), Sample);
            if (Status != Gen::Status::Ok)
            {
                UE_LOG(LogHomesteadWorld, Error, TEXT("Generated creek sample failed: %s"), UTF8_TO_TCHAR(Gen::StatusMessage(Status)));
                return false;
            }
            const double Z = Sample.waterHeightCm - CreekSurfaceDropCm;
            for (int X = 0; X <= Columns; ++X)
            {
                const double Offset = (X - Columns * 0.5) * CreekSurfaceHalfSpanCm * 2.0 / Columns;
                const double PX = FMath::Clamp<double>(Center + Offset, OriginX, OriginX + Gen::ChunkSizeCm);
                const double Depth = Z - RenderedHeight(PX, PY);
                Depths.Add(Depth);
                Vertices.Add(FVector(PX, PY, Z));
                Normals.Add(FVector::UpVector);
                UV.Add(FVector2D(Offset / 100.0, PY / 100.0));
                Colors.Add(FLinearColor(FMath::Clamp(Depth / 35.0, 0.0, 1.0), FMath::Clamp(Depth / 8.0, 0.0, 1.0), 0, 1));
                Tangents.Add(FProcMeshTangent(1, 0, 0));
            }
        }
        for (int Y = 0; Y < Rows; ++Y)
            for (int X = 0; X < Columns; ++X)
            {
                const int A = Y * (Columns + 1) + X;
                const int C = A + Columns + 1;
                // Quads wholly under the bank are never visible; leave them out.
                if (FMath::Max(FMath::Max(Depths[A], Depths[A + 1]), FMath::Max(Depths[C], Depths[C + 1])) <= 0) continue;
                Triangles.Append({A, C, A + 1, A + 1, C, C + 1});
            }
        if (Triangles.IsEmpty()) return true;
        auto* Water = NewObject<UProceduralMeshComponent>(this);
        Water->SetupAttachment(GetRootComponent());
        Water->SetMobility(EComponentMobility::Static);
        Water->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
        Water->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Water->SetGenerateOverlapEvents(false);
        Water->SetCanEverAffectNavigation(false);
        Water->SetCastShadow(false);
        Water->ComponentTags.Add(TEXT("CreekWater"));
        Water->RegisterComponent();
        Water->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UV, Colors, Tangents, false);
        Water->SetMaterial(0, CreekWaterMaterial ? CreekWaterMaterial.Get()
            : Material(FLinearColor(0.075f, 0.26f, 0.29f), 0.16f));
        OutWater = Water;
        return true;
    };
    if (OriginX <= 1680 + CreekSurfaceHalfSpanCm && OriginX + Gen::ChunkSizeCm >= 1320 - CreekSurfaceHalfSpanCm)
    {
        if (!CreekSurface())
        {
            Mesh->DestroyComponent();
            return nullptr;
        }
    }
    const double WaterPublished = FPlatformTime::Seconds();
    Mesh->SetCollisionEnabled(bCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    if (ProfileChunkPublishing())
        TerrainChunkProfile += FString::Printf(
            TEXT("CHUNK_STAGE terrain_tile=%d,%d collision=%d object_register_ms=%.3f vertices_ms=%.3f section_with_cook_ms=%.3f water_ms=%.3f collision_enable_ms=%.3f\n"),
            Baseline.chunk.x, Baseline.chunk.y, bCollision ? 1 : 0,
            (Registered - Started) * 1000, (SectionStarted - Registered) * 1000,
            (SectionPublished - SectionStarted) * 1000, (WaterPublished - WaterStarted) * 1000,
            (FPlatformTime::Seconds() - WaterPublished) * 1000);
    return Mesh;
}
