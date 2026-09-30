#include "HomesteadWorld.h"
#include "HomesteadWorldCommon.h"
#include "HomesteadWorldLog.h"
#include "HomesteadWorldLook.h"
#include "HomesteadEstateTerrain.h"
#include "HomesteadGrassField.h"
#include "Simulation/HomesteadOvergrowth.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Misc/Paths.h"
#include "PhysicsEngine/BodySetup.h"

namespace HomesteadWorldWoodland
{
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

using HomesteadWorldCommon::ProfileChunkPublishing;
using HomesteadWorldWoodland::FRockKind;
using HomesteadWorldWoodland::RockKindCount;
using HomesteadWorldWoodland::RockKinds;
using HomesteadWorldWoodland::UnderbrushSpecies;
using HomesteadWorldWoodland::UnderbrushSpeciesCount;
using HomesteadWorldWoodland::StartingClearing;
using HomesteadWorldWoodland::StartingClearingBlockingRadius;
using HomesteadWorldWoodland::StartingClearingShrubRadius;
using HomesteadWorldWoodland::PickUnderbrush;

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
