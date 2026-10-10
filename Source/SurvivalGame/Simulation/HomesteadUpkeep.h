#pragma once

// The estate's upkeep loop (openspec upkeep-regrowth-foraging-stumps): each morning a little of the
// cleared ground near the manor and farm grows over again, and the wind drops the odd branch, so
// keeping the property tidy is an ongoing chore rather than a one-off. Tuned with the Balance Agent.
// Regrowth only revives overgrowth placements she has already cleared, so it can never land on a path,
// doorway or forecourt that was not overgrown to begin with, and never on tilled or built-on ground.
namespace Homestead::Upkeep
{
// Only ground this close to the manor or the derelict farm regrows, cm.
constexpr double RegrowRadiusCm = 9000.0;
// Daily chance (percent) that one cleared grass, weed, nettle or thin-bramble spot grows back.
constexpr int RegrowChancePercent = 4;
// The same chance for spots within EdgeReachCm of a plot or structure (a fence line or bed edge), which
// is where the weeds creep in first.
constexpr int EdgeRegrowChancePercent = 8;
constexpr double EdgeReachCm = 600.0;
// The most spots that regrow in one morning, so a day brings about one chore, not a flood.
constexpr int MaxRegrowthsPerDay = 4;
// Wind-blown branches: daily chance (percent) per dormant windfall spot, the most dropped in one morning,
// and the most windfall branches lying at once.
constexpr int WindfallChancePercent = 8;
constexpr int MaxWindfallsPerDay = 2;
constexpr int MaxWindfallsLying = 10;
}
