#pragma once

#include "HomesteadShops.h"
#include "HomesteadSimulation.h"

#include <string>

// Iron tool upgrades at the General Store: a placeholder until mining gives iron another source.
// A tool's tier belongs to its type (State::toolTiers); iron already cuts swings and energy through
// the overgrowth tables. She must have crafted the tool before it can be upgraded. Only the worn-to-iron step is sold; steel and master-forged wait for later.
namespace Homestead
{
namespace ToolUpgrade
{
// Jenny (2026-10-04): each iron tool starts at 2,000 coins.
constexpr Coins IronPrice = 2000;

// Whether a shop offers the iron upgrade for `tool`: the General Store, while the tool is still worn.
bool Offered(const State& state, ShopKind kind, ToolKind tool);
// Whether she has crafted `tool` (in her pack or a chest); the upgrade needs it in hand to be fitted.
bool Owned(const State& state, ToolKind tool);
// "Iron axe", or "Iron-bound pail".
std::string Name(ToolKind tool);
// The one-line shelf description.
std::string Description(ToolKind tool);
// "Craft an axe first."
std::string CraftFirst(ToolKind tool);
}
}
