#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HomesteadDerelictFarm.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMesh;

// add-derelict-farm-and-estate-disrepair: the old field behind the manor and the neglected
// grounds. It lays out the rotten post-and-rail fence round Anchor::DerelictFarm (leaning,
// snapped and fallen bays, the broken gate), the ghost crop ridges with dead stalks and bean
// poles, the abandoned plough, and the debris round the ruin and the drive's toppled fence from
// HomesteadEstateDebrisPlacements.inc, all sat on the estate heightfield. It is scenery only: the
// clearable weeds and brambles are Simulation placements (550000+). Placed once in the Estate
// level (folder "Farm"); the parts are transient and rebuilt on construction and at BeginPlay.
UCLASS()
class SURVIVALGAME_API AHomesteadDerelictFarm : public AActor
{
    GENERATED_BODY()
public:
    AHomesteadDerelictFarm();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    // Lays everything out again; returns the number of instances placed.
    int32 Rebuild();

private:
    UPROPERTY(Transient)
    TArray<TObjectPtr<UInstancedStaticMeshComponent>> Parts;
    UPROPERTY(Transient)
    TMap<FName, TObjectPtr<UInstancedStaticMeshComponent>> Batches;
    UPROPERTY(Transient)
    TMap<FName, TObjectPtr<UStaticMesh>> Meshes;
    UStaticMesh* Mesh(const TCHAR* Folder, const TCHAR* Name);
    // The instanced batch for one mesh, with or without collision; null when the mesh isn't imported.
    UInstancedStaticMeshComponent* Batch(const TCHAR* Folder, const TCHAR* Name, bool bBlocks);
    bool Add(const TCHAR* Folder, const TCHAR* Name, const FTransform& World, bool bBlocks);
    float Ground(double X, double Y) const;
    // The ground's up vector at a world XY, from the heightfield over a Span (cm) footprint.
    FVector GroundUp(double X, double Y, double Span) const;
    // A transform on the ground at XY, yawed and tilted to the terrain.
    FTransform OnGround(double X, double Y, float Yaw, float Scale = 1.0f, double Span = 200.0, float Lift = 0.0f) const;
    // A post-and-rail run through Posts (world XY). Condition biases it from ruined (0) to sound (1);
    // Salt keeps runs distinct; the first/last post is skipped when it belongs to the gateway.
    void Fence(const TArray<FVector2D>& Posts, float Condition, int32 Salt, bool bSkipFirst, bool bSkipLast);
    float FallbackZ = 0.0f;
    int32 Placed = 0;
};
