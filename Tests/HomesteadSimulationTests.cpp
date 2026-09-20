#include "HomesteadSimulation.h"

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

std::string Envelope(const std::string& body, int version = 3)
{
    std::uint64_t hash = UINT64_C(14695981039346656037);
    for (unsigned char c : body) { hash ^= c; hash *= UINT64_C(1099511628211); }
    return "HOMESTEAD " + std::to_string(version) + " " + std::to_string(body.size()) + " " +
        std::to_string(hash) + "\n" + body;
}
// Independent fixture writer intentionally permits invalid states so parser validation is exercised.
std::string Encode(const State& s, int version = 3)
{
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setprecision(17) << s.hour << ' ' << s.dayMinutes << ' ' << s.hunger << ' '
        << s.energy << ' ' << s.warmth << ' ' << s.failed << ' ' << s.warmOutfit << ' ' << s.nextId << '\n';
    for (int value : s.inventory) out << value << ' ';
    out << '\n' << s.resources.size() << '\n';
    for (const auto& n : s.resources)
        out << n.id << ' ' << static_cast<int>(n.kind) << ' ' << n.position.x << ' ' << n.position.y
            << ' ' << n.readyAtHour << ' ' << n.cleared << '\n';
    out << s.structures.size() << '\n';
    for (const auto& p : s.structures)
    {
        out << p.id << ' ' << static_cast<int>(p.kind) << ' ' << p.cellX << ' ' << p.cellY << ' '
            << p.rotation << ' ' << p.fuelHours << ' ';
        for (int value : p.storage) out << value << ' ';
        out << '\n';
    }
    out << s.plots.size() << '\n';
    for (const auto& p : s.plots)
    {
        out << p.id << ' ' << p.cellX << ' ' << p.cellY << ' ' << p.planted << ' '
            << p.growth << ' ' << p.moisture << ' ' << p.weeds;
        if (version >= 3) out << ' ' << static_cast<int>(p.kind);
        out << '\n';
    }
    return Envelope(out.str(), version);
}
void Edit(Simulation& sim, const std::function<void(State&)>& edit)
{
    State state = sim.GetState();
    edit(state);
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
    const Point center = CellCenter(x, y);
    OK(sim.Place(Piece::Foundation, x, y, 0, center));
    OK(sim.Place(Piece::Roof, x, y, 0, center));
    OK(sim.Place(Piece::Doorway, x, y, 0, center));
    for (int rotation : {1, 2, 3}) OK(sim.Place(Piece::Wall, x, y, rotation, center));
}
void UnchangedFailure(Simulation& sim, const std::function<Result()>& action)
{
    const std::string before = sim.Serialize();
    CHECK(!action().ok);
    CHECK(sim.Serialize() == before);
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
    CHECK(sim.Count(static_cast<Item>(-1)) == 0);
    CHECK(std::string(RecipeName(static_cast<Recipe>(500))) == "Unknown recipe");
    CHECK(Close(StreamX(0), 1500));
    CHECK(IsNearWater({1680, 0}));
    CHECK(!IsNearWater({1681, 0}));
    CHECK(Close(CellCenter(-1, -1).x, -150));
    CHECK(sim.FindNearestResource({-1000, 0}, 300) != -1);
    CHECK(sim.FindNearestResource({-3900, -3900}, 100) == -1);
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
        Stock(sim, {{Item::Knife, 1}, {Item::Branch, 40}, {Item::Stone, 20},
            {Item::Fiber, 20}, {Item::Roots, 10}, {Item::Flowers, 10}});
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

void GameplayWalkthrough()
{
    Simulation sim;
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
    const double ready = sim.GetState().resources[0].readyAtHour;
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
    CHECK(sim.GetState().resources[0].cleared);
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

void CropKindPersistenceAndMigration()
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
    CHECK(expected.rfind("HOMESTEAD 3 ", 0) == 0);
    const std::string legacy = Encode(sim.GetState(), 2);
    Simulation migrated;
    OK(migrated.Deserialize(legacy));
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
    const auto kindAt = payload.find_last_of(' ');
    std::string missingKind = payload;
    missingKind.erase(kindAt, payload.size() - kindAt - 1);
    reject(Envelope(missingKind));
    std::string textKind = payload;
    textKind.replace(kindAt + 1, payload.size() - kindAt - 2, "berries");
    reject(Envelope(textKind));
    reject(Envelope(payload, 2));
    reject(Envelope(payload, 1));
    reject(Envelope(payload, 4));
    State malformedLegacy = sim.GetState();
    malformedLegacy.plots[0].growth = 1.1;
    reject(Encode(malformedLegacy, 2));
    reject(legacy.substr(0, legacy.size() - 1));

    Edit(sim, [](State& state) { state.plots[0].growth = 1; });
    OK(restored.Deserialize(Encode(sim.GetState(), 2)));
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
    Edit(once, [](State& state) { state.nextId = std::numeric_limits<int>::max() - 1; });
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
    reject(Envelope(payload, 4));
    reject(Envelope(payload + "garbage"));
    reject(Envelope(payload.substr(0, payload.size() - 8)));
    reject(std::string(1024 * 1024 + 1, 'x'));
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
        [](State& s) { s.resources[0].id = s.resources[1].id; },
        [](State& s) { s.resources[0].id = 0; },
        [](State& s) { s.resources[0].kind = static_cast<ResourceKind>(99); },
        [](State& s) { s.resources[0].position.x = 4001; },
        [](State& s) { s.resources[0].readyAtHour = -1; },
        [](State& s) { s.resources[0].readyAtHour = 1; },
        [](State& s) { s.resources[0].readyAtHour = s.hour + 10000; },
        [](State& s) { s.resources[0].cleared = true; s.resources[0].readyAtHour = 20; },
        [](State& s) { s.structures[0].id = s.resources[0].id; },
        [](State& s) { s.structures[0].kind = static_cast<Piece>(99); },
        [](State& s) { s.structures[0].cellX = -14; },
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
        [](State& s) { s.plots[0].id = s.resources[0].id; },
        [](State& s) { s.plots[0].cellX = 100; },
        [](State& s) { s.plots[0].cellX = -3; s.plots[0].cellY = 0; },
        [](State& s) { s.plots[0].growth = -0.01; },
        [](State& s) { s.plots[0].growth = 1.01; },
        [](State& s) { s.plots[0].moisture = -0.01; },
        [](State& s) { s.plots[0].weeds = 1.01; },
        [](State& s) { s.plots[0].kind = static_cast<CropKind>(-1); },
        [](State& s) { s.plots[0].kind = CropKind::Count; },
        [](State& s) { s.plots[0].planted = false; s.plots[0].growth = 0.5; },
        [](State& s) { auto p = s.plots[0]; p.id = s.nextId++; s.plots.push_back(p); },
        [](State& s) { s.resources.resize(4097); }
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
    Run("default gameplay walkthrough", GameplayWalkthrough);
    Run("atomic inventory transactions", AtomicTransactions);
    Run("regrowth and persistent clearing", RegrowthAndClearing);
    Run("placement and enclosure", PlacementAndShelter);
    Run("multi-cell enclosure", MultiCellShelter);
    Run("ordinal cardinal edges and 700cm build reach", CardinalEdgesAndPlacementReach);
    Run("independent fires and chest storage", FireAndStorage);
    Run("farming, weeds, moisture and rain", FarmingAndRain);
    Run("berry planting, forgiving growth and recurring harvest", BerryCropCycle);
    Run("crop-kind persistence and version-two migration", CropKindPersistenceAndMigration);
    Run("clock, pause and batching consistency", ClockPauseAndBatching);
    Run("warmth, sleep and failure recovery", WarmthSleepAndFailure);
    Run("sleep integration and finite boundaries", SleepAndFiniteBoundaries);
    Run("strict atomic persistence", PersistenceRejection);
    std::cout << cases << " scenarios, " << checks << " explicit checks passed.\n";
    return 0;
}
