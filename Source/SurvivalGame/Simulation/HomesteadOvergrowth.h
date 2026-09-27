#pragma once

#include "HomesteadSimulation.h"

#include <array>
#include <string>

// The estate's layered overgrowth (add-overgrown-estate-clearing): which tool and tier clears each
// kind, what it costs and what it yields. The Simulation transactions live in HomesteadOvergrowth.cpp.
namespace Homestead
{
struct OvergrowthYield
{
    Item item = Item::Count;
    int minCount = 0;
    int maxCount = 0;
    int chancePercent = 100; // Rolled once per clear, deterministically from the node id.
};

struct OvergrowthInfo
{
    ResourceKind kind = ResourceKind::Count;
    ToolKind tool = ToolKind::Count; // Count: bare hands only.
    bool byHand = false; // Also clearable with bare hands (one swing).
    ToolTier minTier = ToolTier::Worn;
    double energy = 0.0; // At worn tier; better tools cost less.
    std::array<int, ToolTierCount> swings{}; // Swings to clear at each tool tier.
    std::array<OvergrowthYield, 3> yields{};
};

// Null for kinds that aren't overgrowth (trees, forage).
const OvergrowthInfo* FindOvergrowth(ResourceKind kind);
inline bool IsOvergrowth(ResourceKind kind) { return FindOvergrowth(kind) != nullptr; }
// Lower-case, for prompts: "axe", "billhook".
const char* ToolName(ToolKind tool);
// Lower-case: "worn", "iron", "steel", "master-forged".
const char* ToolTierName(ToolTier tier);
Item ToolItem(ToolKind tool);
// ToolKind::Count when the item isn't a tiered tool.
ToolKind ToolForItem(Item item);
// "Needs an iron axe".
std::string NeedsToolMessage(ToolKind tool, ToolTier tier);
// The rusted head a salvage pile gives next: the first of billhook, axe, scythe, pickaxe and hoe
// she owns neither as a head nor as a hafted tool; Item::Count once she has all five.
Item NextSalvageHead(const State& state);
// Energy multiplier for a tool tier (1 at worn).
double TierEnergyFactor(ToolTier tier);

namespace Overgrowth
{
constexpr double Reach = 300.0;
// Weed creep (design 7): a cleared grass or weed placement within this distance of any uncleared
// overgrowth has this daily chance of growing back.
constexpr double CreepNeighbourDistance = 600.0;
constexpr double CreepChance = 0.03;
}
}
