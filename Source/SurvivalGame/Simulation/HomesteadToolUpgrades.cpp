#include "HomesteadToolUpgrades.h"

#include "HomesteadOvergrowth.h"

namespace Homestead
{
namespace ToolUpgrade
{
bool Offered(const State& state, ShopKind kind, ToolKind tool)
{
    const int index = static_cast<int>(tool);
    return kind == ShopKind::GeneralStore && index >= 0 && index < ToolKindCount
        && state.toolTiers[static_cast<size_t>(index)] == ToolTier::Worn;
}

bool Owned(const State& state, ToolKind tool)
{
    const Item item = ToolItem(tool);
    if (item == Item::Count) return false;
    const size_t index = static_cast<size_t>(item);
    if (state.inventory[index] > 0) return true;
    for (const auto& piece : state.structures)
        if (piece.kind == Piece::Chest && piece.storage[index] > 0) return true;
    return false;
}

std::string Name(ToolKind tool)
{
    return tool == ToolKind::Pail ? std::string("Iron-bound pail") : std::string("Iron ") + ToolName(tool);
}

std::string Description(ToolKind tool)
{
    return std::string("Mr. Trethewey fits a new iron head to your ") + ToolName(tool) + ".";
}

std::string CraftFirst(ToolKind tool)
{
    const std::string name = ToolName(tool);
    const bool vowel = !name.empty() && std::string("aeiou").find(name[0]) != std::string::npos;
    return std::string("Craft ") + (vowel ? "an " : "a ") + name + " first.";
}
}

Result Simulation::BuyToolUpgrade(int shopId, ToolKind tool, Point player)
{
    const auto access = CheckShopAccess(shopId, player);
    if (!access) return access;
    const Shop* shop = FindShop(shopId);
    const auto bad = [this](const std::string& text) { return Result{false, text, ResultCode::Invalid, revision_}; };
    const int index = static_cast<int>(tool);
    if (index < 0 || index >= ToolKindCount) return bad("Choose a tool to upgrade.");
    if (shop->kind != ShopKind::GeneralStore)
        return bad(std::string(ShopDisplayName(shop->kind)) + " doesn't sell tool upgrades.");
    if (state_.toolTiers[static_cast<size_t>(index)] != ToolTier::Worn)
        return bad(std::string("Your ") + ToolName(tool) + " is already " + ToolTierName(state_.toolTiers[static_cast<size_t>(index)]) + ".");
    if (!ToolUpgrade::Owned(state_, tool)) return bad(ToolUpgrade::CraftFirst(tool));
    if (ToolUpgrade::IronPrice > state_.money)
        return bad("That costs " + FormatMoney(ToolUpgrade::IronPrice) + "; you have " + FormatMoney(state_.money) + ".");
    State candidate = state_;
    candidate.toolTiers[static_cast<size_t>(index)] = ToolTier::Iron;
    candidate.money -= ToolUpgrade::IronPrice;
    const std::string message = std::string("Your ") + ToolName(tool) +     " is now iron. Paid "
        + FormatMoney(ToolUpgrade::IronPrice) + ".";
    return CommitInventory(std::move(candidate), message.c_str());
}
}
