#pragma once

// The oil lamp's light (add-oil-lamp; Jenny's playtest, 2026-09-29: "about four times the visible reach").
// Plain C++ so the reach is native-tested: how bright the lamp lights the ground at a distance under the
// engine's two point-light attenuation models, the tuned profile, and the placed lamp's shadow policy.
// HomesteadLampLook.cpp applies the profile to the held, placed and lab lamps (console variables
// homestead.Lamp* override it for tuning).
namespace Homestead
{
struct LampLightProfile
{
    double intensity = 0.0;         // the engine's unitless intensity
    double radiusCm = 0.0;          // attenuation radius
    double falloffExponent = 0.0;   // used when !inverseSquared
    bool inverseSquared = false;
};

// The first lamp (inverse-square, 1400 unitless, a 10 m window): moonlit-ground level (0.2 lux) by 3.3 m.
constexpr LampLightProfile LegacyOilLampLight{1400.0, 1000.0, 8.0, true};
// The lamp now: a gentler falloff than inverse-square, as the eye adapts to the pool it lights, so it
// carries about four times as far (0.2 lux at 13.4 m) without a blaze round her hand: 4 lux at the flame,
// fading to nothing at 20 m. Inverse-square at 16 times the intensity would reach as far but put 140 lux on
// her dress, and the night's auto exposure would take the reach straight back.
constexpr LampLightProfile OilLampLight{4.0, 2000.0, 5.0, false};

// The ground a lamp lights reads against moonlit ground (HomesteadNightLight: 0.2 lux at full night).
constexpr double LampReachLux = 0.2;
// The placed lamp casts shadows only while the camera is this near (the held lamp always does).
constexpr double PlacedLampShadowDistanceCm = 2500.0;

// Illuminance (lux) a lamp gives a surface facing it at distanceCm, under the engine's attenuation: for
// inverse-square lights brightness 16 x intensity over (d^2 + 1) with d in cm, windowed by
// (1 - (d/R)^4)^2; otherwise intensity x (1 - (d/R)^2)^exponent. Zero at and past the radius.
double LampIlluminance(const LampLightProfile& profile, double distanceCm);
// The farthest distance (cm) at which the lamp gives at least `lux`.
double LampReachCm(const LampLightProfile& profile, double lux);
// Whether the placed lamp casts shadows with the camera cameraDistanceCm away (shadowDistanceCm <= 0: always).
bool PlacedLampCastsShadows(double cameraDistanceCm, double shadowDistanceCm = PlacedLampShadowDistanceCm);
}
