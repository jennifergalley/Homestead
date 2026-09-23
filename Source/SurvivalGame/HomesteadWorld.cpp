#include "HomesteadWorld.h"

#include "Async/Async.h"
#include "Async/ParallelFor.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"
#include "PhysicsEngine/BodySetup.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogHomesteadWorld, Log, All);

namespace
{
// Original provisional shapes, not the final realistic environment asset set.
const FLinearColor Meadow(0.22f, 0.31f, 0.095f);
const FLinearColor Leaf(0.12f, 0.26f, 0.065f);
const FLinearColor LightLeaf(0.27f, 0.37f, 0.095f);
const FLinearColor Bark(0.19f, 0.105f, 0.052f);
const FLinearColor Wood(0.37f, 0.23f, 0.115f);
const FLinearColor Stone(0.32f, 0.36f, 0.34f);
const FLinearColor Soil(0.16f, 0.085f, 0.039f);
const FLinearColor Cloth(0.55f, 0.43f, 0.25f);
const FLinearColor PreviewColor(0.65f, 0.79f, 0.77f);

bool RegionalReachLess(const Homestead::RegionalGeneration::RiverReach& A,
    const Homestead::RegionalGeneration::RiverReach& B)
{
    return A.key.upstream != B.key.upstream ? A.key.upstream < B.key.upstream
        : A.key.downstream < B.key.downstream;
}

Homestead::ResourceKind ResourceKindFor(Homestead::Generation::EntityKind Kind)
{
    using Entity = Homestead::Generation::EntityKind;
    using Resource = Homestead::ResourceKind;
    switch (Kind)
    {
    case Entity::Branches: return Resource::Branches;
    case Entity::Stones: return Resource::Stones;
    case Entity::BerryBush: return Resource::BerryBush;
    case Entity::Roots: return Resource::Roots;
    case Entity::Flowers: return Resource::Flowers;
    case Entity::Reeds: return Resource::Reeds;
    case Entity::Sapling: return Resource::Sapling;
    case Entity::ForestTree: return Resource::ForestTree;
    default: return Resource::Count;
    }
}

bool ClipRegionalSegment(FVector2D& A, FVector2D& B,
    double LowX, double LowY, double HighX, double HighY)
{
    double Minimum = 0.0;
    double Maximum = 1.0;
    const FVector2D Delta = B - A;
    const auto Clip = [&](double Direction, double Distance)
    {
        if (FMath::IsNearlyZero(Direction)) return Distance >= 0.0;
        const double Ratio = Distance / Direction;
        if (Direction < 0.0) Minimum = FMath::Max(Minimum, Ratio);
        else Maximum = FMath::Min(Maximum, Ratio);
        return Minimum <= Maximum;
    };
    if (!Clip(-Delta.X, A.X - LowX) || !Clip(Delta.X, HighX - A.X)
        || !Clip(-Delta.Y, A.Y - LowY) || !Clip(Delta.Y, HighY - A.Y))
        return false;
    const FVector2D Start = A;
    A = Start + Delta * Minimum;
    B = Start + Delta * Maximum;
    return FVector2D::Distance(A, B) > 1.0;
}

FName MaterialKey(const FLinearColor& Color, float Roughness, float Glow)
{
    return FName(*FString::Printf(TEXT("Tint_%d_%d_%d_R%d_G%d"),
        FMath::RoundToInt(Color.R * 1000), FMath::RoundToInt(Color.G * 1000),
        FMath::RoundToInt(Color.B * 1000), FMath::RoundToInt(Roughness * 100),
        FMath::RoundToInt(Glow * 100)));
}

int Stage(double Value, int Steps)
{
    return FMath::Clamp(FMath::FloorToInt(Value * Steps), 0, Steps);
}

template<typename T>
void RemoveMissing(TMap<int32, FHomesteadWorldVisual>& Visuals, const T& Entries)
{
    TSet<int32> Existing;
    for (const auto& Entry : Entries)
    {
        Existing.Add(Entry.id);
    }
    for (auto It = Visuals.CreateIterator(); It; ++It)
    {
        if (!Existing.Contains(It.Key()))
        {
            for (USceneComponent* Component : It.Value().Components)
            {
                if (IsValid(Component))
                {
                    Component->DestroyComponent();
                }
            }
            It.RemoveCurrent();
        }
    }
}
}

AHomesteadWorld::AHomesteadWorld()
{
    PrimaryActorTick.bCanEverTick = true;
    USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("WorldRoot"));
    SceneRoot->SetMobility(EComponentMobility::Static);
    SetRootComponent(SceneRoot);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeAsset(TEXT("/Engine/BasicShapes/Cone.Cone"));
    Cube = CubeAsset.Object;
    Sphere = SphereAsset.Object;
    Cylinder = CylinderAsset.Object;
    Cone = ConeAsset.Object;
}

void AHomesteadWorld::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (ChunkBaselineBuild && ChunkBaselineBuild->IsReady())
    {
        FHomesteadChunkBaselineBuild Completed = ChunkBaselineBuild->Get();
        ChunkBaselineBuild.Reset();
        if (Descriptor.seed == Completed.World.seed
            && Descriptor.generationVersion == Completed.World.generationVersion)
            for (auto& Chunk : Completed.Chunks)
                ChunkBaselineCache.emplace(Chunk.chunk, MoveTemp(Chunk));
    }
    if (!RegionalDescriptorBuild || !RegionalDescriptorBuild->IsReady()) return;

    FHomesteadRegionalDescriptorBuild Completed = RegionalDescriptorBuild->Get();
    RegionalDescriptorBuild.Reset();
    if (Descriptor.seed == Completed.World.seed
        && Descriptor.generationVersion == Completed.World.generationVersion)
    {
        for (const auto& Entry : Completed.Entries)
        {
            if (RegionalDescriptors.StoreGenerated(
                Completed.World, Entry.ResultStatus, Entry.Result)
                != Homestead::Generation::RegionalCacheStoreResult::Rejected)
            {
                continue;
            }
            if (Entry.ResultStatus != Homestead::RegionalGeneration::Status::Ok)
            {
                RegionalDescriptorFailures[Entry.Region] = Entry.ResultStatus;
                UE_LOG(LogHomesteadWorld, Warning,
                    TEXT("Regional descriptor %d,%d unavailable: %s"),
                    Entry.Region.x, Entry.Region.y,
                    UTF8_TO_TCHAR(Homestead::RegionalGeneration::StatusMessage(Entry.ResultStatus)));
            }
        }
        int32 ReadyChunks = 0;
        int32 PartialChunks = 0;
        int32 IncompleteChunks = 0;
        int32 ReachDescriptors = 0;
        int32 LakeDescriptors = 0;
        for (const auto& Terrain : TerrainChunks)
        {
            Homestead::Generation::LoadedChunkWaterDescriptors Water;
            const auto Status = RegionalDescriptors.DescribeChunkWater(Descriptor,
                {Terrain.Key.X, Terrain.Key.Y}, Water);
            if (Status == Homestead::Generation::RegionalChunkDescriptorStatus::Ready)
            {
                ++ReadyChunks;
                ReachDescriptors += static_cast<int32>(Water.reaches.size());
                LakeDescriptors += static_cast<int32>(Water.lakes.size());
            }
            else if (Status == Homestead::Generation::RegionalChunkDescriptorStatus::Partial)
            {
                ++PartialChunks;
                ReachDescriptors += static_cast<int32>(Water.reaches.size());
                LakeDescriptors += static_cast<int32>(Water.lakes.size());
            }
            else
            {
                ++IncompleteChunks;
            }
        }
        UE_LOG(LogHomesteadWorld, Display,
            TEXT("Regional descriptor cache: loaded=%llu cached=%llu failures=%llu ready_chunks=%d partial_chunks=%d incomplete_chunks=%d reaches=%d lakes=%d builds=%llu"),
            static_cast<unsigned long long>(RegionalDescriptors.LoadedRegionCount()),
            static_cast<unsigned long long>(RegionalDescriptors.CachedRegionCount()),
            static_cast<unsigned long long>(RegionalDescriptorFailures.size()),
            ReadyChunks, PartialChunks, IncompleteChunks, ReachDescriptors, LakeDescriptors,
            static_cast<unsigned long long>(RegionalDescriptorBuildCount));
        if (!RebuildRegionalWater())
            UE_LOG(LogHomesteadWorld, Error, TEXT("Regional water rebuild failed."));
    }
    QueueRegionalDescriptorBuild();
}

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

void AHomesteadWorld::ClearRegionalWater()
{
    for (auto& Entry : RegionalWaterMeshes)
        if (IsValid(Entry.Value.Get())) Entry.Value->DestroyComponent();
    RegionalWaterMeshes.Reset();
    RegionalWaterSignature.Reset();
    RenderedRegionalReachKey.Reset();
    RenderedRegionalReachReferences = 0;
    UnrenderedRegionalReachReferences = 0;
    UnrenderedRegionalLakeReferences = 0;
}

bool AHomesteadWorld::RebuildRegionalWater()
{
    using namespace Homestead::Generation;
    using namespace Homestead::RegionalGeneration;
    TArray<RiverReach> Reaches;
    int32 LakeReferences = 0;
    for (const auto& Terrain : TerrainChunks)
    {
        LoadedChunkWaterDescriptors Water;
        const auto Status = RegionalDescriptors.DescribeChunkWater(
            Descriptor, {Terrain.Key.X, Terrain.Key.Y}, Water);
        if (Status != RegionalChunkDescriptorStatus::Ready
            && Status != RegionalChunkDescriptorStatus::Partial)
            continue;
        LakeReferences += static_cast<int32>(Water.lakes.size());
        for (const auto& Reach : Water.reaches)
            if (!Reaches.ContainsByPredicate([&](const RiverReach& Existing)
                { return Existing.key == Reach.key; }))
                Reaches.Add(Reach);
    }
    const FVector2D ActiveCenter(
        (static_cast<double>(PreparedChunk.x) + 0.5) * ChunkSizeCm,
        (static_cast<double>(PreparedChunk.y) + 0.5) * ChunkSizeCm);
    Reaches.Sort([ActiveCenter](const RiverReach& A, const RiverReach& B)
    {
        const FVector2D MidA(
            (A.key.upstream.x + A.key.downstream.x) * DrainageSpacingCm * 0.5,
            (A.key.upstream.y + A.key.downstream.y) * DrainageSpacingCm * 0.5);
        const FVector2D MidB(
            (B.key.upstream.x + B.key.downstream.x) * DrainageSpacingCm * 0.5,
            (B.key.upstream.y + B.key.downstream.y) * DrainageSpacingCm * 0.5);
        const double DistanceA = FVector2D::DistSquared(MidA, ActiveCenter);
        const double DistanceB = FVector2D::DistSquared(MidB, ActiveCenter);
        return !FMath::IsNearlyEqual(DistanceA, DistanceB)
            ? DistanceA < DistanceB : RegionalReachLess(A, B);
    });
    FString Signature = FString::Printf(TEXT("%llu:%u;"),
        static_cast<unsigned long long>(Descriptor.seed), Descriptor.generationVersion);
    for (const auto& Terrain : TerrainChunks)
        Signature += FString::Printf(TEXT("%d,%d;"), Terrain.Key.X, Terrain.Key.Y);
    if (!Reaches.IsEmpty())
        Signature += FString::Printf(TEXT("%lld,%lld>%lld,%lld;"),
            Reaches[0].key.upstream.x, Reaches[0].key.upstream.y,
            Reaches[0].key.downstream.x, Reaches[0].key.downstream.y);
    if (RegionalWaterSignature == Signature) return true;

    ClearRegionalWater();
    RegionalWaterSignature = Signature;
    UnrenderedRegionalReachReferences = Reaches.Num();
    UnrenderedRegionalLakeReferences = LakeReferences;
    if (Reaches.IsEmpty()) return true;

    const RiverReach& Reach = Reaches[0];
    RenderedRegionalReachKey = FString::Printf(TEXT("%lld,%lld>%lld,%lld"),
        Reach.key.upstream.x, Reach.key.upstream.y,
        Reach.key.downstream.x, Reach.key.downstream.y);
    const double Width = 70.0 + Reach.widthClass * 25.0;
    for (const auto& Terrain : TerrainChunks)
    {
        const double LowX = static_cast<double>(Terrain.Key.X) * ChunkSizeCm;
        const double LowY = static_cast<double>(Terrain.Key.Y) * ChunkSizeCm;
        const double HighX = LowX + ChunkSizeCm;
        const double HighY = LowY + ChunkSizeCm;
        FVector2D A(Reach.key.upstream.x * DrainageSpacingCm,
            Reach.key.upstream.y * DrainageSpacingCm);
        FVector2D B(Reach.key.downstream.x * DrainageSpacingCm,
            Reach.key.downstream.y * DrainageSpacingCm);
        if (!ClipRegionalSegment(A, B, LowX, LowY, HighX, HighY)) continue;
        auto* Mesh = NewObject<UProceduralMeshComponent>(this);
        if (!Mesh) return false;
        Mesh->SetupAttachment(GetRootComponent());
        Mesh->SetMobility(EComponentMobility::Static);
        Mesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetGenerateOverlapEvents(false);
        Mesh->SetCanEverAffectNavigation(false);
        Mesh->SetCastShadow(false);
        Mesh->ComponentTags.Add(TEXT("GeneratedRegionalWater"));
        Mesh->ComponentTags.Add(FName(*RenderedRegionalReachKey));

        const FVector2D Direction = (B - A).GetSafeNormal();
        const FVector2D Perpendicular(-Direction.Y, Direction.X);
        const int32 Steps = FMath::Clamp(
            FMath::CeilToInt(FVector2D::Distance(A, B) / 100.0), 1, 32);
        TArray<FVector> Vertices;
        TArray<int32> Triangles;
        TArray<FVector> Normals;
        TArray<FVector2D> UVs;
        TArray<FLinearColor> Colors;
        TArray<FProcMeshTangent> Tangents;
        for (int32 Step = 0; Step <= Steps; ++Step)
        {
            const double Alpha = static_cast<double>(Step) / Steps;
            const FVector2D Center = FMath::Lerp(A, B, Alpha);
            const double Z = GroundHeight(Center.X, Center.Y) + 3.0;
            Vertices.Add(FVector(Center - Perpendicular * Width, Z));
            Vertices.Add(FVector(Center + Perpendicular * Width, Z));
            Normals.Add(FVector::UpVector); Normals.Add(FVector::UpVector);
            UVs.Add(FVector2D(0, Alpha)); UVs.Add(FVector2D(1, Alpha));
            Colors.Add(FLinearColor::White); Colors.Add(FLinearColor::White);
            Tangents.Add(FProcMeshTangent(Direction.X, Direction.Y, 0));
            Tangents.Add(FProcMeshTangent(Direction.X, Direction.Y, 0));
            if (Step < Steps)
            {
                const int32 Index = Step * 2;
                Triangles.Append({Index, Index + 2, Index + 1,
                    Index + 1, Index + 2, Index + 3});
            }
        }
        Mesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals,
            UVs, Colors, Tangents, false);
        Mesh->SetMaterial(0, Material(FLinearColor(0.075f, 0.26f, 0.29f), 0.16f));
        Mesh->RegisterComponent();
        RegionalWaterMeshes.Add(Terrain.Key, Mesh);
        ++RenderedRegionalReachReferences;
    }
    UnrenderedRegionalReachReferences = FMath::Max(
        0, UnrenderedRegionalReachReferences - (RenderedRegionalReachReferences > 0 ? 1 : 0));
    return true;
}

float AHomesteadWorld::GroundHeight(float X, float Y, Homestead::Generation::WorldDescriptor World)
{
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

float AHomesteadWorld::CellBase(int CellX, int CellY) const
{
    const Homestead::Point Center = Homestead::CellCenter(CellX, CellY);
    float Height = GroundHeight(Center.x, Center.y);
    for (int X : {-1, 1})
    {
        for (int Y : {-1, 1})
        {
            Height = FMath::Max(Height, GroundHeight(Center.x + X * 150.0, Center.y + Y * 150.0));
        }
    }
    return Height + 16.0f;
}

UMaterialInterface* AHomesteadWorld::Material(FLinearColor Color, float Roughness, float Glow)
{
    const FName Key = MaterialKey(Color, Roughness, Glow);
    if (TObjectPtr<UMaterialInstanceDynamic>* Existing = Materials.Find(Key))
    {
        return Existing->Get();
    }
    const bool bTexturedRock = RockMaterial && Color.Equals(Stone);
    UMaterialInstanceDynamic* Instance = UMaterialInstanceDynamic::Create(
        bTexturedRock ? RockMaterial.Get() : FieldMaterial.Get(), this);
    if (!Instance)
    {
        return UMaterial::GetDefaultMaterial(MD_Surface);
    }
    Instance->SetVectorParameterValue(TEXT("Tint"), bTexturedRock ? FLinearColor::White : Color);
    Instance->SetScalarParameterValue(TEXT("Roughness"), Roughness);
    Instance->SetScalarParameterValue(TEXT("Glow"), Glow);
    Materials.Add(Key, Instance);
    return Instance;
}

UStaticMeshComponent* AHomesteadWorld::AddPart(FHomesteadWorldVisual& Visual, UStaticMesh* Mesh,
    const FVector& Position, const FVector& Size, FLinearColor Color, bool bCollision,
    const FRotator& Rotation, float Roughness, float Glow)
{
    if (!Mesh)
    {
        return nullptr;
    }
    UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this);
    Part->SetupAttachment(GetRootComponent());
    Part->SetMobility(EComponentMobility::Movable);
    Part->SetStaticMesh(Mesh);
    Part->SetMaterial(0, Material(Color, Roughness, Glow));
    Part->SetRelativeTransform(FTransform(Rotation, Position, Size / 100.0f));
    Part->SetCollisionProfileName(bCollision ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
    Part->SetGenerateOverlapEvents(false);
    Part->SetCanEverAffectNavigation(bCollision);
    Part->SetCastShadow(Glow <= 0.0f);
    Part->RegisterComponent();
    Visual.Components.Add(Part);
    return Part;
}

void AHomesteadWorld::AddDecoration(UStaticMesh* Mesh, const FVector& Position, const FVector& Size,
    FLinearColor Color, bool bCollision, const FRotator& Rotation, bool bHideMesh)
{
    if (!Mesh)
    {
        return;
    }
    const FName Key(*FString::Printf(TEXT("%s_%s_%d_%d"), *Mesh->GetName(),
        *MaterialKey(Color, 0.85f, 0.0f).ToString(), bCollision, bHideMesh));
    UHierarchicalInstancedStaticMeshComponent* Batch = nullptr;
    if (TObjectPtr<UHierarchicalInstancedStaticMeshComponent>* Existing = DecorationBatches.Find(Key))
    {
        Batch = Existing->Get();
    }
    else
    {
        Batch = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
        Batch->SetupAttachment(GetRootComponent());
        Batch->SetMobility(EComponentMobility::Static);
        Batch->SetStaticMesh(Mesh);
        for (int Slot = 0; Slot < FMath::Max(1, Mesh->GetStaticMaterials().Num()); ++Slot)
        {
            Batch->SetMaterial(Slot, Material(Color));
        }
        Batch->SetCollisionProfileName(bCollision ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
        Batch->SetGenerateOverlapEvents(false);
        Batch->SetCanEverAffectNavigation(bCollision);
        Batch->SetCullDistances(bCollision ? 0 : 4500, bCollision ? 0 : 7000);
        Batch->bAutoRebuildTreeOnInstanceChanges = false;
        Batch->SetVisibility(!bHideMesh);
        Batch->SetCastShadow(!bHideMesh);
        Batch->RegisterComponent();
        DecorationBatches.Add(Key, Batch);
    }
    // Imported FBX assets need not share the engine primitives' centered 100 cm bounds.
    const FBox Bounds = Mesh->GetBoundingBox();
    const FVector Dimensions = Bounds.GetSize().ComponentMax(FVector(0.01f));
    const FVector Scale = Size / Dimensions;
    const FVector Origin = Position - Rotation.RotateVector(Bounds.GetCenter() * Scale);
    Batch->AddInstance(FTransform(Rotation, Origin, Scale));
}

bool AHomesteadWorld::IsPreparedFor(const Homestead::State& State) const
{
    return bTerrainReady && Descriptor.seed == State.world.seed
        && Descriptor.generationVersion == State.world.generationVersion && PreparedChunk == State.activeChunk;
}

int32 AHomesteadWorld::StartingViewObstructions(FVector Focus, FVector Camera) const
{
    int32 Count = 0;
    for (const auto& Entry : ActiveTreeInstances)
    {
        const auto* Batch = ActiveTreeBatches.FindRef(Entry.Value.Visual.MeshPath).Get();
        const UStaticMesh* Mesh = Batch ? Batch->GetStaticMesh().Get() : nullptr;
        if (!Mesh) continue;
        const FTransform& Transform = Entry.Value.Visual.Transform;
        const FVector Start = Transform.InverseTransformPosition(Focus);
        const FVector End = Transform.InverseTransformPosition(Camera);
        const FBox Bounds = Mesh->GetBoundingBox().ExpandBy(20);
        Count += FMath::LineBoxIntersection(Bounds, Start, End, End - Start) ? 1 : 0;
    }
    return Count;
}

bool AHomesteadWorld::BuildTerrain(const Homestead::State& State)
{
    if (IsPreparedFor(State)) return true;
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
    for (int Y = -2; Y <= 2; ++Y)
        for (int X = -2; X <= 2; ++X)
        {
            const FIntPoint Key(State.activeChunk.x + X, State.activeChunk.y + Y);
            if (SameWorld && TerrainChunks.Contains(Key)) continue;
            Gen::ChunkBaseline Baseline;
            const bool Generated = GetChunkBaseline(State.world, {Key.X, Key.Y}, Baseline);
            auto* Mesh = Generated
                ? BuildTerrainChunk(Baseline, State.world, FMath::Abs(X) <= 1 && FMath::Abs(Y) <= 1) : nullptr;
            if (!Mesh)
            {
                for (auto& Entry : Prepared) Entry.Value.Terrain->DestroyComponent();
                UE_LOG(LogHomesteadWorld, Error, TEXT("Chunk %d,%d preparation failed; previous terrain retained."),
                    Key.X, Key.Y);
                return false;
            }
            FHomesteadTerrainChunk Chunk;
            Chunk.Terrain = Mesh;
            Chunk.bCollision = FMath::Abs(X) <= 1 && FMath::Abs(Y) <= 1;
            Prepared.Add(Key, MoveTemp(Chunk));
        }
    // Every destination tile exists and its synchronous collision cook has completed.
    for (auto It = TerrainChunks.CreateIterator(); It; ++It)
    {
        if (!SameWorld || FMath::Abs(It.Key().X - State.activeChunk.x) > 2
            || FMath::Abs(It.Key().Y - State.activeChunk.y) > 2)
        {
            ClearVisual(It.Value().Cover);
            It.Value().Terrain->DestroyComponent();
            It.RemoveCurrent();
        }
    }
    for (auto& Entry : Prepared) TerrainChunks.Add(Entry.Key, MoveTemp(Entry.Value));
    for (auto& Entry : TerrainChunks)
    {
        const bool Colliding = FMath::Abs(Entry.Key.X - State.activeChunk.x) <= 1
            && FMath::Abs(Entry.Key.Y - State.activeChunk.y) <= 1;
        Entry.Value.Terrain->SetCollisionEnabled(Colliding ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
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
    QueueChunkBaselineBuild(State.world, State.activeChunk);
    UE_LOG(LogHomesteadWorld, Display, TEXT("Generated terrain: seed=%llu version=%u center=%d,%d tiles=%d colliding=9 vertices_per_tile=625 prepare_ms=%.3f baseline_hits=%llu baseline_misses=%llu baseline_async_builds=%llu"),
        static_cast<unsigned long long>(Descriptor.seed), Descriptor.generationVersion,
        PreparedChunk.x, PreparedChunk.y, TerrainChunks.Num(), LastTerrainPrepareMilliseconds,
        static_cast<unsigned long long>(ChunkBaselineCacheHits),
        static_cast<unsigned long long>(ChunkBaselineCacheMisses),
        static_cast<unsigned long long>(ChunkBaselineBuildCount));
    return true;
}

bool AHomesteadWorld::RefreshRegionalDescriptors(
    Homestead::Generation::WorldDescriptor World,
    const std::vector<Homestead::RegionalGeneration::RegionCoord>& Regions)
{
    const bool SameWorld = RegionalDescriptors.IsForWorld(World);
    const auto Status = RegionalDescriptors.RefreshLoadedRegions(World, Regions);
    if (Status != Homestead::RegionalGeneration::Status::Ok)
    {
        UE_LOG(LogHomesteadWorld, Error, TEXT("Loaded regional cache refresh failed: %s"),
            UTF8_TO_TCHAR(Homestead::RegionalGeneration::StatusMessage(Status)));
        return false;
    }

    DesiredRegionalDescriptors = Regions;
    std::sort(DesiredRegionalDescriptors.begin(), DesiredRegionalDescriptors.end());
    DesiredRegionalDescriptors.erase(
        std::unique(DesiredRegionalDescriptors.begin(), DesiredRegionalDescriptors.end()),
        DesiredRegionalDescriptors.end());
    if (!SameWorld)
    {
        RegionalDescriptorFailures.clear();
    }
    else
    {
        for (auto Iterator = RegionalDescriptorFailures.begin();
            Iterator != RegionalDescriptorFailures.end();)
        {
            if (!std::binary_search(DesiredRegionalDescriptors.begin(),
                DesiredRegionalDescriptors.end(), Iterator->first))
                Iterator = RegionalDescriptorFailures.erase(Iterator);
            else
                ++Iterator;
        }
    }
    QueueRegionalDescriptorBuild();
    return true;
}

void AHomesteadWorld::QueueRegionalDescriptorBuild()
{
    if (RegionalDescriptorBuild) return;

    std::vector<Homestead::RegionalGeneration::RegionCoord> Missing;
    for (const auto Region : DesiredRegionalDescriptors)
        if (!RegionalDescriptors.Find(Descriptor, Region)
            && RegionalDescriptorFailures.find(Region) == RegionalDescriptorFailures.end())
            Missing.push_back(Region);
    if (Missing.empty()) return;

    const auto World = Descriptor;
    RegionalDescriptorBuildCount += Missing.size();
    RegionalDescriptorBuild = MakeUnique<TFuture<FHomesteadRegionalDescriptorBuild>>(
        Async(EAsyncExecution::ThreadPool, [World, Missing = std::move(Missing)]()
        {
            FHomesteadRegionalDescriptorBuild Build;
            Build.World = World;
            const Homestead::RegionalGeneration::RegionalDescriptor RegionalWorld{
                World.seed, Homestead::RegionalGeneration::RegionalGenerationVersion};
            Build.Entries.resize(Missing.size());
            ParallelFor(static_cast<int32>(Missing.size()), [&](int32 Index)
            {
                auto& Entry = Build.Entries[Index];
                const auto Region = Missing[Index];
                Entry.Region = Region;
                Entry.ResultStatus = Homestead::RegionalGeneration::GenerateRegion(
                    RegionalWorld, Region, Entry.Result);
            });
            return Build;
        }));
}

UProceduralMeshComponent* AHomesteadWorld::BuildTerrainChunk(
    const Homestead::Generation::ChunkBaseline& Baseline, Homestead::Generation::WorldDescriptor World, bool bCollision)
{
    namespace Gen = Homestead::Generation;
    auto* Mesh = NewObject<UProceduralMeshComponent>(this);
    Mesh->SetupAttachment(GetRootComponent());
    Mesh->SetMobility(EComponentMobility::Static);
    Mesh->bUseAsyncCooking = false;
    Mesh->bUseComplexAsSimpleCollision = true;
    Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
    Mesh->SetGenerateOverlapEvents(false);
    Mesh->RegisterComponent();

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
    Mesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UV, Colors, Tangents, true);
    Mesh->SetMaterial(0, GroundMaterial ? GroundMaterial.Get() : Material(Meadow));
    if (!Mesh->GetBodySetup() || !Mesh->GetBodySetup()->bCreatedPhysicsMeshes
        || Mesh->GetBodySetup()->bFailedToCreatePhysicsMeshes)
    {
        UE_LOG(LogHomesteadWorld, Error, TEXT("Generated terrain collision cook did not produce physics meshes."));
        Mesh->DestroyComponent();
        return nullptr;
    }

    // The colliding terrain carries the muddy bank blend; only water needs an overlay.
    auto WaterRibbon = [&](int Section)
    {
        Vertices.Reset();
        Triangles.Reset();
        Normals.Reset();
        UV.Reset();
        Colors.Reset();
        Tangents.Reset();
        constexpr int Columns = 1;
        for (int Y = 0; Y <= Cells; ++Y)
        {
            const float PY = OriginY + Y * Spacing;
            const float Center = Homestead::StreamX(PY);
            const double Left = Gen::CreekWaterHalfWidthCm(World, PY, false);
            const double Right = Gen::CreekWaterHalfWidthCm(World, PY, true);
            for (int X = 0; X <= Columns; ++X)
            {
                const double PX = FMath::Clamp<double>(Center + (X == 0 ? -Left : Right),
                    OriginX, OriginX + Gen::ChunkSizeCm);
                Gen::TerrainSample Sample;
                const auto Status = Gen::SampleTerrain(World, FMath::RoundToInt64(PX), FMath::RoundToInt64(PY), Sample);
                if (Status != Gen::Status::Ok)
                {
                    UE_LOG(LogHomesteadWorld, Error, TEXT("Generated ribbon sample failed: %s"), UTF8_TO_TCHAR(Gen::StatusMessage(Status)));
                    return false;
                }
                const float Z = Sample.waterHeightCm;
                Vertices.Add(FVector(PX, PY, Z));
                Normals.Add(FVector::UpVector);
                UV.Add(FVector2D((PX - Center) / 100.0f, PY / 300.0f));
                Tangents.Add(FProcMeshTangent(1, 0, 0));
                if (Y < Cells && X < Columns)
                {
                    const int A = Y * (Columns + 1) + X;
                    Triangles.Append({A, A + Columns + 1, A + 1,
                        A + 1, A + Columns + 1, A + Columns + 2});
                }
            }
        }
        Mesh->CreateMeshSection_LinearColor(Section, Vertices, Triangles, Normals, UV, Colors, Tangents, false);
        Mesh->SetMaterial(Section, Material(FLinearColor(0.075f, 0.26f, 0.29f), 0.16f));
        return true;
    };
    if (OriginX <= 1680 + Gen::CreekWaterMaximumHalfWidthCm
        && OriginX + Gen::ChunkSizeCm >= 1320 - Gen::CreekWaterMaximumHalfWidthCm)
    {
        if (!WaterRibbon(1))
        {
            Mesh->DestroyComponent();
            return nullptr;
        }
    }
    Mesh->SetCollisionEnabled(bCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    return Mesh;
}

void AHomesteadWorld::BuildLighting()
{
    Exposure = NewObject<UPostProcessComponent>(this, TEXT("MeadowExposure"));
    Exposure->SetupAttachment(GetRootComponent());
    Exposure->bUnbound = true;
    Exposure->BlendWeight = 1.0f;
    FPostProcessSettings& Settings = Exposure->Settings;
    Settings.bOverride_AutoExposureMethod = true;
    Settings.AutoExposureMethod = AEM_Histogram;
    // Extended luminance range makes these EV100: accommodate the physical sun,
    // but cap dark adaptation so moonlit ground is not exposed like daylight.
    Settings.bOverride_AutoExposureMinBrightness = true;
    Settings.AutoExposureMinBrightness = 0.0f;
    Settings.bOverride_AutoExposureMaxBrightness = true;
    Settings.AutoExposureMaxBrightness = 16.0f;
    Settings.bOverride_AutoExposureBias = true;
    Settings.AutoExposureBias = -0.15f;
    Settings.bOverride_AutoExposureSpeedUp = true;
    Settings.AutoExposureSpeedUp = 3.0f;
    Settings.bOverride_AutoExposureSpeedDown = true;
    Settings.AutoExposureSpeedDown = 1.0f;
    Exposure->RegisterComponent();

    Sun = NewObject<UDirectionalLightComponent>(this, TEXT("MeadowSun"));
    Sun->SetupAttachment(GetRootComponent());
    Sun->SetMobility(EComponentMobility::Movable);
    Sun->bAtmosphereSunLight = true;
    Sun->SetIntensity(46000.0f);
    Sun->RegisterComponent();

    Moon = NewObject<UDirectionalLightComponent>(this, TEXT("MeadowMoonlight"));
    Moon->SetupAttachment(GetRootComponent());
    Moon->SetMobility(EComponentMobility::Movable);
    Moon->bAtmosphereSunLight = true;
    Moon->AtmosphereSunLightIndex = 1;
    Moon->SetLightColor(FLinearColor(0.53f, 0.66f, 1.0f));
    Moon->SetIntensity(0.5f);
    Moon->RegisterComponent();

    Sky = NewObject<USkyLightComponent>(this, TEXT("MeadowSkyLight"));
    Sky->SetupAttachment(GetRootComponent());
    Sky->SetMobility(EComponentMobility::Movable);
    Sky->bRealTimeCapture = true;
    Sky->SetIntensity(1.0f);
    Sky->RegisterComponent();

    USkyAtmosphereComponent* Atmosphere = NewObject<USkyAtmosphereComponent>(this, TEXT("MeadowAtmosphere"));
    Atmosphere->SetupAttachment(GetRootComponent());
    Atmosphere->RegisterComponent();

    Fog = NewObject<UExponentialHeightFogComponent>(this, TEXT("MeadowDistanceHaze"));
    Fog->SetupAttachment(GetRootComponent());
    Fog->SetMobility(EComponentMobility::Movable);
    Fog->SetFogDensity(0.007f);
    Fog->SetFogHeightFalloff(0.3f);
    Fog->SetStartDistance(1100.0f);
    Fog->RegisterComponent();
}

bool AHomesteadWorld::IsDecorationReserved(const Homestead::State& State, float X, float Y,
    float FootprintRadius, float CanopyRadius, bool bLowCover)
{
    const float OccupiedRadius = FMath::Max(FootprintRadius, CanopyRadius);
    for (const auto& Node : State.resources)
    {
        if (FVector2D(X - Node.position.x, Y - Node.position.y).Size()
            < FootprintRadius + (bLowCover ? 35.0f : 130.0f))
            return true;
        if (Node.cleared)
        {
            const auto Center = Homestead::CellCenter(
                FMath::FloorToInt(Node.position.x / Homestead::CellSize),
                FMath::FloorToInt(Node.position.y / Homestead::CellSize));
            const float DX = FMath::Max(0.0, FMath::Abs(X - Center.x) - Homestead::CellSize * 0.5);
            const float DY = FMath::Max(0.0, FMath::Abs(Y - Center.y) - Homestead::CellSize * 0.5);
            if (DX * DX + DY * DY <= OccupiedRadius * OccupiedRadius)
                return true;
        }
    }
    for (const auto& Structure : State.structures)
    {
        const auto Center = Homestead::CellCenter(Structure.cellX, Structure.cellY);
        if (FVector2D(X - Center.x, Y - Center.y).Size() < OccupiedRadius + 225.0f)
            return true;
    }
    for (const auto& Plot : State.plots)
    {
        const auto Center = Homestead::CellCenter(Plot.cellX, Plot.cellY);
        if (FVector2D(X - Center.x, Y - Center.y).Size() < OccupiedRadius + 175.0f)
            return true;
    }
    return false;
}

bool AHomesteadWorld::BuildDecorations(const Homestead::Simulation& Simulation)
{
    const auto& State = Simulation.GetState();
    const double Started = FPlatformTime::Seconds();
    const FName FernTag(TEXT("AuthoredFern02"));
    const FName GrassTag(TEXT("AuthoredGrassMedium01"));
    const FName FlowerTag(TEXT("DecorativeWildflower"));
    TArray<UStaticMesh*> FernMeshes;
    for (const TCHAR* Suffix : {TEXT("a"), TEXT("b"), TEXT("c"), TEXT("d")})
    {
        const FString Path = FString::Printf(
            TEXT("/Game/Trials/Fern02_20260920_01/Meshes/SM_Fern02_%s.SM_Fern02_%s"), Suffix, Suffix);
        UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Path);
        if (!Mesh || Mesh->GetStaticMaterials().Num() != 1 || !Mesh->GetStaticMaterials()[0].MaterialInterface)
        {
            UE_LOG(LogHomesteadWorld, Error, TEXT("Authored woodland fern is unavailable or has no material: %s"), *Path);
            FernMeshes.Reset();
            return false;
        }
        FernMeshes.Add(Mesh);
    }
    TArray<UStaticMesh*> GrassMeshes;
    const TCHAR* GrassNames[] = { TEXT("mid_b"), TEXT("small_b"), TEXT("tall_a"), TEXT("tiny_a") };
    const int32 GrassTriangles[] = { 1257, 653, 290, 79 };
    for (int32 Index = 0; Index < 4; ++Index)
    {
        const FString Path = FString::Printf(
            TEXT("/Game/Trials/GrassGround_20260921_01/Meshes/SM_GrassMedium01_%s.SM_GrassMedium01_%s"),
            GrassNames[Index], GrassNames[Index]);
        auto* Mesh = LoadObject<UStaticMesh>(nullptr, *Path);
        if (!Mesh || Mesh->GetStaticMaterials().Num() != 1 || !Mesh->GetMaterial(0)
            || Mesh->GetMaterial(0)->GetPathName() != TEXT("/Game/Trials/GrassGround_20260921_01/Materials/M_GrassMedium01.M_GrassMedium01")
            || !Mesh->GetRenderData() || Mesh->GetRenderData()->LODResources.Num() != 1
            || Mesh->GetRenderData()->LODResources[0].GetNumTriangles() != GrassTriangles[Index])
        {
            UE_LOG(LogHomesteadWorld, Error, TEXT("Admitted authored grass is missing or differs: %s"), *Path);
            GrassMeshes.Reset();
            return false;
        }
        GrassMeshes.Add(Mesh);
    }
    TArray<UStaticMesh*> FlowerMeshes;
    for (const TCHAR* Suffix : {TEXT("a"), TEXT("b")})
    {
        const FString Path = FString::Printf(
            TEXT("/Game/Trials/WoodlandResources_20260921_01/Meshes/SM_FlowerEmpodium_%s.SM_FlowerEmpodium_%s"),
            Suffix, Suffix);
        auto* Mesh = LoadObject<UStaticMesh>(nullptr, *Path);
        if (!Mesh || Mesh->GetStaticMaterials().Num() != 1 || !Mesh->GetMaterial(0)
            || Mesh->GetMaterial(0)->GetPathName() != TEXT("/Game/Trials/WoodlandResources_20260921_01/Materials/M_FlowerEmpodium.M_FlowerEmpodium")
            || !Mesh->GetRenderData() || Mesh->GetRenderData()->LODResources.Num() != 1
            || Mesh->GetRenderData()->LODResources[0].GetNumTriangles() != 758)
        {
            UE_LOG(LogHomesteadWorld, Error, TEXT("Admitted decorative flower is missing or differs: %s"), *Path);
            return false;
        }
        FlowerMeshes.Add(Mesh);
    }
    int32 GrassCount = 0;
    int32 GrassTriangleCount = 0;
    int32 FernCount = 0;
    int32 BankGrassCount = 0;
    int32 BankFernCount = 0;
    int32 FlowerCount = 0;
    int32 FlowerTriangleCount = 0;
    int32 RebuiltChunks = 0;
    for (auto& Chunk : TerrainChunks)
    {
        const double OriginX = static_cast<int64>(Chunk.Key.X) * Homestead::Generation::ChunkSizeCm;
        const double OriginY = static_cast<int64>(Chunk.Key.Y) * Homestead::Generation::ChunkSizeCm;
        auto Nearby = [&](double X, double Y)
        {
            return X >= OriginX - 450 && X <= OriginX + 2850 && Y >= OriginY - 450 && Y <= OriginY + 2850;
        };
        FString Signature;
        for (const auto& Edit : State.resourceEdits)
            if (Edit.cleared)
            {
                Homestead::Generation::GeneratedEntity Entity;
                if (Homestead::Generation::FindEntity(State.world, Edit.key, Entity) == Homestead::Generation::Status::Ok
                    && Nearby(Entity.xCm, Entity.yCm))
                    Signature += FString::Printf(TEXT("R%d,%d,%u;"), Edit.key.chunk.x, Edit.key.chunk.y, Edit.key.localId);
            }
        for (const auto& Structure : State.structures)
            if (Nearby((Structure.cellX + 0.5) * Homestead::CellSize, (Structure.cellY + 0.5) * Homestead::CellSize))
                Signature += FString::Printf(TEXT("S%d;"), Structure.id);
        for (const auto& Plot : State.plots)
            if (Nearby((Plot.cellX + 0.5) * Homestead::CellSize, (Plot.cellY + 0.5) * Homestead::CellSize))
                Signature += FString::Printf(TEXT("P%d;"), Plot.id);
        Signature += TEXT("natural-creek-v1-decorative-wildflower-v1");
        if (Chunk.Value.CoverSignature == Signature) continue;
        Homestead::State CoverState;
        CoverState.structures = State.structures;
        CoverState.plots = State.plots;
        for (int DY = -1; DY <= 1; ++DY)
            for (int DX = -1; DX <= 1; ++DX)
            {
                Homestead::Generation::ChunkBaseline Baseline;
                if (!GetChunkBaseline(State.world,
                    {Chunk.Key.X + DX, Chunk.Key.Y + DY}, Baseline))
                {
                    UE_LOG(LogHomesteadWorld, Error, TEXT("Cover resource generation failed."));
                    return false;
                }
                for (const auto& Entity : Baseline.entities)
                {
                    Homestead::ResourceNode Node;
                    const auto Active = std::find_if(State.resources.begin(), State.resources.end(),
                        [&](const Homestead::ResourceNode& Value) { return Value.key == Entity.key; });
                    if (Active != State.resources.end()) Node = *Active;
                    else
                    {
                        Node.key = Entity.key;
                        Node.position = {static_cast<double>(Entity.xCm), static_cast<double>(Entity.yCm)};
                        Node.kind = ResourceKindFor(Entity.kind);
                        if (Node.kind == Homestead::ResourceKind::Count) continue;
                        const auto Edit = std::lower_bound(State.resourceEdits.begin(), State.resourceEdits.end(),
                            Entity.key, [](const Homestead::ResourceEdit& Value,
                                const Homestead::Generation::GeneratedEntityKey& Key) { return Value.key < Key; });
                        if (Edit != State.resourceEdits.end() && Edit->key == Entity.key)
                        {
                            Node.cleared = Edit->cleared;
                            Node.readyAtHour = Edit->readyAtHour;
                        }
                    }
                    CoverState.resources.push_back(Node);
                }
            }
        ClearVisual(Chunk.Value.Cover);
        ++RebuiltChunks;
        const uint32 Seed = GetTypeHash(State.world.seed) ^ GetTypeHash(Chunk.Key);
        FRandomStream Random(static_cast<int32>(Seed));
        FRandomStream FlowerRandom(static_cast<int32>(Seed ^ 0x8DA6B343u));
        auto CreateCoverBatch = [&](UStaticMesh* Mesh, FName Tag, int32 StartCullDistance,
            int32 EndCullDistance)
        {
            auto* Batch = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
            Batch->SetupAttachment(GetRootComponent());
            Batch->SetMobility(EComponentMobility::Static);
            Batch->ComponentTags.Add(Tag);
            Batch->SetStaticMesh(Mesh);
            Batch->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
            Batch->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Batch->SetGenerateOverlapEvents(false);
            Batch->SetCanEverAffectNavigation(false);
            Batch->SetCullDistances(StartCullDistance, EndCullDistance);
            Batch->SetVisibility(true);
            Batch->SetHiddenInGame(false);
            Batch->SetCastShadow(false);
            Batch->bAutoRebuildTreeOnInstanceChanges = false;
            Batch->RegisterComponent();
            Chunk.Value.Cover.Components.Add(Batch);
            return Batch;
        };
        TArray<UHierarchicalInstancedStaticMeshComponent*> GrassBatches;
        if (GrassMeshes.Num() == 4)
            for (auto* Mesh : GrassMeshes)
            {
                GrassBatches.Add(CreateCoverBatch(Mesh, GrassTag, 3500, 5000));
            }
        TMap<FString, UHierarchicalInstancedStaticMeshComponent*> FernBatches;
        if (FernMeshes.Num() == 4)
            for (auto* Mesh : FernMeshes)
                FernBatches.Add(Mesh->GetPathName(), CreateCoverBatch(Mesh, FernTag, 0, 5000));
        TArray<UHierarchicalInstancedStaticMeshComponent*> FlowerBatches;
        for (auto* Mesh : FlowerMeshes)
            FlowerBatches.Add(CreateCoverBatch(Mesh, FlowerTag, 3000, 4800));
        for (int32 Attempt = 0; Attempt < 1200; ++Attempt)
        {
            const double X = OriginX + Random.FRandRange(0, 2399.99f);
            const double Y = OriginY + Random.FRandRange(0, 2399.99f);
            const FRotator Rotation(0, Random.FRandRange(0, 360), 0);
            const double StreamDistance = FMath::Abs(X - Homestead::StreamX(Y));
            const bool bCreekBank = StreamDistance >= 100.0 && StreamDistance < 215.0
                && Attempt % 5 == 0;
            if (IsDecorationReserved(CoverState, X, Y, 20, 0, true)
                || (StreamDistance < 215.0 && !bCreekBank))
                continue;
            const int32 Variety = Attempt % 16;
            const int32 Index = Variety == 0 ? 0 : Variety < 3 ? 1 : Variety < 12 ? 2 : 3;
            if (GrassBatches.Num() == 4)
            {
                const FBox Bounds = GrassMeshes[Index]->GetBoundingBox();
                const FVector Anchor(Bounds.GetCenter().X, Bounds.GetCenter().Y, Bounds.Min.Z);
                GrassBatches[Index]->AddInstance(FTransform(Rotation,
                    AtGround(X, Y) - Rotation.RotateVector(Anchor), FVector::OneVector));
                ++GrassCount;
                GrassTriangleCount += GrassTriangles[Index];
                if (bCreekBank) ++BankGrassCount;
            }
            if (Attempt % 32 == 0 && FernMeshes.Num() == 4 && !IsDecorationReserved(CoverState, X, Y, 75, 0, true))
            {
                auto* Mesh = FernMeshes[(Attempt / 32) % 4];
                const FBox Bounds = Mesh->GetBoundingBox();
                const FVector Anchor(Bounds.GetCenter().X, Bounds.GetCenter().Y, Bounds.Min.Z);
                FernBatches.FindChecked(Mesh->GetPathName())->AddInstance(FTransform(Rotation,
                    AtGround(X, Y) - Rotation.RotateVector(Anchor), FVector::OneVector));
                ++FernCount;
                if (bCreekBank) ++BankFernCount;
            }
            if (Attempt % 64 == 17 && FlowerBatches.Num() == 2 && StreamDistance >= 260.0
                && !IsDecorationReserved(CoverState, X, Y, 55, 0, true))
            {
                const int32 FlowerIndex = (Attempt / 64) % 2;
                auto* Mesh = FlowerMeshes[FlowerIndex];
                const FBox Bounds = Mesh->GetBoundingBox();
                const float Scale = FlowerRandom.FRandRange(0.55f, 0.80f);
                const FVector Anchor(Bounds.GetCenter().X, Bounds.GetCenter().Y, Bounds.Min.Z);
                FlowerBatches[FlowerIndex]->AddInstance(FTransform(Rotation,
                    AtGround(X, Y) - Rotation.RotateVector(Anchor * Scale), FVector(Scale)));
                ++FlowerCount;
                FlowerTriangleCount += 758;
            }
        }
        for (auto* Batch : GrassBatches) Batch->BuildTreeIfOutdated(false, true);
        for (const auto& Batch : FernBatches) Batch.Value->BuildTreeIfOutdated(false, true);
        for (auto* Batch : FlowerBatches) Batch->BuildTreeIfOutdated(false, true);
        Chunk.Value.CoverSignature = Signature;
    }
    DecorationBuildMilliseconds = (FPlatformTime::Seconds() - Started) * 1000;
    LastCoverPrepareMilliseconds = DecorationBuildMilliseconds;
    UE_LOG(LogHomesteadWorld, Display, TEXT("Generated cover refresh: rebuilt_chunks=%d added_ferns=%d added_grass=%d added_flowers=%d bank_ferns=%d bank_grass=%d added_grass_triangles=%d added_flower_triangles=%d elapsed_ms=%.3f; CPU wall time, not GPU frame cost."),
        RebuiltChunks, FernCount, GrassCount, FlowerCount, BankFernCount, BankGrassCount,
        GrassTriangleCount, FlowerTriangleCount, DecorationBuildMilliseconds);
    return true;
}

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
}

bool AHomesteadWorld::ResolveGeneratedTreeVisual(const Homestead::ResourceNode& Node, UStaticMesh*& Mesh,
    FHomesteadOuterTreeInstance& Instance)
{
    Homestead::Generation::GeneratedEntity Entity;
    if (Node.kind != Homestead::ResourceKind::ForestTree
        || Homestead::Generation::FindEntity(Descriptor, Node.key, Entity) != Homestead::Generation::Status::Ok)
    {
        UE_LOG(LogHomesteadWorld, Error, TEXT("Generated tree key cannot resolve; no visual substitute."));
        return false;
    }
    FString RequestedPath;
    switch (Entity.paletteRole)
    {
    case Homestead::Generation::TreePaletteRole::BroadleafMature:
        RequestedPath = TEXT("/Game/Trials/WoodlandResources_20260921_01/Meshes/SM_TreeSmall02_Woodland.SM_TreeSmall02_Woodland");
        break;
    case Homestead::Generation::TreePaletteRole::ConiferMature:
        RequestedPath = TEXT("/Game/Trials/MatureFir_20260922_02/Meshes/SM_MatureFir.SM_MatureFir");
        break;
    case Homestead::Generation::TreePaletteRole::WoodlandAccent:
        RequestedPath = TEXT("/Game/Trials/TreePalette_20260921_01/Meshes/SM_Jacaranda.SM_Jacaranda");
        break;
    default:
        UE_LOG(LogHomesteadWorld, Error, TEXT("Generated mature tree has illegal palette role %d."),
            static_cast<int32>(Entity.paletteRole));
        return false;
    }
    Mesh = LoadObject<UStaticMesh>(nullptr, *RequestedPath);
    const int32 ExpectedSlots = Entity.paletteRole
        == Homestead::Generation::TreePaletteRole::ConiferMature ? 4 : 3;
    if (!Mesh || !Mesh->GetBodySetup() || Mesh->GetBodySetup()->AggGeom.SphylElems.Num() != 1
        || Mesh->GetStaticMaterials().Num() != ExpectedSlots
        || !Mesh->GetRenderData() || Mesh->GetRenderData()->LODResources.Num() != 3)
    {
        UE_LOG(LogHomesteadWorld, Error,
            TEXT("Generated tree role=%d variant=%u has missing native material/LOD/collision at %s; no substitute."),
            static_cast<int32>(Entity.paletteRole), Entity.variantIndex, *RequestedPath);
        return false;
    }
    for (int32 Slot = 0; Slot < ExpectedSlots; ++Slot)
        if (!Mesh->GetMaterial(Slot))
        {
            UE_LOG(LogHomesteadWorld, Error, TEXT("Generated tree material slot %d is missing at %s."),
                Slot, *RequestedPath);
            return false;
        }
    const FRotator Rotation(0, Entity.yawDegrees, 0);
    const float Scale = Entity.scalePermille / 1000.0f;
    const auto& Capsule = Mesh->GetBodySetup()->AggGeom.SphylElems[0];
    const FVector Anchor(Capsule.Center.X, Capsule.Center.Y, Mesh->GetBoundingBox().Min.Z);
    const float Radius = Capsule.Radius * Scale;
    const float Embed = Entity.paletteRole == Homestead::Generation::TreePaletteRole::ConiferMature ? 14.0f
        : Entity.paletteRole == Homestead::Generation::TreePaletteRole::WoodlandAccent ? 9.0f : 7.0f;
    float RootGround = GroundHeight(Node.position.x, Node.position.y);
    for (const FVector2D Direction : {FVector2D(1,0), FVector2D(-1,0), FVector2D(0,1),
        FVector2D(0,-1), FVector2D(0.7071f,0.7071f), FVector2D(-0.7071f,0.7071f),
        FVector2D(0.7071f,-0.7071f), FVector2D(-0.7071f,-0.7071f)})
        RootGround = FMath::Min(RootGround, GroundHeight(
            Node.position.x + Direction.X * Radius, Node.position.y + Direction.Y * Radius));
    const FVector Base(Node.position.x, Node.position.y, RootGround - Embed);
    Instance.MeshPath = Mesh->GetPathName();
    Instance.Transform = FTransform(Rotation, Base - Rotation.RotateVector(Anchor * Scale), FVector(Scale));
    Instance.PaletteRole = static_cast<int32>(Entity.paletteRole);
    Instance.VariantIndex = Entity.variantIndex;
    return true;
}

void AHomesteadWorld::ClearOuterTreeBatches()
{
    for (auto& Entry : OuterTreeBatches)
        if (IsValid(Entry.Value.Get()))
            Entry.Value->DestroyComponent();
    OuterTreeBatches.Reset();
    OuterTreeInstances.Reset();
}

bool AHomesteadWorld::RebuildOuterTreeBatches(const Homestead::Simulation& Simulation)
{
    const double Started = FPlatformTime::Seconds();
    struct FBuildEntry
    {
        FString Key;
        UStaticMesh* Mesh = nullptr;
        FHomesteadOuterTreeInstance Instance;
    };

    TArray<FBuildEntry> Desired;
    TSet<FString> DesiredKeys;
    for (const auto& Chunk : TerrainChunks)
    {
        if (Chunk.Value.bCollision) continue;
        Homestead::Generation::ChunkBaseline Baseline;
        if (!GetChunkBaseline(Simulation.GetState().world,
            {Chunk.Key.X, Chunk.Key.Y}, Baseline))
        {
            UE_LOG(LogHomesteadWorld, Error, TEXT("Outer tree generation failed."));
            return false;
        }
        for (const auto& Entity : Baseline.entities)
        {
            if (Entity.kind != Homestead::Generation::EntityKind::ForestTree) continue;
            Homestead::ResourceNode Node;
            Node.key = Entity.key;
            Node.kind = Homestead::ResourceKind::ForestTree;
            Node.position = {static_cast<double>(Entity.xCm), static_cast<double>(Entity.yCm)};
            const auto Edit = std::lower_bound(Simulation.GetState().resourceEdits.begin(),
                Simulation.GetState().resourceEdits.end(), Entity.key,
                [](const Homestead::ResourceEdit& Value,
                    const Homestead::Generation::GeneratedEntityKey& Key) { return Value.key < Key; });
            if (Edit != Simulation.GetState().resourceEdits.end() && Edit->key == Entity.key)
                Node.cleared = Edit->cleared;
            if (Node.cleared) continue;
            FBuildEntry Entry;
            Entry.Key = FString::Printf(TEXT("%d,%d,%u"),
                Node.key.chunk.x, Node.key.chunk.y, Node.key.localId);
            if (DesiredKeys.Contains(Entry.Key))
            {
                UE_LOG(LogHomesteadWorld, Error, TEXT("Duplicate generated outer tree key %s."), *Entry.Key);
                return false;
            }
            if (!ResolveGeneratedTreeVisual(Node, Entry.Mesh, Entry.Instance))
                return false;
            DesiredKeys.Add(Entry.Key);
            Desired.Add(MoveTemp(Entry));
        }
    }
    Desired.Sort([](const FBuildEntry& A, const FBuildEntry& B)
    {
        const int32 PathOrder = A.Instance.MeshPath.Compare(B.Instance.MeshPath, ESearchCase::CaseSensitive);
        return PathOrder == 0 ? A.Key < B.Key : PathOrder < 0;
    });

    TMap<FString, UHierarchicalInstancedStaticMeshComponent*> PreparedBatches;
    TMap<FString, FHomesteadOuterTreeInstance> PreparedInstances;
    auto DiscardPrepared = [&PreparedBatches]()
    {
        for (auto& Entry : PreparedBatches)
            if (IsValid(Entry.Value))
                Entry.Value->DestroyComponent();
    };
    for (const FBuildEntry& Entry : Desired)
    {
        UHierarchicalInstancedStaticMeshComponent* Batch = nullptr;
        if (auto** Existing = PreparedBatches.Find(Entry.Instance.MeshPath))
            Batch = *Existing;
        else
        {
            Batch = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
            if (!Batch)
            {
                DiscardPrepared();
                UE_LOG(LogHomesteadWorld, Error, TEXT("Could not allocate outer tree batch for %s."),
                    *Entry.Instance.MeshPath);
                return false;
            }
            Batch->SetupAttachment(GetRootComponent());
            Batch->SetMobility(EComponentMobility::Static);
            Batch->SetStaticMesh(Entry.Mesh);
            Batch->bOverrideMinLOD = true;
            Batch->MinLOD = OuterMatureTreeMinLOD;
            Batch->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
            Batch->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Batch->SetCollisionResponseToAllChannels(ECR_Ignore);
            Batch->SetGenerateOverlapEvents(false);
            Batch->SetCanEverAffectNavigation(false);
            Batch->ComponentTags.Add(TEXT("GeneratedOuterTreeBatch"));
            Batch->bAutoRebuildTreeOnInstanceChanges = false;
            PreparedBatches.Add(Entry.Instance.MeshPath, Batch);
        }
        Batch->AddInstance(Entry.Instance.Transform);
        PreparedInstances.Add(Entry.Key, Entry.Instance);
    }

    ClearOuterTreeBatches();
    for (auto& Entry : PreparedBatches)
    {
        Entry.Value->bAutoRebuildTreeOnInstanceChanges = true;
        Entry.Value->RegisterComponent();
        Entry.Value->BuildTreeIfOutdated(false, true);
        OuterTreeBatches.Add(Entry.Key, Entry.Value);
    }
    OuterTreeInstances = MoveTemp(PreparedInstances);
    LastOuterTreePrepareMilliseconds = (FPlatformTime::Seconds() - Started) * 1000;
    UE_LOG(LogHomesteadWorld, Display,
        TEXT("Generated outer mature trees rebuilt: batches=%d instances=%d; collision/navigation/overlap disabled."),
        OuterTreeBatches.Num(), OuterTreeInstances.Num());
    return true;
}

void AHomesteadWorld::ClearActiveTreeBatches()
{
    for (auto& Entry : ActiveTreeBatches)
        if (IsValid(Entry.Value.Get()))
            Entry.Value->DestroyComponent();
    for (auto& Entry : ActiveTreeCollisions)
        if (IsValid(Entry.Value.Get()))
            Entry.Value->DestroyComponent();
    ActiveTreeBatches.Reset();
    ActiveTreeCollisions.Reset();
    ActiveTreeInstances.Reset();
    ActiveTreeLayoutSignature.Reset();
}

bool AHomesteadWorld::RebuildActiveTreeBatches(const Homestead::Simulation& Simulation)
{
    const double Started = FPlatformTime::Seconds();
    struct FBuildEntry
    {
        FString Key;
        UStaticMesh* Mesh = nullptr;
        FHomesteadActiveTreeInstance Instance;
    };

    TArray<FBuildEntry> Desired;
    TSet<FString> DesiredKeys;
    for (const auto& Node : Simulation.GetState().resources)
    {
        if (Node.kind != Homestead::ResourceKind::ForestTree || Node.cleared) continue;
        FBuildEntry Entry;
        Entry.Key = FString::Printf(TEXT("%d,%d,%u"),
            Node.key.chunk.x, Node.key.chunk.y, Node.key.localId);
        if (DesiredKeys.Contains(Entry.Key))
        {
            UE_LOG(LogHomesteadWorld, Error, TEXT("Duplicate generated active tree key %s."), *Entry.Key);
            return false;
        }
        if (!ResolveGeneratedTreeVisual(Node, Entry.Mesh, Entry.Instance.Visual))
            return false;
        const auto& Capsule = Entry.Mesh->GetBodySetup()->AggGeom.SphylElems[0];
        const FVector Scale = Entry.Instance.Visual.Transform.GetScale3D();
        Entry.Instance.CapsuleRadius = Capsule.GetScaledRadius(Scale);
        Entry.Instance.CapsuleHalfHeight = Capsule.GetScaledHalfLength(Scale);
        Entry.Instance.CollisionTransform = FTransform(
            Entry.Instance.Visual.Transform.TransformRotation(Capsule.Rotation.Quaternion()),
            Entry.Instance.Visual.Transform.TransformPosition(Capsule.Center));
        Entry.Instance.ResourceId = Node.id;
        DesiredKeys.Add(Entry.Key);
        Desired.Add(MoveTemp(Entry));
    }
    Desired.Sort([](const FBuildEntry& A, const FBuildEntry& B)
    {
        const int32 PathOrder = A.Instance.Visual.MeshPath.Compare(
            B.Instance.Visual.MeshPath, ESearchCase::CaseSensitive);
        return PathOrder == 0 ? A.Key < B.Key : PathOrder < 0;
    });

    TMap<FString, UHierarchicalInstancedStaticMeshComponent*> PreparedBatches;
    TMap<FString, UCapsuleComponent*> PreparedCollisions;
    TMap<FString, FHomesteadActiveTreeInstance> PreparedInstances;
    auto DiscardPrepared = [&PreparedBatches, &PreparedCollisions]()
    {
        for (auto& Entry : PreparedBatches)
            if (IsValid(Entry.Value))
                Entry.Value->DestroyComponent();
        for (auto& Entry : PreparedCollisions)
            if (IsValid(Entry.Value))
                Entry.Value->DestroyComponent();
    };
    for (const FBuildEntry& Entry : Desired)
    {
        UHierarchicalInstancedStaticMeshComponent* Batch = nullptr;
        if (auto** Existing = PreparedBatches.Find(Entry.Instance.Visual.MeshPath))
            Batch = *Existing;
        else
        {
            Batch = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
            if (!Batch)
            {
                DiscardPrepared();
                UE_LOG(LogHomesteadWorld, Error, TEXT("Could not allocate active tree batch for %s."),
                    *Entry.Instance.Visual.MeshPath);
                return false;
            }
            Batch->SetupAttachment(GetRootComponent());
            Batch->SetMobility(EComponentMobility::Static);
            Batch->SetStaticMesh(Entry.Mesh);
            Batch->bOverrideMinLOD = true;
            Batch->MinLOD = ActiveMatureTreeMinLOD;
            Batch->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
            Batch->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Batch->SetCollisionResponseToAllChannels(ECR_Ignore);
            Batch->SetGenerateOverlapEvents(false);
            Batch->SetCanEverAffectNavigation(false);
            Batch->ComponentTags.Add(TEXT("GeneratedActiveTreeBatch"));
            Batch->bAutoRebuildTreeOnInstanceChanges = false;
            PreparedBatches.Add(Entry.Instance.Visual.MeshPath, Batch);
        }
        Batch->AddInstance(Entry.Instance.Visual.Transform);

        auto* Collision = NewObject<UCapsuleComponent>(this);
        if (!Collision)
        {
            DiscardPrepared();
            UE_LOG(LogHomesteadWorld, Error, TEXT("Could not allocate active tree collision for %s."),
                *Entry.Key);
            return false;
        }
        Collision->SetupAttachment(GetRootComponent());
        Collision->SetMobility(EComponentMobility::Static);
        Collision->SetCapsuleSize(Entry.Instance.CapsuleRadius, Entry.Instance.CapsuleHalfHeight, false);
        Collision->SetRelativeTransform(Entry.Instance.CollisionTransform);
        Collision->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
        Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Collision->SetGenerateOverlapEvents(false);
        Collision->SetCanEverAffectNavigation(false);
        Collision->SetCastShadow(false);
        Collision->SetVisibility(false);
        Collision->SetHiddenInGame(true);
        Collision->ComponentTags.Add(TEXT("GeneratedForestTreeCollision"));
        Collision->ComponentTags.Add(FName(*(FString(TEXT("TreeKey_")) + Entry.Key)));
        Collision->ComponentTags.Add(*FString::Printf(TEXT("Resource_%d"), Entry.Instance.ResourceId));
        PreparedCollisions.Add(Entry.Key, Collision);
        PreparedInstances.Add(Entry.Key, Entry.Instance);
    }

    ClearActiveTreeBatches();
    for (auto& Entry : PreparedBatches)
    {
        Entry.Value->bAutoRebuildTreeOnInstanceChanges = true;
        Entry.Value->RegisterComponent();
        Entry.Value->BuildTreeIfOutdated(false, true);
        ActiveTreeBatches.Add(Entry.Key, Entry.Value);
    }
    for (auto& Entry : PreparedCollisions)
    {
        Entry.Value->RegisterComponent();
        ActiveTreeCollisions.Add(Entry.Key, Entry.Value);
    }
    ActiveTreeInstances = MoveTemp(PreparedInstances);
    LastActiveTreePrepareMilliseconds = (FPlatformTime::Seconds() - Started) * 1000;
    UE_LOG(LogHomesteadWorld, Display,
        TEXT("Generated active mature trees rebuilt: batches=%d instances=%d collisions=%d."),
        ActiveTreeBatches.Num(), ActiveTreeInstances.Num(), ActiveTreeCollisions.Num());
    return true;
}

void AHomesteadWorld::BuildResource(FHomesteadWorldVisual& Visual, const Homestead::ResourceNode& Node, bool bProduceOnly)
{
    if (Node.cleared)
    {
        return;
    }
    const FVector Base = AtGround(Node.position.x, Node.position.y);
    const uint32 Variation = GetTypeHash(Descriptor.seed) ^ GetTypeHash(Node.key.chunk.x)
        ^ (GetTypeHash(Node.key.chunk.y) * 127u) ^ Node.key.localId;
    FRandomStream Random(static_cast<int32>(Variation));
    auto Part = [&](UStaticMesh* Mesh, FVector Offset, FVector Size, FLinearColor Color,
        FRotator Rotation = FRotator::ZeroRotator, bool bProduce = false)
    {
        if (bProduce == bProduceOnly)
        {
            AddPart(Visual, Mesh, Base + Offset, Size, Color, false, Rotation);
        }
    };
    auto LoadResource = [&](const TCHAR* Name, bool bGrass = false)
    {
        const FString Path = FString(bGrass ? TEXT("/Game/Trials/GrassGround_20260921_01/Meshes/")
            : TEXT("/Game/Trials/WoodlandResources_20260921_01/Meshes/")) + Name;
        auto* Mesh = LoadObject<UStaticMesh>(nullptr, *Path);
        if (!Mesh)
        {
            bVisualBuildFailed = true;
            UE_LOG(LogHomesteadWorld, Error, TEXT("Resource %d is missing authored mesh %s"), Node.id, *Path);
        }
        return Mesh;
    };
    auto Authored = [&](UStaticMesh* Mesh, FVector2D Offset, float Yaw, bool bProduce, float Scale = 1.0f)
    {
        if (bProduce != bProduceOnly || !Mesh) return;
        const FBox Bounds = Mesh->GetBoundingBox();
        if (!Bounds.IsValid || Bounds.Min.ContainsNaN() || Bounds.Max.ContainsNaN())
        {
            UE_LOG(LogHomesteadWorld, Error, TEXT("Resource %d has invalid authored bounds: %s"), Node.id, *Mesh->GetPathName());
            bVisualBuildFailed = true;
            return;
        }
        const FRotator Rotation(0, Yaw, 0);
        const FVector Anchor(Bounds.GetCenter().X, Bounds.GetCenter().Y, Bounds.Min.Z);
        const FVector Ground = AtGround(Base.X + Offset.X, Base.Y + Offset.Y);
        auto* Component = NewObject<UStaticMeshComponent>(this);
        Component->SetupAttachment(GetRootComponent());
        Component->SetMobility(EComponentMobility::Movable);
        Component->SetStaticMesh(Mesh);
        Component->SetRelativeTransform(FTransform(Rotation, Ground - Rotation.RotateVector(Anchor * Scale), FVector(Scale)));
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetCollisionResponseToAllChannels(ECR_Ignore);
        if (bProduce && Node.kind == Homestead::ResourceKind::Sapling)
        {
            Component->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            Component->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);
        }
        if (Node.kind == Homestead::ResourceKind::Stones && RockMaterial)
            Component->SetMaterial(0, RockMaterial);
        Component->SetGenerateOverlapEvents(false);
        Component->SetCanEverAffectNavigation(false);
        Component->ComponentTags.Append({TEXT("AuthoredResource"), bProduce ? TEXT("ResourceProduce") : TEXT("ResourceBase")});
        Component->RegisterComponent();
        Visual.Components.Add(Component);
    };

    switch (Node.kind)
    {
    case Homestead::ResourceKind::ForestTree:
        break;
    case Homestead::ResourceKind::Branches:
        if (bProduceOnly)
        {
            for (int I = 0; I < 3; ++I)
            {
                const TCHAR* Names[] = {TEXT("SM_DryBranchesMedium01_a"), TEXT("SM_DryBranchesMedium01_b"), TEXT("SM_DryBranchesMedium01_c")};
                Authored(LoadResource(Names[I]), FVector2D(I * 9 - 9, I * 7 - 7), I * 35 + 20, true);
            }
        }
        break;
    case Homestead::ResourceKind::Stones:
        if (bProduceOnly)
        {
            for (int I = 0; I < 3; ++I)
            {
                if (ImportedRock && ImportedRock->GetBoundingBox().GetSize().GetMax() > 0)
                    Authored(ImportedRock, FVector2D(I * 17 - 17, I % 2 * 14), I * 79, true,
                        (24.0f + I * 4.0f) / ImportedRock->GetBoundingBox().GetSize().GetMax());
                else
                {
                    bVisualBuildFailed = true;
                    UE_LOG(LogHomesteadWorld, Error, TEXT("Stone resource %d is missing admitted rock geometry."), Node.id);
                }
            }
        }
        break;
    case Homestead::ResourceKind::BerryBush:
        for (int I = 0; I < 3; ++I)
        {
            Authored(LoadResource(I == 1 ? TEXT("SM_Shrub04_a") : TEXT("SM_Shrub04_c")),
                FVector2D((I - 1) * 10, I % 2 * 10 - 5), I * 113, false);
        }
        if (bProduceOnly)
        {
            for (int I = 0; I < 8; ++I)
            {
                const float Angle = I * 2.399f;
                Part(Sphere, FVector(FMath::Cos(Angle) * 15, FMath::Sin(Angle) * 11, 16 + I % 3 * 4),
                    FVector(3.8f), FLinearColor(0.42f, 0.025f, 0.055f), FRotator::ZeroRotator, true);
            }
        }
        break;
    case Homestead::ResourceKind::Roots:
        for (int I = 0; I < 3; ++I)
        {
            Authored(LoadResource(TEXT("SM_Shrub04_a")), FVector2D(I * 8 - 8, I % 2 * 9), I * 120, false);
        }
        if (bProduceOnly)
        {
            Part(Sphere, FVector(0, 0, 5), FVector(18, 18, 12), FLinearColor(0.65f, 0.43f, 0.19f),
                FRotator::ZeroRotator, true);
        }
        break;
    case Homestead::ResourceKind::Flowers:
        for (int I = 0; I < 2; ++I)
        {
            const FVector2D Offset(I * 18 - 9, I * 6 - 3);
            Authored(LoadResource(TEXT("SM_GrassMedium01_tiny_a"), true), Offset, I * 137, false);
            Authored(LoadResource(I ? TEXT("SM_FlowerEmpodium_b") : TEXT("SM_FlowerEmpodium_a")),
                Offset, I * 137, true);
        }
        break;
    case Homestead::ResourceKind::Reeds:
        for (int I = 0; I < 5; ++I)
        {
            const FVector2D Offset(Random.FRandRange(-18, 18), Random.FRandRange(-18, 18));
            if (I < 2) Authored(LoadResource(TEXT("SM_GrassMedium01_tiny_a"), true), Offset, I * 49, false);
            Authored(LoadResource(TEXT("SM_GrassMedium01_mid_b"), true), Offset, I * 49, true);
        }
        break;
    case Homestead::ResourceKind::Sapling:
    {
        Homestead::Generation::GeneratedEntity Entity;
        if (Homestead::Generation::FindEntity(Descriptor, Node.key, Entity) != Homestead::Generation::Status::Ok)
        {
            bVisualBuildFailed = true;
            UE_LOG(LogHomesteadWorld, Error, TEXT("Generated sapling key cannot resolve."));
            break;
        }
        Authored(LoadResource(TEXT("SM_GrassMedium01_tiny_a"), true), FVector2D::ZeroVector,
            Entity.yawDegrees, false);
        if (Entity.paletteRole == Homestead::Generation::TreePaletteRole::BroadleafYoung)
            Authored(LoadResource(TEXT("SM_TreeSmall02_Woodland")), FVector2D::ZeroVector,
                Entity.yawDegrees, true, Entity.scalePermille / 1000.0f);
        else if (Entity.paletteRole == Homestead::Generation::TreePaletteRole::ConiferYoung)
        {
            if (Entity.variantIndex == 2)
            {
                auto* Mesh = LoadObject<UStaticMesh>(nullptr,
                    TEXT("/Game/Trials/TreePalette_20260921_01/Meshes/SM_FirPole.SM_FirPole"));
                if (!Mesh)
                {
                    bVisualBuildFailed = true;
                    UE_LOG(LogHomesteadWorld, Error, TEXT("Generated intermediate fir is missing; no substitute."));
                }
                Authored(Mesh, FVector2D::ZeroVector, Entity.yawDegrees, true,
                    Entity.scalePermille / 1000.0f);
            }
            else
                Authored(LoadResource(Entity.variantIndex % 2 ? TEXT("SM_FirSapling_a") : TEXT("SM_FirSapling_c")),
                    FVector2D::ZeroVector, Entity.yawDegrees, true, Entity.scalePermille / 1000.0f);
        }
        else
        {
            bVisualBuildFailed = true;
            UE_LOG(LogHomesteadWorld, Error, TEXT("Generated sapling has illegal palette role %d."),
                static_cast<int32>(Entity.paletteRole));
        }
        break;
    }
    default:
        break;
    }
}

void AHomesteadWorld::BuildStructure(FHomesteadWorldVisual& Visual, const Homestead::Structure& Structure, bool bPreview)
{
    const Homestead::Point Center = Homestead::CellCenter(Structure.cellX, Structure.cellY);
    const FVector Base(Center.x, Center.y, CellBase(Structure.cellX, Structure.cellY));
    // UE positive yaw rotates +X toward +Y; negative yaw maps the north edge to east.
    const FRotator Rotation(0, -90.0f * (Structure.rotation % 4), 0);
    auto Part = [&](UStaticMesh* Mesh, FVector Offset, FVector Size, FLinearColor Color,
        bool bSolid = false, FRotator LocalRotation = FRotator::ZeroRotator, float Glow = 0.0f)
    {
        const FRotator Combined = (Rotation.Quaternion() * LocalRotation.Quaternion()).Rotator();
        return AddPart(Visual, Mesh, Base + Rotation.RotateVector(Offset), Size,
            bPreview ? PreviewColor : Color, bSolid && !bPreview, Combined, 0.85f, bPreview ? 0.0f : Glow);
    };
    switch (Structure.kind)
    {
    case Homestead::Piece::Foundation:
        Part(Cube, FVector(0, 0, -10), FVector(298, 298, 20), Wood, true);
        for (int I = 0; I < 6; ++I)
        {
            Part(Cube, FVector(-125 + I * 50, 0, 0.8f), FVector(2, 292, 1), Bark);
        }
        break;
    case Homestead::Piece::Wall:
        Part(Cube, FVector(0, 144, 130), FVector(300, 12, 260), Wood, true);
        for (int I = 0; I < 4; ++I)
        {
            Part(Cube, FVector(-142 + I * 94.67f, 142, 130), FVector(12, 20, 260), Bark);
        }
        Part(Cube, FVector(0, 142, 252), FVector(300, 22, 16), Bark);
        break;
    case Homestead::Piece::Doorway:
        Part(Cube, FVector(-107.5f, 144, 130), FVector(85, 12, 260), Wood, true);
        Part(Cube, FVector(107.5f, 144, 130), FVector(85, 12, 260), Wood, true);
        Part(Cube, FVector(0, 144, 247.5f), FVector(130, 16, 25), Bark, true);
        // Tied-back cloth suggests a simple shelter entrance without a hidden collider.
        Part(Cube, FVector(-61, 133, 123), FVector(8, 8, 214), Cloth);
        Part(Cube, FVector(61, 133, 123), FVector(8, 8, 214), Cloth);
        Part(Cube, FVector(0, 137, 235), FVector(124, 5, 8), Cloth);
        break;
    case Homestead::Piece::Roof:
        Part(Cube, FVector(0, 0, 273), FVector(310, 310, 22), FLinearColor(0.27f, 0.25f, 0.105f), true);
        for (int I = -1; I <= 1; ++I)
        {
            Part(Cube, FVector(I * 110, 0, 256), FVector(12, 300, 12), Bark);
        }
        break;
    case Homestead::Piece::Fire:
    {
        const FVector Hearth(-100, 95, 0);
        for (int I = 0; I < 8; ++I)
        {
            const float Angle = I * PI / 4;
            Part(Sphere, Hearth + FVector(FMath::Cos(Angle) * 28, FMath::Sin(Angle) * 28, 7),
                FVector(19, 16, 14), Stone);
        }
        Part(Cylinder, Hearth + FVector(0, 0, 8), FVector(10, 10, 42), Bark, false, FRotator(85, 30, 0));
        Part(Cylinder, Hearth + FVector(0, 0, 10), FVector(10, 10, 42), Bark, false, FRotator(85, -30, 0));
        if (Structure.fuelHours > 0 && !bPreview)
        {
            Part(Cone, Hearth + FVector(0, 0, 28), FVector(24, 24, 45),
                FLinearColor(0.95f, 0.2f, 0.015f), false, FRotator::ZeroRotator, 3.0f);
            Part(Cone, Hearth + FVector(0, 0, 24), FVector(13, 13, 28),
                FLinearColor(1.0f, 0.63f, 0.08f), false, FRotator::ZeroRotator, 4.0f);
            UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
            Light->SetupAttachment(GetRootComponent());
            Light->SetMobility(EComponentMobility::Movable);
            Light->SetRelativeLocation(Base + Rotation.RotateVector(Hearth + FVector(0, 0, 65)));
            Light->SetLightColor(FLinearColor(1.0f, 0.49f, 0.19f));
            Light->SetIntensity(1800.0f);
            Light->SetAttenuationRadius(550.0f);
            Light->SetSourceRadius(18.0f);
            Light->SetCastShadows(false);
            Light->RegisterComponent();
            Visual.Components.Add(Light);
        }
        break;
    }
    case Homestead::Piece::Bed:
        Part(Cube, FVector(95, -10, 6), FVector(70, 155, 12), Cloth, true);
        Part(Cube, FVector(95, 46, 14), FVector(62, 32, 12), FLinearColor(0.7f, 0.65f, 0.46f));
        Part(Cube, FVector(95, -28, 14), FVector(68, 106, 8), FLinearColor(0.23f, 0.31f, 0.21f));
        break;
    case Homestead::Piece::Chest:
        Part(Cube, FVector(-100, -100, 27), FVector(70, 55, 54), Wood, true);
        Part(Cube, FVector(-100, -100, 55), FVector(74, 59, 6), Bark);
        Part(Cube, FVector(-100, -71, 35), FVector(12, 4, 12), Stone);
        break;
    default:
        break;
    }
    if (bPreview)
    {
        for (USceneComponent* Component : Visual.Components)
        {
            if (UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Component))
            {
                Mesh->SetCastShadow(false);
            }
        }
    }
}

void AHomesteadWorld::BuildPlot(FHomesteadWorldVisual& Visual, const Homestead::Plot& Plot)
{
    const Homestead::Point Center = Homestead::CellCenter(Plot.cellX, Plot.cellY);
    const float Moisture = Stage(Plot.moisture, 5) / 5.0f;
    const FLinearColor WetSoil = FMath::Lerp(Soil, FLinearColor(0.075f, 0.044f, 0.025f), Moisture);
    // Small separate soil tiles follow the terrain rather than floating above a slope.
    for (int X = -1; X <= 1; ++X)
    {
        for (int Y = -1; Y <= 1; ++Y)
        {
            const float PX = Center.x + X * 76;
            const float PY = Center.y + Y * 76;
            AddPart(Visual, Cube, AtGround(PX, PY, 1.5f), FVector(74, 74, 3), WetSoil,
                false, FRotator::ZeroRotator, 0.9f - Moisture * 0.35f);
            if (Plot.planted)
            {
                const float Growth = Stage(Plot.growth, 12) / 12.0f;
                const bool BerryCrop = Plot.kind == Homestead::CropKind::Berries;
                const float Height = 7 + Growth * (BerryCrop ? 65 : 48);
                AddPart(Visual, Cone, AtGround(PX, PY, Height * 0.5f + 3),
                    FVector(12 + Growth * 23, 12 + Growth * 23, Height), Leaf);
                AddPart(Visual, Sphere, AtGround(PX, PY, Height * 0.55f + 3),
                    FVector(20 + Growth * 35, 12 + Growth * 20, 7 + Growth * 8), LightLeaf,
                    false, FRotator(0, (X + Y) * 52, 0));
                if (Plot.growth >= 1.0)
                {
                    if (BerryCrop)
                    {
                        for (int Berry = 0; Berry < 3; ++Berry)
                            AddPart(Visual, Sphere, AtGround(PX + (Berry - 1) * 10, PY + 5, Height * 0.6f),
                                FVector(9, 9, 9), FLinearColor(0.42f, 0.035f, 0.09f));
                    }
                    else
                        AddPart(Visual, Sphere, AtGround(PX, PY, 7), FVector(24, 24, 15),
                            FLinearColor(0.66f, 0.43f, 0.21f));
                }
            }
        }
    }
    FRandomStream Random(Plot.id * 193 + 51);
    const int WeedCount = Stage(Plot.weeds, 8);
    for (int I = 0; I < WeedCount; ++I)
    {
        const float X = Center.x + Random.FRandRange(-100, 100);
        const float Y = Center.y + Random.FRandRange(-100, 100);
        AddPart(Visual, Cone, AtGround(X, Y, 19), FVector(22, 22, 38),
            FLinearColor(0.34f, 0.31f, 0.07f), false, FRotator(0, I * 47, 16));
    }
}

void AHomesteadWorld::UpdateLighting(const Homestead::State& State)
{
    const float Hour = static_cast<float>(FMath::Fmod(State.hour, 24.0));
    const float SolarAngle = (Hour - 6.0f) / 24.0f * 2.0f * PI;
    const float Elevation = FMath::Sin(SolarAngle);
    const float Daylight = FMath::SmoothStep(-0.1f, 0.25f, Elevation);
    // Matches the simulation's deterministic three-day spring weather cycle.
    const bool bRaining = static_cast<int64>(State.hour / 24.0) % 3 == 1 && Hour >= 9.0f && Hour < 15.0f;
    Sun->SetRelativeRotation(FRotator(-Elevation * 65.0f, (Hour - 6) * 15.0f - 70.0f, 0));
    Sun->SetIntensity(FMath::Lerp(0.0f, bRaining ? 17000.0f : 46000.0f, Daylight));
    Sun->SetLightColor(FMath::Lerp(FLinearColor(1.0f, 0.76f, 0.56f),
        FLinearColor(1.0f, 0.99f, 0.95f), FMath::Clamp(Elevation * 2, 0.0f, 1.0f)));
    Moon->SetRelativeRotation(FRotator(Elevation * 65.0f, (Hour - 6) * 15.0f + 110.0f, 0));
    Moon->SetIntensity(0.5f * (1.0f - Daylight));
    Sky->SetIntensity(FMath::Lerp(0.35f, 1.0f, Daylight));
    Fog->SetFogDensity(bRaining ? 0.035f : FMath::Lerp(0.016f, 0.007f, Daylight));
    Fog->SetFogInscatteringColor(bRaining ? FLinearColor(0.43f, 0.49f, 0.52f)
        : FMath::Lerp(FLinearColor(0.055f, 0.085f, 0.14f), FLinearColor(0.64f, 0.72f, 0.68f), Daylight));
}

bool AHomesteadWorld::Initialize(const Homestead::Simulation& Simulation)
{
    ClearVisual(Preview);
    if (!bInitialized)
    {
        FieldMaterial = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/SurvivalGame/Materials/M_Field.M_Field"));
        GroundMaterial = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/Trials/GrassGround_20260921_01/Materials/M_GrassGroundBlend.M_GrassGroundBlend"));
        if (!GroundMaterial)
        {
            UE_LOG(LogHomesteadWorld, Error, TEXT("Admitted grass-ground blend is missing; world preparation stopped."));
            return false;
        }
        RockMaterial = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/SurvivalGame/Materials/M_Rock.M_Rock"));
        ImportedRock = LoadObject<UStaticMesh>(nullptr,
            TEXT("/Game/SurvivalGame/Environment/MossRocks.MossRocks"));
        if (!GroundMaterial || !RockMaterial || !ImportedRock)
        {
            UE_LOG(LogHomesteadWorld, Error,
                TEXT("Required woodland assets: ground material=%s, rock material=%s, MossRocks mesh=%s. World preparation stopped."),
                GroundMaterial ? TEXT("loaded") : TEXT("missing"),
                RockMaterial ? TEXT("loaded") : TEXT("missing"),
                ImportedRock ? TEXT("loaded") : TEXT("missing"));
            return false;
        }
        if (!FieldMaterial)
        {
            UE_LOG(LogHomesteadWorld, Error,
                TEXT("M_Field is missing; world preparation stopped without a default-material substitute."));
            return false;
        }
        if (!Cube || !Sphere || !Cylinder || !Cone)
        {
            UE_LOG(LogHomesteadWorld, Error, TEXT("PROTOTYPE FALLBACK: required Engine/BasicShapes assets are missing."));
        }
        if (!FieldMaterial || !GroundMaterial || !RockMaterial || !ImportedRock || !Cube || !Sphere || !Cylinder || !Cone)
            return false;
        BuildLighting();
        bInitialized = true;
        UE_LOG(LogHomesteadWorld, Display,
            TEXT("Preparing persistent generated woodland. Runtime evidence does not establish final art acceptance."));
    }
    return Refresh(Simulation);
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
    if (!BuildTerrain(State)) return false;
    bVisualBuildFailed = false;
    if (WorldChanged)
    {
        for (auto& Entry : ResourceVisuals) ClearVisual(Entry.Value);
        for (auto& Entry : ResourceProduceVisuals) ClearVisual(Entry.Value);
        for (auto& Entry : StructureVisuals) ClearVisual(Entry.Value);
        for (auto& Entry : PlotVisuals) ClearVisual(Entry.Value);
        ClearOuterTreeBatches();
        ClearActiveTreeBatches();
        ClearVisual(Preview);
    }
    FString Layout = FString::Printf(TEXT("%llu:%u:%d,%d;"), static_cast<unsigned long long>(State.world.seed),
        State.world.generationVersion, State.activeChunk.x, State.activeChunk.y);
    for (const auto& Node : State.resources)
    {
        Layout += FString::Printf(TEXT("%d:%.3f:%.3f:%d;"), Node.id, Node.position.x, Node.position.y, Node.cleared);
    }
    for (const auto& Structure : State.structures)
    {
        Layout += FString::Printf(TEXT("S:%d:%d;"), Structure.cellX, Structure.cellY);
    }
    for (const auto& Plot : State.plots)
    {
        Layout += FString::Printf(TEXT("P:%d:%d;"), Plot.cellX, Plot.cellY);
    }
    if (ResourceLayoutSignature != Layout)
    {
        if (!BuildDecorations(Simulation)) return false;
        ResourceLayoutSignature = MoveTemp(Layout);
    }
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
    if (OuterTreeLayoutSignature != OuterLayout)
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
        const bool bReady = Node.readyAtHour <= State.hour;
        const FString Signature = FString::Printf(TEXT("%d:%.3f:%.3f:%d"),
            static_cast<int>(Node.kind), Node.position.x, Node.position.y, Node.cleared);
        FHomesteadWorldVisual& Visual = ResourceVisuals.FindOrAdd(Node.id);
        if (Visual.Signature != Signature)
        {
            ClearVisual(Visual);
            BuildResource(Visual, Node, false);
            if (bVisualBuildFailed) return false;
            Visual.Signature = Signature;
        }
        const FString ProduceSignature = Signature + (bReady ? TEXT(":ready") : TEXT(":harvested"));
        FHomesteadWorldVisual& Produce = ResourceProduceVisuals.FindOrAdd(Node.id);
        if (Produce.Signature != ProduceSignature)
        {
            ClearVisual(Produce);
            if (bReady)
            {
                BuildResource(Produce, Node, true);
                if (bVisualBuildFailed) return false;
            }
            Produce.Signature = ProduceSignature;
        }
    }

    std::vector<Homestead::Structure> NearStructures;
    std::vector<Homestead::Plot> NearPlots;
    auto Near = [&](int X, int Y)
    {
        const auto Center = Homestead::CellCenter(X, Y);
        return FMath::Abs(Center.x - (State.activeChunk.x + 0.5) * Homestead::Generation::ChunkSizeCm) <= 6000
            && FMath::Abs(Center.y - (State.activeChunk.y + 0.5) * Homestead::Generation::ChunkSizeCm) <= 6000;
    };
    for (const auto& Structure : State.structures)
        if (Near(Structure.cellX, Structure.cellY)) NearStructures.push_back(Structure);
    for (const auto& Plot : State.plots)
        if (Near(Plot.cellX, Plot.cellY)) NearPlots.push_back(Plot);
    RemoveMissing(StructureVisuals, NearStructures);
    for (const auto& Structure : NearStructures)
    {
        const FString Signature = FString::Printf(TEXT("%d:%d:%d:%d:%d"),
            static_cast<int>(Structure.kind), Structure.cellX, Structure.cellY, Structure.rotation,
            Structure.fuelHours > 0);
        FHomesteadWorldVisual& Visual = StructureVisuals.FindOrAdd(Structure.id);
        if (Visual.Signature != Signature)
        {
            ClearVisual(Visual);
            BuildStructure(Visual, Structure, false);
            Visual.Signature = Signature;
        }
    }

    RemoveMissing(PlotVisuals, NearPlots);
    for (const auto& Plot : NearPlots)
    {
        const FString Signature = FString::Printf(TEXT("%d:%d:%d:%d:%d:%d:%d"),
            Plot.cellX, Plot.cellY, Plot.planted, Stage(Plot.growth, 12),
            Stage(Plot.moisture, 5), Stage(Plot.weeds, 8), static_cast<int>(Plot.kind));
        FHomesteadWorldVisual& Visual = PlotVisuals.FindOrAdd(Plot.id);
        if (Visual.Signature != Signature)
        {
            ClearVisual(Visual);
            BuildPlot(Visual, Plot);
            Visual.Signature = Signature;
        }
    }
    UpdateLighting(State);
    LastRefreshMilliseconds = (FPlatformTime::Seconds() - RefreshStarted) * 1000;
    if (Transition)
    {
        LastTransitionRefreshMilliseconds = LastRefreshMilliseconds;
        LastTransitionTerrainMilliseconds = LastTerrainPrepareMilliseconds;
        LastTransitionCoverMilliseconds = LastCoverPrepareMilliseconds;
        LastTransitionOuterTreeMilliseconds = LastOuterTreePrepareMilliseconds;
        LastTransitionActiveTreeMilliseconds = LastActiveTreePrepareMilliseconds;
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

void AHomesteadWorld::SetPlacementPreview(bool Visible, Homestead::Piece Kind, int CellX, int CellY, int Rotation)
{
    if (!Visible || !bInitialized || Kind == Homestead::Piece::Count)
    {
        ClearVisual(Preview);
        return;
    }
    const FString Signature = FString::Printf(TEXT("%d:%d:%d:%d"), static_cast<int>(Kind), CellX, CellY, Rotation);
    if (Preview.Signature == Signature)
    {
        return;
    }
    ClearVisual(Preview);
    Homestead::Structure Structure;
    Structure.kind = Kind;
    Structure.cellX = CellX;
    Structure.cellY = CellY;
    Structure.rotation = Rotation;
    BuildStructure(Preview, Structure, true);
    Preview.Signature = Signature;
}
