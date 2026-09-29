#include "HomesteadLamp.h"

#include <algorithm>
#include <cmath>
#include <istream>
#include <ostream>

namespace Homestead
{
const WorldDrop* Simulation::SetDownLampDrop() const
{
    for (const auto& drop : state_.worldDrops)
        if (drop.wearableId == 0 && drop.item == Item::OilLamp && drop.quantity > 0) return &drop;
    return nullptr;
}

bool Simulation::IsLampLit(bool sleeping) const
{
    if (state_.lampOilHours <= 0.0) return false;
    return SetDownLampDrop() || (!sleeping && IsLampInHand());
}

void Simulation::BurnLamp(double hours, bool sleeping)
{
    if (hours <= 0.0 || !IsLampLit(sleeping)) return;
    state_.lampOilHours = std::max(0.0, state_.lampOilHours - hours);
}

void Simulation::SetLampOil(double hours)
{
    if (!std::isfinite(hours)) return;
    state_.lampOilHours = std::clamp(hours, 0.0, Lamp::CapacityHours);
    ++revision_;
}

Result Simulation::RefillLamp()
{
    if (state_.failed) return {false, "You need to recover first.", ResultCode::Unavailable, revision_};
    if (Count(Item::OilLamp) == 0)
        return {false, SetDownLampDrop() ? "Pick up the lamp to fill it." : "You have no lamp to fill.", ResultCode::Invalid, revision_};
    if (state_.lampOilHours >= Lamp::FullEnoughHours)
        return {false, "The lamp is already full.", ResultCode::Invalid, revision_};
    if (Count(Item::OilFlask) == 0)
        return {false, "You have no oil. Oil flasks are sold at the general store.", ResultCode::Invalid, revision_};
    Inventory change{};
    change[static_cast<int>(Item::OilFlask)] = -1;
    if (!TryAdjust(change)) return {false, "You have no oil. Oil flasks are sold at the general store.", ResultCode::Invalid, revision_};
    state_.lampOilHours = Lamp::CapacityHours;
    return {true, "Filled the lamp.", ResultCode::None, revision_};
}

Result Simulation::SetDownLamp(Point position, Point player)
{
    if (state_.failed) return {false, "You need to recover first.", ResultCode::Unavailable, revision_};
    const auto carried = [](const LayoutEntry& value) { return value.item == Item::OilLamp && value.wearableId == 0 && value.quantity > 0; };
    if (std::none_of(state_.inventoryLayout.begin(), state_.inventoryLayout.end(), carried))
        return {false, "You aren't carrying the lamp.", ResultCode::Invalid, revision_};
    // Unlike other dropped things, a lamp may stand on a floor, a hearthstone or by a wall.
    const double dx = position.x - player.x, dy = position.y - player.y;
    if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(player.x) || !std::isfinite(player.y)
        || std::abs(position.x) > MaxWorldCoordinate || std::abs(position.y) > MaxWorldCoordinate
        || dx * dx + dy * dy > Lamp::SetDownReach * Lamp::SetDownReach)
        return {false, "Set the lamp down closer to you.", ResultCode::Invalid, revision_};
    if (NearWater(position)) return {false, "Set the lamp on dry ground.", ResultCode::Invalid, revision_};
    if (state_.worldDrops.size() >= MaxWorldDrops || state_.nextId >= TransientResourceIdBase - 1)
        return {false, "Too many possessions are already resting in the world. Pick one up first.", ResultCode::Capacity, revision_};
    State candidate = state_;
    auto entry = std::find_if(candidate.inventoryLayout.begin(), candidate.inventoryLayout.end(), carried);
    --entry->quantity;
    --candidate.inventory[static_cast<int>(Item::OilLamp)];
    candidate.worldDrops.push_back({candidate.nextId++, position, Item::OilLamp, 1, 0});
    return CommitInventory(std::move(candidate), state_.lampOilHours > 0.0 ? "Set the lamp down." : "Set the lamp down. It has no oil.");
}

void Simulation::GrantLampKit()
{
    if (!state_.fixedEstate || state_.lampKitGranted || state_.failed) return;
    Inventory change{};
    change[static_cast<int>(Item::OilLamp)] = 1;
    change[static_cast<int>(Item::OilFlask)] = Lamp::StartingFlasks;
    // A full pack keeps the flag clear, so the kit arrives on a later load.
    if (!TryAdjust(change)) return;
    state_.lampOilHours = Lamp::CapacityHours;
    state_.lampKitGranted = true;
}

namespace Lamp
{
void WriteSaveSection(std::ostream& output, const State& state)
{
    output << SaveTag << ' ' << state.lampOilHours << ' ' << (state.lampKitGranted ? 1 : 0) << '\n';
}

bool ReadSaveSection(std::istream& input, State& state)
{
    double oil = 0.0;
    int granted = -1;
    if (!(input >> oil >> granted) || !std::isfinite(oil) || oil < 0.0 || oil > CapacityHours || (granted != 0 && granted != 1))
        return false;
    state.lampOilHours = oil;
    state.lampKitGranted = granted == 1;
    return true;
}
}
}
