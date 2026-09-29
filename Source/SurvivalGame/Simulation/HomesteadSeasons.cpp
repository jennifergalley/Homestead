#include "HomesteadSeasons.h"

#include "HomesteadShops.h"

#include <algorithm>
#include <cmath>

namespace Homestead::Seasons
{
namespace
{
constexpr int DaysPerYear = DaysPerSeason * SeasonCount;

double DaysSinceSpring1(double hour)
{
    return std::isfinite(hour) ? std::max(0.0, (hour - DayRolloverHour) / 24.0) : 0.0;
}
double SeasonEase(double t)
{
    t = std::clamp(t, 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}
// Where in the year a day falls, from the season and its day (1-28).
constexpr int YearDay(int season, int day) { return season * DaysPerSeason + day - 1; }
}

int DayIndex(double hour) { return static_cast<int>(std::floor(DaysSinceSpring1(hour))); }
int SeasonOf(double hour) { return (DayIndex(hour) / DaysPerSeason) % SeasonCount; }
int DayOfSeason(double hour) { return DayIndex(hour) % DaysPerSeason + 1; }

const ForageSeason& ForageSeasonFor(ResourceKind kind)
{
    static const ForageSeason allYear{};
    // Blackberries ripen from mid-summer into autumn and come again every three days while they last.
    static const ForageSeason blackberries{Summer | Autumn, YearDay(1, 15), YearDay(2, 28), 72.0};
    static const ForageSeason mushrooms{Autumn, YearDay(2, 1), YearDay(2, 28), 48.0};
    static const ForageSeason springFlowers{Spring, YearDay(0, 1), YearDay(0, 28)};
    static const ForageSeason meadowHerb{Spring | Summer | Autumn, YearDay(0, 1), YearDay(2, 28)};
    switch (kind)
    {
    case ResourceKind::BerryBush: return blackberries;
    case ResourceKind::FieldMushrooms: return mushrooms;
    case ResourceKind::Primroses:
    case ResourceKind::Bluebells:
    case ResourceKind::WildDaffodils:
    case ResourceKind::WildGarlic: return springFlowers;
    case ResourceKind::Flowers: return meadowHerb;
    default: return allYear;
    }
}

bool InSeason(ResourceKind kind, double hour)
{
    const ForageSeason& window = ForageSeasonFor(kind);
    const int dayOfYear = DayIndex(hour) % DaysPerYear;
    return (window.seasons & (1u << SeasonOf(hour))) != 0
        && dayOfYear >= window.firstDayOfYear && dayOfYear <= window.lastDayOfYear;
}

Look LookAt(double hour)
{
    const double yearDay = std::fmod(DaysSinceSpring1(hour), static_cast<double>(DaysPerYear));
    const int season = std::min(SeasonCount - 1, static_cast<int>(yearDay / DaysPerSeason));
    const double within = yearDay - season * DaysPerSeason; // Days into this season, 0-28.
    Look look;
    look.seasonBlend = season - 1 + SeasonEase(within / TransitionDays);
    if (look.seasonBlend < 0.0) look.seasonBlend += SeasonCount;
    // Leaves: the first tints in summer's last days, full colour by Autumn 12, falling from Autumn 20,
    // bare through winter and leafing out over the first ten days of spring.
    switch (season)
    {
    case 0:
        look.winterBare = 1.0 - SeasonEase(within / 10.0);
        look.autumn = look.winterBare;
        break;
    case 1:
        look.autumn = 0.15 * SeasonEase((within - 24.0) / 4.0);
        break;
    case 2:
        look.autumn = 0.15 + 0.85 * SeasonEase(within / 12.0);
        look.winterBare = 0.7 * SeasonEase((within - 19.0) / 9.0);
        break;
    default:
        look.autumn = 1.0;
        look.winterBare = 0.7 + 0.3 * SeasonEase(within / TransitionDays);
        break;
    }
    // Frost on winter mornings (and lightly in the last week of autumn and the first of spring): it
    // settles before dawn, holds till half past seven and is gone by ten.
    const double coldness = season == 3 ? 1.0
        : season == 2 ? 0.5 * SeasonEase((within - 21.0) / 7.0)
        : season == 0 ? 0.5 * (1.0 - SeasonEase(within / 7.0)) : 0.0;
    const double ofDay = std::fmod(std::max(0.0, hour), 24.0);
    const double morning = SeasonEase((ofDay - 3.0) / 2.0) * (1.0 - SeasonEase((ofDay - 7.5) / 2.5));
    look.frost = coldness * morning;
    return look;
}
}
