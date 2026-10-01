#include "HomesteadFood.h"

#include "HomesteadShops.h"
#include "HomesteadSimulation.h"

#include <algorithm>
#include <cmath>
#include <istream>
#include <ostream>

namespace Homestead
{
namespace Food
{
bool EnergyFull(const State& state) { return state.energy >= FullEnergyAt; }

bool IsWellFed(const State& state) { return state.hour < state.wellFedUntilHour; }

std::string EstateRefusal(const State& state, Item item)
{
    const FoodClass food = FoodClassOf(item);
    if (food == FoodClass::None)
        return item == Item::Roots ? std::string("Raw roots need cooking first.")
            : std::string(ItemName(item)) + " isn't something to eat.";
    if (!EnergyFull(state)) return {};
    if (food == FoodClass::Snack) return "You're full of energy. Save it for later.";
    // A Meal at full Energy: only if it starts Well fed or extends it by an hour or more, so a meal
    // before a big job still works but one can't be wasted by accident.
    if (!IsWellFed(state) || state.hour + WellFedHours >= state.wellFedUntilHour + MinExtensionHours) return {};
    return "You're full, and already well fed until " + FormatHour(state.wellFedUntilHour) + ". Save it for later.";
}

std::string EatOnEstate(State& state, Item item)
{
    const bool wasFull = EnergyFull(state);
    const double gain = std::min(100.0, state.energy + GetItemInfo(item).energy) - state.energy;
    state.energy += gain;
    const std::string energy = "+" + std::to_string(std::max(gain > 0.0 ? 1L : 0L, std::lround(gain))) + " Energy";
    if (FoodClassOf(item) != FoodClass::Meal) return energy;
    state.wellFedUntilHour = state.hour + WellFedHours;
    const std::string until = "Well fed until " + FormatHour(state.wellFedUntilHour);
    return wasFull ? "Your energy was already full. " + until + "." : energy + " \xC2\xB7 " + until;
}

std::string EffectLabel(const State& state, Item item)
{
    const std::string energy = FoodEnergyLabel(item);
    if (energy.empty() || !state.fixedEstate || FoodClassOf(item) != FoodClass::Meal) return energy;
    return energy + " \xC2\xB7 Well fed until " + FormatHour(state.hour + WellFedHours);
}

std::string PackUseText(const State& state, Item item)
{
    if (!IsEdible(item)) return {};
    const std::string energy = FoodEnergyLabel(item);
    std::string text = energy.empty() ? std::string("Food.") : "Food: " + energy + " each.";
    if (state.fixedEstate && FoodClassOf(item) == FoodClass::Meal)
        text += " Well fed until " + FormatHour(state.hour + WellFedHours) + ".";
    return text + " Eat one from your pack.";
}

std::string WellFedBadge(const State& state)
{
    return IsWellFed(state) ? "Well fed until " + FormatHour(state.wellFedUntilHour) : std::string();
}

void WriteSaveSection(std::ostream& output, const State& state)
{
    if (IsWellFed(state)) output << SaveTag << ' ' << state.wellFedUntilHour << '\n';
}

bool ReadSaveSection(std::istream& input, State& state)
{
    double until = 0.0;
    if (!(input >> until) || !std::isfinite(until) || until > state.hour + WellFedHours) return false;
    // At or before the save's hour it has simply run out.
    state.wellFedUntilHour = until > state.hour ? until : 0.0;
    return true;
}
}
}
