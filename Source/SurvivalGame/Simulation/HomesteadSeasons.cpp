#include "HomesteadSeasons.h"

#include <algorithm>
#include <cmath>

namespace Homestead::Seasons
{
namespace
{
double DaysSinceSpring1(double hour)
{
    return std::isfinite(hour) ? std::max(0.0, (hour - Calendar::DayStartHour) / 24.0) : 0.0;
}
double SeasonEase(double t)
{
    t = std::clamp(t, 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}
// Where in the year a day falls, from its season and day (1-28).
constexpr int YearDay(Season season, int day) { return static_cast<int>(season) * Calendar::DaysPerSeason + day - 1; }
}

const ForageSeason& ForageSeasonFor(ResourceKind kind)
{
    static const ForageSeason allYear{};
    // Blackberries ripen from mid-summer into autumn and come again every three days while they last.
    static const ForageSeason blackberries{SeasonBit(Season::Summer) | SeasonBit(Season::Autumn),
        YearDay(Season::Summer, 15), YearDay(Season::Autumn, 28), 72.0};
    static const ForageSeason mushrooms{SeasonBit(Season::Autumn),
        YearDay(Season::Autumn, 1), YearDay(Season::Autumn, 28), 48.0};
    static const ForageSeason springFlowers{SeasonBit(Season::Spring), YearDay(Season::Spring, 1), YearDay(Season::Spring, 28)};
    static const ForageSeason meadowHerb{SeasonBit(Season::Spring) | SeasonBit(Season::Summer) | SeasonBit(Season::Autumn),
        YearDay(Season::Spring, 1), YearDay(Season::Autumn, 28)};
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

bool ForageInSeason(ResourceKind kind, double hour)
{
    const ForageSeason& window = ForageSeasonFor(kind);
    const Calendar::Date date = Calendar::DateAt(hour);
    const int dayOfYear = YearDay(date.season, date.dayOfSeason);
    return Homestead::InSeason(window.seasons, date.season)
        && dayOfYear >= window.firstDayOfYear && dayOfYear <= window.lastDayOfYear;
}

Look LookAt(double hour)
{
    const double yearDay = std::fmod(DaysSinceSpring1(hour), static_cast<double>(Calendar::DaysPerYear));
    const int season = std::min(Calendar::SeasonsPerYear - 1, static_cast<int>(yearDay / Calendar::DaysPerSeason));
    const double within = yearDay - season * Calendar::DaysPerSeason; // Days into this season, 0-28.
    Look look;
    look.seasonBlend = season - 1 + SeasonEase(within / TransitionDays);
    if (look.seasonBlend < 0.0) look.seasonBlend += Calendar::SeasonsPerYear;
    // Leaves: the first tints in summer's last days, full colour by Autumn 12, falling from Autumn 20,
    // bare through winter and leafing out over the first ten days of spring.
    switch (static_cast<Season>(season))
    {
    case Season::Spring:
        look.winterBare = 1.0 - SeasonEase(within / 10.0);
        look.autumn = look.winterBare;
        break;
    case Season::Summer:
        look.autumn = 0.15 * SeasonEase((within - 24.0) / 4.0);
        break;
    case Season::Autumn:
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
    const Season now = static_cast<Season>(season);
    const double coldness = now == Season::Winter ? 1.0
        : now == Season::Autumn ? 0.5 * SeasonEase((within - 21.0) / 7.0)
        : now == Season::Spring ? 0.5 * (1.0 - SeasonEase(within / 7.0)) : 0.0;
    const double ofDay = std::fmod(std::max(0.0, hour), 24.0);
    const double morning = SeasonEase((ofDay - 3.0) / 2.0) * (1.0 - SeasonEase((ofDay - 7.5) / 2.5));
    look.frost = coldness * morning;
    return look;
}
}
