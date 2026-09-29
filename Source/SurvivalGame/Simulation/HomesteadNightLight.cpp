#include "HomesteadNightLight.h"

#include <algorithm>
#include <cmath>

namespace Homestead
{
namespace
{
constexpr double NightLightPi = 3.14159265358979323846;
// The world's day circle: the sun's pitch is -elevation * 65 degrees (HomesteadWorld.cpp UpdateLighting).
constexpr double NightLightArcDegrees = 65.0;
// Daylight = smoothstep(-0.1, 0.25, sun elevation), as UpdateLighting uses.
constexpr double NightLightDuskElevation = -0.1, NightLightDayElevation = 0.25;
// Incident-light exposure: EV100 0 is about 2.5 lux (EV = log2(lux * 100 / 250)).
constexpr double NightLightLuxAtEV0 = 2.5;
// The moon fades in over its first 3 degrees above the horizon (sine 0.05), about 5 game minutes.
constexpr double NightLightMoonFadeAltitude = 0.05;

double NightSmoothStep(double a, double b, double x)
{
    const double t = std::clamp((x - a) / (b - a), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}
}

NightLight NightLightAt(double hour, const NightLightTuning& tuning)
{
    NightLight light;
    const double h = std::fmod(std::fmod(hour, 24.0) + 24.0, 24.0);
    light.sunElevation = std::sin((h - 6.0) / 24.0 * 2.0 * NightLightPi);
    light.daylight = NightSmoothStep(NightLightDuskElevation, NightLightDayElevation, light.sunElevation);
    light.moonAltitude = std::sin(-light.sunElevation * NightLightArcDegrees * NightLightPi / 180.0);
    // Level-ground moonlight fades in with the dusk and then holds: the directional intensity falls as
    // the moon climbs, so a rising moon doesn't brighten the night.
    const double night = 1.0 - light.daylight;
    const double up = std::max(light.moonAltitude, 0.0);
    // Near the horizon the moon fades in as it rises (and out as it sets) instead of popping on.
    const double rise = NightSmoothStep(0.0, NightLightMoonFadeAltitude, up);
    light.moonLux = tuning.moonGroundLux * night * rise / std::max(up, tuning.moonCompensationFloor);
    light.moonGroundLux = light.moonLux * up;
    light.skyScale = tuning.nightSky + (1.0 - tuning.nightSky) * light.daylight;
    light.minExposureEV = tuning.nightMinExposureEV * night;
    return light;
}

double NightGroundKey(const NightLight& light)
{
    const double lux = std::max(light.moonGroundLux, 1e-6);
    const double sceneEV = std::log2(lux / NightLightLuxAtEV0);
    const double exposureEV = std::max(sceneEV, light.minExposureEV);
    return std::exp2(sceneEV - exposureEV);
}
}
