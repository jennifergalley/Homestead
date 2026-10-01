#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HomesteadRoadSign.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

// A fingerpost on the public road (Simulation/HomesteadEstatePublicRoad.h signs): walking up to it
// offers the Map tab's walk to town or home (Simulation/HomesteadTravel.h RoadSignDestinations). Until
// Props' original sign mesh is imported it is a clearly labelled stand-in: a post and a painted board.
UCLASS()
class SURVIVALGAME_API AHomesteadRoadSign : public AActor
{
    GENERATED_BODY()
public:
    AHomesteadRoadSign();
    // Stands the sign at `Location` with its face along `Yaw`, painted with `Words`.
    void Place(const FString& Name, const FVector& Location, float Yaw, const FString& Words);
    const FString& GetSignName() const { return SignName; }
    bool IsStandIn() const { return bStandIn; }

    // Props' sign mesh, used when it exists.
    static constexpr const TCHAR* MeshPackage = TEXT("/Game/SurvivalGame/Environment/Props/RoadSign/SM_RoadSign");
    static constexpr const TCHAR* MeshObject = TEXT("/Game/SurvivalGame/Environment/Props/RoadSign/SM_RoadSign.SM_RoadSign");

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
