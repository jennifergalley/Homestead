#include "HomesteadLampLight.h"

#include <algorithm>
#include <cmath>

namespace Homestead
{
double LampIlluminance(const LampLightProfile& profile, double distanceCm)
{
    if (profile.radiusCm <= 0.0 || distanceCm >= profile.radiusCm) return 0.0;
    const double d = std::max(0.0, distanceCm);
    const double t = d / profile.radiusCm;
    if (profile.inverseSquared)
    {
        const double window = std::pow(std::clamp(1.0 - t * t * t * t, 0.0, 1.0), 2.0);
        return 16.0 * profile.intensity / (d * d + 1.0) * window;
    }
    return profile.intensity * std::pow(std::clamp(1.0 - t * t, 0.0, 1.0), profile.falloffExponent);
}

double LampReachCm(const LampLightProfile& profile, double lux)
{
    // Both models fall monotonically with distance: bisect for the last distance still at `lux`.
    if (LampIlluminance(profile, 0.0) < lux) return 0.0;
    double lo = 0.0, hi = profile.radiusCm;
    for (int i = 0; i < 60; ++i)
    {
        const double mid = 0.5 * (lo + hi);
        (LampIlluminance(profile, mid) >= lux ? lo : hi) = mid;
    }
    return lo;
}

bool PlacedLampCastsShadows(double cameraDistanceCm, double shadowDistanceCm)
{
    return shadowDistanceCm <= 0.0 || cameraDistanceCm <= shadowDistanceCm;
}
}
