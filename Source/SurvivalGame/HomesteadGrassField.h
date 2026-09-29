#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Simulation/HomesteadSimulation.h"

#include "HomesteadGrassField.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;

/**
 * The fixed estate's near meadow: 2 x 2 m patches of grass blades (SM_GrassPatch_LOD0-2) instanced
 * in 6 m chunks around the camera out to about 50 m, wherever EstateGround.bin grows grass.
 *
 * M_EstateGrass does the look: it thins blades by rank with distance and density, sways them in the
 * wind, parts them round the heroine, keeps the road's wheel tracks clear and clears a circle round
 * each of up to three interactables per patch, and keeps garden plots bare (per-instance custom data:
 * x, y, radius cm, three times; a negative radius is a bare square of that half size).
 * The game only picks which patch LOD a chunk uses, never showing fewer blades than the material
 * would draw there, so a chunk changing LOD doesn't pop.
 */
UCLASS()
class SURVIVALGAME_API UHomesteadGrassField : public USceneComponent
{
    GENERATED_BODY()

public:
    // Tie these to M_EstateGrass's GrassFade parameter (x = 12 m, y = 1.7): blades ranked below
    // min(1, (12 / d)^1.7) show at d metres. LOD1 keeps 38 % of the blades, LOD2 14 % (bake_ground.py's
    // LOD_KEEP), so the curve drops under them at 21.2 m and 38.1 m.
    static constexpr float ChunkCm = 600.0f;
    static constexpr float TileCm = 200.0f;
    static constexpr float RadiusCm = 5100.0f;
    static constexpr float Lod1Cm = 2200.0f;
    static constexpr float Lod2Cm = 3900.0f;
    // Chunks are re-picked every Refresh (0.25 s), so pick LODs as if the camera were this much nearer.
    static constexpr float LodMarginCm = 400.0f;
    static constexpr int32 CustomFloats = 9;
    // Bare soil round a tilled garden square, beyond its own half size (M_EstateGrass fades over 18 cm more).
    static constexpr double PlotMarginCm = 15.0;

    /** Keeps the grass round View up to date with the state's interactables and buildings. */
    void Update(const Homestead::State& State, const FVector& View);
    /** Hides and forgets all grass (leaving the fixed estate). */
    void Clear();
    int32 LiveChunks() const { return Live.Num(); }
    int32 LiveInstances() const;

private:
    struct FObstacle
    {
        FVector2f Centre;
        float Radius = 0;
    };
    struct FChunk
    {
        int32 Component = INDEX_NONE;
        int32 Lod = INDEX_NONE;
        uint64 Obstacles = 0; // ChunkObstacleSignature when built.
    };

    bool LoadAssets();
    uint64 LayoutSignature(const Homestead::State& State) const;
    void RebuildObstacles(const Homestead::State& State);
    void BuildChunk(FIntPoint Chunk, FChunk& Out, int32 Lod);
    int32 AcquireComponent();
    void ReleaseChunk(FChunk& Chunk);
    bool IsBlocked(FVector2D Centre) const;
    // What the chunk's patches depend on: the interactables that can reach it and nearby building pieces.
    uint64 ChunkObstacleSignature(FIntPoint Chunk) const;

    UPROPERTY() TArray<TObjectPtr<UStaticMesh>> Meshes;
    UPROPERTY() TObjectPtr<UMaterialInterface> Material;
    UPROPERTY() TArray<TObjectPtr<UInstancedStaticMeshComponent>> Pool;
    TArray<int32> FreeComponents;
    TMap<FIntPoint, FChunk> Live;
    TArray<FObstacle> Obstacles;
    TMultiMap<FIntPoint, int32> ObstacleCells; // Obstacles by ChunkCm cell.
    TArray<Homestead::Footprint> Blocks;
    uint64 Signature = 0;
    bool bAssetsTried = false;
    bool bAssetsReady = false;
};
