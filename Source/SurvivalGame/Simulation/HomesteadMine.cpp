#include "HomesteadMine.h"

#include "HomesteadBeach.h"
#include "HomesteadCalendar.h"
#include "HomesteadOvergrowth.h"
#include "HomesteadSimulationDetail.h"

#include <algorithm>
#include <cmath>
#include <limits>

#if __has_include("HomesteadMineLayout.inc")
#define HOMESTEAD_MINE_LAYOUT_FILE "HomesteadMineLayout.inc"
#else
#define HOMESTEAD_MINE_LAYOUT_FILE "HomesteadMineLayoutGreybox.inc"
#endif

namespace Homestead::Mine
{
namespace
{
Layout BuildLayout()
{
    Layout layout;
#define MINE_HOUSE(X, Y, Z, YAW) layout.house = {X, Y, Z, YAW};
#define MINE_DOOR(X, Y, Z) layout.door = {X, Y, Z, 0.0};
#define MINE_HOUSE_FLOOR(X, Y, Z) layout.houseFloor = {X, Y, Z, 0.0};
#define MINE_TRAPDOOR(HX, HY, HZ, YAW, LID_LENGTH, LID_WIDTH) layout.trapdoor = {HX, HY, HZ, YAW, LID_LENGTH, LID_WIDTH};
#define MINE_LADDER_TOP(X, Y, Z, YAW) layout.ladderTop = {X, Y, Z, YAW};
#define MINE_LADDER_FOOT(X, Y, Z, YAW) layout.ladderFoot = {X, Y, Z, YAW};
#define MINE_PILE(X, Y, Z) layout.pile = {X, Y, Z, 0.0};
#define MINE_DESK_LAMP(X, Y, Z) layout.deskLamp = {X, Y, Z, 0.0};
#define MINE_DAYLIGHT(X, Y, Z) layout.daylight = {X, Y, Z, 0.0};
#define MINE_NODE(ID, KIND, X, Y, Z, YAW, NX, NY) \
    layout.nodes.push_back({ID, static_cast<NodeKind>(KIND), X, Y, Z, YAW, NX, NY});
#define MINE_FLOOR(X0, Y0, X1, Y1, HALF_WIDTH, Z) layout.floors.push_back({{X0, Y0}, {X1, Y1}, HALF_WIDTH, Z});
#include HOMESTEAD_MINE_LAYOUT_FILE
#undef MINE_HOUSE
#undef MINE_DOOR
#undef MINE_HOUSE_FLOOR
#undef MINE_TRAPDOOR
#undef MINE_LADDER_TOP
#undef MINE_LADDER_FOOT
#undef MINE_PILE
#undef MINE_DESK_LAMP
#undef MINE_DAYLIGHT
#undef MINE_NODE
#undef MINE_FLOOR
    return layout;
}

// Balance's numbers (design section 7): swings are worn / iron / steel / master (master = steel).
const NodeInfo MineNodeTable[] = {
    {{3, 2, 2, 2}, 2.5, 5, 7, 0.0},     // Rock face: stone, back every morning.
    {{2, 1, 1, 1}, 2.0, 2, 3, 24.0},    // Coal seam: back after a day.
    {{3, 2, 2, 2}, 3.0, 2, 2, 48.0},    // Copper vein: back after two days.
    {{4, 3, 2, 2}, 3.0, 1, 2, 96.0},    // Gem pocket: back after four days.
};
// Gem pockets roll quartz or amethyst each time they come back.
constexpr int AmethystChancePercent = 20;

const ResourceNode* MineFindResource(const std::vector<ResourceNode>& nodes, int id)
{
    for (const auto& node : nodes) if (node.id == id) return &node;
    return nullptr;
}

Result MineBad(const std::string& text) { return {false, text, ResultCode::Invalid}; }

double MineDistanceToSegment(Point p, Point a, Point b)
{
    const double dx = b.x - a.x, dy = b.y - a.y;
    const double lengthSquared = dx * dx + dy * dy;
    double t = lengthSquared > 0.0 ? ((p.x - a.x) * dx + (p.y - a.y) * dy) / lengthSquared : 0.0;
    t = std::clamp(t, 0.0, 1.0);
    return std::hypot(p.x - (a.x + dx * t), p.y - (a.y + dy * t));
}

double MineLowestFloorZ(const Layout& layout)
{
    double lowest = std::numeric_limits<double>::max();
    for (const auto& floor : layout.floors) lowest = std::min(lowest, floor.z);
    return layout.floors.empty() ? layout.ladderFoot.z : lowest;
}
}

const Layout& GetLayout()
{
    static const Layout layout = BuildLayout();
    return layout;
}

ResourceKind ResourceFor(NodeKind kind)
{
    switch (kind)
    {
    case NodeKind::Rock: return ResourceKind::MineRockFace;
    case NodeKind::Coal: return ResourceKind::MineCoalSeam;
    case NodeKind::Copper: return ResourceKind::MineCopperVein;
    case NodeKind::Gem: return ResourceKind::MineGemPocket;
    }
    return ResourceKind::MineRockFace;
}

bool IsMineNodeKind(ResourceKind kind)
{
    return kind == ResourceKind::MineRockFace || kind == ResourceKind::MineCoalSeam
        || kind == ResourceKind::MineCopperVein || kind == ResourceKind::MineGemPocket;
}

bool IsMinePlacementId(int id) { return id >= FirstPlacementId && id <= LastPlacementId; }

const Node* FindNode(int id)
{
    for (const auto& node : GetLayout().nodes) if (node.id == id) return &node;
    return nullptr;
}

const NodeInfo& InfoFor(NodeKind kind)
{
    const int index = static_cast<int>(kind);
    return MineNodeTable[index >= 0 && index < 4 ? index : 0];
}

double OutsideFloorBy(Point p)
{
    double best = std::numeric_limits<double>::max();
    for (const auto& floor : GetLayout().floors)
        best = std::min(best, MineDistanceToSegment(p, floor.a, floor.b) - floor.halfWidth);
    return best;
}

bool IsUnderground(Point p, double z)
{
    const Layout& layout = GetLayout();
    if (layout.floors.empty() || !std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(z)) return false;
    if (z >= layout.ladderFoot.z + UndergroundCeilingAboveFloorCm) return false;
    for (const auto& floor : layout.floors)
    {
        const double margin = floor.halfWidth + FootprintMarginCm;
        if (p.x >= std::min(floor.a.x, floor.b.x) - margin && p.x <= std::max(floor.a.x, floor.b.x) + margin
            && p.y >= std::min(floor.a.y, floor.b.y) - margin && p.y <= std::max(floor.a.y, floor.b.y) + margin)
            return true;
    }
    return false;
}

bool IsStuck(Point p, double z)
{
    if (!IsUnderground(p, z)) return false;
    return z < MineLowestFloorZ(GetLayout()) - StuckBelowFloorCm || OutsideFloorBy(p) > StuckSlackCm;
}

Spot StuckTarget() { return GetLayout().ladderFoot; }
}

namespace Homestead
{
namespace
{
using Mine::MineBad;
using Mine::MineFindResource;
}

Result Simulation::CheckMine(int nodeId, Item tool, Point player, double playerZ) const
{
    if (state_.failed) return MineBad("You need to recover. Load your recent checkpoint to continue.");
    const auto* node = MineFindResource(state_.resources, nodeId);
    const auto* spot = Mine::FindNode(nodeId);
    if (!node || !spot || !Mine::IsMineNodeKind(node->kind)) return MineBad("There's nothing to mine here.");
    if (std::hypot(player.x - node->position.x, player.y - node->position.y) > Overgrowth::Reach
        || std::abs(playerZ - spot->z) > Mine::ReachZCm)
        return MineBad("Move closer to mine this.");
    if (node->readyAtHour > state_.hour) return MineBad("Nothing left here until it comes back.");
    if (!IsLampLit()) return MineBad("Too dark to work.");
    if (tool != Item::Pickaxe || Count(Item::Pickaxe) == 0) return MineBad("Needs a pickaxe.");
    return CheckExertion(MineCost(nodeId));
}

int Simulation::MineSwings(int nodeId) const
{
    const auto* spot = Mine::FindNode(nodeId);
    if (!spot) return 0;
    return std::max(1, Mine::InfoFor(spot->kind).swings[static_cast<int>(GetToolTier(ToolKind::Pickaxe))]);
}

double Simulation::MineCost(int nodeId) const
{
    const auto* spot = Mine::FindNode(nodeId);
    if (!spot) return 0.0;
    return Mine::InfoFor(spot->kind).energy * TierEnergyFactor(GetToolTier(ToolKind::Pickaxe));
}

int Simulation::FindAimedMine(Point player, double playerZ, Point facing) const
{
    const double length = std::hypot(facing.x, facing.y);
    if (!std::isfinite(player.x) || !std::isfinite(player.y) || !std::isfinite(playerZ)
        || !std::isfinite(length) || length < 1e-6) return -1;
    const Point forward{facing.x / length, facing.y / length};
    const double cosine = std::cos(Overgrowth::AimHalfAngleDegrees * 3.14159265358979323846 / 180.0);
    int aimed = -1;
    double best = Overgrowth::Reach;
    for (const auto& node : state_.resources)
    {
        const auto* spot = Mine::IsMineNodeKind(node.kind) ? Mine::FindNode(node.id) : nullptr;
        if (!spot || std::abs(playerZ - spot->z) > Mine::ReachZCm) continue;
        const Point offset{node.position.x - player.x, node.position.y - player.y};
        const double distance = std::hypot(offset.x, offset.y);
        if (distance > best || (distance == best && aimed != -1)) continue;
        if (distance > Overgrowth::AimAnyDirection && (offset.x * forward.x + offset.y * forward.y) / distance < cosine) continue;
        aimed = node.id;
        best = distance;
    }
    return aimed;
}

bool Simulation::MineDepleted(int nodeId) const
{
    const auto* node = MineFindResource(state_.resources, nodeId);
    return node && node->readyAtHour > state_.hour;
}

Result Simulation::MineNode(int nodeId, Item tool, Point player, double playerZ)
{
    const auto ready = CheckMine(nodeId, tool, player, playerZ);
    if (!ready) return ready;
    const auto* node = MineFindResource(state_.resources, nodeId);
    const auto* spot = Mine::FindNode(nodeId);
    const Mine::NodeInfo& info = Mine::InfoFor(spot->kind);
    const double cost = MineCost(nodeId);

    // Rolls are stable per node and per day, so a swing never depends on hidden random state.
    const int day = Calendar::DayIndex(state_.hour);
    const int spread = info.maxYield - info.minYield + 1;
    int quantity = info.minYield + Overgrowth::StableRoll(nodeId, day * 4) % spread;
    Item item = Item::Stone;
    switch (spot->kind)
    {
    case Mine::NodeKind::Rock: item = Item::Stone; break;
    case Mine::NodeKind::Coal: item = Item::Coal; break;
    case Mine::NodeKind::Copper: item = Item::CopperOre; break;
    case Mine::NodeKind::Gem:
        item = Item::Quartz;
        if (Overgrowth::StableRoll(nodeId, day * 4 + 2) < Mine::AmethystChancePercent)
        {
            item = Item::Amethyst;
            quantity = 1;
        }
        break;
    }

    State candidate = state_;
    ResourceNode* updated = nullptr;
    for (auto& value : candidate.resources) if (value.id == nodeId) updated = &value;
    updated->readyAtHour = Beach::NextMorning(state_.hour) + info.extraRefillHours;
    if (!Detail::SaveResourceEdit(candidate, *updated))
        return MineBad("The world has reached its 16384 persistent resource edit limit.");

    const int room = Homestead::PackCapacity(candidate) - Detail::PackUsed(candidate);
    const int slot = static_cast<int>(item);
    const int fits = std::max(0, std::min({quantity, room, Homestead::PackCapacity(candidate) - candidate.inventory[slot]}));
    candidate.inventory[slot] += fits;
    const int left = quantity - fits;
    const bool dropped = left > 0 && Detail::AddWorldDrop(candidate, node->position, item, left);
    std::string message;
    if (dropped)
        message = "Pack full: " + std::to_string(left) + " " + ItemName(item) + " left on the ground.";
    else
        message = "Mined +" + std::to_string(quantity) + " " + ItemName(item) + ".";
    auto done = Exert(cost, CommitInventory(std::move(candidate), message.c_str()));
    if (done.ok && dropped) done.code = ResultCode::PackOverflow;
    return done;
}

Result Simulation::WakeAtMine(Point player, double playerZ)
{
    if (state_.failed) return MineBad("You need to recover. Load your recent checkpoint to continue.");
    if (!Mine::IsUnderground(player, playerZ)) return MineBad("You can only sleep it off in the mine.");
    DozeOff(player);
    if (state_.failed) return MineBad("You need to recover. Load your recent checkpoint to continue.");
    state_.energy = 100.0;
    return {true, "You wake in the mine house, rested.", ResultCode::None, ++revision_};
}
}
