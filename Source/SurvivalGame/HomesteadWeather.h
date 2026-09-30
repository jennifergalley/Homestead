#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Simulation/HomesteadSimulation.h"

#include "HomesteadWeather.generated.h"

class UAudioComponent;
class UMaterialParameterCollection;
class UStaticMeshComponent;

/**
 * The estate's rain (add-rain-weather): falling rain round the camera, the overcast cloud layer and
 * the rain's sound, following the simulation's schedule (Homestead::RainAmount and Overcast).
 * AHomesteadWorld::UpdateLighting feeds it the state every refresh and reads Overcast back to grey
 * the light; it ticks every frame to follow the camera.
 *
 * The streaks are one static mesh of small quads (SM_RainStreaks) that M_Rain lays out in a
 * world-anchored lattice wrapped round the camera, slanted by the wind and thinned by the rain's
 * strength. Streaks inside a roofed building (up to MaxShelters roofs nearest the camera, passed
 * through MPC_EstateWeather) are hidden, and when the camera is under any other roof the rain fades
 * out altogether. The cloud layer (M_RainClouds on a large sphere round the camera) only covers sky
 * pixels. Assets are built by Scripts/Terrain/build_weather.py.
 */
UCLASS()
class SURVIVALGAME_API UHomesteadWeather : public USceneComponent
{
    GENERATED_BODY()

public:
    static constexpr int32 MaxShelters = 8;
    // A roof piece's reach round its centre (a 3 m cell's half diagonal) and the height its rain stops.
    static constexpr float ShelterRadiusCm = 215.0f;
    static constexpr float ShelterTopCm = 520.0f;
    // How far overhead a roof shelters the camera (and muffles the rain and ambience) when it isn't a
    // building piece. Checked every OverheadCheckSeconds in any weather.
    static constexpr float OverheadCheckCm = 2500.0f;
    static constexpr float OverheadCheckSeconds = 0.25f;
    // Rain ambience volume: Homestead::RainAudioGain (the simulation, native-tested). Indoors it's also
    // muffled behind a low-pass at IndoorCutoffHz, like rain heard on a roof.
    static constexpr float IndoorCutoffHz = 900.0f;

    UHomesteadWeather();
    /** The weather and daylight at the state's hour, and the roofs that shelter from rain. */
    void Update(const Homestead::State& State, float Daylight);
    /** Every frame: follow the camera, check for a roof overhead, drive the sound and the materials. */
    void TickWeather(float DeltaSeconds);
    float GetOvercast() const { return Overcast; }
    float GetRain() const { return Rain; }
    /** 0 outdoors .. 1 indoors at the camera, eased (under a roof piece or anything else overhead). The
     *  rain, the woodland ambience, the creek and a roofed hearth all mix by it (HomesteadRoomAudio). */
    float GetIndoorMix() const { return Indoors; }
    /** Whether a point is under one of the building pieces' roofs. */
    bool IsUnderShelter(const FVector& Point) const;

private:
    bool LoadAssets();

    UPROPERTY() TObjectPtr<UStaticMeshComponent> Streaks;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Clouds;
    UPROPERTY() TObjectPtr<UAudioComponent> Sound;
    UPROPERTY() TObjectPtr<UMaterialParameterCollection> Parameters;
    TArray<FVector4f> Shelters; // x, y, radius, top z (cm) of each roof piece.
    FVector ShelterFrom = FVector(FLT_MAX);
    float Rain = 0.0f;
    float Overcast = 0.0f;
    float Daylight = 1.0f;
    float Indoors = 0.0f;       // 0 outdoors .. 1 indoors, eased.
    float StreakFade = 1.0f;    // 0 under a roof that isn't a building piece.
    float OverheadCheckIn = 0.0f;
    bool bOverhead = false;
    bool bInShelter = false;
    bool bAssetsTried = false;
    bool bAssetsReady = false;
};
