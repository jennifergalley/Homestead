#include "HomesteadEstate.h"
#include "HomesteadCrops.h"
#include "HomesteadGardenTarget.h"
#include "HomesteadPackRow.h"
#include "HomesteadSimulation.h"
#include "../Source/SurvivalGame/HomesteadActionHints.h"

#include <algorithm>
#include <cstdlib>
#include <chrono>
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
        if (entry.wearableId == 0 && entry.item == item) groups.push_back(entry.groupId);
    return groups;
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

void IdentitiesAndStacking()
{
    using namespace Homestead;
    Simulation sim;
    sim.SetPackRowAutoFill(false);
    std::set<std::string> names, icons;
    const Item crops[] = {Item::Seeds, Item::TurnipSeed, Item::CarrotSeed, Item::SeedPotato,
        Item::CabbageSeed, Item::BroadBeanSeed, Item::StrawberryRunner};
    for (Item item : crops)
    {
        CHECK(IsSeedPacket(item) && CanStackItem(item));
        CHECK(std::string(ItemName(item)) != "Seeds");
        CHECK(names.insert(ItemName(item)).second);
        CHECK(icons.insert(GetItemInfo(item).icon).second);
        OK(sim.GrantItems(item, 3));
        const auto groups = Groups(sim.GetState().inventoryLayout, item);
        CHECK(groups.size() == 1 && sim.Count(item) == 3);
    }
    // Packets of one crop stack in one slot; each crop keeps its own slot.
    CHECK(Groups(sim.GetState().inventoryLayout, Item::CarrotSeed).size() == 1);
    CHECK(Groups(sim.GetState().inventoryLayout, Item::TurnipSeed).size() == 1);
    OK(sim.GrantItems(Item::CarrotSeed, 2));
    const auto carrots = Groups(sim.GetState().inventoryLayout, Item::CarrotSeed);
    CHECK(carrots.size() == 1 && sim.Count(Item::CarrotSeed) == 5);
    const auto turnips = Groups(sim.GetState().inventoryLayout, Item::TurnipSeed);
    const auto before = sim.Serialize();
    CHECK(!sim.MergeGroups(0, carrots[0], turnips[0], {0, 0}, sim.GetRevision()));
    CHECK(sim.Serialize() == before);
    // Two carrot stacks split apart merge back into one.
    OK(sim.SplitGroup(0, carrots[0], 2, {0, 0}, sim.GetRevision()));
    const auto split = Groups(sim.GetState().inventoryLayout, Item::CarrotSeed);
    CHECK(split.size() == 2);
    OK(sim.MergeGroups(0, split[0], split[1], {0, 0}, sim.GetRevision()));
    CHECK(Groups(sim.GetState().inventoryLayout, Item::CarrotSeed).size() == 1 && sim.Count(Item::CarrotSeed) == 5);
    Simulation loaded = sim;
    OK(loaded.Deserialize(sim.Serialize()));
    CHECK(loaded.Serialize() == sim.Serialize());
    CHECK(Groups(loaded.GetState().inventoryLayout, Item::CarrotSeed).size() == 1 && loaded.Count(Item::CarrotSeed) == 5);
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
    OK(sim.GrantItems(Item::CarrotSeed, 4));
    const auto group = Groups(sim.GetState().inventoryLayout, Item::CarrotSeed)[0];
    OK(sim.TransferGroup(chest, group, 4, true, at, sim.GetRevision()));
    CHECK(sim.Count(Item::CarrotSeed) == 0);
    const auto stored = std::find_if(sim.GetState().structures.begin(), sim.GetState().structures.end(),
        [chest](const Structure& piece) { return piece.id == chest; });
    CHECK(stored != sim.GetState().structures.end());
    const auto held = Groups(stored->layout, Item::CarrotSeed);
    CHECK(held.size() == 1);
    OK(sim.TransferGroup(chest, held[0], 4, false, at, sim.GetRevision()));
    CHECK(sim.Count(Item::CarrotSeed) == 4);
    const auto taken = Groups(sim.GetState().inventoryLayout, Item::CarrotSeed);
    CHECK(taken.size() == 1);
    Point ground{};
    bool found = false;
    for (double y = -1000; y <= 1000 && !found; y += 200)
        for (double x = -1000; x <= 1000 && !found; x += 200)
        {
            Simulation trial = sim;
            if (trial.DropGroup(taken[0], 3, {x, y}, {x, y}, trial.GetRevision()))
            { ground = {x, y}; sim = std::move(trial); found = true; }
        }
    CHECK(found);
    CHECK(sim.Count(Item::CarrotSeed) == 1);
    int drops = 0;
    for (const auto& drop : sim.GetState().worldDrops)
        if (drop.item == Item::CarrotSeed) { CHECK(drop.quantity == 3); ++drops; }
    CHECK(drops == 1);
    for (const auto& drop : sim.GetState().worldDrops)
        if (drop.item == Item::CarrotSeed) { OK(sim.PickUpDrop(drop.id, ground)); break; }
    CHECK(sim.Count(Item::CarrotSeed) == 4);
    CHECK(Groups(sim.GetState().inventoryLayout, Item::CarrotSeed).size() <= 2);
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
    CHECK(Groups(sim.GetState().inventoryLayout, Item::CarrotSeed).size() <= 2);
    CHECK(Groups(sim.GetState().inventoryLayout, Item::TurnipSeed).size() <= 2);
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
    OK(sim.Plant(plotId, garden, CropKind::Carrots, packets[0]));
    const auto remaining = Groups(sim.GetState().inventoryLayout, Item::CarrotSeed);
    CHECK(packets.size() == 1 && remaining.size() == 1 && remaining[0] == packets[0] && sim.Count(Item::CarrotSeed) == 1);
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
    SeedPacketTests::IdentitiesAndStacking();
    SeedPacketTests::ChestAndGround();
    SeedPacketTests::BuyPackets();
    SeedPacketTests::HarvestWeedsFirst();
    SeedPacketTests::SowAndHarvestCue();
    std::cout << "Seed packets: " << SeedPacketTests::Checks << " checks passed.\n";
}
