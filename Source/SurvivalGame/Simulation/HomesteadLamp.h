#pragma once

#include "HomesteadSimulation.h"

#include <iosfwd>

// add-oil-lamp: her first light. One lamp and one reservoir of oil, wherever the lamp is; a lamp
// set down is an ordinary world drop of Item::OilLamp. Presentation lives in the Unreal module.
namespace Homestead
{
namespace Lamp
{
// A full lamp burns six game hours (about fifteen real minutes); a night is eleven.
constexpr double CapacityHours = 6.0;
// "The lamp is burning low" below this.
constexpr double LowHours = 1.0;
// A refill refuses when the lamp is already this full.
constexpr double FullEnoughHours = CapacityHours - 0.05;
constexpr int StartingFlasks = 3;
// She sets it down within arm's reach ahead of her (the game uses about 45 cm).
constexpr double SetDownReach = 150.0;

// Optional trailing save section (tag "lamp"): oil hours and the kit flag.
constexpr const char* SaveTag = "lamp";
void WriteSaveSection(std::ostream& output, const State& state);
// Reads the section after its tag.
bool ReadSaveSection(std::istream& input, State& state);
}
}
