#include "HomesteadEstate.h"
#include "HomesteadCrops.h"
#include "HomesteadGardenTarget.h"
#include "HomesteadPackRow.h"
#include "HomesteadSimulation.h"
#include "../Source/SurvivalGame/HomesteadActionHints.h"

#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <set>
#include <sstream>

namespace SeedPacketTests
{
using Homestead::Item;
using Homestead::Simulation;
int Checks = 0;
void Check(bool value, int line)
{
    ++Checks;
    if (!value) { std::cerr << "Seed packet check failed at line " << line << '\n'; std::exit(1); }
}
#define CHECK(value) SeedPacketTests::Check(static_cast<bool>(value), __LINE__)
void Okay(const Homestead::Result& result, int line)
{
    if (!result.ok) std::cerr << result.message << '\n';
    Check(result.ok, line);
}
#define OK(value) SeedPacketTests::Okay(value, __LINE__)

std::vector<int> Groups(const Homestead::InventoryLayout& layout, Item item)
{
    std::vector<int> groups;
    for (const auto& entry : layout)
        if (entry.wearableId == 0 && entry.item == item)
        {
            CHECK(entry.quantity == 1);
            groups.push_back(entry.groupId);
        }
    return groups;
}

std::string LayoutText(const Homestead::InventoryLayout& layout)
{
    std::ostringstream out;
    out << layout.size() << '\n';
    for (const auto& entry : layout)
        out << entry.groupId << ' ' << static_cast<int>(entry.item) << ' ' << entry.quantity << ' ' << entry.wearableId << '\n';
    return out.str();
}

std::string Reseal(const std::string& saved, const std::string& payload)
{
    std::uint64_t hash = UINT64_C(14695981039346656037);
    for (unsigned char c : payload) { hash ^= c; hash *= UINT64_C(1099511628211); }
    std::istringstream header(saved.substr(0, saved.find('\n')));
    std::string magic, version;
    header >> magic >> version;
    return magic + " " + version + " " + std::to_string(payload.size()) + " " + std::to_string(hash) + "\n" + payload;
}

std::string LegacyStacks(const Simulation& sim, bool explicitSlots)
{
    const auto saved = sim.Serialize();
    auto payload = saved.substr(saved.find('\n') + 1);
    Homestead::InventoryLayout old;
    for (const auto& entry : sim.GetState().inventoryLayout)
    {
        const auto first = std::find_if(old.begin(), old.end(), [&entry](const Homestead::LayoutEntry& group)
            { return group.wearableId == 0 && group.item == entry.item; });
        if (Homestead::IsSeedPacket(entry.item) && first != old.end()) first->quantity += entry.quantity;
        else old.push_back(entry);
    }
    const auto current = LayoutText(sim.GetState().inventoryLayout);
    const auto at = payload.rfind(current);
    CHECK(at != std::string::npos);
    payload.replace(at, current.size(), LayoutText(old));
    if (explicitSlots)
    {
        std::vector<Homestead::PackRowCell> slots;
        for (const auto& entry : old)
            if (Homestead::PackRowRules::CellOf(sim.GetState().packRow, entry) < 0)
                slots.push_back(Homestead::PackRowRules::CellFor(entry));
        payload += "packslots " + std::to_string(slots.size());
        for (const auto& slot : slots)
            payload += " " + std::to_string(slot.groupId) + " " + std::to_string(slot.wearableId);
        payload += "\n";
    }
    return Reseal(saved, payload);
}

void IdentitiesAndMoves()
{
    using namespace Homestead;
    Simulation sim;
    sim.SetPackRowAutoFill(false);
    std::set<std::string> names, icons;
    for (Item item : {Item::Seeds, Item::TurnipSeed, Item::CarrotSeed, Item::SeedPotato,
        Item::CabbageSeed, Item::BroadBeanSeed, Item::StrawberryRunner})
    {
        CHECK(IsSeedPacket(item) && !CanStackItem(item));
        CHECK(std::string(ItemName(item)) != "Seeds");
        CHECK(names.insert(ItemName(item)).second);
        CHECK(icons.insert(GetItemInfo(item).icon).second);
        OK(sim.GrantItems(item, 3));
        CHECK(Groups(sim.GetState().inventoryLayout, item).size() == 3);
    }
    CHECK(!IsSeedPacket(Item::Berries) && CanStackItem(Item::Stone));
    const auto carrots = Groups(sim.GetState().inventoryLayout, Item::CarrotSeed);
    const auto saved = sim.Serialize();
    CHECK(!sim.MergeGroups(0, carrots[0], carrots[1], {0, 0}, sim.GetRevision()));
    CHECK(sim.Serialize() == saved);
    OK(sim.MoveToPackRow(carrots[0], 0, 0, sim.GetRevision()));
    OK(sim.MoveToPackRow(carrots[1], 0, 0, sim.GetRevision()));
    CHECK(sim.GetState().packRow[0].groupId == carrots[1] && Groups(sim.GetState().inventoryLayout, Item::CarrotSeed).size() == 3);
    OK(sim.MoveFromPackRow(0, carrots[2], 0, sim.GetRevision()));
    CHECK(sim.GetState().packRow[0].groupId == carrots[2]);
    OK(sim.MoveToPackSlot(carrots[0], 0, 4, sim.GetRevision()));
    OK(sim.MoveToPackSlot(carrots[1], 0, 4, sim.GetRevision()));
    CHECK(Groups(sim.GetState().inventoryLayout, Item::CarrotSeed).size() == 3);
    OK(sim.SortPack(sim.GetRevision()));
    const auto sortedPackets = Groups(sim.GetState().inventoryLayout, Item::CarrotSeed);
    for (int id : carrots) CHECK(std::find(sortedPackets.begin(), sortedPackets.end(), id) != sortedPackets.end());
    Simulation loaded = sim;
    OK(loaded.Deserialize(sim.Serialize()));
    CHECK(loaded.Serialize() == sim.Serialize());
}

void LegacyFullPack()
{
    using namespace Homestead;
    Simulation sim;
    sim.SetPackRowAutoFill(false);
    for (Item item : {Item::Seeds, Item::CarrotSeed, Item::TurnipSeed}) OK(sim.GrantItems(item, 25));
    const int rootsId = Groups(sim.GetState().inventoryLayout, Item::Seeds)[0];
    const int carrotsId = Groups(sim.GetState().inventoryLayout, Item::CarrotSeed)[0];
    OK(sim.MoveToPackRow(rootsId, 0, 0, sim.GetRevision()));
    OK(sim.MoveToPackRow(carrotsId, 0, 9, sim.GetRevision()));
    OK(sim.GrantItems(Item::Stone, sim.PackCapacity() - sim.UsedCapacity()));
    CHECK(sim.UsedCapacity() == sim.PackCapacity());
    for (bool explicitSlots : {false, true})
    {
        const auto legacy = LegacyStacks(sim, explicitSlots);
        Simulation loaded = sim;
        OK(loaded.Deserialize(legacy));
        CHECK(loaded.GetState().inventory == sim.GetState().inventory);
        CHECK(loaded.UsedCapacity() == sim.PackCapacity());
        CHECK(loaded.GetState().packRow[0].groupId == rootsId && loaded.GetState().packRow[9].groupId == carrotsId);
        for (Item item : {Item::Seeds, Item::CarrotSeed, Item::TurnipSeed})
            CHECK(Groups(loaded.GetState().inventoryLayout, item).size() == 25);
        CHECK(PackRowRules::Valid(loaded.GetState().packRow, loaded.GetState().inventoryLayout));
        CHECK(PackRowRules::ValidSlots(loaded.GetState()));
        std::set<int> visible;
        for (const auto& cell : loaded.GetState().packRow)
            if (cell.groupId != 0) CHECK(visible.insert(cell.groupId).second);
        for (int index : PackRowRules::Grid(loaded.GetState()))
            if (index >= 0 && loaded.GetState().inventoryLayout[index].groupId != 0)
                CHECK(visible.insert(loaded.GetState().inventoryLayout[index].groupId).second);
        for (const auto& entry : loaded.GetState().inventoryLayout)
            if (entry.groupId != 0) CHECK(visible.count(entry.groupId) == 1);
        const auto normalized = loaded.Serialize();
        Simulation roundtrip = loaded;
        OK(roundtrip.Deserialize(normalized));
        CHECK(roundtrip.Serialize() == normalized);
        auto payload = legacy.substr(legacy.find('\n') + 1);
        const auto roots = "\n" + std::to_string(rootsId) + " " + std::to_string(static_cast<int>(Item::Seeds)) + " 25 0\n";
        const auto at = payload.find(roots);
        CHECK(at != std::string::npos);
        payload.replace(at, roots.size(), "\n" + std::to_string(rootsId) + " "
            + std::to_string(static_cast<int>(Item::Seeds)) + " 24 0\n");
        CHECK(!roundtrip.Deserialize(Reseal(legacy, payload)));
        CHECK(roundtrip.Serialize() == normalized);
    }
}

void ChestAndGround()
{
    using namespace Homestead;
    Simulation sim;
    OK(sim.NewEstateGame(ProvisionalEstateLayout(), ProvisionalEstatePlacements()));
    sim.SetPackRowAutoFill(false);
    int chest = 0;
    Point at{};
    for (const auto& piece : sim.GetState().structures)
        if (piece.kind == Piece::Chest) { chest = piece.id; at = StructureCenter(sim.GetState(), piece); break; }
    CHECK(chest > 0);
    OK(sim.GrantItems(Item::CarrotSeed, 2));
    const auto groups = Groups(sim.GetState().inventoryLayout, Item::CarrotSeed);
    for (int group : groups) OK(sim.TransferGroup(chest, group, 1, true, at, sim.GetRevision()));
    const auto stored = std::find_if(sim.GetState().structures.begin(), sim.GetState().structures.end(),
        [chest](const Structure& piece) { return piece.id == chest; });
    CHECK(stored != sim.GetState().structures.end());
    const auto held = Groups(stored->layout, Item::CarrotSeed);
    CHECK(held.size() == 2 && sim.Count(Item::CarrotSeed) == 0);
    {
        Simulation legacyChest = sim;
        const auto modern = sim.Serialize();
        auto payload = modern.substr(modern.find('\n') + 1);
        auto oldLayout = stored->layout;
        for (auto& entry : oldLayout)
            if (entry.groupId == held[0]) entry.quantity = 2;
        oldLayout.erase(std::remove_if(oldLayout.begin(), oldLayout.end(),
            [&](const LayoutEntry& entry) { return entry.groupId == held[1]; }), oldLayout.end());
        const auto atLayout = payload.find(LayoutText(stored->layout));
        CHECK(atLayout != std::string::npos);
        payload.replace(atLayout, LayoutText(stored->layout).size(), LayoutText(oldLayout));
        OK(legacyChest.Deserialize(Reseal(modern, payload)));
        const auto normalized = Groups(*legacyChest.GetLayout(chest), Item::CarrotSeed);
        CHECK(normalized.size() == 2 && normalized.front() == held[0]);
        for (int group : normalized)
            OK(legacyChest.TransferGroup(chest, group, 1, false, at, legacyChest.GetRevision()));
        CHECK(legacyChest.Count(Item::CarrotSeed) == 2);
    }
    for (int group : held) OK(sim.TransferGroupToPackRow(chest, group, 1, 0, at, sim.GetRevision()));
    const auto taken = Groups(sim.GetState().inventoryLayout, Item::CarrotSeed);
    CHECK(taken.size() == 2 && sim.Count(Item::CarrotSeed) == 2);
    Point ground{};
    bool found = false;
    for (double y = -1000; y <= 1000 && !found; y += 200)
        for (double x = -1000; x <= 1000 && !found; x += 200)
        {
            Simulation trial = sim;
            if (trial.DropGroup(taken[0], 1, {x, y}, {x, y}, trial.GetRevision()))
            { ground = {x, y}; sim = std::move(trial); found = true; }
        }
    CHECK(found);
    OK(sim.DropGroup(taken[1], 1, ground, ground, sim.GetRevision()));
    std::vector<int> drops;
    for (const auto& drop : sim.GetState().worldDrops)
        if (drop.item == Item::CarrotSeed) { CHECK(drop.quantity == 1); drops.push_back(drop.id); }
    CHECK(drops.size() == 2);
    Simulation oldDrop = sim;
    const auto modern = sim.Serialize();
    auto payload = modern.substr(modern.find('\n') + 1);
    std::ostringstream currentDrops, legacyDrops;
    currentDrops << sim.GetState().worldDrops.size() << '\n';
    legacyDrops << sim.GetState().worldDrops.size() - 1 << '\n';
    bool kept = false;
    for (const auto& drop : sim.GetState().worldDrops)
    {
        const auto line = [&drop](int quantity)
        {
            std::ostringstream out;
            out << drop.id << ' ' << drop.position.x << ' ' << drop.position.y << ' '
                << static_cast<int>(drop.item) << ' ' << quantity << ' ' << drop.wearableId << '\n';
            return out.str();
        };
        currentDrops << line(drop.quantity);
        if (drop.item != Item::CarrotSeed) legacyDrops << line(drop.quantity);
        else if (!kept) { legacyDrops << line(2); kept = true; }
    }
    const auto atDrops = payload.find(currentDrops.str());
    CHECK(atDrops != std::string::npos);
    payload.replace(atDrops, currentDrops.str().size(), legacyDrops.str());
    OK(oldDrop.Deserialize(Reseal(modern, payload)));
    OK(oldDrop.PickUpDrop(drops[0], ground));
    CHECK(oldDrop.Count(Item::CarrotSeed) == 2 && Groups(oldDrop.GetState().inventoryLayout, Item::CarrotSeed).size() == 2);
    for (int id : drops) OK(sim.PickUpDrop(id, ground));
    CHECK(Groups(sim.GetState().inventoryLayout, Item::CarrotSeed).size() == 2);
}

void BuyPackets()
{
    using namespace Homestead;
    Simulation sim;
    OK(sim.NewEstateGame(ProvisionalEstateLayout(), ProvisionalEstatePlacements()));
    sim.SkipToHourOfDay(10.0);
    const auto* store = sim.FindShop(ShopKind::GeneralStore);
    CHECK(store);
    const int id = store->id;
    const Point counter{store->counterX, store->counterY - 150.0};
    OK(sim.GrantMoney(10000));
    const int carrots = sim.Count(Item::CarrotSeed), turnips = sim.Count(Item::TurnipSeed);
    OK(sim.Buy(id, Item::CarrotSeed, 2, false, counter));
    OK(sim.Buy(id, Item::TurnipSeed, 1, false, counter));
    CHECK(sim.Count(Item::CarrotSeed) == carrots + 2 && sim.Count(Item::TurnipSeed) == turnips + 1);
    CHECK(Groups(sim.GetState().inventoryLayout, Item::CarrotSeed).size() == static_cast<std::size_t>(carrots + 2));
    CHECK(Groups(sim.GetState().inventoryLayout, Item::TurnipSeed).size() == static_cast<std::size_t>(turnips + 1));
}

void HarvestWeedsFirst()
{
    using namespace Homestead;
    Simulation sim;
    sim.SetPackRowAutoFill(false);
    OK(sim.GrantItems(Item::DiggingStick, 1));
    OK(sim.GrantItems(Item::Seeds, 2));
    int id = -1;
    Point at{};
    for (int y = -5; y <= 5 && id < 0; ++y)
        for (int x = -5; x <= 5 && id < 0; ++x)
        {
            const auto point = GardenCellCenter(x, y);
            if (sim.Till(x, y, point))
            {
                const int made = sim.GetState().plots.back().id;
                if (HarvestBonusCount(CropKind::Roots, made, sim.GetState().hour) == 1)
                { at = point; id = made; }
            }
        }
    CHECK(id >= 0);
    OK(sim.Plant(id, at, CropKind::Roots));
    const auto plot = sim.GetState().plots.back();
    const auto plotLine = [](const Plot& entry)
    {
        std::ostringstream out;
        out << std::setprecision(std::numeric_limits<double>::max_digits10);
        out << entry.id << ' ' << entry.cellX << ' ' << entry.cellY << ' ' << entry.planted << ' '
            << entry.growth << ' ' << entry.moisture << ' ' << entry.weeds << ' '
            << static_cast<int>(entry.kind) << '\n';
        return out.str();
    };
    auto ripe = plot;
    ripe.growth = 1.0;
    ripe.weeds = 0.25;
    const auto modern = sim.Serialize();
    auto payload = modern.substr(modern.find('\n') + 1);
    const auto found = payload.find(plotLine(plot));
    CHECK(found != std::string::npos);
    payload.replace(found, plotLine(plot).size(), plotLine(ripe));
    OK(sim.Deserialize(Reseal(modern, payload)));
    const auto before = sim.Serialize();
    const auto refused = sim.HarvestCrop(id, at);
    CHECK(!refused.ok && sim.Serialize() == before);
    CHECK(DescribeHarvest(ripe).empty());
    OK(sim.Weed(id, at));
    CHECK(DescribeHarvest(sim.GetState().plots.back()) == "Harvest Roots");
    const int packets = sim.Count(Item::Seeds);
    OK(sim.HarvestCrop(id, at));
    CHECK(sim.Count(Item::Roots) > 0 && sim.Count(Item::Seeds) == packets + 1);
    CHECK(Groups(sim.GetState().inventoryLayout, Item::Seeds).size() == static_cast<std::size_t>(packets + 1));
}

void SowAndHarvestCue()
{
    using namespace Homestead;
    Simulation sim;
    sim.SetPackRowAutoFill(false);
    OK(sim.GrantItems(Item::DiggingStick, 1));
    OK(sim.GrantItems(Item::CarrotSeed, 2));
    int plotId = -1;
    Point garden{};
    for (int y = -5; y <= 5 && plotId < 0; ++y)
        for (int x = -5; x <= 5 && plotId < 0; ++x)
        {
            const auto point = GardenCellCenter(x, y);
            if (sim.Till(x, y, point)) { garden = point; plotId = sim.GetState().plots.back().id; }
        }
    CHECK(plotId >= 0);
    const auto packets = Groups(sim.GetState().inventoryLayout, Item::CarrotSeed);
    OK(sim.Plant(plotId, garden, CropKind::Carrots, packets[1]));
    const auto remaining = Groups(sim.GetState().inventoryLayout, Item::CarrotSeed);
    CHECK(remaining.size() == 1 && remaining[0] == packets[0]);
    Plot ripe{};
    ripe.planted = true;
    ripe.growth = 1.0;
    for (int kind = 0; kind < static_cast<int>(CropKind::Count); ++kind)
    {
        ripe.kind = static_cast<CropKind>(kind);
        const auto cue = DescribeHarvest(ripe);
        CHECK(cue == std::string("Harvest ") + GetCropInfo(ripe.kind).name);
        CHECK(!HomesteadActionHints::ShouldRetire(cue, 1000, 3));
    }
    CHECK(HomesteadActionHints::ShouldRetire("Gather", 3, 3));
    ripe.weeds = CropCare::VisibleWeeds + 0.01;
    CHECK(DescribeHarvest(ripe).empty());
    ripe.weeds = 0.0;
    ripe.withered = true;
    CHECK(DescribeHarvest(ripe).empty());
}
}

int main()
{
    SeedPacketTests::IdentitiesAndMoves();
    SeedPacketTests::LegacyFullPack();
    SeedPacketTests::ChestAndGround();
    SeedPacketTests::BuyPackets();
    SeedPacketTests::HarvestWeedsFirst();
    SeedPacketTests::SowAndHarvestCue();
    std::cout << "Seed packets: " << SeedPacketTests::Checks << " checks passed.\n";
}
