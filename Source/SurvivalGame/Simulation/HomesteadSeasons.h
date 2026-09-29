#pragma once

// Seasonal rules that don't belong to the calendar itself (rework-farming-calendar-and-period-crafting,
// lane D): which forage is in season, and the continuous season values the world's look is driven by.
// Plain C++17, no Unreal types.

#include "HomesteadSimulation.h"

namespace Homestead::Seasons
{
constexpr int DaysPerSeason = 28;
constexpr int SeasonCount = 4;
// Seasons in calendar order; a mask bit per season.
enum : unsigned { Spring = 1u << 0, Summer = 1u << 1, Autumn = 1u << 2, Winter = 1u << 3, AllYear = 15u };

// Whole days since Spring 1 of the first year; the day turns over at DayRolloverHour (06:00).
int DayIndex(double hour);
// 0 Spring, 1 Summer, 2 Autumn, 3 Winter.
int SeasonOf(double hour);
// 1-28.
int DayOfSeason(double hour);

// When a forageable has something to gather: a mask of seasons, and for a window that starts or ends
// part-way through a season, the first and last day of the year (0-111) it's in (both inclusive).
struct ForageSeason
{
    unsigned seasons = AllYear;
    int firstDayOfYear = 0;
    int lastDayOfYear = DaysPerSeason * SeasonCount - 1;
    double renewHours = 0.0; // Regrowth while in season; 0 keeps the kind's default.
};
const ForageSeason& ForageSeasonFor(ResourceKind kind);
// A forageable out of season is present but has nothing to gather.
bool InSeason(ResourceKind kind, double hour);

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
