#include "HomesteadSimulation.h"

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
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setprecision(17) << s.hour << ' ' << s.dayMinutes << ' ' << s.hunger << ' '
        << s.energy << ' ' << s.warmth << ' ' << s.failed << ' ' << s.warmOutfit << ' ' << s.nextId << '\n';
    for (int value : s.inventory) out << value << ' ';
    out << '\n' << s.world.seed << ' ' << s.world.generationVersion << ' ' << s.activeChunk.x << ' ' << s.activeChunk.y << '\n';
    out << s.resourceEdits.size() << '\n';
    for (const auto& edit : s.resourceEdits)
        out << edit.key.chunk.x << ' ' << edit.key.chunk.y << ' ' << edit.key.localId << ' '
            << edit.cleared << ' ' << edit.readyAtHour << '\n';
    out << s.structures.size() << '\n';
    for (const auto& p : s.structures)
    {
        out << p.id << ' ' << static_cast<int>(p.kind) << ' ' << p.cellX << ' ' << p.cellY << ' '
            << p.rotation << ' ' << p.fuelHours << ' ';
        for (int value : p.storage) out << value << ' ';
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
    if (version >= 4)
    {
        out << s.nextWearableId << ' ' << s.nextGroupId << '\n' << s.wearables.size() << '\n';
        for (const auto& w : s.wearables)
            out << w.id << ' ' << static_cast<int>(w.definition) << ' ' << w.dye << ' '
                << static_cast<int>(w.owner) << ' ' << w.chestId << '\n';
        for (int id : s.equipment) out << id << ' ';
        out << '\n' << s.inventoryLayout.size() << '\n';
        for (const auto& e : s.inventoryLayout)
            out << e.groupId << ' ' << static_cast<int>(e.item) << ' ' << e.quantity << ' ' << e.wearableId << '\n';
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
    Stock(sim, {{Item::Knife, 1}, {Item::Branch, 65}, {Item::Stone, 24}, {Item::Fiber, 25}});
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

void DefaultsAndValidation()
{
    Simulation sim;
    CHECK(sim.Count(Item::Knife) == 1);
    CHECK(sim.UsedCapacity() == 1);
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
        Stock(sim, {{Item::Knife, 1}, {Item::Hatchet, 1}, {Item::Branch, 40}, {Item::Stone, 20},
            {Item::Fiber, 20}, {Item::Roots, 10}, {Item::Flowers, 10}, {Item::Timber, 1}});
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
    const auto missing = sim.AssessRecipe(Recipe::Hatchet, Home);
    CHECK(!missing.craftable);
    CHECK(missing.output == Item::Hatchet && missing.outputCount == 1);
    CHECK(missing.retainedTool == Item::Knife && missing.retainedToolMet);
    CHECK(!missing.stationRequired && missing.stationMet && missing.capacityMet);
    CHECK(missing.ingredients.size() == 3);
    CHECK(missing.ingredients[0].item == Item::Branch);
    CHECK(std::string(missing.ingredients[0].source) == "Fallen branches");
    CHECK(missing.ingredients[1].item == Item::Stone);
    CHECK(std::string(missing.ingredients[1].source) == "Loose stones");
    CHECK(missing.ingredients[2].item == Item::Fiber);
    CHECK(std::string(missing.ingredients[2].source) == "Reeds near water");
    CHECK(sim.Serialize() == original && sim.GetRevision() == originalRevision);

    Stock(sim, {{Item::Knife, 1}, {Item::Branch, 4}, {Item::Stone, 3}, {Item::Fiber, 2}});
    const auto ready = sim.AssessRecipe(Recipe::Hatchet, Home);
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
    CHECK(std::string(firewood.ingredients[0].source) == "Mature trees with Hatchet");

    const auto invalid = sim.AssessRecipe(static_cast<Recipe>(-1), Home);
    CHECK(!invalid.craftable && invalid.output == Item::Count);

    Simulation complete;
    BuildingStock(complete);
    const Point completeFire = CellCenter(-3, -1);
    OK(complete.Place(Piece::Fire, -3, -1, 0, completeFire));
    OK(complete.AddFuel(complete.GetState().structures.back().id, completeFire));
    Stock(complete, {{Item::Knife, 1}, {Item::Hatchet, 1}, {Item::Branch, 40},
        {Item::Stone, 20}, {Item::Fiber, 20}, {Item::Roots, 10},
        {Item::Flowers, 10}, {Item::Timber, 4}});
    const Item Outputs[] = {Item::Hatchet, Item::DiggingStick, Item::WateringCan,
        Item::RoastedRoots, Item::HerbedRoots, Item::Firewood};
    const int OutputCounts[] = {1, 1, 1, 1, 1, 4};
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
    const auto failedAssessment = failed.AssessRecipe(Recipe::Hatchet, Home);
    CHECK(!failedAssessment.craftable && !failedAssessment.blocker.empty());
}

void GameplayWalkthrough()
{
    Simulation sim;
    Edit(sim, [](State&) {});
    GatherUntil(sim, Item::Branch, ResourceKind::Branches, 15);
    GatherUntil(sim, Item::Stone, ResourceKind::Stones, 8);
    GatherUntil(sim, Item::Fiber, ResourceKind::Reeds, 10);
    OK(sim.Craft(Recipe::Hatchet, Home));
    OK(sim.Craft(Recipe::DiggingStick, Home));
    OK(sim.Craft(Recipe::WateringCan, Home));
    CHECK(sim.Count(Item::Hatchet) == 1);
    auto sapling = Node(sim, ResourceKind::Sapling);
    OK(sim.Clear(sapling.id, sapling.position));
    CHECK(!sim.CanHarvest(sapling.id));
    GatherUntil(sim, Item::Roots, ResourceKind::Roots, 6);
    CHECK(sim.Count(Item::Seeds) >= 2);
    GatherUntil(sim, Item::Flowers, ResourceKind::Flowers, 1);
    GatherUntil(sim, Item::Berries, ResourceKind::BerryBush, 10);
    OK(sim.Eat(Item::Berries));
    const Point garden = CellCenter(-2, -1);
    OK(sim.Till(-2, -1, garden));
    int plotId = sim.FindNearestPlot(garden, 1);
    CHECK(plotId != -1);
    OK(sim.Plant(plotId, garden));
    OK(sim.FillWater(WaterSource));
    CHECK(sim.Count(Item::Water) == 6);
    OK(sim.Water(plotId, garden));
    GatherUntil(sim, Item::Branch, ResourceKind::Branches, 40);
    GatherUntil(sim, Item::Stone, ResourceKind::Stones, 8);
    GatherUntil(sim, Item::Fiber, ResourceKind::Reeds, 15);
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
    OK(sim.Eat(Item::HerbedRoots));
    CHECK(sim.GetState().energy < 100);
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
    UnchangedFailure(sim, [&] { return sim.Craft(Recipe::Hatchet, Home); });
    UnchangedFailure(sim, [&] { return sim.Place(Piece::Foundation, -3, 0, 0, Home); });
    UnchangedFailure(sim, [&] { return sim.FillWater(WaterSource); });
    Stock(sim, {{Item::WateringCan, 1}, {Item::Water, 1}, {Item::Stone, 117}});
    UnchangedFailure(sim, [&] { return sim.FillWater(WaterSource); });
    Stock(sim, {{Item::Knife, 1}, {Item::WateringCan, 1}, {Item::Branch, 112}});
    OK(sim.FillWater(WaterSource));
    CHECK(sim.UsedCapacity() == 120);
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
    Stock(loaded, {{Item::Knife, 1}});
    ResourceNode sapling;
    for (const auto& node : loaded.GetState().resources)
        if (node.kind == ResourceKind::Sapling) { sapling = node; break; }
    CHECK(!loaded.CanHarvest(sapling.id));
    UnchangedFailure(loaded, [&] { return loaded.Clear(sapling.id, sapling.position); });
    Stock(loaded, {{Item::Hatchet, 1}});
    CHECK(loaded.FindNearestResource(branch.position, 100) == -1);
    OK(loaded.Clear(sapling.id, sapling.position));
    CHECK(loaded.Count(Item::Branch) == 8);
    CHECK(loaded.Count(Item::Fiber) == 2);
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
    UnchangedFailure(sim, [&] { return sim.Till(-3, 0, Home); });
    OK(sim.Till(-2, -1, CellCenter(-2, -1)));
    UnchangedFailure(sim, [&] { return sim.Till(-2, -1, CellCenter(-2, -1)); });
    UnchangedFailure(sim, [&] { return sim.Place(Piece::Fire, -2, -1, 0, CellCenter(-2, -1)); });
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
        state.structures[2].storage[static_cast<int>(Item::Branch)] = 120;
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
    static_assert(ItemCount == 16, "Two processing materials are present");
    static_assert(static_cast<int>(Recipe::HerbedRoots) == 4, "Existing recipe IDs are unchanged");
    static_assert(static_cast<int>(Recipe::SplitFirewood) == 5, "Split Firewood appends after existing recipes");
    static_assert(static_cast<int>(Recipe::Count) == 6, "One processing recipe is present");

    Simulation sim;
    BuildingStock(sim);
    const Point firePosition = CellCenter(-3, -1);
    const Point chestPosition = CellCenter(-4, 0);
    OK(sim.Place(Piece::Fire, -3, -1, 0, firePosition));
    OK(sim.Place(Piece::Chest, -4, 0, 0, chestPosition));
    const int fire = StructureId(sim, Piece::Fire, firePosition);
    const int chest = StructureId(sim, Piece::Chest, chestPosition);

    CHECK(std::string(RecipeRequirements(Recipe::SplitFirewood)) ==
        "1 Timber; crude hatchet required");
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
    CHECK(!noHatchet && noHatchet.message.find("hatchet") != std::string::npos);
    Stock(sim, {{Item::Hatchet, 1}});
    const auto noTimber = sim.Craft(Recipe::SplitFirewood, Home);
    CHECK(!noTimber && noTimber.message.find("Timber") != std::string::npos);
    Stock(sim, {{Item::Hatchet, 1}, {Item::Timber, 1}, {Item::Stone, 118}});
    UnchangedFailure(sim, [&] { return sim.Craft(Recipe::SplitFirewood, Home); });

    Simulation current;
    const auto before = current.Serialize();
    CHECK(current.Deserialize(Encode(current.GetState(), SimulationSaveVersion - 1)).code ==
        ResultCode::UnsupportedVersion);
    CHECK(current.Serialize() == before);
}

void FarmingAndRain()
{
    Simulation sim;
    const Point garden = CellCenter(-2, -1);
    Stock(sim, {{Item::Seeds, 3}, {Item::DiggingStick, 1}, {Item::WateringCan, 1}});
    OK(sim.Till(-2, -1, garden));
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
    OK(sim.Till(-3, -1, berries));
    OK(sim.Till(-2, -1, roots));
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
    OK(sim.Till(-2, -1, garden));
    OK(sim.Till(-1, -1, CellCenter(-1, -1)));
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
    CHECK(expected.rfind("HOMESTEAD 6 ", 0) == 0);
    const std::string legacy = Encode(sim.GetState(), 2);
    Simulation migrated;
    const auto initial = migrated.Serialize();
    CHECK(migrated.Deserialize(legacy).code == ResultCode::UnsupportedVersion);
    CHECK(migrated.Serialize() == initial);
    CHECK(migrated.Deserialize(Encode(sim.GetState(), 3)).code == ResultCode::UnsupportedVersion);
    CHECK(migrated.Serialize() == initial);
    CHECK(migrated.Deserialize(Encode(sim.GetState(), 5)).code == ResultCode::UnsupportedVersion);
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
    reject(Envelope(payload, 7));
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
    OK(large.Till(-2, -1, CellCenter(-2, -1)));
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
    CHECK(Close(large.GetState().warmth, small.GetState().warmth, 0.03));
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

void WarmthSleepAndFailure()
{
    Simulation bare, clothed, indoor, fire;
    Edit(bare, [](State& state) { state.hour = 19; state.warmth = 80; });
    OK(clothed.Deserialize(bare.Serialize()));
    clothed.SetWarmOutfit(true);
    OK(indoor.Deserialize(bare.Serialize()));
    BuildingStock(indoor);
    BuildRoom(indoor);
    OK(fire.Deserialize(bare.Serialize()));
    BuildingStock(fire);
    OK(fire.Place(Piece::Fire, -3, 0, 0, Home));
    OK(fire.AddFuel(fire.GetState().structures[0].id, Home));
    bare.AdvanceGameHours(3, Home);
    clothed.AdvanceGameHours(3, Home);
    indoor.AdvanceGameHours(3, Home);
    fire.AdvanceGameHours(3, Home);
    CHECK(bare.GetState().warmth < clothed.GetState().warmth);
    CHECK(clothed.GetState().warmth < indoor.GetState().warmth);
    CHECK(fire.GetState().warmth > indoor.GetState().warmth);
    CHECK(Close(bare.GetState().warmth, 62));
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
    bare.SetWarmOutfit(true);
    CHECK(failed == bare.Serialize());
    UnchangedFailure(bare, [&] { return bare.Eat(Item::Berries); });
    UnchangedFailure(bare, [&] { return bare.SetDayMinutes(30); });
    UnchangedFailure(bare, [&] { return bare.Sleep(1, Home); });
    UnchangedFailure(bare, [&] { return bare.Craft(Recipe::Hatchet, Home); });
    Simulation savedFailure;
    OK(savedFailure.Deserialize(failed));
    CHECK(savedFailure.GetState().failed);
    OK(bare.Deserialize(checkpoint));
    CHECK(!bare.GetState().failed);
    Edit(bare, [](State& state) { state.hunger = 80; state.warmth = 3; state.energy = 100; });
    CHECK(!bare.Sleep(8, Home));
    CHECK(bare.GetState().warmth == 0);
    CHECK(Close(bare.GetState().hour, 19.5));
    Simulation tired;
    Edit(tired, [](State& state) { state.energy = 1.2; });
    tired.AdvanceGameHours(10, Home);
    CHECK(tired.GetState().failed);
    CHECK(tired.GetState().energy == 0);
    CHECK(Close(tired.GetState().hour, 6.5));
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
    OK(once.Till(-2, -1, CellCenter(-2, -1)));
    OK(once.Plant(once.GetState().plots[0].id, CellCenter(-2, -1)));
    CHECK(once.Count(Item::Seeds) == 0);
    Edit(once, [](State& state) { state.hour = 31; state.energy = 15; state.warmth = 50; });
    Simulation split;
    OK(split.Deserialize(once.Serialize()));
    OK(once.Sleep(8, Home));
    for (int i = 0; i < 16; ++i) OK(split.Sleep(0.5, Home));
    CHECK(Close(once.GetState().hour, split.GetState().hour, 1e-7));
    CHECK(Close(once.GetState().hunger, split.GetState().hunger, 1e-7));
    CHECK(Close(once.GetState().energy, split.GetState().energy, 1e-7));
    CHECK(Close(once.GetState().warmth, split.GetState().warmth, 0.01));
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
    UnchangedFailure(once, [&] { return once.Till(-1, -1, CellCenter(-1, -1)); });
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
    OK(sim.Till(-2, -1, CellCenter(-2, -1)));
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
    reject(Envelope(payload, 7));
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
        [](State& s) { s.warmth = std::numeric_limits<double>::infinity(); },
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
        [](State& s) { s.plots[0].cellX = 3334; },
        [](State& s) { s.plots[0].cellX = -3; s.plots[0].cellY = 0; },
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
        if (piece.kind == Piece::Chest) CHECK(loaded.ChestUsedCapacity(piece.id) <= InventoryCapacity);
}
void WardrobeDefaultsAndCrafting()
{
    static_assert(static_cast<int>(Item::HerbedRoots) == 13, "Existing fungible save IDs are unchanged");
    static_assert(static_cast<int>(Recipe::HerbedRoots) == 4, "Existing recipe IDs are unchanged");
    Simulation sim;
    CHECK(sim.GetState().wearables.size() == 2);
    CHECK(sim.GetState().equipment == (std::array<int, 4>{1, 1, 0, 2}));
    CHECK(sim.GetWearable(1)->definition == WearableDefinition::LinenTunic);
    CHECK(sim.GetWearable(2)->definition == WearableDefinition::LeatherShoes);
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
    CHECK(sim.GetState().equipment == (std::array<int, 4>{tunic, tunic, apron, 2}));
    CHECK(sim.GetWearable(1)->owner == WearableOwner::Carried);
    UnchangedFailure(sim, [&] { return sim.EquipWearable(tunic, revision); });
    UnchangedFailure(sim, [&] { return sim.UnequipWearable(2, sim.GetRevision()); });
    Stock(sim, {{Item::Knife, 1}, {Item::Branch, 117}});
    CHECK(sim.UsedCapacity() == 119);
    UnchangedFailure(sim, [&] { return sim.UnequipWearable(tunic, sim.GetRevision()); });
    Stock(sim, {{Item::Knife, 1}, {Item::Branch, 116}});
    OK(sim.UnequipWearable(tunic, sim.GetRevision()));
    CHECK(sim.UsedCapacity() == 120);
    CHECK(sim.GetState().equipment == (std::array<int, 4>{0, 0, 0, 2}));
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
    sim.SetWarmOutfit(true);
    const auto before = sim.GetState();
    OK(sim.RecolorWearable(1, 3, Home, sim.GetRevision()));
    CHECK(sim.GetState().warmOutfit && sim.GetState().warmth == before.warmth);
    CHECK(sim.GetState().hunger == before.hunger && sim.GetState().energy == before.energy && sim.GetState().hour == before.hour);
    CHECK(sim.GetWearable(tunic)->dye == 1);
    InventoryRoundTrip(sim);
}
void WardrobeStorageAndReach()
{
    Simulation sim;
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
    CHECK(sim.ChestUsedCapacity(chest) == 120);
    UnchangedFailure(sim, [&] { return sim.Transfer(chest, Item::Knife, 1, edge); });
    OK(sim.UnequipWearable(2, sim.GetRevision()));
    UnchangedFailure(sim, [&] { return sim.MoveWearable(2, chest, Home, sim.GetRevision()); });
    Stock(sim, {{Item::Knife, 1}, {Item::Stone, 118}});
    CHECK(sim.UsedCapacity() == 120);
    UnchangedFailure(sim, [&] { return sim.MoveWearable(tunic, 0, edge, sim.GetRevision()); });
    UnchangedFailure(sim, [&] { return sim.Transfer(chest, Item::Branch, -1, edge); });
    OK(sim.EquipWearable(2, sim.GetRevision()));
    OK(sim.MoveWearable(tunic, 0, edge, sim.GetRevision()));
    CHECK(sim.UsedCapacity() == 120 && sim.ChestUsedCapacity(chest) == 119);
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
    Stock(sim, {{Item::Knife, 1}, {Item::Berries, 8}, {Item::Fiber, 20}, {Item::Branch, 20}, {Item::Stone, 12}});
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
    OK(sim.Craft(Recipe::DiggingStick, Home));
    OK(sim.Craft(Recipe::WateringCan, Home));
    OK(sim.FillWater(WaterSource));
    const Point garden = CellCenter(-2, -1);
    OK(sim.Till(-2, -1, garden));
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
    BuildingStock(sim);
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
    full.structures.back().storage[static_cast<int>(Item::Branch)] = 120;
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
    for (int version : {1, 2, 3, 4, 5, 7, 999})
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
    CHECK(sim.Count(Item::Knife) == 1 && sim.UsedCapacity() == 1);
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
    for (bool present : kinds) CHECK(present);
    const auto original = sim.Serialize();
    const auto originalNodes = sim.GetState().resources;
    const auto revision = sim.GetRevision();
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
    const auto sapling = WorldNode(sim, ResourceKind::Sapling);
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
    CHECK(!resolved.cleared && resolved.readyAtHour == hour + 168);
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
    const int branchesBeforeClear = loaded.Count(Item::Branch);
    OK(loaded.Clear(resolved.id, resolved.position));
    CHECK(loaded.Count(Item::Branch) == branchesBeforeClear + 8);
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
        if (blocks) OK(sim.Clear(node.id, node.position));
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
    UnchangedFailure(sim, [&] { return sim.Till(cellX, cellY, site); });
    OK(sim.SetActiveWorldRegion({12000, -12000}));
    UnchangedFailure(sim, [&] { return sim.Place(Piece::Foundation, cellX, cellY, 0, site); });
    OK(sim.SetActiveWorldRegion(tree.position));
    FellFixtureCell(sim, cellX, cellY);
    OK(sim.Place(Piece::Foundation, cellX, cellY, 0, site));
    const int foundation = sim.GetState().structures.back().id;
    CHECK(foundation > 0 && foundation < TransientResourceIdBase);
    const int plotX = cellX + 2;
    FellFixtureCell(sim, plotX, cellY);
    OK(sim.Till(plotX, cellY, CellCenter(plotX, cellY)));
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
    OK(sim.Till(plotX, cellY, CellCenter(plotX, cellY)));
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
                WorldStock(sim, {{Item::Knife, 1}, {Item::Hatchet, 1}});
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

void Run(const char* name, void (*test)())
{
    test();
    ++cases;
    std::cout << "PASS " << name << '\n';
}
}

int main()
{
    Run("defaults and input validation", DefaultsAndValidation);
    Run("HUD requirements and zero-time action commits", RequirementsMatchTransactions);
    Run("pure structured recipe assessment", StructuredRecipeAssessment);
    Run("default gameplay walkthrough", GameplayWalkthrough);
    Run("atomic inventory transactions", AtomicTransactions);
    Run("regrowth and persistent clearing", RegrowthAndClearing);
    Run("placement and enclosure", PlacementAndShelter);
    Run("multi-cell enclosure", MultiCellShelter);
    Run("ordinal cardinal edges and 700cm build reach", CardinalEdgesAndPlacementReach);
    Run("independent fires and chest storage", FireAndStorage);
    Run("timber processing, dual fuel, storage and save version", TimberAndFirewoodTransactions);
    Run("farming, weeds, moisture and rain", FarmingAndRain);
    Run("berry planting, forgiving growth and recurring harvest", BerryCropCycle);
    Run("crop-kind persistence and incompatible test-save rejection", CropKindPersistenceAndVersionRejection);
    Run("clock, pause and batching consistency", ClockPauseAndBatching);
    Run("warmth, sleep and failure recovery", WarmthSleepAndFailure);
    Run("sleep integration and finite boundaries", SleepAndFiniteBoundaries);
    Run("strict atomic persistence", PersistenceRejection);
    Run("wardrobe defaults, truthful crafting and independent identities", WardrobeDefaultsAndCrafting);
    Run("atomic equipment, full-pack swaps and per-instance dye", AtomicEquipmentAndDye);
    Run("wardrobe storage, shared capacity and exact 280cm reach", WardrobeStorageAndReach);
    Run("persistent layout and stale transaction rejection", PersistentLayoutTransactions);
    Run("existing quantity mutations and 120 groups without slot cost", QuantityMutationReconciliation);
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
