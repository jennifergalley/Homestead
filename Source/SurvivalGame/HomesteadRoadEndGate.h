#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HomesteadRoadEndGate.generated.h"

class UBoxComponent;
class UInstancedStaticMeshComponent;
class UPrimitiveComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
struct FHitResult;

// The shut field gate across the public road's far end and the granite milestone beside it ("Marlbury 9 miles";
// shrink-estate-map decision 7). Built from existing kit meshes: the derelict farm's cleft-oak fence posts and
// rails (a five-bar braced leaf between two fence bays) and a cove-route granite kerb stood on end. A pawn-only
// box stops her at the gate; touching it shows Homestead::RoadEndRefusalMessage, at most once per cooldown.
// Spawned by AHomesteadWorld::BuildWorldEdge (HomesteadWorldEdge.cpp). Nothing here is game state.
UCLASS()
class SURVIVALGAME_API AHomesteadRoadEndGate : public AActor
{
    GENERATED_BODY()
public:
    AHomesteadRoadEndGate();
    // Stands the gate across the road at `Centre` (on its centreline), the road heading out along `RoadYaw`.
    void Place(const FVector2D& Centre, float RoadYaw);
    int32 RefusalCount() const { return Refusals; }

private:
    UFUNCTION()
    void OnTouched(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp, int32 BodyIndex,
        bool bFromSweep, const FHitResult& Sweep);
    float GroundAt(const FVector2D& At) const;

    UPROPERTY() TObjectPtr<USceneComponent> Root;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Posts;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Rails;
    UPROPERTY() TObjectPtr<USceneComponent> MilestoneRoot;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Milestone;
    UPROPERTY() TObjectPtr<UTextRenderComponent> MilestoneWords;
    UPROPERTY() TObjectPtr<UBoxComponent> Blocker;
    UPROPERTY() TObjectPtr<UBoxComponent> Touch;
    double LastRefusalSeconds = -1.0;
    int32 Refusals = 0;
};
