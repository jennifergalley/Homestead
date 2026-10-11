#include "HomesteadEstate.h"
#include "HomesteadItems.h"
#include "HomesteadMine.h"
#include "HomesteadOvergrowth.h"
#include "HomesteadShops.h"
#include "HomesteadSimulation.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>

// open-estate-mine-shallow-level: the mine's node table, mining rules, daily refills, the one-time pile,
// the underground stuck rule, the exhaustion wake and the sale of what she digs.
namespace
{
int MineChecks = 0;
void Check(bool condition, const char* expression, int line)
{
    ++MineChecks;
    if (condition) return;
    std::cerr << "FAILED: " << expression << " (line " << line << ")\n";
    std::exit(1);
}
#define CHECK(expression) Check(static_cast<bool>(expression), #expression, __LINE__)

using namespace Homestead;

struct Mined
{
    Simulation sim;
    Point home{};
};

Mined NewMine()
{
    Mined mined;
    CHECK(mined.sim.NewEstateGame(ProvisionalEstateLayout(), ProvisionalEstatePlacements()).ok);
    CHECK(mined.sim.GrantItems(Item::Pickaxe, 1).ok);
    CHECK(mined.sim.SetEnergy(100.0).ok);
    mined.home = ProvisionalEstateLayout().PointOr(Anchor::TownSquare, {});
    return mined;
}

int NodeOfKind(Mine::NodeKind kind, int skip = 0)
{
    for (const auto& node : Mine::GetLayout().nodes)
        if (node.kind == kind && skip-- == 0) return node.id;
    return 0;
}

Point At(int id)
{
    const Mine::Node* node = Mine::FindNode(id);
    return {node->x, node->y};
}

double ZOf(int id) { return Mine::FindNode(id)->z; }

const ResourceNode* Resource(const Simulation& sim, int id)
{
    for (const auto& node : sim.GetState().resources) if (node.id == id) return &node;
    return nullptr;
}

void Light(Simulation& sim)
{
    sim.SetLampOil(1000.0);
    sim.SetLampInHand(true);
}

// Lamp lit in hand, the pickaxe selected: swing at one node.
Result Swing(Simulation& sim, int id)
{
    return sim.MineNode(id, Item::Pickaxe, At(id), ZOf(id));
}

void LayoutHasTheClaimedNodes()
{
    const Mine::Layout& layout = Mine::GetLayout();
    int counts[4] = {};
    std::set<int> ids;
    for (const auto& node : layout.nodes)
    {
        ++counts[static_cast<int>(node.kind)];
        CHECK(ids.insert(node.id).second && Mine::IsMinePlacementId(node.id) && node.id != Mine::PileId);
        CHECK(Mine::FindNode(node.id) == &node);
    }
    // Design section 7: 20 rock faces, 6 coal seams, 4 copper veins and 3 gem pockets.
    CHECK(counts[0] == 20 && counts[1] == 6 && counts[2] == 4 && counts[3] == 3);
    CHECK(!layout.floors.empty());
    CHECK(Mine::PileId == 588000 && Mine::TrapdoorId == 588001 && Mine::LadderFootId == 588002);
    CHECK(!Mine::IsMinePlacementId(587999) && !Mine::IsMinePlacementId(588200));
}

void EstatePlacementsCarryTheMine()
{
    const auto& table = ProvisionalEstatePlacements();
    int nodes = 0, piles = 0;
    std::set<int> ids;
    for (const auto& placement : table.placements)
    {
        CHECK(ids.insert(placement.id).second);
        if (placement.kind == ResourceKind::MinePile) { ++piles; CHECK(placement.id == Mine::PileId); }
        if (Mine::IsMineNodeKind(placement.kind))
        {
            ++nodes;
            const Mine::Node* spot = Mine::FindNode(placement.id);
            CHECK(spot && Mine::ResourceFor(spot->kind) == placement.kind && placement.z == spot->z);
        }
    }
    CHECK(piles == 1 && nodes == 33);
    Simulation sim;
    CHECK(sim.NewEstateGame(ProvisionalEstateLayout(), table).ok);
    CHECK(Resource(sim, Mine::PileId) && !Resource(sim, Mine::PileId)->cleared);
    for (const auto& node : Mine::GetLayout().nodes) CHECK(Resource(sim, node.id) && !Resource(sim, node.id)->cleared);
}

void CatalogueAndShop()
{
    const Item items[] = {Item::Coal, Item::CopperOre, Item::Quartz, Item::Amethyst};
    const int prices[] = {10, 25, 35, 150};
    const char* names[] = {"Coal", "Copper ore", "Quartz", "Amethyst"};
    for (int i = 0; i < 4; ++i)
    {
        CHECK(BasePrice(items[i]) == prices[i] && SellPrice(items[i]) == prices[i]);
        CHECK(ShopBuys(ShopKind::GeneralStore, items[i]));
        CHECK(!IsEdible(items[i]));
        CHECK(ItemFromKey(ItemKey(items[i])) == items[i]);
        CHECK(std::string(ItemName(items[i])) == names[i]);
    }
}

void MiningRefusals()
{
    Mined mined = NewMine();
    Simulation& sim = mined.sim;
    const int rock = NodeOfKind(Mine::NodeKind::Rock);
    sim.SetLampInHand(false);
    auto dark = sim.CheckMine(rock, Item::Pickaxe, At(rock), ZOf(rock));
    CHECK(!dark.ok && dark.message == "Too dark to work.");
    Light(sim);
    CHECK(sim.CheckMine(rock, Item::Pickaxe, At(rock), ZOf(rock)).ok);
    CHECK(sim.CheckMine(rock, Item::Hatchet, At(rock), ZOf(rock)).message == "Needs a pickaxe.");
    // Too far across, and the surface directly above is too far up.
    Point away = At(rock);
    away.x += Overgrowth::Reach + 50.0;
    CHECK(sim.CheckMine(rock, Item::Pickaxe, away, ZOf(rock)).message == "Move closer to mine this.");
    CHECK(sim.CheckMine(rock, Item::Pickaxe, At(rock), ZOf(rock) + Mine::ReachZCm + 50.0).message == "Move closer to mine this.");
    CHECK(sim.CheckMine(0, Item::Pickaxe, At(rock), ZOf(rock)).message == "There's nothing to mine here.");
    CHECK(sim.CheckMine(Mine::PileId, Item::Pickaxe, At(rock), ZOf(rock)).message == "There's nothing to mine here.");
    // Too tired: below the floor of the exertion rule.
    CHECK(sim.SetEnergy(5.0).ok);
    CHECK(!sim.CheckMine(rock, Item::Pickaxe, At(rock), ZOf(rock)).ok);
    // A refused check changes nothing.
    const int stone = sim.Count(Item::Stone);
    CHECK(!Swing(sim, rock).ok && sim.Count(Item::Stone) == stone);
}

void SwingsAndEnergyFollowTheTable()
{
    Mined mined = NewMine();
    Simulation& sim = mined.sim;
    const Mine::NodeKind kinds[] = {Mine::NodeKind::Rock, Mine::NodeKind::Coal, Mine::NodeKind::Copper, Mine::NodeKind::Gem};
    const int worn[] = {3, 2, 3, 4};
    const double energy[] = {2.5, 2.0, 3.0, 3.0};
    for (int i = 0; i < 4; ++i)
    {
        const int id = NodeOfKind(kinds[i]);
        CHECK(sim.MineSwings(id) == worn[i]);
        CHECK(std::abs(sim.MineCost(id) - energy[i]) < 1e-9);
        CHECK(Mine::InfoFor(kinds[i]).swings[0] == worn[i]);
    }
    // Iron and steel pickaxes cut the swings and the energy (Balance: 3/2/2, 2/1/1, 3/2/2, 4/3/2).
    CHECK(Mine::InfoFor(Mine::NodeKind::Rock).swings[1] == 2 && Mine::InfoFor(Mine::NodeKind::Gem).swings[2] == 2);
    CHECK(Mine::InfoFor(Mine::NodeKind::Coal).swings[1] == 1 && Mine::InfoFor(Mine::NodeKind::Copper).swings[2] == 2);
}

void YieldsAndRefills()
{
    Mined mined = NewMine();
    Simulation& sim = mined.sim;
    Light(sim);
    sim.AdvanceGameHours(2.0, mined.home);
    const int rock = NodeOfKind(Mine::NodeKind::Rock);
    const double energyBefore = sim.GetState().energy;
    const int stoneBefore = sim.Count(Item::Stone);
    CHECK(Swing(sim, rock).ok);
    const int stone = sim.Count(Item::Stone) - stoneBefore;
    CHECK(stone >= 5 && stone <= 7);
    CHECK(std::abs(energyBefore - sim.GetState().energy - 2.5) < 1e-9);
    // Bare until the next 06:00, then back.
    const ResourceNode* node = Resource(sim, rock);
    CHECK(node && !node->cleared && node->readyAtHour > sim.GetState().hour);
    CHECK(Swing(sim, rock).message == "Nothing left here until it comes back.");
    const double morning = node->readyAtHour;
    sim.AdvanceGameHours(morning - sim.GetState().hour - 0.05, mined.home);
    CHECK(!sim.CheckMine(rock, Item::Pickaxe, At(rock), ZOf(rock)).ok);
    sim.AdvanceGameHours(0.1, mined.home);
    CHECK(sim.SetEnergy(100.0).ok);
    Light(sim);
    CHECK(sim.CheckMine(rock, Item::Pickaxe, At(rock), ZOf(rock)).ok);

    // Coal seams, copper veins and gem pockets come back after the days Balance set (every 2, 3 and 5 days).
    const struct { Mine::NodeKind kind; Item item; int low, high; double days; } rows[] = {
        {Mine::NodeKind::Coal, Item::Coal, 2, 3, 2.0}, {Mine::NodeKind::Copper, Item::CopperOre, 2, 2, 3.0},
        {Mine::NodeKind::Gem, Item::Quartz, 1, 2, 5.0}};
    for (const auto& row : rows)
    {
        const int id = NodeOfKind(row.kind);
        CHECK(sim.SetEnergy(100.0).ok);
        Light(sim);
        const double mineHour = sim.GetState().hour;
        const int quartz = sim.Count(Item::Quartz), amethyst = sim.Count(Item::Amethyst), kept = sim.Count(row.item);
        CHECK(Swing(sim, id).ok);
        if (row.kind == Mine::NodeKind::Gem)
        {
            const int gained = sim.Count(Item::Quartz) - quartz + sim.Count(Item::Amethyst) - amethyst;
            CHECK(gained >= 1 && gained <= 2);
        }
        else
        {
            const int gained = sim.Count(row.item) - kept;
            CHECK(gained >= row.low && gained <= row.high);
        }
        const double ready = Resource(sim, id)->readyAtHour;
        const double fromMorning = ready - std::floor(mineHour / 24.0) * 24.0;
        CHECK(std::abs(fromMorning - (24.0 + 6.0 + (row.days - 1.0) * 24.0)) < 1e-6);
    }
}

void GemPocketsRollQuartzOrAmethyst()
{
    int amethyst = 0, quartz = 0;
    for (int day = 0; day < 40; ++day)
    {
        Mined mined = NewMine();
        Simulation& sim = mined.sim;
        sim.AdvanceGameHours(day * 24.0 + 2.0, mined.home);
        CHECK(sim.SetEnergy(100.0).ok);
        Light(sim);
        const int gem = NodeOfKind(Mine::NodeKind::Gem, day % 3);
        const int q = sim.Count(Item::Quartz), a = sim.Count(Item::Amethyst);
        CHECK(Swing(sim, gem).ok);
        if (sim.Count(Item::Amethyst) > a) { ++amethyst; CHECK(sim.Count(Item::Amethyst) - a == 1 && sim.Count(Item::Quartz) == q); }
        else { ++quartz; CHECK(sim.Count(Item::Quartz) > q); }
    }
    CHECK(amethyst > 0 && quartz > amethyst);
}

void StonePileIsOneTime()
{
    Mined mined = NewMine();
    Simulation& sim = mined.sim;
    const auto& pile = Mine::GetLayout().pile;
    const Point spot{pile.x, pile.y};
    const int before = sim.Count(Item::Stone);
    CHECK(sim.Harvest(Mine::PileId, spot).ok);
    CHECK(sim.Count(Item::Stone) == before + Mine::PileStone);
    const ResourceNode* node = Resource(sim, Mine::PileId);
    CHECK(node && node->cleared && node->readyAtHour == 0.0);
    sim.AdvanceGameHours(24.0 * 40.0, mined.home);
    CHECK(Resource(sim, Mine::PileId)->cleared);
    CHECK(!sim.Harvest(Mine::PileId, spot).ok && sim.Count(Item::Stone) == before + Mine::PileStone);
    // It survives a save.
    const std::string saved = sim.Serialize();
    Simulation loaded;
    loaded.SetPlacements(ProvisionalEstatePlacements());
    CHECK(loaded.Deserialize(saved));
    CHECK(Resource(loaded, Mine::PileId)->cleared && loaded.Count(Item::Stone) == before + Mine::PileStone);
    CHECK(!loaded.Clear(Mine::PileId, spot).ok);
}

void StonePileNeedsRoom()
{
    Mined mined = NewMine();
    Simulation& sim = mined.sim;
    const auto& pile = Mine::GetLayout().pile;
    const Point spot{pile.x, pile.y};
    // Fill the pack with stone so 25 more don't fit.
    while (sim.GrantItems(Item::Stone, 1).ok) {}
    const int held = sim.Count(Item::Stone);
    const auto refused = sim.Harvest(Mine::PileId, spot);
    CHECK(!refused.ok && refused.message == "Not enough room for 25 stone.");
    CHECK(sim.Count(Item::Stone) == held && !Resource(sim, Mine::PileId)->cleared);
}

void MineNodesAreNotHarvestedOrCleared()
{
    Mined mined = NewMine();
    Simulation& sim = mined.sim;
    const int rock = NodeOfKind(Mine::NodeKind::Rock);
    CHECK(!sim.CanHarvest(rock));
    const auto hand = sim.Harvest(rock, At(rock));
    CHECK(!hand.ok && hand.message == "Swing your pickaxe at this.");
    CHECK(!sim.Clear(rock, At(rock)).ok);
    CHECK(!Resource(sim, rock)->cleared);
}

void StuckRuleKeepsHerUnderground()
{
    const Mine::Layout& layout = Mine::GetLayout();
    const Mine::FloorSegment& floor = layout.floors.front();
    const Point onFloor{(floor.a.x + floor.b.x) * 0.5, (floor.a.y + floor.b.y) * 0.5};
    CHECK(Mine::IsUnderground(onFloor, floor.z) && !Mine::IsStuck(onFloor, floor.z));
    // The house and the surface above are not underground, so the surface terrain rules apply there.
    CHECK(!Mine::IsUnderground({layout.house.x, layout.house.y}, layout.house.z));
    CHECK(!Mine::IsUnderground({layout.pile.x, layout.pile.y}, layout.pile.z));
    CHECK(!Mine::IsUnderground({0.0, 0.0}, 0.0));
    // Fallen out of the level, or pushed off every gallery by more than the slack: stuck.
    CHECK(Mine::IsStuck(onFloor, floor.z - Mine::StuckBelowFloorCm - 10.0));
    Point wall = onFloor;
    wall.y += floor.halfWidth + Mine::StuckSlackCm + 40.0;
    if (Mine::OutsideFloorBy(wall) > Mine::StuckSlackCm) CHECK(Mine::IsStuck(wall, floor.z));
    // Standing inside the slack beside a wall is fine.
    Point beside = onFloor;
    beside.y += floor.halfWidth + 20.0;
    CHECK(!Mine::IsStuck(beside, floor.z));
    // She goes back to the ladder foot below, never to the surface terrain.
    const Mine::Spot target = Mine::StuckTarget();
    CHECK(target.z == layout.ladderFoot.z && Mine::IsUnderground({target.x, target.y}, target.z) && !Mine::IsStuck({target.x, target.y}, target.z));
    CHECK(target.z < layout.house.z - 1000.0);
}

void ExhaustionUndergroundWakesAtTheMineHouse()
{
    Mined mined = NewMine();
    Simulation& sim = mined.sim;
    const Mine::Spot foot = Mine::StuckTarget();
    const Point below{foot.x, foot.y};
    // Not underground: the rule refuses.
    CHECK(!sim.WakeAtMine(mined.home, 0.0).ok);
    sim.AdvanceGameHours(1.0, mined.home);
    CHECK(sim.GrantItems(Item::Coal, 3).ok);
    CHECK(sim.SetEnergy(4.0).ok);
    const int coal = sim.Count(Item::Coal), flasks = sim.Count(Item::OilFlask), pickaxes = sim.Count(Item::Pickaxe);
    const double hour = sim.GetState().hour;
    const auto woke = sim.WakeAtMine(below, foot.z);
    CHECK(woke.ok && woke.message == "You wake in the mine house, rested.");
    CHECK(sim.GetState().energy == 100.0);
    // Time passes, but nothing she carries is lost.
    CHECK(sim.GetState().hour > hour);
    CHECK(sim.Count(Item::Coal) == coal && sim.Count(Item::OilFlask) == flasks && sim.Count(Item::Pickaxe) == pickaxes);
    CHECK(sim.DozeCount() >= 1);
}

void SaveRoundTripKeepsMinedNodes()
{
    Mined mined = NewMine();
    Simulation& sim = mined.sim;
    Light(sim);
    const int coal = NodeOfKind(Mine::NodeKind::Coal);
    CHECK(Swing(sim, coal).ok);
    const std::string saved = sim.Serialize();
    Simulation loaded;
    loaded.SetPlacements(ProvisionalEstatePlacements());
    CHECK(loaded.Deserialize(saved));
    CHECK(Resource(loaded, coal)->readyAtHour == Resource(sim, coal)->readyAtHour);
    CHECK(loaded.Count(Item::Coal) == sim.Count(Item::Coal));
    CHECK(loaded.Serialize() == saved);
}
}

int main()
{
    LayoutHasTheClaimedNodes();
    EstatePlacementsCarryTheMine();
    CatalogueAndShop();
    MiningRefusals();
    SwingsAndEnergyFollowTheTable();
    YieldsAndRefills();
    GemPocketsRollQuartzOrAmethyst();
    StonePileIsOneTime();
    StonePileNeedsRoom();
    MineNodesAreNotHarvestedOrCleared();
    StuckRuleKeepsHerUnderground();
    ExhaustionUndergroundWakesAtTheMineHouse();
    SaveRoundTripKeepsMinedNodes();
    std::cout << "Mine tests passed (" << MineChecks << " checks)\n";
    return 0;
}
