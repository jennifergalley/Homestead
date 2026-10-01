#include "HomesteadWorld.h"
#include "HomesteadWorldCommon.h"
#include "HomesteadWorldKeys.h"
#include "HomesteadWorldLog.h"
#include "HomesteadWorldLook.h"
#include "HomesteadEstateTerrain.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace HomesteadWorldEstate
{
TAutoConsoleVariable<int32> CVarEstateSceneryCells(TEXT("homestead.EstateSceneryCells"), 1,
    TEXT("1 = batch non-Nanite estate scenery per 128/512 m cell (default), 0 = one batch per kind. ")
    TEXT("Read when the estate scenery is built (set it on the command line with -DPCVars=)."));

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
constexpr FEstateSceneryKind EstateSceneryKinds[] = {
    {TEXT("/Game/Trials/WoodlandResources_20260921_01/Meshes/SM_TreeSmall02_Woodland.SM_TreeSmall02_Woodland"), true, 0, true, 3, 41},
    {TEXT("/Game/Trials/MatureFir_20260922_02/Meshes/SM_MatureFir.SM_MatureFir"), true, 0, true, 30, 76},
    {TEXT("/Game/SurvivalGame/Environment/Props/Hazel/SM_Hazel.SM_Hazel"), false, 14000, false, 0, 0},
    {TEXT("/Game/SurvivalGame/Environment/Props/BrackenFern/SM_BrackenFern.SM_BrackenFern"), false, 9000, false, 0, 0},
    {TEXT("/Game/SurvivalGame/Environment/Props/GrassYarrowTuft/SM_GrassYarrowTuft.SM_GrassYarrowTuft"), false, 6000, false, 0, 0},
    // A granite ledge breaking through the slope (scatter.py LEDGE): a talus block sunk a quarter of its
    // height below the lowest ground under it. It used to be loose cobbles, which read as hand stones.
    {TEXT("/Game/SurvivalGame/Environment/Props/GraniteBlockTalus/SM_GraniteBlockTalus.SM_GraniteBlockTalus"), true, 24000, false, 22, 52},
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
    // 42-48: the lake trail's wildflowers (Scripts/Terrain/lake_path_plants.py; Jenny, 2026-09-30), decorative only: no
    // collision or shadow, culled within a short walk. Bluebells, primroses and wild garlic reuse the forage
    // clumps' meshes; anemone, campion, foxglove and cow parsley come from Props' recipes (a missing mesh logs
    // once and its records are skipped until it's imported).
    {TEXT("/Game/SurvivalGame/Environment/Props/Bluebell/SM_BluebellClump.SM_BluebellClump"), false, 4500, false, 2, 0},
    {TEXT("/Game/SurvivalGame/Environment/Props/Primrose/SM_PrimroseClump.SM_PrimroseClump"), false, 4500, false, 1, 0},
    {TEXT("/Game/SurvivalGame/Environment/Props/WildGarlic/SM_WildGarlic.SM_WildGarlic"), false, 4500, false, 2, 0},
    {TEXT("/Game/SurvivalGame/Environment/Props/WoodAnemone/SM_WoodAnemoneClump.SM_WoodAnemoneClump"), false, 4500, false, 1, 0},
    {TEXT("/Game/SurvivalGame/Environment/Props/RedCampion/SM_RedCampionClump.SM_RedCampionClump"), false, 5500, false, 2, 0},
    {TEXT("/Game/SurvivalGame/Environment/Props/Foxglove/SM_Foxglove.SM_Foxglove"), false, 6500, false, 2, 0},
    {TEXT("/Game/SurvivalGame/Environment/Props/CowParsley/SM_CowParsley.SM_CowParsley"), false, 6000, false, 2, 0},
};
// The kind byte indexes this table directly, so kinds are append-only: a row inserted mid-table re-meshes every
// later kind across the estate. Sentinels pin a few indices to their assets (scatter.py, mvp_woodland.py,
// lake_path_plants.py write these bytes).
constexpr bool EstateKindIs(int32 Kind, const TCHAR* Name)
{
    const TCHAR* Path = EstateSceneryKinds[Kind].Path;
    int32 PathLength = 0, NameLength = 0;
    while (Path[PathLength]) ++PathLength;
    while (Name[NameLength]) ++NameLength;
    if (NameLength > PathLength) return false;
    for (int32 Index = 0; Index < NameLength; ++Index)
        if (Path[PathLength - NameLength + Index] != Name[Index]) return false;
    return true;
}
static_assert(UE_ARRAY_COUNT(EstateSceneryKinds) == 49, "EstateSceneryKinds is append-only: claim the next kind on the round page");
static_assert(EstateKindIs(5, TEXT("SM_GraniteBlockTalus")) && EstateKindIs(13, TEXT("SM_Oak"))
    && EstateKindIs(19, TEXT("SM_Jacaranda")) && EstateKindIs(30, TEXT("SM_Fern02_c")) && EstateKindIs(31, TEXT("SM_Fern02_d"))
    && EstateKindIs(33, TEXT("SM_GrassMedium01_tiny_a")) && EstateKindIs(41, TEXT("SM_GraniteSplitBoulder"))
    && EstateKindIs(42, TEXT("SM_BluebellClump")) && EstateKindIs(48, TEXT("SM_CowParsley")),
    "EstateSceneryKinds moved: a kind's index no longer matches the bytes in EstateScenery.bin");

#pragma pack(push, 1)
struct FEstateSceneryRecord
{
    uint8 Kind;
    uint8 Pad[3];
    float X, Y, Yaw, Scale;
};
#pragma pack(pop)
static_assert(sizeof(FEstateSceneryRecord) == 20, "EstateScenery.bin records are 20 bytes");

// Non-Nanite scenery is batched per kind *and* per square cell of the estate. A non-Nanite HISM
// gathers its ray-tracing instances by scanning every instance it holds, every frame
// (FInstancedStaticMeshSceneProxy::GetDynamicRayTracingInstances), so one estate-wide batch of 80k
// grass tufts cost ~3.5 ms of render thread at the manor. Cells let the renderer drop whole batches
// (ray tracing culls primitives more than r.RayTracing.Culling.Radius, 300 m, away; the draw
// distance below culls cells past the kind's instance cull distance) before any instance is looked at.
// Kinds with a cull distance use small cells; kinds drawn at any distance use large ones, so the far
// view doesn't turn into thousands of draws. Nanite kinds stay one batch: Nanite culls on the GPU.
constexpr float EstateSceneryNearCellCm = 12800.0f;
constexpr float EstateSceneryFarCellCm = 51200.0f;
}

using HomesteadWorldEstate::CVarEstateSceneryCells;
using HomesteadWorldEstate::EstateSceneryKinds;
using HomesteadWorldEstate::FEstateSceneryKind;
using HomesteadWorldEstate::FEstateSceneryRecord;
using HomesteadWorldEstate::EstateSceneryNearCellCm;
using HomesteadWorldEstate::EstateSceneryFarCellCm;
using HomesteadWorldLook::Wood;

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
    UStaticMesh* KindMeshes[KindCount] = {};
    bool KindTried[KindCount] = {};
    float KindCellCm[KindCount] = {};
    TMap<FIntVector, int32> BatchOfCell;
    TArray<UHierarchicalInstancedStaticMeshComponent*> Batches;
    TArray<int32> BatchKinds;
    TArray<TArray<FTransform>> Transforms;
    const bool bUseCells = CVarEstateSceneryCells.GetValueOnGameThread() != 0;
    auto NewBatch = [this, &Batches, &BatchKinds, &Transforms, &KindMeshes, &KindCellCm](int32 KindIndex)
    {
        const FEstateSceneryKind& Kind = EstateSceneryKinds[KindIndex];
        auto* Batch = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
        Batch->SetupAttachment(GetRootComponent());
        Batch->SetMobility(EComponentMobility::Static);
        Batch->SetStaticMesh(KindMeshes[KindIndex]);
        Batch->SetCollisionProfileName(Kind.bCollision ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
        Batch->SetCollisionEnabled(Kind.bCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
        Batch->SetCanEverAffectNavigation(false);
        Batch->SetCastShadow(Kind.bTree || Kind.bCollision || Kind.bShadow);
        // As the woodland's underbrush: gentle sway needn't redraw cached shadow pages every frame.
        if (Kind.bShadow && !Kind.bTree) Batch->ShadowCacheInvalidationBehavior = EShadowCacheInvalidationBehavior::Rigid;
        if (Kind.CullCm > 0)
        {
            Batch->SetCullDistances(static_cast<int32>(Kind.CullCm * 0.8f), static_cast<int32>(Kind.CullCm));
            // A cell whose nearest instance is past the cull distance draws nothing: drop it whole. The
            // distance is measured to the cell's centre, so allow a full cell for its reach.
            if (KindCellCm[KindIndex] > 0)
            {
                Batch->LDMaxDrawDistance = Kind.CullCm + KindCellCm[KindIndex];
                Batch->SetCachedMaxDrawDistance(Batch->LDMaxDrawDistance);
            }
        }
        // Wind sway only near her: beyond 60 m it's invisible, and animated Nanite foliage there would
        // keep invalidating the cached virtual shadow maps of the whole wood every frame.
        Batch->SetWorldPositionOffsetDisableDistance(6000);
        Batch->ComponentTags.Add(TEXT("EstateScenery"));
        ApplyCameraSafeFoliageMaterials(*Batch);
        TagSwayingShrub(*Batch);
        Batches.Add(Batch);
        BatchKinds.Add(KindIndex);
        Transforms.AddDefaulted();
        return Batches.Num() - 1;
    };
    for (uint32 Index = 0; Index < Count; ++Index)
    {
        const FEstateSceneryRecord& Record = Records[Index];
        if (Record.Kind >= KindCount) continue;
        const FEstateSceneryKind& Kind = EstateSceneryKinds[Record.Kind];
        if (!KindTried[Record.Kind])
        {
            KindTried[Record.Kind] = true;
            KindMeshes[Record.Kind] = LoadObject<UStaticMesh>(nullptr, Kind.Path);
            if (!KindMeshes[Record.Kind])
            {
                UE_LOG(LogHomesteadWorld, Warning, TEXT("Estate scenery mesh missing: %s"), Kind.Path);
            }
            else if (bUseCells && !KindMeshes[Record.Kind]->HasValidNaniteData())
            {
                KindCellCm[Record.Kind] = Kind.CullCm > 0 ? EstateSceneryNearCellCm : EstateSceneryFarCellCm;
            }
        }
        UStaticMesh* Mesh = KindMeshes[Record.Kind];
        if (!Mesh) continue;
        const float CellCm = KindCellCm[Record.Kind];
        const FIntVector Cell(Record.Kind,
            CellCm > 0 ? FMath::FloorToInt32(Record.X / CellCm) : 0, CellCm > 0 ? FMath::FloorToInt32(Record.Y / CellCm) : 0);
        const int32* Found = BatchOfCell.Find(Cell);
        const int32 BatchIndex = Found ? *Found : BatchOfCell.Add(Cell, NewBatch(Record.Kind));
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
        Transforms[BatchIndex].Add(FTransform(Rotation, Base - Rotation.RotateVector(Anchor * Record.Scale), FVector(Record.Scale)));
    }
    int32 Total = 0, Cells = 0;
    for (int32 BatchIndex = 0; BatchIndex < Batches.Num(); ++BatchIndex)
    {
        UHierarchicalInstancedStaticMeshComponent* Batch = Batches[BatchIndex];
        const int32 Kind = BatchKinds[BatchIndex];
        Batch->RegisterComponent();
        Batch->AddInstances(Transforms[BatchIndex], false, true);
        EstateScenery.Add(Batch);
        const FEstateSceneryKind& Info = EstateSceneryKinds[Kind];
        const FVector Extent = Batch->GetStaticMesh()->GetBounds().BoxExtent;
        EstateSceneryClearRadius.Add(Info.bTree || Info.bCollision ? 0.0f : FMath::Max(Extent.X, Extent.Y) * 0.7f);
        EstateSceneryTrunkRadius.Add(Info.bTree ? Info.Footprint : 0.0f);
        EstateSceneryHidden.Add(TBitArray<>(false, Transforms[BatchIndex].Num()));
        EstateSceneryTransforms.Add(MoveTemp(Transforms[BatchIndex]));
        Total += EstateSceneryTransforms.Last().Num();
        Cells += KindCellCm[Kind] > 0 ? 1 : 0;
    }
    bEstateSceneryBuilt = true;
    UE_LOG(LogHomesteadWorld, Display, TEXT("Estate scenery: %d instances in %d batches (%d of them non-Nanite cells)."),
        Total, EstateScenery.Num(), Cells);
    return true;
}

void AHomesteadWorld::ClearEstateSceneryUnderPieces(const Homestead::State& State)
{
    // The pieces' footprints and the interactables' positions, to the centimetre (yaw to 0.1 degree).
    const auto Round = [](double Value) { return static_cast<uint64>(FMath::RoundToInt64(Value)); };
    uint64 Key = HomesteadWorldKeys::Seed;
    for (const auto& Structure : State.structures)
    {
        const auto Box = Homestead::StructureFootprint(State, Structure);
        Key = HomesteadWorldKeys::Mix(Key, Round(Box.center.x));
        Key = HomesteadWorldKeys::Mix(Key, Round(Box.center.y));
        Key = HomesteadWorldKeys::Mix(Key, Round(Box.half.x));
        Key = HomesteadWorldKeys::Mix(Key, Round(Box.half.y));
        Key = HomesteadWorldKeys::Mix(Key, Round(Box.yaw * 10.0));
    }
    Key = HomesteadWorldKeys::Mix(Key, State.structures.size());
    for (const auto& Node : State.resources)
    {
        Key = HomesteadWorldKeys::Mix(Key, static_cast<uint64>(Node.id));
        Key = HomesteadWorldKeys::Mix(Key, Round(Node.position.x));
        Key = HomesteadWorldKeys::Mix(Key, Round(Node.position.y));
    }
    if (bEstateSceneryClearKnown && Key == EstateSceneryClearKey) return;
    bEstateSceneryClearKnown = true;
    EstateSceneryClearKey = Key;
    TArray<Homestead::Footprint> Pieces;
    for (const auto& Structure : State.structures)
        Pieces.Add(Homestead::StructureFootprint(State, Structure));
    // Interactables, bucketed by 20 m cell; the cover or trunk reach never spans more than a cell.
    constexpr double NodeCell = 2000.0;
    TMap<FIntPoint, TArray<FVector2D>> Nodes;
    for (const auto& Node : State.resources)
    {
        Nodes.FindOrAdd(FIntPoint(FMath::FloorToInt32(Node.position.x / NodeCell), FMath::FloorToInt32(Node.position.y / NodeCell)))
            .Add(FVector2D(Node.position.x, Node.position.y));
    }
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
