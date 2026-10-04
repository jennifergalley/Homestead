#pragma once

#include <cmath>

namespace Homestead::Daylight
{
// Horizon crossings of the Estate's existing fixed solar path.
constexpr double SunriseHour = 6.0;
constexpr double SunsetHour = 18.0;
constexpr double EveningSleepHour = 18.0;
constexpr double MorningSleepHour = 6.0;

inline double SolarElevation(double hour)
{
    constexpr double TwoPi = 6.28318530717958647692;
    return std::sin((hour - SunriseHour) / 24.0 * TwoPi);
}
}
