#pragma once

// Seasonal rules that don't belong to the calendar itself (rework-farming-calendar-and-period-crafting,
// lane D): which forage is in season, and the continuous season values the world's look is driven by.
// Dates come from Homestead::Calendar (HomesteadCalendar.h). Plain C++17, no Unreal types.

#include "HomesteadSimulation.h"

namespace Homestead::Seasons
{
// When a forageable has something to gather: a mask of seasons, and for a window that starts or ends
// part-way through a season, the first and last day of the year (0-111) it's in (both inclusive).
struct ForageSeason
{
    SeasonMask seasons = AllSeasons;
    int firstDayOfYear = 0;
    int lastDayOfYear = Calendar::DaysPerYear - 1;
    double renewHours = 0.0; // Regrowth while in season; 0 keeps the kind's default.
};
const ForageSeason& ForageSeasonFor(ResourceKind kind);
// A forageable out of season is present but has nothing to gather.
bool ForageInSeason(ResourceKind kind, double hour);

// The world's seasonal look (MPC_Season), all derived from the clock.
struct Look
{
    double seasonBlend = 0.0; // 0 Spring, 1 Summer, 2 Autumn, 3 Winter, 4 Spring again; each season
                              // eases in over its first TransitionDays.
    double autumn = 0.0;      // 0-1: how far into autumn colour deciduous leaves are.
    double winterBare = 0.0;  // 0-1: how bare deciduous canopies are.
    double frost = 0.0;       // 0-1: morning frost on the ground and low plants.
};
constexpr double TransitionDays = 3.0;
Look LookAt(double hour);
}
