#pragma once

#include "CoreMinimal.h"
#include "Async/Future.h"
#include "GameFramework/Actor.h"
#include "SceneTypes.h"
#include "Simulation/HomesteadCrops.h"
#include "Simulation/HomesteadRegionalDescriptorCache.h"
#include "Simulation/HomesteadSimulation.h"
#include "Simulation/HomesteadWorldGeneration.h"
#include <map>
#include <vector>

#include "HomesteadWorld.generated.h"

class UStaticMesh;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UMeshComponent;
class UCapsuleComponent;
class UStaticMeshComponent;
class UHierarchicalInstancedStaticMeshComponent;
class UProceduralMeshComponent;
class USceneComponent;
class UDirectionalLightComponent;
class USkyLightComponent;
class UExponentialHeightFogComponent;
class UPostProcessComponent;
class AHomesteadRoadEndGate;

USTRUCT()
struct FHomesteadWorldVisual
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<TObjectPtr<USceneComponent>> Components;

    FString Signature;
    // Resource visuals: whether this was built showing the node cleared (for the clear-pop).
    bool bShownCleared = false;
};

USTRUCT()
struct FHomesteadTerrainChunk
{
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UProceduralMeshComponent> Terrain;

    // Creek surface over this tile, when the stream crosses it. Single Layer Water, shadowless.
    UPROPERTY()
    TObjectPtr<UProceduralMeshComponent> Water;
    UPROPERTY()
    FHomesteadWorldVisual Cover;

    FString CoverSignature;
    bool bCollision = false;
};

struct FHomesteadOuterTreeInstance
{
    FString MeshPath;
    FTransform Transform;
    int32 PaletteRole = 0;
    uint32 VariantIndex = 0;
};

struct FHomesteadActiveTreeInstance
{
    FHomesteadOuterTreeInstance Visual;
    FTransform CollisionTransform;
    float CapsuleRadius = 0;
    float CapsuleHalfHeight = 0;
    int32 ResourceId = 0;
};

struct FHomesteadRegionalDescriptorBuildEntry
{
    Homestead::RegionalGeneration::RegionCoord Region;
    Homestead::RegionalGeneration::Status ResultStatus =
        Homestead::RegionalGeneration::Status::OutOfRange;
    Homestead::RegionalGeneration::RegionalResult Result;
};

struct FHomesteadRegionalDescriptorBuild
{
    Homestead::Generation::WorldDescriptor World;
    std::vector<FHomesteadRegionalDescriptorBuildEntry> Entries;
};

struct FHomesteadChunkBaselineBuild
{
    Homestead::Generation::WorldDescriptor World;
    std::vector<Homestead::Generation::ChunkBaseline> Chunks;
};

// One deterministic woodland underbrush plant. Index is stable per chunk for a given world
// seed, so a cleared plant can be recorded by (chunk, index).
struct FHomesteadUnderbrush
{
    uint8 Species = 0;
    int32 Index = 0;
    float X = 0;
    float Y = 0;
    float Yaw = 0;
    float Scale = 1;
};

// One Sierra granite rock: small ground clusters, knee-high boulders, erratics, and rare
// house-sized domes and split boulders grouped into outcrops.
struct FHomesteadRock
{
    uint8 Kind = 0;
    float X = 0;
    float Y = 0;
    float Yaw = 0;
    float Scale = 1;
    float Pitch = 0;
    float Roll = 0;
};

UCLASS()
class SURVIVALGAME_API AHomesteadWorld : public AActor
{
    GENERATED_BODY()

public:
    AHomesteadWorld();
    // Creek surface mesh: columns across the stream, half its span either side of the centreline,
    // and how far it sits under the generator's water level (a shallower, clearer brook).
    static constexpr int CreekSurfaceColumns = 24;
    static constexpr double CreekSurfaceHalfSpanCm = 240.0;
    static constexpr double CreekSurfaceDropCm = 6.0;
    virtual void Tick(float DeltaSeconds) override;
    bool Initialize(const Homestead::Simulation& Simulation);
    bool Refresh(const Homestead::Simulation& Simulation);
    // Keep one gathered resource's produce visible (as if still ready) until released, so the
    // pile stays on the ground until the heroine's animation lifts it. One at a time.
    void HoldProduce(int32 Id) { HeldProduceId = Id; }
    void ReleaseProduce() { HeldProduceId = INDEX_NONE; }
    // Keeps a just-planted square looking bare (no seed mound yet) while she plants it.
    // bHidden hides the square entirely (just tilled, before the hoe bites).
    void HoldPlot(int32 Id, bool bHideSquare = false) { HeldPlotId = Id; bHeldPlotHidden = bHideSquare; }
    void ReleasePlot() { HeldPlotId = INDEX_NONE; bHeldPlotHidden = false; }
    // Keeps a just-harvested plot showing its ripe plant until her hands lift the produce.
    void HoldHarvest(int32 Id, Homestead::CropKind Kind) { HeldHarvestPlotId = Id; HeldHarvestKind = Kind; }
    void ReleaseHarvest() { HeldHarvestPlotId = INDEX_NONE; }
    // The plant or produce mesh for a crop stage ("Sprout" ... "Ripe", "Harvest"), or null until imported.
    UStaticMesh* CropMesh(Homestead::CropKind Kind, const TCHAR* Stage);
    // The crop's produce (SM_Crop<Name>_Produce) at each of the stage's anchors on the plant, sized
    // and coloured by growth: roots push up out of the soil, fruit and pods swell and colour up.
    void AddCropProduce(FHomesteadWorldVisual& Visual, const Homestead::Plot& Plot, Homestead::CropStage CropStage,
        const FTransform& PlantTransform);
    // Hide one component of the held produce (a stick she has already lifted from the pile).
    void HideHeldProducePart(int32 Index);
    // The first mesh a resource's visual draws (what she pulls a handful of), or null.
    UStaticMesh* ResourceVisualMesh(int32 Id) const;
    // Presentation only, while she pulls weeds by hand: shrink one resource's visual to Fraction of
    // its size (the first fistful is out) until RestoreThinnedResource puts it back (a cancelled pull)
    // or ForgetThinnedResource lets the refresh remove it (the pull committed). One at a time; safe
    // to call every tick.
    void ThinResource(int32 Id, float Fraction);
    void RestoreThinnedResource();
    void ForgetThinnedResource();
    // Felling: call right after tree or sapling ResourceId is cleared. A standing copy stays up
    // (the rebuilt woodland no longer draws it) until DropFelledTree topples it away from AwayFrom;
    // it lies a few seconds, then sinks away. One at a time; a new felling finishes the last.
    bool BeginFelling(int32 ResourceId);
    void DropFelledTree(FVector2D AwayFrom);
    bool IsFelledTreeStanding() const { return FallingParts.Num() > 0 && !bTreeFalling; }
    // True once per felled tree, when it hits the ground (for the thud).
    bool TakeFelledTreeLanding(FVector& Where);
    // Where an axe meets active tree ResourceId at waist height: the trunk's centre and radius
    // (cm). False when it isn't a standing mature tree.
    bool TreeChopTarget(int32 ResourceId, FVector2D& Centre, float& Radius) const;
    // The estate's decorative trees (HomesteadWorldSceneryTrees.cpp, openspec chop-any-tree). The standing,
    // unprotected tree nearest her within Reach whose trunk is in front of her (or within a pace); its
    // trunk position is the simulation's key. Radius is the trunk's chop radius, cm.
    bool FindSceneryTreeNear(const Homestead::Simulation& Simulation, FVector2D From, FVector2D Facing, float Reach,
        FVector2D& Trunk, float& Radius) const;
    // The same for a near-home stump the axe can clear to bare ground (Simulation::ClearSceneryStump).
    bool FindSceneryStumpNear(const Homestead::Simulation& Simulation, FVector2D From, FVector2D Facing, float Reach,
        FVector2D& Trunk, float& Radius) const;
    // Call right after Simulation::FellSceneryTree: lifts a copy of the tree out of its batch to topple
    // with DropFelledTree (the same machinery as a woodland tree) and brings the stump forward.
    bool BeginFellingScenery(const Homestead::Simulation& Simulation, FVector2D Trunk);
    // Brings the stumps and saplings, and the hidden standing trees, in step with State::felledTrees.
    // Cheap to call every refresh: it only works when the felled set or the hour of the next stage changes.
    void UpdateSceneryTreeStages(const Homestead::Simulation& Simulation, bool bForce = false);
    void SetPlacementPreview(bool Visible, const Homestead::PlacementTarget& Target, bool bValid);
    void SetDeconstructPreview(const Homestead::State& State, int32 StructureId, bool bValid);
    // A thin ground outline round one garden square (HomesteadWorldGardenOutline.cpp): green when the hoe or
    // pail would work there, red when not. Hidden when bVisible is false.
    void SetGardenOutline(bool bVisible, int32 CellX, int32 CellY, bool bValid);
    static float GroundHeight(float X, float Y, Homestead::Generation::WorldDescriptor World);
    float GroundHeight(float X, float Y) const;
    bool IsPreparedFor(const Homestead::State& State) const;
    int32 StartingViewObstructions(FVector Focus, FVector Camera) const;
    const Homestead::Generation::ChunkBaseline* CachedBaselineFor(
        Homestead::Generation::WorldDescriptor World, Homestead::Generation::ChunkCoord Chunk) const;
    bool StageAdjacentResources(const Homestead::Simulation& Destination, uint64 SourceRevision);
    void CancelStagedResources();
    // Underbrush layout for one 24 m chunk, before reservations (resources, structures, the
    // starting clearing) are applied. Density follows low-frequency noise: open meadows,
    // scattered shrubs, and thickets of blocking brambles and hedges.
    static void GenerateUnderbrush(uint64 WorldSeed, FIntPoint Chunk, TArray<FHomesteadUnderbrush>& Out);
    static float UnderbrushDensity(float X, float Y);
    static bool IsUnderbrushBlocking(uint8 Species);
    static float UnderbrushRadius(uint8 Species);
    // Granite layout for one chunk. IsFree(X, Y, Radius, Kind) applies reservations so outcrop
    // anchors can retry elsewhere; the result is deterministic for a seed and reservation set.
    // KnobSite, when set, is the generator's house-sized granite site (Homestead::Generation::GraniteKnob).
    static void GenerateRocks(uint64 WorldSeed, FIntPoint Chunk, const FVector2D* KnobSite,
        TFunctionRef<bool(float, float, float, uint8)> IsFree, TArray<FHomesteadRock>& Out);
    static float RockRadius(uint8 Kind);
    // Placed underbrush (after reservations, rocks and machete clearing) nearest a point, for the
    // machete. Low groundcover (strawberry, yarrow) isn't offered.
    struct FUnderbrushTarget
    {
        FIntPoint Chunk = FIntPoint::ZeroValue;
        int32 Index = INDEX_NONE;
        uint8 Species = 0;
        FVector2D Position = FVector2D::ZeroVector;
        float Radius = 0;
        bool bWoody = false;
    };
    bool FindUnderbrushNear(const Homestead::Simulation& Simulation, FVector2D Point, float Reach, FUnderbrushTarget& Out) const;
    static FString UnderbrushName(uint8 Species);
    // 0 outdoors .. 1 indoors at the camera (UHomesteadWeather), for the ambience and hearth mixes.
    float GetIndoorMix() const;
    // Her own position inside a roofed building cell, eased (UHomesteadWeather::GetRoomMix): the interior
    // daylight follows this, not the camera, so doorways and overhangs outside don't dim the day.
    float GetRoomMix() const;

private:
    friend class AHomesteadVisualPlaytest;
    friend class AHomesteadSmokeTest;
    friend class AHomesteadGardenProbe;
    static constexpr int32 ActiveMatureTreeMinLOD = 1;
    // The tree being felled or falling, as world-space parts pivoting about its base.
    UPROPERTY() TArray<TObjectPtr<USceneComponent>> FallingParts;
    TArray<FTransform> FallingRest;
    FVector FallPivot = FVector::ZeroVector, FallAxis = FVector::ZeroVector;
    float FallHeight = 1000, FallAngle = 0, FallRate = 0, FallLying = 0;
    int32 FallBounces = 0;
    bool bTreeFalling = false, bTreeLanded = false, bLandingPending = false;
    void UpdateFallingTree(float DeltaSeconds);
    void FinishFallingTree();
    static constexpr int32 OuterMatureTreeMinLOD = 2;

    UPROPERTY()
    TObjectPtr<UStaticMesh> Cube;
    UPROPERTY()
    TObjectPtr<UStaticMesh> Sphere;
    UPROPERTY()
    TObjectPtr<UStaticMesh> Cylinder;
    UPROPERTY()
    TObjectPtr<UStaticMesh> Cone;
    UPROPERTY()
    TObjectPtr<UStaticMesh> ImportedRock;
    // Garden props, loaded on first use (they can be imported while the editor runs).
    UPROPERTY()
    TObjectPtr<UStaticMesh> TilledBedMesh;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> TilledBedWetMaterial;
    UPROPERTY()
    TObjectPtr<UStaticMesh> SoilMoundMesh;
    // Crop plants per stage (SM_Crop<Name>_<Stage>), loaded on first use; misses are cached as null.
    UPROPERTY()
    TMap<FName, TObjectPtr<UStaticMesh>> CropMeshes;
    UPROPERTY()
    TObjectPtr<UStaticMesh> WeedTuftMesh;
    // The manor's granite kit and hearth (StoneFoundation, StoneWall, StoneDoorway, StoneRoof,
    // StoneHearth), loaded on first use by name.
    UPROPERTY()
    TMap<FName, TObjectPtr<UStaticMesh>> ManorMeshes;
    UPROPERTY()
    TObjectPtr<class USoundWave> HearthCrackle;
    UStaticMesh* ManorMesh(const TCHAR* Name);
    // Hearth firelight, flickered every tick.
    TArray<TWeakObjectPtr<class UPointLightComponent>> HearthLights;
    float HearthFlickerTime = 0.0f;
    void UpdateHearthFlicker(float DeltaSeconds);
    // A cleared obstacle swells a touch and shrinks away while chips of it (leaf, wood or stone) fly
    // out (add-coral-island-clearout). Cosmetic only: the components are the actor's until destroyed.
    struct FClearPop
    {
        TArray<TWeakObjectPtr<USceneComponent>> Parts;
        TArray<FVector> Scales;
        TArray<FVector> Locations;
        TArray<FVector> Velocities; // Chips only.
        TArray<FRotator> Spins;
        float Age = 0.0f;
        float Life = 0.4f;
        bool bChips = false;
    };
    TArray<FClearPop> ClearPops;
    void StartClearPop(FHomesteadWorldVisual& Visual, const Homestead::ResourceNode& Node);
    void UpdateClearPops(float DeltaSeconds);
    // A hearth's crackle is heard only with a clear line from the listener to the fire: walls between
    // them (even the room's own wall with her standing just outside it) silence it.
    struct FHearthSound
    {
        TWeakObjectPtr<class UAudioComponent> Audio;
        FVector Mouth = FVector::ZeroVector;
        FVector Chimney = FVector::ZeroVector;
        TArray<TWeakObjectPtr<UPrimitiveComponent>> Ignored;
        float Gate = -1.0f;
    };
    // Overcast (add-rain-weather): the sun's share and the sky light's lift under full cloud, the sun's
    // disc widened so what shadows remain are soft, the exposure held down (EV) and the colour taken out.
    static constexpr float OvercastSunScale = 0.12f;
    static constexpr float OvercastSkyScale = 1.3f;
    static constexpr float OvercastSunSourceAngle = 12.0f;
    static constexpr float OvercastExposureBias = -0.8f;
    static constexpr float OvercastSaturation = 0.72f;
    // Under a roof by day, eye adaptation would lift a shaded room back to outdoor brightness. Hold it
    // down (EV, scaled by the room mix and daylight) so the room reads as dim, with daylight at the
    // door and the hearth as the key; at night the lamp and hearth already set the level.
    static constexpr float IndoorDayExposureBias = -0.7f;
    // The hearth as a settled low fire: a warm key low in front of the opening, oil-lamp strength
    // (HomesteadLampLook's lamp is 1400 / 1000 cm), rather than a floodlight filling the room.
    static constexpr float HearthIntensity = 2600.0f;
    static constexpr float HearthRadiusCm = 800.0f;
    // At night (Jenny, 2026-09-30: rain at any hour) full cloud hides most of the moon, but the night sky
    // light keeps its floor and the exposure isn't pushed below the night's own floor, so a rainy night is
    // darker than a clear one without going black; the lamp and the hearth carry it.
    static constexpr float OvercastMoonScale = 0.35f;
    TArray<FHearthSound> HearthSounds;
    void UpdateHearthSound(float DeltaSeconds);
    // The standing room's door (HomesteadWorldDoors.cpp, Simulation/HomesteadDoor): an oak leaf on a hinge
    // pivot in the heritage stone doorway, swung open as she nears and shut behind her. Keyed by the
    // doorway's structure id so a rebuild keeps its swing; presentation only, nothing saved.
    struct FDoorLeaf
    {
        TWeakObjectPtr<USceneComponent> Hinge;
        FVector Opening = FVector::ZeroVector;
        float ClosedYaw = 0.0f;
        float Openness = 0.0f;
        bool bWanted = false;
    };
    TMap<int32, FDoorLeaf> DoorLeaves;
    void AddDoorLeaf(FHomesteadWorldVisual& Visual, int32 StructureId, const FVector& Base, const FRotator& Rotation,
        float HeightScale);
    void UpdateDoors(float DeltaSeconds);
    UPROPERTY()
    TObjectPtr<UMaterialInterface> FieldMaterial;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> CreekWaterMaterial;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> GroundMaterial;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> RockMaterial;
    UPROPERTY()
    TMap<FName, TObjectPtr<UMaterialInstanceDynamic>> Materials;
    UPROPERTY()
    TMap<FName, TObjectPtr<UMaterialInterface>> CameraSafeFoliageMaterials;
    UPROPERTY()
    TMap<FName, TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> DecorationBatches;
    UPROPERTY()
    TObjectPtr<UProceduralMeshComponent> Ground;
    UPROPERTY()
    TMap<FIntPoint, FHomesteadTerrainChunk> TerrainChunks;
    UPROPERTY()
    TMap<FIntPoint, TObjectPtr<UProceduralMeshComponent>> RegionalWaterMeshes;
    UPROPERTY()
    TMap<FString, TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> OuterTreeBatches;
    TMap<FString, FHomesteadOuterTreeInstance> OuterTreeInstances;
    UPROPERTY()
    TMap<FString, TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> ActiveTreeBatches;
    UPROPERTY()
    TMap<FString, TObjectPtr<UCapsuleComponent>> ActiveTreeCollisions;
    TMap<FString, FHomesteadActiveTreeInstance> ActiveTreeInstances;
    UPROPERTY()
    TObjectPtr<UDirectionalLightComponent> Sun;
    UPROPERTY()
    TObjectPtr<UDirectionalLightComponent> Moon;
    // Last rotations pushed to the lights. Rotating a directional light invalidates every cached
    // Virtual Shadow Map page, so small time-of-day changes are applied in coarse steps.
    FRotator AppliedSunRotation = FRotator::ZeroRotator;
    FRotator AppliedMoonRotation = FRotator::ZeroRotator;
    bool bLightRotationApplied = false;
    // True while showing a fixed-estate game on the Estate level's Landscape.
    bool bFixedEstate = false;
    // Baked decorative trees, shrubs and rocks for the Estate map (Content/SurvivalGame/Estate/Runtime).
    bool BuildEstateScenery();
    bool bEstateSceneryBuilt = false;
    // The road bridge over the river (HomesteadWorldRoadBridge.cpp), built once with the estate scenery.
    void BuildRoadBridge();
    bool bRoadBridgeBuilt = false;
    UPROPERTY()
    FHomesteadWorldVisual RoadBridgeVisual;
    // The cove route's step kit (HomesteadWorldCoveRoute.cpp), built once with the estate scenery when Props'
    // meshes are imported.
    void BuildCoveRoute();
    bool bCoveRouteBuilt = false;
    UPROPERTY()
    FHomesteadWorldVisual CoveRouteVisual;
    // The village's lanes, well, benches, store clutter and cottage gardens (HomesteadWorldVillage.cpp), built
    // once with the estate scenery from HomesteadVillageDressing.inc.
    void BuildVillage();
    void BuildVillagePaving();
    bool bVillageBuilt = false;
    UPROPERTY()
    FHomesteadWorldVisual VillageVisual;
    // The playable area's edge (HomesteadWorldEdge.cpp): invisible pawn-only walls along the layout's
    // PlayableBounds and the shut field gate at the public road's far end, built once with the estate scenery.
    void BuildWorldEdge();
    bool bWorldEdgeBuilt = false;
    UPROPERTY()
    FHomesteadWorldVisual WorldEdgeVisual;
    UPROPERTY()
    TObjectPtr<AHomesteadRoadEndGate> RoadEndGate;
    UPROPERTY()
    TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> EstateScenery;
    // Hides low cover (bushes, ferns, grass, cobbles) wherever a placed piece now stands, so none
    // pokes through a floor or wall; it comes back if the piece is taken down. Low cover and trees
    // are also cleared off every interactable (berry bush, herb, bramble, salvage), so each one stands
    // in the open where she can see and reach it.
    void ClearEstateSceneryUnderPieces(const Homestead::Simulation& Simulation, uint64 LayoutKey);
    TArray<TArray<FTransform>> EstateSceneryTransforms;
    TArray<float> EstateSceneryClearRadius; // 0 for trees and rocks, which are never hidden under pieces.
    TArray<float> EstateSceneryTrunkRadius; // Trees only: the trunk footprint kept clear of interactables.
    TArray<float> EstateSceneryFlowerRadius; // Full unscaled XY bounds, including the pivot offset; 0 for other kinds.
    TArray<TBitArray<>> EstateSceneryHidden;
    // Choppable estate trees, in 20 m cells (HomesteadWorldSceneryTrees.cpp). Trunk is where the simulation
    // keys it; Batch/Index locate its instance in EstateScenery.
    struct FSceneryTreeRef
    {
        int32 Batch = INDEX_NONE;
        int32 Index = INDEX_NONE;
        float X = 0, Y = 0;
        float TrunkRadius = 0;
    };
    TMap<FIntPoint, TArray<FSceneryTreeRef>> SceneryTreeGrid;
    // Felled trees' instances (Batch << 32 | Index) that stay hidden, ORed into the clear-under-pieces pass.
    TSet<uint64> FelledSceneryInstances;
    UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> ScenerySaplings;
    UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> SceneryStumps;
    uint64 SceneryStageRevision = ~0ull;
    uint64 SceneryStageSignature = 0;
    double SceneryStageNextHour = 0;
    const FSceneryTreeRef* FindSceneryTreeAt(FVector2D Trunk) const;
    void RegisterSceneryTree(int32 Batch, int32 Index, float X, float Y, float TrunkRadius);
    // The near meadow round the camera on the fixed estate (HomesteadGrassField).
    UPROPERTY() TObjectPtr<class UHomesteadGrassField> EstateGrass;
    // Wetness and Daylight for the estate ground and meadow materials (Scripts/Terrain/build_ground.py).
    UPROPERTY() TObjectPtr<class UMaterialParameterCollection> GroundParameters;
    // Rain, cloud and the rain's sound (HomesteadWeather), fed from UpdateLighting and ticked every frame.
    UPROPERTY() TObjectPtr<class UHomesteadWeather> Weather;
    float AppliedSunSourceAngle = -1.0f;
    bool bGroundParametersTried = false;
    uint64 EstateSceneryClearKey = 0;
    uint64 EstateSceneryInputsKey = 0;
    bool bEstateSceneryClearKnown = false;
    UPROPERTY()
    TObjectPtr<USkyLightComponent> Sky;
    UPROPERTY()
    TObjectPtr<UExponentialHeightFogComponent> Fog;
    UPROPERTY()
    TObjectPtr<UPostProcessComponent> Exposure;
    UPROPERTY()
    TMap<int32, FHomesteadWorldVisual> ResourceVisuals;
    UPROPERTY()
    TMap<int32, FHomesteadWorldVisual> ResourceProduceVisuals;
    UPROPERTY()
    TMap<int32, FHomesteadWorldVisual> StagedResourceVisuals;
    UPROPERTY()
    TMap<int32, FHomesteadWorldVisual> StagedResourceProduceVisuals;
    UPROPERTY()
    TMap<FIntPoint, FHomesteadTerrainChunk> StagedCoverChunks;
    // Underbrush actually placed per chunk at its last cover build (for FindUnderbrushNear).
    TMap<FIntPoint, TArray<FHomesteadUnderbrush>> PlacedUnderbrush;
    std::vector<Homestead::ResourceNode> StagedResourceNodes;
    TArray<FIntPoint> StagedCoverKeys;
    Homestead::Generation::WorldDescriptor StagedWorld;
    Homestead::Generation::ChunkCoord StagedChunk;
    uint64 StagedSourceRevision = 0;
    int32 StagedResourceCursor = 0;
    int32 StagedCoverCursor = 0;
    bool bStagingResourceWindow = false;
    bool bStagingResourceBuild = false;
    UPROPERTY()
    TMap<int32, FHomesteadWorldVisual> StructureVisuals;
    // Cells holding a foundation; furniture elsewhere rests on the bare ground instead of floor height.
    TSet<FIntVector> FoundationCells; // (cellX, cellY, buildingId)
    UPROPERTY()
    TMap<int32, FHomesteadWorldVisual> PlotVisuals;
    UPROPERTY()
    TMap<int32, FHomesteadWorldVisual> DropVisuals;
    UPROPERTY()
    FHomesteadWorldVisual Preview;
    UPROPERTY()
    FHomesteadWorldVisual GardenOutline;

    bool bInitialized = false;
    FString ResourceLayoutSignature;
    int32 HeldProduceId = INDEX_NONE;
    int32 ThinnedResourceId = INDEX_NONE;
    TArray<TWeakObjectPtr<USceneComponent>> ThinnedComponents;
    TArray<FVector> ThinnedScales;
    int32 HeldPlotId = INDEX_NONE;
    bool bHeldPlotHidden = false;
    int32 HeldHarvestPlotId = INDEX_NONE;
    Homestead::CropKind HeldHarvestKind = Homestead::CropKind::Roots;
    // Refresh rebuilds visuals only when something they're built from changed: a hash of every input
    // the per-object signatures read, including the time-driven ones (produce readiness, plot stages,
    // fire fuel), which change without a simulation revision. The simulation revision is mixed in too.
    uint64 RefreshInputsKey(const Homestead::Simulation& Simulation) const;
    void UpdateEstateGrass(const Homestead::State& State);
    uint64 LastRefreshInputs = 0;
    bool bRefreshInputsKnown = false;
    FString OuterTreeLayoutSignature;
    FString ActiveTreeLayoutSignature;
    FString RegionalWaterSignature;
    FString RenderedRegionalReachKey;
    int32 RenderedRegionalReachReferences = 0;
    int32 UnrenderedRegionalReachReferences = 0;
    int32 UnrenderedRegionalLakeReferences = 0;
    double DecorationBuildMilliseconds = 0;
    Homestead::Generation::WorldDescriptor Descriptor;
    Homestead::Generation::LoadedRegionalDescriptorCache RegionalDescriptors;
    std::vector<Homestead::RegionalGeneration::RegionCoord> DesiredRegionalDescriptors;
    std::map<Homestead::RegionalGeneration::RegionCoord,
        Homestead::RegionalGeneration::Status> RegionalDescriptorFailures;
    TUniquePtr<TFuture<FHomesteadRegionalDescriptorBuild>> RegionalDescriptorBuild;
    TUniquePtr<TFuture<FHomesteadChunkBaselineBuild>> ChunkBaselineBuild;
    std::map<Homestead::Generation::ChunkCoord, Homestead::Generation::ChunkBaseline> ChunkBaselineCache;
    uint64 RegionalDescriptorBuildCount = 0;
    uint64 ChunkBaselineBuildCount = 0;
    uint64 ChunkBaselineCacheHits = 0;
    uint64 ChunkBaselineCacheMisses = 0;
    double LastTerrainPrepareMilliseconds = 0;
    double LastCoverPrepareMilliseconds = 0;
    double LastOuterTreePrepareMilliseconds = 0;
    double LastActiveTreePrepareMilliseconds = 0;
    double LastRefreshMilliseconds = 0;
    double LastTransitionRefreshMilliseconds = 0;
    double LastTransitionTerrainMilliseconds = 0;
    double LastTransitionCoverMilliseconds = 0;
    double LastTransitionOuterTreeMilliseconds = 0;
    double LastTransitionActiveTreeMilliseconds = 0;
    int32 LastTransitionStagedResourceVisuals = 0;
    int32 LastTransitionStagedCoverChunks = 0;
    double LastTransitionStagingFrameMilliseconds = 0;
    double StagingFrameMaximumMilliseconds = 0;
    FString TerrainChunkProfile;
    FString TerrainProfile;
    FString CoverProfile;
    FString OuterTreeProfile;
    FString ActiveTreeProfile;
    FString LastTransitionPublishingProfile;
    Homestead::Generation::ChunkCoord PreparedChunk;
    bool bTerrainReady = false;
    bool bVisualBuildFailed = false;

    UMaterialInterface* Material(FLinearColor Color, float Roughness = 0.85f, float Glow = 0.0f);
    UStaticMeshComponent* AddPart(FHomesteadWorldVisual& Visual, UStaticMesh* Mesh,
        const FVector& Position, const FVector& Size, FLinearColor Color,
        bool bCollision = false, const FRotator& Rotation = FRotator::ZeroRotator,
        float Roughness = 0.85f, float Glow = 0.0f);
    void AddDecoration(UStaticMesh* Mesh, const FVector& Position, const FVector& Size,
        FLinearColor Color, bool bCollision = false,
        const FRotator& Rotation = FRotator::ZeroRotator, bool bHideMesh = false);
    bool BuildTerrain(const Homestead::State& State);
    bool GetChunkBaseline(Homestead::Generation::WorldDescriptor World,
        Homestead::Generation::ChunkCoord Chunk, Homestead::Generation::ChunkBaseline& Baseline);
    void QueueChunkBaselineBuild(Homestead::Generation::WorldDescriptor World,
        Homestead::Generation::ChunkCoord Center);
    bool RefreshRegionalDescriptors(Homestead::Generation::WorldDescriptor World,
        const std::vector<Homestead::RegionalGeneration::RegionCoord>& Regions);
    bool RebuildRegionalWater();
    void ClearRegionalWater();
    void QueueRegionalDescriptorBuild();
    UProceduralMeshComponent* BuildTerrainChunk(const Homestead::Generation::ChunkBaseline& Baseline,
        Homestead::Generation::WorldDescriptor World, bool bCollision, TObjectPtr<UProceduralMeshComponent>& OutWater);
    FVector AtGround(float X, float Y, float Offset = 0) const;
    float CachedGroundHeight(float X, float Y) const;
    void BuildLighting();
    bool LoadCameraSafeFoliageMaterials();
    bool ApplyCameraSafeFoliageMaterials(UMeshComponent& Component);
    // Foliage shadow motion (HomesteadWorldFoliageMotion.cpp): the swaying shrubs' wind, shadows and the
    // camera-safe dither, each behind a console variable for A/B; re-applied only when one changes.
    void TagSwayingShrub(UMeshComponent& Component);
    void UpdateFoliageMotion();
    // Foliage shadows from the rest pose (reduce-foliage-shadow-motion; homestead.FoliageShadowSway): the
    // leaves still sway on screen, but their ray-traced and virtual shadows don't follow the sway, so the
    // dappled shade under the canopy holds still as she walks. Called for every foliage component it's
    // given (ApplyCameraSafeFoliageMaterials), and remembered so the console variable can A/B it live.
    void CalmFoliageShadow(UMeshComponent& Component);
    void ApplyFoliageShadowSway(UPrimitiveComponent& Component, EShadowCacheInvalidationBehavior Authored, bool bSway);
    TArray<TPair<TWeakObjectPtr<UPrimitiveComponent>, EShadowCacheInvalidationBehavior>> CalmShadowFoliage;
    int32 AppliedFoliageShadowSway = -1;
    UPROPERTY()
    TMap<TObjectPtr<UMaterialInterface>, TObjectPtr<UMaterialInstanceDynamic>> ShrubWindMaterials;
    TMap<TWeakObjectPtr<UMaterialInstanceDynamic>, FVector2f> ShrubWindBase;   // authored WindStrength, LeafFlutter
    TArray<TWeakObjectPtr<UMeshComponent>> SwayingShrubs;
    UPROPERTY()
    TObjectPtr<class UMaterialParameterCollection> CameraFoliageCollection;
    float AppliedShrubWind = -1.0f;
    int32 AppliedShrubShadows = -1;
    int32 AppliedFoliageDither = -1;
    bool BuildDecorations(const Homestead::Simulation& Simulation,
        const FIntPoint* StageChunk = nullptr);
    void BuildResource(FHomesteadWorldVisual& Visual, const Homestead::ResourceNode& Node, bool bProduceOnly);
    // The estate's overgrowth kinds, each placed through Place(mesh, offset, yaw, produce, scale, pivotOnGround).
    // Soft ground cover drawn right on the soil (HomesteadWorldGrounding.cpp): weeds, nettles and tall
    // grass. SoilHeight is the lowest drawn ground under the clump's footprint (the Landscape's own
    // collision where it's loaded, else the runtime heightfield), never more than a hand's depth below
    // its centre; bOnLandscape is false when the Landscape wasn't there to trace.
    static bool IsSoilGrounded(Homestead::ResourceKind Kind);
    float SoilHeight(FVector2D Centre, FVector2D Half, float Yaw, bool& bOnLandscape) const;
    struct FPendingSoil
    {
        TWeakObjectPtr<USceneComponent> Part;
        FVector2D Centre;
        FVector2D Half;
        float Yaw = 0.0f;
        float PlacedOn = 0.0f; // The ground height it was placed on.
        int32 Tries = 0;
    };
    TArray<FPendingSoil> PendingSoil;
    float SoilRetryTimer = 0.0f;
    int32 SoilRetryCursor = 0;
    void QueueSoilGrounding(USceneComponent* Part, FVector2D Centre, FVector2D Half, float Yaw, float PlacedOn);
    void UpdateSoilGrounding(float DeltaSeconds);
    void BuildOvergrowth(const Homestead::ResourceNode& Node, uint32 Variation,
        const TFunctionRef<void(UStaticMesh*, FVector2D, float, bool, float, bool)>& Place);
    bool ResolveGeneratedTreeVisual(const Homestead::ResourceNode& Node, UStaticMesh*& Mesh,
        FHomesteadOuterTreeInstance& Instance);
    bool RebuildOuterTreeBatches(const Homestead::Simulation& Simulation);
    void ClearOuterTreeBatches();
    bool RebuildActiveTreeBatches(const Homestead::Simulation& Simulation);
    void ClearActiveTreeBatches();
    void BuildStructure(FHomesteadWorldVisual& Visual, const Homestead::Structure& Structure,
        const Homestead::Building& Frame, bool bOnFoundation, bool bPreview, bool bValid = true,
        bool bDeconstruct = false);
    void BuildPlot(FHomesteadWorldVisual& Visual, const Homestead::Plot& Plot);
    void BuildDrop(FHomesteadWorldVisual& Visual, const Homestead::WorldDrop& Drop);
    void BuildOriginalItemDrop(FHomesteadWorldVisual& Visual, const Homestead::WorldDrop& Drop);
    UPROPERTY() TMap<int32, TObjectPtr<UStaticMesh>> OriginalDropMeshes;
    // A set-down oil lamp: the lamp on whatever is underfoot, lit while it has oil (false when the
    // lamp mesh isn't imported, so the generic bundle shows instead).
    bool BuildLampDrop(FHomesteadWorldVisual& Visual, const Homestead::WorldDrop& Drop);
    TWeakObjectPtr<UStaticMeshComponent> LampDropFlame;
    TWeakObjectPtr<UStaticMeshComponent> LampDropGlass;
    TWeakObjectPtr<class UPointLightComponent> LampDropLight;
    bool bLampDropLit = false;
    float LampDropFlickerTime = 0.0f;
    void UpdateLighting(const Homestead::State& State);
    static void ClearVisual(FHomesteadWorldVisual& Visual);
    // Floor height for a turned cell: its highest ground plus a lip.
    float StructureBase(Homestead::Point Center, double Yaw) const;
    static bool IsDecorationReserved(const Homestead::State& State, float X, float Y,
        float FootprintRadius, float CanopyRadius = 0, bool bLowCover = false);
};
