#include "HomesteadSimulation.h"
#include "HomesteadEstate.h"
#include "HomesteadManor.h"
#include "HomesteadOvergrowth.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>
#include <sstream>
#include <string>
#include <vector>

using namespace Homestead;

namespace
{
int checks = 0;
int cases = 0;
void Check(bool condition, const char* expression, int line)
{
    ++checks;
    if (!condition)
    {
        std::cerr << "FAIL line " << line << ": " << expression << '\n';
        std::exit(1);
    }
}
#define CHECK(expression) Check(static_cast<bool>(expression), #expression, __LINE__)
void Okay(Result result, int line)
{
    ++checks;
    if (!result.ok)
    {
        std::cerr << "FAIL line " << line << ": " << result.message << '\n';
        std::exit(1);
    }
}
#define OK(expression) Okay(expression, __LINE__)
bool Close(double a, double b, double tolerance = 1e-7) { return std::abs(a - b) <= tolerance; }
constexpr Point Home{-750, 150};
constexpr Point WaterSource{1500, 0};

std::string Envelope(const std::string& body, int version = SimulationSaveVersion)
{
    std::uint64_t hash = UINT64_C(14695981039346656037);
    for (unsigned char c : body) { hash ^= c; hash *= UINT64_C(1099511628211); }
    return "HOMESTEAD " + std::to_string(version) + " " + std::to_string(body.size()) + " " +
        std::to_string(hash) + "\n" + body;
}
// Independent fixture writer intentionally permits invalid states so parser validation is exercised.
std::string Encode(const State& s, int version = SimulationSaveVersion)
{
    // Version 7 stocks predate the machete, and versions before 11 predate fur and the estate items.
    const int stockItems = version >= ClothingSaveVersion ? ItemCount
        : version >= 8 ? static_cast<int>(Item::Fur) : static_cast<int>(Item::Machete);
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setprecision(17) << s.hour << ' ' << s.dayMinutes << ' ' << s.hunger << ' ' << s.energy << ' ';
    // Older saves carried warmth and the warm-outfit flag; the estate pivot removed both.
    if (version < SimulationSaveVersion) out << (s.failed ? 0.0 : 90.0) << ' ' << s.failed << ' ' << 0 << ' ';
    else out << s.failed << ' ';
    out << s.nextId << '\n';
    for (int i = 0; i < stockItems; ++i) out << s.inventory[i] << ' ';
    out << '\n' << s.world.seed << ' ' << s.world.generationVersion << ' ' << s.activeChunk.x << ' ' << s.activeChunk.y << '\n';
    out << s.resourceEdits.size() << '\n';
    for (const auto& edit : s.resourceEdits)
        out << edit.key.chunk.x << ' ' << edit.key.chunk.y << ' ' << edit.key.localId << ' '
            << edit.cleared << ' ' << edit.readyAtHour << '\n';
    if (version >= FreeBuildingSaveVersion)
    {
        out << s.buildings.size() << '\n';
        for (const auto& b : s.buildings) out << b.id << ' ' << b.origin.x << ' ' << b.origin.y << ' ' << b.yaw << '\n';
    }
    out << s.structures.size() << '\n';
    for (const auto& p : s.structures)
    {
        out << p.id << ' ' << static_cast<int>(p.kind) << ' ';
        if (version >= FreeBuildingSaveVersion) out << p.buildingId << ' ';
        out << p.cellX << ' ' << p.cellY << ' '
            << p.rotation << ' ' << p.fuelHours << ' ';
        for (int i = 0; i < stockItems; ++i) out << p.storage[i] << ' ';
        out << '\n';
        if (version >= 4)
        {
            out << p.layout.size() << '\n';
            for (const auto& e : p.layout)
                out << e.groupId << ' ' << static_cast<int>(e.item) << ' ' << e.quantity << ' ' << e.wearableId << '\n';
        }
    }
    out << s.plots.size() << '\n';
    for (const auto& p : s.plots)
    {
        out << p.id << ' ' << p.cellX << ' ' << p.cellY << ' ' << p.planted << ' '
            << p.growth << ' ' << p.moisture << ' ' << p.weeds;
        if (version >= 3) out << ' ' << static_cast<int>(p.kind);
        out << '\n';
    }
    if (version >= 7)
    {
        out << s.worldDrops.size() << '\n';
        for (const auto& drop : s.worldDrops)
            out << drop.id << ' ' << drop.position.x << ' ' << drop.position.y << ' '
                << static_cast<int>(drop.item) << ' ' << drop.quantity << ' ' << drop.wearableId << '\n';
    }
    if (version >= 4)
    {
        out << s.nextWearableId << ' ' << s.nextGroupId << '\n' << s.wearables.size() << '\n';
        for (const auto& w : s.wearables)
            out << w.id << ' ' << static_cast<int>(w.definition) << ' ' << w.dye << ' '
                << static_cast<int>(w.owner) << ' ' << w.chestId << '\n';
        for (int slot = 0; slot < (version >= ClothingSaveVersion ? EquipmentSlotCount : 4); ++slot)
            out << s.equipment[slot] << ' ';
        out << '\n' << s.inventoryLayout.size() << '\n';
        for (const auto& e : s.inventoryLayout)
            out << e.groupId << ' ' << static_cast<int>(e.item) << ' ' << e.quantity << ' ' << e.wearableId << '\n';
    }
    if (version >= 8)
    {
        out << s.clearedUnderbrush.size() << '\n';
        for (const auto& plant : s.clearedUnderbrush)
            out << plant.chunk.x << ' ' << plant.chunk.y << ' ' << plant.index << '\n';
    }
    if (version >= SimulationSaveVersion)
    {
        out << "tools " << ToolKindCount;
        for (const ToolTier tier : s.toolTiers) out << ' ' << static_cast<int>(tier);
        out << '\n';
    }
    return Envelope(out.str(), version);
}
void FixtureLayouts(State& state)
{
    // Synthetic fixture edits replace counts; real gameplay must reconcile stable groups instead.
    state.nextGroupId = 1;
    const auto rebuild = [&](InventoryLayout& layout, const Inventory& stock, int chestId) {
        layout.clear();
        for (int i = 0; i < ItemCount; ++i)
            if (stock[i] > 0) layout.push_back({state.nextGroupId++, static_cast<Item>(i), stock[i], 0});
        for (const auto& item : state.wearables)
            if ((chestId == 0 && item.owner == WearableOwner::Carried) ||
                (chestId != 0 && item.owner == WearableOwner::Chest && item.chestId == chestId))
                layout.push_back({0, Item::Knife, 0, item.id});
    };
    rebuild(state.inventoryLayout, state.inventory, 0);
    for (auto& piece : state.structures) rebuild(piece.layout, piece.storage, piece.id);
}
void ClearFixtureSites(State& state)
{
    // Legacy rule fixtures use prepared building sites; seeded-world tests below exercise actual felling.
    for (const auto& node : state.resources)
    {
        if (node.kind != ResourceKind::ForestTree && node.kind != ResourceKind::Sapling) continue;
        if (node.position.x < -1550 || node.position.x > 1250 ||
            node.position.y < -950 || node.position.y > 650) continue;
        auto found = std::find_if(state.resourceEdits.begin(), state.resourceEdits.end(),
            [&](const ResourceEdit& value) { return value.key == node.key; });
        if (found == state.resourceEdits.end()) state.resourceEdits.push_back({node.key, true, 0});
        else *found = {node.key, true, 0};
    }
    std::sort(state.resourceEdits.begin(), state.resourceEdits.end(),
        [](const ResourceEdit& a, const ResourceEdit& b) { return a.key < b.key; });
}
void Edit(Simulation& sim, const std::function<void(State&)>& edit, bool prepareSites = true)
{
    State state = sim.GetState();
    edit(state);
    if (prepareSites) ClearFixtureSites(state);
    FixtureLayouts(state);
    OK(sim.Deserialize(Encode(state)));
}
// A new game starts barefoot; wardrobe fixtures written for the old starter shoes put them back on
// as garment 2 (the Feet slot), exactly as earlier saves carry them.
void Shod(Simulation& sim)
{
    Edit(sim, [](State& state) {
        state.wearables.push_back({2, WearableDefinition::LeatherShoes, 0, WearableOwner::Equipped, 0});
        state.equipment[static_cast<int>(EquipmentSlot::Feet)] = 2;
        state.nextWearableId = 3;
    });
}
void Stock(Simulation& sim, std::initializer_list<std::pair<Item, int>> items)
{
    Edit(sim, [&](State& state) {
        state.inventory.fill(0);
        for (const auto& item : items) state.inventory[static_cast<int>(item.first)] = item.second;
    });
}
int StructureId(const Simulation& sim, Piece kind, Point location)
{
    int id = sim.FindNearestStructure(location, kind, 1);
    CHECK(id != -1);
    return id;
}
ResourceNode Node(const Simulation& sim, ResourceKind kind)
{
    for (const auto& n : sim.GetState().resources)
        if (n.kind == kind && sim.CanHarvest(n.id)) return n;
    CHECK(false);
    return {};
}
ResourceNode OvergrowthNode(const Simulation& sim, ResourceKind kind)
{
    for (const auto& n : sim.GetState().resources)
        if (n.kind == kind && !n.cleared) return n;
    CHECK(false);
    return {};
}
void GatherUntil(Simulation& sim, Item item, ResourceKind kind, int count)
{
    while (sim.Count(item) < count)
    {
        const auto node = Node(sim, kind);
        OK(sim.Harvest(node.id, node.position));
    }
}
void BuildingStock(Simulation& sim)
{
    Stock(sim, {{Item::Branch, 65}, {Item::Stone, 24}, {Item::BrambleCanes, 25}});
}
void BuildRoom(Simulation& sim, int x = -3, int y = 0)
{
    Edit(sim, [](State&) {});
    const Point center = CellCenter(x, y);
    OK(sim.Place(Piece::Foundation, x, y, 0, center));
    OK(sim.Place(Piece::Roof, x, y, 0, center));
    OK(sim.Place(Piece::Doorway, x, y, 0, center));
    for (int rotation : {1, 2, 3}) OK(sim.Place(Piece::Wall, x, y, rotation, center));
}
void UnchangedFailure(Simulation& sim, const std::function<Result()>& action)
{
    const std::string before = sim.Serialize();
    const auto revision = sim.GetRevision();
    CHECK(!action().ok);
    CHECK(sim.Serialize() == before);
    CHECK(sim.GetRevision() == revision);
}

void SkipToMorning()
{
    Simulation sim;
    const double energy = sim.GetState().energy;
    const auto revision = sim.GetRevision();
    sim.SkipToHourOfDay(8);
    CHECK(Close(sim.GetState().hour, 8));
    CHECK(sim.GetState().energy == energy);
    CHECK(sim.GetRevision() == revision + 1);
    sim.SkipToHourOfDay(8);
    CHECK(Close(sim.GetState().hour, 32));
    sim.SkipToHourOfDay(7);
    CHECK(Close(sim.GetState().hour, 55));
    sim.SkipToHourOfDay(25);
    sim.SkipToHourOfDay(std::numeric_limits<double>::quiet_NaN());
    CHECK(Close(sim.GetState().hour, 55));
}

void DefaultsAndValidation()
{
    Simulation sim;
    CHECK(sim.Count(Item::Knife) == 0);
    CHECK(sim.UsedCapacity() == 0);
    CHECK(sim.GetState().resources.size() >= 70);
    CHECK(sim.GetState().hour == 6);
    CHECK(sim.GetState().dayMinutes == 60);
    CHECK(std::string(ResourceName(ResourceKind::Flowers)) == "Meadow herb");
    CHECK(std::string(ItemName(Item::Flowers)) == "Meadow herb");
    CHECK(std::string(ItemName(Item::Timber)) == "Timber");
    CHECK(std::string(ItemName(Item::Firewood)) == "Firewood");
    CHECK(std::string(RecipeName(Recipe::SplitFirewood)) == "Split firewood");
    CHECK(sim.Count(static_cast<Item>(-1)) == 0);
    CHECK(std::string(RecipeName(static_cast<Recipe>(500))) == "Unknown recipe");
    CHECK(Close(StreamX(0), 1500));
    CHECK(IsNearWater({1680, 0}));
    CHECK(!IsNearWater({1681, 0}));
    CHECK(Close(CellCenter(-1, -1).x, -150));
    const auto branch = Node(sim, ResourceKind::Branches);
    CHECK(sim.FindNearestResource(branch.position, 0) == branch.id);
    CHECK(sim.FindNearestResource({-900000, -900000}, 100) == -1);
    CHECK(sim.FindNearestPlot(Home, 100) == -1);
    CHECK(sim.FindNearestStructure(Home, Piece::Bed, 100) == -1);
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto inf = std::numeric_limits<double>::infinity();
    CHECK(sim.FindNearestResource({nan, 0}, 300) == -1);
    CHECK(sim.FindNearestResource(Home, -1) == -1);
    CHECK(sim.FindNearestPlot(Home, inf) == -1);
    CHECK(!IsNearWater({nan, 0}));
    UnchangedFailure(sim, [&] { return sim.SetDayMinutes(nan); });
    UnchangedFailure(sim, [&] { return sim.SetDayMinutes(inf); });
    UnchangedFailure(sim, [&] { return sim.SetDayMinutes(0); });
    UnchangedFailure(sim, [&] { return sim.Craft(static_cast<Recipe>(-1), Home); });
    UnchangedFailure(sim, [&] { return sim.Eat(Item::Roots); });
    UnchangedFailure(sim, [&] { return sim.Harvest(-1, Home); });
    const auto before = sim.Serialize();
    sim.Advance(-1, Home);
    sim.Advance(nan, Home);
    sim.AdvanceGameHours(inf, Home);
    sim.AdvanceGameHours(1, {nan, 0});
    CHECK(before == sim.Serialize());
}

std::string SpentDescription(const Inventory& before, const Inventory& after)
{
    std::string description;
    for (int i = 0; i < ItemCount; ++i)
    {
        if (before[i] <= after[i]) continue;
        if (!description.empty()) description += " + ";
        description += std::to_string(before[i] - after[i]) + " " + ItemName(static_cast<Item>(i));
    }
    return description;
}
void RequirementsMatchTransactions()
{
    CHECK(std::string(RecipeRequirements(static_cast<Recipe>(-1))) == "Unknown recipe");
    CHECK(std::string(PieceRequirements(Piece::Count)) == "Unknown structure");
    CHECK(std::string(RecipeRequirements(Recipe::HerbedRoots)) ==
        "2 Roots + 1 Meadow herb; nearby fueled fire (no pot needed)");
    for (int i = 0; i < static_cast<int>(Recipe::Count); ++i)
    {
        const auto recipe = static_cast<Recipe>(i);
        Simulation sim;
        BuildingStock(sim);
        OK(sim.Place(Piece::Fire, -3, -1, 0, CellCenter(-3, -1)));
        OK(sim.AddFuel(sim.GetState().structures.back().id, CellCenter(-3, -1)));
        Stock(sim, {{Item::Hatchet, 1}, {Item::Branch, 40}, {Item::Stone, 20},
            {Item::RustedAxeHead, 1}, {Item::RustedHoeBlade, 1}, {Item::RustedScytheBlade, 1},
            {Item::RustedBillhookHead, 1}, {Item::RustedPickHead, 1},
            {Item::Roots, 10}, {Item::Flowers, 10}, {Item::Timber, 1}});
        const auto before = sim.GetState().inventory;
        const double hour = sim.GetState().hour;
        const char* description = RecipeRequirements(recipe);
        OK(sim.Craft(recipe, Home));
        CHECK(sim.GetState().hour == hour);
        const std::string spent = SpentDescription(before, sim.GetState().inventory);
        CHECK(std::string(description).substr(0, spent.size() + 1) == spent + ";");
        CHECK(description == RecipeRequirements(recipe));
    }

    for (int i = 0; i < static_cast<int>(Piece::Count); ++i)
    {
        const auto piece = static_cast<Piece>(i);
        if (!IsBuildable(piece)) continue;
        Simulation sim;
        BuildingStock(sim);
        if (piece == Piece::Wall || piece == Piece::Doorway || piece == Piece::Roof)
            OK(sim.Place(Piece::Foundation, -3, 0, 0, Home));
        const auto before = sim.GetState().inventory;
        const double hour = sim.GetState().hour;
        const char* description = PieceRequirements(piece);
        OK(sim.Place(piece, -3, 0, 0, Home));
        CHECK(sim.GetState().hour == hour);
        const std::string spent = SpentDescription(before, sim.GetState().inventory);
        const std::string text = description;
        CHECK(text.substr(0, text.find(';')) == spent);
        CHECK(description == PieceRequirements(piece));
    }
}

void StructuredRecipeAssessment()
{
    Simulation sim;
    const auto original = sim.Serialize();
    const auto originalRevision = sim.GetRevision();
    const auto missing = sim.AssessRecipe(Recipe::HaftBillhook, Home);
    CHECK(!missing.craftable);
    CHECK(missing.output == Item::Billhook && missing.outputCount == 1);
    // Hafting is by hand: no knife, no station.
    CHECK(missing.retainedTool == Item::Count && missing.retainedToolMet);
    CHECK(!missing.stationRequired && missing.stationMet && missing.capacityMet);
    CHECK(missing.ingredients.size() == 2);
    CHECK(missing.ingredients[0].item == Item::Branch && missing.ingredients[0].need == 2);
    CHECK(std::string(missing.ingredients[0].source) == "Fallen branches");
    CHECK(missing.ingredients[1].item == Item::RustedBillhookHead && missing.ingredients[1].need == 1);
    CHECK(std::string(missing.ingredients[1].source) == "Salvage piles around the manor");
    CHECK(sim.Serialize() == original && sim.GetRevision() == originalRevision);

    Stock(sim, {{Item::Branch, 2}, {Item::RustedBillhookHead, 1}});
    const auto ready = sim.AssessRecipe(Recipe::HaftBillhook, Home);
    CHECK(ready.craftable && ready.blocker.empty());
    for (const auto& ingredient : ready.ingredients) CHECK(ingredient.met);

    Stock(sim, {{Item::Roots, 2}});
    auto cooking = sim.AssessRecipe(Recipe::RoastedRoots, Home);
    CHECK(cooking.stationRequired && !cooking.stationMet && !cooking.craftable);
    CHECK(cooking.retainedTool == Item::Count && cooking.retainedToolMet);

    Simulation fire;
    BuildingStock(fire);
    const Point firePosition = CellCenter(-3, -1);
    OK(fire.Place(Piece::Fire, -3, -1, 0, firePosition));
    OK(fire.AddFuel(fire.GetState().structures.back().id, firePosition));
    Stock(fire, {{Item::Roots, 2}});
    cooking = fire.AssessRecipe(Recipe::RoastedRoots, firePosition);
    CHECK(cooking.craftable && cooking.stationMet);

    Stock(sim, {{Item::Timber, 1}});
    const auto firewood = sim.AssessRecipe(Recipe::SplitFirewood, Home);
    CHECK(firewood.retainedTool == Item::Hatchet && !firewood.retainedToolMet);
    CHECK(firewood.output == Item::Firewood && firewood.outputCount == 4);
    CHECK(std::string(firewood.ingredients[0].source) == "Mature trees, large stumps and fallen logs, with the axe");

    const auto invalid = sim.AssessRecipe(static_cast<Recipe>(-1), Home);
    CHECK(!invalid.craftable && invalid.output == Item::Count);

    Simulation complete;
    BuildingStock(complete);
    const Point completeFire = CellCenter(-3, -1);
    OK(complete.Place(Piece::Fire, -3, -1, 0, completeFire));
    OK(complete.AddFuel(complete.GetState().structures.back().id, completeFire));
    Stock(complete, {{Item::Hatchet, 1}, {Item::Branch, 40},
        {Item::RustedAxeHead, 1}, {Item::RustedHoeBlade, 1}, {Item::RustedScytheBlade, 1},
        {Item::RustedBillhookHead, 1}, {Item::RustedPickHead, 1}, {Item::Roots, 10},
        {Item::Flowers, 10}, {Item::Timber, 4}});
    const Item Outputs[] = {Item::Hatchet, Item::DiggingStick, Item::Scythe, Item::Billhook, Item::Pickaxe,
        Item::RoastedRoots, Item::HerbedRoots, Item::Firewood};
    const int OutputCounts[] = {1, 1, 1, 1, 1, 1, 1, 4};
    static_assert(sizeof(Outputs) / sizeof(Outputs[0]) == static_cast<int>(Recipe::Count), "Every recipe is assessed.");
    for (int index = 0; index < static_cast<int>(Recipe::Count); ++index)
    {
        const auto recipe = static_cast<Recipe>(index);
        const Point position = index == static_cast<int>(Recipe::RoastedRoots)
            || index == static_cast<int>(Recipe::HerbedRoots) ? completeFire : Home;
        const auto before = complete.Serialize();
        const auto revision = complete.GetRevision();
        const auto assessment = complete.AssessRecipe(recipe, position);
        CHECK(assessment.craftable && assessment.output == Outputs[index]);
        CHECK(assessment.outputCount == OutputCounts[index] && assessment.capacityMet);
        for (const auto& ingredient : assessment.ingredients)
            CHECK(ingredient.have >= ingredient.need && ingredient.met);
        CHECK(complete.Serialize() == before && complete.GetRevision() == revision);
    }

    Simulation full;
    Stock(full, {{Item::Knife, 1}, {Item::Hatchet, 1}, {Item::Timber, 1}, {Item::Stone, 117}});
    CHECK(full.UsedCapacity() == InventoryCapacity);
    const auto capacity = full.AssessRecipe(Recipe::SplitFirewood, Home);
    CHECK(!capacity.craftable && !capacity.capacityMet);
    CHECK(capacity.ingredients[0].met && capacity.retainedToolMet);
    CHECK(capacity.blocker == "Not enough pack space. Store some items in a chest first.");

    Simulation failed;
    failed.AdvanceGameHours(120, Home);
    CHECK(failed.GetState().failed);
    const auto failedAssessment = failed.AssessRecipe(Recipe::HaftAxe, Home);
    CHECK(!failedAssessment.craftable && !failedAssessment.blocker.empty());
}

void GameplayWalkthrough()
{
    Simulation sim;
    Edit(sim, [](State&) {});
    GatherUntil(sim, Item::Branch, ResourceKind::Branches, 15);
    GatherUntil(sim, Item::Stone, ResourceKind::Stones, 8);
    // The woodland has no salvage piles; hand her the rusted heads and haft them.
    for (Item head : {Item::RustedAxeHead, Item::RustedHoeBlade, Item::RustedBillhookHead}) OK(sim.GrantItems(head, 1));
    OK(sim.Craft(Recipe::HaftAxe, Home));
    OK(sim.Craft(Recipe::HaftHoe, Home));
    OK(sim.Craft(Recipe::HaftBillhook, Home));
    OK(sim.GrantItems(Item::WateringCan, 1));
    CHECK(sim.Count(Item::Hatchet) == 1);
    auto sapling = OvergrowthNode(sim, ResourceKind::Sapling);
    OK(sim.ClearOvergrowth(sapling.id, Item::Billhook, sapling.position));
    CHECK(!sim.CanHarvest(sapling.id));
    GatherUntil(sim, Item::Roots, ResourceKind::Roots, 6);
    CHECK(sim.Count(Item::Seeds) >= 2);
    GatherUntil(sim, Item::Flowers, ResourceKind::Flowers, 1);
    GatherUntil(sim, Item::Berries, ResourceKind::BerryBush, 10);
    OK(sim.Eat(Item::Berries));
    const Point garden = CellCenter(-2, -1);
    OK(sim.Till(CellToGarden(-2), CellToGarden(-1), garden));
    int plotId = sim.FindNearestPlot(garden, 1);
    CHECK(plotId != -1);
    OK(sim.Plant(plotId, garden));
    OK(sim.FillWater(WaterSource));
    CHECK(sim.Count(Item::Water) == 6);
    OK(sim.Water(plotId, garden));
    GatherUntil(sim, Item::Branch, ResourceKind::Branches, 40);
    GatherUntil(sim, Item::Stone, ResourceKind::Stones, 8);
    OK(sim.GrantItems(Item::BrambleCanes, 15));
    BuildRoom(sim);
    CHECK(sim.IsSheltered(Home));
    OK(sim.Place(Piece::Bed, -3, 0, 0, Home));
    const Point firePosition = CellCenter(-3, -1);
    OK(sim.Place(Piece::Fire, -3, -1, 0, firePosition));
    int fireId = StructureId(sim, Piece::Fire, firePosition);
    UnchangedFailure(sim, [&] { return sim.Craft(Recipe::RoastedRoots, firePosition); });
    OK(sim.AddFuel(fireId, firePosition));
    OK(sim.Craft(Recipe::RoastedRoots, firePosition));
    OK(sim.Craft(Recipe::HerbedRoots, firePosition));
    CHECK(sim.Count(Item::RoastedRoots) == 1);
    CHECK(sim.Count(Item::HerbedRoots) == 1);
    const Point chestPosition = CellCenter(-4, 0);
    OK(sim.Place(Piece::Chest, -4, 0, 0, chestPosition));
    const int chestId = StructureId(sim, Piece::Chest, chestPosition);
    OK(sim.Transfer(chestId, Item::Stone, 1, chestPosition));
    OK(sim.Transfer(chestId, Item::Stone, -1, chestPosition));
    sim.AdvanceGameHours(4, Home);
    CHECK(sim.GetState().plots[0].growth > 0.0);
    OK(sim.Weed(plotId, garden));
    const double energyBeforeMeal = sim.GetState().energy;
    CHECK(energyBeforeMeal < 100);
    OK(sim.Eat(Item::HerbedRoots));
    // A cooked meal restores some energy as well as food.
    CHECK(sim.GetState().energy > energyBeforeMeal || sim.GetState().energy == 100);
    const double beforeSleep = sim.GetState().hour;
    OK(sim.Sleep(8, Home));
    CHECK(Close(sim.GetState().hour, beforeSleep + 8));
    CHECK(sim.GetState().energy == 100);
    CHECK(!sim.IsNearFire(firePosition));
    for (int cycle = 0; cycle < 8 && sim.GetState().plots[0].growth < 1.0; ++cycle)
    {
        if (sim.Count(Item::Water) == 0) OK(sim.FillWater(WaterSource));
        if (sim.GetState().plots[0].moisture < 1) OK(sim.Water(plotId, garden));
        if (sim.GetState().plots[0].weeds > 0) OK(sim.Weed(plotId, garden));
        if (sim.Count(Item::Berries) < 3) GatherUntil(sim, Item::Berries, ResourceKind::BerryBush, 10);
        while (sim.GetState().hunger < 85) OK(sim.Eat(Item::Berries));
        sim.AdvanceGameHours(4, Home);
        OK(sim.Sleep(8, Home));
    }
    CHECK(sim.GetState().plots[0].growth == 1);
    const int roots = sim.Count(Item::Roots);
    const int seeds = sim.Count(Item::Seeds);
    OK(sim.HarvestCrop(plotId, garden));
    CHECK(sim.Count(Item::Roots) == roots + 4);
    CHECK(sim.Count(Item::Seeds) == seeds + 2);
    CHECK(!sim.GetState().plots[0].planted);
    CHECK(sim.GetState().plots[0].growth == 0);
    OK(sim.Plant(plotId, garden));
    OK(sim.AddFuel(fireId, firePosition));
    OK(sim.Transfer(chestId, Item::Roots, 1, chestPosition));
    const auto save = sim.Serialize();
    Simulation loaded;
    OK(loaded.Deserialize(save));
    CHECK(loaded.Serialize() == save);
    CHECK(loaded.IsSheltered(Home));
    CHECK(!loaded.CanHarvest(sapling.id));
    CHECK(loaded.GetState().plots[0].planted);
    CHECK(loaded.IsNearFire(firePosition));
    CHECK(loaded.GetState().hour == sim.GetState().hour);
}

void AtomicTransactions()
{
    Simulation sim;
    const auto roots = Node(sim, ResourceKind::Roots);
    Stock(sim, {{Item::Knife, 1}, {Item::Stone, 116}});
    UnchangedFailure(sim, [&] { return sim.Harvest(roots.id, roots.position); });
    UnchangedFailure(sim, [&] { return sim.Clear(roots.id, roots.position); });
    CHECK(sim.CanHarvest(roots.id));
    UnchangedFailure(sim, [&] { return sim.Harvest(roots.id, {3900, 3900}); });
    UnchangedFailure(sim, [&] { return sim.Craft(Recipe::HaftAxe, Home); });
    UnchangedFailure(sim, [&] { return sim.Place(Piece::Foundation, -3, 0, 0, Home); });
    UnchangedFailure(sim, [&] { return sim.FillWater(WaterSource); });
    Stock(sim, {{Item::WateringCan, 1}, {Item::Water, 1}, {Item::Stone, 117}});
    OK(sim.FillWater(WaterSource));
    CHECK(sim.UsedCapacity() == 118);
    CHECK(sim.Count(Item::Water) == 6);
    Stock(sim, {{Item::Knife, 1}, {Item::WateringCan, 1}, {Item::Branch, 112}});
    OK(sim.FillWater(WaterSource));
    CHECK(sim.UsedCapacity() == 114);
    CHECK(sim.Count(Item::Water) == 6);
    UnchangedFailure(sim, [&] { return sim.FillWater(WaterSource); });
    Stock(sim, {{Item::Knife, 1}, {Item::Roots, 2}});
    UnchangedFailure(sim, [&] { return sim.Craft(Recipe::RoastedRoots, Home); });
    UnchangedFailure(sim, [&] { return sim.Craft(Recipe::HerbedRoots, Home); });
    UnchangedFailure(sim, [&] { return sim.Eat(Item::Berries); });
    UnchangedFailure(sim, [&] { return sim.Clear(Node(sim, ResourceKind::Branches).id, Home); });
}

void RegrowthAndClearing()
{
    Simulation sim;
    const auto branch = Node(sim, ResourceKind::Branches);
    OK(sim.Harvest(branch.id, branch.position));
    CHECK(!sim.CanHarvest(branch.id));
    CHECK(sim.FindNearestResource(branch.position, 0) == -1);
    UnchangedFailure(sim, [&] { return sim.Harvest(branch.id, branch.position); });
    ResourceNode branchAfter;
    OK(sim.ResolveGeneratedResource(branch.key, branchAfter));
    const double ready = branchAfter.readyAtHour;
    CHECK(ready == 30.0);
    Edit(sim, [&](State& state) { state.hour = ready - 0.02; });
    sim.AdvanceGameHours(0.01, Home);
    CHECK(!sim.CanHarvest(branch.id));
    sim.AdvanceGameHours(0.02, Home);
    CHECK(sim.CanHarvest(branch.id));
    OK(sim.Harvest(branch.id, branch.position));
    const int branches = sim.Count(Item::Branch);
    OK(sim.Clear(branch.id, branch.position));
    CHECK(sim.Count(Item::Branch) == branches);
    OK(sim.ResolveGeneratedResource(branch.key, branchAfter));
    CHECK(branchAfter.cleared);
    Edit(sim, [](State& state) { state.hour += 500; });
    CHECK(!sim.CanHarvest(branch.id));
    Simulation loaded;
    OK(loaded.Deserialize(sim.Serialize()));
    CHECK(!loaded.CanHarvest(branch.id));
    ResourceNode sapling;
    for (const auto& node : loaded.GetState().resources)
        if (node.kind == ResourceKind::Sapling) { sapling = node; break; }
    // Saplings are overgrowth now: the billhook clears them, not hands or the axe.
    CHECK(!loaded.CanHarvest(sapling.id));
    UnchangedFailure(loaded, [&] { return loaded.Clear(sapling.id, sapling.position); });
    Stock(loaded, {{Item::Hatchet, 1}});
    UnchangedFailure(loaded, [&] { return loaded.ClearOvergrowth(sapling.id, Item::Hatchet, sapling.position); });
    Stock(loaded, {{Item::Billhook, 1}});
    CHECK(loaded.FindNearestResource(branch.position, 100) == -1);
    OK(loaded.ClearOvergrowth(sapling.id, Item::Billhook, sapling.position));
    CHECK(loaded.Count(Item::Branch) >= 3 && loaded.Count(Item::Branch) <= 4);
    CHECK(loaded.Count(Item::Kindling) == 1 && loaded.Count(Item::Fiber) == 0);
    CHECK(!loaded.CanHarvest(sapling.id));
}

void PlacementAndShelter()
{
    Simulation sim;
    BuildingStock(sim);
    UnchangedFailure(sim, [&] { return sim.Place(Piece::Roof, -3, 0, 0, Home); });
    UnchangedFailure(sim, [&] { return sim.Place(Piece::Count, -3, 0, 0, Home); });
    UnchangedFailure(sim, [&] { return sim.Place(Piece::Foundation, 500, 0, 0, Home); });
    UnchangedFailure(sim, [&] { return sim.Place(Piece::Foundation, -3, 0, 0, {3900, 3900}); });
    ResourceNode sapling;
    for (const auto& node : sim.GetState().resources)
        if (node.kind == ResourceKind::Sapling) { sapling = node; break; }
    int sx = static_cast<int>(std::floor(sapling.position.x / CellSize));
    int sy = static_cast<int>(std::floor(sapling.position.y / CellSize));
    UnchangedFailure(sim, [&] { return sim.Place(Piece::Foundation, sx, sy, 0, sapling.position); });
    OK(sim.Place(Piece::Foundation, -3, 0, -4, Home));
    CHECK(sim.GetState().structures.back().rotation == 0);
    UnchangedFailure(sim, [&] { return sim.Place(Piece::Foundation, -3, 0, 1, Home); });
    OK(sim.Place(Piece::Wall, -3, 0, -1, Home));
    CHECK(sim.GetState().structures.back().rotation == 3);
    CHECK(!sim.IsSheltered(Home));
    OK(sim.Place(Piece::Roof, -3, 0, 0, Home));
    CHECK(!sim.IsSheltered(Home));
    UnchangedFailure(sim, [&] { return sim.Place(Piece::Roof, -3, 0, 2, Home); });
    OK(sim.Place(Piece::Wall, -3, 0, 0, Home));
    OK(sim.Place(Piece::Wall, -3, 0, 2, Home));
    CHECK(!sim.IsSheltered(Home));
    OK(sim.Place(Piece::Doorway, -3, 0, 5, Home));
    CHECK(sim.GetState().structures.back().rotation == 1);
    CHECK(sim.IsSheltered(Home));
    CHECK(!sim.IsSheltered(CellCenter(-2, 0)));
    UnchangedFailure(sim, [&] { return sim.Place(Piece::Wall, -3, 0, 1, Home); });
    OK(sim.Place(Piece::Foundation, -2, 0, 0, Home));
    UnchangedFailure(sim, [&] { return sim.Place(Piece::Wall, -2, 0, 3, Home); });
    OK(sim.Place(Piece::Bed, -3, 0, 0, Home));
    UnchangedFailure(sim, [&] { return sim.Place(Piece::Fire, -3, 0, 0, Home); });
    Stock(sim, {{Item::DiggingStick, 1}, {Item::Branch, 20}, {Item::Stone, 20}});
    UnchangedFailure(sim, [&] { return sim.Till(CellToGarden(-3), CellToGarden(0), Home); });
    OK(sim.Till(CellToGarden(-2), CellToGarden(-1), CellCenter(-2, -1)));
    UnchangedFailure(sim, [&] { return sim.Till(CellToGarden(-2), CellToGarden(-1), CellCenter(-2, -1)); });
    UnchangedFailure(sim, [&] { return sim.Place(Piece::Fire, -2, -1, 0, CellCenter(-2, -1)); });
}

void FreeStandingBuildings()
{
    Simulation sim;
    BuildingStock(sim);
    const auto offset = [](Point base, Point local, double yaw) {
        const Point turned = RotateYaw(local, yaw);
        return Point{base.x + turned.x, base.y + turned.y};
    };
    // A first foundation stands wherever it is aimed, at the free heading.
    const Point site{-600, 100};
    const auto first = sim.ResolvePlacement(Piece::Foundation, site, 30.0, 0);
    CHECK(!first.snapped && first.buildingId == -1 && first.blocker.empty());
    CHECK(Close(BuildingCellCenter(first.frame, 0, 0).x, site.x, 1e-6));
    CHECK(Close(BuildingCellCenter(first.frame, 0, 0).y, site.y, 1e-6));
    OK(sim.Place(first, site));
    CHECK(sim.GetState().buildings.size() == 1 && Close(sim.GetState().buildings[0].yaw, 30.0));
    const int building = sim.GetState().structures.back().buildingId;
    CHECK(building == sim.GetState().buildings[0].id);
    CHECK(Close(sim.StructureCenter(sim.GetState().structures.back()).x, site.x, 1e-6));

    // Aimed near its open east side, the next foundation snaps onto the same turned grid whatever
    // the free heading says.
    const auto east = sim.ResolvePlacement(Piece::Foundation, offset(site, {330, -40}, 30.0), 77.0, 0);
    CHECK(east.snapped && east.buildingId == building && east.cellX == 1 && east.cellY == 0);
    OK(sim.Place(east, site));
    CHECK(Close(sim.StructureCenter(sim.GetState().structures.back()).x, offset(site, {300, 0}, 30.0).x, 1e-6));
    // Aimed at the middle of the floor, a foundation snaps onto one of its open sides.
    const auto middle = sim.ResolvePlacement(Piece::Foundation, site, 77.0, 0);
    CHECK(middle.snapped && middle.buildingId == building && middle.cellX == 0 && middle.cellY == 1);
    CHECK(sim.CheckPlacement(middle, site).ok);
    const auto nearWest = sim.ResolvePlacement(Piece::Foundation, offset(site, {-120, 20}, 30.0), 77.0, 0);
    CHECK(nearWest.snapped && nearWest.cellX == -1 && nearWest.cellY == 0);
    // A world-grid foundation overlapping the turned floor is refused too.
    UnchangedFailure(sim, [&] { return sim.Place(Piece::Foundation, -2, 0, 0, site); });

    // Walls and roofs need a foundation; aimed at one, they join its cell and edges. Walls and
    // doorways take the edge nearest the aim whatever rotation is asked for.
    const auto stray = sim.ResolvePlacement(Piece::Wall, {-1300, -800}, 0.0, 0);
    CHECK(!stray.blocker.empty());
    UnchangedFailure(sim, [&] { return sim.Place(stray, {-1300, -800}); });
    struct CabinPiece { Piece kind; Point aim; int rotation; };
    const CabinPiece cabin[] = {{Piece::Roof, {40, 30}, 0}, {Piece::Doorway, {10, 120}, 0},
        {Piece::Wall, {125, 20}, 1}, {Piece::Wall, {-15, -110}, 2}, {Piece::Wall, {-140, 40}, 3}};
    for (const auto& piece : cabin)
    {
        const auto target = sim.ResolvePlacement(piece.kind, offset(site, piece.aim, 30.0), 200.0, 3);
        CHECK(target.snapped && target.buildingId == building && target.cellX == 0 && target.cellY == 0);
        OK(sim.Place(target, site));
        CHECK(piece.kind == Piece::Roof || sim.GetState().structures.back().rotation == piece.rotation);
    }
    // Aimed at a shared edge that is already walled, a wall moves on to the nearest free edge.
    const auto again = sim.ResolvePlacement(Piece::Wall, offset(site, {165, 10}, 30.0), 0.0, 2);
    CHECK(again.snapped && again.buildingId == building && again.cellX == 1 && again.cellY == 0 && again.rotation == 0);
    CHECK(sim.CheckPlacement(again, site).ok);
    CHECK(sim.IsSheltered(site));
    CHECK(sim.IsSheltered(offset(site, {120, 120}, 30.0)));
    CHECK(!sim.IsSheltered(offset(site, {300, 0}, 30.0)));
    CHECK(!sim.IsSheltered(offset(site, {0, 200}, 30.0)));

    // Furniture aimed inside a floor joins it; elsewhere it stands centred on the aim.
    const auto bed = sim.ResolvePlacement(Piece::Bed, site, 0.0, 1);
    CHECK(bed.snapped && bed.buildingId == building && bed.rotation == 1);
    OK(sim.Place(bed, site));
    const Point chestSpot{-600, -500};
    const auto chest = sim.ResolvePlacement(Piece::Chest, chestSpot, 45.0, 3);
    CHECK(!chest.snapped && chest.buildingId == -1 && chest.rotation == 0);
    OK(sim.Place(chest, chestSpot));
    const Structure placedChest = sim.GetState().structures.back();
    const Footprint chestGround = StructureFootprint(sim.GetState(), placedChest);
    CHECK(Close(chestGround.center.x, chestSpot.x, 1e-6) && Close(chestGround.center.y, chestSpot.y, 1e-6));
    CHECK(Close(chestGround.yaw, 45.0));
    CHECK(sim.FindNearestStructure(chestSpot, Piece::Chest, 1) == placedChest.id);
    CHECK(sim.GetState().buildings.size() == 2);

    // Squared up on a world grid cell, a free piece is simply that grid cell.
    const auto grid = sim.ResolvePlacement(Piece::Foundation, {CellCenter(2, -2).x + 3, CellCenter(2, -2).y}, 0.0, 0);
    CHECK(!grid.snapped && grid.buildingId == 0 && grid.cellX == 2 && grid.cellY == -2);
    const auto turnedGrid = sim.ResolvePlacement(Piece::Chest, CellCenter(2, -2), 90.0, 0);
    CHECK(turnedGrid.buildingId == 0 && turnedGrid.rotation == 3);

    // Buildings persist exactly; structures naming a missing building are corrupt.
    const std::string saved = sim.Serialize();
    Simulation loaded;
    OK(loaded.Deserialize(saved));
    CHECK(loaded.Serialize() == saved);
    CHECK(loaded.IsSheltered(site));
    State missing = sim.GetState();
    missing.structures.back().buildingId = missing.nextId + 5;
    CHECK(!loaded.Deserialize(Encode(missing)).ok);
    State turned = sim.GetState();
    turned.buildings[0].yaw = 360.0;
    CHECK(!loaded.Deserialize(Encode(turned)).ok);

    // Soil under the free chest cannot be tilled; the garden works around it.
    Stock(sim, {{Item::DiggingStick, 1}});
    const int gardenX = GardenCell(chestSpot.x), gardenY = GardenCell(chestSpot.y);
    UnchangedFailure(sim, [&] { return sim.Till(gardenX, gardenY, GardenCellCenter(gardenX, gardenY)); });
    OK(sim.Till(gardenX, gardenY - 3, GardenCellCenter(gardenX, gardenY - 3)));

    // A snap blocked by something else standing there yields to the next-nearest free side.
    Stock(sim, {{Item::Branch, 20}, {Item::BrambleCanes, 6}, {Item::Stone, 6}});
    const Point northSpot = offset(site, {0, 300}, 30.0);
    OK(sim.Place(sim.ResolvePlacement(Piece::Chest, northSpot, 10.0, 0), northSpot));
    const auto corner = sim.ResolvePlacement(Piece::Foundation, offset(site, {-160, 180}, 30.0), 0.0, 0);
    CHECK(corner.snapped && corner.buildingId == building && corner.cellX == -1 && corner.cellY == 0);
    OK(sim.Place(corner, site));

    // Older saves put everything on building 0.
    Simulation legacy;
    // Older stocks predate the estate items, so only pre-pivot materials can round-trip.
    Stock(legacy, {{Item::Branch, 65}, {Item::Stone, 24}});
    OK(legacy.Place(Piece::Foundation, -3, 0, 0, Home));
    Simulation migrated;
    OK(migrated.Deserialize(Encode(legacy.GetState(), GardenSquareSaveVersion)));
    CHECK(migrated.GetState().buildings.empty() && migrated.GetState().structures.back().buildingId == 0);
}

void MultiCellShelter()
{
    Simulation sim;
    BuildingStock(sim);
    for (int x : {-3, -2})
    {
        const Point p = CellCenter(x, 0);
        OK(sim.Place(Piece::Foundation, x, 0, 0, p));
        OK(sim.Place(Piece::Roof, x, 0, 0, p));
        OK(sim.Place(Piece::Wall, x, 0, 0, p));
        OK(sim.Place(Piece::Wall, x, 0, 2, p));
    }
    OK(sim.Place(Piece::Wall, -3, 0, 3, Home));
    CHECK(!sim.IsSheltered(Home));
    CHECK(!sim.IsSheltered(CellCenter(-2, 0)));
    OK(sim.Place(Piece::Doorway, -2, 0, 1, CellCenter(-2, 0)));
    CHECK(sim.IsSheltered(Home));
    CHECK(sim.IsSheltered(CellCenter(-2, 0)));
    Edit(sim, [](State& state) {
        for (auto i = state.structures.begin(); i != state.structures.end(); ++i)
            if (i->kind == Piece::Roof && i->cellX == -2) { state.structures.erase(i); break; }
    });
    CHECK(!sim.IsSheltered(Home));
    OK(sim.Place(Piece::Wall, -3, 0, 1, Home));
    CHECK(sim.IsSheltered(Home));
    CHECK(!sim.IsSheltered(CellCenter(-2, 0)));
}

void CardinalEdgesAndPlacementReach()
{
    const int dx[] = {0, 1, 0, -1};
    const int dy[] = {1, 0, -1, 0};
    for (int rotation = 0; rotation < 4; ++rotation)
    {
        Simulation sim;
        BuildingStock(sim);
        const int x = -3 + dx[rotation], y = dy[rotation];
        OK(sim.Place(Piece::Foundation, -3, 0, 0, Home));
        OK(sim.Place(Piece::Foundation, x, y, 0, CellCenter(x, y)));
        OK(sim.Place(Piece::Wall, -3, 0, rotation, Home));
        UnchangedFailure(sim, [&] {
            return sim.Place(Piece::Doorway, x, y, (rotation + 2) % 4, CellCenter(x, y));
        });
    }
    Simulation sim;
    BuildingStock(sim);
    UnchangedFailure(sim, [&] { return sim.Place(Piece::Foundation, -3, 0, 0, {Home.x + 700.1, Home.y}); });
    OK(sim.Place(Piece::Foundation, -3, 0, 0, {Home.x + 700, Home.y}));
}

void FireAndStorage()
{
    Simulation sim;
    BuildingStock(sim);
    const Point a = CellCenter(-3, 0), b = CellCenter(4, 0), chest = CellCenter(-4, 0);
    OK(sim.Place(Piece::Fire, -3, 0, 0, a));
    OK(sim.Place(Piece::Fire, 4, 0, 0, b));
    OK(sim.Place(Piece::Chest, -4, 0, 0, chest));
    const int fa = StructureId(sim, Piece::Fire, a), fb = StructureId(sim, Piece::Fire, b);
    const int ch = StructureId(sim, Piece::Chest, chest);
    CHECK(!sim.IsNearFire(a));
    OK(sim.AddFuel(fa, a));
    OK(sim.AddFuel(fb, b));
    OK(sim.AddFuel(fb, b));
    CHECK(sim.IsNearFire(a));
    CHECK(sim.IsNearFire(b));
    CHECK(!sim.IsNearFire({3900, 3900}));
    UnchangedFailure(sim, [&] { return sim.AddFuel(fa, b); });
    UnchangedFailure(sim, [&] { return sim.AddFuel(ch, chest); });
    sim.AdvanceGameHours(5, {3900, 3900});
    CHECK(!sim.IsNearFire(a));
    CHECK(sim.IsNearFire(b));
    CHECK(Close(sim.GetState().structures[0].fuelHours, 0));
    CHECK(Close(sim.GetState().structures[1].fuelHours, 3));
    int branchCount = sim.Count(Item::Branch);
    OK(sim.Transfer(ch, Item::Branch, 3, chest));
    CHECK(sim.Count(Item::Branch) == branchCount - 3);
    CHECK(sim.GetState().structures[2].storage[static_cast<int>(Item::Branch)] == 3);
    OK(sim.Transfer(ch, Item::Branch, -2, chest));
    CHECK(sim.Count(Item::Branch) == branchCount - 1);
    UnchangedFailure(sim, [&] { return sim.Transfer(ch, Item::Branch, -2, chest); });
    UnchangedFailure(sim, [&] { return sim.Transfer(ch, Item::Roots, 1, chest); });
    UnchangedFailure(sim, [&] { return sim.Transfer(ch, Item::Branch, 0, chest); });
    UnchangedFailure(sim, [&] { return sim.Transfer(ch, Item::Branch, std::numeric_limits<int>::min(), chest); });
    UnchangedFailure(sim, [&] { return sim.Transfer(ch, static_cast<Item>(-1), 1, chest); });
    UnchangedFailure(sim, [&] { return sim.Transfer(ch, Item::Branch, 1, b); });
    UnchangedFailure(sim, [&] { return sim.Transfer(fa, Item::Branch, 1, a); });
    Stock(sim, {{Item::Stone, 120}});
    UnchangedFailure(sim, [&] { return sim.Transfer(ch, Item::Branch, -1, chest); });
    Edit(sim, [](State& state) {
        state.structures[2].storage.fill(0);
        state.structures[2].storage[static_cast<int>(Item::Branch)] = ChestCapacity;
    });
    UnchangedFailure(sim, [&] { return sim.Transfer(ch, Item::Stone, 1, chest); });
    Stock(sim, {{Item::Branch, 20}});
    for (int i = 0; i < 12; ++i) OK(sim.AddFuel(fa, a));
    UnchangedFailure(sim, [&] { return sim.AddFuel(fa, a); });
    Simulation restored;
    OK(restored.Deserialize(sim.Serialize()));
    CHECK(restored.Serialize() == sim.Serialize());
}

void TimberAndFirewoodTransactions()
{
    static_assert(static_cast<int>(Item::HerbedRoots) == 13, "Existing item IDs are unchanged");
    static_assert(static_cast<int>(Item::Timber) == 14, "Timber appends after existing items");
    static_assert(static_cast<int>(Item::Firewood) == 15, "Firewood appends after Timber");
    static_assert(static_cast<int>(Item::Machete) == 16, "The machete appends after Firewood");
    static_assert(static_cast<int>(Item::Fur) == 17, "Fur appends after the machete");
    static_assert(ItemCount > static_cast<int>(Item::Fur), "Later items append after fur");
    static_assert(static_cast<int>(Recipe::HaftPickaxe) == 4, "Five hafting recipes replace the knife-crafted tools");
    static_assert(static_cast<int>(Recipe::SplitFirewood) == 7, "Split Firewood follows cooking");
    static_assert(static_cast<int>(Recipe::Count) == 8, "Hafting, cooking and one processing recipe are present");

    Simulation sim;
    BuildingStock(sim);
    const Point firePosition = CellCenter(-3, -1);
    const Point chestPosition = CellCenter(-4, 0);
    OK(sim.Place(Piece::Fire, -3, -1, 0, firePosition));
    OK(sim.Place(Piece::Chest, -4, 0, 0, chestPosition));
    const int fire = StructureId(sim, Piece::Fire, firePosition);
    const int chest = StructureId(sim, Piece::Chest, chestPosition);

    CHECK(std::string(RecipeRequirements(Recipe::SplitFirewood)) ==
        "1 Timber; axe required");
    Stock(sim, {{Item::Hatchet, 1}, {Item::Timber, 1}, {Item::Branch, 2}});
    OK(sim.Craft(Recipe::SplitFirewood, Home));
    CHECK(sim.Count(Item::Timber) == 0 && sim.Count(Item::Firewood) == 4);
    CHECK(sim.Count(Item::Hatchet) == 1 && sim.Count(Item::Branch) == 2);

    auto fueled = sim.AddFuel(fire, firePosition);
    OK(fueled);
    CHECK(fueled.message.find("firewood") != std::string::npos);
    CHECK(sim.Count(Item::Firewood) == 3 && sim.Count(Item::Branch) == 2);
    CHECK(Close(sim.GetState().structures[0].fuelHours, 4));

    Stock(sim, {{Item::Branch, 2}});
    fueled = sim.AddFuel(fire, firePosition);
    OK(fueled);
    CHECK(fueled.message.find("branch") != std::string::npos);
    CHECK(sim.Count(Item::Branch) == 1);
    CHECK(Close(sim.GetState().structures[0].fuelHours, 8));
    Stock(sim, {});
    const auto noFuel = sim.AddFuel(fire, firePosition);
    CHECK(!noFuel && noFuel.message.find("firewood or a branch") != std::string::npos);

    Stock(sim, {{Item::Hatchet, 1}, {Item::Timber, 2}, {Item::Firewood, 3}});
    OK(sim.Transfer(chest, Item::Timber, 1, chestPosition));
    OK(sim.Transfer(chest, Item::Firewood, 2, chestPosition));
    CHECK(sim.Count(Item::Timber) == 1 && sim.Count(Item::Firewood) == 1);
    CHECK(sim.GetState().structures[1].storage[static_cast<int>(Item::Timber)] == 1);
    CHECK(sim.GetState().structures[1].storage[static_cast<int>(Item::Firewood)] == 2);
    Simulation restored;
    OK(restored.Deserialize(sim.Serialize()));
    CHECK(restored.Serialize() == sim.Serialize());
    CHECK(restored.Count(Item::Timber) == 1 && restored.Count(Item::Firewood) == 1);

    Stock(sim, {{Item::Timber, 1}});
    const auto noHatchet = sim.Craft(Recipe::SplitFirewood, Home);
    CHECK(!noHatchet && noHatchet.message.find("axe") != std::string::npos);
    Stock(sim, {{Item::Hatchet, 1}});
    const auto noTimber = sim.Craft(Recipe::SplitFirewood, Home);
    CHECK(!noTimber && noTimber.message.find("Timber") != std::string::npos);
    Stock(sim, {{Item::Hatchet, 1}, {Item::Timber, 1}, {Item::Stone, 118}});
    UnchangedFailure(sim, [&] { return sim.Craft(Recipe::SplitFirewood, Home); });

    Simulation current;
    const auto before = current.Serialize();
    CHECK(current.Deserialize(Encode(current.GetState(), LegacySimulationSaveVersion - 1)).code ==
        ResultCode::UnsupportedVersion);
    CHECK(current.Serialize() == before);
    // Version 7 (before the machete and underbrush clearing) still loads, with neither.
    Simulation legacy;
    OK(legacy.Deserialize(Encode(current.GetState(), LegacySimulationSaveVersion)));
    CHECK(legacy.Count(Item::Machete) == 0 && legacy.GetState().clearedUnderbrush.empty());
    // Version 8 stored crop plots on whole building cells; each becomes that cell's middle square.
    Simulation farm;
    Stock(farm, {{Item::DiggingStick, 1}});
    OK(farm.Till(CellToGarden(-2), CellToGarden(-1), CellCenter(-2, -1)));
    State old = farm.GetState();
    old.plots[0].cellX = -2;
    old.plots[0].cellY = -1;
    Simulation moved;
    OK(moved.Deserialize(Encode(old, GardenSquareSaveVersion - 1)));
    CHECK(moved.GetState().plots[0].cellX == CellToGarden(-2) && moved.GetState().plots[0].cellY == CellToGarden(-1));
}

void GardenSquares()
{
    Simulation sim;
    Stock(sim, {{Item::DiggingStick, 1}, {Item::Seeds, 2}, {Item::Berries, 1}});
    const Point garden = CellCenter(-2, -1);
    const int x = CellToGarden(-2), y = CellToGarden(-1);
    // Each till turns over one small square; its neighbours in the same building cell stay untilled.
    OK(sim.Till(x, y, garden));
    CHECK(sim.GetState().plots.size() == 1);
    OK(sim.Till(x + 1, y, garden));
    OK(sim.Till(x, y - 1, garden));
    CHECK(sim.GetState().plots.size() == 3);
    UnchangedFailure(sim, [&] { return sim.Till(x + 1, y, garden); });
    CHECK(Close(GardenCellCenter(x, y).x, garden.x) && Close(GardenCellCenter(x, y).y, garden.y));
    CHECK(GardenToCell(x - 1) == -2 && GardenToCell(x + 1) == -2 && GardenToCell(x + 2) == -1);
    CHECK(GardenToCell(-1) == -1 && GardenToCell(-3) == -1 && GardenToCell(-4) == -2);
    // And each square is planted with its own choice.
    OK(sim.Plant(sim.GetState().plots[0].id, garden, CropKind::Roots));
    OK(sim.Plant(sim.GetState().plots[1].id, garden, CropKind::Berries));
    CHECK(sim.GetState().plots[0].kind == CropKind::Roots && sim.GetState().plots[1].kind == CropKind::Berries);
    CHECK(!sim.GetState().plots[2].planted);
    // No building on a building cell with a garden square in it.
    BuildingStock(sim);
    UnchangedFailure(sim, [&] { return sim.Place(Piece::Foundation, -2, -1, 0, garden); });
}

void FarmingAndRain()
{
    Simulation sim;
    const Point garden = CellCenter(-2, -1);
    Stock(sim, {{Item::Seeds, 3}, {Item::DiggingStick, 1}, {Item::WateringCan, 1}});
    OK(sim.Till(CellToGarden(-2), CellToGarden(-1), garden));
    int id = sim.FindNearestPlot(garden, 1);
    UnchangedFailure(sim, [&] { return sim.HarvestCrop(id, garden); });
    OK(sim.Plant(id, garden));
    UnchangedFailure(sim, [&] { return sim.Plant(id, garden); });
    UnchangedFailure(sim, [&] { return sim.Water(id, garden); });
    UnchangedFailure(sim, [&] { return sim.FillWater(Home); });
    OK(sim.FillWater(WaterSource));
    OK(sim.Water(id, garden));
    UnchangedFailure(sim, [&] { return sim.Water(id, garden); });
    UnchangedFailure(sim, [&] { return sim.Weed(id, garden); });
    Simulation dry, weedy;
    OK(dry.Deserialize(sim.Serialize()));
    OK(weedy.Deserialize(sim.Serialize()));
    Edit(dry, [](State& state) { state.plots[0].moisture = 0; });
    Edit(weedy, [](State& state) { state.plots[0].weeds = 1; });
    sim.AdvanceGameHours(4, Home);
    dry.AdvanceGameHours(4, Home);
    weedy.AdvanceGameHours(4, Home);
    CHECK(sim.GetState().plots[0].moisture < 1);
    CHECK(sim.GetState().plots[0].weeds > 0 && sim.GetState().plots[0].weeds < 0.1);
    CHECK(dry.GetState().plots[0].planted);
    CHECK(dry.GetState().plots[0].growth > 0);
    CHECK(sim.GetState().plots[0].growth > dry.GetState().plots[0].growth);
    CHECK(sim.GetState().plots[0].growth > weedy.GetState().plots[0].growth);
    const double growth = weedy.GetState().plots[0].growth;
    OK(weedy.Weed(id, garden));
    CHECK(weedy.GetState().plots[0].growth == growth);
    CHECK(weedy.GetState().plots[0].weeds == 0);
    Edit(dry, [](State& state) { state.hour = 33; });
    CHECK(dry.IsRaining());
    dry.AdvanceGameHours(2, Home);
    CHECK(dry.GetState().plots[0].moisture > 0.5);
    CHECK(dry.GetState().plots[0].growth > 0);
    Edit(dry, [](State& state) { state.hour = 39; });
    CHECK(!dry.IsRaining());
    Edit(dry, [](State& state) { state.plots[0].growth = 1; });
    Stock(dry, {{Item::Stone, 115}});
    UnchangedFailure(dry, [&] { return dry.HarvestCrop(id, garden); });
    Stock(dry, {{Item::Stone, 114}});
    OK(dry.HarvestCrop(id, garden));
    CHECK(dry.UsedCapacity() == 120);
    CHECK(dry.Count(Item::Roots) == 4 && dry.Count(Item::Seeds) == 2);
    UnchangedFailure(dry, [&] { return dry.HarvestCrop(id, garden); });
    OK(dry.Plant(id, garden));
}

void GrowBerryBush(Simulation& sim, int plotId, Point garden)
{
    for (int cycle = 0; cycle < 12 && sim.GetState().plots[0].growth < 1; ++cycle)
    {
        if (sim.Count(Item::Water) == 0) OK(sim.FillWater(WaterSource));
        if (sim.GetState().plots[0].moisture < 1) OK(sim.Water(plotId, garden));
        if (sim.GetState().plots[0].weeds > 0) OK(sim.Weed(plotId, garden));
        while (sim.GetState().hunger < 75)
        {
            if (sim.Count(Item::Berries) == 0)
                GatherUntil(sim, Item::Berries, ResourceKind::BerryBush, 5);
            OK(sim.Eat(Item::Berries));
        }
        OK(sim.Sleep(8, Home));
        CHECK(!sim.GetState().failed);
        CHECK(sim.GetState().plots[0].kind == CropKind::Berries);
        CHECK(sim.GetState().plots[0].planted);
    }
    CHECK(sim.GetState().plots[0].growth == 1);
}

void BerryCropCycle()
{
    CHECK(std::string(CropName(CropKind::Roots)) == "Roots");
    CHECK(std::string(CropName(CropKind::Berries)) == "Berries");
    CHECK(std::string(CropName(static_cast<CropKind>(-1))) == "Unknown crop");
    CHECK(std::string(CropName(CropKind::Count)) == "Unknown crop");
    Simulation sim;
    BuildingStock(sim);
    BuildRoom(sim);
    OK(sim.Place(Piece::Bed, -3, 0, 0, Home));
    Stock(sim, {{Item::Knife, 1}, {Item::DiggingStick, 1}, {Item::WateringCan, 1}, {Item::Seeds, 2}});
    const Point berries = CellCenter(-3, -1);
    const Point roots = CellCenter(-2, -1);
    OK(sim.Till(CellToGarden(-3), CellToGarden(-1), berries));
    OK(sim.Till(CellToGarden(-2), CellToGarden(-1), roots));
    const int berryId = sim.FindNearestPlot(berries, 1);
    const int rootId = sim.FindNearestPlot(roots, 1);
    CHECK(sim.GetState().plots[0].kind == CropKind::Roots);
    UnchangedFailure(sim, [&] { return sim.Plant(berryId, berries, CropKind::Berries); });
    UnchangedFailure(sim, [&] { return sim.Plant(berryId, berries, CropKind::Count); });
    UnchangedFailure(sim, [&] { return sim.Plant(berryId, berries, static_cast<CropKind>(-1)); });
    GatherUntil(sim, Item::Berries, ResourceKind::BerryBush, 1);
    const int fruit = sim.Count(Item::Berries);
    const int seeds = sim.Count(Item::Seeds);
    UnchangedFailure(sim, [&] { return sim.Plant(berryId, {3900, 3900}, CropKind::Berries); });
    OK(sim.Plant(berryId, berries, CropKind::Berries));
    CHECK(sim.Count(Item::Berries) == fruit - 1);
    CHECK(sim.Count(Item::Seeds) == seeds);
    CHECK(sim.GetState().plots[0].kind == CropKind::Berries && sim.GetState().plots[0].planted);
    CHECK(sim.GetState().plots[0].growth == 0);
    UnchangedFailure(sim, [&] { return sim.Plant(berryId, berries); });
    UnchangedFailure(sim, [&] { return sim.Plant(berryId, berries, CropKind::Berries); });
    UnchangedFailure(sim, [&] { return sim.HarvestCrop(berryId, berries); });
    OK(sim.Plant(rootId, roots));
    CHECK(sim.Count(Item::Seeds) == seeds - 1);
    CHECK(sim.GetState().plots[1].kind == CropKind::Roots);
    OK(sim.FillWater(WaterSource));
    OK(sim.Water(berryId, berries));
    OK(sim.Water(rootId, roots));
    const std::string paused = sim.Serialize();
    sim.Advance(3600, Home, true);
    CHECK(sim.Serialize() == paused);
    Simulation dry, weedy, rain;
    OK(dry.Deserialize(paused));
    OK(weedy.Deserialize(paused));
    OK(rain.Deserialize(paused));
    Edit(dry, [](State& state) { state.plots[0].moisture = 0; });
    Edit(weedy, [](State& state) { state.plots[0].weeds = 1; });
    sim.AdvanceGameHours(4, Home);
    dry.AdvanceGameHours(4, Home);
    weedy.AdvanceGameHours(4, Home);
    CHECK(sim.GetState().plots[0].growth < sim.GetState().plots[1].growth);
    CHECK(Close(sim.GetState().plots[1].growth / sim.GetState().plots[0].growth, 42.0 / 30.0));
    CHECK(dry.GetState().plots[0].growth > 0);
    CHECK(dry.GetState().plots[0].growth < sim.GetState().plots[0].growth);
    CHECK(weedy.GetState().plots[0].growth < sim.GetState().plots[0].growth);
    CHECK(dry.GetState().plots[0].planted && weedy.GetState().plots[0].planted);
    Edit(rain, [](State& state) { state.hour = 33; state.plots[0].moisture = 0; });
    rain.AdvanceGameHours(2, Home);
    CHECK(rain.GetState().plots[0].moisture > 0.59);
    CHECK(rain.GetState().plots[0].growth > 0);
    CHECK(rain.GetState().plots[0].kind == CropKind::Berries);

    GrowBerryBush(sim, berryId, berries);
    const Inventory originalStock = sim.GetState().inventory;
    Stock(sim, {{Item::Stone, 115}});
    UnchangedFailure(sim, [&] { return sim.HarvestCrop(berryId, berries); });
    CHECK(sim.GetState().plots[0].growth == 1 && sim.GetState().plots[0].planted);
    Edit(sim, [&](State& state) { state.inventory = originalStock; });
    const int berriesBefore = sim.Count(Item::Berries);
    const int seedsBefore = sim.Count(Item::Seeds);
    const int rootsBefore = sim.Count(Item::Roots);
    const double moistureBefore = sim.GetState().plots[0].moisture;
    const double weedsBefore = sim.GetState().plots[0].weeds;
    OK(sim.HarvestCrop(berryId, berries));
    CHECK(sim.Count(Item::Berries) == berriesBefore + 6);
    CHECK(sim.Count(Item::Seeds) == seedsBefore && sim.Count(Item::Roots) == rootsBefore);
    CHECK(sim.GetState().plots[0].growth == 0 && sim.GetState().plots[0].planted);
    CHECK(sim.GetState().plots[0].kind == CropKind::Berries);
    CHECK(sim.GetState().plots[0].moisture == moistureBefore);
    CHECK(sim.GetState().plots[0].weeds == weedsBefore);
    UnchangedFailure(sim, [&] { return sim.HarvestCrop(berryId, berries); });
    UnchangedFailure(sim, [&] { return sim.Plant(berryId, berries, CropKind::Roots); });
    Simulation regrowing;
    OK(regrowing.Deserialize(sim.Serialize()));
    CHECK(regrowing.Serialize() == sim.Serialize());
    const int regrowthSeeds = regrowing.Count(Item::Seeds);
    GrowBerryBush(regrowing, berryId, berries);
    CHECK(regrowing.Count(Item::Seeds) == regrowthSeeds);
    Stock(regrowing, {{Item::Stone, 114}});
    OK(regrowing.HarvestCrop(berryId, berries));
    CHECK(regrowing.UsedCapacity() == 120);
    CHECK(regrowing.Count(Item::Berries) == 6);
    CHECK(regrowing.Count(Item::Roots) == 0 && regrowing.Count(Item::Seeds) == 0);
    CHECK(regrowing.GetState().plots[0].planted && regrowing.GetState().plots[0].growth == 0);
    const double hungerBefore = regrowing.GetState().hunger;
    OK(regrowing.Eat(Item::Berries));
    CHECK(regrowing.GetState().hunger > hungerBefore);
}

void CropKindPersistenceAndVersionRejection()
{
    Simulation sim;
    BuildingStock(sim);
    BuildRoom(sim);
    OK(sim.Place(Piece::Fire, -3, -1, 0, CellCenter(-3, -1)));
    OK(sim.Place(Piece::Chest, -4, 0, 0, CellCenter(-4, 0)));
    Stock(sim, {{Item::Knife, 1}, {Item::DiggingStick, 1}, {Item::WateringCan, 1},
        {Item::Seeds, 3}, {Item::Roots, 2}, {Item::Berries, 2}, {Item::Branch, 3}});
    const Point garden = CellCenter(-2, -1);
    OK(sim.Till(CellToGarden(-2), CellToGarden(-1), garden));
    OK(sim.Till(CellToGarden(-1), CellToGarden(-1), CellCenter(-1, -1)));
    const int id = sim.FindNearestPlot(garden, 1);
    OK(sim.Plant(id, garden));
    OK(sim.FillWater(WaterSource));
    OK(sim.Water(id, garden));
    OK(sim.AddFuel(StructureId(sim, Piece::Fire, CellCenter(-3, -1)), CellCenter(-3, -1)));
    OK(sim.Transfer(StructureId(sim, Piece::Chest, CellCenter(-4, 0)), Item::Roots, 1, CellCenter(-4, 0)));
    auto branch = Node(sim, ResourceKind::Branches);
    OK(sim.Harvest(branch.id, branch.position));
    auto berry = Node(sim, ResourceKind::BerryBush);
    OK(sim.Clear(berry.id, berry.position));
    sim.AdvanceGameHours(2, Home);
    const std::string expected = sim.Serialize();
    CHECK(expected.rfind("HOMESTEAD " + std::to_string(SimulationSaveVersion) + " ", 0) == 0);
    const std::string legacy = Encode(sim.GetState(), 2);
    Simulation migrated;
    const auto initial = migrated.Serialize();
    CHECK(migrated.Deserialize(legacy).code == ResultCode::UnsupportedVersion);
    CHECK(migrated.Serialize() == initial);
    CHECK(migrated.Deserialize(Encode(sim.GetState(), 3)).code == ResultCode::UnsupportedVersion);
    CHECK(migrated.Serialize() == initial);
    CHECK(migrated.Deserialize(Encode(sim.GetState(), 5)).code == ResultCode::UnsupportedVersion);
    CHECK(migrated.Deserialize(Encode(sim.GetState(), 6)).code == ResultCode::UnsupportedVersion);
    CHECK(migrated.Serialize() == initial);
    OK(migrated.Deserialize(expected));
    CHECK(migrated.Serialize() == expected);
    CHECK(migrated.GetState().plots[0].kind == CropKind::Roots);
    CHECK(migrated.GetState().plots[1].kind == CropKind::Roots);
    CHECK(migrated.GetState().plots[0].growth == sim.GetState().plots[0].growth);
    CHECK(migrated.GetState().plots[0].moisture == sim.GetState().plots[0].moisture);
    CHECK(migrated.GetState().plots[0].weeds == sim.GetState().plots[0].weeds);
    CHECK(migrated.IsSheltered(Home));
    CHECK(migrated.IsNearFire(Home));
    CHECK(!migrated.CanHarvest(branch.id) && !migrated.CanHarvest(berry.id));
    OK(migrated.Deserialize(migrated.Serialize()));
    CHECK(migrated.Serialize() == expected);
    OK(migrated.Plant(migrated.GetState().plots[1].id, CellCenter(-1, -1), CropKind::Berries));
    migrated.AdvanceGameHours(1, Home);
    const std::string mixed = migrated.Serialize();
    Simulation restored;
    OK(restored.Deserialize(mixed));
    CHECK(restored.Serialize() == mixed);
    CHECK(restored.GetState().plots[0].kind == CropKind::Roots);
    CHECK(restored.GetState().plots[1].kind == CropKind::Berries);
    CHECK(restored.GetState().plots[1].growth > 0);

    const auto reject = [&](const std::string& data) {
        CHECK(!restored.Deserialize(data));
        CHECK(restored.Serialize() == mixed);
    };
    for (int invalid : {-1, 2, 999999, std::numeric_limits<int>::max()})
    {
        State state = restored.GetState();
        state.plots[1].kind = static_cast<CropKind>(invalid);
        reject(Encode(state));
    }
    State impossible = restored.GetState();
    impossible.plots[1].planted = false;
    impossible.plots[1].growth = 0;
    reject(Encode(impossible));
    const std::string payload = mixed.substr(mixed.find('\n') + 1);
    reject(Envelope(payload, 2));
    reject(Envelope(payload, 1));
    reject(Envelope(payload, SimulationSaveVersion + 1));
    State malformedLegacy = sim.GetState();
    malformedLegacy.plots[0].growth = 1.1;
    reject(Encode(malformedLegacy, 2));
    reject(legacy.substr(0, legacy.size() - 1));

    Edit(sim, [](State& state) { state.plots[0].growth = 1; });
    OK(restored.Deserialize(Encode(sim.GetState())));
    const int rootsBefore = restored.Count(Item::Roots), seedsBefore = restored.Count(Item::Seeds);
    OK(restored.HarvestCrop(id, garden));
    CHECK(restored.Count(Item::Roots) == rootsBefore + 4);
    CHECK(restored.Count(Item::Seeds) == seedsBefore + 2);
    CHECK(!restored.GetState().plots[0].planted && restored.GetState().plots[0].kind == CropKind::Roots);
    OK(restored.Plant(id, garden));
    CHECK(restored.GetState().plots[0].kind == CropKind::Roots);
}

void ClockPauseAndBatching()
{
    Simulation sim;
    const auto initial = sim.Serialize();
    sim.Advance(3600, Home, true);
    CHECK(sim.Serialize() == initial);
    sim.Advance(150, Home);
    CHECK(Close(sim.GetState().hour, 7));
    OK(sim.SetDayMinutes(30));
    sim.Advance(150, Home);
    CHECK(Close(sim.GetState().hour, 9));
    Simulation large, small;
    BuildingStock(large);
    BuildRoom(large);
    OK(large.Place(Piece::Fire, -3, -1, 0, CellCenter(-3, -1)));
    OK(large.AddFuel(large.GetState().structures.back().id, CellCenter(-3, -1)));
    Stock(large, {{Item::DiggingStick, 1}, {Item::Seeds, 2}});
    OK(large.Till(CellToGarden(-2), CellToGarden(-1), CellCenter(-2, -1)));
    OK(large.Plant(large.GetState().plots[0].id, CellCenter(-2, -1)));
    Edit(large, [](State& state) { state.hour = 31.25; });
    OK(small.Deserialize(large.Serialize()));
    const auto paused = large.Serialize();
    large.Advance(900, Home, true);
    CHECK(paused == large.Serialize());
    large.AdvanceGameHours(9.75, Home);
    for (int i = 0; i < 975; ++i) small.AdvanceGameHours(0.01, Home);
    CHECK(Close(large.GetState().hour, small.GetState().hour, 1e-6));
    CHECK(Close(large.GetState().hunger, small.GetState().hunger, 1e-6));
    CHECK(Close(large.GetState().energy, small.GetState().energy, 1e-6));
    CHECK(Close(large.GetState().plots[0].growth, small.GetState().plots[0].growth, 0.001));
    CHECK(Close(large.GetState().plots[0].moisture, small.GetState().plots[0].moisture, 0.001));
    CHECK(Close(large.GetState().plots[0].weeds, small.GetState().plots[0].weeds, 0.001));
    CHECK(Close(large.GetState().structures.back().fuelHours, small.GetState().structures.back().fuelHours));
    Edit(sim, [](State& state) { state.hour = 14 * 24; });
    CHECK(sim.DayNumber() == 15);
    CHECK(std::string(sim.SeasonName()) == "Summer");
    Edit(sim, [](State& state) { state.hour = 28 * 24; });
    CHECK(std::string(sim.SeasonName()) == "Autumn");
    Edit(sim, [](State& state) { state.hour = 42 * 24; });
    CHECK(std::string(sim.SeasonName()) == "Winter");
    Edit(sim, [](State& state) { state.hour = 56 * 24; });
    CHECK(std::string(sim.SeasonName()) == "Spring");
}

void CosmeticClothingAndRetiredFur()
{
    Simulation sim;
    // Deer remains are retired: they're never gatherable and yield no fur.
    for (const auto& node : sim.GetState().resources)
        if (node.kind == ResourceKind::DeerRemains)
        {
            CHECK(!sim.CanHarvest(node.id));
            UnchangedFailure(sim, [&] { return sim.Harvest(node.id, node.position); });
        }
    // Reeds (the fibre source) are retired the same way.
    for (const auto& node : sim.GetState().resources)
        if (node.kind == ResourceKind::Reeds)
        {
            CHECK(!sim.CanHarvest(node.id));
            UnchangedFailure(sim, [&] { return sim.Harvest(node.id, node.position); });
        }

    // Garments carried over in older saves still wear, but only for looks.
    Stock(sim, {{Item::Knife, 1}, {Item::Fiber, 60}, {Item::Fur, 20}});
    for (auto definition : {WearableDefinition::FurCoat, WearableDefinition::FurBoots,
        WearableDefinition::Trousers, WearableDefinition::LinenLongShirt})
        OK(sim.CraftGarment(definition, Home, sim.GetRevision()));
    CHECK(sim.CraftGarment(WearableDefinition::LeatherShoes, Home, sim.GetRevision()).ok == false);

    // Trousers take the legs from the starter tunic, which also frees its apron.
    int coat = 0, trousers = 0, shirt = 0, boots = 0;
    for (const auto& item : sim.GetState().wearables)
    {
        if (item.definition == WearableDefinition::FurCoat) coat = item.id;
        if (item.definition == WearableDefinition::Trousers) trousers = item.id;
        if (item.definition == WearableDefinition::LinenLongShirt) shirt = item.id;
        if (item.definition == WearableDefinition::FurBoots) boots = item.id;
    }
    OK(sim.EquipWearable(trousers, sim.GetRevision()));
    CHECK(sim.GetWearable(1)->owner == WearableOwner::Carried);
    for (int id : {coat, shirt, boots}) OK(sim.EquipWearable(id, sim.GetRevision()));
    CHECK(sim.GetState().equipment == (std::array<int, 5>{shirt, trousers, 0, boots, coat}));

    // Current saves keep the outer layer; older ones load with none.
    Simulation restored;
    OK(restored.Deserialize(sim.Serialize()));
    CHECK(restored.GetState().equipment == sim.GetState().equipment && restored.Count(Item::Fur) == sim.Count(Item::Fur));
    Simulation older;
    OK(older.Deserialize(Encode(Simulation().GetState(), FreeBuildingSaveVersion)));
    CHECK(older.Count(Item::Fur) == 0 && older.GetState().equipment[static_cast<int>(EquipmentSlot::Outer)] == 0);

    // A winter night costs the dressed and the bare exactly the same: there is no cold.
    Simulation bare;
    Edit(bare, [](State& state) { state.hour = 42 * 24 + 20; state.energy = 100; state.hunger = 80; });
    Edit(sim, [](State& state) { state.hour = 42 * 24 + 20; state.energy = 100; state.hunger = 80; });
    CHECK(std::string(bare.SeasonName()) == "Winter");
    bare.AdvanceGameHours(2, Home);
    sim.AdvanceGameHours(2, Home);
    CHECK(Close(bare.GetState().hunger, sim.GetState().hunger) && Close(bare.GetState().energy, sim.GetState().energy));
    CHECK(Close(bare.GetState().hunger, 80 - 2 * 2) && Close(bare.GetState().energy, 100 - 2 * Exertion::AwakePerHour));
    CHECK(!bare.GetState().failed && !sim.GetState().failed);
}
void SleepAndFailure()
{
    Simulation bare, indoor, fire;
    Edit(bare, [](State& state) { state.hour = 19; });
    OK(indoor.Deserialize(bare.Serialize()));
    BuildingStock(indoor);
    BuildRoom(indoor);
    OK(fire.Deserialize(bare.Serialize()));
    BuildingStock(fire);
    OK(fire.Place(Piece::Fire, -3, 0, 0, Home));
    OK(fire.AddFuel(fire.GetState().structures[0].id, Home));
    const double energies[] = {bare.GetState().energy, indoor.GetState().energy, fire.GetState().energy};
    bare.AdvanceGameHours(3, Home);
    indoor.AdvanceGameHours(3, Home);
    fire.AdvanceGameHours(3, Home);
    // Shelter and fire no longer change her vitals; a night outdoors is merely a night.
    CHECK(Close(bare.GetState().hunger, indoor.GetState().hunger) && Close(bare.GetState().hunger, fire.GetState().hunger));
    CHECK(Close(energies[0] - bare.GetState().energy, 3 * Exertion::AwakePerHour));
    CHECK(Close(energies[1] - indoor.GetState().energy, 3 * Exertion::AwakePerHour));
    CHECK(Close(energies[2] - fire.GetState().energy, 3 * Exertion::AwakePerHour));
    UnchangedFailure(bare, [&] { return bare.Sleep(8, Home); });
    BuildingStock(bare);
    OK(bare.Place(Piece::Bed, -3, 0, 0, Home));
    UnchangedFailure(bare, [&] { return bare.Sleep(13, Home); });
    UnchangedFailure(bare, [&] { return bare.Sleep(-1, Home); });
    UnchangedFailure(bare, [&] { return bare.Sleep(std::numeric_limits<double>::quiet_NaN(), Home); });
    UnchangedFailure(bare, [&] { return bare.Sleep(8, {3900, 3900}); });
    Edit(bare, [](State& state) { state.hour = 19; state.hunger = 0.65; state.energy = 10; });
    const auto checkpoint = bare.Serialize();
    CHECK(!bare.Sleep(8, Home).ok);
    CHECK(bare.GetState().failed);
    CHECK(Close(bare.GetState().hour, 19.5));
    CHECK(Close(bare.GetState().energy, 15));
    CHECK(bare.GetState().hunger == 0);
    const auto failed = bare.Serialize();
    bare.AdvanceGameHours(12, Home);
    CHECK(failed == bare.Serialize());
    UnchangedFailure(bare, [&] { return bare.Eat(Item::Berries); });
    UnchangedFailure(bare, [&] { return bare.SetDayMinutes(30); });
    UnchangedFailure(bare, [&] { return bare.Sleep(1, Home); });
    UnchangedFailure(bare, [&] { return bare.Craft(Recipe::HaftAxe, Home); });
    Simulation savedFailure;
    OK(savedFailure.Deserialize(failed));
    CHECK(savedFailure.GetState().failed);
    OK(bare.Deserialize(checkpoint));
    CHECK(!bare.GetState().failed);
    // A cold night in bed is simply rest now.
    Edit(bare, [](State& state) { state.hour = 42 * 24 + 20; state.hunger = 80; state.energy = 60; });
    OK(bare.Sleep(8, Home));
    CHECK(!bare.GetState().failed && bare.GetState().energy == 100);
    Simulation tired;
    Edit(tired, [](State& state) { state.energy = 1.2; });
    tired.AdvanceGameHours(10, Home);
    CHECK(tired.GetState().failed);
    CHECK(tired.GetState().energy == 0);
    // Time awake drains Energy slowly: 1.2 points last two hours.
    CHECK(Close(tired.GetState().hour, 8.0));
}

void SleepAndFiniteBoundaries()
{
    Simulation once;
    BuildingStock(once);
    BuildRoom(once);
    OK(once.Place(Piece::Bed, -3, 0, 0, Home));
    OK(once.Place(Piece::Fire, -3, -1, 0, CellCenter(-3, -1)));
    OK(once.AddFuel(once.GetState().structures.back().id, CellCenter(-3, -1)));
    Stock(once, {{Item::DiggingStick, 1}, {Item::Seeds, 1}});
    OK(once.Till(CellToGarden(-2), CellToGarden(-1), CellCenter(-2, -1)));
    OK(once.Plant(once.GetState().plots[0].id, CellCenter(-2, -1)));
    CHECK(once.Count(Item::Seeds) == 0);
    Edit(once, [](State& state) { state.hour = 31; state.energy = 15; });
    Simulation split;
    OK(split.Deserialize(once.Serialize()));
    OK(once.Sleep(8, Home));
    for (int i = 0; i < 16; ++i) OK(split.Sleep(0.5, Home));
    CHECK(Close(once.GetState().hour, split.GetState().hour, 1e-7));
    CHECK(Close(once.GetState().hunger, split.GetState().hunger, 1e-7));
    CHECK(Close(once.GetState().energy, split.GetState().energy, 1e-7));
    CHECK(Close(once.GetState().plots[0].moisture, split.GetState().plots[0].moisture, 0.001));
    CHECK(Close(once.GetState().plots[0].growth, split.GetState().plots[0].growth, 0.001));
    CHECK(once.GetState().structures.back().fuelHours == 0);
    CHECK(split.GetState().structures.back().fuelHours == 0);
    CHECK(once.GetState().plots[0].growth > 0);
    Edit(once, [](State& state) { state.hour = 1000000; });
    const std::string before = once.Serialize();
    once.AdvanceGameHours(1, Home);
    CHECK(once.Serialize() == before);
    UnchangedFailure(once, [&] { return once.Sleep(1, Home); });
    Edit(once, [](State& state) { state.nextId = TransientResourceIdBase - 1; });
    UnchangedFailure(once, [&] { return once.Till(CellToGarden(-1), CellToGarden(-1), CellCenter(-1, -1)); });
    UnchangedFailure(once, [&] { return once.Place(Piece::Foundation, -1, -1, 0, CellCenter(-1, -1)); });
    Simulation restored;
    OK(restored.Deserialize(once.Serialize()));
}

void PersistenceRejection()
{
    Simulation sim;
    BuildingStock(sim);
    BuildRoom(sim);
    OK(sim.Place(Piece::Chest, -4, 0, 0, CellCenter(-4, 0)));
    Stock(sim, {{Item::DiggingStick, 1}, {Item::Seeds, 2}});
    OK(sim.Till(CellToGarden(-2), CellToGarden(-1), CellCenter(-2, -1)));
    OK(sim.Plant(sim.GetState().plots[0].id, CellCenter(-2, -1)));
    const std::string original = sim.Serialize();
    const std::string payload = original.substr(original.find('\n') + 1);
    const auto reject = [&](const std::string& data) {
        CHECK(!sim.Deserialize(data).ok);
        CHECK(sim.Serialize() == original);
    };
    reject("");
    reject("HOMESTEAD 1\n");
    reject(original.substr(0, original.size() - 1));
    reject(original + "garbage");
    reject(Envelope(payload, 1));
    reject(Envelope(payload, SimulationSaveVersion + 1));
    reject(Envelope(payload + "garbage"));
    reject(Envelope(payload.substr(0, payload.size() - 8)));
    reject(std::string(8 * 1024 * 1024 + 1, 'x'));
    std::string corrupt = original;
    corrupt.back() = 'x';
    reject(corrupt);
    std::vector<std::function<void(State&)>> edits = {
        [](State& s) { s.hour = -1; },
        [](State& s) { s.hour = 1000001; },
        [](State& s) { s.hour = std::numeric_limits<double>::quiet_NaN(); },
        [](State& s) { s.dayMinutes = 0; },
        [](State& s) { s.energy = 101; },
        [](State& s) { s.hunger = 0; },
        [](State& s) { s.failed = true; },
        [](State& s) { s.energy = std::numeric_limits<double>::infinity(); },
        [](State& s) { s.inventory[0] = -1; },
        [](State& s) { s.inventory[0] = 121; },
        [](State& s) { s.inventory[0] = 100; s.inventory[1] = 100; },
        [](State& s) { s.nextId = 0; },
        [](State& s) { s.nextId = 1; },
        [](State& s) { s.resourceEdits.push_back(s.resourceEdits[0]); },
        [](State& s) { s.resourceEdits[0].key.localId = 0; },
        [](State& s) { s.resourceEdits[0].key.localId = 0xffffffffu; },
        [](State& s) { s.resourceEdits[0].key.chunk.x = 999999; },
        [](State& s) { s.resourceEdits[0].readyAtHour = -1; },
        [](State& s) { s.resourceEdits[0].readyAtHour = 1; },
        [](State& s) { s.resourceEdits[0].readyAtHour = s.hour + 10000; },
        [](State& s) { s.resourceEdits[0].cleared = true; s.resourceEdits[0].readyAtHour = 20; },
        [](State& s) { s.structures[0].id = TransientResourceIdBase; },
        [](State& s) { s.structures[0].kind = static_cast<Piece>(99); },
        [](State& s) { s.structures[0].cellX = -3334; },
        [](State& s) { s.structures[0].rotation = 4; },
        [](State& s) { s.structures[0].rotation = -1; },
        [](State& s) { s.structures[0].fuelHours = 1; },
        [](State& s) { s.structures[0].storage[0] = 1; },
        [](State& s) { s.structures.back().storage[0] = -1; },
        [](State& s) { s.structures.back().storage[0] = 121; },
        [](State& s) { s.structures.back().kind = Piece::Fire; s.structures.back().fuelHours = 49; },
        [](State& s) { s.structures[0].cellX = 0; },
        [](State& s) { auto p = s.structures[0]; p.id = s.nextId++; s.structures.push_back(p); },
        [](State& s) { auto p = s.structures[2]; p.id = s.nextId++; s.structures.push_back(p); },
        [](State& s) { s.plots[0].id = TransientResourceIdBase; },
        [](State& s) { s.plots[0].cellX = 3334 * GardenCellsPerCell; },
        [](State& s) { s.plots[0].cellX = CellToGarden(-3); s.plots[0].cellY = CellToGarden(0); },
        [](State& s) { s.plots[0].growth = -0.01; },
        [](State& s) { s.plots[0].growth = 1.01; },
        [](State& s) { s.plots[0].moisture = -0.01; },
        [](State& s) { s.plots[0].weeds = 1.01; },
        [](State& s) { s.plots[0].kind = static_cast<CropKind>(-1); },
        [](State& s) { s.plots[0].kind = CropKind::Count; },
        [](State& s) { s.plots[0].planted = false; s.plots[0].growth = 0.5; },
        [](State& s) { auto p = s.plots[0]; p.id = s.nextId++; s.plots.push_back(p); },
        [](State& s) { s.resourceEdits.resize(MaxResourceEdits + 1); }
    };
    for (const auto& edit : edits)
    {
        State state = sim.GetState();
        edit(state);
        reject(Encode(state));
    }
    // Valid checksums must not bypass strict field/count/bool/trailing-field checks.
    std::string body = payload;
    body.replace(0, body.find('\n'), "6 60 85 100 90 2 0 200");
    reject(Envelope(body));
    body = payload;
    const auto inventoryEnd = body.find('\n', body.find('\n') + 1);
    const auto countEnd = body.find('\n', inventoryEnd + 1);
    body.replace(inventoryEnd + 1, countEnd - inventoryEnd - 1, "-1");
    reject(Envelope(body));
    body = payload;
    body.replace(inventoryEnd + 1, countEnd - inventoryEnd - 1, "999999999999999999999999");
    reject(Envelope(body));
    OK(sim.Deserialize(original));
    CHECK(sim.Serialize() == original);
}

int Group(const Simulation& sim, Item item, int container = 0)
{
    const auto* layout = sim.GetLayout(container);
    CHECK(layout);
    for (const auto& entry : *layout)
        if (entry.wearableId == 0 && entry.item == item) return entry.groupId;
    CHECK(false);
    return 0;
}
void InventoryRoundTrip(const Simulation& sim)
{
    Simulation loaded;
    OK(loaded.Deserialize(sim.Serialize()));
    CHECK(loaded.Serialize() == sim.Serialize());
    CHECK(loaded.UsedCapacity() <= InventoryCapacity);
    for (const auto& piece : loaded.GetState().structures)
        if (piece.kind == Piece::Chest) CHECK(loaded.ChestUsedCapacity(piece.id) <= ChestCapacity);
}
void WardrobeDefaultsAndCrafting()
{
    static_assert(static_cast<int>(Item::HerbedRoots) == 13, "Existing fungible save IDs are unchanged");
    Simulation sim;
    CHECK(sim.GetState().wearables.size() == 1);
    CHECK(sim.GetState().equipment == (std::array<int, 5>{1, 1, 0, 0, 0}));
    CHECK(sim.GetWearable(1)->definition == WearableDefinition::LinenTunic);
    CHECK(sim.GetWearable(2) == nullptr);
    CHECK(sim.GetState().nextWearableId == 2);
    CHECK(sim.GetWearable(0) == nullptr);
    CHECK(sim.GetLayout(-1) == nullptr);
    CHECK(sim.ChestUsedCapacity(0) == -1);
    CHECK(std::string(DyeName(0)) == "Moss");
    CHECK(std::string(DyeName(3)) == "Flax");
    CHECK(std::string(DyeName(4)) == "Unknown dye");
    CHECK(GetWearableDefinition(WearableDefinition::Count) == nullptr);
    const int worldId = sim.GetState().nextId;
    for (auto definition : {WearableDefinition::LinenTunic, WearableDefinition::LinenApron, WearableDefinition::WovenFootwraps})
    {
        const int cost = GetWearableDefinition(definition)->fiberCost;
        Stock(sim, {{Item::Knife, 1}, {Item::Fiber, cost - 1}});
        UnchangedFailure(sim, [&] { return sim.CraftGarment(definition, Home, sim.GetRevision()); });
        Stock(sim, {{Item::Fiber, cost}});
        UnchangedFailure(sim, [&] { return sim.CraftGarment(definition, Home, sim.GetRevision()); });
        Stock(sim, {{Item::Knife, 1}, {Item::Fiber, 115}});
        const int beforeCapacity = sim.UsedCapacity();
        const int beforeCount = static_cast<int>(sim.GetState().wearables.size());
        const auto hour = sim.GetState().hour;
        const int expectedId = sim.GetState().nextWearableId;
        const auto revision = sim.GetRevision();
        const auto result = sim.CraftGarment(definition, Home, revision);
        OK(result);
        CHECK(result.revision == sim.GetRevision() && result.revision > revision);
        CHECK(sim.Count(Item::Fiber) == 115 - cost);
        CHECK(sim.UsedCapacity() == beforeCapacity - cost + 1);
        CHECK(static_cast<int>(sim.GetState().wearables.size()) == beforeCount + 1);
        CHECK(sim.GetWearable(expectedId)->definition == definition);
        CHECK(sim.GetWearable(expectedId)->owner == WearableOwner::Carried);
        CHECK(sim.GetState().nextId == worldId);
        CHECK(sim.GetState().hour == hour);
        CHECK(std::string(GarmentRequirements(definition)).rfind(std::to_string(cost) + " Fiber; knife required", 0) == 0);
        UnchangedFailure(sim, [&] { return sim.CraftGarment(definition, Home, revision); });
        InventoryRoundTrip(sim);
    }
    UnchangedFailure(sim, [&] { return sim.CraftGarment(WearableDefinition::LeatherShoes, Home, sim.GetRevision()); });
    UnchangedFailure(sim, [&] { return sim.CraftGarment(WearableDefinition::Count, Home, sim.GetRevision()); });
    UnchangedFailure(sim, [&] { return sim.CraftGarment(WearableDefinition::LinenTunic, {1000001, 0}, sim.GetRevision()); });
    Stock(sim, {{Item::Knife, 1}, {Item::Fiber, 116}});
    CHECK(sim.UsedCapacity() == 120);
    OK(sim.CraftGarment(WearableDefinition::LinenTunic, Home, sim.GetRevision()));
    CHECK(sim.UsedCapacity() == 109);
    InventoryRoundTrip(sim);
}
void AtomicEquipmentAndDye()
{
    Simulation sim;
    Shod(sim);
    Stock(sim, {{Item::Knife, 1}, {Item::Fiber, 40}});
    OK(sim.CraftGarment(WearableDefinition::LinenApron, Home, sim.GetRevision()));
    const int apron = sim.GetState().wearables.back().id;
    OK(sim.EquipWearable(apron, sim.GetRevision()));
    OK(sim.CraftGarment(WearableDefinition::LinenTunic, Home, sim.GetRevision()));
    const int tunic = sim.GetState().wearables.back().id;
    CHECK(tunic != 1);
    OK(sim.RecolorWearable(tunic, 1, Home, sim.GetRevision()));
    CHECK(sim.GetWearable(1)->dye == 0 && sim.GetWearable(tunic)->dye == 1);
    Stock(sim, {{Item::Knife, 1}, {Item::Branch, 118}});
    CHECK(sim.UsedCapacity() == 120);
    const auto revision = sim.GetRevision();
    OK(sim.EquipWearable(tunic, revision));
    CHECK(sim.UsedCapacity() == 120);
    CHECK(sim.GetState().equipment == (std::array<int, 5>{tunic, tunic, apron, 2, 0}));
    CHECK(sim.GetWearable(1)->owner == WearableOwner::Carried);
    UnchangedFailure(sim, [&] { return sim.EquipWearable(tunic, revision); });
    UnchangedFailure(sim, [&] { return sim.UnequipWearable(2, sim.GetRevision()); });
    Stock(sim, {{Item::Knife, 1}, {Item::Branch, 117}});
    CHECK(sim.UsedCapacity() == 119);
    UnchangedFailure(sim, [&] { return sim.UnequipWearable(tunic, sim.GetRevision()); });
    Stock(sim, {{Item::Knife, 1}, {Item::Branch, 116}});
    OK(sim.UnequipWearable(tunic, sim.GetRevision()));
    CHECK(sim.UsedCapacity() == 120);
    CHECK(sim.GetState().equipment == (std::array<int, 5>{0, 0, 0, 2, 0}));
    CHECK(sim.GetWearable(apron)->owner == WearableOwner::Carried);
    UnchangedFailure(sim, [&] { return sim.EquipWearable(apron, sim.GetRevision()); });
    OK(sim.EquipWearable(1, sim.GetRevision()));
    OK(sim.EquipWearable(apron, sim.GetRevision()));
    UnchangedFailure(sim, [&] { return sim.EquipWearable(1, sim.GetRevision()); });
    UnchangedFailure(sim, [&] { return sim.UnequipWearable(tunic, sim.GetRevision()); });
    UnchangedFailure(sim, [&] { return sim.EquipWearable(999999, sim.GetRevision()); });
    UnchangedFailure(sim, [&] { return sim.RecolorWearable(2, 1, Home, sim.GetRevision()); });
    UnchangedFailure(sim, [&] { return sim.RecolorWearable(1, 4, Home, sim.GetRevision()); });
    UnchangedFailure(sim, [&] { return sim.RecolorWearable(1, -1, Home, sim.GetRevision()); });
    UnchangedFailure(sim, [&] { return sim.RecolorWearable(1, 0, Home, sim.GetRevision()); });
    const auto before = sim.GetState();
    OK(sim.RecolorWearable(1, 3, Home, sim.GetRevision()));
    CHECK(sim.GetState().hunger == before.hunger && sim.GetState().energy == before.energy && sim.GetState().hour == before.hour);
    CHECK(sim.GetWearable(tunic)->dye == 1);
    InventoryRoundTrip(sim);
}
void WardrobeStorageAndReach()
{
    Simulation sim;
    Shod(sim);
    BuildingStock(sim);
    OK(sim.Place(Piece::Chest, -3, 0, 0, Home));
    const int chest = sim.GetState().structures.back().id;
    Stock(sim, {{Item::Knife, 1}, {Item::Fiber, 12}});
    OK(sim.CraftGarment(WearableDefinition::LinenTunic, Home, sim.GetRevision()));
    const int tunic = sim.GetState().wearables.back().id;
    OK(sim.RecolorWearable(tunic, 2, Home, sim.GetRevision()));
    UnchangedFailure(sim, [&] { return sim.MoveWearable(1, chest, Home, sim.GetRevision()); });
    UnchangedFailure(sim, [&] { return sim.MoveWearable(tunic, 0, Home, sim.GetRevision()); });
    UnchangedFailure(sim, [&] { return sim.MoveWearable(tunic, 99999, Home, sim.GetRevision()); });
    const Point outside{Home.x + 281, Home.y};
    const Point edge{Home.x + 280, Home.y};
    UnchangedFailure(sim, [&] { return sim.MoveWearable(tunic, chest, outside, sim.GetRevision()); });
    UnchangedFailure(sim, [&] { return sim.Transfer(chest, Item::Knife, 1, outside); });
    OK(sim.MoveWearable(tunic, chest, edge, sim.GetRevision()));
    CHECK(sim.ChestUsedCapacity(chest) == 1 && sim.UsedCapacity() == 1);
    CHECK(sim.GetWearable(tunic)->chestId == chest && sim.GetWearable(tunic)->dye == 2);
    UnchangedFailure(sim, [&] { return sim.EquipWearable(tunic, sim.GetRevision()); });
    UnchangedFailure(sim, [&] { return sim.RecolorWearable(tunic, 1, outside, sim.GetRevision()); });
    Stock(sim, {{Item::Knife, 1}, {Item::Branch, 119}});
    OK(sim.Transfer(chest, Item::Branch, 119, edge));
    Edit(sim, [chest](State& state) {
        for (auto& piece : state.structures)
            if (piece.id == chest) piece.storage[static_cast<int>(Item::Branch)] = ChestCapacity - 1;
    });
    CHECK(sim.ChestUsedCapacity(chest) == ChestCapacity);
    UnchangedFailure(sim, [&] { return sim.Transfer(chest, Item::Knife, 1, edge); });
    OK(sim.UnequipWearable(2, sim.GetRevision()));
    UnchangedFailure(sim, [&] { return sim.MoveWearable(2, chest, Home, sim.GetRevision()); });
    Stock(sim, {{Item::Knife, 1}, {Item::Stone, 118}});
    CHECK(sim.UsedCapacity() == 120);
    UnchangedFailure(sim, [&] { return sim.MoveWearable(tunic, 0, edge, sim.GetRevision()); });
    UnchangedFailure(sim, [&] { return sim.Transfer(chest, Item::Branch, -1, edge); });
    OK(sim.EquipWearable(2, sim.GetRevision()));
    OK(sim.MoveWearable(tunic, 0, edge, sim.GetRevision()));
    CHECK(sim.UsedCapacity() == 120 && sim.ChestUsedCapacity(chest) == ChestCapacity - 1);
    CHECK(sim.GetWearable(tunic)->dye == 2);
    InventoryRoundTrip(sim);
}
void PersistentLayoutTransactions()
{
    Simulation sim;
    BuildingStock(sim);
    OK(sim.Place(Piece::Chest, -3, 0, 0, Home));
    const int chest = sim.GetState().structures.back().id;
    Stock(sim, {{Item::Knife, 1}, {Item::Branch, 20}, {Item::Fiber, 20}});
    const int branches = Group(sim, Item::Branch), fibers = Group(sim, Item::Fiber);
    const int used = sim.UsedCapacity();
    OK(sim.SplitGroup(0, branches, 7, Home, sim.GetRevision()));
    const int split = sim.GetState().nextGroupId - 1;
    CHECK(sim.GetLayout(0)->at(1).quantity == 13);
    CHECK(sim.GetLayout(0)->at(2).groupId == split);
    CHECK(sim.GetLayout(0)->at(2).quantity == 7);
    CHECK(sim.UsedCapacity() == used && sim.Count(Item::Branch) == 20);
    const auto revision = sim.GetRevision();
    OK(sim.TransferGroup(chest, split, 3, true, Home, revision));
    CHECK(sim.GetLayout(0)->at(1).quantity == 13 && sim.GetLayout(0)->at(2).quantity == 4);
    CHECK(sim.Count(Item::Branch) == 17 && sim.ChestUsedCapacity(chest) == 3);
    UnchangedFailure(sim, [&] { return sim.TransferGroup(chest, split, 3, true, Home, revision); });
    UnchangedFailure(sim, [&] { return sim.TransferGroup(chest, split, 5, true, Home, sim.GetRevision()); });
    UnchangedFailure(sim, [&] { return sim.TransferGroup(chest, split, 0, true, Home, sim.GetRevision()); });
    UnchangedFailure(sim, [&] { return sim.SplitGroup(0, split, 4, Home, sim.GetRevision()); });
    UnchangedFailure(sim, [&] { return sim.MergeGroups(0, split, fibers, Home, sim.GetRevision()); });
    OK(sim.ReorderEntry(0, 2, 0, Home, sim.GetRevision()));
    CHECK(sim.GetLayout(0)->front().groupId == split);
    OK(sim.Transfer(chest, Item::Branch, 5, Home));
    CHECK(sim.Count(Item::Branch) == 12);
    CHECK(sim.GetLayout(0)->at(1).groupId == branches && sim.GetLayout(0)->at(1).quantity == 12);
    const int chestGroup = Group(sim, Item::Branch, chest);
    const Point outside{Home.x + 281, Home.y};
    UnchangedFailure(sim, [&] { return sim.SplitGroup(chest, chestGroup, 1, outside, sim.GetRevision()); });
    OK(sim.SplitGroup(chest, chestGroup, 2, Home, sim.GetRevision()));
    const int chestSplit = sim.GetState().nextGroupId - 1;
    UnchangedFailure(sim, [&] { return sim.MergeGroups(chest, chestSplit, chestGroup, outside, sim.GetRevision()); });
    UnchangedFailure(sim, [&] { return sim.ReorderEntry(chest, 0, 1, outside, sim.GetRevision()); });
    OK(sim.ReorderEntry(chest, 0, 1, Home, sim.GetRevision()));
    OK(sim.MergeGroups(chest, chestSplit, chestGroup, Home, sim.GetRevision()));
    OK(sim.TransferGroup(chest, chestGroup, 8, false, Home, sim.GetRevision()));
    CHECK(sim.GetLayout(chest)->empty());
    CHECK(sim.Count(Item::Branch) == 20 && sim.UsedCapacity() == used);
    UnchangedFailure(sim, [&] { return sim.TransferGroup(chest, chestGroup, 1, false, Home, sim.GetRevision()); });
    InventoryRoundTrip(sim);
    const auto beforeLoad = sim.GetRevision();
    OK(sim.Deserialize(sim.Serialize()));
    CHECK(sim.GetRevision() > beforeLoad);
    UnchangedFailure(sim, [&] { return sim.SplitGroup(0, branches, 1, Home, beforeLoad); });
    const auto beforeNew = sim.GetRevision();
    sim.NewGame();
    CHECK(sim.GetRevision() > beforeNew);
    UnchangedFailure(sim, [&] { return sim.UnequipWearable(1, beforeNew); });
}
void QuantityMutationReconciliation()
{
    Simulation sim;
    Stock(sim, {{Item::WateringCan, 1}, {Item::RustedHoeBlade, 1}, {Item::Berries, 8}, {Item::BrambleCanes, 20},
        {Item::Branch, 20}, {Item::Stone, 12}});
    int berries = Group(sim, Item::Berries);
    OK(sim.SplitGroup(0, berries, 3, Home, sim.GetRevision()));
    OK(sim.Eat(Item::Berries));
    CHECK(sim.GetLayout(0)->back().quantity > 0);
    InventoryRoundTrip(sim);
    const auto oldRevision = sim.GetRevision();
    auto node = Node(sim, ResourceKind::BerryBush);
    OK(sim.Harvest(node.id, node.position));
    UnchangedFailure(sim, [&] { return sim.SplitGroup(0, berries, 1, Home, oldRevision); });
    InventoryRoundTrip(sim);
    OK(sim.Craft(Recipe::HaftHoe, Home));
    OK(sim.FillWater(WaterSource));
    const Point garden = CellCenter(-2, -1);
    OK(sim.Till(CellToGarden(-2), CellToGarden(-1), garden));
    const int plot = sim.GetState().plots.back().id;
    OK(sim.Plant(plot, garden, CropKind::Berries));
    OK(sim.Water(plot, garden));
    OK(sim.Place(Piece::Fire, -3, 0, 0, Home));
    OK(sim.AddFuel(sim.GetState().structures.back().id, Home));
    InventoryRoundTrip(sim);
    Stock(sim, {{Item::Knife, 1}, {Item::Branch, 119}});
    const int branch = Group(sim, Item::Branch);
    for (int i = 0; i < 118; ++i) OK(sim.SplitGroup(0, branch, 1, Home, sim.GetRevision()));
    CHECK(sim.GetLayout(0)->size() == 120 && sim.UsedCapacity() == 120);
    UnchangedFailure(sim, [&] { return sim.SplitGroup(0, branch, 1, Home, sim.GetRevision()); });
    InventoryRoundTrip(sim);
    while (sim.GetLayout(0)->size() > 2)
    {
        const int source = sim.GetLayout(0)->back().groupId;
        OK(sim.MergeGroups(0, source, branch, Home, sim.GetRevision()));
    }
    CHECK(sim.Count(Item::Branch) == 119 && sim.UsedCapacity() == 120);
    InventoryRoundTrip(sim);
}
void DirectSplitAndDeterministicSort()
{
    Simulation sim;
    Stock(sim, {{Item::Knife, 1}, {Item::Seeds, 2}, {Item::Berries, 4},
        {Item::Fiber, 5}, {Item::Hatchet, 1}, {Item::Stone, 3}});
    OK(sim.UnequipWearable(1, sim.GetRevision()));
    const auto tunic = *sim.GetWearable(1);
    const int fiber = Group(sim, Item::Fiber);
    OK(sim.SplitHalf(0, fiber, Home, sim.GetRevision()));
    CHECK(sim.GetLayout(0)->at(2).groupId == fiber);
    CHECK(sim.GetLayout(0)->at(2).quantity == 3);
    CHECK(sim.GetLayout(0)->at(3).quantity == 2);
    const int split = sim.GetLayout(0)->at(3).groupId;
    CHECK(split != fiber);
    UnchangedFailure(sim, [&] { return sim.SplitHalf(0, Group(sim, Item::Knife), Home, sim.GetRevision()); });
    UnchangedFailure(sim, [&] { return sim.SplitHalf(0, 0, Home, sim.GetRevision()); });
    const auto stale = sim.GetRevision() - 1;
    UnchangedFailure(sim, [&] { return sim.SplitHalf(0, fiber, Home, stale); });

    const int used = sim.UsedCapacity();
    const auto totals = sim.GetState().inventory;
    OK(sim.SortPack(sim.GetRevision()));
    CHECK(sim.UsedCapacity() == used && sim.GetState().inventory == totals);
    CHECK(sim.GetLayout(0)->size() == 7);
    const Item expected[] = {Item::Knife, Item::Hatchet, Item::Stone,
        Item::Fiber, Item::Berries, Item::Seeds};
    for (int index = 0; index < 6; ++index)
        CHECK(sim.GetLayout(0)->at(index).item == expected[index]);
    CHECK(sim.GetLayout(0)->back().wearableId == tunic.id);
    CHECK(sim.GetWearable(tunic.id)->definition == tunic.definition
        && sim.GetWearable(tunic.id)->dye == tunic.dye
        && sim.GetWearable(tunic.id)->owner == WearableOwner::Carried);
    CHECK(sim.GetState().equipment[static_cast<int>(EquipmentSlot::Torso)] == 0);
    CHECK(sim.GetLayout(0)->at(3).groupId == fiber);
    CHECK(sim.GetLayout(0)->at(3).quantity == 5);
    CHECK(std::none_of(sim.GetLayout(0)->begin(), sim.GetLayout(0)->end(),
        [split](const LayoutEntry& entry) { return entry.groupId == split; }));
    const auto sorted = sim.Serialize();
    const auto revision = sim.GetRevision();
    OK(sim.SortPack(revision));
    CHECK(sim.Serialize() == sorted && sim.GetRevision() == revision);
    InventoryRoundTrip(sim);
    CHECK(sim.GetWearable(tunic.id)->definition == tunic.definition
        && sim.GetWearable(tunic.id)->dye == tunic.dye);
}
void PersistentWorldDropTransactions()
{
    for (int index = 0; index < ItemCount; ++index)
    {
        const Item item = static_cast<Item>(index);
        Simulation category;
        Stock(category, {{item, 3}});
        const int group = Group(category, item);
        OK(category.DropGroup(group, 1, Home, Home, category.GetRevision()));
        CHECK(category.Count(item) == 2 && category.GetState().worldDrops.size() == 1);
        const int firstDrop = category.GetState().worldDrops.front().id;
        OK(category.PickUpDrop(firstDrop, Home));
        CHECK(category.Count(item) == 3 && category.GetState().worldDrops.empty());
        OK(category.DropGroup(Group(category, item), 3, Home, Home, category.GetRevision()));
        CHECK(category.Count(item) == 0 && category.GetState().worldDrops.front().quantity == 3);
        OK(category.PickUpDrop(category.GetState().worldDrops.front().id, Home));
        CHECK(category.Count(item) == 3);
    }

    Simulation sim;
    Stock(sim, {{Item::Knife, 1}, {Item::Branch, 5}});
    const int branches = Group(sim, Item::Branch);
    const auto before = sim.GetRevision();
    OK(sim.DropGroup(branches, 2, Home, Home, before));
    CHECK(sim.Count(Item::Branch) == 3 && sim.GetState().worldDrops.size() == 1);
    CHECK(sim.GetState().worldDrops[0].item == Item::Branch
        && sim.GetState().worldDrops[0].quantity == 2);
    const int drop = sim.GetState().worldDrops[0].id;
    CHECK(sim.FindNearestDrop(Home, 1) == drop);
    UnchangedFailure(sim, [&] { return sim.PickUpDrop(drop, {Home.x + 301, Home.y}); });
    UnchangedFailure(sim, [&] { return sim.DropGroup(branches, 0, Home, Home, sim.GetRevision()); });
    UnchangedFailure(sim, [&] { return sim.DropGroup(branches, 1, WaterSource, WaterSource, sim.GetRevision()); });
    UnchangedFailure(sim, [&] { return sim.DropGroup(branches, 1, Home, Home, before); });
    OK(sim.DropGroup(branches, 3, {Home.x + 50, Home.y}, Home, sim.GetRevision()));
    CHECK(sim.Count(Item::Branch) == 0 && sim.GetState().worldDrops.size() == 1
        && sim.GetState().worldDrops[0].quantity == 5);

    const auto persisted = sim.Serialize();
    Simulation loaded;
    OK(loaded.Deserialize(persisted));
    CHECK(loaded.Serialize() == persisted && loaded.FindNearestDrop(Home, 100) == drop);
    Stock(loaded, {{Item::Knife, 1}, {Item::Stone, 119}});
    UnchangedFailure(loaded, [&] { return loaded.PickUpDrop(drop, Home); });
    CHECK(loaded.GetState().worldDrops.size() == 1);
    Stock(loaded, {{Item::Knife, 1}});
    OK(loaded.PickUpDrop(drop, Home));
    CHECK(loaded.Count(Item::Branch) == 5 && loaded.GetState().worldDrops.empty());
    UnchangedFailure(loaded, [&] { return loaded.PickUpDrop(drop, Home); });

    Simulation clothing;
    OK(clothing.UnequipWearable(1, clothing.GetRevision()));
    OK(clothing.RecolorWearable(1, 2, Home, clothing.GetRevision()));
    const auto tunic = *clothing.GetWearable(1);
    OK(clothing.DropWearable(tunic.id, Home, Home, clothing.GetRevision()));
    CHECK(clothing.GetWearable(tunic.id)->owner == WearableOwner::World);
    CHECK(clothing.GetState().worldDrops.size() == 1
        && clothing.GetState().worldDrops[0].wearableId == tunic.id);
    const int garmentDrop = clothing.GetState().worldDrops[0].id;
    InventoryRoundTrip(clothing);
    OK(clothing.PickUpDrop(garmentDrop, Home));
    CHECK(clothing.GetWearable(tunic.id)->owner == WearableOwner::Carried
        && clothing.GetWearable(tunic.id)->definition == tunic.definition
        && clothing.GetWearable(tunic.id)->dye == tunic.dye);

    State malformed = clothing.GetState();
    malformed.worldDrops.push_back({malformed.nextId++, Home, Item::Count, 1, 9999});
    Simulation rejected;
    UnchangedFailure(rejected, [&] { return rejected.Deserialize(Encode(malformed)); });
    malformed = clothing.GetState();
    malformed.worldDrops.push_back({malformed.nextId++, {MaxWorldCoordinate + 1, 0}, Item::Branch, 1, 0});
    UnchangedFailure(rejected, [&] { return rejected.Deserialize(Encode(malformed)); });
    malformed = clothing.GetState();
    malformed.worldDrops.push_back({malformed.nextId, Home, Item::Branch, 121, 0});
    ++malformed.nextId;
    UnchangedFailure(rejected, [&] { return rejected.Deserialize(Encode(malformed)); });
    malformed = clothing.GetState();
    malformed.worldDrops.push_back({malformed.nextId, Home, Item::Count, 1, tunic.id});
    ++malformed.nextId;
    UnchangedFailure(rejected, [&] { return rejected.Deserialize(Encode(malformed)); });

    Simulation blocked;
    BuildingStock(blocked);
    OK(blocked.Place(Piece::Chest, -3, 0, 0, Home));
    Stock(blocked, {{Item::Knife, 1}, {Item::Branch, 1}});
    UnchangedFailure(blocked, [&] { return blocked.DropGroup(Group(blocked, Item::Branch),
        1, CellCenter(-3, 0), CellCenter(-3, 0), blocked.GetRevision()); });
    Simulation plotBlocked;
    Stock(plotBlocked, {{Item::DiggingStick, 1}, {Item::Branch, 1}});
    OK(plotBlocked.Till(CellToGarden(-2), CellToGarden(-1), CellCenter(-2, -1)));
    UnchangedFailure(plotBlocked, [&] { return plotBlocked.DropGroup(Group(plotBlocked, Item::Branch),
        1, CellCenter(-2, -1), CellCenter(-2, -1), plotBlocked.GetRevision()); });

    Simulation multiple;
    Stock(multiple, {{Item::Branch, 2}, {Item::Stone, 2}, {Item::Berries, 2}, {Item::Seeds, 2}});
    const Item categories[] = {Item::Branch, Item::Stone, Item::Berries, Item::Seeds};
    for (int index = 0; index < 4; ++index)
        OK(multiple.DropGroup(Group(multiple, categories[index]), 1,
            {Home.x + index * 20.0, Home.y}, Home, multiple.GetRevision()));
    CHECK(multiple.GetState().worldDrops.size() == 4);
    for (const auto& value : std::vector<WorldDrop>(multiple.GetState().worldDrops))
        OK(multiple.PickUpDrop(value.id, Home));
    CHECK(multiple.GetState().worldDrops.empty());

    Simulation capped;
    Stock(capped, {{Item::Knife, 1}, {Item::Branch, 1}});
    Edit(capped, [](State& state) {
        for (int index = 0; index < MaxWorldDrops; ++index)
            state.worldDrops.push_back({state.nextId++, {10000, 10000}, Item::Stone, 1, 0});
    });
    UnchangedFailure(capped, [&] { return capped.DropGroup(Group(capped, Item::Branch),
        1, Home, Home, capped.GetRevision()); });
    const auto oldVersion = rejected.Deserialize(Encode(clothing.GetState(), 6));
    CHECK(!oldVersion && oldVersion.code == ResultCode::UnsupportedVersion);
}
void SelectedFoodGroupTransactions()
{
    for (const auto food : {std::pair<Item, double>{Item::Berries, 12.0},
        {Item::RoastedRoots, 28.0}, {Item::HerbedRoots, 38.0}})
    {
        for (double hunger : {50.0, 95.0})
        {
            Simulation sim;
            Stock(sim, {{Item::Knife, 1}, {food.first, 5}});
            Edit(sim, [&](State& state) { state.hunger = hunger; });
            const int first = Group(sim, food.first);
            OK(sim.SplitGroup(0, first, 2, Home, sim.GetRevision()));
            const int selected = sim.GetState().nextGroupId - 1;
            State expectedState = sim.GetState();
            expectedState.inventoryLayout.back().quantity = 1;
            --expectedState.inventory[static_cast<int>(food.first)];
            expectedState.hunger = hunger + food.second > 100.0 ? 100.0 : hunger + food.second;
            const double mealEnergy = food.first == Item::Berries ? 6.0 : food.first == Item::RoastedRoots ? 12.0 : 18.0;
            expectedState.energy = std::min(100.0, expectedState.energy + mealEnergy);
            Simulation expected;
            OK(expected.Deserialize(Encode(expectedState)));
            Simulation aggregate = sim;
            const auto aggregateResult = aggregate.Eat(food.first);
            OK(aggregateResult);
            const auto revision = sim.GetRevision();
            const auto result = sim.EatGroup(selected, revision);
            OK(result);
            CHECK(result.revision == revision + 1 && sim.GetRevision() == result.revision);
            CHECK(result.message == aggregateResult.message);
            CHECK(sim.GetState().hunger == aggregate.GetState().hunger);
            CHECK(sim.GetLayout(0)->at(1).groupId == first && sim.GetLayout(0)->at(1).quantity == 3);
            CHECK(sim.GetLayout(0)->back().groupId == selected && sim.GetLayout(0)->back().quantity == 1);
            CHECK(sim.Serialize() == expected.Serialize());
            UnchangedFailure(sim, [&] { return sim.EatGroup(selected, revision); });
            if (sim.GetState().hunger == 100.0)
            {
                UnchangedFailure(sim, [&] { return sim.EatGroup(selected, sim.GetRevision()); });
            }
            InventoryRoundTrip(sim);
        }
    }
    Simulation sim;
    BuildingStock(sim);
    OK(sim.Place(Piece::Chest, -3, 0, 0, Home));
    const int chest = sim.GetState().structures.back().id;
    Stock(sim, {{Item::Knife, 1}, {Item::Berries, 5}, {Item::Roots, 2}});
    Edit(sim, [](State& state) { state.hunger = 50; });
    const int berries = Group(sim, Item::Berries);
    OK(sim.SplitGroup(0, berries, 1, Home, sim.GetRevision()));
    const int single = sim.GetState().nextGroupId - 1;
    OK(sim.EatGroup(single, sim.GetRevision()));
    CHECK(sim.Count(Item::Berries) == 4 && sim.GetState().hunger == 62);
    CHECK(sim.GetLayout(0)->at(1).groupId == berries && sim.GetLayout(0)->at(1).quantity == 4);
    CHECK(sim.GetLayout(0)->size() == 3);
    UnchangedFailure(sim, [&] { return sim.EatGroup(single, sim.GetRevision()); });
    UnchangedFailure(sim, [&] { return sim.EatGroup(0, sim.GetRevision()); });
    UnchangedFailure(sim, [&] { return sim.EatGroup(-1, sim.GetRevision()); });
    UnchangedFailure(sim, [&] { return sim.EatGroup(Group(sim, Item::Knife), sim.GetRevision()); });
    UnchangedFailure(sim, [&] { return sim.EatGroup(Group(sim, Item::Roots), sim.GetRevision()); });
    OK(sim.TransferGroup(chest, berries, 2, true, Home, sim.GetRevision()));
    UnchangedFailure(sim, [&] { return sim.EatGroup(Group(sim, Item::Berries, chest), sim.GetRevision()); });
    const auto beforeLoad = sim.GetRevision();
    OK(sim.Deserialize(sim.Serialize()));
    UnchangedFailure(sim, [&] { return sim.EatGroup(berries, beforeLoad); });
    Edit(sim, [](State& state) { state.failed = true; state.energy = 0; });
    UnchangedFailure(sim, [&] { return sim.EatGroup(Group(sim, Item::Berries), sim.GetRevision()); });
    InventoryRoundTrip(sim);
}
void WardrobeSaveRejection()
{
    Simulation sim;
    Shod(sim);
    // Garment crafting survives only for older saves that still carry a knife and fibre.
    Stock(sim, {{Item::Knife, 1}, {Item::Fiber, 25}, {Item::Branch, 30}, {Item::Stone, 10}, {Item::BrambleCanes, 10}});
    OK(sim.Place(Piece::Chest, -3, 0, 0, Home));
    const int chest = sim.GetState().structures.back().id;
    OK(sim.CraftGarment(WearableDefinition::LinenApron, Home, sim.GetRevision()));
    const int apron = sim.GetState().wearables.back().id;
    OK(sim.MoveWearable(apron, chest, Home, sim.GetRevision()));
    const auto original = sim.Serialize();
    const auto revision = sim.GetRevision();
    const std::vector<std::function<void(State&)>> edits = {
        [](State& s) { s.wearables.push_back(s.wearables[0]); },
        [](State& s) { s.wearables[0].id = 0; },
        [](State& s) { s.wearables[0].definition = WearableDefinition::Count; },
        [](State& s) { s.wearables[0].definition = static_cast<WearableDefinition>(-1); },
        [](State& s) { s.wearables[0].dye = 4; },
        [](State& s) { s.wearables[1].dye = 1; },
        [](State& s) { s.wearables[0].owner = static_cast<WearableOwner>(3); },
        [](State& s) { s.wearables[0].chestId = 123; },
        [](State& s) { s.wearables.back().chestId = 0; },
        [](State& s) { s.wearables.back().chestId = 99999; },
        [](State& s) { s.wearables[0].owner = WearableOwner::Carried; },
        [](State& s) { s.equipment[1] = 0; },
        [](State& s) { s.equipment[2] = 1; },
        [](State& s) { s.equipment[3] = 99999; },
        [](State& s) { s.nextWearableId = s.wearables.back().id; },
        [](State& s) { s.nextWearableId = std::numeric_limits<int>::max(); },
        [](State& s) { s.nextGroupId = 1; },
        [](State& s) { s.nextGroupId = std::numeric_limits<int>::max(); },
        [](State& s) { s.inventoryLayout[0].quantity = 0; },
        [](State& s) { s.inventoryLayout[0].quantity = -1; },
        [](State& s) { s.inventoryLayout[0].quantity = 121; },
        [](State& s) { s.inventoryLayout[0].item = Item::Count; },
        [](State& s) { s.inventoryLayout[0].groupId = 0; },
        [](State& s) { s.inventoryLayout.push_back(s.inventoryLayout[0]); },
        [](State& s) { s.inventoryLayout[0].wearableId = 1; },
        [](State& s) { s.inventoryLayout.push_back({0, Item::Knife, 0, 1}); },
        [](State& s) { s.structures.back().layout.clear(); },
        [](State& s) { s.structures.back().layout.push_back(s.structures.back().layout[0]); },
        [](State& s) { s.structures.back().layout[0].quantity = 1; },
        [](State& s) { s.structures.back().layout[0].item = Item::Fiber; },
        [](State& s) { s.structures.back().layout[0].wearableId = 99999; }
    };
    for (const auto& edit : edits)
    {
        State bad = sim.GetState();
        edit(bad);
        CHECK(sim.Deserialize(Encode(bad)).code == ResultCode::CorruptSave);
        CHECK(sim.Serialize() == original && sim.GetRevision() == revision);
    }
    State full = sim.GetState();
    full.structures.back().storage[static_cast<int>(Item::Branch)] = ChestCapacity + 1;
    FixtureLayouts(full);
    CHECK(sim.Deserialize(Encode(full)).code == ResultCode::CorruptSave);
    CHECK(sim.Serialize() == original);
    State dependent = sim.GetState();
    dependent.wearables[0].owner = WearableOwner::Carried;
    dependent.wearables.back().owner = WearableOwner::Equipped;
    dependent.wearables.back().chestId = 0;
    dependent.equipment = {0, 0, apron, 2};
    FixtureLayouts(dependent);
    CHECK(sim.Deserialize(Encode(dependent)).code == ResultCode::CorruptSave);
    CHECK(sim.Serialize() == original);
    for (int version : {1, 2, 3, 4, 5, 6, RetiredTestSaveVersion, SimulationSaveVersion + 1, 999})
    {
        CHECK(sim.Deserialize(Encode(sim.GetState(), version)).code == ResultCode::UnsupportedVersion);
        CHECK(sim.Serialize() == original && sim.GetRevision() == revision);
    }
    InventoryRoundTrip(sim);
    OK(sim.Deserialize(original));
    OK(sim.Deserialize(original));
    CHECK(sim.GetState().wearables.size() == 3 && sim.GetWearable(apron)->chestId == chest);
    CHECK(sim.Serialize() == original);
}

void WorldStock(Simulation& sim, std::initializer_list<std::pair<Item, int>> items)
{
    Edit(sim, [&](State& state) {
        state.inventory.fill(0);
        for (const auto& item : items) state.inventory[static_cast<int>(item.first)] = item.second;
    }, false);
}
ResourceNode WorldNode(const Simulation& sim, ResourceKind kind)
{
    for (const auto& node : sim.GetState().resources)
        if (node.kind == kind && !node.cleared) return node;
    CHECK(false);
    return {};
}
void GeneratedWorldIdentityAndActivation()
{
    Simulation sim, same;
    CHECK(sim.Serialize() == same.Serialize());
    CHECK(sim.GetState().world.generationVersion == Generation::WorldGenerationVersion);
    CHECK(sim.GetState().activeChunk == (Generation::ChunkCoord{-1, 0}));
    CHECK(sim.GetState().nextId == 1);
    CHECK(sim.GetState().resourceEdits.empty());
    CHECK(sim.GetState().resources.size() <= 9 * Generation::MaxEntitiesPerChunk);
    CHECK(sim.GetState().resources.size() > 98);
    CHECK(sim.Count(Item::Knife) == 0 && sim.UsedCapacity() == 0);
    std::array<bool, static_cast<int>(ResourceKind::Count)> kinds{};
    for (const auto& node : sim.GetState().resources)
    {
        CHECK(node.id >= TransientResourceIdBase);
        kinds[static_cast<int>(node.kind)] = true;
        ResourceNode resolved;
        const auto before = sim.Serialize();
        const auto revision = sim.GetRevision();
        OK(sim.ResolveGeneratedResource(node.key, resolved));
        CHECK(resolved.id == node.id && resolved.kind == node.kind);
        CHECK(resolved.position.x == node.position.x && resolved.position.y == node.position.y);
        CHECK(before == sim.Serialize() && revision == sim.GetRevision());
        if (node.kind == ResourceKind::ForestTree)
        {
            const double dx = node.position.x + 1000, dy = node.position.y;
            CHECK(dx * dx + dy * dy > 90 * 90);
            double t = ((node.position.x + 1300) * 300 + (node.position.y + 80) * 80) / 96400.0;
            t = t < 0 ? 0 : t > 1 ? 1 : t;
            const double sx = node.position.x + 1300 - t * 300;
            const double sy = node.position.y + 80 - t * 80;
            CHECK(sx * sx + sy * sy > 60 * 60);
        }
    }
    // The seeded woodland generates the pre-estate kinds; overgrowth and spring flowers are estate placements.
    for (int kind = 0; kind <= static_cast<int>(ResourceKind::DeerRemains); ++kind) CHECK(kinds[kind]);
    const auto original = sim.Serialize();
    const auto originalNodes = sim.GetState().resources;
    const auto revision = sim.GetRevision();
    std::vector<Generation::ChunkBaseline> preparedChunks(9);
    PreparedWorldRegion prepared;
    prepared.world = sim.GetState().world;
    for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx)
        {
            const int index = (dy + 1) * 3 + dx + 1;
            CHECK(Generation::GenerateChunk(sim.GetState().world, {dx, dy},
                preparedChunks[index]) == Generation::Status::Ok);
            prepared.chunks[index] = &preparedChunks[index];
        }
    Simulation fromPrepared = sim;
    Simulation regenerated = sim;
    auto mismatched = prepared;
    mismatched.chunks[0] = prepared.chunks[1];
    UnchangedFailure(fromPrepared, [&] {
        return fromPrepared.SetActiveWorldRegion({1, 1}, &mismatched);
    });
    mismatched = prepared;
    ++mismatched.world.seed;
    UnchangedFailure(fromPrepared, [&] {
        return fromPrepared.SetActiveWorldRegion({1, 1}, &mismatched);
    });
    OK(fromPrepared.SetActiveWorldRegion({1, 1}, &prepared));
    OK(regenerated.SetActiveWorldRegion({1, 1}));
    CHECK(fromPrepared.Serialize() == regenerated.Serialize());
    CHECK(fromPrepared.GetRevision() == regenerated.GetRevision());
    OK(sim.SetActiveWorldRegion({-1, 1}));
    CHECK(sim.GetRevision() == revision);
    OK(sim.SetActiveWorldRegion({1, 1}));
    CHECK(sim.GetRevision() == revision + 1);
    for (const auto& old : originalNodes)
        for (const auto& current : sim.GetState().resources)
            if (old.key == current.key) CHECK(old.id == current.id);
    OK(sim.SetActiveWorldRegion({-1, -1}));
    CHECK(sim.GetState().activeChunk == (Generation::ChunkCoord{-1, -1}));
    for (const Point position : {Point{10000, 10000}, {-10000, 10000}, {10000, -10000},
        {-10000, -10000}, {MaxWorldCoordinate, MaxWorldCoordinate}, {-MaxWorldCoordinate, -MaxWorldCoordinate}})
    {
        OK(sim.SetActiveWorldRegion(position));
        CHECK(sim.GetState().resources.size() <= 9 * Generation::MaxEntitiesPerChunk);
        CHECK(sim.GetState().resourceEdits.empty());
        CHECK(sim.GetState().nextId == 1);
        CHECK(sim.Serialize().size() < 1024);
    }
    OK(sim.SetActiveWorldRegion({-1000, 0}));
    CHECK(sim.Serialize() == original);
    for (const auto& node : sim.GetState().resources) CHECK(node.id > originalNodes.back().id);
    UnchangedFailure(sim, [&] { return sim.Harvest(originalNodes.front().id, originalNodes.front().position); });
    UnchangedFailure(sim, [&] { return sim.SetActiveWorldRegion({1000001, 0}); });
    UnchangedFailure(sim, [&] { return sim.SetActiveWorldRegion({0, -1000001}); });
    UnchangedFailure(sim, [&] { return sim.SetActiveWorldRegion({std::numeric_limits<double>::quiet_NaN(), 0}); });
    ResourceNode untouched = sim.GetState().resources.front();
    CHECK(!sim.ResolveGeneratedResource({{0, 0}, 0}, untouched));
    CHECK(untouched.id == sim.GetState().resources.front().id);
    const auto previousHandle = sim.GetState().resources.back().id;
    OK(sim.NewGame(987654321));
    CHECK(sim.GetState().world.seed == 987654321 && sim.GetState().resourceEdits.empty());
    CHECK(sim.GetState().resources.front().id > previousHandle);
    OK(same.NewGame(987654321));
    CHECK(sim.Serialize() == same.Serialize());
    const auto saved = sim.Serialize();
    UnchangedFailure(sim, [&] { return sim.Deserialize(saved, {0, Generation::WorldGenerationVersion}); });
    UnchangedFailure(sim, [&] { return sim.Deserialize(saved,
        {987654321, Generation::WorldGenerationVersion + 1}); });
    OK(sim.Deserialize(saved, {987654321, Generation::WorldGenerationVersion}));
}
void GeneratedFellingAndPersistentTimers()
{
    Simulation sim;
    const auto tree = WorldNode(sim, ResourceKind::ForestTree);
    CHECK(!sim.CanHarvest(tree.id));
    UnchangedFailure(sim, [&] { return sim.Harvest(tree.id, tree.position); });
    UnchangedFailure(sim, [&] { return sim.Clear(tree.id, tree.position); });
    WorldStock(sim, {{Item::Hatchet, 1}, {Item::Branch, 110}});
    UnchangedFailure(sim, [&] { return sim.Clear(tree.id, tree.position); });
    CHECK(sim.GetState().resourceEdits.empty());
    WorldStock(sim, {{Item::Hatchet, 1}, {Item::Branch, 109}});
    UnchangedFailure(sim, [&] { return sim.Harvest(tree.id, {tree.position.x + 301, tree.position.y}); });
    const double hour = sim.GetState().hour;
    OK(sim.Harvest(tree.id, {tree.position.x + 300, tree.position.y}));
    CHECK(sim.UsedCapacity() == 120 && sim.Count(Item::Branch) == 113 &&
        sim.Count(Item::Timber) == 6 && sim.Count(Item::Fiber) == 0);
    CHECK(sim.GetState().hour == hour);
    CHECK(sim.GetState().resourceEdits.size() == 1);
    CHECK(sim.GetState().resourceEdits[0].key == tree.key && sim.GetState().resourceEdits[0].cleared);
    UnchangedFailure(sim, [&] { return sim.Harvest(tree.id, tree.position); });
    UnchangedFailure(sim, [&] { return sim.Clear(tree.id, tree.position); });
    WorldStock(sim, {{Item::Hatchet, 1}, {Item::Knife, 1}});
    const auto branch = WorldNode(sim, ResourceKind::Branches);
    OK(sim.Harvest(branch.id, branch.position));
    // Wild roots are the renewable forage whose timer persists (saplings are billhook overgrowth now).
    const auto sapling = WorldNode(sim, ResourceKind::Roots);
    OK(sim.Harvest(sapling.id, sapling.position));
    CHECK(sim.GetState().resourceEdits.size() == 3);
    ResourceNode resolved;
    OK(sim.SetActiveWorldRegion({-12000, -12000}));
    OK(sim.ResolveGeneratedResource(tree.key, resolved));
    CHECK(resolved.id == 0 && resolved.cleared && resolved.readyAtHour == 0);
    const auto save = sim.Serialize();
    Simulation loaded;
    OK(loaded.Deserialize(save));
    CHECK(loaded.Serialize() == save);
    OK(loaded.ResolveGeneratedResource(branch.key, resolved));
    CHECK(resolved.id == 0 && !resolved.cleared && resolved.readyAtHour == hour + 24);
    OK(loaded.ResolveGeneratedResource(sapling.key, resolved));
    CHECK(!resolved.cleared && resolved.readyAtHour == hour + 48);
    OK(loaded.SetActiveWorldRegion(tree.position));
    OK(loaded.ResolveGeneratedResource(tree.key, resolved));
    CHECK(resolved.id >= TransientResourceIdBase && resolved.cleared && !loaded.CanHarvest(resolved.id));
    Edit(loaded, [](State& state) { state.hour += 200; }, false);
    OK(loaded.ResolveGeneratedResource(tree.key, resolved));
    CHECK(resolved.cleared && !loaded.CanHarvest(resolved.id));
    OK(loaded.ResolveGeneratedResource(branch.key, resolved));
    CHECK(loaded.CanHarvest(resolved.id));
    const int before = loaded.Count(Item::Branch);
    OK(loaded.Harvest(resolved.id, resolved.position));
    CHECK(loaded.Count(Item::Branch) == before + 5);
    CHECK(loaded.GetState().resourceEdits.size() == 3);
    OK(loaded.ResolveGeneratedResource(sapling.key, resolved));
    CHECK(loaded.CanHarvest(resolved.id));
    const int rootsBeforeClear = loaded.Count(Item::Roots);
    OK(loaded.Clear(resolved.id, resolved.position));
    CHECK(loaded.Count(Item::Roots) == rootsBeforeClear + 2);
    InventoryRoundTrip(loaded);
}
void FellFixtureCell(Simulation& sim, int x, int y)
{
    const auto nodes = sim.GetState().resources;
    for (const auto& node : nodes)
    {
        if (node.cleared || (node.kind != ResourceKind::ForestTree && node.kind != ResourceKind::Sapling)) continue;
        const double left = x * CellSize, bottom = y * CellSize;
        const double px = std::max(left, std::min(left + CellSize, node.position.x));
        const double py = std::max(bottom, std::min(bottom + CellSize, node.position.y));
        const double dx = px - node.position.x, dy = py - node.position.y;
        const bool blocks = node.kind == ResourceKind::ForestTree ? dx * dx + dy * dy <= 2500 :
            static_cast<int>(std::floor(node.position.x / CellSize)) == x &&
            static_cast<int>(std::floor(node.position.y / CellSize)) == y;
        if (!blocks) continue;
        if (node.kind == ResourceKind::Sapling)
        {
            if (sim.Count(Item::Billhook) == 0) OK(sim.GrantItems(Item::Billhook, 1));
            OK(sim.ClearOvergrowth(node.id, Item::Billhook, node.position));
        }
        else OK(sim.Clear(node.id, node.position));
    }
}
void GeneratedBuildingFootprintAndReload()
{
    Simulation sim;
    OK(sim.SetActiveWorldRegion({-12000, 12000}));
    WorldStock(sim, {{Item::Hatchet, 1}, {Item::DiggingStick, 1}, {Item::Branch, 20}, {Item::Stone, 10}});
    auto tree = WorldNode(sim, ResourceKind::ForestTree);
    int cellX = static_cast<int>(std::floor(tree.position.x / CellSize));
    int cellY = static_cast<int>(std::floor(tree.position.y / CellSize));
    for (const auto& node : sim.GetState().resources)
    {
        if (node.kind != ResourceKind::ForestTree) continue;
        const int cx = static_cast<int>(std::floor(node.position.x / CellSize));
        if (node.position.x - cx * CellSize <= 50)
        {
            tree = node;
            cellX = cx - 1;
            cellY = static_cast<int>(std::floor(node.position.y / CellSize));
            break;
        }
    }
    CHECK(static_cast<int>(std::floor(tree.position.x / CellSize)) != cellX);
    const auto site = CellCenter(cellX, cellY);
    UnchangedFailure(sim, [&] { return sim.Place(Piece::Foundation, cellX, cellY, 0, site); });
    // The garden square beside the trunk, on this side of the cell edge, is blocked by it too.
    UnchangedFailure(sim, [&] { return sim.Till(cellX * GardenCellsPerCell + GardenCellsPerCell - 1, GardenCell(tree.position.y), site); });
    OK(sim.SetActiveWorldRegion({12000, -12000}));
    UnchangedFailure(sim, [&] { return sim.Place(Piece::Foundation, cellX, cellY, 0, site); });
    OK(sim.SetActiveWorldRegion(tree.position));
    FellFixtureCell(sim, cellX, cellY);
    OK(sim.Place(Piece::Foundation, cellX, cellY, 0, site));
    const int foundation = sim.GetState().structures.back().id;
    CHECK(foundation > 0 && foundation < TransientResourceIdBase);
    const int plotX = cellX + 2;
    FellFixtureCell(sim, plotX, cellY);
    OK(sim.Till(CellToGarden(plotX), CellToGarden(cellY), CellCenter(plotX, cellY)));
    CHECK(sim.GetState().plots.back().id < TransientResourceIdBase);
    OK(sim.SetActiveWorldRegion({12000, -12000}));
    const auto save = sim.Serialize();
    Simulation loaded;
    OK(loaded.Deserialize(save));
    CHECK(loaded.Serialize() == save);
    CHECK(loaded.GetState().structures.back().id == foundation);
    ResourceNode resolved;
    OK(loaded.ResolveGeneratedResource(tree.key, resolved));
    CHECK(resolved.cleared && resolved.id == 0);
    State corrupt = loaded.GetState();
    corrupt.resourceEdits.erase(std::remove_if(corrupt.resourceEdits.begin(), corrupt.resourceEdits.end(),
        [&](const ResourceEdit& edit) { return edit.key == tree.key; }), corrupt.resourceEdits.end());
    UnchangedFailure(loaded, [&] { return loaded.Deserialize(Encode(corrupt)); });
    OK(loaded.SetActiveWorldRegion(tree.position));
    CHECK(loaded.FindNearestStructure(site, Piece::Foundation, 1) == foundation);
}
void GeneratedSaveValidationAndEditLimit()
{
    Simulation sim;
    WorldStock(sim, {{Item::Knife, 1}, {Item::Hatchet, 1}});
    const auto branch = WorldNode(sim, ResourceKind::Branches);
    OK(sim.Harvest(branch.id, branch.position));
    const auto save = sim.Serialize();
    for (const auto& mutate : std::vector<std::function<void(State&)>>{
        [](State& s) { s.activeChunk.x = 417; },
        [](State& s) { s.activeChunk.y = -418; },
        [](State& s) { s.resourceEdits[0].key.localId = 0; },
        [](State& s) { s.resourceEdits[0].key.chunk.x = 500; },
        [](State& s) { s.resourceEdits[0].readyAtHour = 0; },
        [](State& s) { s.resourceEdits[0].readyAtHour = std::numeric_limits<double>::infinity(); },
        [](State& s) { s.resourceEdits[0].cleared = true; },
        [](State& s) { s.resourceEdits.push_back(s.resourceEdits[0]); },
        [](State& s) { s.resourceEdits.resize(MaxResourceEdits + 1); }})
    {
        State state = sim.GetState();
        mutate(state);
        UnchangedFailure(sim, [&] { return sim.Deserialize(Encode(state)); });
    }
    for (auto version : {0u, Generation::WorldGenerationVersion - 1,
        Generation::WorldGenerationVersion + 1, std::numeric_limits<std::uint32_t>::max()})
    {
        State state = sim.GetState();
        state.world.generationVersion = version;
        CHECK(sim.Deserialize(Encode(state)).code == ResultCode::UnsupportedVersion);
        CHECK(sim.Serialize() == save);
    }
    State full = sim.GetState();
    for (int y = 10; full.resourceEdits.size() < MaxResourceEdits; ++y)
        for (int x = 10; x < 40 && full.resourceEdits.size() < MaxResourceEdits; ++x)
        {
            Generation::ChunkBaseline chunk;
            CHECK(Generation::GenerateChunk(full.world, {x, y}, chunk) == Generation::Status::Ok);
            for (const auto& entity : chunk.entities)
            {
                if (full.resourceEdits.size() == MaxResourceEdits) break;
                full.resourceEdits.push_back({entity.key, true, 0});
            }
        }
    std::sort(full.resourceEdits.begin(), full.resourceEdits.end(),
        [](const ResourceEdit& a, const ResourceEdit& b) { return a.key < b.key; });
    OK(sim.Deserialize(Encode(full)));
    CHECK(sim.GetState().resourceEdits.size() == MaxResourceEdits);
    CHECK(sim.Serialize().size() < 8 * 1024 * 1024);
    const auto tree = WorldNode(sim, ResourceKind::ForestTree);
    UnchangedFailure(sim, [&] { return sim.Clear(tree.id, tree.position); });
    const auto herbs = WorldNode(sim, ResourceKind::Flowers);
    UnchangedFailure(sim, [&] { return sim.Harvest(herbs.id, herbs.position); });
    Edit(sim, [](State& state) { state.hour += 24; }, false);
    ResourceNode resolved;
    OK(sim.ResolveGeneratedResource(branch.key, resolved));
    OK(sim.Harvest(resolved.id, resolved.position));
    CHECK(sim.GetState().resourceEdits.size() == MaxResourceEdits);
    OK(sim.SetActiveWorldRegion({-100000, -100000}));
    CHECK(sim.GetState().resourceEdits.size() == MaxResourceEdits);
    InventoryRoundTrip(sim);
}

Point ChunkCenter(int x, int y)
{
    return {x * static_cast<double>(Generation::ChunkSizeCm) + Generation::ChunkSizeCm / 2.0,
        y * static_cast<double>(Generation::ChunkSizeCm) + Generation::ChunkSizeCm / 2.0};
}
std::vector<Generation::GeneratedEntityKey> ChunkKeys(const Simulation& sim, Generation::ChunkCoord chunk)
{
    std::vector<Generation::GeneratedEntityKey> keys;
    for (const auto& node : sim.GetState().resources)
        if (node.key.chunk == chunk) keys.push_back(node.key);
    std::sort(keys.begin(), keys.end());
    return keys;
}
ResourceNode ChunkNode(const Simulation& sim, Generation::ChunkCoord chunk, ResourceKind kind)
{
    for (const auto& node : sim.GetState().resources)
        if (node.key.chunk == chunk && node.kind == kind && !node.cleared) return node;
    CHECK(false);
    return {};
}
void LongDeterministicChunkWalk()
{
    Simulation sim;
    const auto startingInventory = sim.GetState().inventory;
    const int startingNextId = sim.GetState().nextId;
    std::size_t smallestSave = std::numeric_limits<std::size_t>::max();
    std::size_t largestSave = 0;
    int visited = 0;
    std::vector<Generation::GeneratedEntityKey> rememberedKeys;
    int rememberedMaxHandle = 0;
    for (int row = -8; row <= 8; ++row)
    {
        const int first = row % 2 == 0 ? -8 : 8;
        const int last = -first;
        const int step = first < last ? 1 : -1;
        for (int column = first;; column += step)
        {
            const Point center = ChunkCenter(column, row);
            OK(sim.SetActiveWorldRegion(center));
            CHECK(sim.GetState().activeChunk == (Generation::ChunkCoord{column, row}));
            CHECK(sim.GetState().resources.size() <= 9 * Generation::MaxEntitiesPerChunk);
            CHECK(sim.GetState().resourceEdits.empty());
            CHECK(sim.GetState().inventory == startingInventory);
            CHECK(sim.GetState().nextId == startingNextId);
            for (const auto& node : sim.GetState().resources)
                CHECK(node.id >= TransientResourceIdBase);
            const auto save = sim.Serialize();
            smallestSave = std::min(smallestSave, save.size());
            largestSave = std::max(largestSave, save.size());
            if (visited % 31 == 0)
            {
                Simulation loaded;
                OK(loaded.Deserialize(save));
                CHECK(loaded.Serialize() == save);
                CHECK(ChunkKeys(loaded, {column, row}) == ChunkKeys(sim, {column, row}));
                for (const auto& node : loaded.GetState().resources)
                {
                    CHECK(node.id >= TransientResourceIdBase);
                    ResourceNode resolved;
                    OK(loaded.ResolveGeneratedResource(node.key, resolved));
                    CHECK(resolved.id == node.id && resolved.position.x == node.position.x &&
                        resolved.position.y == node.position.y);
                }
            }
            if (column == 0 && row == 0)
            {
                rememberedKeys = ChunkKeys(sim, {0, 0});
                for (const auto& node : sim.GetState().resources)
                    rememberedMaxHandle = std::max(rememberedMaxHandle, node.id);
            }
            ++visited;
            if (column == last) break;
        }
    }
    CHECK(visited == 289);
    CHECK(largestSave - smallestSave <= 8);
    CHECK(largestSave < 1024);
    CHECK(sim.GetState().resourceEdits.empty());
    OK(sim.SetActiveWorldRegion(ChunkCenter(0, 0)));
    CHECK(ChunkKeys(sim, {0, 0}) == rememberedKeys);
    // Current implementation guarantees no handle reuse during this Simulation lifetime.
    for (const auto& node : sim.GetState().resources) CHECK(node.id > rememberedMaxHandle);
    const auto save = sim.Serialize();
    Simulation loaded;
    OK(loaded.Deserialize(save));
    CHECK(loaded.Serialize() == save);
    CHECK(ChunkKeys(loaded, {0, 0}) == rememberedKeys);
    CHECK(loaded.GetState().resourceEdits.empty());
}
void MixedPersistentWorldChurn()
{
    Simulation sim;
    WorldStock(sim, {{Item::Knife, 1}, {Item::Hatchet, 1}, {Item::DiggingStick, 1}});
    struct Remembered
    {
        Generation::GeneratedEntityKey tree;
        Generation::GeneratedEntityKey branches;
        double branchesReady = 0.0;
    };
    std::vector<Remembered> edits;
    for (const auto chunk : {Generation::ChunkCoord{-12, -9}, {-12, 9}, {12, -9}, {12, 9}})
    {
        OK(sim.SetActiveWorldRegion(ChunkCenter(chunk.x, chunk.y)));
        const auto tree = ChunkNode(sim, chunk, ResourceKind::ForestTree);
        const auto branches = ChunkNode(sim, chunk, ResourceKind::Branches);
        OK(sim.Clear(tree.id, tree.position));
        OK(sim.Harvest(branches.id, branches.position));
        ResourceNode resolved;
        OK(sim.ResolveGeneratedResource(branches.key, resolved));
        edits.push_back({tree.key, branches.key, resolved.readyAtHour});
        WorldStock(sim, {{Item::Knife, 1}, {Item::Hatchet, 1}, {Item::DiggingStick, 1}});
    }
    CHECK(sim.GetState().resourceEdits.size() == 8);
    OK(sim.SetActiveWorldRegion(ChunkCenter(-12, -9)));
    const auto buildTree = ChunkNode(sim, {-12, -9}, ResourceKind::ForestTree);
    const int cellX = static_cast<int>(std::floor(buildTree.position.x / CellSize));
    const int cellY = static_cast<int>(std::floor(buildTree.position.y / CellSize));
    WorldStock(sim, {{Item::Knife, 1}, {Item::Hatchet, 1}, {Item::DiggingStick, 1},
        {Item::Branch, 30}, {Item::Stone, 20}});
    FellFixtureCell(sim, cellX, cellY);
    OK(sim.Place(Piece::Foundation, cellX, cellY, 0, CellCenter(cellX, cellY)));
    const int structureId = sim.GetState().structures.back().id;
    int plotX = cellX + 2;
    FellFixtureCell(sim, plotX, cellY);
    OK(sim.Till(CellToGarden(plotX), CellToGarden(cellY), CellCenter(plotX, cellY)));
    const int plotId = sim.GetState().plots.back().id;
    const auto beforeTravel = sim.Serialize();
    for (int i = -20; i <= 20; ++i)
        OK(sim.SetActiveWorldRegion(ChunkCenter(i, i % 2 ? -15 : 15)));
    CHECK(sim.GetState().resourceEdits.size() >= 8);
    CHECK(sim.GetState().structures.back().id == structureId);
    CHECK(sim.GetState().plots.back().id == plotId);
    Simulation loaded;
    OK(loaded.Deserialize(sim.Serialize()));
    CHECK(loaded.Serialize() == sim.Serialize());
    for (const auto& edit : edits)
    {
        ResourceNode tree, branches;
        OK(loaded.ResolveGeneratedResource(edit.tree, tree));
        OK(loaded.ResolveGeneratedResource(edit.branches, branches));
        CHECK(tree.id == 0 && tree.cleared);
        CHECK(branches.id == 0 && !branches.cleared && branches.readyAtHour == edit.branchesReady);
    }
    Edit(loaded, [](State& state) { state.hour += 25; }, false);
    for (const auto& edit : edits)
    {
        OK(loaded.SetActiveWorldRegion(ChunkCenter(edit.branches.chunk.x, edit.branches.chunk.y)));
        ResourceNode tree, branches;
        OK(loaded.ResolveGeneratedResource(edit.tree, tree));
        OK(loaded.ResolveGeneratedResource(edit.branches, branches));
        CHECK(tree.cleared && !loaded.CanHarvest(tree.id));
        CHECK(loaded.CanHarvest(branches.id));
    }
    CHECK(loaded.FindNearestStructure(CellCenter(cellX, cellY), Piece::Foundation, 1) == structureId);
    CHECK(loaded.FindNearestPlot(CellCenter(plotX, cellY), 1) == plotId);
    CHECK(beforeTravel.size() <= loaded.Serialize().size() + 32);
}
void SparseEditScaleAndPayloadBounds()
{
    Simulation sim;
    WorldStock(sim, {{Item::Knife, 1}, {Item::Hatchet, 1}});
    const std::size_t untouchedSize = sim.Serialize().size();
    std::size_t lastSize = untouchedSize;
    int actions = 0;
    for (int row = -8; row < 8; ++row)
        for (int column = -8; column < 8; ++column)
        {
            const Generation::ChunkCoord chunk{column * 2, row * 2};
            OK(sim.SetActiveWorldRegion(ChunkCenter(chunk.x, chunk.y)));
            const auto branches = ChunkNode(sim, chunk, ResourceKind::Branches);
            OK(sim.Harvest(branches.id, branches.position));
            ++actions;
            CHECK(static_cast<int>(sim.GetState().resourceEdits.size()) == actions);
            if (actions % 20 == 0)
            {
                WorldStock(sim, {{Item::Knife, 1}, {Item::Hatchet, 1}});
                // 256 harvests is far more work than one day's Energy; rest between batches.
                Edit(sim, [](State& state) { state.energy = 100; }, false);
            }
            const auto size = sim.Serialize().size();
            CHECK(size >= lastSize - 4);
            lastSize = size;
        }
    CHECK(actions == 256);
    CHECK(sim.GetState().resourceEdits.size() == 256);
    CHECK(lastSize > untouchedSize);
    CHECK(lastSize < 32 * 1024);
    Simulation loaded;
    OK(loaded.Deserialize(sim.Serialize()));
    CHECK(loaded.Serialize() == sim.Serialize());
    State nearlyFull = sim.GetState();
    for (int y = 100; nearlyFull.resourceEdits.size() < MaxResourceEdits; ++y)
        for (int x = -100; x < 100 && nearlyFull.resourceEdits.size() < MaxResourceEdits; ++x)
        {
            Generation::ChunkBaseline chunk;
            CHECK(Generation::GenerateChunk(nearlyFull.world, {x, y}, chunk) == Generation::Status::Ok);
            for (const auto& entity : chunk.entities)
            {
                if (nearlyFull.resourceEdits.size() == MaxResourceEdits) break;
                nearlyFull.resourceEdits.push_back({entity.key, true, 0});
            }
        }
    std::sort(nearlyFull.resourceEdits.begin(), nearlyFull.resourceEdits.end(),
        [](const ResourceEdit& a, const ResourceEdit& b) { return a.key < b.key; });
    nearlyFull.resourceEdits.erase(std::unique(nearlyFull.resourceEdits.begin(), nearlyFull.resourceEdits.end(),
        [](const ResourceEdit& a, const ResourceEdit& b) { return a.key == b.key; }), nearlyFull.resourceEdits.end());
    CHECK(nearlyFull.resourceEdits.size() == MaxResourceEdits);
    OK(sim.Deserialize(Encode(nearlyFull)));
    const auto fullSave = sim.Serialize();
    CHECK(fullSave.size() > lastSize);
    CHECK(fullSave.size() < 8 * 1024 * 1024);
    const auto revision = sim.GetRevision();
    const auto inventory = sim.GetState().inventory;
    ResourceNode existing;
    for (const auto& node : sim.GetState().resources)
    {
        if (node.kind != ResourceKind::Branches) continue;
        const auto found = std::lower_bound(sim.GetState().resourceEdits.begin(), sim.GetState().resourceEdits.end(),
            node.key, [](const ResourceEdit& edit, const Generation::GeneratedEntityKey& key) { return edit.key < key; });
        if (found != sim.GetState().resourceEdits.end() && found->key == node.key)
        {
            existing = node;
            break;
        }
    }
    CHECK(existing.id != 0);
    OK(sim.Clear(existing.id, existing.position));
    CHECK(sim.GetState().resourceEdits.size() == MaxResourceEdits);
    CHECK(sim.GetRevision() == revision + 1);
    CHECK(sim.GetState().inventory == inventory);
    const auto beforeReject = sim.Serialize();
    ResourceNode fresh;
    for (const auto& node : sim.GetState().resources)
    {
        const auto found = std::lower_bound(sim.GetState().resourceEdits.begin(), sim.GetState().resourceEdits.end(),
            node.key, [](const ResourceEdit& edit, const Generation::GeneratedEntityKey& key) { return edit.key < key; });
        if (found == sim.GetState().resourceEdits.end() || found->key != node.key) { fresh = node; break; }
    }
    CHECK(fresh.id != 0);
    UnchangedFailure(sim, [&] {
        return fresh.kind == ResourceKind::ForestTree ? sim.Clear(fresh.id, fresh.position) :
            sim.Harvest(fresh.id, fresh.position);
    });
    CHECK(sim.Serialize() == beforeReject);
    const std::string oversized(8 * 1024 * 1024 + 1, 'x');
    UnchangedFailure(sim, [&] { return sim.Deserialize(oversized); });
    std::string corrupt = fullSave;
    corrupt[corrupt.size() / 2] ^= 1;
    UnchangedFailure(sim, [&] { return sim.Deserialize(corrupt); });
    std::cout << "Persistent world stress: visited=289 unedited, edited=256, maxEdits="
        << MaxResourceEdits << ", fullSaveBytes=" << fullSave.size() << ".\n";
}

void SprintEnergyContract()
{
    Simulation sim;
    const auto revision = sim.GetRevision();
    const double before = sim.GetState().energy;
    UnchangedFailure(sim, [&] { return sim.SpendSprintEnergy(-1); });
    UnchangedFailure(sim, [&] { return sim.SpendSprintEnergy(0); });
    UnchangedFailure(sim, [&] {
        return sim.SpendSprintEnergy(std::numeric_limits<double>::quiet_NaN());
    });
    OK(sim.SpendSprintEnergy(1));
    CHECK(std::abs(sim.GetState().energy - (before - 0.35)) < 0.00001);
    CHECK(sim.GetRevision() == revision);
    for (int i = 0; i < 30 && sim.GetState().energy > 10; ++i)
        OK(sim.SpendSprintEnergy(10));
    CHECK(sim.GetState().energy == 10.0);
    UnchangedFailure(sim, [&] { return sim.SpendSprintEnergy(1); });
    Simulation loaded;
    OK(loaded.Deserialize(sim.Serialize()));
    CHECK(loaded.GetState().energy == 10.0);
}

void ActionEnergyContract()
{
    Simulation sim;
    Stock(sim, {{Item::RustedHoeBlade, 1}, {Item::DiggingStick, 1}, {Item::Seeds, 2}, {Item::Branch, 12}, {Item::Stone, 8}});
    const double start = sim.GetState().energy;
    const auto branch = Node(sim, ResourceKind::Branches);
    OK(sim.Harvest(branch.id, branch.position));
    CHECK(Close(sim.GetState().energy, start - Exertion::GatherEnergy, 1e-9));
    OK(sim.Craft(Recipe::HaftHoe, Home));
    CHECK(Close(sim.GetState().energy, start - Exertion::GatherEnergy - Exertion::CraftEnergy, 1e-9));
    OK(sim.Till(CellToGarden(-2), CellToGarden(-1), CellCenter(-2, -1)));
    CHECK(Close(sim.GetState().energy, start - Exertion::GatherEnergy - Exertion::CraftEnergy - Exertion::TillEnergy, 1e-9));
    // Rejected work costs nothing.
    UnchangedFailure(sim, [&] { return sim.Till(CellToGarden(-2), CellToGarden(-1), CellCenter(-2, -1)); });
    // Too tired: work is refused before it would leave her under the reserve, and nothing changes.
    Edit(sim, [](State& state) { state.energy = Exertion::Reserve + Exertion::TillEnergy - 0.01; state.hunger = 50; });
    UnchangedFailure(sim, [&] { return sim.Till(CellToGarden(-3), CellToGarden(-1), CellCenter(-3, -1)); });
    Simulation probe;
    OK(probe.Deserialize(sim.Serialize()));
    CHECK(probe.Till(CellToGarden(-3), CellToGarden(-1), CellCenter(-3, -1)).message.find("exhausted") != std::string::npos);
    // Light work is still possible at the same Energy, and exertion never drops her below the reserve.
    const int plot = sim.GetState().plots[0].id;
    OK(sim.Plant(plot, CellCenter(-2, -1)));
    CHECK(sim.GetState().energy >= Exertion::Reserve);
    CHECK(!sim.GetState().failed);
    // A meal restores enough to carry on.
    Stock(sim, {{Item::DiggingStick, 1}, {Item::Berries, 1}});
    OK(sim.Eat(Item::Berries));
    OK(sim.Till(CellToGarden(-3), CellToGarden(-1), CellCenter(-3, -1)));
    // An awake game hour drains only a little.
    Simulation idle;
    const double rested = idle.GetState().energy;
    idle.AdvanceGameHours(1, Home);
    CHECK(Close(idle.GetState().energy, rested - Exertion::AwakePerHour, 1e-9));
}

void Run(const char* name, void (*test)())
{
    test();
    ++cases;
    std::cout << "PASS " << name << '\n';
}

void FixedEstateNewGameAndSave()
{
    const EstateLayout& layout = ProvisionalEstateLayout();
    CHECK(layout.FindLandmark(Anchor::StandingRoomSpawn) != nullptr);
    CHECK(layout.FindPolygon(Anchor::EstateBoundary) != nullptr);
    const Point spawn = layout.PointOr(Anchor::StandingRoomSpawn, {});
    CHECK(PointInPolygon(layout.FindPolygon(Anchor::EstateBoundary)->points, spawn));
    CHECK(!PointInPolygon(layout.FindPolygon(Anchor::EstateBoundary)->points,
        layout.PointOr(Anchor::TownSquare, {})));
    // The standing room is the carved-out corner of the ruin, and everything but its salvage lies outside it.
    const auto& footprint = layout.FindPolygon(Anchor::ManorFootprint)->points;
    CHECK(!PointInPolygon(footprint, spawn));
    int misplaced = 0;
    for (const auto& placement : ProvisionalEstatePlacements().placements)
    {
        const bool onEstate = PointInPolygon(layout.FindPolygon(Anchor::EstateBoundary)->points, placement.position);
        const bool inRuin = placement.kind != ResourceKind::SalvagePile && PointInPolygon(footprint, placement.position);
        if (!onEstate || inRuin)
            std::cout << "Placement " << placement.id << " at (" << placement.position.x << ", " << placement.position.y
                << ") is " << (inRuin ? "inside the ruin footprint" : "off the estate") << ".\n";
        misplaced += !onEstate || inRuin;
    }
    CHECK(misplaced == 0);
    EstatePlacements placements;
    placements.bakeVersion = 3;
    placements.placements = {
        {EstatePlacementIdBase + 1, ResourceKind::Branches, {spawn.x + 100, spawn.y}, 0, 0, 1, 0},
        {EstatePlacementIdBase + 2, ResourceKind::Stones, {spawn.x - 100, spawn.y}, 0, 0, 1, 0},
    };
    Simulation provisional;
    OK(provisional.NewEstateGame(layout, ProvisionalEstatePlacements()));
    CHECK(!provisional.GetState().resources.empty());
    Simulation sim;
    OK(sim.NewEstateGame(layout, placements));
    CHECK(sim.GetState().fixedEstate);
    CHECK(sim.GetState().resources.size() == 2);
    CHECK(sim.GetState().resources[0].id == EstatePlacementIdBase + 1);
    OK(sim.SetActiveWorldRegion({spawn.x + 900000, spawn.y}));
    CHECK(sim.GetState().resources.size() == 2);
    // The pail is her one starting tool, waiting in the standing room's chest; gathering needs no knife.
    CHECK(sim.Count(Item::WateringCan) == 0 && sim.Count(Item::Knife) == 0 && sim.UsedCapacity() == 0);
    int pails = 0;
    for (const auto& piece : sim.GetState().structures) pails += piece.storage[static_cast<int>(Item::WateringCan)];
    CHECK(pails == 1);
    OK(sim.Harvest(EstatePlacementIdBase + 1, spawn));
    const std::string saved = sim.Serialize();
    Simulation loaded;
    CHECK(!loaded.Deserialize(saved));
    loaded.SetPlacements(placements);
    OK(loaded.Deserialize(saved));
    CHECK(loaded.GetState().fixedEstate && loaded.GetState().placementBakeVersion == 3);
    CHECK(loaded.GetState().resources.size() == 2);
    CHECK(loaded.GetState().resources[0].cleared == sim.GetState().resources[0].cleared);
    CHECK(loaded.GetState().resources[0].readyAtHour == sim.GetState().resources[0].readyAtHour);
    CHECK(loaded.Count(Item::Branch) == sim.Count(Item::Branch));
    EstatePlacements rebaked = placements;
    rebaked.bakeVersion = 4;
    Simulation other;
    other.SetPlacements(rebaked);
    CHECK(other.Deserialize(saved).code == ResultCode::UnsupportedVersion);
    // Water on the estate comes only from the game's probe, never the generated stream.
    OK(sim.GrantItems(Item::WateringCan, 1));
    const Point stream{StreamX(spawn.y), spawn.y};
    CHECK(!sim.FillWater(stream).ok);
    sim.SetWaterProbe([spawn](Point p) { return std::abs(p.x - spawn.x) < 50 && std::abs(p.y - spawn.y) < 50; });
    CHECK(!sim.FillWater(stream).ok);
    OK(sim.FillWater(spawn));
}

// A small hand-authored estate for the overgrowth rules.
EstatePlacements OvergrowthFixture(Point at)
{
    EstatePlacements table;
    table.bakeVersion = 7;
    int next = EstatePlacementIdBase + 10000;
    const auto add = [&](ResourceKind kind, double dx, double dy, int minTier = 0)
    {
        table.placements.push_back({next++, kind, {at.x + dx, at.y + dy}, 0, 0, 1, minTier});
        return table.placements.back().id;
    };
    add(ResourceKind::SalvagePile, 100, 0);
    add(ResourceKind::FallenBranch, 0, 100);
    add(ResourceKind::BrambleThin, -100, 0);
    add(ResourceKind::BrambleThicket, 0, -100);
    add(ResourceKind::StumpSmall, 150, 150);
    add(ResourceKind::StumpLarge, -150, 150);
    add(ResourceKind::Rubble, 150, -150);
    add(ResourceKind::Boulder, -150, -150);
    add(ResourceKind::BrambleThin, 200, 0, 1); // Authored as an iron-tier tease.
    return table;
}
int PlacedId(const Simulation& sim, ResourceKind kind, int skip = 0)
{
    for (const auto& node : sim.GetState().resources)
        if (node.kind == kind && skip-- == 0) return node.id;
    CHECK(false);
    return -1;
}
const ResourceNode& PlacedNode(const Simulation& sim, int id)
{
    for (const auto& node : sim.GetState().resources) if (node.id == id) return node;
    CHECK(false);
    return sim.GetState().resources.front();
}

const Structure& StructureById(const Simulation& sim, int id)
{
    for (const auto& piece : sim.GetState().structures) if (piece.id == id) return piece;
    CHECK(false);
    return sim.GetState().structures.front();
}

int StructureIdAt(const Simulation& sim, Piece kind, int x, int y)
{
    for (const auto& piece : sim.GetState().structures)
        if (piece.kind == kind && piece.cellX == x && piece.cellY == y) return piece.id;
    CHECK(false);
    return -1;
}

void DeconstructRefundsAndFoundationRefusal()
{
    Simulation sim;
    BuildingStock(sim);
    OK(sim.Place(Piece::Foundation, -3, 0, 0, Home));
    const int floor = StructureIdAt(sim, Piece::Foundation, -3, 0);
    OK(sim.Place(Piece::Wall, -3, 0, 1, Home));
    const int wall = StructureIdAt(sim, Piece::Wall, -3, 0);
    OK(sim.Place(Piece::Roof, -3, 0, 0, Home));
    OK(sim.Place(Piece::Chest, -3, 0, 0, Home));
    CHECK(!sim.CheckDeconstruct(floor, Home).ok);
    CHECK(sim.CheckDeconstruct(floor, Home).message.find("floor first") != std::string::npos);
    const int branches = sim.Count(Item::Branch);
    const int canes = sim.Count(Item::BrambleCanes);
    const double energy = sim.GetState().energy;
    OK(sim.CheckDeconstruct(wall, Home));
    OK(sim.Deconstruct(wall, Home));
    CHECK(sim.Count(Item::Branch) == branches + 3);
    CHECK(sim.Count(Item::BrambleCanes) == canes + 1);
    CHECK(Close(sim.GetState().energy, energy - Exertion::DeconstructEnergy));
    CHECK(std::none_of(sim.GetState().structures.begin(), sim.GetState().structures.end(),
        [wall](const Structure& piece) { return piece.id == wall; }));
}

void DeconstructChestContentsAndOverflow()
{
    Simulation returned;
    Stock(returned, {{Item::Branch, 105}, {Item::BrambleCanes, 2}});
    const Point chestSite = CellCenter(2, 0);
    OK(returned.Place(Piece::Chest, 2, 0, 0, chestSite));
    const int chest = StructureIdAt(returned, Piece::Chest, 2, 0);
    OK(returned.Transfer(chest, Item::Branch, 10, chestSite));
    OK(returned.Deconstruct(chest, chestSite));
    CHECK(returned.Count(Item::Branch) == 105);
    CHECK(returned.Count(Item::BrambleCanes) == 2);
    CHECK(returned.GetState().worldDrops.empty());

    Simulation overflow;
    Stock(overflow, {{Item::Branch, 105}, {Item::BrambleCanes, 2}});
    OK(overflow.Place(Piece::Chest, 2, 0, 0, chestSite));
    const int fullChest = StructureIdAt(overflow, Piece::Chest, 2, 0);
    OK(overflow.Transfer(fullChest, Item::Branch, 60, chestSite));
    OK(overflow.GrantItems(Item::Stone, 80));
    CHECK(overflow.UsedCapacity() == InventoryCapacity);
    OK(overflow.Deconstruct(fullChest, chestSite));
    CHECK(overflow.UsedCapacity() == InventoryCapacity);
    int dropped = 0;
    for (const auto& drop : overflow.GetState().worldDrops) dropped += drop.quantity;
    CHECK(dropped == 67);
}

void HeritageManorPiecesCannotBeDeconstructed()
{
    EstatePlacements placements;
    placements.bakeVersion = 11;
    Simulation sim;
    OK(sim.NewEstateGame(ProvisionalEstateLayout(), placements));
    int refused = 0;
    for (const auto& piece : sim.GetState().structures)
    {
        if (!piece.heritage) continue;
        const auto result = sim.CheckDeconstruct(piece.id, StructureCenter(sim.GetState(), piece));
        CHECK(!result.ok);
        CHECK(result.message == "This is part of the old house; it can't be taken down.");
        ++refused;
    }
    CHECK(refused >= Manor::RoomCells * Manor::RoomCells + 3);
}

void ChestCapacityAndWaterSpace()
{
    Simulation chestSim;
    Stock(chestSim, {{Item::Branch, 5}, {Item::BrambleCanes, 2}});
    const Point chestSite = CellCenter(3, 0);
    OK(chestSim.Place(Piece::Chest, 3, 0, 0, chestSite));
    const int chest = StructureIdAt(chestSim, Piece::Chest, 3, 0);
    for (int i = 0; i < 10; ++i)
    {
        OK(chestSim.GrantItems(Item::Branch, InventoryCapacity));
        OK(chestSim.Transfer(chest, Item::Branch, InventoryCapacity, chestSite));
    }
    CHECK(chestSim.ChestUsedCapacity(chest) == ChestCapacity);
    OK(chestSim.GrantItems(Item::Branch, 1));
    CHECK(chestSim.Transfer(chest, Item::Branch, 1, chestSite).code == ResultCode::Capacity);

    Simulation waterSim;
    OK(waterSim.GrantItems(Item::WateringCan, 1));
    OK(waterSim.GrantItems(Item::Branch, InventoryCapacity - 1));
    CHECK(waterSim.UsedCapacity() == InventoryCapacity);
    OK(waterSim.FillWater(WaterSource));
    CHECK(waterSim.Count(Item::Water) == 6);
    CHECK(waterSim.UsedCapacity() == InventoryCapacity);
}

void BedSleepHourPolicy()
{
    CHECK(Close(BedSleepHours(20.0), 10.75));
    CHECK(Close(BedSleepHours(3.0), 8.0));
    CHECK(Close(BedSleepHours(12.0), 2.0));
    CHECK(Close(BedSleepHours(16.5), 1.5));
}

void OvergrowthTableAndPrompts()
{
    for (auto kind : {ResourceKind::TallGrass, ResourceKind::Weeds}) CHECK(FindOvergrowth(kind)->tool == ToolKind::Scythe);
    for (auto kind : {ResourceKind::BrambleThin, ResourceKind::Sapling, ResourceKind::BrambleThicket, ResourceKind::BrambleBank})
        CHECK(FindOvergrowth(kind)->tool == ToolKind::Billhook);
    for (auto kind : {ResourceKind::FallenBranch, ResourceKind::StumpSmall, ResourceKind::StumpLarge,
        ResourceKind::StumpAncient, ResourceKind::FallenLog, ResourceKind::GiantLog})
        CHECK(FindOvergrowth(kind)->tool == ToolKind::Axe);
    for (auto kind : {ResourceKind::Rubble, ResourceKind::SmallRock, ResourceKind::Boulder})
        CHECK(FindOvergrowth(kind)->tool == ToolKind::Pickaxe);
    CHECK(FindOvergrowth(ResourceKind::SalvagePile)->tool == ToolKind::Count && FindOvergrowth(ResourceKind::SalvagePile)->byHand);
    CHECK(FindOvergrowth(ResourceKind::BrambleThicket)->minTier == ToolTier::Iron);
    CHECK(FindOvergrowth(ResourceKind::BrambleBank)->minTier == ToolTier::Steel);
    CHECK(FindOvergrowth(ResourceKind::StumpLarge)->minTier == ToolTier::Iron && FindOvergrowth(ResourceKind::Boulder)->minTier == ToolTier::Iron);
    CHECK(!IsOvergrowth(ResourceKind::ForestTree) && !IsOvergrowth(ResourceKind::Branches) && !IsOvergrowth(ResourceKind::Primroses));
    for (int i = 0; i < static_cast<int>(ResourceKind::Count); ++i)
        if (const auto* info = FindOvergrowth(static_cast<ResourceKind>(i)))
            for (int tier = 1; tier < ToolTierCount; ++tier) CHECK(info->swings[tier] <= info->swings[tier - 1] && info->swings[tier] >= 1);
    CHECK(NeedsToolMessage(ToolKind::Axe, ToolTier::Iron) == "Needs an iron axe");
    CHECK(NeedsToolMessage(ToolKind::Billhook, ToolTier::Steel) == "Needs a steel billhook");
    CHECK(NeedsToolMessage(ToolKind::Pickaxe, ToolTier::Master) == "Needs a master-forged pickaxe");
    CHECK(ToolForItem(Item::Hatchet) == ToolKind::Axe && ToolItem(ToolKind::Hoe) == Item::DiggingStick);
    CHECK(ToolForItem(Item::Knife) == ToolKind::Count && ToolForItem(Item::Machete) == ToolKind::Count);
    CHECK(std::string(ItemName(Item::Hatchet)) == "Axe" && std::string(ItemName(Item::DiggingStick)) == "Hoe");
    CHECK(std::string(ItemName(Item::WateringCan)) == "Pail" && std::string(RecipeName(Recipe::HaftBillhook)) == "Craft a billhook");
    CHECK(std::string(RecipeRequirements(Recipe::HaftBillhook)) == "2 Branch + 1 Rusted billhook head; by hand, no station");
    CHECK(std::string(ResourceName(ResourceKind::FallenBranch)) == "Fallen bough");

    // Overgrowth sits on the estate outside the ruin's footprint; salvage lies in and around the
    // ruin, never in the standing room, with one pile a few steps from its door. Thin bramble chokes
    // the gap outside the fallen front door, and the billhook's pile stays indoors on her side of it.
    const EstateLayout& layout = ProvisionalEstateLayout();
    const auto& boundary = layout.FindPolygon(Anchor::EstateBoundary)->points;
    const auto& manor = layout.FindPolygon(Anchor::ManorFootprint)->points;
    const Point spawn = layout.PointOr(Anchor::StandingRoomSpawn, {});
    const Point roomCentre = layout.PointOr(Anchor::StandingRoomOrigin, {});
    const Point frontDoor = EstateManorFrontDoor(layout);
    int overgrowth = 0, salvage = 0, doorway = 0, rearGap = 0, teases = 0;
    double nearestSalvage = 1e9;
    for (const auto& placement : ProvisionalEstatePlacements().placements)
    {
        if (placement.id < 510000 || placement.id >= 530000) continue;
        const bool isSalvage = placement.kind == ResourceKind::SalvagePile;
        CHECK(PointInPolygon(boundary, placement.position) && (isSalvage || !PointInPolygon(manor, placement.position)));
        const auto* info = FindOvergrowth(placement.kind);
        overgrowth += info && !isSalvage;
        salvage += isSalvage;
        if (isSalvage)
        {
            CHECK(std::abs(placement.position.x - roomCentre.x) > 300.0 || std::abs(placement.position.y - roomCentre.y) > 300.0);
            nearestSalvage = std::min(nearestSalvage, std::hypot(placement.position.x - spawn.x, placement.position.y - spawn.y));
        }
        doorway += placement.kind == ResourceKind::BrambleThin && placement.position.x < frontDoor.x
            && std::hypot(placement.position.x - frontDoor.x, placement.position.y - frontDoor.y) < 900.0;
        rearGap += placement.kind == ResourceKind::BrambleThin && placement.position.x > -24100.0
            && std::hypot(placement.position.x + 24100.0, placement.position.y + 65150.0) < 400.0;
        if (placement.id == 520001) CHECK(PointInPolygon(manor, placement.position));
        teases += info && info->minTier > ToolTier::Worn;
    }
    CHECK(overgrowth >= 60 && salvage == 5 && doorway >= 5 && rearGap >= 2 && teases >= 4);
    CHECK(nearestSalvage < 700.0);
    Simulation estate;
    OK(estate.NewEstateGame(layout, ProvisionalEstatePlacements()));
}

void HaftingBootstrapAndClearing()
{
    const Point at = ProvisionalEstateLayout().PointOr(Anchor::StandingRoomSpawn, {});
    const EstatePlacements placements = OvergrowthFixture(at);
    Simulation sim;
    OK(sim.NewEstateGame(ProvisionalEstateLayout(), placements));
    const int salvage = PlacedId(sim, ResourceKind::SalvagePile);
    const int bough = PlacedId(sim, ResourceKind::FallenBranch);
    const int bramble = PlacedId(sim, ResourceKind::BrambleThin);
    const int thicket = PlacedId(sim, ResourceKind::BrambleThicket);
    const int tease = PlacedId(sim, ResourceKind::BrambleThin, 1);
    // Bramble can't be hacked with bare hands; the salvage pile and bough come away by hand.
    UnchangedFailure(sim, [&] { return sim.Harvest(bramble, at); });
    UnchangedFailure(sim, [&] { return sim.ClearOvergrowth(bramble, Item::Count, at); });
    CHECK(sim.CanHarvest(salvage) && sim.CanHarvest(bough) && !sim.CanHarvest(bramble));
    CHECK(NextSalvageHead(sim.GetState()) == Item::RustedBillhookHead);
    UnchangedFailure(sim, [&] { return sim.Harvest(salvage, {at.x + 900, at.y}); });
    OK(sim.Harvest(salvage, at));
    CHECK(sim.Count(Item::RustedBillhookHead) == 1 && sim.Count(Item::ScrapIron) == 1);
    CHECK(PlacedNode(sim, salvage).cleared && !sim.CanHarvest(salvage));
    UnchangedFailure(sim, [&] { return sim.Harvest(salvage, at); });
    CHECK(NextSalvageHead(sim.GetState()) == Item::RustedAxeHead);
    UnchangedFailure(sim, [&] { return sim.Craft(Recipe::HaftBillhook, at); });
    OK(sim.Harvest(bough, at));
    CHECK(sim.Count(Item::Branch) == 3 && sim.Count(Item::Kindling) == 1);

    // Haft a tool: one rusted head and two branches, by hand, no knife and no station.
    const double energy = sim.GetState().energy;
    OK(sim.Craft(Recipe::HaftBillhook, at));
    CHECK(sim.Count(Item::Billhook) == 1 && sim.Count(Item::RustedBillhookHead) == 0 && sim.Count(Item::Branch) == 1);
    CHECK(Close(sim.GetState().energy, energy - Exertion::CraftEnergy));
    CHECK(sim.GetToolTier(ToolKind::Billhook) == ToolTier::Worn);
    CHECK(NextSalvageHead(sim.GetState()) == Item::RustedAxeHead);

    // The wrong tool never clears; a worn billhook clears thin bramble in one swing and spends energy once.
    OK(sim.GrantItems(Item::Hatchet, 1));
    UnchangedFailure(sim, [&] { return sim.ClearOvergrowth(bramble, Item::Hatchet, at); });
    UnchangedFailure(sim, [&] { return sim.ClearOvergrowth(bramble, Item::Scythe, at); });
    UnchangedFailure(sim, [&] { return sim.ClearOvergrowth(bramble, Item::Billhook, {at.x + 900, at.y}); });
    CHECK(sim.OvergrowthSwings(bramble) == 1);
    OK(sim.CheckOvergrowth(bramble, Item::Billhook, at));
    const double beforeClear = sim.GetState().energy;
    const auto cleared = sim.ClearOvergrowth(bramble, Item::Billhook, at);
    OK(cleared);
    CHECK(cleared.message.find("Bramble canes") != std::string::npos);
    const int canes = sim.Count(Item::BrambleCanes);
    CHECK(canes >= 2 && canes <= 3);
    CHECK(Close(sim.GetState().energy, beforeClear - FindOvergrowth(ResourceKind::BrambleThin)->energy));
    UnchangedFailure(sim, [&] { return sim.ClearOvergrowth(bramble, Item::Billhook, at); });
    CHECK(sim.Count(Item::BrambleCanes) == canes);
    CHECK(sim.FindNearestOvergrowth(at, 500, Item::Billhook) != bramble);

    // A thicket needs an iron billhook: nothing changes and no energy is spent.
    const auto gated = sim.CheckOvergrowth(thicket, Item::Billhook, at);
    CHECK(!gated.ok && gated.code == ResultCode::ToolTier && gated.message == "Needs an iron billhook");
    UnchangedFailure(sim, [&] { return sim.ClearOvergrowth(thicket, Item::Billhook, at); });
    CHECK(sim.ClearOvergrowth(thicket, Item::Billhook, at).code == ResultCode::ToolTier);
    // The placement can ask for more than its kind: this thin bramble is authored at iron.
    CHECK(sim.CheckOvergrowth(tease, Item::Billhook, {at.x + 200, at.y}).message == "Needs an iron billhook");

    // Cleared state persists by stable placement id; the tool tier rides in the save's tools section.
    OK(sim.SetToolTier(ToolKind::Billhook, ToolTier::Iron));
    CHECK(sim.OvergrowthSwings(thicket) == 2);
    CHECK(sim.OvergrowthCost(thicket) < FindOvergrowth(ResourceKind::BrambleThicket)->energy);
    const std::string saved = sim.Serialize();
    CHECK(saved.find("\ntools ") != std::string::npos);
    Simulation loaded;
    loaded.SetPlacements(placements);
    OK(loaded.Deserialize(saved));
    CHECK(loaded.Serialize() == saved);
    CHECK(PlacedNode(loaded, bramble).cleared && !PlacedNode(loaded, thicket).cleared);
    CHECK(loaded.GetToolTier(ToolKind::Billhook) == ToolTier::Iron && loaded.GetToolTier(ToolKind::Axe) == ToolTier::Worn);
    OK(loaded.ClearOvergrowth(thicket, Item::Billhook, at));
    CHECK(loaded.Count(Item::BrambleCanes) >= canes + 4);
    Simulation reloaded;
    reloaded.SetPlacements(placements);
    OK(reloaded.Deserialize(loaded.Serialize()));
    CHECK(PlacedNode(reloaded, thicket).cleared);

    // A save without the tools section (an earlier build of this version) loads with worn tools.
    const std::string payload = saved.substr(saved.find('\n') + 1);
    const std::string withoutTools = payload.substr(0, payload.find("tools "));
    Simulation older;
    older.SetPlacements(placements);
    OK(older.Deserialize(Envelope(withoutTools)));
    CHECK(older.GetToolTier(ToolKind::Billhook) == ToolTier::Worn);
    Simulation rejected;
    rejected.SetPlacements(placements);
    CHECK(!rejected.Deserialize(Envelope(withoutTools + "tools 6 0 0 0 0 9 0\n")));
    CHECK(!rejected.Deserialize(Envelope(withoutTools + "shovels 1 0\n")));
    OK(rejected.Deserialize(Envelope(withoutTools + "tools 8 0 0 0 0 1 0 3 3\n")));
    CHECK(rejected.GetToolTier(ToolKind::Billhook) == ToolTier::Iron);
}

void MultiSwingTiersAndCapacity()
{
    const Point at = ProvisionalEstateLayout().PointOr(Anchor::StandingRoomSpawn, {});
    const EstatePlacements placements = OvergrowthFixture(at);
    Simulation sim;
    OK(sim.NewEstateGame(ProvisionalEstateLayout(), placements));
    const int small = PlacedId(sim, ResourceKind::StumpSmall);
    const int large = PlacedId(sim, ResourceKind::StumpLarge);
    const int rubble = PlacedId(sim, ResourceKind::Rubble);
    const int boulder = PlacedId(sim, ResourceKind::Boulder);
    OK(sim.GrantItems(Item::Hatchet, 1));
    OK(sim.GrantItems(Item::Pickaxe, 1));
    // Worn axe on a large stump: unchanged, no energy, and the prompt names the upgrade.
    const auto gated = sim.CheckOvergrowth(large, Item::Hatchet, at);
    CHECK(gated.code == ResultCode::ToolTier && gated.message == "Needs an iron axe");
    UnchangedFailure(sim, [&] { return sim.ClearOvergrowth(large, Item::Hatchet, at); });
    CHECK(sim.CheckOvergrowth(boulder, Item::Pickaxe, at).message == "Needs an iron pickaxe");
    // A small stump takes three worn swings; each swing checks, only the last commits, yielding once.
    CHECK(sim.OvergrowthSwings(small) == 3);
    const std::string before = sim.Serialize();
    for (int swing = 1; swing < sim.OvergrowthSwings(small); ++swing) OK(sim.CheckOvergrowth(small, Item::Hatchet, at));
    CHECK(sim.Serialize() == before);
    OK(sim.ClearOvergrowth(small, Item::Hatchet, at));
    const int firewood = sim.Count(Item::Firewood);
    CHECK(firewood >= 2 && firewood <= 3 && sim.Count(Item::Kindling) == 1);
    UnchangedFailure(sim, [&] { return sim.ClearOvergrowth(small, Item::Hatchet, at); });
    CHECK(sim.Count(Item::Firewood) == firewood);
    // Rubble breaks into stone, with scrap yields decided by the node, not by chance on each try.
    OK(sim.ClearOvergrowth(rubble, Item::Pickaxe, at));
    CHECK(sim.Count(Item::Stone) >= 2 && sim.Count(Item::Stone) <= 3);
    // An iron axe takes a large stump in four swings.
    OK(sim.SetToolTier(ToolKind::Axe, ToolTier::Iron));
    CHECK(sim.OvergrowthSwings(large) == 4);
    // Too tired: refused before the reserve, unchanged.
    Simulation tired;
    OK(tired.NewEstateGame(ProvisionalEstateLayout(), placements));
    OK(tired.GrantItems(Item::Hatchet, 1));
    OK(tired.SetToolTier(ToolKind::Axe, ToolTier::Iron));
    while (tired.GetState().energy > 10.0) OK(tired.SpendSprintEnergy(10));
    // Sprint stops at 10 energy, above a large stump's reserve line, so let the hours wear her down.
    while (tired.GetState().energy - tired.OvergrowthCost(large) >= Exertion::Reserve)
        tired.AdvanceGameHours(0.5, at);
    UnchangedFailure(tired, [&] { return tired.ClearOvergrowth(large, Item::Hatchet, at); });
    CHECK(tired.CheckOvergrowth(large, Item::Hatchet, at).message.find("exhausted") != std::string::npos);

    // A full pack still clears; what doesn't fit lies on the ground as one pickable drop.
    Simulation full;
    OK(full.NewEstateGame(ProvisionalEstateLayout(), placements));
    OK(full.GrantItems(Item::Pickaxe, 1));
    OK(full.GrantItems(Item::Stone, InventoryCapacity - full.UsedCapacity()));
    CHECK(full.UsedCapacity() == InventoryCapacity);
    const auto overflow = full.ClearOvergrowth(PlacedId(full, ResourceKind::Rubble), Item::Pickaxe, at);
    OK(overflow);
    CHECK(overflow.message.find("on the ground") != std::string::npos);
    CHECK(full.UsedCapacity() == InventoryCapacity && !full.GetState().worldDrops.empty());
    int dropped = 0;
    for (const auto& drop : full.GetState().worldDrops) if (drop.item == Item::Stone) dropped += drop.quantity;
    CHECK(dropped >= 2 && dropped <= 3);
    Simulation restored;
    restored.SetPlacements(placements);
    OK(restored.Deserialize(full.Serialize()));
    CHECK(restored.GetState().worldDrops.size() == full.GetState().worldDrops.size());
}

void SalvageOrderAndScytheArc()
{
    const Point at = ProvisionalEstateLayout().PointOr(Anchor::StandingRoomSpawn, {});
    EstatePlacements placements;
    placements.bakeVersion = 8;
    int next = EstatePlacementIdBase + 20000;
    for (int i = 0; i < 6; ++i) placements.placements.push_back({next++, ResourceKind::SalvagePile, {at.x + i * 50.0, at.y}, 0, 0, 1, 0});
    // A grass patch ahead of her (+X) and one tuft behind.
    for (int row = -2; row <= 2; ++row)
        for (int column = 1; column <= 3; ++column)
            placements.placements.push_back({next++, ResourceKind::TallGrass, {at.x + 1000 + column * 50.0, at.y + row * 50.0}, 0, 0, 1, 0});
    placements.placements.push_back({next++, ResourceKind::TallGrass, {at.x + 900, at.y}, 0, 0, 1, 0});
    placements.placements.push_back({next++, ResourceKind::Weeds, {at.x + 1100, at.y + 20}, 0, 0, 1, 0});
    Simulation sim;
    OK(sim.NewEstateGame(ProvisionalEstateLayout(), placements));
    // Each pile gives the next missing head, billhook first, then only scrap.
    const Item order[] = {Item::RustedBillhookHead, Item::RustedAxeHead, Item::RustedScytheBlade,
        Item::RustedPickHead, Item::RustedHoeBlade};
    for (int i = 0; i < 6; ++i)
    {
        const int pile = PlacedId(sim, ResourceKind::SalvagePile, i);
        OK(sim.Harvest(pile, PlacedNode(sim, pile).position));
        if (i < 5) CHECK(sim.Count(order[i]) == 1);
    }
    CHECK(NextSalvageHead(sim.GetState()) == Item::Count && sim.Count(Item::ScrapIron) == 6);
    for (Item head : order) CHECK(sim.Count(head) == 1);
    // A hafted tool counts as owning its head.
    OK(sim.GrantItems(Item::Branch, 2));
    OK(sim.Craft(Recipe::HaftScythe, at));
    CHECK(sim.Count(Item::Scythe) == 1 && sim.Count(Item::RustedScytheBlade) == 0 && NextSalvageHead(sim.GetState()) == Item::Count);

    // The scythe sweeps the forward arc: targets in reach ahead, never behind her.
    const Point stand{at.x + 1000, at.y};
    const auto arc = sim.ScytheArcTargets(stand, {1, 0});
    CHECK(arc.size() >= 6);
    for (int id : arc)
    {
        const auto& node = PlacedNode(sim, id);
        CHECK(node.position.x > stand.x - 1 && (node.kind == ResourceKind::TallGrass || node.kind == ResourceKind::Weeds));
        CHECK(std::hypot(node.position.x - stand.x, node.position.y - stand.y) <= Simulation::ScytheArcRadius(ToolTier::Worn));
    }
    const int behind = PlacedId(sim, ResourceKind::TallGrass, 15);
    CHECK(std::find(arc.begin(), arc.end(), behind) == arc.end());
    CHECK(sim.ScytheArcTargets(stand, {-1, 0}) == std::vector<int>{behind});
    int hay = 0;
    for (int id : arc) OK(sim.ClearOvergrowth(id, Item::Scythe, stand));
    hay = sim.Count(Item::Hay);
    CHECK(hay >= static_cast<int>(arc.size()) - 1 && sim.Count(Item::Weeds) == 1);
    // The billhook doesn't cut grass; a better scythe reaches further.
    OK(sim.GrantItems(Item::Billhook, 1));
    UnchangedFailure(sim, [&] { return sim.ClearOvergrowth(behind, Item::Billhook, stand); });
    CHECK(Simulation::ScytheArcRadius(ToolTier::Iron) > Simulation::ScytheArcRadius(ToolTier::Worn));
    CHECK(sim.ScytheArcTargets(stand, {0, 0}).empty());
}

void WeedCreepNearOvergrowth()
{
    // Out on the forecourt, clear of the standing room that a new estate game builds at the spawn.
    const Point spawn = ProvisionalEstateLayout().PointOr(Anchor::StandingRoomSpawn, {});
    const Point at{spawn.x - 1500, spawn.y + 1500};
    EstatePlacements placements;
    placements.bakeVersion = 9;
    int next = EstatePlacementIdBase + 30000;
    // A yard of grass beside a bramble, and a patch 60 m away from any other overgrowth.
    for (int row = 0; row < 8; ++row)
        for (int column = 0; column < 8; ++column)
            placements.placements.push_back({next++, ResourceKind::TallGrass, {at.x + column * 60.0, at.y + row * 60.0}, 0, 0, 1, 0});
    placements.placements.push_back({next++, ResourceKind::BrambleThicket, {at.x + 200, at.y - 150}, 0, 0, 1, 1});
    for (int column = 0; column < 8; ++column)
        placements.placements.push_back({next++, ResourceKind::Weeds, {at.x + 6000 + column * 60.0, at.y}, 0, 0, 1, 0});
    Simulation sim;
    OK(sim.NewEstateGame(ProvisionalEstateLayout(), placements));
    OK(sim.GrantItems(Item::Scythe, 1));
    OK(sim.GrantItems(Item::DiggingStick, 1));
    // Uncleared overgrowth blocks tilling on the estate.
    const ResourceNode first = sim.GetState().resources.front();
    const int gx = GardenCell(first.position.x), gy = GardenCell(first.position.y);
    UnchangedFailure(sim, [&] { return sim.Till(gx, gy, first.position); });
    for (const auto& node : std::vector<ResourceNode>(sim.GetState().resources))
        if (node.kind != ResourceKind::BrambleThicket) OK(sim.ClearOvergrowth(node.id, Item::Scythe, node.position));
    OK(sim.Till(gx, gy, first.position));
    const int tilledId = first.id;
    const auto standing = [&](bool nearYard)
    {
        int count = 0;
        for (const auto& node : sim.GetState().resources)
            if (!node.cleared && node.kind != ResourceKind::BrambleThicket && (node.position.x < at.x + 3000) == nearYard) ++count;
        return count;
    };
    CHECK(standing(true) == 0 && standing(false) == 0);
    // Several slept days: some yard grass creeps back; the far patch and the tilled square never do.
    for (int hour = 0; hour < 24 * 12; hour += 6)
    {
        while (sim.GetState().hunger < 80)
        {
            if (sim.Count(Item::Berries) == 0) OK(sim.GrantItems(Item::Berries, 4));
            OK(sim.Eat(Item::Berries));
        }
        sim.AdvanceGameHours(6, at);
        CHECK(!sim.GetState().failed);
    }
    const int regrown = standing(true);
    CHECK(regrown >= 3 && regrown <= 40);
    CHECK(standing(false) == 0);
    CHECK(PlacedNode(sim, tilledId).cleared);
    // A regrown tuft is its authored kind again, cleared and saved like any other.
    Simulation loaded;
    loaded.SetPlacements(placements);
    OK(loaded.Deserialize(sim.Serialize()));
    CHECK(loaded.Serialize() == sim.Serialize());
    int regrownId = -1;
    for (const auto& node : loaded.GetState().resources)
        if (!node.cleared && node.kind == ResourceKind::TallGrass) { regrownId = node.id; break; }
    CHECK(regrownId != -1);
    OK(loaded.ClearOvergrowth(regrownId, Item::Scythe, PlacedNode(loaded, regrownId).position));
}

void LegacyVitalsLine()
{
    // Pre-pivot saves carried warmth and the warm-outfit flag on the first line; they're dropped on load.
    Simulation sim;
    const std::string current = sim.Serialize();
    const std::string payload = current.substr(current.find('\n') + 1);
    const std::string vitals = payload.substr(0, payload.find('\n'));
    CHECK(std::count(vitals.begin(), vitals.end(), ' ') == 5);
    std::istringstream fields(vitals);
    std::string hour, minutes, hunger, energy, failed, nextId;
    fields >> hour >> minutes >> hunger >> energy >> failed >> nextId;
    const std::string rest = payload.substr(payload.find('\n'));
    Simulation legacy;
    OK(legacy.Deserialize(Envelope(hour + " " + minutes + " " + hunger + " " + energy + " 42.5 " + failed + " 1 " + nextId + rest)));
    CHECK(legacy.Serialize() == current);
    CHECK(!legacy.Deserialize(Envelope(hour + " " + minutes + " " + hunger + " " + energy + " 42.5 " + failed + " " + nextId + rest)));
    CHECK(!legacy.Deserialize(Envelope(hour + " " + minutes + " " + hunger + " " + energy + " nan " + failed + " 1 " + nextId + rest)));
}

}

int main()
{
    Run("defaults and input validation", DefaultsAndValidation);
    Run("fixed estate new game and save", FixedEstateNewGameAndSave);
    Run("taking down placed pieces refunds full cost and protects occupied floors", DeconstructRefundsAndFoundationRefusal);
    Run("taking down chests returns contents and drops overflow", DeconstructChestContentsAndOverflow);
    Run("heritage manor pieces cannot be taken down", HeritageManorPiecesCannotBeDeconstructed);
    Run("large chests and water portions do not crowd the pack", ChestCapacityAndWaterSpace);
    Run("bed sleep hour policy", BedSleepHourPolicy);
    Run("overgrowth tools, tiers and prompts", OvergrowthTableAndPrompts);
    Run("salvage, hafting and tier-gated clearing by stable id", HaftingBootstrapAndClearing);
    Run("multi-swing clears, energy reserve and full-pack yields", MultiSwingTiersAndCapacity);
    Run("salvage head order and the scythe's forward arc", SalvageOrderAndScytheArc);
    Run("daily weed creep near remaining overgrowth only", WeedCreepNearOvergrowth);
    Run("pre-pivot vitals line without warmth", LegacyVitalsLine);
    Run("playtest skip to morning", SkipToMorning);
    Run("HUD requirements and zero-time action commits", RequirementsMatchTransactions);
    Run("pure structured recipe assessment", StructuredRecipeAssessment);
    Run("default gameplay walkthrough", GameplayWalkthrough);
    Run("atomic inventory transactions", AtomicTransactions);
    Run("regrowth and persistent clearing", RegrowthAndClearing);
    Run("placement and enclosure", PlacementAndShelter);
    Run("multi-cell enclosure", MultiCellShelter);
    Run("free-standing buildings, snapping and persistence", FreeStandingBuildings);
    Run("ordinal cardinal edges and 700cm build reach", CardinalEdgesAndPlacementReach);
    Run("independent fires and chest storage", FireAndStorage);
    Run("timber processing, dual fuel, storage and save version", TimberAndFirewoodTransactions);
    Run("farming, weeds, moisture and rain", FarmingAndRain);
    Run("small garden squares, per-square planting and plot migration", GardenSquares);
    Run("berry planting, forgiving growth and recurring harvest", BerryCropCycle);
    Run("crop-kind persistence and incompatible test-save rejection", CropKindPersistenceAndVersionRejection);
    Run("clock, pause and batching consistency", ClockPauseAndBatching);
    Run("sprint consumes existing Energy with a reserve", SprintEnergyContract);
    Run("work spends Energy and time drains it slowly", ActionEnergyContract);
    Run("sleep and failure recovery without cold", SleepAndFailure);
    Run("retired fur and reeds, and cosmetic clothing", CosmeticClothingAndRetiredFur);
    Run("sleep integration and finite boundaries", SleepAndFiniteBoundaries);
    Run("strict atomic persistence", PersistenceRejection);
    Run("wardrobe defaults, truthful crafting and independent identities", WardrobeDefaultsAndCrafting);
    Run("atomic equipment, full-pack swaps and per-instance dye", AtomicEquipmentAndDye);
    Run("wardrobe storage, shared capacity and exact 280cm reach", WardrobeStorageAndReach);
    Run("persistent layout and stale transaction rejection", PersistentLayoutTransactions);
    Run("existing quantity mutations and 120 groups without slot cost", QuantityMutationReconciliation);
    Run("direct split-half and deterministic pack sort", DirectSplitAndDeterministicSort);
    Run("persistent atomic world drop transactions", PersistentWorldDropTransactions);
    Run("selected carried food groups and atomic eating", SelectedFoodGroupTransactions);
    Run("strict wardrobe ownership and current-schema save rejection", WardrobeSaveRejection);
    Run("seeded resource identity and bounded region activation", GeneratedWorldIdentityAndActivation);
    Run("permanent generated felling and persistent renewable timers", GeneratedFellingAndPersistentTimers);
    Run("cross-cell trunk footprint, chosen building sites and reload", GeneratedBuildingFootprintAndReload);
    Run("generated save validation and explicit sparse edit limit", GeneratedSaveValidationAndEditLimit);
    Run("long deterministic negative and positive chunk walk", LongDeterministicChunkWalk);
    Run("mixed distant edits, cache churn and exact reload", MixedPersistentWorldChurn);
    Run("sparse edit scale, payload bounds and atomic rejection", SparseEditScaleAndPayloadBounds);
    std::cout << cases << " scenarios, " << checks << " explicit checks passed.\n";
    return 0;
}
