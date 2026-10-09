#include "HomesteadUpkeep.h"
#include "HomesteadEstate.h"
#include "HomesteadOvergrowth.h"
#include "HomesteadSimulationDetail.h"
#include "HomesteadTreeFelling.h"

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

namespace Homestead
{
namespace
{
bool RegrowsNearHome(ResourceKind kind)
{
    return kind == ResourceKind::TallGrass || kind == ResourceKind::Weeds || kind == ResourceKind::Nettles
        || kind == ResourceKind::BrambleThin;
}

// A stable order for picking among the spots that passed their roll, different each day.
int OrderKey(int id, int day)
{
    return Overgrowth::StableRoll(id ^ (day * 104729), 211) * 100 + Overgrowth::StableRoll(id, day % 997 + 300);
}

bool RollsUnder(int id, int day, int salt, int percent)
{
    return Overgrowth::StableRoll(id ^ (day * 7919), salt) < percent;
}
}

void Simulation::UpkeepRegrowth(int day)
{
    if (!state_.fixedEstate) return;
    std::vector<std::pair<int, int>> regrow; // (order key, node id)
    std::vector<std::pair<int, int>> windfalls;
    int lying = 0;
    for (const auto& node : state_.resources)
    {
        if (IsWindfallPlacement(node.id))
        {
            if (!node.cleared) ++lying;
            else if (RollsUnder(node.id, day, 131, Upkeep::WindfallChancePercent)
                && !Overgrowth::GroundBuiltOn(state_, node.position, 90.0)) windfalls.emplace_back(OrderKey(node.id, day), node.id);
            continue;
        }
        if (!node.cleared || !RegrowsNearHome(node.kind)) continue;
        const auto* info = FindOvergrowth(node.kind);
        const double spoil = info ? info->spoil : 40.0;
        if (!RollsUnder(node.id, day, 97, Upkeep::EdgeRegrowChancePercent)) continue;
        if (Overgrowth::GroundBuiltOn(state_, node.position, spoil)) continue;
        const bool edge = Overgrowth::GroundBuiltOn(state_, node.position, spoil + Upkeep::EdgeReachCm);
        if (!edge && !RollsUnder(node.id, day, 97, Upkeep::RegrowChancePercent)) continue;
        if (TreeFelling::HomeDistanceCm(node.position) > Upkeep::RegrowRadiusCm) continue;
        regrow.emplace_back(OrderKey(node.id, day), node.id);
    }
    std::sort(regrow.begin(), regrow.end());
    if (static_cast<int>(regrow.size()) > Upkeep::MaxRegrowthsPerDay) regrow.resize(Upkeep::MaxRegrowthsPerDay);
    std::sort(windfalls.begin(), windfalls.end());
    const int room = std::max(0, std::min(Upkeep::MaxWindfallsPerDay, Upkeep::MaxWindfallsLying - lying));
    if (static_cast<int>(windfalls.size()) > room) windfalls.resize(room);
    if (regrow.empty() && windfalls.empty()) return;

    for (auto& node : state_.resources)
    {
        const bool grows = std::any_of(regrow.begin(), regrow.end(), [&](const auto& pick) { return pick.second == node.id; });
        const bool blows = std::any_of(windfalls.begin(), windfalls.end(), [&](const auto& pick) { return pick.second == node.id; });
        if (!grows && !blows) continue;
        node.cleared = false;
        node.readyAtHour = 0.0;
        // A windfall spot is dormant by default, so waking it needs an explicit saved edit; ordinary overgrowth
        // is standing by default, so its edit is simply dropped.
        if (blows)
        {
            if (!Detail::SaveResourceEdit(state_, node)) node.cleared = true;
        }
        else Detail::EraseResourceEdit(state_, node.key);
    }
    ++revision_;
}
}
