#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HomesteadRoadSign.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

// A fingerpost on the public road (Simulation/HomesteadEstatePublicRoad.h signs): a solid wooden post whose
// painted arms point the way ("To the village" / "To the manor" / "Village / Manor"). It only points: nothing
// travels from it. The meshes are Blender-built (Scripts/Blender/Recipes/fingerpost_road.py); until they are
// imported it is a clearly labelled stand-in: a post and a painted board.
UCLASS()
class SURVIVALGAME_API AHomesteadRoadSign : public AActor
{
    GENERATED_BODY()
public:
    AHomesteadRoadSign();
    // Stands the sign at `Location` with its upper arm along `Yaw` (Simulation::RoadSignArmYaw), painted with `Words`.
    void Place(const FString& Name, const FVector& Location, float Yaw, const FString& Words);
    const FString& GetSignName() const { return SignName; }
    bool IsStandIn() const { return bStandIn; }

    // The authored fingerpost for a sign name (ManorRoadSign, TownRoadSign, GatewayRoadSign), or null.
    static const TCHAR* MeshObjectFor(const FString& Name);

private:
    UPROPERTY() TObjectPtr<USceneComponent> Root;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Post;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Board;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Authored;
    UPROPERTY() TObjectPtr<UTextRenderComponent> Front;
    UPROPERTY() TObjectPtr<UTextRenderComponent> Back;
    FString SignName;
    bool bStandIn = true;
};
