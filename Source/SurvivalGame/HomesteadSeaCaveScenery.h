#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HomesteadSeaCaveScenery.generated.h"

class UAudioComponent;
class UMaterialInstanceDynamic;
class USpotLightComponent;
class UStaticMesh;
class UStaticMeshComponent;
class AHomesteadWaterPool;

/**
 * The sea cave in the cove's west cliff (openspec add-sea-cave), placed from Homestead::EstateSeaCave():
 * the cave mesh (SM_SeaCave, fitted under the landscape by Scripts/Blender/Recipes/sea_cave.py; the
 * landscape's hole at its mouth is painted by Scripts/Terrain/build_landscape_holes.py), its still freshwater
 * pool (an AHomesteadWaterPool, so her pail fills there), a thread of sky through the roof fissure that
 * follows the daylight (faint moonlight at night), reused rocks, ferns and flowers, and its drips and the
 * muffled sea. Scenery only: nothing is saved.
 */
UCLASS()
class SURVIVALGAME_API AHomesteadSeaCave : public AActor
{
    GENERATED_BODY()

public:
    AHomesteadSeaCave();
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

    /** Builds the cave once; false (nothing placed) when the cave mesh isn't imported. */
    bool Build();
    /** The sky over the fissure: Daylight 0 (night) .. 1, Overcast 0 .. 1 (UpdateLighting's values). */
    void SetSky(float Daylight, float Overcast);

private:
    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> Rock;
    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> SkyCard;
    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> SkyCardMaterial;
    UPROPERTY()
    TObjectPtr<USpotLightComponent> Shaft;
    UPROPERTY()
    TObjectPtr<AHomesteadWaterPool> Pool;
    UPROPERTY()
    TArray<TObjectPtr<UStaticMeshComponent>> Dressing;
    /** The cove's west cliff: one instanced batch per rock or plant mesh (HomesteadCoveCliff.inc). */
    UPROPERTY()
    TArray<TObjectPtr<class UInstancedStaticMeshComponent>> Cliff;
    void BuildCliff();
    UPROPERTY()
    TArray<TObjectPtr<UAudioComponent>> Sounds;
    TArray<float> SoundGains;
    bool bBuilt = false;
    float AppliedDaylight = -1.0f;
    float AppliedOvercast = -1.0f;
};
