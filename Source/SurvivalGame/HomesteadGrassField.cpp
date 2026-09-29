#include "HomesteadGrassField.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "HomesteadEstateGround.h"
#include "HomesteadEstateTerrain.h"
#include "Materials/MaterialInterface.h"

DEFINE_LOG_CATEGORY_STATIC(LogHomesteadGround, Log, All);

namespace
{
// Clear radius (cm) round each kind of interactable, so the meadow never hides one.
float GrassClearRadius(Homestead::ResourceKind Kind)
{
    using K = Homestead::ResourceKind;
    switch (Kind)
    {
    case K::ForestTree: return 70.0f;
    case K::Sapling: return 60.0f;
    case K::BrambleThicket: case K::BrambleBank: case K::GiantLog: case K::FallenLog: case K::SalvagePile:
    case K::Boulder: case K::StumpAncient: case K::DeerRemains: case K::Rubble:
        return 170.0f;
    case K::TallGrass: case K::Weeds: case K::BrambleThin: case K::BerryBush: case K::StumpLarge:
        return 125.0f;
    case K::Flowers: case K::Primroses: case K::Bluebells: case K::WildDaffodils: case K::WildGarlic:
        return 85.0f;
    default:
        return 100.0f;
    }
}

uint64 GrassMix(uint64 Hash, uint64 Value)
{
    Hash ^= Value + 0x9e3779b97f4a7c15ull + (Hash << 6) + (Hash >> 2);
    return Hash;
}

uint64 GrassQuantize(double Value) { return static_cast<uint64>(static_cast<int64>(FMath::RoundToDouble(Value))); }

FIntPoint GrassCellOf(float X, float Y, float Size)
{
    return FIntPoint(FMath::FloorToInt32(X / Size), FMath::FloorToInt32(Y / Size));
}
}

bool UHomesteadGrassField::LoadAssets()
{
    if (bAssetsTried) return bAssetsReady;
    bAssetsTried = true;
    for (int32 Lod = 0; Lod < 3; ++Lod)
    {
        const FString Path = FString::Printf(TEXT("/Game/SurvivalGame/Estate/Ground/SM_GrassPatch_LOD%d.SM_GrassPatch_LOD%d"), Lod, Lod);
        UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Path);
        if (!Mesh)
        {
            UE_LOG(LogHomesteadGround, Warning, TEXT("Estate grass mesh missing: %s. Run Scripts/Terrain/build_ground.py."), *Path);
            return false;
        }
        Meshes.Add(Mesh);
    }
    Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/SurvivalGame/Estate/Ground/MI_EstateGrass.MI_EstateGrass"));
    if (!Material)
    {
        UE_LOG(LogHomesteadGround, Warning, TEXT("MI_EstateGrass is missing. Run Scripts/Terrain/build_ground.py."));
        return false;
    }
    bAssetsReady = true;
    return true;
}

int32 UHomesteadGrassField::LiveInstances() const
{
    int32 Count = 0;
    for (const auto& Entry : Live)
        if (Pool.IsValidIndex(Entry.Value.Component) && Pool[Entry.Value.Component])
            Count += Pool[Entry.Value.Component]->GetInstanceCount();
    return Count;
}

uint64 UHomesteadGrassField::LayoutSignature(const Homestead::State& State) const
{
    uint64 Hash = 1469598103934665603ull;
    for (const auto& Node : State.resources)
    {
        Hash = GrassMix(Hash, static_cast<uint64>(Node.id));
        Hash = GrassMix(Hash, GrassQuantize(Node.position.x));
        Hash = GrassMix(Hash, GrassQuantize(Node.position.y));
    }
    for (const auto& Drop : State.worldDrops)
    {
        Hash = GrassMix(Hash, static_cast<uint64>(Drop.id) + 0x5000000ull);
        Hash = GrassMix(Hash, GrassQuantize(Drop.position.x));
        Hash = GrassMix(Hash, GrassQuantize(Drop.position.y));
    }
    for (const auto& Plot : State.plots)
        Hash = GrassMix(Hash, (static_cast<uint64>(static_cast<uint32>(Plot.cellX)) << 32) ^ static_cast<uint32>(Plot.cellY) ^ 0x70ull);
    for (const auto& Structure : State.structures)
    {
        Hash = GrassMix(Hash, static_cast<uint64>(Structure.id) + 0x9000000ull);
        Hash = GrassMix(Hash, (static_cast<uint64>(static_cast<uint32>(Structure.cellX)) << 32) ^ static_cast<uint32>(Structure.cellY));
        Hash = GrassMix(Hash, static_cast<uint64>(Structure.buildingId) * 31 + static_cast<uint64>(Structure.kind));
    }
    return Hash;
}

void UHomesteadGrassField::RebuildObstacles(const Homestead::State& State)
{
    Obstacles.Reset();
    ObstacleCells.Reset();
    Blocks.Reset();
    // Radius > 0 is a grazed circle round an interactable; < 0 is a bare, world-aligned square of half
    // size -Radius (a garden plot), where no blade grows at all.
    auto Add = [this](double X, double Y, float Radius)
    {
        const int32 Index = Obstacles.Add({FVector2f(static_cast<float>(X), static_cast<float>(Y)), Radius});
        // Register in every cell a tile could see it from.
        const float Reach = FMath::Abs(Radius) * (Radius < 0 ? UE_SQRT_2 : 1.0f) + TileCm;
        const FIntPoint Low = GrassCellOf(X - Reach, Y - Reach, ChunkCm), High = GrassCellOf(X + Reach, Y + Reach, ChunkCm);
        for (int32 CX = Low.X; CX <= High.X; ++CX)
            for (int32 CY = Low.Y; CY <= High.Y; ++CY)
                ObstacleCells.Add(FIntPoint(CX, CY), Index);
    };
    for (const auto& Node : State.resources)
        Add(Node.position.x, Node.position.y, GrassClearRadius(Node.kind));
    for (const auto& Drop : State.worldDrops)
        Add(Drop.position.x, Drop.position.y, 60.0f);
    for (const auto& Plot : State.plots)
    {
        const Homestead::Point Centre = Homestead::PlotCenter(Plot);
        Add(Centre.x, Centre.y, -static_cast<float>(Homestead::GardenCellSize * 0.5 + PlotMarginCm));
    }
    for (const auto& Structure : State.structures)
        Blocks.Add(Homestead::StructureFootprint(State, Structure));
}

uint64 UHomesteadGrassField::ChunkObstacleSignature(FIntPoint Chunk) const
{
    uint64 Hash = 0x9e37ull;
    TArray<int32> Near;
    ObstacleCells.MultiFind(Chunk, Near);
    Near.Sort();
    for (const int32 Index : Near)
    {
        const FObstacle& O = Obstacles[Index];
        Hash = GrassMix(Hash, GrassQuantize(O.Centre.X));
        Hash = GrassMix(Hash, GrassQuantize(O.Centre.Y));
        Hash = GrassMix(Hash, GrassQuantize(O.Radius));
    }
    const FBox2D Box(FVector2D(Chunk.X * ChunkCm - TileCm, Chunk.Y * ChunkCm - TileCm),
        FVector2D((Chunk.X + 1) * ChunkCm + TileCm, (Chunk.Y + 1) * ChunkCm + TileCm));
    for (const auto& Block : Blocks)
    {
        const double Reach = FMath::Max(Block.half.x, Block.half.y) * UE_SQRT_2;
        if (Box.ComputeSquaredDistanceToPoint(FVector2D(Block.center.x, Block.center.y)) > Reach * Reach) continue;
        Hash = GrassMix(Hash, GrassQuantize(Block.center.x));
        Hash = GrassMix(Hash, GrassQuantize(Block.center.y));
        Hash = GrassMix(Hash, GrassQuantize(Block.yaw * 10.0));
    }
    return Hash;
}

bool UHomesteadGrassField::IsBlocked(FVector2D Centre) const
{
    // Skip a patch that overlaps a building piece at all, with a trodden fringe round it.
    constexpr double Fringe = TileCm * 0.5 + 40.0;
    for (const auto& Box : Blocks)
    {
        const double Yaw = FMath::DegreesToRadians(Box.yaw);
        const double DX = Centre.X - Box.center.x, DY = Centre.Y - Box.center.y;
        const double LocalX = DX * FMath::Cos(Yaw) + DY * FMath::Sin(Yaw);
        const double LocalY = -DX * FMath::Sin(Yaw) + DY * FMath::Cos(Yaw);
        if (FMath::Abs(LocalX) < Box.half.x + Fringe && FMath::Abs(LocalY) < Box.half.y + Fringe)
            return true;
    }
    return false;
}

int32 UHomesteadGrassField::AcquireComponent()
{
    if (FreeComponents.Num() > 0)
        return FreeComponents.Pop(EAllowShrinking::No);
    auto* Component = NewObject<UInstancedStaticMeshComponent>(GetOwner());
    Component->SetupAttachment(this);
    Component->SetMobility(EComponentMobility::Movable);
    Component->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Component->SetCanEverAffectNavigation(false);
    // Grass is a surface finish: no shadows (WPO would invalidate virtual shadow pages every frame),
    // no distance field, no ray tracing, no decals.
    Component->SetCastShadow(false);
    Component->bAffectDistanceFieldLighting = false;
    Component->SetVisibleInRayTracing(false);
    Component->bReceivesDecals = false;
    Component->SetGenerateOverlapEvents(false);
    Component->NumCustomDataFloats = CustomFloats;
    Component->SetCullDistances(0, static_cast<int32>(RadiusCm + 600.0f));
    // Wind and the heroine push blades up to half a metre past the patch's own bounds.
    Component->SetBoundsScale(1.25f);
    Component->ComponentTags.Add(TEXT("EstateGrass"));
    Component->RegisterComponent();
    return Pool.Add(Component);
}

void UHomesteadGrassField::ReleaseChunk(FChunk& Chunk)
{
    if (!Pool.IsValidIndex(Chunk.Component)) return;
    UInstancedStaticMeshComponent* Component = Pool[Chunk.Component];
    if (Component)
    {
        Component->ClearInstances();
        Component->SetVisibility(false);
    }
    FreeComponents.Add(Chunk.Component);
    Chunk.Component = INDEX_NONE;
    Chunk.Lod = INDEX_NONE;
}

void UHomesteadGrassField::BuildChunk(FIntPoint Chunk, FChunk& Out, int32 Lod)
{
    constexpr int32 Tiles = static_cast<int32>(ChunkCm / TileCm);
    TArray<FTransform> Transforms;
    TArray<float> Custom;
    Transforms.Reserve(Tiles * Tiles);
    Custom.Reserve(Tiles * Tiles * CustomFloats);
    TArray<int32> Near;
    ObstacleCells.MultiFind(Chunk, Near);
    for (int32 TX = 0; TX < Tiles; ++TX)
        for (int32 TY = 0; TY < Tiles; ++TY)
        {
            const double CX = Chunk.X * ChunkCm + (TX + 0.5) * TileCm;
            const double CY = Chunk.Y * ChunkCm + (TY + 0.5) * TileCm;
            if (HomesteadEstateGround::GrassDensity(CX, CY) < 0.02f) continue;
            if (IsBlocked(FVector2D(CX, CY))) continue;
            // The three nearest interactables the patch can reach; a crowded patch is left bare.
            float Found[3][3] = {};
            int32 Count = 0;
            bool bCrowded = false;
            for (const int32 Index : Near)
            {
                const FObstacle& O = Obstacles[Index];
                const double Reach = FMath::Abs(O.Radius) * (O.Radius < 0 ? UE_SQRT_2 : 1.0) + TileCm * 0.72 + 35.0;
                if (FVector2D::DistSquared(FVector2D(O.Centre.X, O.Centre.Y), FVector2D(CX, CY)) > Reach * Reach) continue;
                if (Count == 3) { bCrowded = true; break; }
                Found[Count][0] = O.Centre.X;
                Found[Count][1] = O.Centre.Y;
                Found[Count][2] = O.Radius;
                ++Count;
            }
            if (bCrowded) continue;
            // Lie the patch on the ground's plane; the landscape's quads are 1 m, so over 2 m this is
            // within a few centimetres, and the patch sits 2 cm down to hide the gap.
            const float H = HomesteadEstateTerrain::Height(CX, CY);
            const float HX = HomesteadEstateTerrain::Height(CX + 100.0, CY) - HomesteadEstateTerrain::Height(CX - 100.0, CY);
            const float HY = HomesteadEstateTerrain::Height(CX, CY + 100.0) - HomesteadEstateTerrain::Height(CX, CY - 100.0);
            if (!FMath::IsFinite(H) || !FMath::IsFinite(HX) || !FMath::IsFinite(HY)) continue;
            const FVector Up = FVector(-HX / 200.0, -HY / 200.0, 1.0).GetSafeNormal();
            // A quarter turn per patch from its cell, so neighbouring patches don't repeat.
            const uint32 Hash = HashCombine(GetTypeHash(FIntPoint(Chunk.X * Tiles + TX, Chunk.Y * Tiles + TY)), 0x51a7u);
            const float Yaw = 90.0f * (Hash & 3);
            const FVector Forward = FRotator(0, Yaw, 0).Vector();
            const FQuat Rotation = FRotationMatrix::MakeFromZX(Up, Forward).ToQuat();
            Transforms.Add(FTransform(Rotation, FVector(CX, CY, H - 2.0), FVector::OneVector));
            for (int32 I = 0; I < 3; ++I)
                Custom.Append({Found[I][0], Found[I][1], Found[I][2]});
        }
    Out.Obstacles = ChunkObstacleSignature(Chunk);
    if (Transforms.IsEmpty())
    {
        ReleaseChunk(Out);
        Out.Lod = Lod;
        return;
    }
    if (Out.Component == INDEX_NONE) Out.Component = AcquireComponent();
    UInstancedStaticMeshComponent* Component = Pool[Out.Component];
    Component->ClearInstances();
    if (Component->GetStaticMesh() != Meshes[Lod]) Component->SetStaticMesh(Meshes[Lod]);
    if (Component->GetMaterial(0) != Material) Component->SetMaterial(0, Material);
    Component->AddInstances(Transforms, false, true);
    for (int32 I = 0; I < Transforms.Num(); ++I)
        Component->SetCustomData(I, TArrayView<const float>(Custom.GetData() + I * CustomFloats, CustomFloats), false);
    Component->SetVisibility(true);
    Component->MarkRenderStateDirty();
    Out.Lod = Lod;
}

void UHomesteadGrassField::Update(const Homestead::State& State, const FVector& View)
{
    if (!LoadAssets() || !HomesteadEstateTerrain::IsActive() || !HomesteadEstateGround::Activate()) return;
    if (!FMath::IsFinite(View.X) || !FMath::IsFinite(View.Y)) return;
    const uint64 Now = LayoutSignature(State);
    if (Now != Signature)
    {
        Signature = Now;
        RebuildObstacles(State);
        // Rebuild only the chunks whose interactables or building pieces changed.
        for (auto& Entry : Live)
            if (Entry.Value.Obstacles != ChunkObstacleSignature(Entry.Key)) Entry.Value.Lod = INDEX_NONE;
    }
    const FIntPoint Low = GrassCellOf(View.X - RadiusCm, View.Y - RadiusCm, ChunkCm);
    const FIntPoint High = GrassCellOf(View.X + RadiusCm, View.Y + RadiusCm, ChunkCm);
    TMap<FIntPoint, int32> Want;
    for (int32 CX = Low.X; CX <= High.X; ++CX)
        for (int32 CY = Low.Y; CY <= High.Y; ++CY)
        {
            const FBox2D Box(FVector2D(CX * ChunkCm, CY * ChunkCm), FVector2D((CX + 1) * ChunkCm, (CY + 1) * ChunkCm));
            const double Nearest = FMath::Sqrt(Box.ComputeSquaredDistanceToPoint(FVector2D(View.X, View.Y)));
            if (Nearest > RadiusCm) continue;
            if (!HomesteadEstateTerrain::Contains(Box.GetCenter().X, Box.GetCenter().Y)) continue;
            const double Pick = Nearest - LodMarginCm;
            Want.Add(FIntPoint(CX, CY), Pick >= Lod2Cm ? 2 : Pick >= Lod1Cm ? 1 : 0);
        }
    for (auto It = Live.CreateIterator(); It; ++It)
        if (!Want.Contains(It.Key()))
        {
            ReleaseChunk(It.Value());
            It.RemoveCurrent();
        }
    for (const auto& Entry : Want)
    {
        FChunk& Chunk = Live.FindOrAdd(Entry.Key);
        if (Chunk.Lod != Entry.Value) BuildChunk(Entry.Key, Chunk, Entry.Value);
    }
}

void UHomesteadGrassField::Clear()
{
    for (auto& Entry : Live) ReleaseChunk(Entry.Value);
    Live.Reset();
    Signature = 0;
}
