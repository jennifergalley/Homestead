#pragma once

#include "CoreMinimal.h"
#include "Async/Future.h"
#include "GameFramework/Actor.h"
#include "Simulation/HomesteadRegionalDescriptorCache.h"
#include "Simulation/HomesteadSimulation.h"
#include "Simulation/HomesteadWorldGeneration.h"
#include <map>
#include <vector>

#include "HomesteadWorld.generated.h"

class UStaticMesh;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UCapsuleComponent;
class UStaticMeshComponent;
class UHierarchicalInstancedStaticMeshComponent;
class UProceduralMeshComponent;
class USceneComponent;
class UDirectionalLightComponent;
class USkyLightComponent;
class UExponentialHeightFogComponent;
class UPostProcessComponent;

USTRUCT()
struct FHomesteadWorldVisual
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<TObjectPtr<USceneComponent>> Components;

    FString Signature;
};

USTRUCT()
struct FHomesteadTerrainChunk
{
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UProceduralMeshComponent> Terrain;

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

UCLASS()
class SURVIVALGAME_API AHomesteadWorld : public AActor
{
    GENERATED_BODY()

public:
    AHomesteadWorld();
    virtual void Tick(float DeltaSeconds) override;
    bool Initialize(const Homestead::Simulation& Simulation);
    bool Refresh(const Homestead::Simulation& Simulation);
    void SetPlacementPreview(bool Visible, Homestead::Piece Kind, int CellX, int CellY, int Rotation);
    static float GroundHeight(float X, float Y, Homestead::Generation::WorldDescriptor World);
    float GroundHeight(float X, float Y) const;
    bool IsPreparedFor(const Homestead::State& State) const;
    int32 StartingViewObstructions(FVector Focus, FVector Camera) const;

private:
    friend class AHomesteadVisualPlaytest;
    friend class AHomesteadSmokeTest;
    static constexpr int32 ActiveMatureTreeMinLOD = 1;
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
    UPROPERTY()
    TObjectPtr<UMaterialInterface> FieldMaterial;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> GroundMaterial;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> RockMaterial;
    UPROPERTY()
    TMap<FName, TObjectPtr<UMaterialInstanceDynamic>> Materials;
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
    TMap<int32, FHomesteadWorldVisual> StructureVisuals;
    UPROPERTY()
    TMap<int32, FHomesteadWorldVisual> PlotVisuals;
    UPROPERTY()
    FHomesteadWorldVisual Preview;

    bool bInitialized = false;
    FString ResourceLayoutSignature;
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
    uint64 RegionalDescriptorBuildCount = 0;
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
    bool RefreshRegionalDescriptors(Homestead::Generation::WorldDescriptor World,
        const std::vector<Homestead::RegionalGeneration::RegionCoord>& Regions);
    bool RebuildRegionalWater();
    void ClearRegionalWater();
    void QueueRegionalDescriptorBuild();
    UProceduralMeshComponent* BuildTerrainChunk(const Homestead::Generation::ChunkBaseline& Baseline,
        Homestead::Generation::WorldDescriptor World, bool bCollision);
    FVector AtGround(float X, float Y, float Offset = 0) const;
    void BuildLighting();
    bool BuildDecorations(const Homestead::Simulation& Simulation);
    void BuildResource(FHomesteadWorldVisual& Visual, const Homestead::ResourceNode& Node, bool bProduceOnly);
    bool ResolveGeneratedTreeVisual(const Homestead::ResourceNode& Node, UStaticMesh*& Mesh,
        FHomesteadOuterTreeInstance& Instance);
    bool RebuildOuterTreeBatches(const Homestead::Simulation& Simulation);
    void ClearOuterTreeBatches();
    bool RebuildActiveTreeBatches(const Homestead::Simulation& Simulation);
    void ClearActiveTreeBatches();
    void BuildStructure(FHomesteadWorldVisual& Visual, const Homestead::Structure& Structure, bool bPreview);
    void BuildPlot(FHomesteadWorldVisual& Visual, const Homestead::Plot& Plot);
    void UpdateLighting(const Homestead::State& State);
    static void ClearVisual(FHomesteadWorldVisual& Visual);
    float CellBase(int CellX, int CellY) const;
    static float GrassGroundWeight(float X, float Y);
    static bool IsDecorationReserved(const Homestead::State& State, float X, float Y,
        float FootprintRadius, float CanopyRadius = 0, bool bLowCover = false);
};
