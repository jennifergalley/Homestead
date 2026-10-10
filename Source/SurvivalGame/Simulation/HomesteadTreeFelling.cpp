#include "HomesteadTreeFelling.h"
#include "HomesteadEstate.h"
#include "HomesteadEstatePublicRoad.h"
#include "HomesteadOvergrowth.h"
#include "HomesteadSimulationDetail.h"

#include <algorithm>
#include <cmath>
#include <istream>
#include <limits>
#include <ostream>

namespace Homestead
{
namespace
{
double SegmentDistance(Point p, Point a, Point b)
{
    const double dx = b.x - a.x, dy = b.y - a.y;
    const double lengthSquared = dx * dx + dy * dy;
    const double t = lengthSquared > 0.0
        ? std::clamp(((p.x - a.x) * dx + (p.y - a.y) * dy) / lengthSquared, 0.0, 1.0) : 0.0;
    return std::hypot(p.x - (a.x + t * dx), p.y - (a.y + t * dy));
}

double DistanceToRing(const std::vector<Point>& ring, Point p)
{
    if (ring.size() < 3) return std::numeric_limits<double>::infinity();
    if (PointInPolygon(ring, p)) return 0.0;
    double best = std::numeric_limits<double>::infinity();
    for (size_t i = 0; i < ring.size(); ++i) best = std::min(best, SegmentDistance(p, ring[i], ring[(i + 1) % ring.size()]));
    return best;
}

bool Before(const FelledTree& entry, int xCm, int yCm)
{
    return entry.xCm < xCm || (entry.xCm == xCm && entry.yCm < yCm);
}

double HomeDistance(Point tree)
{
    const EstateLayout& layout = ProvisionalEstateLayout();
    double best = std::numeric_limits<double>::infinity();
    if (const auto* manor = layout.FindPolygon(Anchor::ManorFootprint)) best = DistanceToRing(manor->points, tree);
    const DerelictFarmPlan farm = EstateDerelictFarm(layout);
    if (farm.valid)
    {
        const double dx = std::max({farm.southWest.x - tree.x, 0.0, tree.x - (farm.southWest.x + farm.lengthU)});
        const double dy = std::max({farm.southWest.y - tree.y, 0.0, tree.y - (farm.southWest.y + farm.lengthV)});
        best = std::min(best, std::hypot(dx, dy));
    }
    return best;
}
}

namespace TreeFelling
{
int Cm(double value) { return static_cast<int>(std::lround(value)); }

const FelledTree* Find(const State& state, Point tree)
{
    const int x = Cm(tree.x), y = Cm(tree.y);
    const auto at = std::lower_bound(state.felledTrees.begin(), state.felledTrees.end(), 0,
        [x, y](const FelledTree& entry, int) { return Before(entry, x, y); });
    return at != state.felledTrees.end() && at->xCm == x && at->yCm == y ? &*at : nullptr;
}

Stage StageOf(const FelledTree& entry, double hour)
{
    const double age = hour - entry.fellHour;
    if (entry.cleared) return Stage::Cleared;
    if (!entry.regrows) return Stage::Stump;
    if (age >= RegrowHours) return Stage::Standing;
    return age >= StumpHours ? Stage::Sapling : Stage::Stump;
}

Stage StageOf(const State& state, Point tree)
{
    const FelledTree* entry = Find(state, tree);
    return entry ? StageOf(*entry, state.hour) : Stage::Standing;
}

bool Regrows(Point tree) { return HomeDistance(tree) >= RegrowFromHomeCm; }

double HomeDistanceCm(Point point) { return HomeDistance(point); }

const char* ProtectedReason(Point tree)
{
    if (EstatePublicRoad().NearestTo(tree).distanceCm < RoadVergeCm) return "This tree is part of the village road.";
    if (const auto* square = ProvisionalEstateLayout().FindLandmark(Anchor::TownSquare);
        square && std::hypot(tree.x - square->position.x, tree.y - square->position.y) < TownSquareCm)
        return "This tree is part of the village.";
    return "";
}

double NextChangeHour(const State& state)
{
    double next = std::numeric_limits<double>::max();
    for (const FelledTree& entry : state.felledTrees)
    {
        if (!entry.regrows) continue;
        const double sapling = entry.fellHour + StumpHours, grown = entry.fellHour + RegrowHours;
        if (sapling > state.hour) next = std::min(next, sapling);
        if (grown > state.hour) next = std::min(next, grown);
    }
    return next;
}

void Prune(State& state)
{
    const double hour = state.hour;
    state.felledTrees.erase(std::remove_if(state.felledTrees.begin(), state.felledTrees.end(),
        [hour](const FelledTree& entry) { return StageOf(entry, hour) == Stage::Standing; }), state.felledTrees.end());
}

void WriteSaveSection(std::ostream& output, const State& state)
{
    if (state.felledTrees.empty()) return;
    output << SaveTag << ' ' << state.felledTrees.size();
    for (const FelledTree& entry : state.felledTrees)
        output << ' ' << entry.xCm << ' ' << entry.yCm << ' ' << entry.fellHour << ' '
               << (entry.cleared ? 2 : entry.regrows ? 1 : 0);
    output << '\n';
}

bool ReadSaveSection(std::istream& input, State& state)
{
    int count = 0;
    if (!state.fixedEstate || !(input >> count) || count < 0 || count > MaxFelled) return false;
    std::vector<FelledTree> entries;
    entries.reserve(count);
    for (int index = 0; index < count; ++index)
    {
        FelledTree entry;
        int state = 0;
        if (!(input >> entry.xCm >> entry.yCm >> entry.fellHour >> state) || state < 0 || state > 2
            || !std::isfinite(entry.fellHour) || std::abs(entry.xCm) > 1000000 || std::abs(entry.yCm) > 1000000
            || (!entries.empty() && !Before(entries.back(), entry.xCm, entry.yCm))) return false;
        entry.regrows = state == 1;
        entry.cleared = state == 2;
        entries.push_back(entry);
    }
    state.felledTrees = std::move(entries);
    return true;
}
}

Result Simulation::CheckFellSceneryTree(Point tree, Point player) const
{
    const auto refuse = [this](const char* text) { return Result{false, text, ResultCode::Invalid, revision_}; };
    if (state_.failed) return refuse("You need to recover. Load your recent checkpoint to continue.");
    if (!state_.fixedEstate) return refuse("There is nothing here to fell.");
    if (Count(Item::Hatchet) == 0) return refuse("Craft an axe before felling trees.");
    if (!std::isfinite(tree.x) || !std::isfinite(tree.y)
        || std::hypot(tree.x - player.x, tree.y - player.y) > TreeFelling::ReachCm) return refuse("Move closer to fell this tree.");
    if (const char* reason = TreeFelling::ProtectedReason(tree); *reason) return refuse(reason);
    if (const auto* entry = TreeFelling::Find(state_, tree);
        entry && TreeFelling::StageOf(*entry, state_.hour) != TreeFelling::Stage::Standing)
        return refuse("This tree is already down.");
    if (auto ready = CheckExertion(Exertion::FellEnergy); !ready) return ready;
    return {true, "", ResultCode::None, revision_};
}

Result Simulation::FellSceneryTree(Point tree, Point player)
{
    if (const auto check = CheckFellSceneryTree(tree, player); !check) return check;
    State candidate = state_;
    TreeFelling::Prune(candidate);
    if (static_cast<int>(candidate.felledTrees.size()) >= TreeFelling::MaxFelled)
        return {false, "Too many felled trees are waiting to regrow.", ResultCode::Capacity, revision_};
    const FelledTree entry{TreeFelling::Cm(tree.x), TreeFelling::Cm(tree.y), state_.hour, TreeFelling::Regrows(tree)};
    const auto at = std::lower_bound(candidate.felledTrees.begin(), candidate.felledTrees.end(), 0,
        [&entry](const FelledTree& other, int) { return other.xCm < entry.xCm || (other.xCm == entry.xCm && other.yCm < entry.yCm); });
    candidate.felledTrees.insert(at, entry);
    // The felled-tree yield is the sim forest tree's (Yield(ForestTree)).
    candidate.inventory[static_cast<int>(Item::Timber)] += 6;
    candidate.inventory[static_cast<int>(Item::Branch)] += 4;
    return Exert(Exertion::FellEnergy, CommitInventory(std::move(candidate), "Tree felled."));
}

namespace
{
double ClearStumpCost(const Simulation& sim)
{
    return Exertion::ClearStumpEnergy * TierEnergyFactor(sim.GetToolTier(ToolKind::Axe));
}
}

Result Simulation::CheckClearSceneryStump(Point tree, Point player) const
{
    const auto refuse = [this](const char* text) { return Result{false, text, ResultCode::Invalid, revision_}; };
    if (state_.failed) return refuse("You need to recover. Load your recent checkpoint to continue.");
    if (!state_.fixedEstate) return refuse("There is no stump here to clear.");
    if (Count(Item::Hatchet) == 0) return refuse("Craft an axe before clearing stumps.");
    if (!std::isfinite(tree.x) || !std::isfinite(tree.y)
        || std::hypot(tree.x - player.x, tree.y - player.y) > TreeFelling::ReachCm) return refuse("Move closer to clear this stump.");
    const auto* entry = TreeFelling::Find(state_, tree);
    if (!entry || TreeFelling::StageOf(*entry, state_.hour) == TreeFelling::Stage::Standing) return refuse("There is no stump here to clear.");
    if (entry->cleared) return refuse("This ground is already cleared.");
    if (entry->regrows) return refuse("This far from home the stump grows back on its own.");
    if (auto ready = CheckExertion(ClearStumpCost(*this)); !ready) return ready;
    return {true, "", ResultCode::None, revision_};
}

Result Simulation::ClearSceneryStump(Point tree, Point player)
{
    if (const auto check = CheckClearSceneryStump(tree, player); !check) return check;
    State candidate = state_;
    const int x = TreeFelling::Cm(tree.x), y = TreeFelling::Cm(tree.y);
    const auto at = std::lower_bound(candidate.felledTrees.begin(), candidate.felledTrees.end(), 0,
        [x, y](const FelledTree& other, int) { return other.xCm < x || (other.xCm == x && other.yCm < y); });
    at->cleared = true;

    // Rolled from the trunk position so a clear never depends on hidden RNG state.
    const int id = x * 73856 ^ y * 19349;
    Inventory yield{};
    const int firewoodSpread = TreeFelling::ClearStumpFirewoodMax - TreeFelling::ClearStumpFirewoodMin + 1;
    yield[static_cast<int>(Item::Firewood)] = TreeFelling::ClearStumpFirewoodMin + Overgrowth::StableRoll(id, 0) % firewoodSpread;
    yield[static_cast<int>(Item::Kindling)] = TreeFelling::ClearStumpKindling;

    std::string gained, dropped;
    int room = Homestead::PackCapacity(candidate) - Detail::PackUsed(candidate);
    for (int i = 0; i < ItemCount; ++i)
    {
        if (yield[i] <= 0) continue;
        const Item item = static_cast<Item>(i);
        const int fits = std::max(0, std::min({yield[i], room, Homestead::PackCapacity(candidate) - candidate.inventory[i]}));
        candidate.inventory[i] += fits;
        room -= fits;
        gained += (gained.empty() ? "" : ", ") + std::to_string(yield[i]) + " " + ItemName(item);
        if (const int left = yield[i] - fits; left > 0 && Detail::AddWorldDrop(candidate, tree, item, left))
            dropped += (dropped.empty() ? "" : ", ") + std::to_string(left) + " " + ItemName(item);
    }
    std::string message = "Cleared the stump: +" + gained + ".";
    if (!dropped.empty()) message = "Pack full: " + dropped + " left on the ground.";
    auto done = Exert(ClearStumpCost(*this), CommitInventory(std::move(candidate), message.c_str()));
    if (done.ok && !dropped.empty()) done.code = ResultCode::PackOverflow;
    return done;
}
}
