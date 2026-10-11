#include "HomesteadNightLight.h"

#include <algorithm>
#include <cmath>

namespace Homestead
{
namespace
{
constexpr double NightLightPi = 3.14159265358979323846;
// Daylight = smoothstep(-0.1, 0.25, sun elevation), as UpdateLighting uses.
constexpr double NightLightDuskElevation = -0.1, NightLightDayElevation = 0.25;
// Incident-light exposure: EV100 0 is about 2.5 lux (EV = log2(lux * 100 / 250)).
constexpr double NightLightLuxAtEV0 = 2.5;

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
    // One fixed moon all night: level-ground moonlight follows only the night factor, so it blends in with
    // the dusk as the sun goes out, holds level to the pre-dawn, and blends out with the dawn.
    light.moonAltitude = std::sin(tuning.moonAltitudeDegrees * NightLightPi / 180.0);
    const double night = 1.0 - light.daylight;
    light.moonLux = tuning.moonGroundLux * night / light.moonAltitude;
    light.moonGroundLux = light.moonLux * light.moonAltitude;
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
