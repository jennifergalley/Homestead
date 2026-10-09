#include "HomesteadGarmentShop.h"

#include <algorithm>
#include <limits>
#include <string>

namespace Homestead
{
namespace GarmentShop
{
Coins Price(WearableDefinition definition)
{
    for (const Offer& offer : Offers) if (offer.definition == definition) return offer.price;
    return 0;
}

bool Owned(const State& state, WearableDefinition definition)
{
    return std::any_of(state.wearables.begin(), state.wearables.end(),
        [definition](const WearableInstance& item) { return item.definition == definition; });
}

bool Offered(ShopKind kind, WearableDefinition definition)
{
    return kind == ShopKind::GeneralStore && Price(definition) > 0;
}
}

Result Simulation::BuyGarment(int shopId, WearableDefinition definition, Point player)
{
    const auto access = CheckShopAccess(shopId, player);
    if (!access) return access;
    const Shop* shop = FindShop(shopId);
    const auto* info = GetWearableDefinition(definition);
    if (!info || !GarmentShop::Offered(shop->kind, definition))
        return {false, std::string(ShopDisplayName(shop->kind)) + " doesn't sell that.", ResultCode::Invalid, revision_};
    std::string name = info->name;
    if (!name.empty() && name[0] >= 'A' && name[0] <= 'Z') name[0] = static_cast<char>(name[0] - 'A' + 'a');
    if (GarmentShop::Owned(state_, definition))
        return {false, "You already own the " + name + ".", ResultCode::Invalid, revision_};
    const Coins price = GarmentShop::Price(definition);
    if (price > state_.money)
        return {false, "That costs " + FormatMoney(price) + "; you have " + FormatMoney(state_.money) + ".",
            ResultCode::Invalid, revision_};
    if (UsedCapacity() + 1 > PackCapacity())
        return {false, "Not enough pack space.", ResultCode::Capacity, revision_};
    if (state_.nextWearableId <= 0 || state_.nextWearableId >= std::numeric_limits<int>::max() - 1)
        return {false, "The homestead has reached its garment identity limit.", ResultCode::Unavailable, revision_};
    State candidate = state_;
    candidate.wearables.push_back({candidate.nextWearableId++, definition, 0, WearableOwner::Carried, 0});
    candidate.money -= price;
    const std::string message = "Bought the " + name + " for " + FormatMoney(price) + ".";
    return CommitInventory(std::move(candidate), message.c_str());
}
}
