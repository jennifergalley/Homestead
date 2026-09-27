#include "HomesteadOvergrowth.h"
#include "HomesteadSimulationDetail.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace Homestead
{
namespace
{
constexpr std::array<int, ToolTierCount> Swings(int worn, int iron, int steel, int master)
{
    return {worn, iron, steel, master};
}
constexpr OvergrowthYield Gives(Item item, int low, int high, int chance = 100) { return {item, low, high, chance}; }

// Tuned for Jenny's first playtest (tasks 4.2): mostly one-swing worn targets, with multi-swing
// stumps and rocks, and the iron and steel kinds visible as teases.
const OvergrowthInfo Table[] = {
    {ResourceKind::TallGrass, ToolKind::Scythe, false, ToolTier::Worn, 0.3, Swings(1, 1, 1, 1),
        {Gives(Item::Hay, 1, 2)}},
    {ResourceKind::Weeds, ToolKind::Scythe, false, ToolTier::Worn, 0.3, Swings(1, 1, 1, 1),
        {Gives(Item::Weeds, 1, 1)}},
    {ResourceKind::BrambleThin, ToolKind::Billhook, false, ToolTier::Worn, 1.2, Swings(1, 1, 1, 1),
        {Gives(Item::BrambleCanes, 2, 3)}},
    {ResourceKind::Sapling, ToolKind::Billhook, false, ToolTier::Worn, 1.5, Swings(2, 1, 1, 1),
        {Gives(Item::Branch, 3, 4), Gives(Item::Kindling, 1, 1)}},
    {ResourceKind::BrambleThicket, ToolKind::Billhook, false, ToolTier::Iron, 2.0, Swings(3, 2, 1, 1),
        {Gives(Item::BrambleCanes, 4, 5)}},
    {ResourceKind::BrambleBank, ToolKind::Billhook, false, ToolTier::Steel, 3.0, Swings(4, 3, 2, 1),
        {Gives(Item::BrambleCanes, 6, 8), Gives(Item::ScrapIron, 1, 1, 25)}},
    {ResourceKind::FallenBranch, ToolKind::Axe, true, ToolTier::Worn, 0.8, Swings(1, 1, 1, 1),
        {Gives(Item::Branch, 3, 3), Gives(Item::Kindling, 1, 1)}},
    {ResourceKind::StumpSmall, ToolKind::Axe, false, ToolTier::Worn, 3.0, Swings(3, 2, 1, 1),
        {Gives(Item::Firewood, 2, 3), Gives(Item::Kindling, 1, 1)}},
    {ResourceKind::StumpLarge, ToolKind::Axe, false, ToolTier::Iron, 4.5, Swings(5, 4, 3, 2),
        {Gives(Item::Timber, 1, 2), Gives(Item::Firewood, 3, 3)}},
    {ResourceKind::StumpAncient, ToolKind::Axe, false, ToolTier::Steel, 6.0, Swings(6, 5, 4, 3),
        {Gives(Item::Timber, 3, 4), Gives(Item::Firewood, 4, 4)}},
    {ResourceKind::FallenLog, ToolKind::Axe, false, ToolTier::Iron, 3.5, Swings(4, 3, 2, 1),
        {Gives(Item::Timber, 2, 3), Gives(Item::Firewood, 2, 2)}},
    {ResourceKind::GiantLog, ToolKind::Axe, false, ToolTier::Steel, 5.0, Swings(6, 5, 4, 3),
        {Gives(Item::Timber, 5, 6)}},
    {ResourceKind::Rubble, ToolKind::Pickaxe, false, ToolTier::Worn, 2.0, Swings(2, 2, 1, 1),
        {Gives(Item::Stone, 2, 3), Gives(Item::ScrapIron, 1, 1, 50), Gives(Item::ScrapLead, 1, 1, 20)}},
    {ResourceKind::SmallRock, ToolKind::Pickaxe, false, ToolTier::Worn, 1.5, Swings(2, 1, 1, 1),
        {Gives(Item::Stone, 2, 3)}},
    {ResourceKind::Boulder, ToolKind::Pickaxe, false, ToolTier::Iron, 4.0, Swings(5, 4, 3, 2),
        {Gives(Item::Stone, 6, 8)}},
    // Searched by hand; the rusted head itself comes from NextSalvageHead.
    {ResourceKind::SalvagePile, ToolKind::Count, true, ToolTier::Worn, 0.5, Swings(1, 1, 1, 1),
        {Gives(Item::ScrapIron, 1, 1)}},
};

const OvergrowthInfo* ByKind(ResourceKind kind)
{
    static const auto index = [] {
        std::array<const OvergrowthInfo*, static_cast<int>(ResourceKind::Count)> result{};
        for (const auto& info : Table) result[static_cast<int>(info.kind)] = &info;
        return result;
    }();
    const int value = static_cast<int>(kind);
    return value >= 0 && value < static_cast<int>(ResourceKind::Count) ? index[value] : nullptr;
}

bool Valid(Point p)
{
    return std::isfinite(p.x) && std::isfinite(p.y) && std::abs(p.x) <= MaxWorldCoordinate
        && std::abs(p.y) <= MaxWorldCoordinate;
}
double DistanceSquared(Point a, Point b) { return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y); }
bool Near(Point a, Point b, double reach) { return Valid(a) && DistanceSquared(a, b) <= reach * reach; }
Result Good(const std::string& text) { return {true, text}; }
Result Bad(const std::string& text) { return {false, text, ResultCode::Invalid}; }

// A stable 0..99 roll for one yield line of one node, so a clear never depends on hidden RNG state.
int Roll(int nodeId, int salt)
{
    std::uint64_t value = static_cast<std::uint64_t>(static_cast<std::uint32_t>(nodeId)) * UINT64_C(0x9E3779B97F4A7C15)
        ^ static_cast<std::uint64_t>(salt + 1) * UINT64_C(0xC2B2AE3D27D4EB4F);
    value ^= value >> 31;
    value *= UINT64_C(0xBF58476D1CE4E5B9);
    value ^= value >> 29;
    return static_cast<int>(value % 100);
}

const ResourceNode* FindNode(const std::vector<ResourceNode>& nodes, int id)
{
    for (const auto& node : nodes) if (node.id == id) return &node;
    return nullptr;
}

std::string Lower(const char* text)
{
    std::string result = text;
    for (char& c : result) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    return result;
}

ToolTier RequiredTier(const OvergrowthInfo& info, const ResourceNode& node)
{
    return std::max(info.minTier, node.minTier);
}

bool CreepCandidate(ResourceKind kind) { return kind == ResourceKind::TallGrass || kind == ResourceKind::Weeds; }
}

const OvergrowthInfo* FindOvergrowth(ResourceKind kind) { return ByKind(kind); }

const char* ToolName(ToolKind tool)
{
    static const char* names[] = {"axe", "hoe", "pail", "scythe", "billhook", "pickaxe"};
    static_assert(sizeof(names) / sizeof(names[0]) == ToolKindCount, "Every tool needs a name.");
    const int value = static_cast<int>(tool);
    return value >= 0 && value < ToolKindCount ? names[value] : "bare hands";
}

const char* ToolTierName(ToolTier tier)
{
    static const char* names[] = {"worn", "iron", "steel", "master-forged"};
    static_assert(sizeof(names) / sizeof(names[0]) == ToolTierCount, "Every tier needs a name.");
    const int value = static_cast<int>(tier);
    return value >= 0 && value < ToolTierCount ? names[value] : "unknown";
}

Item ToolItem(ToolKind tool)
{
    switch (tool)
    {
    case ToolKind::Axe: return Item::Hatchet;
    case ToolKind::Hoe: return Item::DiggingStick;
    case ToolKind::Pail: return Item::WateringCan;
    case ToolKind::Scythe: return Item::Scythe;
    case ToolKind::Billhook: return Item::Billhook;
    case ToolKind::Pickaxe: return Item::Pickaxe;
    default: return Item::Count;
    }
}

ToolKind ToolForItem(Item item)
{
    for (int i = 0; i < ToolKindCount; ++i)
        if (ToolItem(static_cast<ToolKind>(i)) == item) return static_cast<ToolKind>(i);
    return ToolKind::Count;
}

std::string NeedsToolMessage(ToolKind tool, ToolTier tier)
{
    const std::string name = ToolTierName(tier);
    const bool vowel = !name.empty() && std::string("aeiou").find(name[0]) != std::string::npos;
    return std::string("Needs ") + (vowel ? "an " : "a ") + name + " " + ToolName(tool);
}

double TierEnergyFactor(ToolTier tier)
{
    static const double factors[] = {1.0, 0.85, 0.7, 0.55};
    const int value = static_cast<int>(tier);
    return value >= 0 && value < ToolTierCount ? factors[value] : 1.0;
}

Item NextSalvageHead(const State& state)
{
    const auto owned = [&](Item item)
    {
        const int index = static_cast<int>(item);
        if (state.inventory[index] > 0) return true;
        for (const auto& piece : state.structures)
            if (piece.kind == Piece::Chest && piece.storage[index] > 0) return true;
        for (const auto& drop : state.worldDrops)
            if (drop.wearableId == 0 && drop.item == item) return true;
        return false;
    };
    const std::pair<Item, Item> order[] = {
        {Item::RustedBillhookHead, Item::Billhook}, {Item::RustedAxeHead, Item::Hatchet},
        {Item::RustedScytheBlade, Item::Scythe}, {Item::RustedPickHead, Item::Pickaxe},
        {Item::RustedHoeBlade, Item::DiggingStick}};
    for (const auto& entry : order)
        if (!owned(entry.first) && !owned(entry.second)) return entry.first;
    return Item::Count;
}

ToolTier Simulation::GetToolTier(ToolKind tool) const
{
    const int value = static_cast<int>(tool);
    return value >= 0 && value < ToolKindCount ? state_.toolTiers[value] : ToolTier::Worn;
}

Result Simulation::SetToolTier(ToolKind tool, ToolTier tier)
{
    const int value = static_cast<int>(tool);
    if (value < 0 || value >= ToolKindCount || static_cast<int>(tier) < 0 || static_cast<int>(tier) >= ToolTierCount)
        return Bad("Choose a tool and a tier.");
    state_.toolTiers[value] = tier;
    return {true, std::string("Your ") + ToolName(tool) + " is now " + ToolTierName(tier) + ".", ResultCode::None, ++revision_};
}

Result Simulation::CheckOvergrowth(int nodeId, Item tool, Point player) const
{
    if (state_.failed) return Bad("You need to recover. Load your recent checkpoint to continue.");
    const auto* node = FindNode(state_.resources, nodeId);
    const auto* info = node ? FindOvergrowth(node->kind) : nullptr;
    if (!node || !info) return Bad("There's nothing here to clear.");
    if (node->cleared) return Bad("This ground is already cleared.");
    if (!Near(player, node->position, Overgrowth::Reach)) return Bad("Move closer to clear this.");
    const std::string target = Lower(ResourceName(node->kind));
    if (tool == Item::Count)
    {
        if (!info->byHand) return Bad(std::string("Clearing ") + target + " needs a " + ToolName(info->tool) + ".");
    }
    else
    {
        const ToolKind used = ToolForItem(tool);
        if (used == ToolKind::Count || used != info->tool)
        {
            if (info->tool == ToolKind::Count) return Bad(std::string("Search the ") + target + " by hand.");
            return Bad(std::string("A ") + (used == ToolKind::Count ? ItemName(tool) : ToolName(used))
                + " won't clear " + target + ". Use a " + ToolName(info->tool) + ".");
        }
        if (Count(tool) == 0) return Bad(std::string("Take your ") + ToolName(used) + " from storage first.");
        const ToolTier needed = RequiredTier(*info, *node);
        if (GetToolTier(used) < needed)
            return {false, NeedsToolMessage(used, needed), ResultCode::ToolTier, revision_};
    }
    return CheckExertion(OvergrowthCost(nodeId));
}

int Simulation::OvergrowthSwings(int nodeId) const
{
    const auto* node = FindNode(state_.resources, nodeId);
    const auto* info = node ? FindOvergrowth(node->kind) : nullptr;
    if (!info) return 0;
    if (info->tool == ToolKind::Count) return 1;
    return std::max(1, info->swings[static_cast<int>(GetToolTier(info->tool))]);
}

double Simulation::OvergrowthCost(int nodeId) const
{
    const auto* node = FindNode(state_.resources, nodeId);
    const auto* info = node ? FindOvergrowth(node->kind) : nullptr;
    if (!info) return 0.0;
    return info->tool == ToolKind::Count ? info->energy : info->energy * TierEnergyFactor(GetToolTier(info->tool));
}

Result Simulation::ClearOvergrowth(int nodeId, Item tool, Point player)
{
    const auto ready = CheckOvergrowth(nodeId, tool, player);
    if (!ready) return ready;
    const auto* node = FindNode(state_.resources, nodeId);
    const auto& info = *FindOvergrowth(node->kind);
    const double cost = OvergrowthCost(nodeId);

    Inventory yield{};
    for (int line = 0; line < static_cast<int>(info.yields.size()); ++line)
    {
        const auto& give = info.yields[line];
        if (give.item == Item::Count || Roll(nodeId, line * 2) >= give.chancePercent) continue;
        const int spread = std::max(0, give.maxCount - give.minCount);
        yield[static_cast<int>(give.item)] += give.minCount + (spread ? Roll(nodeId, line * 2 + 1) % (spread + 1) : 0);
    }
    if (node->kind == ResourceKind::SalvagePile)
        if (const Item head = NextSalvageHead(state_); head != Item::Count) ++yield[static_cast<int>(head)];

    State candidate = state_;
    ResourceNode* updated = nullptr;
    for (auto& value : candidate.resources) if (value.id == nodeId) updated = &value;
    updated->cleared = true;
    updated->readyAtHour = 0.0;
    if (!Detail::SaveResourceEdit(candidate, *updated))
        return Bad("The world has reached its 16384 persistent resource edit limit.");

    std::string gained, dropped;
    int room = InventoryCapacity - Detail::PackUsed(candidate);
    for (int i = 0; i < ItemCount; ++i)
    {
        if (yield[i] <= 0) continue;
        const Item item = static_cast<Item>(i);
        const int fits = std::max(0, std::min({yield[i], room, InventoryCapacity - candidate.inventory[i]}));
        candidate.inventory[i] += fits;
        room -= fits;
        const std::string line = std::to_string(yield[i]) + " " + ItemName(item);
        gained += (gained.empty() ? "" : ", ") + line;
        if (const int left = yield[i] - fits; left > 0 && Detail::AddWorldDrop(candidate, node->position, item, left))
            dropped += (dropped.empty() ? "" : ", ") + std::to_string(left) + " " + ItemName(item);
    }
    std::string message = std::string(node->kind == ResourceKind::SalvagePile ? "Searched the " : "Cleared the ")
        + Lower(ResourceName(node->kind));
    message += gained.empty() ? "." : ": +" + gained + ".";
    if (!dropped.empty()) message += " Your pack is full, so " + dropped + " lie on the ground.";
    return Exert(cost, CommitInventory(std::move(candidate), message.c_str()));
}

int Simulation::FindNearestOvergrowth(Point position, double maxDistance, Item tool) const
{
    if (!Valid(position) || !std::isfinite(maxDistance) || maxDistance < 0 || maxDistance > 12000) return -1;
    const ToolKind used = ToolForItem(tool);
    int nearest = -1;
    double best = maxDistance * maxDistance;
    for (const auto& node : state_.resources)
    {
        const auto* info = node.cleared ? nullptr : FindOvergrowth(node.kind);
        if (!info) continue;
        const bool handles = tool == Item::Count ? info->byHand : info->tool == used;
        const double distance = DistanceSquared(position, node.position);
        if (handles && (distance < best || (distance == best && nearest == -1)))
        {
            nearest = node.id;
            best = distance;
        }
    }
    return nearest;
}

double Simulation::ScytheArcRadius(ToolTier tier)
{
    static const double radii[] = {160.0, 200.0, 240.0, 280.0};
    const int value = static_cast<int>(tier);
    return value >= 0 && value < ToolTierCount ? radii[value] : radii[0];
}

double Simulation::ScytheArcHalfAngle(ToolTier tier)
{
    static const double halves[] = {70.0, 80.0, 90.0, 100.0};
    const int value = static_cast<int>(tier);
    return value >= 0 && value < ToolTierCount ? halves[value] : halves[0];
}

std::vector<int> Simulation::ScytheArcTargets(Point player, Point facing) const
{
    std::vector<std::pair<double, int>> found;
    const double length = std::sqrt(facing.x * facing.x + facing.y * facing.y);
    if (!Valid(player) || !std::isfinite(length) || length < 1e-6) return {};
    const Point forward{facing.x / length, facing.y / length};
    const ToolTier tier = GetToolTier(ToolKind::Scythe);
    const double radius = ScytheArcRadius(tier);
    const double cosine = std::cos(ScytheArcHalfAngle(tier) * 3.14159265358979323846 / 180.0);
    for (const auto& node : state_.resources)
    {
        const auto* info = node.cleared ? nullptr : FindOvergrowth(node.kind);
        if (!info || info->tool != ToolKind::Scythe) continue;
        const Point offset{node.position.x - player.x, node.position.y - player.y};
        const double distance = std::sqrt(offset.x * offset.x + offset.y * offset.y);
        if (distance > radius) continue;
        // Right at her feet counts as in front of her.
        if (distance > 40.0 && (offset.x * forward.x + offset.y * forward.y) / distance < cosine) continue;
        found.push_back({distance, node.id});
    }
    std::sort(found.begin(), found.end());
    std::vector<int> result;
    for (const auto& entry : found) result.push_back(entry.second);
    return result;
}

void Simulation::CreepWeeds(int day)
{
    std::vector<const ResourceNode*> standing;
    for (const auto& node : state_.resources)
        if (!node.cleared && IsOvergrowth(node.kind) && node.kind != ResourceKind::SalvagePile) standing.push_back(&node);
    if (standing.empty()) return;
    std::vector<int> regrown;
    const double neighbour = Overgrowth::CreepNeighbourDistance * Overgrowth::CreepNeighbourDistance;
    for (const auto& node : state_.resources)
    {
        if (!node.cleared || !CreepCandidate(node.kind)) continue;
        if (Roll(node.id ^ (day * 7919), 97) >= static_cast<int>(Overgrowth::CreepChance * 100.0)) continue;
        if (std::none_of(standing.begin(), standing.end(), [&](const ResourceNode* other)
            { return DistanceSquared(other->position, node.position) <= neighbour; })) continue;
        // Tilled or built-on ground never regrows overgrowth.
        const Footprint spot{node.position, {40.0, 40.0}, 0.0};
        bool covered = false;
        for (const auto& plot : state_.plots)
            covered = covered || FootprintsOverlap(spot, {PlotCenter(plot), {GardenCellSize * 0.5, GardenCellSize * 0.5}, 0.0});
        for (const auto& structure : state_.structures)
            covered = covered || FootprintsOverlap(spot, StructureFootprint(state_, structure));
        if (!covered) regrown.push_back(node.id);
    }
    if (regrown.empty()) return;
    for (auto& node : state_.resources)
    {
        if (std::find(regrown.begin(), regrown.end(), node.id) == regrown.end()) continue;
        node.cleared = false;
        node.readyAtHour = 0.0;
        Detail::EraseResourceEdit(state_, node.key);
    }
    ++revision_;
}
}
