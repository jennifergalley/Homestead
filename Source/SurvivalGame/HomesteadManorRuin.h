#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HomesteadManorRuin.generated.h"

class UStaticMeshComponent;

// add-ruined-manor-and-arrival: the ruined manor's set dressing on ManorFootprint. It lays the
// Blender ruin kit (broken granite wall runs, the chimney stack) and granite rubble out on the
// footprint's rectangle, leaving the standing room's carved-out corner to the heritage pieces. It
// is scenery only; Simulation reserves the footprint. Placed once in the Estate level (folder
// "Manor"); it rebuilds from the layout whenever it is constructed, so moving the anchor moves
// the ruin.
UCLASS()
class SURVIVALGAME_API AHomesteadManorRuin : public AActor
{
    GENERATED_BODY()
public:
    AHomesteadManorRuin();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    // Lays the set out again from the current layout; returns the number of pieces placed.
    int32 Rebuild();

private:
    UPROPERTY(Transient)
    TArray<TObjectPtr<UStaticMeshComponent>> Pieces;
    UPROPERTY(Transient)
    TMap<FName, TObjectPtr<UStaticMesh>> Meshes;
    UStaticMesh* Mesh(const TCHAR* Name);
};
