#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Simulation/HomesteadSimulation.h"
#include "HomesteadWorld.generated.h"

class UStaticMesh;
class UMaterialInterface;
class UMaterialInstanceDynamic;
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

UCLASS()
class SURVIVALGAME_API AHomesteadWorld : public AActor
{
    GENERATED_BODY()

public:
    AHomesteadWorld();
    void Initialize(const Homestead::State& State);
    void Refresh(const Homestead::State& State);
    void SetPlacementPreview(bool Visible, Homestead::Piece Kind, int CellX, int CellY, int Rotation);
    static float GroundHeight(float X, float Y);

private:
    friend class AHomesteadVisualPlaytest;
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

    UMaterialInterface* Material(FLinearColor Color, float Roughness = 0.85f, float Glow = 0.0f);
    UStaticMeshComponent* AddPart(FHomesteadWorldVisual& Visual, UStaticMesh* Mesh,
        const FVector& Position, const FVector& Size, FLinearColor Color,
        bool bCollision = false, const FRotator& Rotation = FRotator::ZeroRotator,
        float Roughness = 0.85f, float Glow = 0.0f);
    void AddDecoration(UStaticMesh* Mesh, const FVector& Position, const FVector& Size,
        FLinearColor Color, bool bCollision = false,
        const FRotator& Rotation = FRotator::ZeroRotator, bool bHideMesh = false);
    void BuildTerrain();
    void BuildLighting();
    void BuildDecorations(const Homestead::State& State);
    void BuildResource(FHomesteadWorldVisual& Visual, const Homestead::ResourceNode& Node, bool bProduceOnly);
    void BuildStructure(FHomesteadWorldVisual& Visual, const Homestead::Structure& Structure, bool bPreview);
    void BuildPlot(FHomesteadWorldVisual& Visual, const Homestead::Plot& Plot);
    void UpdateLighting(const Homestead::State& State);
    static void ClearVisual(FHomesteadWorldVisual& Visual);
    static float CellBase(int CellX, int CellY);
};
