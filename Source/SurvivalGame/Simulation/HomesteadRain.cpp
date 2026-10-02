#include "HomesteadRain.h"

#include "HomesteadCalendar.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace Homestead
{
// File-local helpers, named so a unity build can't mix them with another file's (no file-scope using).
namespace HomesteadRainDetail
{
constexpr std::uint64_t RainSalt = 0x52A12026ull;

std::uint64_t Mix(std::uint64_t z)
{
    z += 0x9E3779B97F4A7C15ull;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

// The day's k-th uniform draw in [0, 1).
double Draw(long long day, int k)
{
    const std::uint64_t seed = Mix(static_cast<std::uint64_t>(day) ^ RainSalt);
    return static_cast<double>(Mix(seed + static_cast<std::uint64_t>(k) * 0xD1B54A32D192ED03ull) >> 11) * 0x1.0p-53;
}

double Ease(double t)
{
    t = std::clamp(t, 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

double DayStart(long long day) { return static_cast<double>(day) * 24.0 + Calendar::DayStartHour; }

// The day's drawn spell before the clear-sky check against the day before.
bool DrawnSpell(long long day, RainSpell& spell)
{
    if (day <= 0) return false;
    const auto season = Calendar::DateOfDay(static_cast<int>(std::clamp<long long>(day, 0, 1LL << 30))).season;
    if (Draw(day, 0) >= Rain::SeasonChance[static_cast<int>(season)]) return false;
    // Long spells are the commonest (mean about 5.7 h), as the old six-hour rain days were.
    const double shortness = Draw(day, 2);
    spell.day = day;
    spell.start = DayStart(day) + Draw(day, 1) * 24.0;
    spell.end = spell.start + Rain::MaxSpellHours - (Rain::MaxSpellHours - Rain::MinSpellHours) * shortness * shortness;
    spell.buildUp = Rain::MinCloudHours + (Rain::MaxCloudHours - Rain::MinCloudHours) * Draw(day, 3);
    spell.clearing = Rain::MinCloudHours + (Rain::MaxCloudHours - Rain::MinCloudHours) * Draw(day, 4);
    spell.phase = Draw(day, 5) * 6.2831853;
    return true;
}

template <typename Visit>
void ForSpellsNear(double hour, Visit&& visit)
{
    // A day's spell starts within its own 24 hours and ends at most MaxSpellHours later, with its cloud up
    // to MaxCloudHours either side, so only the day before, the day and the day after can reach `hour`.
    const long long day = Calendar::DayIndex(hour);
    RainSpell spell;
    for (long long d = day - 1; d <= day + 1; ++d)
        if (RainSpellOfDay(d, spell)) visit(spell);
}
}

bool RainSpellOfDay(long long day, RainSpell& spell)
{
    using namespace HomesteadRainDetail;
    if (!DrawnSpell(day, spell)) return false;
    RainSpell before;
    // Spells stay apart: a drawn spell whose cloud would meet the previous day's is dropped.
    if (DrawnSpell(day - 1, before) && spell.start - spell.buildUp < before.end + before.clearing + Rain::MinClearHours)
        return false;
    return true;
}

bool RainSpellAt(double hour, RainSpell& spell)
{
    using namespace HomesteadRainDetail;
    bool found = false;
    ForSpellsNear(hour, [&](const RainSpell& candidate)
    {
        if (!found && hour >= candidate.start - candidate.buildUp && hour < candidate.end + candidate.clearing)
        {
            spell = candidate;
            found = true;
        }
    });
    return found;
}

bool NextRainSpell(double hour, RainSpell& spell, double withinHours)
{
    using namespace HomesteadRainDetail;
    if (!std::isfinite(hour) || !(withinHours >= 0.0)) return false;
    const long long last = Calendar::DayIndex(hour + withinHours);
    for (long long day = Calendar::DayIndex(hour) - 1; day <= last; ++day)
    {
        RainSpell candidate;
        if (RainSpellOfDay(day, candidate) && candidate.end > hour && candidate.start <= hour + withinHours)
        {
            spell = candidate;
            return true;
        }
    }
    return false;
}

bool IsRainingAt(double hour)
{
    using namespace HomesteadRainDetail;
    if (!std::isfinite(hour)) return false;
    bool raining = false;
    ForSpellsNear(hour, [&](const RainSpell& spell) { raining = raining || (hour >= spell.start && hour < spell.end); });
    return raining;
}

bool IsRainDay(double hour)
{
    using namespace HomesteadRainDetail;
    if (!std::isfinite(hour)) return false;
    const long long day = Calendar::DayIndex(hour);
    const double from = DayStart(day), to = from + 24.0;
    RainSpell spell;
    for (long long d = day - 1; d <= day; ++d)
        if (RainSpellOfDay(d, spell) && spell.start < to && spell.end > from) return true;
    return false;
}

double RainAmount(double hour)
{
    using namespace HomesteadRainDetail;
    if (!std::isfinite(hour)) return 0.0;
    double amount = 0.0;
    ForSpellsNear(hour, [&](const RainSpell& spell)
    {
        if (hour < spell.start || hour >= spell.end) return;
        const double into = hour - spell.start;
        const double envelope = Ease(into / 0.25) * Ease((spell.end - hour) / (1.0 / 6.0));
        const double wave = 0.5 + 0.5 * std::sin(into * 6.2831853 / 1.6 + spell.phase)
            * (0.8 + 0.2 * std::sin(into * 6.2831853 / 0.55 + spell.phase * 0.53));
        amount = std::max(amount, envelope * (0.3 + 0.7 * Ease((wave - 0.35) / 0.5)));
    });
    return amount;
}

double Overcast(double hour)
{
    using namespace HomesteadRainDetail;
    if (!std::isfinite(hour)) return 0.0;
    double cloud = 0.0;
    ForSpellsNear(hour, [&](const RainSpell& spell)
    {
        cloud = std::max(cloud, Ease((hour - (spell.start - spell.buildUp)) / spell.buildUp)
            * Ease((spell.end + spell.clearing - hour) / spell.clearing));
    });
    return cloud;
}

double GroundWetness(double hour)
{
    using namespace HomesteadRainDetail;
    if (!std::isfinite(hour)) return 0.0;
    double wet = 0.0;
    ForSpellsNear(hour, [&](const RainSpell& spell)
    {
        if (hour < spell.start) return;
        const double soaked = Ease((std::min(hour, spell.end) - spell.start) / Rain::WetInHours);
        wet = std::max(wet, soaked * (1.0 - Ease((hour - spell.end) / Rain::DryHours)));
    });
    return wet;
}

double NextRainChange(double hour)
{
    using namespace HomesteadRainDetail;
    double next = std::numeric_limits<double>::infinity();
    if (!std::isfinite(hour)) return next;
    ForSpellsNear(hour, [&](const RainSpell& spell)
    {
        for (const double edge : {spell.start, spell.end})
            if (edge > hour + 1e-9) next = std::min(next, edge);
    });
    return next;
}
}
