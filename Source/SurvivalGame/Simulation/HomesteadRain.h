#pragma once

// The estate's rain (add-rain-weather; Jenny, 2026-09-30: "let it randomize throughout the day/night
// cycle"). Each calendar day (06:00 to 06:00) may hold one spell of rain. A stable hash of the day picks
// whether it rains (more often in autumn and winter, the Cornish coast's wet seasons), when it starts
// (any hour), how long it lasts (1-8 h, most often 5-7) and how long the cloud takes to build before it
// and clear after it (half an hour to an hour and a half). A spell may run past midnight and past the next
// 06:00. It all depends on the hour alone: no seed and nothing saved, so a reload brings the same weather.
namespace Homestead
{
namespace Rain
{
// The chance that a day's spell is drawn, by season (Spring, Summer, Autumn, Winter). Spring keeps about
// the old schedule's share (two days in ten); the year averages about 5% of hours wet, as before.
constexpr double SeasonChance[4] = {0.20, 0.13, 0.25, 0.28};
constexpr double MinSpellHours = 1.0;
constexpr double MaxSpellHours = 8.0;
constexpr double MinCloudHours = 0.5; // build-up before a spell and clearing after it
constexpr double MaxCloudHours = 1.5;
// A drawn spell is dropped when its cloud would meet the previous day's spell's within this many hours,
// so spells never merge into one longer than MaxSpellHours.
constexpr double MinClearHours = 1.0;
// The ground and meadow wet through over the first half hour of rain and dry over four hours after it.
constexpr double WetInHours = 0.5;
constexpr double DryHours = 4.0;
}

struct RainSpell
{
    long long day = 0;     // the calendar day (Calendar::DayIndex) that drew it
    double start = 0.0;    // running game hours, State::hour
    double end = 0.0;      // rain falls start <= hour < end
    double buildUp = Rain::MinCloudHours;  // cloud gathers from start - buildUp
    double clearing = Rain::MinCloudHours; // and is gone by end + clearing
    double phase = 0.0;    // where the showers' swell starts
};

// The spell calendar day `day` holds, if any. Day 0 (a new game's first day) is always dry.
bool RainSpellOfDay(long long day, RainSpell& spell);
// The spell whose rain or cloud covers `hour`, if any.
bool RainSpellAt(double hour, RainSpell& spell);
// The first spell still to come or raining now at `hour` (its rain ends after `hour`) that starts within
// `withinHours`; false when none does.
bool NextRainSpell(double hour, RainSpell& spell, double withinHours = 24.0 * 28.0);
bool IsRainingAt(double hour);
// Whether rain falls at any time in the calendar day (06:00 to 06:00) holding `hour`.
bool IsRainDay(double hour);
// How hard it's raining at `hour`, 0-1: nothing outside a spell; inside one a drizzle (0.3) that swells into
// passing showers (up to 1) every hour and a half or so, easing in over the first quarter hour and out over
// the last ten minutes.
double RainAmount(double hour);
// Cloud cover at `hour`, 0-1: builds before a spell and clears after it, so the sky greys before a drop falls.
double Overcast(double hour);
// How wet the ground and meadow are at `hour`, 0-1 (Rain::WetInHours, Rain::DryHours).
double GroundWetness(double hour);
// The next hour after `hour` at which rain starts or stops (within the next day or so; +infinity when none
// is that near). Simulation::Step stops there, so a step never spans the start or end of a spell.
double NextRainChange(double hour);
}
