#include "HomesteadWorld.h"
#include "HomesteadWorldLog.h"
#include "HomesteadWeather.h"
#include "Simulation/HomesteadNightLight.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "RenderUtils.h"

namespace HomesteadWorldLighting
{
TAutoConsoleVariable<int32> CVarRayTracedSun(TEXT("homestead.RayTracedSun"), 1,
    TEXT("1 = ray-traced sun/moon shadows with continuous sun movement (default when hardware ray "
         "tracing is on). 0 = Virtual Shadow Maps with the sun stepped by 0.5 degrees."));
// Night light (Simulation/HomesteadNightLight; defaults match Homestead::NightLightTuning, which
// HomesteadNightLightTests checks against this file).
TAutoConsoleVariable<float> CVarNightMoonLux(TEXT("homestead.NightMoonLux"), 0.2f,
    TEXT("Moonlit level ground (lux) at full night; the moon's intensity compensates for its altitude."));
TAutoConsoleVariable<float> CVarNightSky(TEXT("homestead.NightSky"), 0.3f,
    TEXT("Sky light intensity at full night."));
TAutoConsoleVariable<float> CVarNightMinExposure(TEXT("homestead.NightMinExposure"), -1.0f,
    TEXT("Auto exposure min brightness (EV100) at full night."));
TAutoConsoleVariable<float> CVarIndoorDaySky(TEXT("homestead.IndoorDaySky"), 0.3f,
    TEXT("Sky light scale while she is inside a roofed room by day (times the room mix and daylight). Groom sky lighting sees the "
         "open sky capture through a roof, so a full sky light blew her hair out white under one."));
}

using HomesteadWorldLighting::CVarRayTracedSun;
using HomesteadWorldLighting::CVarNightMoonLux;
using HomesteadWorldLighting::CVarNightSky;
using HomesteadWorldLighting::CVarNightMinExposure;
using HomesteadWorldLighting::CVarIndoorDaySky;

void AHomesteadWorld::BuildLighting()
{
    Exposure = NewObject<UPostProcessComponent>(this, TEXT("MeadowExposure"));
    Exposure->SetupAttachment(GetRootComponent());
    Exposure->bUnbound = true;
    Exposure->BlendWeight = 1.0f;
    FPostProcessSettings& Settings = Exposure->Settings;
    Settings.bOverride_AutoExposureMethod = true;
    Settings.AutoExposureMethod = AEM_Histogram;
    // Extended luminance range makes these EV100: accommodate the physical sun,
    // but cap dark adaptation so moonlit ground is not exposed like daylight.
    Settings.bOverride_AutoExposureMinBrightness = true;
    Settings.AutoExposureMinBrightness = 0.0f;
    Settings.bOverride_AutoExposureMaxBrightness = true;
    Settings.AutoExposureMaxBrightness = 16.0f;
    Settings.bOverride_AutoExposureBias = true;
    Settings.AutoExposureBias = -0.15f;
    // Virtual Shadow Maps give real canopy shadows, and a low sun lights upright trunks and the
    // heroine head-on while the ground only gets grazing light. Compress local highlights and lift
    // local shade so dawn doesn't clip sunlit bark or crush nearby shade to black.
    Settings.bOverride_LocalExposureHighlightContrastScale = true;
    Settings.LocalExposureHighlightContrastScale = 0.5f;
    Settings.bOverride_LocalExposureShadowContrastScale = true;
    Settings.LocalExposureShadowContrastScale = 0.8f;
    // Near-horizon sunlight is deep orange after atmospheric transmittance; slightly desaturating
    // highlights stops saturated albedo (the green dress) from hue-clipping to flat yellow.
    Settings.bOverride_ColorSaturationHighlights = true;
    Settings.ColorSaturationHighlights = FVector4(0.85f, 0.85f, 0.85f, 1.0f);
    Settings.bOverride_AutoExposureSpeedUp = true;
    Settings.AutoExposureSpeedUp = 3.0f;
    Settings.bOverride_AutoExposureSpeedDown = true;
    Settings.AutoExposureSpeedDown = 1.0f;
    // Overcast greys the scene (UpdateLighting).
    Settings.bOverride_ColorSaturation = true;
    Settings.ColorSaturation = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
    Exposure->RegisterComponent();

    Sun = NewObject<UDirectionalLightComponent>(this, TEXT("MeadowSun"));
    Sun->SetupAttachment(GetRootComponent());
    Sun->SetMobility(EComponentMobility::Movable);
    Sun->bAtmosphereSunLight = true;
    // The moon is also an atmosphere light; the sun wins forward shading (water, translucency, fog).
    Sun->ForwardShadingPriority = 1;
    Sun->SetIntensity(46000.0f);
    // Ray-traced sun shadows have no cache, so the sun can move every refresh without the Virtual
    // Shadow Map re-render stalls a rotating sun causes over these non-Nanite trees (4K: 85 vs 78 FPS,
    // p99 15 vs 24 ms). UpdateLighting applies homestead.RayTracedSun; without hardware ray tracing
    // the lights use VSM.
    Sun->RegisterComponent();

    Moon = NewObject<UDirectionalLightComponent>(this, TEXT("MeadowMoonlight"));
    Moon->SetupAttachment(GetRootComponent());
    Moon->SetMobility(EComponentMobility::Movable);
    Moon->bAtmosphereSunLight = true;
    Moon->AtmosphereSunLightIndex = 1;
    Moon->SetLightColor(FLinearColor(0.53f, 0.66f, 1.0f));
    Moon->SetIntensity(0.5f);
    Moon->RegisterComponent();

    Sky = NewObject<USkyLightComponent>(this, TEXT("MeadowSkyLight"));
    Sky->SetupAttachment(GetRootComponent());
    Sky->SetMobility(EComponentMobility::Movable);
    Sky->bRealTimeCapture = true;
    Sky->SetIntensity(1.0f);
    Sky->RegisterComponent();

    USkyAtmosphereComponent* Atmosphere = NewObject<USkyAtmosphereComponent>(this, TEXT("MeadowAtmosphere"));
    Atmosphere->SetupAttachment(GetRootComponent());
    Atmosphere->RegisterComponent();

    Fog = NewObject<UExponentialHeightFogComponent>(this, TEXT("MeadowDistanceHaze"));
    Fog->SetupAttachment(GetRootComponent());
    Fog->SetMobility(EComponentMobility::Movable);
    Fog->SetFogDensity(0.007f);
    Fog->SetFogHeightFalloff(0.3f);
    Fog->SetStartDistance(1100.0f);
    Fog->RegisterComponent();

    Weather = NewObject<UHomesteadWeather>(this, TEXT("Weather"));
    Weather->SetupAttachment(GetRootComponent());
    Weather->RegisterComponent();
}

void AHomesteadWorld::UpdateLighting(const Homestead::State& State)
{
    const float Hour = static_cast<float>(FMath::Fmod(State.hour, 24.0));
    const float SolarAngle = (Hour - 6.0f) / 24.0f * 2.0f * PI;
    const float Elevation = FMath::Sin(SolarAngle);
    const float Daylight = FMath::SmoothStep(-0.1f, 0.25f, Elevation);
    // The simulation's three-day spring weather: cloud builds half an hour before the rain and clears
    // half an hour after it (Homestead::Overcast), the rain swells and eases (Homestead::RainAmount).
    if (Weather) Weather->Update(State, Daylight);
    const float Cloud = Weather ? Weather->GetOvercast() : 0.0f;
    const float Shower = Weather ? Weather->GetRain() : 0.0f;
    const FRotator SunRotation(-Elevation * 65.0f, (Hour - 6) * 15.0f - 70.0f, 0);
    const FRotator MoonRotation(Elevation * 65.0f, (Hour - 6) * 15.0f + 110.0f, 0);
    // With ray-traced sun shadows, follow the sun every refresh (about 0.025 degrees at normal game
    // speed, so no visible shadow step). Virtual Shadow Maps re-render every cached page when a
    // directional light rotates, so on that fallback step by 0.5 degrees, a few real seconds of
    // daylight travel, to keep full re-renders rare.
    const bool bRayTracedSun = IsRayTracingEnabled() && CVarRayTracedSun.GetValueOnGameThread() != 0;
    const auto ShadowMode = bRayTracedSun ? ECastRayTracedShadow::Enabled : ECastRayTracedShadow::Disabled;
    if (Sun->GetCastRaytracedShadow() != ShadowMode)
    {
        Sun->SetCastRaytracedShadows(ShadowMode);
        Moon->SetCastRaytracedShadows(ShadowMode);
    }
    const float LightRotationStepDegrees = bRayTracedSun ? 0.0f : 0.5f;
    if (!bLightRotationApplied || !SunRotation.Equals(AppliedSunRotation, LightRotationStepDegrees))
    {
        Sun->SetRelativeRotation(SunRotation);
        Moon->SetRelativeRotation(MoonRotation);
        AppliedSunRotation = SunRotation;
        AppliedMoonRotation = MoonRotation;
        bLightRotationApplied = true;
    }
    // Under cloud the sun is a dim, broad glow: little direct light and soft, faint shadows; the sky
    // light carries the scene instead.
    Sun->SetIntensity(FMath::Lerp(0.0f, 46000.0f, Daylight) * FMath::Lerp(1.0f, OvercastSunScale, Cloud));
    const float SourceAngle = FMath::Lerp(0.5357f, OvercastSunSourceAngle, Cloud);
    if (FMath::Abs(SourceAngle - AppliedSunSourceAngle) > 0.25f)
    {
        Sun->SetLightSourceAngle(SourceAngle);
        AppliedSunSourceAngle = SourceAngle;
    }
    // The sky atmosphere already reddens a low sun through its transmittance; keep only a mild
    // extra tint so dawn stays golden instead of saturating to orange.
    Sun->SetLightColor(FMath::Lerp(FLinearColor(1.0f, 0.9f, 0.8f),
        FLinearColor(1.0f, 0.99f, 0.95f), FMath::Clamp(Elevation * 2, 0.0f, 1.0f)));
    // Night (Jenny, 2026-09-29: "around 9 PM it brightens and the moon lights like the sun"): the moonlit
    // ground holds level from dusk to dawn instead of brightening threefold as the moon climbs, and the
    // exposure floor keeps auto-exposure from adapting moonlight up to a daylight grey
    // (Homestead::NightLightAt, native-tested).
    Homestead::NightLightTuning NightTuning;
    NightTuning.moonGroundLux = CVarNightMoonLux.GetValueOnGameThread();
    NightTuning.nightSky = CVarNightSky.GetValueOnGameThread();
    NightTuning.nightMinExposureEV = CVarNightMinExposure.GetValueOnGameThread();
    const Homestead::NightLight Night = Homestead::NightLightAt(Hour, NightTuning);
    Moon->SetIntensity(static_cast<float>(Night.moonLux));
    // Only while she is inside a roofed room (her position, eased), so the outdoors keeps its sky fill when
    // the camera passes a doorway or an overhang.
    const float IndoorDay = GetRoomMix() * Daylight;
    Sky->SetIntensity(static_cast<float>(Night.skyScale) * FMath::Lerp(1.0f, OvercastSkyScale, Cloud)
        * FMath::Lerp(1.0f, CVarIndoorDaySky.GetValueOnGameThread(), IndoorDay));
    // The real-time sky capture still sees the clear blue atmosphere under the cloud layer, so warm it
    // back towards a neutral grey overcast.
    Sky->SetLightColor(FMath::Lerp(FLinearColor::White, FLinearColor(1.0f, 0.93f, 0.84f), Cloud));
    Exposure->Settings.AutoExposureMinBrightness = static_cast<float>(Night.minExposureEV);
    // Auto-exposure would brighten a dull day back to a sunny one; hold it down and take the colour out.
    Exposure->Settings.AutoExposureBias = -0.15f + OvercastExposureBias * Cloud
        + IndoorDayExposureBias * IndoorDay;
    const float Saturation = FMath::Lerp(1.0f, OvercastSaturation, Cloud);
    Exposure->Settings.ColorSaturation = FVector4(Saturation, Saturation, Saturation, 1.0f);
    const float ClearFog = FMath::Lerp(0.016f, 0.007f, Daylight);
    Fog->SetFogDensity(FMath::Lerp(ClearFog, 0.022f, Cloud) + 0.016f * Shower);
    Fog->SetFogHeightFalloff(FMath::Lerp(0.3f, 0.12f, Cloud));
    const FLinearColor ClearHaze = FMath::Lerp(FLinearColor(0.055f, 0.085f, 0.14f), FLinearColor(0.64f, 0.72f, 0.68f), Daylight);
    Fog->SetFogInscatteringColor(FMath::Lerp(ClearHaze, FLinearColor(0.43f, 0.47f, 0.5f) * FMath::Lerp(0.15f, 1.0f, Daylight), Cloud));

    // The ground and meadow wet through in the first half hour of rain and dry over the four hours
    // after it stops; the rain days follow Homestead::IsRainDay.
    if (!bGroundParametersTried)
    {
        bGroundParametersTried = true;
        GroundParameters = LoadObject<UMaterialParameterCollection>(nullptr,
            TEXT("/Game/SurvivalGame/Estate/Ground/MPC_EstateGround.MPC_EstateGround"));
    }
    if (GroundParameters && GetWorld())
        if (UMaterialParameterCollectionInstance* GroundValues = GetWorld()->GetParameterCollectionInstance(GroundParameters))
        {
            constexpr float RainStart = static_cast<float>(Homestead::RainStartHour);
            constexpr float RainEnd = static_cast<float>(Homestead::RainEndHour);
            const float Wetness = !Homestead::IsRainDay(State.hour) ? 0.0f
                : FMath::SmoothStep(RainStart, RainStart + 0.5f, Hour) * (1.0f - FMath::SmoothStep(RainEnd, RainEnd + 4.0f, Hour));
            GroundValues->SetScalarParameterValue(TEXT("Wetness"), Wetness);
            GroundValues->SetScalarParameterValue(TEXT("Daylight"), Daylight);
        }
}
