#include "HomesteadWorld.h"
#include "Simulation/HomesteadCrops.h"
#include "Components/MaterialBillboardComponent.h"
#include "HomesteadEstateTerrain.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "HomesteadGrassField.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Materials/MaterialParameterCollection.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "HomesteadCharacter.h"
#include "Simulation/HomesteadOvergrowth.h"
#include "Async/Async.h"
#include "Async/ParallelFor.h"
#include "Components/DirectionalLightComponent.h"
#include "RenderUtils.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/MeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/AudioComponent.h"
#include "GameFramework/PlayerController.h"
#include "Sound/SoundWave.h"
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
#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogHomesteadWorld, Log, All);

namespace
{
// Estate blackberry brambles (BerryBush): the fruiting bramble (SM_BlackberryBramble) at about 1 m, with
// the authored ripe clusters (SM_BerryBushProduce, built for a 60-80 cm shrub) scaled out onto its
// crown. The clusters go when she picks; the bramble's own green, red and odd black fruit stays.
constexpr float BerryBrambleScale = 0.8f;
constexpr int32 BerryBrambleFruit = 3;
constexpr float BerryFruitOffset = 24.0f, BerryFruitScale = 1.5f, BerryFruitLift = 20.0f;
TAutoConsoleVariable<int32> CVarRayTracedSun(TEXT("homestead.RayTracedSun"), 1,
    TEXT("1 = ray-traced sun/moon shadows with continuous sun movement (default when hardware ray "
         "tracing is on). 0 = Virtual Shadow Maps with the sun stepped by 0.5 degrees."));
TAutoConsoleVariable<float> CVarNightMoonLux(TEXT("homestead.NightMoonLux"), 2.0f,
    TEXT("Moonlight lux at full night."));
TAutoConsoleVariable<float> CVarNightSky(TEXT("homestead.NightSky"), 0.6f,
    TEXT("Sky light intensity at full night."));
TAutoConsoleVariable<float> CVarNightMinExposure(TEXT("homestead.NightMinExposure"), -2.0f,
    TEXT("Auto exposure min brightness at full night."));

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
const FLinearColor PreviewBlockedColor(0.86f, 0.33f, 0.26f);
const FLinearColor DeconstructColor(0.93f, 0.62f, 0.2f);

bool RegionalReachLess(const Homestead::RegionalGeneration::RiverReach& A,
    const Homestead::RegionalGeneration::RiverReach& B)
{
    return A.key.upstream != B.key.upstream ? A.key.upstream < B.key.upstream
        : A.key.downstream < B.key.downstream;
}

bool ProfileChunkPublishing()
{
    return FParse::Param(FCommandLine::Get(), TEXT("HomesteadGeneratedWoodland"));
}

// Blender-built woodland underbrush (Assets\Props, docs\blender-assets.md). Radius is the
// footprint half-width at scale 1; blocking species are brambles and hedges the heroine can't
// walk through until she clears them.
struct FUnderbrushSpecies
{
    const TCHAR* Asset;
    float Radius;
    float MinScale;
    float MaxScale;
    bool bBlocking;
    bool bShadow;
};
const FUnderbrushSpecies UnderbrushSpecies[] = {
    {TEXT("BlackberryBramble/SM_BlackberryBramble"), 105, 0.85f, 1.2f, true, true},
    {TEXT("BlackberryBramble/SM_BlackberryBrambleLarge"), 155, 0.9f, 1.15f, true, true},
    {TEXT("ToyonHedge/SM_ToyonHedge"), 135, 0.85f, 1.1f, true, true},
    {TEXT("Hazel/SM_Hazel"), 90, 0.8f, 1.15f, false, true},
    {TEXT("DeerBrush/SM_DeerBrush"), 72, 0.8f, 1.2f, false, true},
    {TEXT("Thimbleberry/SM_Thimbleberry"), 70, 0.8f, 1.2f, false, true},
    {TEXT("BrackenFern/SM_BrackenFern"), 75, 0.75f, 1.25f, false, false},
    {TEXT("WildStrawberry/SM_WildStrawberry"), 28, 0.8f, 1.3f, false, false},
    {TEXT("GrassYarrowTuft/SM_GrassYarrowTuft"), 30, 0.8f, 1.3f, false, false},
};
constexpr int32 UnderbrushSpeciesCount = UE_ARRAY_COUNT(UnderbrushSpecies);
// Keep the starting clearing (new games spawn at -1000,0) walkable and open.
const FVector2D StartingClearing(-1000, 0);
constexpr float StartingClearingBlockingRadius = 1600;
constexpr float StartingClearingShrubRadius = 550;

uint8 PickUnderbrush(FRandomStream& Random, std::initializer_list<std::pair<uint8, int32>> Weights)
{
    int32 Total = 0;
    for (const auto& Entry : Weights) Total += Entry.second;
    int32 Roll = Random.RandRange(0, Total - 1);
    for (const auto& Entry : Weights)
    {
        if (Roll < Entry.second) return Entry.first;
        Roll -= Entry.second;
    }
    return Weights.begin()->first;
}

// Blender-built Sierra granite (docs\blender-assets.md "Rocks"). Pivots are on the ground line
// and already sunk; Radius is the footprint half-width at scale 1. Size: 0 ground cluster,
// 1 knee-high boulder, 2 erratic, 3 house-sized.
struct FRockKind
{
    const TCHAR* Asset;
    float Radius;
    float MinScale;
    float MaxScale;
    uint8 Size;
};
const FRockKind RockKinds[] = {
    {TEXT("GraniteCobbles/SM_GraniteCobbles"), 45, 0.8f, 1.3f, 0},
    {TEXT("GraniteSpalls/SM_GraniteSpalls"), 40, 0.8f, 1.3f, 0},
    {TEXT("GraniteRubble/SM_GraniteRubble"), 40, 0.8f, 1.4f, 0},
    {TEXT("GraniteBoulderLoaf/SM_GraniteBoulderLoaf"), 62, 0.8f, 1.35f, 1},
    {TEXT("GraniteBlockTalus/SM_GraniteBlockTalus"), 52, 0.8f, 1.4f, 1},
    {TEXT("GraniteBoulderLow/SM_GraniteBoulderLow"), 72, 0.8f, 1.3f, 1},
    {TEXT("GraniteErratic/SM_GraniteErratic"), 150, 0.85f, 1.2f, 2},
    {TEXT("GraniteBoulderJointed/SM_GraniteBoulderJointed"), 160, 0.85f, 1.15f, 2},
    {TEXT("GraniteDome/SM_GraniteDome"), 470, 0.85f, 1.05f, 3},
    {TEXT("GraniteSplitBoulder/SM_GraniteSplitBoulder"), 400, 0.9f, 1.1f, 3},
};
constexpr int32 RockKindCount = UE_ARRAY_COUNT(RockKinds);
}

float AHomesteadWorld::RockRadius(uint8 Kind)
{
    return Kind < RockKindCount ? RockKinds[Kind].Radius : 0.0f;
}

void AHomesteadWorld::GenerateRocks(uint64 WorldSeed, FIntPoint Chunk, const FVector2D* KnobSite,
    TFunctionRef<bool(float, float, float, uint8)> IsFree, TArray<FHomesteadRock>& Out)
{
    enum : uint8 { Cobbles, Spalls, Rubble, Loaf, Talus, Low, Erratic, Jointed, Dome, Split };
    Out.Reset();
    const float OriginX = static_cast<float>(static_cast<int64>(Chunk.X) * Homestead::Generation::ChunkSizeCm);
    const float OriginY = static_cast<float>(static_cast<int64>(Chunk.Y) * Homestead::Generation::ChunkSizeCm);
    const float Size = static_cast<float>(Homestead::Generation::ChunkSizeCm);
    FRandomStream Random(static_cast<int32>(GetTypeHash(WorldSeed) ^ GetTypeHash(Chunk) ^ 0x6A7A17E5u));
    auto TryAdd = [&](uint8 Kind, float X, float Y) -> bool
    {
        const FRockKind& Rock = RockKinds[Kind];
        const float Scale = Random.FRandRange(Rock.MinScale, Rock.MaxScale);
        // +Y (the mossy face) stays roughly north, like the authored boulders.
        const float Yaw = Random.FRandRange(-40.0f, 40.0f) + (Rock.Size == 0 ? Random.FRandRange(0, 360) : 0);
        const float Tilt = Rock.Size == 1 ? 5.0f : Rock.Size == 0 ? 3.0f : 0.0f;
        const float Pitch = Random.FRandRange(-Tilt, Tilt);
        const float Roll = Random.FRandRange(-Tilt, Tilt);
        if (X < OriginX || Y < OriginY || X >= OriginX + Size || Y >= OriginY + Size) return false;
        const float Radius = Rock.Radius * Scale;
        for (const auto& Other : Out)
        {
            const float Spacing = 0.85f * (Radius + RockKinds[Other.Kind].Radius * Other.Scale);
            if (FVector2D::DistSquared(FVector2D(X, Y), FVector2D(Other.X, Other.Y)) < Spacing * Spacing)
                return false;
        }
        if (!IsFree(X, Y, Radius, Rock.Size)) return false;
        FHomesteadRock Placed;
        Placed.Kind = Kind;
        Placed.X = X;
        Placed.Y = Y;
        Placed.Yaw = Yaw;
        Placed.Scale = Scale;
        Placed.Pitch = Pitch;
        Placed.Roll = Roll;
        Out.Add(Placed);
        return true;
    };
    auto Around = [&](uint8 Kind, const FHomesteadRock& Anchor, float MinGap, float MaxGap)
    {
        const float Base = RockKinds[Anchor.Kind].Radius * Anchor.Scale + RockKinds[Kind].Radius;
        for (int32 Attempt = 0; Attempt < 4; ++Attempt)
        {
            const float Angle = Random.FRandRange(0.0f, UE_TWO_PI);
            const float Distance = Base + Random.FRandRange(MinGap, MaxGap);
            if (TryAdd(Kind, Anchor.X + FMath::Cos(Angle) * Distance, Anchor.Y + FMath::Sin(Angle) * Distance)) return;
        }
    };
    auto Outcrop = [&](FHomesteadRock Anchor, bool bHouse)
    {
        if (Random.FRand() < (bHouse ? 0.45f : 0.3f)) Around(Random.FRand() < 0.5f ? Erratic : Jointed, Anchor, 20, 260);
        const int32 Boulders = Random.RandRange(2, bHouse ? 5 : 3);
        for (int32 Index = 0; Index < Boulders; ++Index)
            Around(PickUnderbrush(Random, {{Loaf, 2}, {Talus, 3}, {Low, 2}}), Anchor, -20, 320);
        const int32 Clusters = Random.RandRange(3, bHouse ? 8 : 5);
        for (int32 Index = 0; Index < Clusters; ++Index)
            Around(PickUnderbrush(Random, {{Cobbles, 2}, {Spalls, 2}, {Rubble, 3}}), Anchor, -30, 420);
    };
    // Outcrops cluster in broad bands, like the granite knobs strewn through Sierra woodland.
    const float Band = FMath::PerlinNoise2D(FVector2D(OriginX / 7200.0f + 4.3f, OriginY / 7200.0f - 9.1f));
    const float OutcropChance = 0.12f + 0.4f * FMath::Max(0.0f, Band);
    if (KnobSite)
    {
        // The generator keeps trees and forage off the knob, so a dome or split boulder fits there.
        if (TryAdd(Random.FRand() < 0.55f ? Dome : Split, KnobSite->X, KnobSite->Y))
            Outcrop(Out.Last(), true);
        else if (TryAdd(Random.FRand() < 0.5f ? Erratic : Jointed, KnobSite->X, KnobSite->Y))
            Outcrop(Out.Last(), false);
    }
    else if (Random.FRand() < OutcropChance)
    {
        const uint8 AnchorKind = Random.FRand() < 0.5f ? Erratic : Jointed;
        const float Margin = RockKinds[AnchorKind].Radius;
        for (int32 Attempt = 0; Attempt < 14; ++Attempt)
        {
            const float X = OriginX + Random.FRandRange(Margin * 0.6f, Size - Margin * 0.6f);
            const float Y = OriginY + Random.FRandRange(Margin * 0.6f, Size - Margin * 0.6f);
            if (!TryAdd(AnchorKind, X, Y)) continue;
            Outcrop(Out.Last(), false);
            break;
        }
    }
    else if (Random.FRand() < 0.1f + 0.2f * FMath::Max(0.0f, Band))
    {
        // A lone erratic dropped among the trees.
        for (int32 Attempt = 0; Attempt < 5; ++Attempt)
            if (TryAdd(Random.FRand() < 0.5f ? Erratic : Jointed,
                OriginX + Random.FRandRange(200, Size - 200), OriginY + Random.FRandRange(200, Size - 200)))
                break;
    }
    if (Random.FRand() < 0.45f)
        TryAdd(PickUnderbrush(Random, {{Loaf, 2}, {Talus, 1}, {Low, 2}}),
            OriginX + Random.FRandRange(0, Size), OriginY + Random.FRandRange(0, Size));
    const int32 Scattered = Random.RandRange(1, 4);
    for (int32 Index = 0; Index < Scattered; ++Index)
        TryAdd(PickUnderbrush(Random, {{Cobbles, 3}, {Spalls, 2}, {Rubble, 3}}),
            OriginX + Random.FRandRange(0, Size), OriginY + Random.FRandRange(0, Size));
}

float AHomesteadWorld::UnderbrushDensity(float X, float Y)
{
    const float Broad = FMath::PerlinNoise2D(FVector2D(X / 3400.0f + 11.3f, Y / 3400.0f - 7.1f));
    const float Fine = FMath::PerlinNoise2D(FVector2D(X / 1100.0f - 3.7f, Y / 1100.0f + 19.9f));
    return FMath::Clamp(0.45f + Broad * 0.95f + Fine * 0.3f, 0.0f, 1.0f);
}

bool AHomesteadWorld::IsUnderbrushBlocking(uint8 Species)
{
    return Species < UnderbrushSpeciesCount && UnderbrushSpecies[Species].bBlocking;
}

float AHomesteadWorld::UnderbrushRadius(uint8 Species)
{
    return Species < UnderbrushSpeciesCount ? UnderbrushSpecies[Species].Radius : 0.0f;
}

FString AHomesteadWorld::UnderbrushName(uint8 Species)
{
    static const TCHAR* Names[] = {TEXT("Blackberry bramble"), TEXT("Blackberry thicket"), TEXT("Toyon hedge"),
        TEXT("Hazel"), TEXT("Deer brush"), TEXT("Thimbleberry"), TEXT("Bracken fern"), TEXT("Wild strawberry"),
        TEXT("Grass and yarrow")};
    static_assert(UE_ARRAY_COUNT(Names) == UnderbrushSpeciesCount);
    return Species < UnderbrushSpeciesCount ? Names[Species] : TEXT("Undergrowth");
}

bool AHomesteadWorld::FindUnderbrushNear(const Homestead::Simulation& Simulation, FVector2D Point, float Reach,
    FUnderbrushTarget& Out) const
{
    enum : uint8 { Hedge = 2, Hazel = 3, DeerBrush = 4, Strawberry = 7 };
    float Best = TNumericLimits<float>::Max();
    const int64 ChunkX = FMath::FloorToInt64(Point.X / Homestead::Generation::ChunkSizeCm);
    const int64 ChunkY = FMath::FloorToInt64(Point.Y / Homestead::Generation::ChunkSizeCm);
    for (int64 DY = -1; DY <= 1; ++DY)
        for (int64 DX = -1; DX <= 1; ++DX)
        {
            const FIntPoint Key(static_cast<int32>(ChunkX + DX), static_cast<int32>(ChunkY + DY));
            // Only chunks whose cover is live; a staged neighbour isn't visible yet.
            if (!TerrainChunks.Contains(Key)) continue;
            const auto* Plants = PlacedUnderbrush.Find(Key);
            if (!Plants) continue;
            for (const FHomesteadUnderbrush& Plant : *Plants)
            {
                if (Plant.Species >= Strawberry) continue;
                // Just cleared; the chunk's cover hasn't rebuilt without it yet.
                if (Simulation.IsUnderbrushCleared({Key.X, Key.Y}, Plant.Index)) continue;
                const float Radius = UnderbrushSpecies[Plant.Species].Radius * Plant.Scale;
                // Distance to the plant's near edge, not its stem.
                const float Edge = FVector2D::Distance(Point, FVector2D(Plant.X, Plant.Y)) - Radius * 0.8f;
                if (Edge > Reach || Edge >= Best) continue;
                Best = Edge;
                Out.Chunk = Key;
                Out.Index = Plant.Index;
                Out.Species = Plant.Species;
                Out.Position = FVector2D(Plant.X, Plant.Y);
                Out.Radius = Radius;
                Out.bWoody = Plant.Species == Hedge || Plant.Species == Hazel || Plant.Species == DeerBrush;
            }
        }
    return Best < TNumericLimits<float>::Max();
}

void AHomesteadWorld::GenerateUnderbrush(uint64 WorldSeed, FIntPoint Chunk, TArray<FHomesteadUnderbrush>& Out)
{
    enum : uint8 { Bramble, BrambleLarge, Hedge, Hazel, DeerBrush, Thimbleberry, Fern, Strawberry, Yarrow };
    Out.Reset();
    const float OriginX = static_cast<float>(static_cast<int64>(Chunk.X) * Homestead::Generation::ChunkSizeCm);
    const float OriginY = static_cast<float>(static_cast<int64>(Chunk.Y) * Homestead::Generation::ChunkSizeCm);
    const float Size = static_cast<float>(Homestead::Generation::ChunkSizeCm);
    FRandomStream Random(static_cast<int32>(GetTypeHash(WorldSeed) ^ GetTypeHash(Chunk) ^ 0x51F15EEDu));
    auto TryAdd = [&](uint8 Species, float X, float Y)
    {
        const float Scale = Random.FRandRange(UnderbrushSpecies[Species].MinScale, UnderbrushSpecies[Species].MaxScale);
        const float Yaw = Random.FRandRange(0.0f, 360.0f);
        if (X < OriginX || Y < OriginY || X >= OriginX + Size || Y >= OriginY + Size) return;
        const float Radius = UnderbrushSpecies[Species].Radius * Scale;
        for (const auto& Other : Out)
        {
            const float Spacing = 0.6f * (Radius + UnderbrushSpecies[Other.Species].Radius * Other.Scale);
            if (FVector2D::DistSquared(FVector2D(X, Y), FVector2D(Other.X, Other.Y)) < Spacing * Spacing) return;
        }
        FHomesteadUnderbrush Plant;
        Plant.Species = Species;
        Plant.Index = Out.Num();
        Plant.X = X;
        Plant.Y = Y;
        Plant.Yaw = Yaw;
        Plant.Scale = Scale;
        Out.Add(Plant);
    };
    // Clusters: meadow groundcover, loose shrub groups, or bramble and hedge thickets.
    for (int32 Cluster = 0; Cluster < 11; ++Cluster)
    {
        const float CX = OriginX + Random.FRandRange(0, Size);
        const float CY = OriginY + Random.FRandRange(0, Size);
        const float Density = UnderbrushDensity(CX, CY);
        const bool bThicket = Density >= 0.64f;
        const bool bShrubs = Density >= 0.32f;
        const int32 Count = bThicket ? 7 + FMath::FloorToInt((Density - 0.64f) * 40.0f)
            : bShrubs ? 3 + FMath::FloorToInt(Density * 9.0f) : Random.RandRange(4, 9);
        const float Spread = bThicket ? 420.0f : bShrubs ? 340.0f : 240.0f;
        for (int32 Plant = 0; Plant < Count; ++Plant)
        {
            const uint8 Species = bThicket
                ? PickUnderbrush(Random, {{Bramble, 5}, {BrambleLarge, 3}, {Hedge, 2}, {Thimbleberry, 1}, {Fern, 1}, {Hazel, 1}})
                : bShrubs
                ? PickUnderbrush(Random, {{Hazel, 2}, {DeerBrush, 3}, {Thimbleberry, 3}, {Fern, 4}, {Strawberry, 1}, {Yarrow, 1}})
                : PickUnderbrush(Random, {{Strawberry, 3}, {Yarrow, 3}, {Fern, 1}});
            const float Angle = Random.FRandRange(0.0f, UE_TWO_PI);
            const float Distance = Spread * FMath::Sqrt(Random.FRand());
            TryAdd(Species, CX + FMath::Cos(Angle) * Distance, CY + FMath::Sin(Angle) * Distance);
        }
    }
    // Scattered individuals so shrubs aren't only in clumps.
    for (int32 Attempt = 0; Attempt < 30; ++Attempt)
    {
        const float X = OriginX + Random.FRandRange(0, Size);
        const float Y = OriginY + Random.FRandRange(0, Size);
        const float Roll = Random.FRand();
        const float Density = UnderbrushDensity(X, Y);
        if (Roll > 0.12f + 0.6f * Density) continue;
        const uint8 Species = Density >= 0.35f
            ? PickUnderbrush(Random, {{Hazel, 2}, {DeerBrush, 2}, {Thimbleberry, 2}, {Fern, 3}, {Bramble, Density >= 0.6f ? 2 : 0}})
            : PickUnderbrush(Random, {{Strawberry, 2}, {Yarrow, 3}, {Fern, 1}});
        TryAdd(Species, X, Y);
    }
}

bool AHomesteadWorld::LoadCameraSafeFoliageMaterials()
{
    if (CameraSafeFoliageMaterials.Num() == 10) return true;
    CameraSafeFoliageMaterials.Reset();
    const TCHAR* Names[] = {
        TEXT("FirSaplingBranches"), TEXT("FirSaplingTwigs"), TEXT("Shrub"), TEXT("Flower"),
        TEXT("Grass"), TEXT("Fern"), TEXT("TreeSmallLeaves"), TEXT("MatureFirTwig"),
        TEXT("JacarandaLeaves"), TEXT("FirPoleTwigs")
    };
    for (const TCHAR* Name : Names)
    {
        const FString Path = FString::Printf(
            TEXT("/Game/SurvivalGame/Environment/CameraSafeFoliage/MI_CameraSafe_%s.MI_CameraSafe_%s"),
            Name, Name);
        auto* Material = LoadObject<UMaterialInterface>(nullptr, *Path);
        if (!Material)
        {
            UE_LOG(LogHomesteadWorld, Error,
                TEXT("Required camera-safe foliage material is missing: %s"), *Path);
            CameraSafeFoliageMaterials.Reset();
            return false;
        }
        CameraSafeFoliageMaterials.Add(FName(Name), Material);
    }
    return true;
}

bool AHomesteadWorld::ApplyCameraSafeFoliageMaterials(UMeshComponent& Component)
{
    UStaticMesh* Mesh = Cast<UStaticMeshComponent>(&Component)
        ? CastChecked<UStaticMeshComponent>(&Component)->GetStaticMesh()
        : Cast<UHierarchicalInstancedStaticMeshComponent>(&Component)
            ? CastChecked<UHierarchicalInstancedStaticMeshComponent>(&Component)->GetStaticMesh()
            : nullptr;
    if (!Mesh) return true;

    struct FContract
    {
        const TCHAR* MeshPath;
        TArray<TPair<int32, FName>> Slots;
    };
    const TArray<FContract> Contracts = {
        {TEXT("/Game/Trials/WoodlandResources_20260921_01/Meshes/SM_FirSapling_a.SM_FirSapling_a"),
            {{0, TEXT("FirSaplingBranches")}, {1, TEXT("FirSaplingTwigs")}}},
        {TEXT("/Game/Trials/WoodlandResources_20260921_01/Meshes/SM_FirSapling_c.SM_FirSapling_c"),
            {{0, TEXT("FirSaplingBranches")}, {1, TEXT("FirSaplingTwigs")}}},
        {TEXT("/Game/Trials/WoodlandResources_20260921_01/Meshes/SM_Shrub04_a.SM_Shrub04_a"),
            {{0, TEXT("Shrub")}}},
        {TEXT("/Game/Trials/WoodlandResources_20260921_01/Meshes/SM_Shrub04_c.SM_Shrub04_c"),
            {{0, TEXT("Shrub")}}},
        {TEXT("/Game/Trials/WoodlandResources_20260921_01/Meshes/SM_FlowerEmpodium_a.SM_FlowerEmpodium_a"),
            {{0, TEXT("Flower")}}},
        {TEXT("/Game/Trials/WoodlandResources_20260921_01/Meshes/SM_FlowerEmpodium_b.SM_FlowerEmpodium_b"),
            {{0, TEXT("Flower")}}},
        {TEXT("/Game/Trials/GrassGround_20260921_01/Meshes/SM_GrassMedium01_mid_b.SM_GrassMedium01_mid_b"),
            {{0, TEXT("Grass")}}},
        {TEXT("/Game/Trials/GrassGround_20260921_01/Meshes/SM_GrassMedium01_small_b.SM_GrassMedium01_small_b"),
            {{0, TEXT("Grass")}}},
        {TEXT("/Game/Trials/GrassGround_20260921_01/Meshes/SM_GrassMedium01_tall_a.SM_GrassMedium01_tall_a"),
            {{0, TEXT("Grass")}}},
        {TEXT("/Game/Trials/GrassGround_20260921_01/Meshes/SM_GrassMedium01_tiny_a.SM_GrassMedium01_tiny_a"),
            {{0, TEXT("Grass")}}},
        {TEXT("/Game/Trials/Fern02_20260920_01/Meshes/SM_Fern02_a.SM_Fern02_a"),
            {{0, TEXT("Fern")}}},
        {TEXT("/Game/Trials/Fern02_20260920_01/Meshes/SM_Fern02_b.SM_Fern02_b"),
            {{0, TEXT("Fern")}}},
        {TEXT("/Game/Trials/Fern02_20260920_01/Meshes/SM_Fern02_c.SM_Fern02_c"),
            {{0, TEXT("Fern")}}},
        {TEXT("/Game/Trials/Fern02_20260920_01/Meshes/SM_Fern02_d.SM_Fern02_d"),
            {{0, TEXT("Fern")}}},
        {TEXT("/Game/Trials/WoodlandResources_20260921_01/Meshes/SM_TreeSmall02_Woodland.SM_TreeSmall02_Woodland"),
            {{1, TEXT("TreeSmallLeaves")}}},
        {TEXT("/Game/Trials/MatureFir_20260922_02/Meshes/SM_MatureFir.SM_MatureFir"),
            {{1, TEXT("MatureFirTwig")}}},
        {TEXT("/Game/Trials/TreePalette_20260921_01/Meshes/SM_Jacaranda.SM_Jacaranda"),
            {{2, TEXT("JacarandaLeaves")}}},
        {TEXT("/Game/Trials/TreePalette_20260921_01/Meshes/SM_FirPole.SM_FirPole"),
            {{1, TEXT("FirPoleTwigs")}}}
    };

    const FContract* Contract = Contracts.FindByPredicate(
        [Mesh](const FContract& Value) { return Mesh->GetPathName() == Value.MeshPath; });
    if (!Contract) return true;
    if (!LoadCameraSafeFoliageMaterials()) return false;
    for (const auto& Slot : Contract->Slots)
    {
        const TObjectPtr<UMaterialInterface>* Material = CameraSafeFoliageMaterials.Find(Slot.Value);
        if (!Material || !Material->Get() || Slot.Key < 0 || Slot.Key >= Mesh->GetStaticMaterials().Num())
        {
            UE_LOG(LogHomesteadWorld, Error,
                TEXT("Camera-safe foliage contract differs for %s slot %d (%s)."),
                *Mesh->GetPathName(), Slot.Key, *Slot.Value.ToString());
            return false;
        }
        Component.SetMaterial(Slot.Key, Material->Get());
    }
    Component.SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
    Component.ComponentTags.AddUnique(TEXT("CameraSafeFoliage"));
    return true;
}

namespace
{
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
    UpdateFallingTree(DeltaSeconds);
    UpdateHearthFlicker(DeltaSeconds);
    UpdateHearthSound(DeltaSeconds);
    UpdateClearPops(DeltaSeconds);
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
    if (bStagingResourceBuild)
    {
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetVisibility(false);
        Part->SetHiddenInGame(true);
    }
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

namespace
{
// The MVP woodland's interactables (add-mvp-woodland-biome; Scripts/Terrain/mvp_woodland.py).
constexpr int32 MvpWoodlandIdBase = 560000;
constexpr int32 MvpWoodlandIdEnd = 570000;
bool IsMvpWoodlandId(int32 Id) { return Id >= MvpWoodlandIdBase && Id < MvpWoodlandIdEnd; }

struct FEstateSceneryKind
{
    const TCHAR* Path;
    bool bCollision;
    float CullCm;
    bool bTree;
    // Trees: how far the root flare's rim rises above the mesh's lowest vertex. Other kinds: how far
    // to sink the pivot (cm at scale 1), as the woodland's underbrush roots into uneven ground.
    float RimLift;
    // Trees: the root flare's reach. Other kinds: the footprint radius they settle on (lowest
    // ground under it, as the woodland's granite does); 0 sits on the ground at the pivot.
    float Footprint;
    bool bShadow = false;
};
// Index = the kind byte written by Scripts/Terrain/scatter.py (19+: mvp_woodland.py). Keep them in step.
const FEstateSceneryKind EstateSceneryKinds[] = {
    {TEXT("/Game/Trials/WoodlandResources_20260921_01/Meshes/SM_TreeSmall02_Woodland.SM_TreeSmall02_Woodland"), true, 0, true, 3, 41},
    {TEXT("/Game/Trials/MatureFir_20260922_02/Meshes/SM_MatureFir.SM_MatureFir"), true, 0, true, 30, 76},
    {TEXT("/Game/SurvivalGame/Environment/Props/Hazel/SM_Hazel.SM_Hazel"), false, 14000, false, 0, 0},
    {TEXT("/Game/SurvivalGame/Environment/Props/BrackenFern/SM_BrackenFern.SM_BrackenFern"), false, 9000, false, 0, 0},
    {TEXT("/Game/SurvivalGame/Environment/Props/GrassYarrowTuft/SM_GrassYarrowTuft.SM_GrassYarrowTuft"), false, 6000, false, 0, 0},
    {TEXT("/Game/SurvivalGame/Environment/Props/GraniteCobbles/SM_GraniteCobbles.SM_GraniteCobbles"), false, 9000, false, 0, 0},
    {TEXT("/Game/SurvivalGame/Environment/Props/GraniteBoulderLoaf/SM_GraniteBoulderLoaf.SM_GraniteBoulderLoaf"), true, 24000, false, 0, 0},
    {TEXT("/Game/SurvivalGame/Environment/Props/GraniteErratic/SM_GraniteErratic.SM_GraniteErratic"), true, 60000, false, 0, 0},
    {TEXT("/Game/SurvivalGame/Environment/Props/GraniteDome/SM_GraniteDome.SM_GraniteDome"), true, 0, false, 0, 0},
    {TEXT("/Game/Trials/Fern02_20260920_01/Meshes/SM_Fern02_a.SM_Fern02_a"), false, 7000, false, 0, 0},
    {TEXT("/Game/Trials/GrassGround_20260921_01/Meshes/SM_GrassMedium01_tall_a.SM_GrassMedium01_tall_a"), false, 4500, false, 0, 0},
    {TEXT("/Game/Trials/GrassGround_20260921_01/Meshes/SM_GrassMedium01_mid_b.SM_GrassMedium01_mid_b"), false, 4500, false, 0, 0},
    {TEXT("/Game/Trials/WoodlandResources_20260921_01/Meshes/SM_Shrub04_a.SM_Shrub04_a"), false, 12000, false, 0, 0},
    // Mature Cornish woodland (Blender recipes oak.py, beech.py, sycamore.py): the pivot is the bottom of a
    // 30 cm skirt under the flare, so RimLift 30 puts the ground line 4 cm under the lowest root sample.
    {TEXT("/Game/SurvivalGame/Environment/Trees/Oak/SM_Oak.SM_Oak"), true, 0, true, 30, 60},
    {TEXT("/Game/SurvivalGame/Environment/Trees/Beech/SM_Beech.SM_Beech"), true, 0, true, 30, 50},
    {TEXT("/Game/SurvivalGame/Environment/Trees/Sycamore/SM_Sycamore.SM_Sycamore"), true, 0, true, 30, 45},
    // Windswept hawthorn (hawthorn.py) for wood fringes and hedges. Its capsule leans downwind (+X), so the
    // capsule centre sits ~46 cm from the trunk base: a 50 cm footprint keeps the root ring round the base.
    {TEXT("/Game/SurvivalGame/Environment/Trees/Hawthorn/SM_Hawthorn.SM_Hawthorn"), true, 0, true, 30, 50},
    // Woodland understory shrubs (holly.py, hazel_coppice.py): walk-through, no collision.
    {TEXT("/Game/SurvivalGame/Environment/Trees/Holly/SM_Holly.SM_Holly"), false, 14000, false, 0, 0},
    {TEXT("/Game/SurvivalGame/Environment/Trees/HazelCoppice/SM_HazelCoppice.SM_HazelCoppice"), false, 14000, false, 0, 0},
    // The MVP woodland (add-mvp-woodland-biome), with the MVP's cull distances and shadows. Its trees'
    // rim lift and flare reach are ResolveGeneratedTreeVisual's.
    {TEXT("/Game/Trials/TreePalette_20260921_01/Meshes/SM_Jacaranda.SM_Jacaranda"), true, 0, true, 19, 240},
    {TEXT("/Game/Trials/TreePalette_20260921_01/Meshes/SM_FirPole.SM_FirPole"), true, 0, true, 0, 20},
    {TEXT("/Game/Trials/WoodlandResources_20260921_01/Meshes/SM_FirSapling_a.SM_FirSapling_a"), false, 9000, false, 0, 0, true},
    {TEXT("/Game/Trials/WoodlandResources_20260921_01/Meshes/SM_FirSapling_c.SM_FirSapling_c"), false, 9000, false, 0, 0, true},
    {TEXT("/Game/SurvivalGame/Environment/Props/BlackberryBramble/SM_BlackberryBramble.SM_BlackberryBramble"), false, 9000, false, 4, 0, true},
    {TEXT("/Game/SurvivalGame/Environment/Props/BlackberryBramble/SM_BlackberryBrambleLarge.SM_BlackberryBrambleLarge"), false, 9000, false, 4, 0, true},
    {TEXT("/Game/SurvivalGame/Environment/Props/ToyonHedge/SM_ToyonHedge.SM_ToyonHedge"), false, 9000, false, 4, 0, true},
    {TEXT("/Game/SurvivalGame/Environment/Props/DeerBrush/SM_DeerBrush.SM_DeerBrush"), false, 9000, false, 4, 0, true},
    {TEXT("/Game/SurvivalGame/Environment/Props/Thimbleberry/SM_Thimbleberry.SM_Thimbleberry"), false, 9000, false, 4, 0, true},
    {TEXT("/Game/SurvivalGame/Environment/Props/WildStrawberry/SM_WildStrawberry.SM_WildStrawberry"), false, 3800, false, 4, 0},
    {TEXT("/Game/Trials/Fern02_20260920_01/Meshes/SM_Fern02_b.SM_Fern02_b"), false, 7000, false, 0, 0},
    {TEXT("/Game/Trials/Fern02_20260920_01/Meshes/SM_Fern02_c.SM_Fern02_c"), false, 7000, false, 0, 0},
    {TEXT("/Game/Trials/Fern02_20260920_01/Meshes/SM_Fern02_d.SM_Fern02_d"), false, 7000, false, 0, 0},
    {TEXT("/Game/Trials/GrassGround_20260921_01/Meshes/SM_GrassMedium01_small_b.SM_GrassMedium01_small_b"), false, 4500, false, 0, 0},
    {TEXT("/Game/Trials/GrassGround_20260921_01/Meshes/SM_GrassMedium01_tiny_a.SM_GrassMedium01_tiny_a"), false, 4500, false, 0, 0},
    {TEXT("/Game/Trials/WoodlandResources_20260921_01/Meshes/SM_FlowerEmpodium_a.SM_FlowerEmpodium_a"), false, 4800, false, 0, 0},
    {TEXT("/Game/Trials/WoodlandResources_20260921_01/Meshes/SM_FlowerEmpodium_b.SM_FlowerEmpodium_b"), false, 4800, false, 0, 0},
    {TEXT("/Game/SurvivalGame/Environment/Props/GraniteSpalls/SM_GraniteSpalls.SM_GraniteSpalls"), false, 9000, false, 0, 40, true},
    {TEXT("/Game/SurvivalGame/Environment/Props/GraniteRubble/SM_GraniteRubble.SM_GraniteRubble"), false, 9000, false, 0, 40, true},
    {TEXT("/Game/SurvivalGame/Environment/Props/GraniteBlockTalus/SM_GraniteBlockTalus.SM_GraniteBlockTalus"), true, 24000, false, 0, 52},
    {TEXT("/Game/SurvivalGame/Environment/Props/GraniteBoulderLow/SM_GraniteBoulderLow.SM_GraniteBoulderLow"), true, 24000, false, 0, 72},
    {TEXT("/Game/SurvivalGame/Environment/Props/GraniteBoulderJointed/SM_GraniteBoulderJointed.SM_GraniteBoulderJointed"), true, 60000, false, 0, 160},
    {TEXT("/Game/SurvivalGame/Environment/Props/GraniteSplitBoulder/SM_GraniteSplitBoulder.SM_GraniteSplitBoulder"), true, 0, false, 0, 400},
};

#pragma pack(push, 1)
struct FEstateSceneryRecord
{
    uint8 Kind;
    uint8 Pad[3];
    float X, Y, Yaw, Scale;
};
#pragma pack(pop)
static_assert(sizeof(FEstateSceneryRecord) == 20, "EstateScenery.bin records are 20 bytes");
}

bool AHomesteadWorld::BuildEstateScenery()
{
    if (bEstateSceneryBuilt) return true;
    const FString Path = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("SurvivalGame/Estate/Runtime/EstateScenery.bin"));
    TArray<uint8> Raw;
    if (!FFileHelper::LoadFileToArray(Raw, *Path) || Raw.Num() < 8 || FMemory::Memcmp(Raw.GetData(), "HSC1", 4) != 0)
    {
        UE_LOG(LogHomesteadWorld, Warning, TEXT("Estate scenery is missing or unreadable: %s"), *Path);
        bEstateSceneryBuilt = true;
        return true;
    }
    uint32 Count = 0;
    FMemory::Memcpy(&Count, Raw.GetData() + 4, 4);
    if (Raw.Num() != 8 + static_cast<int64>(Count) * sizeof(FEstateSceneryRecord))
    {
        UE_LOG(LogHomesteadWorld, Error, TEXT("Estate scenery has %d bytes for %u records."), Raw.Num(), Count);
        bEstateSceneryBuilt = true;
        return true;
    }
    const auto* Records = reinterpret_cast<const FEstateSceneryRecord*>(Raw.GetData() + 8);
    constexpr int32 KindCount = UE_ARRAY_COUNT(EstateSceneryKinds);
    UHierarchicalInstancedStaticMeshComponent* Batches[KindCount] = {};
    TArray<FTransform> Transforms[KindCount];
    for (uint32 Index = 0; Index < Count; ++Index)
    {
        const FEstateSceneryRecord& Record = Records[Index];
        if (Record.Kind >= KindCount) continue;
        const FEstateSceneryKind& Kind = EstateSceneryKinds[Record.Kind];
        if (!Batches[Record.Kind])
        {
            UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, Kind.Path);
            if (!Mesh)
            {
                UE_LOG(LogHomesteadWorld, Warning, TEXT("Estate scenery mesh missing: %s"), Kind.Path);
                continue;
            }
            auto* Batch = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
            Batch->SetupAttachment(GetRootComponent());
            Batch->SetMobility(EComponentMobility::Static);
            Batch->SetStaticMesh(Mesh);
            Batch->SetCollisionProfileName(Kind.bCollision ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
            Batch->SetCollisionEnabled(Kind.bCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
            Batch->SetCanEverAffectNavigation(false);
            Batch->SetCastShadow(Kind.bTree || Kind.bCollision || Kind.bShadow);
            // As the woodland's underbrush: gentle sway needn't redraw cached shadow pages every frame.
            if (Kind.bShadow && !Kind.bTree) Batch->ShadowCacheInvalidationBehavior = EShadowCacheInvalidationBehavior::Rigid;
            if (Kind.CullCm > 0) Batch->SetCullDistances(static_cast<int32>(Kind.CullCm * 0.8f), static_cast<int32>(Kind.CullCm));
            // Wind sway only near her: beyond 60 m it's invisible, and animated Nanite foliage there would
            // keep invalidating the cached virtual shadow maps of the whole wood every frame.
            Batch->SetWorldPositionOffsetDisableDistance(6000);
            Batch->ComponentTags.Add(TEXT("EstateScenery"));
            ApplyCameraSafeFoliageMaterials(*Batch);
            Batches[Record.Kind] = Batch;
        }
        UStaticMesh* Mesh = Batches[Record.Kind]->GetStaticMesh();
        const FRotator Rotation(0, Record.Yaw, 0);
        FVector Base(Record.X, Record.Y, HomesteadEstateTerrain::Height(Record.X, Record.Y));
        FVector Anchor = FVector::ZeroVector;
        if (Kind.bTree && Mesh->GetBodySetup() && Mesh->GetBodySetup()->AggGeom.SphylElems.Num() > 0)
        {
            // Sink the root flare as the woodland trees do, so no base floats on a slope.
            const auto& Capsule = Mesh->GetBodySetup()->AggGeom.SphylElems[0];
            Anchor = FVector(Capsule.Center.X, Capsule.Center.Y, Mesh->GetBoundingBox().Min.Z);
            const float Radius = FMath::Max(Capsule.Radius, Kind.Footprint) * Record.Scale;
            float Root = Base.Z;
            for (int32 Step = 0; Step < 8; ++Step)
            {
                const float Angle = Step * UE_PI / 4.0f;
                Root = FMath::Min(Root, HomesteadEstateTerrain::Height(Record.X + Radius * FMath::Cos(Angle), Record.Y + Radius * FMath::Sin(Angle)));
            }
            Base.Z = Root - (4.0f + Kind.RimLift * Record.Scale);
        }
        else if (!Kind.bTree)
        {
            const float Reach = Kind.Footprint * Record.Scale * 0.55f;
            for (int32 Step = 0; Reach > 0 && Step < 6; ++Step)
            {
                const float Angle = Step * UE_TWO_PI / 6.0f;
                Base.Z = FMath::Min(Base.Z, HomesteadEstateTerrain::Height(Record.X + Reach * FMath::Cos(Angle), Record.Y + Reach * FMath::Sin(Angle)));
            }
            Base.Z -= Kind.RimLift * Record.Scale;
        }
        Transforms[Record.Kind].Add(FTransform(Rotation, Base - Rotation.RotateVector(Anchor * Record.Scale), FVector(Record.Scale)));
    }
    int32 Total = 0;
    for (int32 Kind = 0; Kind < KindCount; ++Kind)
    {
        if (!Batches[Kind]) continue;
        Batches[Kind]->RegisterComponent();
        Batches[Kind]->AddInstances(Transforms[Kind], false, true);
        EstateScenery.Add(Batches[Kind]);
        const FEstateSceneryKind& Info = EstateSceneryKinds[Kind];
        const FVector Extent = Batches[Kind]->GetStaticMesh()->GetBounds().BoxExtent;
        EstateSceneryClearRadius.Add(Info.bTree || Info.bCollision ? 0.0f : FMath::Max(Extent.X, Extent.Y) * 0.7f);
        EstateSceneryTrunkRadius.Add(Info.bTree ? Info.Footprint : 0.0f);
        EstateSceneryHidden.Add(TBitArray<>(false, Transforms[Kind].Num()));
        EstateSceneryTransforms.Add(MoveTemp(Transforms[Kind]));
        Total += EstateSceneryTransforms.Last().Num();
    }
    bEstateSceneryBuilt = true;
    UE_LOG(LogHomesteadWorld, Display, TEXT("Estate scenery: %d instances in %d batches."), Total, EstateScenery.Num());
    return true;
}

void AHomesteadWorld::ClearEstateSceneryUnderPieces(const Homestead::State& State)
{
    TArray<Homestead::Footprint> Pieces;
    FString Signature;
    for (const auto& Structure : State.structures)
    {
        const auto Box = Homestead::StructureFootprint(State, Structure);
        Pieces.Add(Box);
        Signature += FString::Printf(TEXT("%.0f:%.0f:%.0f:%.0f:%.1f;"), Box.center.x, Box.center.y, Box.half.x, Box.half.y, Box.yaw);
    }
    // Interactables, bucketed by 20 m cell; the cover or trunk reach never spans more than a cell.
    constexpr double NodeCell = 2000.0;
    TMap<FIntPoint, TArray<FVector2D>> Nodes;
    for (const auto& Node : State.resources)
    {
        Nodes.FindOrAdd(FIntPoint(FMath::FloorToInt32(Node.position.x / NodeCell), FMath::FloorToInt32(Node.position.y / NodeCell)))
            .Add(FVector2D(Node.position.x, Node.position.y));
        Signature += FString::Printf(TEXT("n%d:%.0f:%.0f;"), Node.id, Node.position.x, Node.position.y);
    }
    if (Signature == EstateSceneryClearSignature) return;
    EstateSceneryClearSignature = MoveTemp(Signature);
    constexpr float Margin = 20.0f;
    // Clear ground round an interactable: enough to show a berry bush or herb tuft whole.
    constexpr float NodeCoverClear = 110.0f;
    constexpr float NodeTrunkClear = 170.0f;
    auto NearNode = [&Nodes, NodeCell](const FVector& Location, float Reach)
    {
        const FIntPoint Cell(FMath::FloorToInt32(Location.X / NodeCell), FMath::FloorToInt32(Location.Y / NodeCell));
        for (int32 DX = -1; DX <= 1; ++DX)
            for (int32 DY = -1; DY <= 1; ++DY)
                if (const TArray<FVector2D>* Found = Nodes.Find(Cell + FIntPoint(DX, DY)))
                    for (const FVector2D& Node : *Found)
                        if (FVector2D::DistSquared(Node, FVector2D(Location)) < Reach * Reach) return true;
        return false;
    };
    for (int32 Batch = 0; Batch < EstateScenery.Num(); ++Batch)
    {
        const float Radius = EstateSceneryClearRadius[Batch];
        const float Trunk = EstateSceneryTrunkRadius.IsValidIndex(Batch) ? EstateSceneryTrunkRadius[Batch] : 0.0f;
        if ((Radius <= 0 && Trunk <= 0) || !EstateScenery[Batch]) continue;
        const TArray<FTransform>& Transforms = EstateSceneryTransforms[Batch];
        TBitArray<>& Hidden = EstateSceneryHidden[Batch];
        bool bChanged = false;
        for (int32 Index = 0; Index < Transforms.Num(); ++Index)
        {
            const FVector Location = Transforms[Index].GetLocation();
            const float Scale = Transforms[Index].GetScale3D().X;
            if (Radius <= 0)
            {
                const bool bBlocking = NearNode(Location, NodeTrunkClear + Trunk * Scale);
                if (Hidden[Index] == bBlocking) continue;
                Hidden[Index] = bBlocking;
                FTransform Shown = Transforms[Index];
                if (bBlocking) Shown.SetScale3D(FVector(0.0001f));
                EstateScenery[Batch]->UpdateInstanceTransform(Index, Shown, true, false, true);
                bChanged = true;
                continue;
            }
            const float Reach = Radius * Scale + Margin;
            bool bUnder = NearNode(Location, NodeCoverClear + Radius * Scale * 0.5f);
            for (const auto& Box : Pieces)
            {
                const double Near = FMath::Max(Box.half.x, Box.half.y) + Reach;
                if (FMath::Abs(Location.X - Box.center.x) > Near || FMath::Abs(Location.Y - Box.center.y) > Near) continue;
                const Homestead::Point Local = Homestead::RotateYaw({Location.X - Box.center.x, Location.Y - Box.center.y}, -Box.yaw);
                const double DX = FMath::Max(0.0, FMath::Abs(Local.x) - Box.half.x);
                const double DY = FMath::Max(0.0, FMath::Abs(Local.y) - Box.half.y);
                if (DX * DX + DY * DY < Reach * Reach) { bUnder = true; break; }
            }
            if (Hidden[Index] == bUnder) continue;
            Hidden[Index] = bUnder;
            FTransform Shown = Transforms[Index];
            if (bUnder) Shown.SetScale3D(FVector(0.0001f));
            EstateScenery[Batch]->UpdateInstanceTransform(Index, Shown, true, false, true);
            bChanged = true;
        }
        if (bChanged) EstateScenery[Batch]->MarkRenderStateDirty();
    }
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
        return BuildEstateScenery();
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
    // Virtual Shadow Maps give real canopy shadows, and a low sun lights upright trunks and the
    // heroine head-on while the ground only gets grazing light. Compress local highlights and lift
    // local shade so dawn doesn't clip sunlit bark or crush nearby shade to black.
    Settings.bOverride_LocalExposureHighlightContrastScale = true;
    Settings.LocalExposureHighlightContrastScale = 0.5f;
    Settings.bOverride_LocalExposureShadowContrastScale = true;
    Settings.LocalExposureShadowContrastScale = 0.8f;
    // Near-horizon sunlight is deep orange after atmospheric transmittance; slightly desaturating
    // highlights stops saturated albedo (the green dress) from hue-clipping to flat yellow.
    Settings.bOverride_ColorSaturationHighlights = true;
    Settings.ColorSaturationHighlights = FVector4(0.85f, 0.85f, 0.85f, 1.0f);
    Settings.bOverride_AutoExposureSpeedUp = true;
    Settings.AutoExposureSpeedUp = 3.0f;
    Settings.bOverride_AutoExposureSpeedDown = true;
    Settings.AutoExposureSpeedDown = 1.0f;
    Exposure->RegisterComponent();

    Sun = NewObject<UDirectionalLightComponent>(this, TEXT("MeadowSun"));
    Sun->SetupAttachment(GetRootComponent());
    Sun->SetMobility(EComponentMobility::Movable);
    Sun->bAtmosphereSunLight = true;
    // The moon is also an atmosphere light; the sun wins forward shading (water, translucency, fog).
    Sun->ForwardShadingPriority = 1;
    Sun->SetIntensity(46000.0f);
    // Ray-traced sun shadows have no cache, so the sun can move every refresh without the Virtual
    // Shadow Map re-render stalls a rotating sun causes over these non-Nanite trees (4K: 85 vs 78 FPS,
    // p99 15 vs 24 ms). UpdateLighting applies homestead.RayTracedSun; without hardware ray tracing
    // the lights use VSM.
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
        const auto Center = Homestead::StructureCenter(State, Structure);
        if (FVector2D(X - Center.x, Y - Center.y).Size() < OccupiedRadius + 225.0f)
            return true;
    }
    for (const auto& Plot : State.plots)
    {
        const auto Center = Homestead::PlotCenter(Plot);
        if (FVector2D(X - Center.x, Y - Center.y).Size() < OccupiedRadius + 70.0f)
            return true;
    }
    for (const auto& Drop : State.worldDrops)
        if (FVector2D(X - Drop.position.x, Y - Drop.position.y).Size()
            < FootprintRadius + 45.0f)
            return true;
    return false;
}

bool AHomesteadWorld::BuildDecorations(const Homestead::Simulation& Simulation,
    const FIntPoint* StageChunk)
{
    const auto& State = Simulation.GetState();
    const double Started = FPlatformTime::Seconds();
    double CoverScanMilliseconds = 0;
    double CoverTeardownMilliseconds = 0;
    double BatchSetupMilliseconds = 0;
    double InstanceMilliseconds = 0;
    double TreeBuildMilliseconds = 0;
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
    int32 UnderbrushCount = 0;
    int32 BlockingUnderbrushCount = 0;
    UStaticMesh* UnderbrushMeshes[UnderbrushSpeciesCount] = {};
    for (int32 Species = 0; Species < UnderbrushSpeciesCount; ++Species)
    {
        const FString Asset(UnderbrushSpecies[Species].Asset);
        const FString Name = FPaths::GetCleanFilename(Asset);
        UnderbrushMeshes[Species] = LoadObject<UStaticMesh>(nullptr,
            *FString::Printf(TEXT("/Game/SurvivalGame/Environment/Props/%s.%s"), *Asset, *Name));
        if (!UnderbrushMeshes[Species])
            UE_LOG(LogHomesteadWorld, Warning, TEXT("Underbrush %s is not imported; it is skipped."), *Asset);
    }
    int32 RockCount = 0;
    int32 BigRockCount = 0;
    UStaticMesh* RockMeshes[RockKindCount] = {};
    for (int32 Kind = 0; Kind < RockKindCount; ++Kind)
    {
        const FString Asset(RockKinds[Kind].Asset);
        const FString Name = FPaths::GetCleanFilename(Asset);
        RockMeshes[Kind] = LoadObject<UStaticMesh>(nullptr,
            *FString::Printf(TEXT("/Game/SurvivalGame/Environment/Props/%s.%s"), *Asset, *Name));
        if (!RockMeshes[Kind])
            UE_LOG(LogHomesteadWorld, Warning, TEXT("Granite %s is not imported; it is skipped."), *Asset);
    }
    int32 RebuiltChunks = 0;
    struct FChunkCoverWork
    {
        FIntPoint Key;
        FHomesteadTerrainChunk* Value;
    };
    TArray<FChunkCoverWork> Work;
    if (StageChunk)
    {
        if (TerrainChunks.Contains(*StageChunk))
        {
            UE_LOG(LogHomesteadWorld, Error, TEXT("Cover staging cannot replace a live terrain chunk."));
            return false;
        }
        Work.Add({*StageChunk, &StagedCoverChunks.FindOrAdd(*StageChunk)});
    }
    else
        for (auto& Chunk : TerrainChunks)
            Work.Add({Chunk.Key, &Chunk.Value});
    for (auto& Chunk : Work)
    {
        const double ScanStarted = FPlatformTime::Seconds();
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
            if (Nearby(Homestead::StructureCenter(State, Structure).x, Homestead::StructureCenter(State, Structure).y))
                Signature += FString::Printf(TEXT("S%d;"), Structure.id);
        for (const auto& Plot : State.plots)
            if (Nearby(Homestead::PlotCenter(Plot).x, Homestead::PlotCenter(Plot).y))
                Signature += FString::Printf(TEXT("P%d;"), Plot.id);
        for (const auto& Cut : State.clearedUnderbrush)
            if (Cut.chunk.x == Chunk.Key.X && Cut.chunk.y == Chunk.Key.Y)
                Signature += FString::Printf(TEXT("U%d;"), Cut.index);
        Signature += TEXT("natural-creek-v1-decorative-wildflower-v1-underbrush-v2-granite-v2");
        if (!StageChunk && bStagingResourceWindow
            && StagedChunk == State.activeChunk && StagedWorld.seed == State.world.seed
            && StagedWorld.generationVersion == State.world.generationVersion
            && Simulation.GetRevision() == StagedSourceRevision + 1)
            if (auto* Prepared = StagedCoverChunks.Find(Chunk.Key);
                Prepared && Prepared->CoverSignature == Signature)
            {
                ClearVisual(Chunk.Value->Cover);
                Chunk.Value->Cover = MoveTemp(Prepared->Cover);
                Chunk.Value->CoverSignature = Signature;
                StagedCoverChunks.Remove(Chunk.Key);
                for (USceneComponent* Component : Chunk.Value->Cover.Components)
                {
                    Component->SetVisibility(true);
                    Component->SetHiddenInGame(false);
                    auto* Primitive = Cast<UPrimitiveComponent>(Component);
                    if (Primitive && (Primitive->GetCollisionProfileName() == UCollisionProfile::BlockAll_ProfileName
                        || Primitive->ComponentHasTag(TEXT("HomesteadBlocking"))))
                        Primitive->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
                }
                ++RebuiltChunks;
                ++LastTransitionStagedCoverChunks;
                continue;
            }
        if (Chunk.Value->CoverSignature == Signature)
        {
            CoverScanMilliseconds += (FPlatformTime::Seconds() - ScanStarted) * 1000;
            continue;
        }
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
        CoverScanMilliseconds += (FPlatformTime::Seconds() - ScanStarted) * 1000;
        const double TeardownStarted = FPlatformTime::Seconds();
        ClearVisual(Chunk.Value->Cover);
        CoverTeardownMilliseconds += (FPlatformTime::Seconds() - TeardownStarted) * 1000;
        ++RebuiltChunks;
        const uint32 Seed = GetTypeHash(State.world.seed) ^ GetTypeHash(Chunk.Key);
        FRandomStream Random(static_cast<int32>(Seed));
        FRandomStream FlowerRandom(static_cast<int32>(Seed ^ 0x8DA6B343u));
        auto CreateCoverBatch = [&](UStaticMesh* Mesh, FName Tag, int32 StartCullDistance,
            int32 EndCullDistance)
        {
            const double SetupStarted = FPlatformTime::Seconds();
            auto* Batch = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
            Batch->SetupAttachment(GetRootComponent());
            Batch->SetMobility(EComponentMobility::Static);
            Batch->ComponentTags.Add(Tag);
            Batch->SetStaticMesh(Mesh);
            if (!ApplyCameraSafeFoliageMaterials(*Batch))
                bVisualBuildFailed = true;
            Batch->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
            Batch->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Batch->SetGenerateOverlapEvents(false);
            Batch->SetCanEverAffectNavigation(false);
            Batch->SetCullDistances(StartCullDistance, EndCullDistance);
            Batch->SetVisibility(!StageChunk);
            Batch->SetHiddenInGame(StageChunk != nullptr);
            Batch->SetCastShadow(false);
            Batch->bAutoRebuildTreeOnInstanceChanges = false;
            Batch->RegisterComponent();
            Chunk.Value->Cover.Components.Add(Batch);
            BatchSetupMilliseconds += (FPlatformTime::Seconds() - SetupStarted) * 1000;
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
        if (bVisualBuildFailed) return false;
        const double InstanceStarted = FPlatformTime::Seconds();
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
                    FVector(X, Y, CachedGroundHeight(X, Y)) - Rotation.RotateVector(Anchor), FVector::OneVector));
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
                    FVector(X, Y, CachedGroundHeight(X, Y)) - Rotation.RotateVector(Anchor), FVector::OneVector));
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
                    FVector(X, Y, CachedGroundHeight(X, Y)) - Rotation.RotateVector(Anchor * Scale), FVector(Scale)));
                ++FlowerCount;
                FlowerTriangleCount += 758;
            }
        }
        {
            TArray<FHomesteadRock> Rocks;
            std::int64_t KnobX = 0, KnobY = 0;
            const bool bKnob = Homestead::Generation::GraniteKnob(State.world, {Chunk.Key.X, Chunk.Key.Y}, KnobX, KnobY);
            const FVector2D Knob(static_cast<double>(KnobX), static_cast<double>(KnobY));
            GenerateRocks(State.world.seed, Chunk.Key, bKnob ? &Knob : nullptr, [&](float X, float Y, float Radius, uint8 RockSize)
            {
                if (!RockMeshes[0]) return false;
                const float ClearingDistance = FVector2D::Distance(FVector2D(X, Y), StartingClearing);
                const double StreamDistance = FMath::Abs(X - Homestead::StreamX(Y));
                return ClearingDistance >= (RockSize == 0 ? 450.0f : StartingClearingBlockingRadius) + Radius
                    && StreamDistance >= (RockSize == 0 ? 140.0 : 240.0) + Radius
                    && !IsDecorationReserved(CoverState, X, Y, Radius * (RockSize == 0 ? 0.6f : 0.85f), 0, true);
            }, Rocks);
            UHierarchicalInstancedStaticMeshComponent* RockBatches[RockKindCount] = {};
            for (const FHomesteadRock& Rock : Rocks)
            {
                const FRockKind& Kind = RockKinds[Rock.Kind];
                UStaticMesh* Mesh = RockMeshes[Rock.Kind];
                if (!Mesh) continue;
                UHierarchicalInstancedStaticMeshComponent*& Batch = RockBatches[Rock.Kind];
                if (!Batch)
                {
                    const double SetupStarted = FPlatformTime::Seconds();
                    Batch = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
                    Batch->SetupAttachment(GetRootComponent());
                    Batch->SetMobility(EComponentMobility::Static);
                    Batch->ComponentTags.Add(TEXT("WoodlandGranite"));
                    Batch->SetStaticMesh(Mesh);
                    if (Kind.Size > 0)
                    {
                        Batch->ComponentTags.Add(TEXT("HomesteadBlocking"));
                        Batch->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
                        Batch->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
                        // Knee-high boulders let the camera boom pass; erratics and domes push it in.
                        if (Kind.Size == 1) Batch->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
                        Batch->SetCanEverAffectNavigation(true);
                    }
                    else
                    {
                        Batch->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
                        Batch->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                        Batch->SetCanEverAffectNavigation(false);
                    }
                    Batch->SetGenerateOverlapEvents(false);
                    Batch->SetCullDistances(0, Kind.Size == 0 ? 4500 : Kind.Size == 1 ? 9000 : 0);
                    Batch->SetVisibility(!StageChunk);
                    Batch->SetHiddenInGame(StageChunk != nullptr);
                    Batch->SetCastShadow(true);
                    Batch->bAutoRebuildTreeOnInstanceChanges = false;
                    Batch->RegisterComponent();
                    if (StageChunk) Batch->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                    Chunk.Value->Cover.Components.Add(Batch);
                    BatchSetupMilliseconds += (FPlatformTime::Seconds() - SetupStarted) * 1000;
                }
                // Settle on the lowest ground under the footprint so the downhill side never floats.
                const float Reach = Kind.Radius * Rock.Scale * 0.55f;
                float Z = CachedGroundHeight(Rock.X, Rock.Y);
                for (int32 Sample = 0; Sample < 6; ++Sample)
                {
                    const float Angle = Sample * UE_TWO_PI / 6;
                    Z = FMath::Min(Z, CachedGroundHeight(Rock.X + FMath::Cos(Angle) * Reach, Rock.Y + FMath::Sin(Angle) * Reach));
                }
                Batch->AddInstance(FTransform(FRotator(Rock.Pitch, Rock.Yaw, Rock.Roll),
                    FVector(Rock.X, Rock.Y, Z), FVector(Rock.Scale)));
                ++RockCount;
                BigRockCount += Kind.Size >= 2 ? 1 : 0;
            }
            for (auto* Batch : RockBatches)
                if (Batch) Batch->BuildTreeIfOutdated(false, true);

            TArray<FHomesteadUnderbrush> Plants;
            GenerateUnderbrush(State.world.seed, Chunk.Key, Plants);
            TArray<FHomesteadUnderbrush>& Placed = PlacedUnderbrush.FindOrAdd(Chunk.Key);
            Placed.Reset();
            UHierarchicalInstancedStaticMeshComponent* UnderbrushBatches[UnderbrushSpeciesCount] = {};
            for (const FHomesteadUnderbrush& Plant : Plants)
            {
                const FUnderbrushSpecies& Species = UnderbrushSpecies[Plant.Species];
                UStaticMesh* Mesh = UnderbrushMeshes[Plant.Species];
                if (!Mesh) continue;
                if (Simulation.IsUnderbrushCleared({Chunk.Key.X, Chunk.Key.Y}, Plant.Index)) continue;
                const float Radius = Species.Radius * Plant.Scale;
                bool bOnRock = false;
                for (const FHomesteadRock& Rock : Rocks)
                {
                    const float Clear = RockKinds[Rock.Kind].Radius * Rock.Scale * (RockKinds[Rock.Kind].Size == 0 ? 0.5f : 0.9f)
                        + Radius * 0.35f;
                    if (FVector2D::DistSquared(FVector2D(Plant.X, Plant.Y), FVector2D(Rock.X, Rock.Y)) < Clear * Clear)
                    {
                        bOnRock = true;
                        break;
                    }
                }
                if (bOnRock) continue;
                const float ClearingDistance = FVector2D::Distance(FVector2D(Plant.X, Plant.Y), StartingClearing);
                const double StreamDistance = FMath::Abs(Plant.X - Homestead::StreamX(Plant.Y));
                const bool bLow = Plant.Species >= 6;
                if (ClearingDistance < (Species.bBlocking ? StartingClearingBlockingRadius : StartingClearingShrubRadius) + Radius
                    || StreamDistance < (bLow ? 150.0 : 230.0) + Radius * 0.5
                    || IsDecorationReserved(CoverState, Plant.X, Plant.Y, Radius * (bLow ? 0.5f : 0.7f), 0, bLow))
                    continue;
                UHierarchicalInstancedStaticMeshComponent*& Batch = UnderbrushBatches[Plant.Species];
                if (!Batch)
                {
                    const double SetupStarted = FPlatformTime::Seconds();
                    Batch = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
                    Batch->SetupAttachment(GetRootComponent());
                    Batch->SetMobility(EComponentMobility::Static);
                    Batch->ComponentTags.Add(TEXT("WoodlandUnderbrush"));
                    Batch->SetStaticMesh(Mesh);
                    if (Species.bBlocking)
                    {
                        Batch->ComponentTags.Add(TEXT("HomesteadBlocking"));
                        Batch->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
                        // The camera boom and interaction traces pass through; only her body is stopped.
                        Batch->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
                        Batch->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
                        Batch->SetCanEverAffectNavigation(true);
                    }
                    else
                    {
                        Batch->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
                        Batch->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                        Batch->SetCanEverAffectNavigation(false);
                    }
                    Batch->SetGenerateOverlapEvents(false);
                    // The gentle wind sway needn't redraw cached virtual shadow pages every frame;
                    // that made the bush shadows shimmer while she stood still.
                    Batch->ShadowCacheInvalidationBehavior = EShadowCacheInvalidationBehavior::Rigid;
                    Batch->SetCullDistances(bLow ? 2200 : 0, bLow ? 3800 : 9000);
                    Batch->SetVisibility(!StageChunk);
                    Batch->SetHiddenInGame(StageChunk != nullptr);
                    Batch->SetCastShadow(Species.bShadow);
                    Batch->bAutoRebuildTreeOnInstanceChanges = false;
                    Batch->RegisterComponent();
                    if (StageChunk) Batch->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                    Chunk.Value->Cover.Components.Add(Batch);
                    BatchSetupMilliseconds += (FPlatformTime::Seconds() - SetupStarted) * 1000;
                }
                const FRotator Rotation(0, Plant.Yaw, 0);
                // Pivots are at the base; sink a little so stems root into uneven ground.
                Batch->AddInstance(FTransform(Rotation,
                    FVector(Plant.X, Plant.Y, CachedGroundHeight(Plant.X, Plant.Y) - 4.0f * Plant.Scale),
                    FVector(Plant.Scale)));
                Placed.Add(Plant);
                ++UnderbrushCount;
                BlockingUnderbrushCount += Species.bBlocking ? 1 : 0;
            }
            for (auto* Batch : UnderbrushBatches)
                if (Batch) Batch->BuildTreeIfOutdated(false, true);
        }
        InstanceMilliseconds += (FPlatformTime::Seconds() - InstanceStarted) * 1000;
        const double TreeStarted = FPlatformTime::Seconds();
        for (auto* Batch : GrassBatches) Batch->BuildTreeIfOutdated(false, true);
        for (const auto& Batch : FernBatches) Batch.Value->BuildTreeIfOutdated(false, true);
        for (auto* Batch : FlowerBatches) Batch->BuildTreeIfOutdated(false, true);
        TreeBuildMilliseconds += (FPlatformTime::Seconds() - TreeStarted) * 1000;
        Chunk.Value->CoverSignature = Signature;
    }
    DecorationBuildMilliseconds = (FPlatformTime::Seconds() - Started) * 1000;
    LastCoverPrepareMilliseconds = DecorationBuildMilliseconds;
    if (ProfileChunkPublishing())
        CoverProfile = FString::Printf(
            TEXT("CHUNK_STAGE cover rebuilt_chunks=%d scan_ms=%.3f teardown_ms=%.3f batch_setup_ms=%.3f add_instances_ms=%.3f build_tree_ms=%.3f total_ms=%.3f\n"),
            RebuiltChunks, CoverScanMilliseconds, CoverTeardownMilliseconds,
            BatchSetupMilliseconds, InstanceMilliseconds, TreeBuildMilliseconds,
            DecorationBuildMilliseconds);
    UE_LOG(LogHomesteadWorld, Display, TEXT("Generated cover refresh: rebuilt_chunks=%d added_ferns=%d added_grass=%d added_flowers=%d bank_ferns=%d bank_grass=%d added_grass_triangles=%d added_flower_triangles=%d underbrush=%d blocking_underbrush=%d granite=%d big_granite=%d elapsed_ms=%.3f; CPU wall time, not GPU frame cost."),
        RebuiltChunks, FernCount, GrassCount, FlowerCount, BankFernCount, BankGrassCount,
        GrassTriangleCount, FlowerTriangleCount, UnderbrushCount, BlockingUnderbrushCount, RockCount, BigRockCount, DecorationBuildMilliseconds);
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
        const FString Signature = FString::Printf(TEXT("%d:%.3f:%.3f:%d"),
            static_cast<int>(Node.kind), Node.position.x, Node.position.y, Node.cleared);
        bStagingResourceBuild = true;
        auto& Base = StagedResourceVisuals.FindOrAdd(Node.id);
        BuildResource(Base, Node, false);
        Base.Signature = Signature;
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

bool AHomesteadWorld::ResolveGeneratedTreeVisual(const Homestead::ResourceNode& Node, UStaticMesh*& Mesh,
    FHomesteadOuterTreeInstance& Instance)
{
    Homestead::Generation::GeneratedEntity Entity;
    if (bFixedEstate && Node.kind == Homestead::ResourceKind::ForestTree)
    {
        // Estate trees are baked placements, not generated entities: pick a period-plausible
        // broadleaf or conifer and a stable yaw and size from the placement id. The MVP woodland's
        // trees (ids 560000+) take the MVP palette: 60% broadleaf, 35% fir, 5% jacaranda.
        const uint32 Hash = HashCombine(GetTypeHash(Node.id), 0x9E3779B9u);
        const bool bMvpWoodland = IsMvpWoodlandId(Node.id);
        const uint32 PaletteRoll = Hash % 100;
        Entity.paletteRole = bMvpWoodland
            ? (PaletteRoll < 60 ? Homestead::Generation::TreePaletteRole::BroadleafMature
                : PaletteRoll < 95 ? Homestead::Generation::TreePaletteRole::ConiferMature
                : Homestead::Generation::TreePaletteRole::WoodlandAccent)
            : Hash % 5 == 0 ? Homestead::Generation::TreePaletteRole::ConiferMature
            : Homestead::Generation::TreePaletteRole::BroadleafMature;
        Entity.variantIndex = 0;
        Entity.yawDegrees = static_cast<decltype(Entity.yawDegrees)>((Hash >> 8) % 360);
        // AssignTreePalette's MVP sizes: 0.90 up to 1.04 (broadleaf), 1.05 (fir) or 1.06 (jacaranda).
        const uint32 Spread = Entity.paletteRole == Homestead::Generation::TreePaletteRole::BroadleafMature ? 141
            : Entity.paletteRole == Homestead::Generation::TreePaletteRole::ConiferMature ? 151 : 161;
        Entity.scalePermille = static_cast<decltype(Entity.scalePermille)>(900 + (Hash >> 16) % (bMvpWoodland ? Spread : 260));
    }
    else if (Node.kind != Homestead::ResourceKind::ForestTree
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
    // Measured from each mesh's LOD0: how far the underside of the root flare rim rises above the
    // mesh's lowest vertex, and how far the flare reaches from the trunk axis. The fir's skirt
    // curls up to 30 cm and the jacaranda's surface roots lift up to 19 cm, so sinking by less leaves
    // the flare floating with a shadow under it.
    const bool bConifer = Entity.paletteRole == Homestead::Generation::TreePaletteRole::ConiferMature;
    const bool bAccent = Entity.paletteRole == Homestead::Generation::TreePaletteRole::WoodlandAccent;
    const float RimLift = bConifer ? 30.0f : bAccent ? 19.0f : 3.0f;
    const float Footprint = bConifer ? 76.0f : bAccent ? 240.0f : 41.0f;
    const float Radius = FMath::Max(Capsule.Radius, Footprint) * Scale;
    const float Embed = 4.0f + RimLift * Scale;
    float RootGround = CachedGroundHeight(Node.position.x, Node.position.y);
    for (const float Reach : {0.5f, 1.0f})
        for (const FVector2D Direction : {FVector2D(1,0), FVector2D(-1,0), FVector2D(0,1),
            FVector2D(0,-1), FVector2D(0.7071f,0.7071f), FVector2D(-0.7071f,0.7071f),
            FVector2D(0.7071f,-0.7071f), FVector2D(-0.7071f,-0.7071f)})
            RootGround = FMath::Min(RootGround, CachedGroundHeight(
                Node.position.x + Direction.X * Radius * Reach, Node.position.y + Direction.Y * Radius * Reach));
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
    double BatchSetupMilliseconds = 0;
    double InstanceMilliseconds = 0;
    double TeardownMilliseconds = 0;
    double RegisterMilliseconds = 0;
    double TreeBuildMilliseconds = 0;
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
    const double DesiredPrepared = FPlatformTime::Seconds();

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
            const double SetupStarted = FPlatformTime::Seconds();
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
            if (!ApplyCameraSafeFoliageMaterials(*Batch))
            {
                Batch->DestroyComponent();
                DiscardPrepared();
                return false;
            }
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
            BatchSetupMilliseconds += (FPlatformTime::Seconds() - SetupStarted) * 1000;
        }
        const double InstanceStarted = FPlatformTime::Seconds();
        Batch->AddInstance(Entry.Instance.Transform);
        InstanceMilliseconds += (FPlatformTime::Seconds() - InstanceStarted) * 1000;
        PreparedInstances.Add(Entry.Key, Entry.Instance);
    }

    const double TeardownStarted = FPlatformTime::Seconds();
    ClearOuterTreeBatches();
    TeardownMilliseconds = (FPlatformTime::Seconds() - TeardownStarted) * 1000;
    for (auto& Entry : PreparedBatches)
    {
        Entry.Value->bAutoRebuildTreeOnInstanceChanges = true;
        const double RegisterStarted = FPlatformTime::Seconds();
        Entry.Value->RegisterComponent();
        RegisterMilliseconds += (FPlatformTime::Seconds() - RegisterStarted) * 1000;
        const double TreeStarted = FPlatformTime::Seconds();
        Entry.Value->BuildTreeIfOutdated(false, true);
        TreeBuildMilliseconds += (FPlatformTime::Seconds() - TreeStarted) * 1000;
        OuterTreeBatches.Add(Entry.Key, Entry.Value);
    }
    OuterTreeInstances = MoveTemp(PreparedInstances);
    LastOuterTreePrepareMilliseconds = (FPlatformTime::Seconds() - Started) * 1000;
    if (ProfileChunkPublishing())
        OuterTreeProfile = FString::Printf(
            TEXT("CHUNK_STAGE outer_tree desired_ms=%.3f batch_setup_ms=%.3f add_instances_ms=%.3f teardown_ms=%.3f register_ms=%.3f build_tree_ms=%.3f total_ms=%.3f\n"),
            (DesiredPrepared - Started) * 1000, BatchSetupMilliseconds, InstanceMilliseconds,
            TeardownMilliseconds, RegisterMilliseconds, TreeBuildMilliseconds,
            LastOuterTreePrepareMilliseconds);
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
    double BatchSetupMilliseconds = 0;
    double InstanceMilliseconds = 0;
    double CollisionSetupMilliseconds = 0;
    double TeardownMilliseconds = 0;
    double RegisterMilliseconds = 0;
    double TreeBuildMilliseconds = 0;
    double CollisionRegisterMilliseconds = 0;
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
    const double DesiredPrepared = FPlatformTime::Seconds();

    TMap<FString, UHierarchicalInstancedStaticMeshComponent*> PreparedBatches;
    TMap<FString, UCapsuleComponent*> PreparedCollisions;
    TMap<FString, FHomesteadActiveTreeInstance> PreparedInstances;
    TSet<FString> ReusedCollisionKeys;
    auto DiscardPrepared = [&PreparedBatches, &PreparedCollisions, &ReusedCollisionKeys]()
    {
        for (auto& Entry : PreparedBatches)
            if (IsValid(Entry.Value))
                Entry.Value->DestroyComponent();
        for (auto& Entry : PreparedCollisions)
            if (!ReusedCollisionKeys.Contains(Entry.Key) && IsValid(Entry.Value))
                Entry.Value->DestroyComponent();
    };
    for (const FBuildEntry& Entry : Desired)
    {
        UHierarchicalInstancedStaticMeshComponent* Batch = nullptr;
        if (auto** Existing = PreparedBatches.Find(Entry.Instance.Visual.MeshPath))
            Batch = *Existing;
        else
        {
            const double SetupStarted = FPlatformTime::Seconds();
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
            if (!ApplyCameraSafeFoliageMaterials(*Batch))
            {
                Batch->DestroyComponent();
                DiscardPrepared();
                return false;
            }
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
            BatchSetupMilliseconds += (FPlatformTime::Seconds() - SetupStarted) * 1000;
        }
        const double InstanceStarted = FPlatformTime::Seconds();
        Batch->AddInstance(Entry.Instance.Visual.Transform);
        InstanceMilliseconds += (FPlatformTime::Seconds() - InstanceStarted) * 1000;

        const auto* PreviousInstance = ActiveTreeInstances.Find(Entry.Key);
        const auto* PreviousCollision = ActiveTreeCollisions.Find(Entry.Key);
        if (PreviousInstance && PreviousCollision && IsValid(PreviousCollision->Get())
            && PreviousInstance->ResourceId == Entry.Instance.ResourceId
            && PreviousInstance->Visual.MeshPath == Entry.Instance.Visual.MeshPath
            && PreviousInstance->Visual.Transform.Equals(Entry.Instance.Visual.Transform)
            && PreviousInstance->CollisionTransform.Equals(Entry.Instance.CollisionTransform)
            && FMath::IsNearlyEqual(PreviousInstance->CapsuleRadius, Entry.Instance.CapsuleRadius)
            && FMath::IsNearlyEqual(PreviousInstance->CapsuleHalfHeight, Entry.Instance.CapsuleHalfHeight))
        {
            PreparedCollisions.Add(Entry.Key, PreviousCollision->Get());
            PreparedInstances.Add(Entry.Key, Entry.Instance);
            ReusedCollisionKeys.Add(Entry.Key);
            continue;
        }

        const double CollisionStarted = FPlatformTime::Seconds();
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
        CollisionSetupMilliseconds += (FPlatformTime::Seconds() - CollisionStarted) * 1000;
    }

    const double TeardownStarted = FPlatformTime::Seconds();
    for (auto& Entry : ActiveTreeBatches)
        if (IsValid(Entry.Value.Get()))
            Entry.Value->DestroyComponent();
    ActiveTreeBatches.Reset();
    for (auto& Entry : ActiveTreeCollisions)
        if (!ReusedCollisionKeys.Contains(Entry.Key) && IsValid(Entry.Value.Get()))
            Entry.Value->DestroyComponent();
    ActiveTreeCollisions.Reset();
    ActiveTreeInstances.Reset();
    TeardownMilliseconds = (FPlatformTime::Seconds() - TeardownStarted) * 1000;
    for (auto& Entry : PreparedBatches)
    {
        Entry.Value->bAutoRebuildTreeOnInstanceChanges = true;
        const double RegisterStarted = FPlatformTime::Seconds();
        Entry.Value->RegisterComponent();
        RegisterMilliseconds += (FPlatformTime::Seconds() - RegisterStarted) * 1000;
        const double TreeStarted = FPlatformTime::Seconds();
        Entry.Value->BuildTreeIfOutdated(false, true);
        TreeBuildMilliseconds += (FPlatformTime::Seconds() - TreeStarted) * 1000;
        ActiveTreeBatches.Add(Entry.Key, Entry.Value);
    }
    for (auto& Entry : PreparedCollisions)
    {
        if (ReusedCollisionKeys.Contains(Entry.Key))
        {
            ActiveTreeCollisions.Add(Entry.Key, Entry.Value);
            continue;
        }
        const double RegisterStarted = FPlatformTime::Seconds();
        Entry.Value->RegisterComponent();
        CollisionRegisterMilliseconds += (FPlatformTime::Seconds() - RegisterStarted) * 1000;
        ActiveTreeCollisions.Add(Entry.Key, Entry.Value);
    }
    ActiveTreeInstances = MoveTemp(PreparedInstances);
    LastActiveTreePrepareMilliseconds = (FPlatformTime::Seconds() - Started) * 1000;
    if (ProfileChunkPublishing())
        ActiveTreeProfile = FString::Printf(
            TEXT("CHUNK_STAGE active_tree desired_ms=%.3f batch_setup_ms=%.3f add_instances_ms=%.3f collision_setup_ms=%.3f teardown_ms=%.3f register_ms=%.3f build_tree_ms=%.3f collision_register_ms=%.3f total_ms=%.3f\n"),
            (DesiredPrepared - Started) * 1000, BatchSetupMilliseconds, InstanceMilliseconds,
            CollisionSetupMilliseconds, TeardownMilliseconds, RegisterMilliseconds,
            TreeBuildMilliseconds, CollisionRegisterMilliseconds, LastActiveTreePrepareMilliseconds);
    UE_LOG(LogHomesteadWorld, Display,
        TEXT("Generated active mature trees rebuilt: batches=%d instances=%d collisions=%d."),
        ActiveTreeBatches.Num(), ActiveTreeInstances.Num(), ActiveTreeCollisions.Num());
    return true;
}

bool AHomesteadWorld::TreeChopTarget(int32 ResourceId, FVector2D& Centre, float& Radius) const
{
    // Trunk centroid and radius 80-100 cm up each mesh's trunk section (measured from LOD0): the
    // collision capsules wrap the whole trunk and crown, and the jacaranda's forked trunk stands
    // well off its origin.
    struct FChop { const TCHAR* Mesh; FVector2D Offset; float Radius; };
    static const FChop Chops[] = {
        {TEXT("SM_TreeSmall02_Woodland"), FVector2D(9.1, 4.0), 9.0f},
        {TEXT("SM_MatureFir"), FVector2D(3.6, -0.2), 13.0f},
        {TEXT("SM_Jacaranda"), FVector2D(8.5, -37.3), 55.0f},
    };
    for (const auto& Entry : ActiveTreeInstances)
    {
        if (Entry.Value.ResourceId != ResourceId) continue;
        const FTransform& Transform = Entry.Value.Visual.Transform;
        for (const FChop& Chop : Chops)
            if (Entry.Value.Visual.MeshPath.Contains(Chop.Mesh))
            {
                const FVector At = GetActorTransform().TransformPosition(Transform.TransformPosition(FVector(Chop.Offset, 90.0)));
                Centre = FVector2D(At.X, At.Y);
                Radius = Chop.Radius * Transform.GetScale3D().X;
                return true;
            }
        const FVector At = GetActorTransform().TransformPosition(Entry.Value.CollisionTransform.GetLocation());
        Centre = FVector2D(At.X, At.Y);
        Radius = Entry.Value.CapsuleRadius;
        return true;
    }
    return false;
}

void AHomesteadWorld::FinishFallingTree()
{
    for (USceneComponent* Part : FallingParts)
        if (IsValid(Part)) Part->DestroyComponent();
    FallingParts.Reset();
    FallingRest.Reset();
    bTreeFalling = bTreeLanded = bLandingPending = false;
}

bool AHomesteadWorld::BeginFelling(int32 ResourceId)
{
    FinishFallingTree();
    for (const auto& Entry : ActiveTreeInstances)
    {
        if (Entry.Value.ResourceId != ResourceId) continue;
        auto* Mesh = LoadObject<UStaticMesh>(nullptr, *Entry.Value.Visual.MeshPath);
        if (!Mesh) return false;
        auto* Part = NewObject<UStaticMeshComponent>(this);
        Part->SetupAttachment(GetRootComponent());
        Part->SetMobility(EComponentMobility::Movable);
        Part->SetStaticMesh(Mesh);
        Part->bOverrideMinLOD = true;
        Part->MinLOD = ActiveMatureTreeMinLOD;
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetGenerateOverlapEvents(false);
        Part->SetCanEverAffectNavigation(false);
        if (!ApplyCameraSafeFoliageMaterials(*Part))
        {
            Part->DestroyComponent();
            return false;
        }
        Part->SetRelativeTransform(Entry.Value.Visual.Transform);
        Part->RegisterComponent();
        FallingParts.Add(Part);
        break;
    }
    // Saplings are ordinary resource visuals: take them over so the rebuild leaves them be.
    if (FallingParts.IsEmpty())
        if (FHomesteadWorldVisual* Visual = ResourceVisuals.Find(ResourceId))
        {
            for (USceneComponent* Part : Visual->Components)
                if (IsValid(Part))
                {
                    Part->SetMobility(EComponentMobility::Movable);
                    FallingParts.Add(Part);
                }
            Visual->Components.Reset();
            ResourceVisuals.Remove(ResourceId);
        }
    if (FallingParts.IsEmpty()) return false;
    FBox Bounds(ForceInit);
    for (USceneComponent* Part : FallingParts)
    {
        FallingRest.Add(Part->GetComponentTransform());
        Bounds += Part->Bounds.GetBox();
    }
    FallPivot = FallingParts[0]->GetComponentLocation();
    FallHeight = FMath::Max(100.0f, static_cast<float>(Bounds.Max.Z - FallPivot.Z));
    FallAngle = FallRate = FallLying = 0;
    FallBounces = 0;
    return true;
}

void AHomesteadWorld::DropFelledTree(FVector2D AwayFrom)
{
    if (FallingParts.IsEmpty() || bTreeFalling) return;
    FVector2D Away = FVector2D(FallPivot.X, FallPivot.Y) - AwayFrom;
    if (!Away.Normalize()) Away = FVector2D(1, 0);
    FallAxis = FVector::CrossProduct(FVector::UpVector, FVector(Away, 0)).GetSafeNormal();
    // A notched trunk starts to lean slowly, then gravity takes it.
    FallAngle = 0.03f;
    FallRate = 0.05f;
    bTreeFalling = true;
}

bool AHomesteadWorld::TakeFelledTreeLanding(FVector& Where)
{
    if (!bLandingPending) return false;
    bLandingPending = false;
    Where = FallPivot;
    return true;
}

void AHomesteadWorld::UpdateFallingTree(float DeltaSeconds)
{
    if (FallingParts.IsEmpty() || !bTreeFalling) return;
    const float Dt = FMath::Min(DeltaSeconds, 1.0f / 20.0f);
    // A rod pivoting on its base: angular acceleration 3g sin(angle) / 2L.
    constexpr float Landed = 1.47f;
    float Sink = 0;
    if (!bTreeLanded)
    {
        FallRate += 1.5f * 980.0f / FallHeight * FMath::Sin(FallAngle) * Dt;
        FallAngle += FallRate * Dt;
        if (FallAngle >= Landed)
        {
            FallAngle = Landed;
            if (FallBounces == 0 && FallRate > 0.4f)
            {
                bLandingPending = true;
                FallRate = -FallRate * 0.12f;
                ++FallBounces;
            }
            else
            {
                FallRate = 0;
                bTreeLanded = true;
            }
        }
    }
    else
    {
        // Lies a few seconds, then sinks into the forest floor.
        FallLying += Dt;
        Sink = 160.0f * FMath::Square(FMath::Clamp((FallLying - 4.0f) / 1.8f, 0.0f, 1.0f));
        if (FallLying > 6.0f)
        {
            FinishFallingTree();
            return;
        }
    }
    const FQuat Turn(FallAxis, FallAngle);
    for (int32 Index = 0; Index < FallingParts.Num(); ++Index)
    {
        if (!IsValid(FallingParts[Index])) continue;
        const FTransform& Rest = FallingRest[Index];
        const FVector Location = FallPivot + Turn.RotateVector(Rest.GetLocation() - FallPivot) - FVector(0, 0, Sink);
        FallingParts[Index]->SetWorldLocationAndRotation(Location, Turn * Rest.GetRotation());
    }
}

void AHomesteadWorld::HideHeldProducePart(int32 Index)
{
    if (HeldProduceId == INDEX_NONE) return;
    if (auto* Produce = ResourceProduceVisuals.Find(HeldProduceId))
        if (Produce->Components.IsValidIndex(Index) && IsValid(Produce->Components[Index]))
            Produce->Components[Index]->SetVisibility(false);
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
    auto Authored = [&](UStaticMesh* Mesh, FVector2D Offset, float Yaw, bool bProduce, float Scale = 1.0f, float Lift = 0.0f)
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
        const FVector Ground = AtGround(Base.X + Offset.X, Base.Y + Offset.Y) + FVector(0, 0, Lift);
        auto* Component = NewObject<UStaticMeshComponent>(this);
        Component->SetupAttachment(GetRootComponent());
        Component->SetMobility(EComponentMobility::Movable);
        Component->SetStaticMesh(Mesh);
        if (!ApplyCameraSafeFoliageMaterials(*Component))
        {
            Component->DestroyComponent();
            bVisualBuildFailed = true;
            return;
        }
        Component->SetRelativeTransform(FTransform(Rotation, Ground - Rotation.RotateVector(Anchor * Scale), FVector(Scale)));
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetCollisionResponseToAllChannels(ECR_Ignore);
        if (IsMvpWoodlandId(Node.id) && !bProduce
            && (Node.kind == Homestead::ResourceKind::BrambleThin || Node.kind == Homestead::ResourceKind::BrambleThicket))
        {
            // As in the MVP, a bramble stops her until she cuts it; the camera boom and traces pass.
            Component->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
            Component->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
            Component->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
        }
        if (Node.kind == Homestead::ResourceKind::Stones && RockMaterial)
            Component->SetMaterial(0, RockMaterial);
        Component->SetGenerateOverlapEvents(false);
        Component->SetCanEverAffectNavigation(false);
        Component->ComponentTags.Append({TEXT("AuthoredResource"), bProduce ? TEXT("ResourceProduce") : TEXT("ResourceBase")});
        if (bStagingResourceBuild)
        {
            Component->SetVisibility(false);
            Component->SetHiddenInGame(true);
        }
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
            // Scale matches the heroine's carried stick props (AHomesteadCharacter::CarriedStickScale),
            // so a stick keeps its size when she picks it up; component 1 (_b) and 2 (_c) are the
            // two she lifts.
            for (int I = 0; I < 3; ++I)
            {
                const TCHAR* Names[] = {TEXT("SM_DryBranchesMedium01_a"), TEXT("SM_DryBranchesMedium01_b"), TEXT("SM_DryBranchesMedium01_c")};
                Authored(LoadResource(Names[I]), FVector2D(I * 9 - 9, I * 7 - 7), I * 35 + 20, true, 0.6f);
            }
        }
        break;
    case Homestead::ResourceKind::Stones:
        if (bProduceOnly)
        {
            for (int I = 0; I < 3; ++I)
            {
                UStaticMesh* HandStone = AHomesteadCharacter::LoadHandStone(I);
                UStaticMesh* Rock = HandStone ? HandStone : ImportedRock.Get();
                if (Rock && Rock->GetBoundingBox().GetSize().GetMax() > 0)
                {
                    const int32 First = Visual.Components.Num();
                    Authored(Rock, FVector2D(I * 17 - 17, I % 2 * 14), I * 79, true,
                        AHomesteadCharacter::StonePileSize(I, HandStone != nullptr) / Rock->GetBoundingBox().GetSize().GetMax());
                    // Authored hand stones keep their baked granite material.
                    if (HandStone && Visual.Components.Num() > First)
                        if (auto* Placed = Cast<UStaticMeshComponent>(Visual.Components.Last()))
                            for (int32 Slot = 0; Slot < HandStone->GetStaticMaterials().Num(); ++Slot)
                                Placed->SetMaterial(Slot, HandStone->GetMaterial(Slot));
                }
                else
                {
                    bVisualBuildFailed = true;
                    UE_LOG(LogHomesteadWorld, Error, TEXT("Stone resource %d is missing admitted rock geometry."), Node.id);
                }
            }
        }
        break;
    case Homestead::ResourceKind::BerryBush:
        if (Node.id >= Homestead::EstatePlacementIdBase && Node.id < Homestead::TransientResourceIdBase)
        {
            // Estate: a fruiting blackberry bramble hung with ripe clusters that go when she picks them.
            auto Load = [&](const TCHAR* Folder, const TCHAR* Name) -> UStaticMesh*
            {
                auto* Mesh = LoadObject<UStaticMesh>(nullptr,
                    *FString::Printf(TEXT("/Game/SurvivalGame/Environment/Props/%s/%s.%s"), Folder, Name, Name));
                if (!Mesh)
                {
                    bVisualBuildFailed = true;
                    UE_LOG(LogHomesteadWorld, Error, TEXT("Berry bramble %d is missing authored mesh %s"), Node.id, Name);
                }
                return Mesh;
            };
            const float Yaw = static_cast<float>(Variation % 360);
            Authored(Load(TEXT("BlackberryBramble"), TEXT("SM_BlackberryBramble")), FVector2D::ZeroVector, Yaw, false,
                BerryBrambleScale * Random.FRandRange(0.95f, 1.08f));
            UStaticMesh* Fruit = Load(TEXT("BerryBushProduce"), TEXT("SM_BerryBushProduce"));
            for (int I = 0; I < BerryBrambleFruit && Fruit; ++I)
            {
                const float Angle = FMath::DegreesToRadians(Yaw + I * 137.5f);
                Authored(Fruit, FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * BerryFruitOffset, Yaw + I * 97.0f, true,
                    BerryFruitScale, Fruit->GetBoundingBox().Min.Z * BerryFruitScale + BerryFruitLift);
            }
            break;
        }
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
            // Two root crowns: the heroine's pouch gather lifts one per pickup.
            for (int I = 0; I < 2; ++I)
                Part(Sphere, FVector(I * 11 - 5, I * 4, 4), FVector(11, 11, 8), FLinearColor(0.65f, 0.43f, 0.19f),
                    FRotator::ZeroRotator, true);
        }
        break;
    case Homestead::ResourceKind::Flowers:
        // Wild marjoram, knee-high and rosy-purple in flower, so a herb patch reads from across the
        // pasture; she cuts the flowering stems and leaves the grass stubble.
        if (UStaticMesh* Marjoram = LoadObject<UStaticMesh>(nullptr,
                TEXT("/Game/SurvivalGame/Environment/Props/WildMarjoram/SM_WildMarjoram.SM_WildMarjoram"), nullptr,
                LOAD_NoWarn | LOAD_Quiet))
        {
            const float Yaw = static_cast<float>(Variation % 360);
            Authored(LoadResource(TEXT("SM_GrassMedium01_tiny_a"), true), FVector2D(6, -4), Yaw, false);
            Authored(Marjoram, FVector2D::ZeroVector, Yaw, true, Random.FRandRange(0.9f, 1.1f));
            break;
        }
        for (int I = 0; I < 2; ++I)
        {
            const FVector2D Offset(I * 18 - 9, I * 6 - 3);
            Authored(LoadResource(TEXT("SM_GrassMedium01_tiny_a"), true), Offset, I * 137, false);
            Authored(LoadResource(I ? TEXT("SM_FlowerEmpodium_b") : TEXT("SM_FlowerEmpodium_a")),
                Offset, I * 137, true);
        }
        break;
    case Homestead::ResourceKind::Reeds:
        {
            const TCHAR* Path = bProduceOnly
                ? TEXT("/Game/SurvivalGame/Environment/Reeds/SM_ReedClump.SM_ReedClump")
                : TEXT("/Game/SurvivalGame/Environment/Reeds/SM_ReedStubble.SM_ReedStubble");
            UStaticMesh* Reeds = LoadObject<UStaticMesh>(nullptr, Path);
            if (!Reeds)
            {
                bVisualBuildFailed = true;
                UE_LOG(LogHomesteadWorld, Error, TEXT("Reed resource %d is missing its original authored mesh: %s"),
                    Node.id, Path);
                break;
            }
            Authored(Reeds, FVector2D::ZeroVector, static_cast<float>(Variation % 360), bProduceOnly);
        }
        break;
    case Homestead::ResourceKind::DeerRemains:
        {
            // The hide-covered remains while there is fur to take, bare bones after; UpdateVisuals
            // hides the bones while the remains are showing (both share one pivot and footprint).
            const TCHAR* Path = bProduceOnly
                ? TEXT("/Game/SurvivalGame/Environment/Props/DeerRemains/SM_DeerRemains.SM_DeerRemains")
                : TEXT("/Game/SurvivalGame/Environment/Props/DeerRemains/SM_DeerBones.SM_DeerBones");
            UStaticMesh* Deer = LoadObject<UStaticMesh>(nullptr, Path);
            if (!Deer)
            {
                bVisualBuildFailed = true;
                UE_LOG(LogHomesteadWorld, Error, TEXT("Deer remains %d are missing their authored mesh: %s"), Node.id, Path);
                break;
            }
            Authored(Deer, FVector2D::ZeroVector, static_cast<float>(Variation % 360), bProduceOnly);
        }
        break;
    case Homestead::ResourceKind::Sapling:
    {
        // Estate saplings are placed, not generated.
        if (Node.id >= Homestead::EstatePlacementIdBase && Node.id < Homestead::TransientResourceIdBase)
        {
            if (!bProduceOnly)
                BuildOvergrowth(Node, Variation, [&](UStaticMesh* Mesh, FVector2D Offset, float Yaw, bool bProduce, float Scale)
                    { Authored(Mesh, Offset, Yaw, bProduce, Scale); });
            break;
        }
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
        // Estate overgrowth and flowers (add-overgrown-estate-clearing).
        BuildOvergrowth(Node, Variation, [&](UStaticMesh* Mesh, FVector2D Offset, float Yaw, bool bProduce, float Scale)
            { Authored(Mesh, Offset, Yaw, bProduce, Scale); });
        break;
    }
}

void AHomesteadWorld::BuildOvergrowth(const Homestead::ResourceNode& Node, uint32 Variation,
    const TFunctionRef<void(UStaticMesh*, FVector2D, float, bool, float)>& Place)
{
    auto Load = [&](const TCHAR* Folder, const TCHAR* Name) -> UStaticMesh*
    {
        const FString Path = FString::Printf(TEXT("/Game/SurvivalGame/Environment/Props/%s/%s.%s"), Folder, Name, Name);
        auto* Mesh = LoadObject<UStaticMesh>(nullptr, *Path);
        if (!Mesh)
        {
            bVisualBuildFailed = true;
            UE_LOG(LogHomesteadWorld, Error, TEXT("Overgrowth %d is missing authored mesh %s"), Node.id, *Path);
        }
        return Mesh;
    };
    FRandomStream Random(static_cast<int32>(Variation * 2654435761u));
    const float Yaw = static_cast<float>(Variation % 360);
    // Overgrowth has no separate produce: the whole clump goes when she clears it.
    auto Whole = [&](UStaticMesh* Mesh, FVector2D Offset, float Turn, float Scale) { Place(Mesh, Offset, Yaw + Turn, false, Scale); };
    switch (Node.kind)
    {
    case Homestead::ResourceKind::BrambleThin:
        if (IsMvpWoodlandId(Node.id))
            Whole(Load(TEXT("BlackberryBramble"), TEXT("SM_BlackberryBramble")), FVector2D::ZeroVector, 0, Random.FRandRange(0.85f, 1.2f));
        else
            Whole(Load(TEXT("BrambleOvergrowth"), TEXT("SM_BrambleThin")), FVector2D::ZeroVector, 0, Random.FRandRange(0.9f, 1.1f));
        break;
    case Homestead::ResourceKind::BrambleThicket:
        if (IsMvpWoodlandId(Node.id))
            Whole(Load(TEXT("BlackberryBramble"), TEXT("SM_BlackberryBrambleLarge")), FVector2D::ZeroVector, 0, Random.FRandRange(0.9f, 1.15f));
        else
            Whole(Load(TEXT("BrambleOvergrowth"), TEXT("SM_BrambleThicket")), FVector2D::ZeroVector, 0, Random.FRandRange(0.95f, 1.1f));
        break;
    case Homestead::ResourceKind::BrambleBank:
        Whole(Load(TEXT("BrambleOvergrowth"), TEXT("SM_BrambleBank")), FVector2D::ZeroVector, 0, Random.FRandRange(1.0f, 1.12f));
        break;
    case Homestead::ResourceKind::TallGrass:
        for (int32 I = 0; I < 3; ++I)
            Whole(Load(TEXT("GrassYarrowTuft"), TEXT("SM_GrassYarrowTuft")),
                FVector2D(FMath::Cos(I * 2.1f) * 16, FMath::Sin(I * 2.1f) * 16), I * 97.0f, Random.FRandRange(1.05f, 1.3f));
        break;
    case Homestead::ResourceKind::Weeds:
        Whole(Load(TEXT("GrassYarrowTuft"), TEXT("SM_GrassYarrowTuft")), FVector2D::ZeroVector, 0, Random.FRandRange(0.7f, 0.85f));
        Whole(Load(TEXT("Thimbleberry"), TEXT("SM_Thimbleberry")), FVector2D(10, -6), 140, 0.35f);
        break;
    case Homestead::ResourceKind::Sapling:
        Whole(Load(TEXT("Hazel"), TEXT("SM_Hazel")), FVector2D::ZeroVector, 0, Random.FRandRange(0.45f, 0.6f));
        break;
    case Homestead::ResourceKind::Rubble:
        Whole(Load(TEXT("GraniteRubble"), TEXT("SM_GraniteRubble")), FVector2D::ZeroVector, 0, Random.FRandRange(0.8f, 1.0f));
        break;
    case Homestead::ResourceKind::SmallRock:
        Whole(Load(TEXT("GraniteSpalls"), TEXT("SM_GraniteSpalls")), FVector2D::ZeroVector, 0, Random.FRandRange(0.8f, 1.0f));
        break;
    case Homestead::ResourceKind::Boulder:
        Whole(Load(TEXT("GraniteBoulderLoaf"), TEXT("SM_GraniteBoulderLoaf")), FVector2D::ZeroVector, 0, Random.FRandRange(0.9f, 1.05f));
        break;
    case Homestead::ResourceKind::SalvagePile:
        // Stand-in until add-ruined-manor-and-arrival dresses its piles: fallen masonry.
        Whole(Load(TEXT("GraniteCobbles"), TEXT("SM_GraniteCobbles")), FVector2D::ZeroVector, 0, 0.7f);
        break;
    case Homestead::ResourceKind::StumpSmall:
        Whole(Load(TEXT("EstateTimber"), TEXT("SM_StumpSmall")), FVector2D::ZeroVector, 0, Random.FRandRange(0.9f, 1.15f));
        break;
    case Homestead::ResourceKind::StumpLarge:
        Whole(Load(TEXT("EstateTimber"), TEXT("SM_StumpLarge")), FVector2D::ZeroVector, 0, Random.FRandRange(0.9f, 1.1f));
        break;
    case Homestead::ResourceKind::StumpAncient:
        Whole(Load(TEXT("EstateTimber"), TEXT("SM_StumpAncient")), FVector2D::ZeroVector, 0, Random.FRandRange(0.95f, 1.05f));
        break;
    case Homestead::ResourceKind::FallenBranch:
        Whole(Load(TEXT("EstateTimber"), TEXT("SM_FallenBough")), FVector2D::ZeroVector, 0, Random.FRandRange(0.85f, 1.1f));
        break;
    case Homestead::ResourceKind::FallenLog:
        Whole(Load(TEXT("EstateTimber"), TEXT("SM_FallenLog")), FVector2D::ZeroVector, 0, Random.FRandRange(0.9f, 1.1f));
        break;
    case Homestead::ResourceKind::GiantLog:
        Whole(Load(TEXT("EstateTimber"), TEXT("SM_GiantLog")), FVector2D::ZeroVector, 0, Random.FRandRange(0.95f, 1.05f));
        break;
    // The manor clear-out (add-coral-island-clearout).
    case Homestead::ResourceKind::Nettles:
        Whole(Load(TEXT("Nettle"), TEXT("SM_NettlePatch")), FVector2D::ZeroVector, 0, Random.FRandRange(0.85f, 1.15f));
        break;
    case Homestead::ResourceKind::StumpMedium:
        Whole(Load(TEXT("EstateTimber"), TEXT("SM_StumpLarge")), FVector2D::ZeroVector, 0, Random.FRandRange(0.58f, 0.7f));
        break;
    case Homestead::ResourceKind::BrokenCrate:
    case Homestead::ResourceKind::BrokenBarrel:
    case Homestead::ResourceKind::RubbishHeap:
    {
        // The Farm Agent's estate debris (EstateDebris, EstateRubbish); the store goods stand in until it lands.
        // The first clear-out rows (570000-570007) are its eight freed spots and keep the full
        // midden; elsewhere a heap is a small midden or a rusty scrap pile.
        auto Quiet = [](const TCHAR* Name, const TCHAR* Folder = TEXT("EstateDebris")) -> UStaticMesh*
        {
            return LoadObject<UStaticMesh>(nullptr,
                *FString::Printf(TEXT("/Game/SurvivalGame/Environment/Props/%s/%s.%s"), Folder, Name, Name), nullptr,
                LOAD_NoWarn | LOAD_Quiet);
        };
        const bool Crate = Node.kind == Homestead::ResourceKind::BrokenCrate;
        const bool Barrel = Node.kind == Homestead::ResourceKind::BrokenBarrel;
        UStaticMesh* Debris = nullptr;
        float Scale = Random.FRandRange(0.8f, 0.95f);
        if (Crate) Debris = Quiet(TEXT("SM_BrokenCrate"));
        else if (Barrel) Debris = Quiet(TEXT("SM_BrokenBarrel"));
        else if (Node.id >= 570000 && Node.id < 570008) Debris = Quiet(TEXT("SM_RubbishHeap"));
        else
        {
            Debris = Quiet(Variation % 2 ? TEXT("SM_ScrapHeap") : TEXT("SM_RubbishHeapSmall"), TEXT("EstateRubbish"));
            if (!Debris && (Debris = Quiet(TEXT("SM_RubbishHeap")))) Scale = Random.FRandRange(0.45f, 0.55f);
        }
        if (Debris)
            Whole(Debris, FVector2D::ZeroVector, 0, Scale);
        else if (Crate)
            Whole(Load(TEXT("StoreCrate"), TEXT("SM_Store_Crate")), FVector2D::ZeroVector, 0, 0.9f);
        else if (Barrel)
            Whole(Load(TEXT("StoreBarrel"), TEXT("SM_Store_Barrel")), FVector2D::ZeroVector, 0, 0.9f);
        else
        {
            // A midden stand-in: broken stone and a rotten board.
            Whole(Load(TEXT("GraniteCobbles"), TEXT("SM_GraniteCobbles")), FVector2D::ZeroVector, 0, 0.8f);
            Whole(Load(TEXT("EstateTimber"), TEXT("SM_FallenBough")), FVector2D(20, 10), 70, 0.6f);
        }
        break;
    }
    case Homestead::ResourceKind::RottenPlanks:
        if (auto* Planks = LoadObject<UStaticMesh>(nullptr,
            TEXT("/Game/SurvivalGame/Environment/Props/EstateRubbish/SM_RottenPlanks.SM_RottenPlanks"), nullptr, LOAD_NoWarn | LOAD_Quiet))
            Whole(Planks, FVector2D::ZeroVector, 0, Random.FRandRange(0.9f, 1.05f));
        else
            // Until the Farm Agent's plank pile lands: the ruin's fallen roof timbers at plank scale.
            Whole(Load(TEXT("RuinFallenTimbers"), TEXT("SM_RuinFallenTimbers")), FVector2D::ZeroVector, 0, Random.FRandRange(0.34f, 0.4f));
        break;
    default:
        break;
    }
    if (Node.kind == Homestead::ResourceKind::WildGarlic || Node.kind == Homestead::ResourceKind::Bluebells
        || Node.kind == Homestead::ResourceKind::Primroses || Node.kind == Homestead::ResourceKind::WildDaffodils)
    {
        // Authored spring flowers (Scripts/Blender/Recipes wild_garlic.py, bluebell.py, primrose.py and wild_daffodil.py): the
        // whole flowering clump is what she picks, so it's the produce.
        const TCHAR* Path = Node.kind == Homestead::ResourceKind::WildGarlic
            ? TEXT("/Game/SurvivalGame/Environment/Props/WildGarlic/SM_WildGarlic.SM_WildGarlic")
            : Node.kind == Homestead::ResourceKind::Bluebells
            ? TEXT("/Game/SurvivalGame/Environment/Props/Bluebell/SM_BluebellClump.SM_BluebellClump")
            : Node.kind == Homestead::ResourceKind::Primroses
            ? TEXT("/Game/SurvivalGame/Environment/Props/Primrose/SM_PrimroseClump.SM_PrimroseClump")
            : TEXT("/Game/SurvivalGame/Environment/Props/WildDaffodil/SM_WildDaffodilClump.SM_WildDaffodilClump");
        if (auto* Clump = LoadObject<UStaticMesh>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet))
        {
            // Primroses hug the ground, so their clumps are drawn a little large to read above the pasture.
            const bool Primrose = Node.kind == Homestead::ResourceKind::Primroses;
            Place(Clump, FVector2D::ZeroVector, Yaw, true, Random.FRandRange(0.9f, 1.15f) * (Primrose ? 1.3f : 1.0f));
            return;
        }
    }
    if (Node.kind == Homestead::ResourceKind::Primroses || Node.kind == Homestead::ResourceKind::Bluebells
        || Node.kind == Homestead::ResourceKind::WildDaffodils || Node.kind == Homestead::ResourceKind::WildGarlic)
    {
        // Stand-in: the meadow-herb flowers in a grass tuft, until each spring flower is authored.
        auto* Flower = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Trials/WoodlandResources_20260921_01/Meshes/SM_FlowerEmpodium_a.SM_FlowerEmpodium_a"));
        for (int32 I = 0; I < 3 && Flower; ++I)
            Place(Flower, FVector2D(FMath::Cos(I * 2.1f) * 12, FMath::Sin(I * 2.1f) * 12), Yaw + I * 120.0f, true, 1.0f);
        Whole(Load(TEXT("GrassYarrowTuft"), TEXT("SM_GrassYarrowTuft")), FVector2D::ZeroVector, 0, 0.6f);
    }
}

UStaticMesh* AHomesteadWorld::ManorMesh(const TCHAR* Name)
{
    const FName Key(Name);
    if (const TObjectPtr<UStaticMesh>* Found = ManorMeshes.Find(Key)) return Found->Get();
    // Not cached while missing, so a mesh imported mid-session shows up on the next rebuild.
    UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr,
        *FString::Printf(TEXT("/Game/SurvivalGame/Environment/Props/%s/SM_%s.SM_%s"), Name, Name, Name),
        nullptr, LOAD_NoWarn | LOAD_Quiet);
    if (Mesh) ManorMeshes.Add(Key, Mesh);
    return Mesh;
}

void AHomesteadWorld::UpdateHearthFlicker(float DeltaSeconds)
{
    if (HearthLights.IsEmpty()) return;
    HearthFlickerTime += DeltaSeconds;
    HearthLights.RemoveAll([](const TWeakObjectPtr<UPointLightComponent>& Light) { return !Light.IsValid(); });
    for (int32 Index = 0; Index < HearthLights.Num(); ++Index)
    {
        const float T = HearthFlickerTime + Index * 7.3f;
        // Layered slow breathing and quick licks, like a settled wood fire.
        const float Flicker = 0.82f + 0.1f * FMath::PerlinNoise1D(T * 1.3f) + 0.08f * FMath::PerlinNoise1D(T * 7.1f)
            + 0.05f * FMath::PerlinNoise1D(T * 17.0f);
        HearthLights[Index]->SetIntensity(5200.0f * Flicker);
    }
}

void AHomesteadWorld::StartClearPop(FHomesteadWorldVisual& Visual, const Homestead::ResourceNode& Node)
{
    FClearPop Pop;
    for (USceneComponent* Part : Visual.Components)
        if (IsValid(Part))
        {
            Pop.Parts.Add(Part);
            Pop.Scales.Add(Part->GetRelativeScale3D());
            Pop.Locations.Add(Part->GetRelativeLocation());
        }
    // The popping parts now belong to the pop, not the node's visual.
    Visual.Components.Reset();
    if (!Pop.Parts.IsEmpty()) ClearPops.Add(MoveTemp(Pop));

    // Chips of whatever it was: clippings, splinters or grit.
    using Homestead::ResourceKind;
    const ResourceKind Kind = Node.kind;
    const bool bStone = Kind == ResourceKind::Rubble || Kind == ResourceKind::SmallRock || Kind == ResourceKind::Boulder
        || Kind == ResourceKind::RubbishHeap || Kind == ResourceKind::SalvagePile;
    const bool bGreen = Kind == ResourceKind::TallGrass || Kind == ResourceKind::Weeds || Kind == ResourceKind::Nettles
        || Kind == ResourceKind::BrambleThin || Kind == ResourceKind::BrambleThicket || Kind == ResourceKind::BrambleBank
        || Kind == ResourceKind::Sapling;
    const TCHAR* Path = bStone ? TEXT("/Game/SurvivalGame/Environment/Props/GraniteSpalls/SM_GraniteSpalls.SM_GraniteSpalls")
        : bGreen ? TEXT("/Game/SurvivalGame/Environment/Props/GrassYarrowTuft/SM_GrassYarrowTuft.SM_GrassYarrowTuft")
        : TEXT("/Game/SurvivalGame/Environment/Props/EstateTimber/SM_FallenBough.SM_FallenBough");
    UStaticMesh* ChipMesh = LoadObject<UStaticMesh>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
    if (!ChipMesh) return;
    const float Size = Kind == ResourceKind::Boulder || Kind == ResourceKind::StumpLarge || Kind == ResourceKind::StumpAncient
        || Kind == ResourceKind::RubbishHeap || Kind == ResourceKind::BrambleThicket ? 1.5f : 1.0f;
    FRandomStream Random(Node.id * 7919 + static_cast<int32>(GetWorld() ? GetWorld()->GetTimeSeconds() * 10.0 : 0.0));
    FClearPop Chips;
    Chips.bChips = true;
    Chips.Life = 0.75f;
    const FVector Origin = AtGround(Node.position.x, Node.position.y) + FVector(0, 0, 18.0f * Size);
    const int32 Count = FMath::RoundToInt(7 * Size);
    for (int32 Index = 0; Index < Count; ++Index)
    {
        auto* Chip = NewObject<UStaticMeshComponent>(this);
        Chip->SetupAttachment(GetRootComponent());
        Chip->SetMobility(EComponentMobility::Movable);
        Chip->SetStaticMesh(ChipMesh);
        Chip->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Chip->SetCollisionResponseToAllChannels(ECR_Ignore);
        Chip->SetGenerateOverlapEvents(false);
        Chip->SetCanEverAffectNavigation(false);
        Chip->SetCastShadow(false);
        const float Scale = (bStone ? 0.12f : bGreen ? 0.16f : 0.07f) * Random.FRandRange(0.7f, 1.2f) * FMath::Sqrt(Size);
        const FVector Start = Origin + FVector(Random.FRandRange(-12, 12), Random.FRandRange(-12, 12), Random.FRandRange(0, 10)) * Size;
        const FRotator Turn(Random.FRandRange(-40, 40), Random.FRandRange(0, 360), Random.FRandRange(-40, 40));
        Chip->SetRelativeTransform(FTransform(Turn, Start, FVector(Scale)));
        Chip->RegisterComponent();
        const float Heading = Random.FRandRange(0.0f, 2.0f * PI);
        const float Out = Random.FRandRange(90.0f, 220.0f) * Size;
        Chips.Parts.Add(Chip);
        Chips.Scales.Add(FVector(Scale));
        Chips.Locations.Add(Start);
        Chips.Velocities.Add(FVector(FMath::Cos(Heading) * Out, FMath::Sin(Heading) * Out, Random.FRandRange(260.0f, 420.0f)));
        Chips.Spins.Add(FRotator(Random.FRandRange(-540, 540), Random.FRandRange(-540, 540), Random.FRandRange(-540, 540)));
    }
    ClearPops.Add(MoveTemp(Chips));
}

void AHomesteadWorld::UpdateClearPops(float DeltaSeconds)
{
    for (int32 PopIndex = ClearPops.Num() - 1; PopIndex >= 0; --PopIndex)
    {
        FClearPop& Pop = ClearPops[PopIndex];
        Pop.Age += DeltaSeconds;
        const float T = FMath::Clamp(Pop.Age / Pop.Life, 0.0f, 1.0f);
        for (int32 Index = 0; Index < Pop.Parts.Num(); ++Index)
        {
            USceneComponent* Part = Pop.Parts[Index].Get();
            if (!Part) continue;
            if (Pop.bChips)
            {
                // Thrown out and falling, tumbling, shrinking away over the last third.
                const float Seconds = Pop.Age;
                const FVector& V = Pop.Velocities[Index];
                Part->SetRelativeLocation(Pop.Locations[Index] + FVector(V.X * Seconds, V.Y * Seconds,
                    V.Z * Seconds - 0.5f * 980.0f * Seconds * Seconds));
                Part->SetRelativeRotation(Part->GetRelativeRotation() + Pop.Spins[Index] * DeltaSeconds);
                Part->SetRelativeScale3D(Pop.Scales[Index] * FMath::Clamp((1.0f - T) * 3.0f, 0.0f, 1.0f));
            }
            else
            {
                // A quick swell, then it shrinks into the ground.
                const float Swell = T < 0.2f ? 1.0f + 0.1f * (T / 0.2f)
                    : 1.1f * (1.0f - FMath::SmoothStep(0.0f, 1.0f, (T - 0.2f) / 0.8f));
                Part->SetRelativeScale3D(Pop.Scales[Index] * FMath::Max(Swell, 0.001f));
                Part->SetRelativeLocation(Pop.Locations[Index] - FVector(0, 0, 12.0f * T));
            }
        }
        if (Pop.Age >= Pop.Life)
        {
            for (const TWeakObjectPtr<USceneComponent>& Part : Pop.Parts)
                if (Part.IsValid()) Part->DestroyComponent();
            ClearPops.RemoveAtSwap(PopIndex);
        }
    }
}

void AHomesteadWorld::UpdateHearthSound(float DeltaSeconds)
{
    HearthSounds.RemoveAll([](const FHearthSound& Sound) { return !Sound.Audio.IsValid(); });
    if (HearthSounds.IsEmpty()) return;
    APlayerController* Controller = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    if (!Controller) return;
    FVector Listener, Front, Right;
    Controller->GetAudioListenerPosition(Listener, Front, Right);
    for (FHearthSound& Sound : HearthSounds)
    {
        bool bHeard = false;
        // Past the attenuation radius there's nothing to hear, so skip the traces.
        if (FVector::DistSquared(Listener, Sound.Mouth) < FMath::Square(900.0f))
        {
            FCollisionQueryParams Params(SCENE_QUERY_STAT(HearthSound), false);
            if (APawn* Pawn = Controller->GetPawn()) Params.AddIgnoredActor(Pawn);
            for (const TWeakObjectPtr<UPrimitiveComponent>& Part : Sound.Ignored)
                if (Part.IsValid()) Params.AddIgnoredComponent(Part.Get());
            // Two sight lines (the fire's mouth and the breast above it), so a chair or her own arm
            // between her and the grate doesn't cut the sound out.
            bHeard = !GetWorld()->LineTraceTestByChannel(Listener, Sound.Mouth, ECC_Visibility, Params)
                || !GetWorld()->LineTraceTestByChannel(Listener, Sound.Chimney, ECC_Visibility, Params);
        }
        const float Target = bHeard ? 1.0f : 0.0f;
        Sound.Gate = Sound.Gate < 0.0f ? Target : FMath::FInterpConstantTo(Sound.Gate, Target, DeltaSeconds, 2.5f);
        Sound.Audio->SetVolumeMultiplier(FMath::Max(HearthCrackleVolume * Sound.Gate, 0.001f));
    }
}

void AHomesteadWorld::BuildStructure(FHomesteadWorldVisual& Visual, const Homestead::Structure& Structure,
    const Homestead::Building& Frame, bool bOnFoundation, bool bPreview, bool bValid, bool bDeconstruct)
{
    const Homestead::Point Center = Homestead::BuildingCellCenter(Frame, Structure.cellX, Structure.cellY);
    FVector Base(Center.x, Center.y, StructureBase(Center, Frame.yaw));
    // UE positive yaw rotates +X toward +Y; negative yaw maps the north edge to east.
    FRotator Rotation(0, Homestead::PieceYaw(Frame, Structure.rotation), 0);
    const bool bFurniture = Homestead::IsFurniture(Structure.kind);
    if (bFurniture && !bOnFoundation)
    {
        // Off a foundation the piece stands centred where it was placed, on the ground under its own
        // footprint, leaning with the slope so a bedroll or chest doesn't hover on the downhill side.
        const FVector2D Local = Structure.kind == Homestead::Piece::Bed ? FVector2D(95, -10)
            : Structure.kind == Homestead::Piece::Chest ? FVector2D(-100, -100)
            : Structure.kind == Homestead::Piece::Hearth ? FVector2D(0, 98) : FVector2D(-100, 95);
        const FVector2D Half = Structure.kind == Homestead::Piece::Bed ? FVector2D(35, 78)
            : Structure.kind == Homestead::Piece::Chest ? FVector2D(35, 28)
            : Structure.kind == Homestead::Piece::Hearth ? FVector2D(85, 32) : FVector2D(34, 34);
        const FVector Pivot = FVector(Center.x, Center.y, 0);
        const FVector AxisX = Rotation.RotateVector(FVector::ForwardVector);
        const FVector AxisY = Rotation.RotateVector(FVector::RightVector);
        auto GroundAt = [&](float DX, float DY)
        {
            const FVector P = Pivot + AxisX * DX + AxisY * DY;
            return GroundHeight(P.X, P.Y);
        };
        const float SlopeX = (GroundAt(Half.X, 0) - GroundAt(-Half.X, 0)) / (2 * Half.X);
        const float SlopeY = (GroundAt(0, Half.Y) - GroundAt(0, -Half.Y)) / (2 * Half.Y);
        const float PivotZ = GroundHeight(Pivot.X, Pivot.Y) - 1.5f;
        Rotation = FRotationMatrix::MakeFromXY(AxisX + FVector::UpVector * SlopeX,
            AxisY + FVector::UpVector * SlopeY).Rotator();
        // Keep the footprint centre on the ground once the tilt swings the cell-centre origin.
        Base = FVector(Pivot.X, Pivot.Y, PivotZ) - Rotation.RotateVector(FVector(Local, 0));
    }
    auto Part = [&](UStaticMesh* Mesh, FVector Offset, FVector Size, FLinearColor Color,
        bool bSolid = false, FRotator LocalRotation = FRotator::ZeroRotator, float Glow = 0.0f)
    {
        const FRotator Combined = (Rotation.Quaternion() * LocalRotation.Quaternion()).Rotator();
        const FVector Shown = bDeconstruct ? Size * 1.02f + FVector(4.0f) : Size;
        const FLinearColor PreviewTint = bDeconstruct ? (bValid ? DeconstructColor : PreviewBlockedColor)
            : (bValid ? PreviewColor : PreviewBlockedColor);
        return AddPart(Visual, Mesh, Base + Rotation.RotateVector(Offset), Shown,
            bPreview ? PreviewTint : Color, bSolid && !bPreview, Combined, 0.85f, bPreview ? 0.0f : Glow);
    };
    // An imported kit mesh at the piece's pivot, keeping its own baked materials.
    auto KitPart = [&](const TCHAR* Name, float HeightScale = 1.0f) -> UStaticMeshComponent*
    {
        UStaticMesh* Mesh = ManorMesh(Name);
        if (!Mesh) return nullptr;
        UStaticMeshComponent* Placed = Part(Mesh, FVector::ZeroVector, FVector(100.0f, 100.0f, 100.0f * HeightScale),
            FLinearColor::White, true);
        if (Placed && !bPreview)
            for (int32 Slot = 0; Slot < Mesh->GetStaticMaterials().Num(); ++Slot)
                Placed->SetMaterial(Slot, Mesh->GetMaterial(Slot));
        return Placed;
    };
    if (Structure.skin == Homestead::StructureSkin::Stone)
    {
        const TCHAR* Kit = Structure.kind == Homestead::Piece::Foundation ? TEXT("StoneFoundation")
            : Structure.kind == Homestead::Piece::Wall ? TEXT("StoneWall")
            : Structure.kind == Homestead::Piece::Doorway ? TEXT("StoneDoorway")
            : Structure.kind == Homestead::Piece::Roof ? TEXT("StoneRoof") : nullptr;
        // The roof tiles only at yaw 0 about the building's own grid.
        if (Kit && Structure.kind == Homestead::Piece::Roof) Rotation = FRotator(0, Frame.yaw, 0);
        // The roof's joists run one way only, so the walls rise past their coping (258 cm) to the deck
        // underside (279 cm): the joist ends bed into the masonry instead of leaving daylight between them.
        const bool bWallPiece = Structure.kind == Homestead::Piece::Wall || Structure.kind == Homestead::Piece::Doorway;
        if (Kit && KitPart(Kit, bWallPiece ? 1.085f : 1.0f)) return;
    }
    if (Structure.kind == Homestead::Piece::Hearth)
    {
        if (!KitPart(TEXT("StoneHearth")))
        {
            // Blockout until the hearth mesh is imported: jambs, lintel and breast.
            Part(Cube, FVector(0, 85, 1.5f), FVector(192, 90, 3), Stone, true);
            for (int Side : {-1, 1})
                Part(Cube, FVector(Side * 70.5f, 98, 55), FVector(29, 64, 104), Stone, true);
            Part(Cube, FVector(0, 97, 124), FVector(180, 67, 32), Stone, true);
            Part(Cube, FVector(0, 98, 199), FVector(170, 64, 118), Stone, true);
            Part(Cube, FVector(0, 126, 55), FVector(112, 8, 104), FLinearColor(0.03f, 0.028f, 0.026f));
        }
        if (bPreview) return;
        const FVector Fire(0, 100, 0);
        // Low flames licking up from the logs, and embers glowing under them.
        Part(Cube, Fire + FVector(0, 4, 9), FVector(70, 26, 4), FLinearColor(0.9f, 0.18f, 0.02f), false,
            FRotator::ZeroRotator, 2.2f);
        const FVector Flames[] = {{-18, 0, 30}, {6, -3, 34}, {24, 3, 28}, {-4, 6, 40}};
        const FVector FlameSizes[] = {{16, 12, 30}, {20, 14, 40}, {14, 11, 26}, {9, 8, 26}};
        for (int I = 0; I < 4; ++I)
        {
            Part(Cone, Fire + Flames[I], FlameSizes[I], FLinearColor(0.95f, 0.26f, 0.03f), false, FRotator::ZeroRotator, 3.0f);
            Part(Cone, Fire + Flames[I] - FVector(0, 0, 4), FlameSizes[I] * 0.55f, FLinearColor(1.0f, 0.66f, 0.16f),
                false, FRotator::ZeroRotator, 4.5f);
        }
        UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
        Light->SetupAttachment(GetRootComponent());
        Light->SetMobility(EComponentMobility::Movable);
        // Just in front of the opening, so the room (not the firebox) takes the light.
        Light->SetRelativeLocation(Base + Rotation.RotateVector(FVector(0, 55, 55)));
        Light->SetLightColor(FLinearColor(1.0f, 0.45f, 0.16f));
        Light->SetIntensity(5200.0f);
        Light->SetAttenuationRadius(900.0f);
        Light->SetSourceRadius(30.0f);
        Light->SetCastShadows(true);
        Light->RegisterComponent();
        Visual.Components.Add(Light);
        HearthLights.Add(Light);
        if (!HearthCrackle)
            HearthCrackle = LoadObject<USoundWave>(nullptr,
                TEXT("/Game/SurvivalGame/Audio/Ambience/HearthCrackle.HearthCrackle"), nullptr, LOAD_NoWarn | LOAD_Quiet);
        if (HearthCrackle)
        {
            UAudioComponent* Crackle = NewObject<UAudioComponent>(this);
            Crackle->SetupAttachment(GetRootComponent());
            Crackle->SetRelativeLocation(Base + Rotation.RotateVector(Fire + FVector(0, 0, 30)));
            Crackle->SetSound(HearthCrackle);
            Crackle->bAutoActivate = false;
            // A small fire: full within a couple of metres, gone about 7 m away, so it fills its own room only.
            Crackle->bOverrideAttenuation = true;
            Crackle->AttenuationOverrides.bAttenuate = true;
            Crackle->AttenuationOverrides.bSpatialize = true;
            Crackle->AttenuationOverrides.DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound;
            Crackle->AttenuationOverrides.dBAttenuationAtMax = -60.0f;
            Crackle->AttenuationOverrides.AttenuationShapeExtents = FVector(150.0f);
            Crackle->AttenuationOverrides.FalloffDistance = 550.0f;
            Crackle->SetVolumeMultiplier(HearthCrackleVolume);
            Crackle->RegisterComponent();
            Crackle->Play(FMath::FRandRange(0.0f, 20.0f));
            Visual.Components.Add(Crackle);
            FHearthSound& Sound = HearthSounds.AddDefaulted_GetRef();
            Sound.Audio = Crackle;
            Sound.Mouth = Light->GetComponentLocation();
            Sound.Chimney = Base + Rotation.RotateVector(FVector(0, 45, 130));
            for (const TObjectPtr<USceneComponent>& Component : Visual.Components)
                if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component.Get()))
                    Sound.Ignored.Add(Primitive);
        }
        return;
    }
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

UStaticMesh* AHomesteadWorld::CropMesh(Homestead::CropKind Kind, const TCHAR* StageName)
{
    const FString Visual = UTF8_TO_TCHAR(Homestead::GetCropInfo(Kind).visual);
    if (Visual.IsEmpty()) return nullptr;
    const FName Key(*FString::Printf(TEXT("%s_%s"), *Visual, StageName));
    if (const TObjectPtr<UStaticMesh>* Cached = CropMeshes.Find(Key)) return Cached->Get();
    UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *FString::Printf(
        TEXT("/Game/SurvivalGame/Environment/Props/%s/SM_%s_%s.SM_%s_%s"), *Visual, *Visual, StageName, *Visual, StageName),
        nullptr, LOAD_NoWarn | LOAD_Quiet);
    CropMeshes.Add(Key, Mesh);
    return Mesh;
}

void AHomesteadWorld::BuildPlot(FHomesteadWorldVisual& Visual, const Homestead::Plot& Plot)
{
    const Homestead::Point Center = Homestead::PlotCenter(Plot);
    const float Moisture = Stage(Plot.moisture, 5) / 5.0f;
    const FLinearColor WetSoil = FMath::Lerp(Soil, FLinearColor(0.075f, 0.044f, 0.025f), Moisture);
    // One hoed square of turned soil, a little inside the garden square so neighbours read apart,
    // with one plant at its middle.
    {
        {
            const float PX = Center.x;
            const float PY = Center.y;
            // One hoed square of loose loam raked into three ridges (the Blender tilled bed; its
            // ragged rim sinks below the ground line), laid on the local slope of the terrain.
            // Watering swaps in the darker wet-soil texture.
            if (!TilledBedMesh)
                TilledBedMesh = LoadObject<UStaticMesh>(nullptr,
                    TEXT("/Game/SurvivalGame/Environment/Props/TilledBed/SM_TilledBed.SM_TilledBed"), nullptr, LOAD_NoWarn | LOAD_Quiet);
            if (!TilledBedWetMaterial)
                TilledBedWetMaterial = LoadObject<UMaterialInterface>(nullptr,
                    TEXT("/Game/SurvivalGame/Environment/Props/TilledBed/MI_TilledBed_Wet.MI_TilledBed_Wet"), nullptr, LOAD_NoWarn | LOAD_Quiet);
            UStaticMesh* Bed = TilledBedMesh;
            UMaterialInterface* WetBed = TilledBedWetMaterial;
            if (Bed)
            {
                constexpr float Probe = 40.0f;
                const FVector Normal = FVector(GroundHeight(PX - Probe, PY) - GroundHeight(PX + Probe, PY),
                    GroundHeight(PX, PY - Probe) - GroundHeight(PX, PY + Probe), 2 * Probe).GetSafeNormal();
                const float Yaw = (Plot.id % 2) ? 180.0f : 0.0f;
                const FRotator Lie = FRotationMatrix::MakeFromZX(Normal, FRotator(0, Yaw, 0).Vector()).Rotator();
                if (auto* Part = AddPart(Visual, Bed, AtGround(PX, PY, -1.2f), FVector(100, 100, 100), WetSoil, false, Lie))
                    Part->SetMaterial(0, !Homestead::NeedsWater(Plot) && WetBed ? WetBed : Bed->GetMaterial(0));
            }
            else
                AddPart(Visual, Cube, AtGround(PX, PY, 0.4f), FVector(Homestead::GardenCellSize - 8.0f, Homestead::GardenCellSize - 8.0f, 1.2f),
                    WetSoil * 0.85f, false, FRotator::ZeroRotator, 0.95f - Moisture * 0.35f);
            constexpr int X = 0, Y = 0;
            const Homestead::CropStage CropStage = Homestead::StageOf(Plot);
            UStaticMesh* Plant = CropStage >= Homestead::CropStage::Sprout
                ? CropMesh(Plot.kind, UTF8_TO_TCHAR(Homestead::StageName(CropStage))) : nullptr;
            if (Plant)
            {
                // The Blender plant for this stage, sized for the square and set on the bed's ridges.
                constexpr float Probe = 40.0f;
                const FVector Normal = FVector(GroundHeight(PX - Probe, PY) - GroundHeight(PX + Probe, PY),
                    GroundHeight(PX, PY - Probe) - GroundHeight(PX, PY + Probe), 2 * Probe).GetSafeNormal();
                const float Yaw = (Plot.id % 2) ? 180.0f : 0.0f;
                const FRotator Lie = FRotationMatrix::MakeFromZX(Normal, FRotator(0, Yaw, 0).Vector()).Rotator();
                if (auto* Part = AddPart(Visual, Plant, AtGround(PX, PY, Bed ? -1.2f : 0.0f), FVector(100, 100, 100), Leaf, false, Lie))
                    Part->SetMaterial(0, Plant->GetMaterial(0));
                if (CropStage == Homestead::CropStage::Ripe)
                {
                    // A slow, soft glint over a ripe plant, readable from across the garden.
                    if (!bRipeGlintLoaded)
                    {
                        RipeGlintMaterial = LoadObject<UMaterialInterface>(nullptr,
                            TEXT("/Game/SurvivalGame/Environment/Props/CropGlint/M_CropRipeGlint.M_CropRipeGlint"), nullptr, LOAD_NoWarn | LOAD_Quiet);
                        bRipeGlintLoaded = true;
                    }
                    if (RipeGlintMaterial)
                    {
                        auto* Glint = NewObject<UMaterialBillboardComponent>(this);
                        Glint->SetupAttachment(GetRootComponent());
                        Glint->SetMobility(EComponentMobility::Movable);
                        const float Top = Plant->GetBounds().GetBox().Max.Z;
                        Glint->SetRelativeLocation(AtGround(PX, PY, Top + 18.0f));
                        Glint->AddElement(RipeGlintMaterial, nullptr, false, 26.0f, 26.0f, nullptr);
                        Glint->SetCastShadow(false);
                        Glint->RegisterComponent();
                        Visual.Components.Add(Glint);
                    }
                }
            }
            else if (Plot.planted && CropStage == Homestead::CropStage::Sown)
            {
                // Just sown: a small mound of soil over the seed, with her fingertip's press.
                if (!SoilMoundMesh)
                    SoilMoundMesh = LoadObject<UStaticMesh>(nullptr,
                        TEXT("/Game/SurvivalGame/Environment/Props/Seeds/SM_SoilMound.SM_SoilMound"), nullptr, LOAD_NoWarn | LOAD_Quiet);
                UStaticMesh* Mound = SoilMoundMesh;
                if (auto* Part = AddPart(Visual, Mound ? Mound : Sphere.Get(), AtGround(PX, PY + 12, Mound ? (Bed ? 0.6f : 0.0f) : 0.5f),
                    Mound ? FVector(120, 120, 130) : FVector(15, 15, 4), WetSoil * 0.8f))
                    if (Mound) Part->SetMaterial(0, Mound->GetMaterial(0));
            }
            else if (Plot.planted)
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
    // Weeds creeping into the bed: small tufts of the estate's yarrow/grass weed at the rim and
    // between the ridges (the cones remain only if the mesh isn't imported).
    if (!WeedTuftMesh)
        WeedTuftMesh = LoadObject<UStaticMesh>(nullptr,
            TEXT("/Game/SurvivalGame/Environment/Props/GrassYarrowTuft/SM_GrassYarrowTuft.SM_GrassYarrowTuft"), nullptr, LOAD_NoWarn | LOAD_Quiet);
    UStaticMesh* WeedTuft = WeedTuftMesh;
    for (int I = 0; I < WeedCount; ++I)
    {
        const float X = Center.x + Random.FRandRange(-40, 40);
        const float Y = Center.y + Random.FRandRange(-40, 40);
        if (WeedTuft)
        {
            const float Scale = Random.FRandRange(0.28f, 0.42f);
            if (auto* Part = AddPart(Visual, WeedTuft, AtGround(X, Y, -1.0f), FVector(100 * Scale), Leaf, false,
                FRotator(0, Random.FRandRange(0, 360), 0)))
                Part->SetMaterial(0, WeedTuft->GetMaterial(0));
        }
        else
            AddPart(Visual, Cone, AtGround(X, Y, 12), FVector(14, 14, 24),
                FLinearColor(0.34f, 0.31f, 0.07f), false, FRotator(0, I * 47, 16));
    }
}

void AHomesteadWorld::BuildDrop(FHomesteadWorldVisual& Visual, const Homestead::WorldDrop& Drop)
{
    FLinearColor Tint(0.56f, 0.43f, 0.22f);
    if (Drop.wearableId != 0) Tint = FLinearColor(0.64f, 0.42f, 0.52f);
    else
    {
        switch (Homestead::CategoryOf(Drop.item))
        {
        case Homestead::ItemCategory::Tool: Tint = FLinearColor(0.22f, 0.28f, 0.26f); break;
        case Homestead::ItemCategory::Forage:
        case Homestead::ItemCategory::Food: Tint = FLinearColor(0.62f, 0.30f, 0.19f); break;
        case Homestead::ItemCategory::Supply: Tint = FLinearColor(0.34f, 0.54f, 0.48f); break;
        default: break;
        }
    }
    const FVector Base = AtGround(Drop.position.x, Drop.position.y, 7);
    AddPart(Visual, Cylinder, Base, FVector(48, 48, 14), Wood, false);
    AddPart(Visual, Cube, Base + FVector(0, 0, 16), FVector(44, 34, 14), Tint,
        false, FRotator(0, Drop.id * 37 % 360, 8), 0.75f);
    const int32 Marks = FMath::Clamp(Drop.quantity, 1, 5);
    for (int32 Index = 0; Index < Marks; ++Index)
        AddPart(Visual, Sphere, Base + FVector(-12 + Index * 6, 0, 29),
            FVector(5, 5, 5), FLinearColor(0.93f, 0.82f, 0.52f), false);
    const FLinearColor Tie(0.78f, 0.57f, 0.18f);
    AddPart(Visual, Cube, Base + FVector(0, 0, 24), FVector(7, 36, 4),
        Tie, false, FRotator::ZeroRotator, 0.7f);
    AddPart(Visual, Cube, Base + FVector(0, 0, 24), FVector(44, 7, 4),
        Tie, false, FRotator::ZeroRotator, 0.7f);
    AddPart(Visual, Sphere, Base + FVector(0, 0, 30), FVector(9, 9, 9),
        FLinearColor(0.93f, 0.72f, 0.24f), false, FRotator::ZeroRotator, 0.65f, 0.05f);
}

void AHomesteadWorld::UpdateLighting(const Homestead::State& State)
{
    const float Hour = static_cast<float>(FMath::Fmod(State.hour, 24.0));
    const float SolarAngle = (Hour - 6.0f) / 24.0f * 2.0f * PI;
    const float Elevation = FMath::Sin(SolarAngle);
    const float Daylight = FMath::SmoothStep(-0.1f, 0.25f, Elevation);
    // Matches the simulation's deterministic three-day spring weather cycle.
    const bool bRaining = static_cast<int64>(State.hour / 24.0) % 3 == 1 && Hour >= 9.0f && Hour < 15.0f;
    const FRotator SunRotation(-Elevation * 65.0f, (Hour - 6) * 15.0f - 70.0f, 0);
    const FRotator MoonRotation(Elevation * 65.0f, (Hour - 6) * 15.0f + 110.0f, 0);
    // With ray-traced sun shadows, follow the sun every refresh (about 0.025 degrees at normal game
    // speed, so no visible shadow step). Virtual Shadow Maps re-render every cached page when a
    // directional light rotates, so on that fallback step by 0.5 degrees, a few real seconds of
    // daylight travel, to keep full re-renders rare.
    const bool bRayTracedSun = IsRayTracingEnabled() && CVarRayTracedSun.GetValueOnGameThread() != 0;
    const auto ShadowMode = bRayTracedSun ? ECastRayTracedShadow::Enabled : ECastRayTracedShadow::Disabled;
    if (Sun->GetCastRaytracedShadow() != ShadowMode)
    {
        Sun->SetCastRaytracedShadows(ShadowMode);
        Moon->SetCastRaytracedShadows(ShadowMode);
    }
    const float LightRotationStepDegrees = bRayTracedSun ? 0.0f : 0.5f;
    if (!bLightRotationApplied || !SunRotation.Equals(AppliedSunRotation, LightRotationStepDegrees))
    {
        Sun->SetRelativeRotation(SunRotation);
        Moon->SetRelativeRotation(MoonRotation);
        AppliedSunRotation = SunRotation;
        AppliedMoonRotation = MoonRotation;
        bLightRotationApplied = true;
    }
    Sun->SetIntensity(FMath::Lerp(0.0f, bRaining ? 17000.0f : 46000.0f, Daylight));
    // The sky atmosphere already reddens a low sun through its transmittance; keep only a mild
    // extra tint so dawn stays golden instead of saturating to orange.
    Sun->SetLightColor(FMath::Lerp(FLinearColor(1.0f, 0.9f, 0.8f),
        FLinearColor(1.0f, 0.99f, 0.95f), FMath::Clamp(Elevation * 2, 0.0f, 1.0f)));
    const float NightMoonLux = CVarNightMoonLux.GetValueOnGameThread();
    const float NightSkyIntensity = CVarNightSky.GetValueOnGameThread();
    const float NightMinExposure = CVarNightMinExposure.GetValueOnGameThread();
    Moon->SetIntensity(NightMoonLux * (1.0f - Daylight));
    Sky->SetIntensity(FMath::Lerp(NightSkyIntensity, 1.0f, Daylight));
    Exposure->Settings.AutoExposureMinBrightness = FMath::Lerp(NightMinExposure, 0.0f, Daylight);
    Fog->SetFogDensity(bRaining ? 0.035f : FMath::Lerp(0.016f, 0.007f, Daylight));
    Fog->SetFogInscatteringColor(bRaining ? FLinearColor(0.43f, 0.49f, 0.52f)
        : FMath::Lerp(FLinearColor(0.055f, 0.085f, 0.14f), FLinearColor(0.64f, 0.72f, 0.68f), Daylight));

    // The ground and meadow wet through in the first half hour of rain and dry over the four hours
    // after it stops; the rain days follow the simulation's schedule (as bRaining above).
    if (!bGroundParametersTried)
    {
        bGroundParametersTried = true;
        GroundParameters = LoadObject<UMaterialParameterCollection>(nullptr,
            TEXT("/Game/SurvivalGame/Estate/Ground/MPC_EstateGround.MPC_EstateGround"));
    }
    if (GroundParameters && GetWorld())
        if (UMaterialParameterCollectionInstance* GroundValues = GetWorld()->GetParameterCollectionInstance(GroundParameters))
        {
            const bool bRainDay = static_cast<int64>(State.hour / 24.0) % 3 == 1;
            const float Wetness = !bRainDay ? 0.0f
                : FMath::SmoothStep(9.0f, 9.5f, Hour) * (1.0f - FMath::SmoothStep(15.0f, 19.0f, Hour));
            GroundValues->SetScalarParameterValue(TEXT("Wetness"), Wetness);
            GroundValues->SetScalarParameterValue(TEXT("Daylight"), Daylight);
        }
}

bool AHomesteadWorld::Initialize(const Homestead::Simulation& Simulation)
{
    ClearVisual(Preview);
    if (!bInitialized)
    {
        FieldMaterial = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/SurvivalGame/Materials/M_Field.M_Field"));
        CreekWaterMaterial = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/SurvivalGame/Materials/M_CreekWater.M_CreekWater"));
        if (!CreekWaterMaterial)
            UE_LOG(LogHomesteadWorld, Warning, TEXT("M_CreekWater is missing; the creek falls back to a flat tint. Run Scripts/bootstrap_unreal.py."));
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
        if (!LoadCameraSafeFoliageMaterials())
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
    FString Layout = FString::Printf(TEXT("%llu:%u:%d,%d;"), static_cast<unsigned long long>(State.world.seed),
        State.world.generationVersion, State.activeChunk.x, State.activeChunk.y);
    for (const auto& Node : State.resources)
    {
        Layout += FString::Printf(TEXT("%d:%.3f:%.3f:%d;"), Node.id, Node.position.x, Node.position.y, Node.cleared);
    }
    for (const auto& Structure : State.structures)
    {
        Layout += FString::Printf(TEXT("S:%d:%d:%d;"), Structure.buildingId, Structure.cellX, Structure.cellY);
    }
    for (const auto& Plot : State.plots)
    {
        Layout += FString::Printf(TEXT("P:%d:%d;"), Plot.cellX, Plot.cellY);
    }
    for (const auto& Drop : State.worldDrops)
        Layout += FString::Printf(TEXT("D:%d:%.3f:%.3f;"), Drop.id, Drop.position.x, Drop.position.y);
    if (ResourceLayoutSignature != Layout)
    {
        if (!State.fixedEstate && !BuildDecorations(Simulation)) return false;
        ResourceLayoutSignature = MoveTemp(Layout);
    }
    if (State.fixedEstate) ClearEstateSceneryUnderPieces(State);
    if (State.fixedEstate)
    {
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
    else if (EstateGrass)
        EstateGrass->Clear();
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
        const FString Signature = FString::Printf(TEXT("%d:%.3f:%.3f:%d"),
            static_cast<int>(Node.kind), Node.position.x, Node.position.y, Shown.cleared);
        AdoptStaged(ResourceVisuals, StagedResourceVisuals, Node.id, Signature);
        if (bVisualBuildFailed) return false;
        FHomesteadWorldVisual& Visual = ResourceVisuals.FindOrAdd(Node.id);
        if (Visual.Signature != Signature)
        {
            // Cleared in play (not loaded or streamed in cleared): it pops away instead of vanishing.
            if (Shown.cleared && !Transition && Homestead::IsOvergrowth(Node.kind) && Visual.Signature.EndsWith(TEXT(":0")))
                StartClearPop(Visual, Node);
            ClearVisual(Visual);
            BuildResource(Visual, Shown, false);
            if (bVisualBuildFailed) return false;
            Visual.Signature = Signature;
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
            Plot.cellX, Plot.cellY, bShownPlanted, bSquareHidden, Stage(Plot.growth, 12),
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

void AHomesteadWorld::SetPlacementPreview(bool Visible, const Homestead::PlacementTarget& Target, bool bValid)
{
    if (!Visible || !bInitialized || Target.kind == Homestead::Piece::Count)
    {
        ClearVisual(Preview);
        return;
    }
    const bool bOnFoundation = Target.buildingId >= 0
        && FoundationCells.Contains(FIntVector(Target.cellX, Target.cellY, Target.buildingId));
    const FString Signature = FString::Printf(TEXT("%d:%d:%d:%d:%d:%.1f:%.1f:%.2f:%d:%d"), static_cast<int>(Target.kind),
        Target.buildingId, Target.cellX, Target.cellY, Target.rotation, Target.frame.origin.x, Target.frame.origin.y,
        Target.frame.yaw, bOnFoundation, bValid);
    if (Preview.Signature == Signature)
    {
        return;
    }
    ClearVisual(Preview);
    Homestead::Structure Structure;
    Structure.kind = Target.kind;
    Structure.buildingId = Target.buildingId;
    Structure.cellX = Target.cellX;
    Structure.cellY = Target.cellY;
    Structure.rotation = Target.rotation;
    BuildStructure(Preview, Structure, Target.frame, bOnFoundation, true, bValid);
    Preview.Signature = Signature;
}

void AHomesteadWorld::SetDeconstructPreview(const Homestead::State& State, int32 StructureId, bool bValid)
{
    if (!bInitialized || StructureId < 0)
    {
        ClearVisual(Preview);
        return;
    }
    const Homestead::Structure* Structure = nullptr;
    for (const auto& Piece : State.structures)
        if (Piece.id == StructureId) { Structure = &Piece; break; }
    if (!Structure)
    {
        ClearVisual(Preview);
        return;
    }
    const bool bOnFoundation = Homestead::HasFoundation(State, Structure->buildingId, Structure->cellX, Structure->cellY);
    const FString Signature = FString::Printf(TEXT("D:%d:%d:%d"), StructureId, bOnFoundation, bValid);
    if (Preview.Signature == Signature) return;
    ClearVisual(Preview);
    const Homestead::Building* Frame = Homestead::FindBuilding(State, Structure->buildingId);
    BuildStructure(Preview, *Structure, Frame ? *Frame : Homestead::Building{}, bOnFoundation, true, bValid, true);
    Preview.Signature = Signature;
}
