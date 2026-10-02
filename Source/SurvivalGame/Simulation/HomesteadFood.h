#pragma once

#include "HomesteadItems.h"

#include <iosfwd>
#include <string>

// One Energy bar with Well fed (rework-farming-calendar-and-period-crafting design §3a). On the
// estate she has no hunger: food restores Energy only, from its catalogue row. A Meal also makes her
// Well fed for a flat WellFedHours, during which every piece of work costs WellFedWorkFactor of its
// Energy (Simulation::WorkCost). Eating a Meal sets the expiry to now + WellFedHours: it refreshes,
// never stacks. The seeded woodland keeps its legacy hunger and eating rules.
namespace Homestead
{
struct State;

namespace Food
{
constexpr double WellFedHours = 3.0;
constexpr double WellFedWorkFactor = 0.85;
// At full Energy a Meal is still eaten if it starts Well fed or extends it by at least this much.
constexpr double MinExtensionHours = 1.0;
// "Full" for eating: the bar reads 100. Awake, Energy drains 0.6 an hour (Exertion::AwakePerHour), so an
// exact 100 lasts a moment and a snack at 99.99 would be eaten for nothing (PIE, 09-30).
constexpr double FullEnergyAt = 99.5;
// Optional trailing save section, written only while she is Well fed: "wellfed <expiry hour>".
constexpr const char* SaveTag = "wellfed";

// Her Energy is at the top of the bar.
bool EnergyFull(const State& state);
// While state.hour < state.wellFedUntilHour.
bool IsWellFed(const State& state);
// Why she won't eat this on the estate right now ("You're full of energy. Save it for later."), or
// empty when she will. Nothing is consumed on a refusal.
std::string EstateRefusal(const State& state, Item item);
// Applies one of the food on the estate (Energy, and Well fed for a Meal) and returns the toast:
// "+12 Energy", "+40 Energy · Well fed until 2:30 PM" or "Your energy was already full. Well fed until
// 2:30 PM." Call only after EstateRefusal came back empty.
std::string EatOnEstate(State& state, Item item);

// What one would do if she ate it now, for the shop and the pack before she eats: "+12 Energy" for a
// Snack, and on the estate "+40 Energy · Well fed until 2:30 PM" for a Meal (the clock time is now +
// WellFedHours; the book and shop pause the clock while she reads it). Empty for anything not food.
std::string EffectLabel(const State& state, Item item);
// The pack hover's use line for food: "Food: +12 Energy each. Eat one from your pack.", and for a
// Meal on the estate "Food: +40 Energy each. Well fed until 2:30 PM. Eat one from your pack." Empty
// for anything that isn't food.
std::string PackUseText(const State& state, Item item);
// The HUD's Well fed badge: "Well fed until 2:30 PM" while it lasts, otherwise empty.
std::string WellFedBadge(const State& state);

void WriteSaveSection(std::ostream& output, const State& state);
// Reads the section after its tag. A non-finite expiry, or one past the save's hour + WellFedHours
// (no meal lasts longer), is corrupt: returns false. One at or before the hour has simply run out.
bool ReadSaveSection(std::istream& input, State& state);
}
}
