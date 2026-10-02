#include "HomesteadSimulation.h"
#include "../Source/SurvivalGame/HomesteadSavePreference.h"
#include "HomesteadBed.h"
#include "HomesteadCrops.h"
#include "HomesteadShops.h"
#include "HomesteadEstate.h"
#include "HomesteadEstatePublicRoad.h"
#include "HomesteadGardenTarget.h"
#include "HomesteadGatherPose.h"
#include "HomesteadWeedPull.h"
#include "HomesteadManor.h"
#include "HomesteadOvergrowth.h"
#include "HomesteadRuinDebris.h"
#include "HomesteadSwingTiming.h"
#include "HomesteadToolRepeat.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>
#include <set>
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
// `width` overrides the item-stock width for version 12 on (a build with a different catalogue).
std::string Encode(const State& s, int version = SimulationSaveVersion, int width = -1)
{
    // Version 7 stocks predate the machete, and versions before 11 predate fur and the estate items.
    // Version 12 stocks are as wide as the catalogue was when it was written (40 items); version 13 on
    // writes the width first.
    const bool prefixed = version > PositionalStockSaveVersion;
    const int stockItems = prefixed ? ItemCount : version >= PositionalStockSaveVersion ? PositionalStockMinimumItems
        : version >= ClothingSaveVersion ? ItemCount
        : version >= 8 ? static_cast<int>(Item::Fur) : static_cast<int>(Item::Machete);
    const int written = width >= 0 && version >= PositionalStockSaveVersion ? width : stockItems;
    const auto count = [](const Inventory& stock, int i) { return i < ItemCount ? stock[i] : 0; };
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setprecision(17) << s.hour << ' ' << s.dayMinutes << ' ' << s.hunger << ' ' << s.energy << ' ';
    // Older saves carried warmth and the warm-outfit flag; the estate pivot removed both.
    if (version < PositionalStockSaveVersion) out << (s.failed ? 0.0 : 90.0) << ' ' << s.failed << ' ' << 0 << ' ';
    else out << s.failed << ' ';
    out << s.nextId << '\n';
    if (prefixed) out << written << ' ';
    for (int i = 0; i < written; ++i) out << count(s.inventory, i) << ' ';
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
        if (prefixed) out << written << ' ';
        for (int i = 0; i < written; ++i) out << count(p.storage, i) << ' ';
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
        if (prefixed) out << EquipmentSlotCount << ' ';
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
    if (version >= PositionalStockSaveVersion)
    {
        out << "tools " << ToolKindCount;
        for (const ToolTier tier : s.toolTiers) out << ' ' << static_cast<int>(tier);
        out << '\n';
        std::vector<int> picked, withered;
        for (const auto& p : s.plots) if (p.planted && p.picked) picked.push_back(p.id);
        for (const auto& p : s.plots) if (p.planted && p.withered) withered.push_back(p.id);
        if (!picked.empty())
        {
            out << "picked " << picked.size();
            for (int id : picked) out << ' ' << id;
            out << '\n';
        }
        if (!withered.empty())
        {
            out << "withered " << withered.size();
            for (int id : withered) out << ' ' << id;
            out << '\n';
        }
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
    // A bed takes 4 Hay now instead of 4 canes; the pack stays within its 120.
    Stock(sim, {{Item::Branch, 65}, {Item::Stone, 24}, {Item::BrambleCanes, 21}, {Item::Hay, 8}});
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
        "2 Roots + 1 Meadow herb + 1 Kindling; nearby fueled fire (no pot needed)");
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
            {Item::Roots, 10}, {Item::Flowers, 10}, {Item::Kindling, 10}, {Item::Timber, 1}});
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

// Jenny's playtest: the bedroll is a branch frame and a hay-stuffed tick, 4 Branch + 4 Hay. Walls,
// doorways, roofs and chests still take canes; beds already built and canes already carried stay as
// they are.
void BedrollTakesHay()
{
    const std::string cost = std::string("4 ") + ItemName(Item::Branch) + " + 4 " + ItemName(Item::Hay);
    CHECK(PieceRequirements(Piece::Bed) == cost);
    CHECK(std::string(PieceRequirements(Piece::Wall)).find(ItemName(Item::BrambleCanes)) != std::string::npos);
    CHECK(std::string(PieceRequirements(Piece::Chest)).find(ItemName(Item::BrambleCanes)) != std::string::npos);
    // The Build tab lists the same cost as rows (PieceCost) and the foundation as a condition.
    CHECK(PieceCost(Piece::Bed)[static_cast<int>(Item::Branch)] == 4 && PieceCost(Piece::Bed)[static_cast<int>(Item::Hay)] == 4);
    CHECK(PieceCost(Piece::Wall)[static_cast<int>(Item::BrambleCanes)] > 0);
    CHECK(PieceNeedsFoundation(Piece::Wall) && PieceNeedsFoundation(Piece::Doorway) && PieceNeedsFoundation(Piece::Roof));
    CHECK(!PieceNeedsFoundation(Piece::Bed) && !PieceNeedsFoundation(Piece::Chest) && !PieceNeedsFoundation(Piece::Foundation));
    CHECK(PieceCost(Piece::Count) == Inventory{});

    // Canes alone, or too little hay, are refused and spend nothing.
    Simulation canes;
    Stock(canes, {{Item::Branch, 4}, {Item::BrambleCanes, 4}});
    const auto noHay = canes.Place(Piece::Bed, -3, 0, 0, Home);
    CHECK(!noHay.ok && noHay.message == "Gather 4 Hay first.");
    UnchangedFailure(canes, [&] { return canes.Place(Piece::Bed, -3, 0, 0, Home); });
    Stock(canes, {{Item::Branch, 4}, {Item::Hay, 3}, {Item::BrambleCanes, 4}});
    UnchangedFailure(canes, [&] { return canes.Place(Piece::Bed, -3, 0, 0, Home); });

    // Exactly 4 Branch + 4 Hay builds it; carried canes are left alone.
    Simulation sim;
    Stock(sim, {{Item::Branch, 4}, {Item::Hay, 4}, {Item::BrambleCanes, 7}});
    OK(sim.Place(Piece::Bed, -3, 0, 0, Home));
    CHECK(sim.Count(Item::Branch) == 0 && sim.Count(Item::Hay) == 0 && sim.Count(Item::BrambleCanes) == 7);
    CHECK(sim.FindNearestStructure(Home, Piece::Bed, 300) != -1);

    // A save with a bed already standing (however it was paid for) and canes in the pack loads as is,
    // current and version-12 stocks alike: no canes are turned into hay.
    Simulation loaded;
    OK(loaded.Deserialize(sim.Serialize()));
    CHECK(loaded.FindNearestStructure(Home, Piece::Bed, 300) != -1);
    CHECK(loaded.Count(Item::BrambleCanes) == 7 && loaded.Count(Item::Hay) == 0);
    Simulation migrated;
    OK(migrated.Deserialize(Encode(sim.GetState(), PositionalStockSaveVersion)));
    CHECK(migrated.FindNearestStructure(Home, Piece::Bed, 300) != -1);
    CHECK(migrated.Count(Item::BrambleCanes) == 7 && migrated.Count(Item::Hay) == 0);

    // The manor's heritage bed is already there on a new estate; nothing about it costs hay.
    Simulation estate;
    estate.SetPlacements(ProvisionalEstatePlacements());
    OK(estate.NewEstateGame(ProvisionalEstateLayout(), ProvisionalEstatePlacements()));
    int beds = 0;
    for (const auto& piece : estate.GetState().structures) beds += piece.kind == Piece::Bed;
    CHECK(beds == 1 && estate.Count(Item::Hay) == 0);
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
    Stock(fire, {{Item::Roots, 2}, {Item::Kindling, 1}});
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
        {Item::Flowers, 10}, {Item::Kindling, 10}, {Item::Timber, 4}});
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

// The garden outline: the square the hoe and the pail act on, and whether it would work, without changing
// anything (Simulation::CheckTill/CheckWeed/CheckWater, Homestead::PreviewGarden).
void GardenTargetPreview()
{
    Simulation sim;
    Edit(sim, [](State&) {});
    for (Item head : {Item::RustedHoeBlade}) OK(sim.GrantItems(head, 1));
    GatherUntil(sim, Item::Branch, ResourceKind::Branches, 5);
    OK(sim.Craft(Recipe::HaftHoe, Home));
    const Point garden = CellCenter(-2, -1);
    const int gx = CellToGarden(-2), gy = CellToGarden(-1);
    // Stand a garden square south of the target, facing north: the hoe bites 85 cm ahead, into it.
    const Point stand{GardenCellCenter(gx, gy).x - 85.0, GardenCellCenter(gx, gy).y};
    int hx = 0, hy = 0;
    HoeCellAhead(stand, 1.0, 0.0, hx, hy);
    CHECK(hx == gx && hy == gy);
    // Near a square's edge the hoe's 85 cm and the pail's 60 cm land in different squares.
    const Point edge{GardenCellCenter(gx, gy).x - GardenCellSize * 0.5 - 70.0, GardenCellCenter(gx, gy).y};
    HoeCellAhead(edge, 1.0, 0.0, hx, hy);
    CHECK(hx == gx && GardenCell(edge.x + GardenReach::PailAheadCm) == gx - 1);

    const std::string before = sim.Serialize();
    GardenTarget hoe = PreviewGarden(sim, GardenTool::Hoe, stand, 1.0, 0.0);
    CHECK(hoe.shown && hoe.valid && hoe.plotId == -1 && hoe.cellX == gx && hoe.cellY == gy);
    for (int repeat = 0; repeat < 50; ++repeat) PreviewGarden(sim, GardenTool::Hoe, stand, 1.0, 0.0);
    CHECK(sim.Serialize() == before);                 // previewing changes nothing
    CHECK(sim.CheckTill(gx, gy, stand).ok == sim.Till(gx, gy, stand).ok);
    const int plotId = sim.FindNearestPlot(garden, 1);
    CHECK(plotId != -1);

    // Tilled: the hoe now weeds it, and a clean plot is refused with the same reason Weed gives.
    hoe = PreviewGarden(sim, GardenTool::Hoe, stand, 1.0, 0.0);
    CHECK(hoe.shown && hoe.plotId == plotId);
    const Result weed = sim.CheckWeed(plotId, stand);
    CHECK(hoe.valid == weed.ok && hoe.reason == (weed.ok ? std::string() : weed.message));
    CHECK(!sim.CheckTill(gx, gy, stand).ok && sim.CheckTill(gx, gy, stand).message == "This cell is already tilled.");

    // Blocked ground gives Till's own refusal: facing away from a square she's out of reach of, and too
    // tired to work.
    const Result farTill = sim.CheckTill(gx + 6, gy, stand);
    CHECK(!farTill.ok && farTill.message == "Move closer to a valid garden square.");
    {
        Simulation tired = sim;
        Edit(tired, [](State& state) { state.energy = 0.5; });
        const GardenTarget exhausted = PreviewGarden(tired, GardenTool::Hoe, {stand.x - 300.0, stand.y}, 1.0, 0.0);
        CHECK(exhausted.shown && !exhausted.valid && exhausted.reason.find("Too tired") != std::string::npos);
    }

    // The pail: nothing without a focused plot; no pail, then empty, then fillable, then fully watered.
    CHECK(!PreviewGarden(sim, GardenTool::Pail, stand, 1.0, 0.0).shown);
    GardenTarget pail = PreviewGarden(sim, GardenTool::Pail, garden, 1.0, 0.0, plotId);
    CHECK(pail.shown && !pail.valid && pail.reason == "Carry your pail to water crops.");
    OK(sim.GrantItems(Item::WateringCan, 1));
    pail = PreviewGarden(sim, GardenTool::Pail, garden, 1.0, 0.0, plotId);
    CHECK(!pail.valid && pail.reason == EmptyPailText);
    OK(sim.FillWater(WaterSource));
    const std::string filled = sim.Serialize();
    pail = PreviewGarden(sim, GardenTool::Pail, garden, 1.0, 0.0, plotId);
    CHECK(pail.valid && pail.reason.empty() && pail.cellX == gx && pail.cellY == gy);
    CHECK(sim.Serialize() == filled);
    OK(sim.Water(plotId, garden));
    pail = PreviewGarden(sim, GardenTool::Pail, garden, 1.0, 0.0, plotId);
    CHECK(!pail.valid && pail.reason == "This soil is already fully watered.");
    CHECK(!PreviewGarden(sim, GardenTool::None, garden, 1.0, 0.0, plotId).shown);

    // A withered crop: the hoe's outline is the plot it hoes out (the focused one first, else the one
    // ahead), judged by ClearWithered, even when it has no weeds to pull.
    {
        Simulation dead = sim;
        Edit(dead, [plotId](State& state)
        {
            for (auto& plot : state.plots)
                if (plot.id == plotId) { plot.planted = true; plot.withered = true; plot.weeds = 0.0; plot.growth = 0.5; }
        });
        const Point beside{GardenCellCenter(gx, gy).x - 60.0, GardenCellCenter(gx, gy).y};
        const GardenTarget focused = PreviewGarden(dead, GardenTool::Hoe, beside, 0.0, 1.0, plotId);
        CHECK(focused.shown && focused.plotId == plotId && focused.valid && focused.reason.empty());
        CHECK(focused.valid == dead.CheckClearWithered(plotId, beside).ok);
        const GardenTarget ahead = PreviewGarden(dead, GardenTool::Hoe, stand, 1.0, 0.0);
        CHECK(ahead.shown && ahead.plotId == plotId && ahead.valid);
        const std::string untouched = dead.Serialize();
        CHECK(dead.CheckClearWithered(plotId, beside).ok && dead.Serialize() == untouched);
        OK(dead.ClearWithered(plotId, beside));
        CHECK(!dead.CheckClearWithered(plotId, beside).ok
            && dead.CheckClearWithered(plotId, beside).message == "Nothing withered grows here.");
    }
}

// Seeds on the hotbar: the outline previews exactly the plot the [A]/[E] sow acts on (the focused plot), with
// Plant's own refusals (Simulation::CheckSow), and the focus line's cue (Homestead::DescribeSow). Previewing
// never sows or spends seed. (Main has no crop seasons or withering yet, so neither refuses.)
void SeedSowPreview()
{
    Simulation sim;
    Edit(sim, [](State&) {});
    OK(sim.GrantItems(Item::RustedHoeBlade, 1));
    GatherUntil(sim, Item::Branch, ResourceKind::Branches, 5);
    OK(sim.Craft(Recipe::HaftHoe, Home));
    const int gx = CellToGarden(-2), gy = CellToGarden(-1);
    const Point stand{GardenCellCenter(gx, gy).x - 85.0, GardenCellCenter(gx, gy).y};
    OK(sim.Till(gx, gy, stand));
    const int plotId = sim.FindNearestPlot(GardenCellCenter(gx, gy), 1);
    CHECK(plotId != -1);
    OK(sim.GrantItems(Item::CarrotSeed, 2));
    const Point beside = GardenCellCenter(gx, gy);

    // Valid: green on the focused plot, "Plant Carrot seed" keyed, and nothing changes however often it's asked.
    const std::string before = sim.Serialize();
    GardenTarget seed = PreviewGarden(sim, GardenTool::Seed, beside, 1.0, 0.0, plotId, Item::CarrotSeed);
    CHECK(seed.shown && seed.valid && seed.reason.empty() && seed.plotId == plotId && seed.cellX == gx && seed.cellY == gy);
    std::vector<Item> row(PackRowSize, Item::Count);
    row[3] = Item::CarrotSeed;
    SowCue cue = DescribeSow(sim, plotId, beside, Item::CarrotSeed, row);
    CHECK(cue.keyed && cue.text == "Plant Carrot seed");
    for (int repeat = 0; repeat < 50; ++repeat)
    {
        PreviewGarden(sim, GardenTool::Seed, beside, 1.0, 0.0, plotId, Item::CarrotSeed);
        DescribeSow(sim, plotId, beside, Item::CarrotSeed, row);
        (void)sim.CheckSow(plotId, beside, CropKind::Carrots);
    }
    CHECK(sim.Serialize() == before);

    // Invalid cases give Plant's own refusal, unkeyed; each agrees with what Plant would do on a copy.
    const auto Refused = [&](const Simulation& at, Point player, int plot, Item item, const std::string& want)
    {
        const GardenTarget target = PreviewGarden(at, GardenTool::Seed, player, 1.0, 0.0, plot, item);
        const SowCue refusal = DescribeSow(at, plot, player, item, row);
        Simulation copy = at;
        const Result planted = copy.Plant(plot, player, CropForSeed(item)->kind);
        CHECK(target.shown && !target.valid && target.reason == want);
        CHECK(!refusal.keyed && refusal.text == want);
        CHECK(!planted.ok && planted.message == want);
    };
    Refused(sim, beside, plotId, Item::BroadBeanSeed, "You have no Broad bean seed to sow.");
    // Out of its seasons (Spring 1, 1851): Plant's own season refusal, seed in hand or not.
    {
        Simulation turnips = sim;
        OK(turnips.GrantItems(Item::TurnipSeed, 1));
        Refused(turnips, beside, plotId, Item::TurnipSeed, OutOfSeasonText(CropKind::Turnips));
        CHECK(OutOfSeasonText(CropKind::Turnips) == "Turnips grow in Autumn and Winter.");
    }
    // A withered plant: hoe it out first (red, with the reason).
    {
        Simulation dead = sim;
        Edit(dead, [plotId](State& state)
        {
            for (Plot& plot : state.plots)
                if (plot.id == plotId) { plot.planted = true; plot.withered = true; plot.kind = CropKind::Potatoes; }
        });
        Refused(dead, beside, plotId, Item::CarrotSeed, "Hoe out the withered plant before planting.");
    }
    Refused(sim, {beside.x + 2000.0, beside.y}, plotId, Item::CarrotSeed, "Move beside a tilled plot to plant.");
    {
        Simulation tired = sim;
        Edit(tired, [](State& state) { state.energy = 0.5; });
        const GardenTarget exhausted = PreviewGarden(tired, GardenTool::Seed, beside, 1.0, 0.0, plotId, Item::CarrotSeed);
        CHECK(exhausted.shown && !exhausted.valid && exhausted.reason.find("Too tired") != std::string::npos);
        CHECK(exhausted.reason == tired.CheckSow(plotId, beside, CropKind::Carrots).message);
    }
    if (sim.Count(Item::Berries) == 0) Refused(sim, beside, plotId, Item::Berries, "Gather a berry to plant the seeds from its fruit.");
    if (sim.Count(Item::Seeds) == 0) Refused(sim, beside, plotId, Item::Seeds, "Gather seeds from wild roots before planting.");

    // Nothing to show without a seed, for a non-seed, or for a plot that isn't there.
    CHECK(!PreviewGarden(sim, GardenTool::Seed, beside, 1.0, 0.0, plotId, Item::Count).shown);
    CHECK(!PreviewGarden(sim, GardenTool::Seed, beside, 1.0, 0.0, plotId, Item::Stone).shown);
    CHECK(!PreviewGarden(sim, GardenTool::Seed, beside, 1.0, 0.0, 987654, Item::CarrotSeed).shown);
    CHECK(!sim.CheckSow(987654, beside, CropKind::Carrots).ok);

    // Untilled: with no plot in focus, the square the hoe would till next is red until it's tilled; a berry,
    // held more often to eat, outlines nothing there.
    const Point edge{GardenCellCenter(gx, gy).x + GardenCellSize * 0.5 - 10.0, GardenCellCenter(gx, gy).y};
    const GardenTarget untilled = PreviewGarden(sim, GardenTool::Seed, edge, 1.0, 0.0, -1, Item::CarrotSeed);
    CHECK(untilled.shown && !untilled.valid && untilled.reason == UntilledSowText && untilled.plotId == -1);
    CHECK(untilled.cellX == gx + 1 && untilled.cellY == gy);
    CHECK(!PreviewGarden(sim, GardenTool::Seed, edge, 1.0, 0.0, -1, Item::Berries).shown);
    // An out-of-season seed (turnips in Spring) doesn't ask her to till first only to be refused (review).
    {
        Simulation turnips = sim;
        OK(turnips.GrantItems(Item::TurnipSeed, 1));
        CHECK(!PreviewGarden(turnips, GardenTool::Seed, edge, 1.0, 0.0, -1, Item::TurnipSeed).shown);
    }
    // Facing back onto her own plot without it in focus shows nothing: it's tilled.
    CHECK(!PreviewGarden(sim, GardenTool::Seed, edge, -1.0, 0.0, -1, Item::CarrotSeed).shown);
    // The same square the hoe tills, wherever she stands in hers (review: the pail's 60 cm pointed at her own
    // square from 15-40 cm in, while the hoe's 85 cm tills the next one).
    for (const double into : {15.0, 25.0, 40.0, 60.0, 90.0})
    {
        const Point at{GardenCellCenter(gx + 1, gy).x - GardenCellSize * 0.5 + into, GardenCellCenter(gx + 1, gy).y};
        int hoeX = 0, hoeY = 0;
        HoeCellAhead(at, 1.0, 0.0, hoeX, hoeY);
        const GardenTarget ahead = PreviewGarden(sim, GardenTool::Seed, at, 1.0, 0.0, -1, Item::CarrotSeed);
        const GardenTarget hoe = PreviewGarden(sim, GardenTool::Hoe, at, 1.0, 0.0);
        CHECK(ahead.cellX == hoeX && ahead.cellY == hoeY && hoe.cellX == hoeX && hoe.cellY == hoeY);
        CHECK(ahead.shown == sim.CheckTillGround(hoeX, hoeY, at).ok);
    }
    // Tillable but for the hoe or her energy: still "till this square first".
    {
        Simulation unready = sim;
        Edit(unready, [](State& state)
        {
            state.energy = 0.5;
            state.inventory[static_cast<int>(Item::DiggingStick)] = 0;
            state.inventoryLayout.erase(std::remove_if(state.inventoryLayout.begin(), state.inventoryLayout.end(),
                [](const LayoutEntry& entry) { return entry.item == Item::DiggingStick; }), state.inventoryLayout.end());
        });
        CHECK(!unready.CheckTill(gx + 1, gy, edge).ok);
        CHECK(PreviewGarden(unready, GardenTool::Seed, edge, 1.0, 0.0, -1, Item::CarrotSeed).reason == UntilledSowText);
    }
    // Ground that can't be tilled (a building, a resource, spoiling overgrowth) shows nothing.
    Simulation built = sim;
    BuildingStock(built);
    OK(built.Place(Piece::Fire, -3, -1, 0, CellCenter(-3, -1)));
    int refused = 0;
    bool building = false;
    for (int y = gy - 30; y <= gy + 30; ++y)
        for (int x = gx - 30; x <= gx + 30; ++x)
        {
            const Point from{GardenCellCenter(x, y).x - 85.0, GardenCellCenter(x, y).y};
            const Result ground = built.CheckTillGround(x, y, from);
            if (ground.ok || ground.message == "Move closer to a valid garden square." || ground.message == "This cell is already tilled.") continue;
            ++refused;
            building = building || ground.message == "Choose soil away from buildings.";
            CHECK(!PreviewGarden(built, GardenTool::Seed, from, 1.0, 0.0, -1, Item::CarrotSeed).shown);
        }
    CHECK(refused > 0 && building);
    CHECK(sim.Serialize() == before);

    // No seed selected: name a seed in the hotbar row and its number key; a berry only when no seed is there.
    std::vector<Item> none(PackRowSize, Item::Count);
    CHECK(DescribeSow(sim, plotId, beside, Item::Count, none).text == "Choose seeds on the hotbar to sow");
    CHECK(!DescribeSow(sim, plotId, beside, Item::Count, row).keyed);
    CHECK(DescribeSow(sim, plotId, beside, Item::Count, row).text == "Select Carrot seed (4) to plant");
    CHECK(DescribeSow(sim, plotId, beside, Item::Hatchet, row).text == "Select Carrot seed (4) to plant");
    std::vector<Item> last(PackRowSize, Item::Count);
    last[PackRowSize - 1] = Item::CarrotSeed;
    CHECK(DescribeSow(sim, plotId, beside, Item::Count, last).text == "Select Carrot seed (0) to plant");
    std::vector<Item> spent(PackRowSize, Item::Count);
    spent[2] = Item::BroadBeanSeed;  // none in the pack: not offered
    CHECK(DescribeSow(sim, plotId, beside, Item::Count, spent).text == "Choose seeds on the hotbar to sow");
    {
        Simulation turnips = sim;
        OK(turnips.GrantItems(Item::TurnipSeed, 3));
        std::vector<Item> autumn(PackRowSize, Item::Count);
        autumn[0] = Item::TurnipSeed;   // out of season in Spring: skipped
        autumn[5] = Item::CarrotSeed;
        CHECK(DescribeSow(turnips, plotId, beside, Item::Count, autumn).text == "Select Carrot seed (6) to plant");
        autumn[5] = Item::Count;
        CHECK(DescribeSow(turnips, plotId, beside, Item::Count, autumn).text == "Choose seeds on the hotbar to sow");
    }
    {
        Simulation berried = sim;
        OK(berried.GrantItems(Item::Berries, 1));
        std::vector<Item> fruit(PackRowSize, Item::Count);
        fruit[1] = Item::Berries;
        CHECK(DescribeSow(berried, plotId, beside, Item::Count, fruit).text == "Select Berries (2) to plant their seeds");
        fruit[6] = Item::CarrotSeed;
        CHECK(DescribeSow(berried, plotId, beside, Item::Count, fruit).text == "Select Carrot seed (7) to plant");
        const SowCue berry = DescribeSow(berried, plotId, beside, Item::Berries, fruit);
        CHECK(berry.keyed && berry.text == "Plant berry seeds");
    }

    // The action sows exactly the previewed plot, spending one seed; then the plot is occupied.
    const int seedsBefore = sim.Count(Item::CarrotSeed);
    OK(sim.Plant(seed.plotId, beside, CropForSeed(Item::CarrotSeed)->kind));
    CHECK(sim.Count(Item::CarrotSeed) == seedsBefore - 1);
    for (const Plot& plot : sim.GetState().plots)
        if (plot.id == seed.plotId) CHECK(plot.planted && plot.kind == CropKind::Carrots);
    Refused(sim, beside, plotId, Item::CarrotSeed, "A crop is already growing here.");
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
    CHECK(sim.Count(Item::Water) == PailPortions);
    OK(sim.Water(plotId, garden));
    CHECK(sim.Count(Item::Water) == PailPortions - 1);   // each watering spends one
    GatherUntil(sim, Item::Branch, ResourceKind::Branches, 40);
    GatherUntil(sim, Item::Stone, ResourceKind::Stones, 8);
    OK(sim.GrantItems(Item::BrambleCanes, 15));
    BuildRoom(sim);
    CHECK(sim.IsSheltered(Home));
    // The woodland walk carries no scythe; hand her the mown hay the bedroll is stuffed with.
    OK(sim.GrantItems(Item::Hay, 4));
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
    // A pail left in the chest: the fill refusal says so, and taking it back lets her fill again.
    CHECK(sim.Count(Item::WateringCan) == 1);
    {
        OK(sim.Transfer(chestId, Item::WateringCan, 1, chestPosition));
        CHECK(sim.PailStored() && sim.Count(Item::WateringCan) == 0);
        CHECK(sim.FillWater(WaterSource).message == "Your pail is in the chest. Take it to fill it.");
        OK(sim.Transfer(chestId, Item::WateringCan, -1, chestPosition));
        CHECK(!sim.PailStored() && sim.Count(Item::WateringCan) == 1);
    }
    sim.AdvanceGameHours(4, Home);
    CHECK(sim.GetState().plots[0].growth > 0.0);
    // Weeds come up once a day (at 6 AM or on waking), never in the hours between.
    CHECK(sim.GetState().plots[0].weeds == 0.0);
    CHECK(sim.Weed(plotId, garden).message == "This plot is already free of weeds.");
    const double energyBeforeMeal = sim.GetState().energy;
    CHECK(energyBeforeMeal < 100);
    OK(sim.Eat(Item::HerbedRoots));
    // A cooked meal restores some energy as well as food.
    CHECK(sim.GetState().energy > energyBeforeMeal || sim.GetState().energy == 100);
    const double beforeSleep = sim.GetState().hour;
    OK(sim.Sleep(8, Home, {1, 0}));
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
        OK(sim.Sleep(8, Home, {1, 0}));
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
    // With no pail carried, the refusal says where to get one (and a stored pail isn't "carried").
    CHECK(!sim.PailStored());
    CHECK(sim.FillWater(WaterSource).message == "You need a pail to carry water.");
    Stock(sim, {{Item::WateringCan, 1}, {Item::Water, 1}, {Item::Stone, 117}});
    CHECK(sim.FillWater({WaterSource.x + 90000, WaterSource.y}).message == "Walk to the river or the lake to fill your pail.");
    OK(sim.FillWater(WaterSource));
    CHECK(sim.UsedCapacity() == 118);   // water takes no pack space
    CHECK(sim.Count(Item::Water) == 15);
    Stock(sim, {{Item::Knife, 1}, {Item::WateringCan, 1}, {Item::Branch, 112}});
    OK(sim.FillWater(WaterSource));
    CHECK(sim.UsedCapacity() == 114);
    CHECK(sim.Count(Item::Water) == 15);   // Jenny, 2026-09-30: a pail holds 15, up from 6
    UnchangedFailure(sim, [&] { return sim.FillWater(WaterSource); });
    CHECK(sim.FillWater(WaterSource).message == "Your pail is already full.");
    CHECK(sim.Count(Item::Water) == PailPortions);
    OK(sim.EmptyPail());
    CHECK(sim.Count(Item::Water) == 0);
    CHECK(sim.Count(Item::WateringCan) == 1);
    UnchangedFailure(sim, [&] { return sim.EmptyPail(); });
    OK(sim.FillWater(WaterSource));
    CHECK(sim.Count(Item::Water) == 15);
    // An old save's six-portion pail, part used, loads with its four portions; the next fill tops it to 15.
    Stock(sim, {{Item::Knife, 1}, {Item::WateringCan, 1}, {Item::Water, 4}});
    Simulation reloaded;
    OK(reloaded.Deserialize(sim.Serialize()));
    CHECK(reloaded.Count(Item::Water) == 4);
    OK(reloaded.FillWater(WaterSource));
    CHECK(reloaded.Count(Item::Water) == 15);
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

// Jenny's playtest: weeds in a square get an explicit [F]/[X] Pull weeds prompt whenever she can see
// them (HasVisibleWeeds, the world's first drawn tuft), on bare, growing and ripe squares alike, and
// F pulls them there: a bare weedy square is weeded, never sown; a clean one is left alone.
void PullWeedsOnAnySquare()
{
    Plot plot;
    plot.weeds = 0.0;
    CHECK(!HasVisibleWeeds(plot));
    plot.weeds = CropCare::VisibleWeeds - 0.001;
    CHECK(!HasVisibleWeeds(plot));
    plot.weeds = CropCare::VisibleWeeds;
    CHECK(HasVisibleWeeds(plot));
    plot.planted = true;
    plot.growth = 1.0;
    CHECK(IsRipe(plot) && HasVisibleWeeds(plot));

    Simulation sim;
    Stock(sim, {{Item::DiggingStick, 1}, {Item::Seeds, 2}});
    const Point square = CellCenter(-2, -1);
    OK(sim.Till(CellToGarden(-2), CellToGarden(-1), square));
    const int plotId = sim.FindNearestPlot(square, 1);
    CHECK(plotId != -1 && !sim.GetState().plots[0].planted);
    // A bare square that's grown weeds: pulled, nothing sown, no items, Energy once.
    Edit(sim, [](State& state) { state.plots[0].weeds = 0.4; }, false);
    CHECK(HasVisibleWeeds(sim.GetState().plots[0]));
    const auto stock = sim.GetState().inventory;
    const double energy = sim.GetState().energy;
    const auto bare = sim.Weed(plotId, square);
    OK(bare);
    CHECK(bare.message == "Weeds pulled. The square is clean for sowing.");
    CHECK(!sim.GetState().plots[0].planted && sim.GetState().plots[0].weeds == 0.0);
    CHECK(sim.GetState().inventory == stock && Close(sim.GetState().energy, energy - Exertion::WeedEnergy, 1e-9));
    CHECK(!HasVisibleWeeds(sim.GetState().plots[0]));
    UnchangedFailure(sim, [&] { return sim.Weed(plotId, square); });
    // A ripe crop with weeds still pulls them (the crop message) and stays ripe.
    OK(sim.Plant(plotId, square));
    Edit(sim, [](State& state) { state.plots[0].growth = 1.0; state.plots[0].weeds = 0.6; }, false);
    CHECK(IsRipe(sim.GetState().plots[0]) && HasVisibleWeeds(sim.GetState().plots[0]));
    const auto ripe = sim.Weed(plotId, square);
    OK(ripe);
    CHECK(ripe.message == "Weeds removed. The crop has more room to grow.");
    CHECK(IsRipe(sim.GetState().plots[0]) && sim.GetState().plots[0].weeds == 0.0);
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

// Jenny (2026-09-30): weeds come up once a day, so there's something to clear each morning but never
// twice a day. Awake, the pass runs at 6 AM; asleep across 6 AM, on waking; never in the hours between,
// and each calendar day gets one pass however it comes.
void DailyWeedPass()
{
    Simulation sim;
    Stock(sim, {{Item::Seeds, 1}, {Item::DiggingStick, 1}, {Item::Branch, 4}, {Item::Hay, 4}});
    OK(sim.Place(Piece::Bed, -3, 0, 0, Home));
    const Point garden = CellCenter(-2, -1);
    OK(sim.Till(CellToGarden(-2), CellToGarden(-1), garden));
    OK(sim.Plant(sim.FindNearestPlot(garden, 1), garden));
    const auto weeds = [](const Simulation& s) { return s.GetState().plots[0].weeds; };
    // Weeds come up on some plots each day (Crops::WeedsComeUp). The timing checks below run from the
    // morning before the first day this plot's weeds come up: day 0 = 06:00 on that morning, hour 30
    // (plus the shift) the 6 AM pass that brings them.
    const int plotId = sim.GetState().plots[0].id;
    int firstDay = 1;
    while (!Crops::WeedsComeUp(plotId, firstDay)) ++firstDay;
    const double shift = 24.0 * (firstDay - 1);
    // DailyWeeds for each day from rom to 	o that this plot's weeds come up, capped at fully weedy.
    const auto expected = [plotId](int from, int to, double start = 0.0) {
        double w = start;
        for (int day = from; day <= to; ++day)
            if (Crops::WeedsComeUp(plotId, day)) w = std::min(1.0, w + CropCare::DailyWeeds);
        return w;
    };
    // Fed and rested, plot clean.
    const auto set = [shift](Simulation& s, double hour, double plotWeeds = 0.0, double energy = 100.0) {
        Edit(s, [=](State& state) {
            state.hour = hour + shift;
            state.hunger = 100.0;
            state.energy = energy;
            state.plots[0].weeds = plotWeeds;
        });
    };
    CHECK(Crops::WeedDay(6.0) == 0 && Crops::WeedDay(29.99) == 0 && Crops::WeedDay(30.0) == 1);
    CHECK(firstDay < 20 && Crops::WeedDay(30.0 + shift) == firstDay);

    // 18 awake hours from noon: nothing until 6 AM, then one pass.
    Simulation awake = sim;
    set(awake, 12.0);
    awake.AdvanceGameHours(17.9, Home);
    CHECK(weeds(awake) == 0.0 && awake.GetState().hour < 30.0 + shift);
    awake.AdvanceGameHours(0.2, Home);
    CHECK(Close(weeds(awake), CropCare::DailyWeeds));
    awake.AdvanceGameHours(20.0, Home);
    CHECK(Close(weeds(awake), expected(firstDay, Crops::WeedDay(awake.GetState().hour))));

    // A night's sleep (22:00 to 06:00): exactly one pass, on waking.
    Simulation night = sim;
    set(night, 22.0);
    OK(night.Sleep(8.0, Home, {1, 0}));
    CHECK(Close(night.GetState().hour, 30.0 + shift) && Close(weeds(night), CropCare::DailyWeeds));
    // Awake for the rest of that day: no second pass.
    night.AdvanceGameHours(10.0, Home);
    CHECK(Close(weeds(night), CropCare::DailyWeeds));
    // A nap that doesn't cross 6 AM adds nothing.
    OK(night.Sleep(2.0, Home, {1, 0}));
    CHECK(Close(weeds(night), CropCare::DailyWeeds));

    // Sleeping across 6 AM (03:00 to 09:00): one pass, then nothing until the next 6 AM.
    Simulation across = sim;
    set(across, 27.0);
    OK(across.Sleep(6.0, Home, {1, 0}));
    CHECK(Close(weeds(across), CropCare::DailyWeeds));
    across.AdvanceGameHours(20.0, Home);
    CHECK(Close(weeds(across), CropCare::DailyWeeds) && across.GetState().hour < 54.0 + shift);
    // Woken before 6 AM (22:00 to 05:00): no pass on waking; it comes at 6 AM with her up.
    Simulation early = sim;
    set(early, 22.0);
    OK(early.Sleep(7.0, Home, {1, 0}));
    CHECK(weeds(early) == 0.0);
    early.AdvanceGameHours(1.5, Home);
    CHECK(Close(weeds(early), CropCare::DailyWeeds));
    // The book's long sleep is two halves; a 16 hour sleep from 22:00 still gets one pass.
    Simulation longSleep = sim;
    set(longSleep, 22.0);
    OK(longSleep.Sleep(8.0, Home, {1, 0}));
    OK(longSleep.Sleep(8.0, Home, {1, 0}));
    CHECK(Close(weeds(longSleep), CropCare::DailyWeeds));

    // Dozing off where she stands across 6 AM: one pass when she comes to.
    Simulation doze = sim;
    set(doze, 29.0, 0.0, 0.3);
    doze.AdvanceGameHours(0.6, Home);
    CHECK(doze.GetState().hour > 30.0 + shift && Close(weeds(doze), CropCare::DailyWeeds));

    // Three days without sleep: one pass each 6 AM, bringing weeds on the days they come up.
    Simulation days = sim;
    set(days, 7.0);
    for (int chunk = 0; chunk < 12; ++chunk)
    {
        Edit(days, [](State& state) { state.hunger = 100.0; state.energy = 100.0; });
        days.AdvanceGameHours(6.0, Home);
        CHECK(Close(weeds(days), expected(firstDay, Crops::WeedDay(days.GetState().hour))));
    }
    CHECK(Close(days.GetState().hour, 79.0 + shift) && Close(weeds(days), expected(firstDay, firstDay + 2)));
    // Capped at fully weedy.
    set(days, 29.5, 0.9);
    days.AdvanceGameHours(1.0, Home);
    CHECK(weeds(days) == 1.0);

    // Reload stability: an old save keeps its weeds, and a reload either side of 6 AM changes nothing.
    Simulation saved = sim;
    set(saved, 29.0, 0.37);
    CHECK(Close(weeds(saved), 0.37));
    Simulation reloaded;
    OK(reloaded.Deserialize(saved.Serialize()));
    CHECK(reloaded.Serialize() == saved.Serialize());
    saved.AdvanceGameHours(2.0, Home);
    reloaded.AdvanceGameHours(2.0, Home);
    CHECK(Close(weeds(saved), 0.37 + CropCare::DailyWeeds) && weeds(reloaded) == weeds(saved));
    Simulation afterPass;
    OK(afterPass.Deserialize(saved.Serialize()));
    afterPass.AdvanceGameHours(5.0, Home);
    saved.AdvanceGameHours(5.0, Home);
    CHECK(weeds(afterPass) == weeds(saved) && Close(weeds(afterPass), 0.37 + CropCare::DailyWeeds));
}

// Weeds come up on some plots each day, not all (Jenny, 2026-10-01), from a fixed hash of the plot and the day,
// so a multi-day pass matches passing the days one at a time and a reload never rerolls.
void SporadicDailyWeeds()
{
    const auto plots = [](int count) {
        State state;
        state.plots.clear();
        for (int i = 0; i < count; ++i)
        {
            Plot plot;
            plot.id = 100 + i;
            plot.cellX = i;
            state.plots.push_back(plot);
        }
        return state;
    };
    // 16 plots over 40 days, cleared each morning: each day some plots and not others.
    State state = plots(16);
    int ups = 0, mixedDays = 0;
    std::vector<int> perPlot(state.plots.size(), 0);
    for (int day = 1; day <= 40; ++day)
    {
        state.hour = 6.0 + 24.0 * day;
        CHECK(Crops::WeedDay(state.hour) == day);
        for (auto& plot : state.plots) plot.weeds = 0.0;
        Crops::GrowDailyWeeds(state, 1);
        int up = 0;
        for (size_t i = 0; i < state.plots.size(); ++i)
        {
            const auto& plot = state.plots[i];
            CHECK(Crops::WeedsComeUp(plot.id, day) == (plot.weeds > 0.0));
            if (plot.weeds > 0.0)
            {
                CHECK(Close(plot.weeds, CropCare::DailyWeeds));
                ++up;
                ++perPlot[i];
            }
        }
        ups += up;
        if (up > 0 && up < static_cast<int>(state.plots.size())) ++mixedDays;
    }
    const double share = ups / (16.0 * 40.0);
    CHECK(share > CropCare::DailyWeedChance - 0.1 && share < CropCare::DailyWeedChance + 0.1);
    CHECK(mixedDays >= 30);
    for (int count : perPlot) CHECK(count > 0 && count < 40);

    // One pass covering 10 days (a long sleep) matches the 10 daily passes.
    State together = plots(16), apart = plots(16);
    together.hour = 6.0 + 24.0 * 10;
    Crops::GrowDailyWeeds(together, 10);
    for (int day = 1; day <= 10; ++day)
    {
        apart.hour = 6.0 + 24.0 * day;
        Crops::GrowDailyWeeds(apart, 1);
    }
    for (size_t i = 0; i < together.plots.size(); ++i) CHECK(together.plots[i].weeds == apart.plots[i].weeds);

    // In the game: tilled squares over four days, saved and reloaded halfway, end up exactly the same.
    Simulation sim;
    Stock(sim, {{Item::DiggingStick, 1}});
    const Point garden = CellCenter(-2, -1);
    for (int x = 0; x < 4; ++x) OK(sim.Till(CellToGarden(-2) + x, CellToGarden(-1), garden));
    CHECK(sim.GetState().plots.size() == 4);
    const auto day = [](Simulation& s) {
        for (int chunk = 0; chunk < 4; ++chunk)
        {
            Edit(s, [](State& st) { st.hunger = 100.0; st.energy = 100.0; });
            s.AdvanceGameHours(6.0, Home);
        }
    };
    Edit(sim, [](State& st) { st.hour = 7.0; for (auto& plot : st.plots) plot.weeds = 0.0; });
    day(sim);
    day(sim);
    Simulation reloaded;
    OK(reloaded.Deserialize(sim.Serialize()));
    day(sim);
    day(sim);
    day(reloaded);
    day(reloaded);
    CHECK(reloaded.Serialize() == sim.Serialize());
    const int lastDay = Crops::WeedDay(sim.GetState().hour);
    CHECK(lastDay == 4);
    for (const auto& plot : sim.GetState().plots)
    {
        double want = 0.0;
        for (int d = 1; d <= lastDay; ++d)
            if (Crops::WeedsComeUp(plot.id, d)) want = std::min(1.0, want + CropCare::DailyWeeds);
        CHECK(Close(plot.weeds, want));
    }
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
    CHECK(sim.Water(id, garden).message == EmptyPailText);
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
    CHECK(sim.GetState().plots[0].weeds == 0); // no weeds until the next 6 AM pass (DailyWeedPass)
    CHECK(dry.GetState().plots[0].planted);
    CHECK(dry.GetState().plots[0].growth > 0);
    CHECK(sim.GetState().plots[0].growth > dry.GetState().plots[0].growth);
    CHECK(sim.GetState().plots[0].growth > weedy.GetState().plots[0].growth);
    const double growth = weedy.GetState().plots[0].growth;
    OK(weedy.Weed(id, garden));
    CHECK(weedy.GetState().plots[0].growth == growth);
    CHECK(weedy.GetState().plots[0].weeds == 0);
    // The first spell of rain at least 2.5 h long (Simulation/HomesteadRain.h): it waters the dry plot.
    RainSpell spell;
    for (double from = 30.0; NextRainSpell(from, spell) && spell.end - spell.start < 2.5;) from = spell.end;
    CHECK(spell.end - spell.start >= 2.5);
    Edit(dry, [&spell](State& state) { state.hour = spell.start + 0.01; });
    CHECK(dry.IsRaining());
    dry.AdvanceGameHours(2, Home);
    CHECK(dry.GetState().plots[0].moisture > 0.5);
    CHECK(dry.GetState().plots[0].growth > 0);
    Edit(dry, [&spell](State& state) { state.hour = spell.end + 0.01; });
    CHECK(!dry.IsRaining());
    // Rain waters the garden at night too (Jenny, 2026-09-30): a spell that starts after dark.
    {
        RainSpell night;
        for (double from = 30.0; NextRainSpell(from, night)
            && (night.end - night.start < 2.5 || (std::fmod(night.start, 24.0) < 21.0 && std::fmod(night.start, 24.0) >= 3.0));)
            from = night.end;
        Simulation dark = dry;
        Edit(dark, [&night](State& state) { state.hour = night.start + 0.01; state.plots[0].moisture = 0.0; });
        CHECK(dark.IsRaining() && dark.IsNight());
        dark.AdvanceGameHours(2, Home);
        CHECK(dark.GetState().plots[0].moisture > 0.5);
        // A step never spans the rain's start: half an hour dry, then half an hour of rain.
        Simulation edge = dry;
        Edit(edge, [&night](State& state) { state.hour = night.start - 0.5; state.plots[0].moisture = 0.0; });
        edge.AdvanceGameHours(1.0, Home);
        CHECK(std::abs(edge.GetState().plots[0].moisture - 0.3 * 0.5) < 0.01);
    }
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
        OK(sim.Sleep(8, Home, {1, 0}));
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
    RainSpell shower;
    for (double from = 30.0; NextRainSpell(from, shower) && shower.end - shower.start < 2.5;) from = shower.end;
    Edit(rain, [&shower](State& state) { state.hour = shower.start; state.plots[0].moisture = 0; });
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
    CHECK(Close(sim.GetState().plots[0].growth, 1.0 - 24.0 / 42.0) && sim.GetState().plots[0].planted);
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
    CHECK(regrowing.GetState().plots[0].planted && Close(regrowing.GetState().plots[0].growth, 1.0 - 24.0 / 42.0));
    const double hungerBefore = regrowing.GetState().hunger;
    OK(regrowing.Eat(Item::Berries));
    CHECK(regrowing.GetState().hunger > hungerBefore);
}

void CropTableAndStatus()
{
    // Every crop has a table row, a whole number of days and a mesh stem.
    for (int kind = 0; kind < static_cast<int>(CropKind::Count); ++kind)
    {
        const auto& info = GetCropInfo(static_cast<CropKind>(kind));
        CHECK(static_cast<int>(info.kind) == kind);
        CHECK(CropDays(info.kind) >= 1 && std::string(info.visual).size() > 0);
        CHECK(CropForSeed(info.seed) == &info);
        CHECK(info.produceCount > 0 && info.growHours > 0);
    }
    CHECK(GetCropInfo(CropKind::Count).kind == CropKind::Count);
    CHECK(CropForSeed(Item::Stone) == nullptr);
    CHECK(CropDays(CropKind::Roots) == 2 && CropDays(CropKind::Berries) == 2);
    CHECK(CropRegrowDays(CropKind::Berries) == 1 && CropRegrowDays(CropKind::Roots) == 0);
    CHECK(ReadyInText(CropKind::Roots) == "Ready in about 2 days if watered.");
    CHECK(std::string(ItemDescription(Item::Seeds)).find("Matures in about 2 days") != std::string::npos);
    // Period crops: each seed's description states its days (and regrow days), the store sells it,
    // and its produce sells back at the store.
    const auto& storeGoods = ShopGoods(ShopKind::GeneralStore);
    for (int kind = static_cast<int>(CropKind::Turnips); kind < static_cast<int>(CropKind::Count); ++kind)
    {
        const auto& info = GetCropInfo(static_cast<CropKind>(kind));
        const std::string description = ItemDescription(info.seed);
        CHECK(description.find("in about " + std::to_string(CropDays(info.kind)) + " days") != std::string::npos);
        if (info.regrowHours > 0)
            CHECK(description.find("every " + std::to_string(CropRegrowDays(info.kind)) + " days") != std::string::npos);
        CHECK(std::find(storeGoods.begin(), storeGoods.end(), info.seed) != storeGoods.end());
        CHECK(ShopBuys(ShopKind::GeneralStore, info.produce));
        // The first harvest pays back the seed at shelf price.
        CHECK(SellPrice(info.produce) * info.produceCount >= BuyPrice(info.seed));
    }
    CHECK(CropDays(CropKind::Turnips) == 4 && CropDays(CropKind::Carrots) == 5 && CropDays(CropKind::Potatoes) == 6);
    CHECK(CropDays(CropKind::BroadBeans) == 7 && CropDays(CropKind::Strawberries) == 8 && CropDays(CropKind::Cabbage) == 9);

    // Watered within a day or weeded within about two days: full speed. Bone dry: a fifth.
    CHECK(MoistureGrowthFactor(1.0) == 1.0 && MoistureGrowthFactor(CropCare::WellWatered) == 1.0);
    CHECK(Close(MoistureGrowthFactor(0.0), CropCare::DryFloor));
    CHECK(MoistureGrowthFactor(0.2) > CropCare::DryFloor && MoistureGrowthFactor(0.2) < 1.0);
    CHECK(1.0 - 0.025 * 24.0 >= CropCare::WellWatered - 1e-9);
    CHECK(WeedGrowthFactor(0.0) == 1.0 && WeedGrowthFactor(CropCare::WeedyFrom) == 1.0);
    CHECK(Close(WeedGrowthFactor(1.0), 1.0 - CropCare::WeedPenalty));
    CHECK(CropCare::WeedyFrom / 0.009 > 48.0);

    Plot plot{1, 0, 0, false, 0.0, 0.35, 0.0, CropKind::Roots};
    CHECK(StageOf(plot) == CropStage::Bare);
    CHECK(PlotStatus(plot) == "Tilled soil: ready to plant");
    plot.planted = true;
    plot.moisture = 1.0;
    const std::pair<double, CropStage> stages[] = {{0.0, CropStage::Sown}, {0.1, CropStage::Sprout},
        {0.4, CropStage::Young}, {0.6, CropStage::Growing}, {0.9, CropStage::Mature}, {1.0, CropStage::Ripe}};
    for (const auto& [growth, stage] : stages)
    {
        plot.growth = growth;
        CHECK(StageOf(plot) == stage);
    }
    CHECK(std::string(StageName(CropStage::Sprout)) == "Sprout" && std::string(StageName(CropStage::Ripe)) == "Ripe");
    plot.growth = 0.1;
    CHECK(CropDay(plot) == 1);
    CHECK(PlotStatus(plot) == "Roots: day 1 of 2");
    plot.growth = 0.6;
    plot.moisture = 0.1;
    CHECK(PlotStatus(plot) == "Roots: day 2 of 2  |  needs water, growing slowly");
    plot.weeds = 0.8;
    CHECK(PlotStatus(plot) == "Roots: day 2 of 2  |  needs water and weeding, growing slowly");
    plot.moisture = 1.0;
    CHECK(PlotStatus(plot) == "Roots: day 2 of 2  |  weedy, growing slowly");
    plot.growth = 1.0;
    CHECK(IsRipe(plot) && PlotStatus(plot) == "Roots: ready to harvest");
    // A berry bush growing for the first time counts up normally (no regrowth special case).
    Plot bush{2, 0, 0, true, 1.0 - 24.0 / 42.0 + 0.01, 1.0, 0.0, CropKind::Berries};
    CHECK(StageOf(bush) == CropStage::Young);
    CHECK(PlotStatus(bush) == "Berries: day 1 of 2");
    // Broad beans past the regrow point on their first growth still count days 5 of 7 and show Growing.
    Plot firstBeans{3, 0, 0, true, 1.0 - 72.0 / 168.0 + 0.02, 1.0, 0.0, CropKind::BroadBeans};
    CHECK(StageOf(firstBeans) == CropStage::Growing);
    CHECK(PlotStatus(firstBeans) == "Broad beans: day 5 of 7");

    // A roots crop sown and watered each morning ripens on its stated day.
    Simulation sim;
    BuildingStock(sim);
    BuildRoom(sim);
    OK(sim.Place(Piece::Bed, -3, 0, 0, Home));
    Stock(sim, {{Item::DiggingStick, 1}, {Item::WateringCan, 1}, {Item::Seeds, 1}});
    const Point roots = CellCenter(-2, -1);
    OK(sim.Till(CellToGarden(-2), CellToGarden(-1), roots));
    const int rootId = sim.FindNearestPlot(roots, 1);
    const Result planted = sim.Plant(rootId, roots);
    OK(planted);
    CHECK(planted.message == "Planted roots. Ready in about 2 days if watered.");
    OK(sim.FillWater(WaterSource));
    for (int day = 0; day < 2; ++day)
    {
        Edit(sim, [](State& state) { state.plots[0].moisture = 1.0; state.plots[0].weeds = 0.0; state.hunger = 100; state.energy = 100; });
        CHECK(!IsRipe(sim.GetState().plots[0]));
        sim.AdvanceGameHours(15, Home);
        Edit(sim, [](State& state) { state.hunger = 100; state.energy = 100; });
        sim.AdvanceGameHours(15, Home);
    }
    CHECK(IsRipe(sim.GetState().plots[0]));
    const Result harvested = sim.HarvestCrop(rootId, roots);
    OK(harvested);
    CHECK(harvested.message == "Harvested 4 roots and 2 seeds. This plot is ready to replant.");
    CHECK(!sim.GetState().plots[0].planted);

    // Sow bought carrot seed and broad beans (both spring crops): carrots clear the plot, beans keep cropping.
    Stock(sim, {{Item::DiggingStick, 1}, {Item::WateringCan, 1}, {Item::CarrotSeed, 1}, {Item::BroadBeanSeed, 1}});
    const Point beans = CellCenter(-3, -1);
    OK(sim.Till(CellToGarden(-3), CellToGarden(-1), beans));
    const int beanId = sim.FindNearestPlot(beans, 1);
    const Result sown = sim.Plant(rootId, roots, CropKind::Carrots);
    OK(sown);
    CHECK(sown.message == "Planted carrots. Ready in about 5 days if watered.");
    CHECK(sim.Count(Item::CarrotSeed) == 0);
    UnchangedFailure(sim, [&] { return sim.Plant(beanId, beans, CropKind::Potatoes); });
    OK(sim.Plant(beanId, beans, CropKind::BroadBeans));
    Edit(sim, [](State& state) { for (auto& plot : state.plots) plot.growth = 1.0; });
    const int carrotsBefore = sim.Count(Item::Carrot);
    const Result pulled = sim.HarvestCrop(rootId, roots);
    OK(pulled);
    CHECK(pulled.message == "Harvested 3 carrots. This plot is ready to replant.");
    CHECK(sim.Count(Item::Carrot) == carrotsBefore + 3);
    for (const auto& each : sim.GetState().plots)
        if (each.id == rootId) CHECK(!each.planted && each.kind == CropKind::Roots);
    const Result picked = sim.HarvestCrop(beanId, beans);
    OK(picked);
    CHECK(picked.message == "Harvested 6 broad bean pods. More will ripen in about 3 days.");
    const Plot* beanPlot = nullptr;
    for (const auto& each : sim.GetState().plots) if (each.id == beanId) beanPlot = &each;
    CHECK(beanPlot && beanPlot->planted && beanPlot->kind == CropKind::BroadBeans);
    CHECK(beanPlot && Close(beanPlot->growth, 1.0 - 72.0 / 168.0) && StageOf(*beanPlot) == CropStage::Growing);
    CHECK(beanPlot && beanPlot->picked && PlotStatus(*beanPlot).rfind("Broad beans: ripening again, day 1 of 3", 0) == 0);
    Edit(sim, [&](State& state) { for (auto& plot : state.plots) if (plot.id == beanId) plot.growth = 1.0 - 24.0 / 168.0 + 0.01; });
    for (const auto& each : sim.GetState().plots)
        if (each.id == beanId) CHECK(PlotStatus(each).rfind("Broad beans: ripening again, day 3 of 3", 0) == 0);
    Edit(sim, [&](State& state) { for (auto& plot : state.plots) if (plot.id == beanId) plot.growth = 1.0 - 72.0 / 168.0; });
    // Appended crop kinds and the picked flag survive a save round trip; saves without it load unpicked.
    Simulation saved;
    OK(saved.Deserialize(sim.Serialize()));
    CHECK(saved.Serialize() == sim.Serialize());
    for (const auto& reloaded : saved.GetState().plots) if (reloaded.id == beanId) CHECK(reloaded.picked);
    {
        const std::string text = sim.Serialize();
        const std::string line = std::string(Crops::SaveTag) + " 1 " + std::to_string(beanId) + "\n";
        const auto at = text.find(line);
        CHECK(at != std::string::npos);
        // An older save without the section loads with the plant unpicked.
        std::string body = text.substr(text.find('\n') + 1);
        body.erase(body.find(line), line.size());
        Simulation older;
        OK(older.Deserialize(Envelope(body)));
        for (const auto& reloaded : older.GetState().plots) if (reloaded.id == beanId) CHECK(!reloaded.picked);
        // A repeated section, or one naming a plot that isn't a picked regrowing crop, is refused.
        Simulation strict;
        CHECK(strict.Deserialize(Envelope(body + line + line)).code == ResultCode::CorruptSave);
        CHECK(strict.Deserialize(Envelope(body + std::string(Crops::SaveTag) + " 1 " + std::to_string(rootId) + "\n")).code
            == ResultCode::CorruptSave);
    }

    // Playtest aid: passing tended days grows a crop on schedule; untended ones dry out and lag.
    Stock(sim, {{Item::CarrotSeed, 2}});
    OK(sim.Plant(rootId, roots, CropKind::Carrots));
    Simulation untended;
    OK(untended.Deserialize(sim.Serialize()));
    const double hourBefore = sim.GetState().hour;
    OK(sim.PassDaysForPlaytest(4.9, true, roots));
    for (const auto& each : sim.GetState().plots)
        if (each.id == rootId) CHECK(each.planted && !IsRipe(each) && each.growth > 0.95);
    OK(sim.PassDaysForPlaytest(0.2, true, roots));
    for (const auto& each : sim.GetState().plots)
        if (each.id == rootId) CHECK(IsRipe(each));
    CHECK(Close(sim.GetState().hour - hourBefore, 5.1 * 24.0) && !sim.GetState().failed);
    CHECK(sim.GetState().hunger == 100.0);
    OK(untended.PassDaysForPlaytest(5.1, false, roots));
    for (const auto& each : untended.GetState().plots)
        if (each.id == rootId) CHECK(!IsRipe(each) && each.growth > 0.1);
    UnchangedFailure(sim, [&] { return sim.PassDaysForPlaytest(0.0, true, roots); });
    UnchangedFailure(sim, [&] { return sim.PassDaysForPlaytest(61.0, true, roots); });
    OK(sim.SetCropGrowthForPlaytest(0.5));
    for (const auto& each : sim.GetState().plots)
        if (each.planted) CHECK(each.growth == 0.5);
    UnchangedFailure(sim, [&] { return sim.SetCropGrowthForPlaytest(1.5); });
}

// A real version 12 estate save, written by main at 8762ba46 (40 items; a new estate game with goods
// granted, some moved into the standing room's chest, and 1.5 hours played). Only its placement bake
// version (the seed slot on line 3) is replaced with the current one before loading.
const char* const Version12EstateSave = R"HS12(7.5000000000000249 60 82.00000000000027 99.100000000000847 0 22
 0 12 5 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 2 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 5 0 0 2
2 15039102 0 0
0
1
2 -25900 -64100 0
19
3 0 2 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
0
4 0 2 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
0
5 0 2 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
0
6 0 2 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
0
7 1 2 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
0
8 1 2 1 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
0
9 2 2 0 0 2 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
0
10 1 2 0 0 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
0
11 1 2 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
0
12 1 2 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
0
13 1 2 1 0 2 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
0
14 1 2 0 1 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
0
15 3 2 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
0
16 3 2 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
0
17 3 2 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
0
18 3 2 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
0
19 7 2 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
0
20 5 2 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
0
21 6 2 1 0 0 0 0 4 4 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1
4
1 10 1 0
2 1 4 0
8 2 4 0
9 39 1 0
0
0
2 10
1
1 0 0 2 0
1 1 0 0 0 
5
3 1 12 0
4 2 5 0
5 18 2 0
6 39 2 0
7 36 5 0
0
parcels 4
EstateBoundary 1
ForSale.Woodland 0
ForSale.MoorField 0
ForSale.WestCove 0
economy 2234 1
1 0 -54000 117600 -90 0 0
tools 6 0 0 0 0 0 0
manor 456c65616e6f72 436176656e64697368 54726576656e6e6f72 19
3 1 1
4 1 1
5 1 1
6 1 1
7 1 1
8 1 1
9 1 1
10 1 1
11 1 1
12 1 1
13 1 1
14 1 1
15 1 1
16 1 1
17 1 1
18 1 1
19 0 1
20 0 1
21 0 1
1 arrival
)HS12";

// harden-save-item-stocks: stocks carry their width from version 13, so a save written before items
// were appended still loads, and version 12 saves (always 40 items wide) migrate.
void ItemStockWidthCompatibility()
{
    Simulation sim;
    BuildingStock(sim);
    BuildRoom(sim);
    OK(sim.Place(Piece::Chest, -4, 0, 0, CellCenter(-4, 0)));
    const Item last = static_cast<Item>(PositionalStockMinimumItems - 1);
    Stock(sim, {{Item::Roots, 4}, {Item::Branch, 3}, {last, 2}});
    const int chest = StructureId(sim, Piece::Chest, CellCenter(-4, 0));
    OK(sim.Transfer(chest, Item::Roots, 1, CellCenter(-4, 0)));
    OK(sim.Transfer(chest, last, 1, CellCenter(-4, 0)));
    const std::string current = sim.Serialize();
    CHECK(current.rfind("HOMESTEAD 13 ", 0) == 0);
    const auto chestStock = [](const Simulation& s) {
        for (const auto& piece : s.GetState().structures) if (piece.kind == Piece::Chest) return piece.storage;
        return Inventory{};
    };
    const auto sameGoods = [&](const Simulation& loaded) {
        CHECK(loaded.GetState().inventory == sim.GetState().inventory);
        CHECK(chestStock(loaded) == chestStock(sim));
        CHECK(loaded.GetState().equipment == sim.GetState().equipment);
    };

    // Round trip at version 13.
    Simulation restored;
    OK(restored.Deserialize(current));
    CHECK(restored.Serialize() == current);
    sameGoods(restored);

    // A version 12 save (positional, 40 wide) migrates, and the next save is version 13.
    Simulation migrated;
    OK(migrated.Deserialize(Encode(sim.GetState(), PositionalStockSaveVersion)));
    sameGoods(migrated);
    CHECK(migrated.Serialize().rfind("HOMESTEAD 13 ", 0) == 0);
    OK(restored.Deserialize(migrated.Serialize()));
    sameGoods(restored);

    // A save from a build with fewer items loads with the missing ones at zero: appending an item
    // no longer breaks same-version saves. (That build never had the last item, so its save holds none.)
    State older = sim.GetState();
    older.inventory[static_cast<int>(last)] = 0;
    for (auto& piece : older.structures) piece.storage[static_cast<int>(last)] = 0;
    FixtureLayouts(older);
    Simulation narrower;
    OK(narrower.Deserialize(Encode(older, SimulationSaveVersion, PositionalStockMinimumItems - 1)));
    CHECK(narrower.Count(last) == 0 && chestStock(narrower)[static_cast<int>(last)] == 0);
    CHECK(narrower.Count(Item::Roots) == sim.Count(Item::Roots) && chestStock(narrower)[static_cast<int>(Item::Roots)] == 1);

    // Stocks wider than this build's catalogue came from a newer build: refused, nothing changed.
    const std::string before = restored.Serialize();
    CHECK(restored.Deserialize(Encode(sim.GetState(), SimulationSaveVersion, ItemCount + 1)).code == ResultCode::NewerBuild);
    CHECK(restored.Deserialize(Encode(sim.GetState(), PositionalStockSaveVersion, ItemCount + 1)).code == ResultCode::NewerBuild);
    CHECK(restored.Serialize() == before);
    // Malformed widths are corrupt.
    CHECK(restored.Deserialize(Encode(sim.GetState(), SimulationSaveVersion, 0)).code == ResultCode::CorruptSave);
    CHECK(restored.Deserialize(Encode(sim.GetState(), PositionalStockSaveVersion, PositionalStockMinimumItems - 1)).code
        == ResultCode::CorruptSave);
    CHECK(restored.Serialize() == before);

    // The real version 12 estate save migrates with every stock, the purse and the manor intact.
    std::string payload = Version12EstateSave;
    std::vector<std::string> lines;
    std::istringstream split(payload);
    for (std::string line; std::getline(split, line);) lines.push_back(line);
    lines[2] = std::to_string(ProvisionalEstatePlacements().bakeVersion) + lines[2].substr(lines[2].find(' '));
    payload.clear();
    for (const auto& line : lines) payload += line + '\n';
    Simulation estate;
    OK(estate.NewEstateGame(ProvisionalEstateLayout(), ProvisionalEstatePlacements()));
    OK(estate.Deserialize(Envelope(payload, PositionalStockSaveVersion)));
    CHECK(estate.Count(Item::Branch) == 12 && estate.Count(Item::Stone) == 5 && estate.Count(Item::Pasty) == 2);
    CHECK(estate.Count(Item::Primroses) == 5 && estate.Count(Item::WildGarlic) == 2);
    const Inventory estateChest = chestStock(estate);
    CHECK(estateChest[static_cast<int>(Item::Stone)] == 4 && estateChest[static_cast<int>(Item::WildGarlic)] == 1);
    CHECK(estate.GetState().money == 2234 && estate.GetState().fixedEstate);
    const std::string upgraded = estate.Serialize();
    CHECK(upgraded.rfind("HOMESTEAD 13 ", 0) == 0);
    Simulation reloaded;
    OK(reloaded.NewEstateGame(ProvisionalEstateLayout(), ProvisionalEstatePlacements()));
    OK(reloaded.Deserialize(upgraded));
    CHECK(reloaded.Serialize() == upgraded);

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
    for (int invalid : {-1, static_cast<int>(CropKind::Count), 999999, std::numeric_limits<int>::max()})
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
    // New games run 60-minute days: 150 real seconds are one game hour.
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
    // 28-day seasons from the 06:00 rollover (the calendar's own tests are in HomesteadCalendarTests).
    Edit(sim, [](State& state) { state.hour = 14 * 24 + 6; });
    CHECK(sim.DayNumber() == 15);
    CHECK(std::string(sim.SeasonName()) == "Spring");
    Edit(sim, [](State& state) { state.hour = 28 * 24 + 6; });
    CHECK(sim.DayNumber() == 1);
    CHECK(std::string(sim.SeasonName()) == "Summer");
    Edit(sim, [](State& state) { state.hour = 56 * 24 + 6; });
    CHECK(std::string(sim.SeasonName()) == "Autumn");
    Edit(sim, [](State& state) { state.hour = 84 * 24 + 6; });
    CHECK(std::string(sim.SeasonName()) == "Winter");
    Edit(sim, [](State& state) { state.hour = 112 * 24 + 6; });
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
    Edit(bare, [](State& state) { state.hour = 84 * 24 + 20; state.energy = 100; state.hunger = 80; });
    Edit(sim, [](State& state) { state.hour = 84 * 24 + 20; state.energy = 100; state.hunger = 80; });
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
    UnchangedFailure(bare, [&] { return bare.Sleep(8, Home, {1, 0}); });
    BuildingStock(bare);
    OK(bare.Place(Piece::Bed, -3, 0, 0, Home));
    UnchangedFailure(bare, [&] { return bare.Sleep(13, Home, {1, 0}); });
    UnchangedFailure(bare, [&] { return bare.Sleep(-1, Home, {1, 0}); });
    UnchangedFailure(bare, [&] { return bare.Sleep(std::numeric_limits<double>::quiet_NaN(), Home, {1, 0}); });
    UnchangedFailure(bare, [&] { return bare.Sleep(8, {3900, 3900}, {1, 0}); });
    Edit(bare, [](State& state) { state.hour = 19; state.hunger = 0.65; state.energy = 10; });
    const auto checkpoint = bare.Serialize();
    CHECK(!bare.Sleep(8, Home, {1, 0}).ok);
    CHECK(bare.GetState().failed);
    CHECK(Close(bare.GetState().hour, 19.5));
    CHECK(Close(bare.GetState().energy, 15));
    CHECK(bare.GetState().hunger == 0);
    const auto failed = bare.Serialize();
    bare.AdvanceGameHours(12, Home);
    CHECK(failed == bare.Serialize());
    UnchangedFailure(bare, [&] { return bare.Eat(Item::Berries); });
    UnchangedFailure(bare, [&] { return bare.SetDayMinutes(30); });
    UnchangedFailure(bare, [&] { return bare.Sleep(1, Home, {1, 0}); });
    UnchangedFailure(bare, [&] { return bare.Craft(Recipe::HaftAxe, Home); });
    Simulation savedFailure;
    OK(savedFailure.Deserialize(failed));
    CHECK(savedFailure.GetState().failed);
    OK(bare.Deserialize(checkpoint));
    CHECK(!bare.GetState().failed);
    // A cold night in bed is simply rest now.
    Edit(bare, [](State& state) { state.hour = 84 * 24 + 20; state.hunger = 80; state.energy = 60; });
    OK(bare.Sleep(8, Home, {1, 0}));
    CHECK(!bare.GetState().failed && bare.GetState().energy == 100);
    // Time awake drains Energy slowly (1.2 points last two hours). Running out doesn't fail her: she
    // dozes off where she stands for DozeHours at the slower rate, then carries on awake.
    Simulation tired;
    Edit(tired, [](State& state) { state.energy = 1.2; });
    const double tiredStart = tired.GetState().hour;
    tired.AdvanceGameHours(10, Home);
    CHECK(!tired.GetState().failed && tired.DozeCount() == 1);
    CHECK(Close(tired.GetState().hour, tiredStart + 10.0));
    CHECK(Close(tired.GetState().energy, Exertion::DozeHours * Exertion::DozePerHour - 2.0 * Exertion::AwakePerHour));
    // A doze that runs out her food still fails her, as any sleep would.
    Simulation starving;
    Edit(starving, [](State& state) { state.energy = 0.6; state.hunger = 3.0; });
    starving.AdvanceGameHours(4, Home);
    CHECK(starving.GetState().failed && starving.GetState().hunger == 0 && starving.DozeCount() == 1);
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
    OK(once.Sleep(8, Home, {1, 0}));
    for (int i = 0; i < 16; ++i) OK(split.Sleep(0.5, Home, {1, 0}));
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
    UnchangedFailure(once, [&] { return once.Sleep(1, Home, {1, 0}); });
    Edit(once, [](State& state) { state.nextId = TransientResourceIdBase - 1; });
    UnchangedFailure(once, [&] { return once.Till(CellToGarden(-1), CellToGarden(-1), CellCenter(-1, -1)); });
    UnchangedFailure(once, [&] { return once.Place(Piece::Foundation, -1, -1, 0, CellCenter(-1, -1)); });
    Simulation restored;
    OK(restored.Deserialize(once.Serialize()));
}

void BedSleepReachAndPriority()
{
    Simulation sim;
    BuildingStock(sim);
    BuildRoom(sim);
    OK(sim.Place(Piece::Bed, -3, 0, 0, Home));
    const auto& state = sim.GetState();
    const Structure* bed = nullptr;
    for (const auto& structure : state.structures)
        if (structure.kind == Piece::Bed) bed = &structure;
    CHECK(bed != nullptr);
    const int bedId = bed->id;
    const Footprint box = StructureFootprint(state, *bed);
    const auto approach = [&](double outside)
    {
        const Point offset = RotateYaw({box.half.x + outside, 0}, box.yaw);
        return Point{box.center.x + offset.x, box.center.y + offset.y};
    };
    const Point near = approach(89);
    const Point toward = RotateYaw({-1, 0}, box.yaw);
    const Point away = RotateYaw({1, 0}, box.yaw);
    CHECK(ReachableBed(state, near, toward) == bedId);
    CHECK(ReachableBed(state, approach(91), toward) == -1);
    CHECK(ReachableBed(state, near, away) == -1);
    CHECK(ReachableBed(state, near, RotateYaw({-0.87, 0.49}, box.yaw)) == bedId);
    CHECK(ReachableBed(state, near, RotateYaw({-0.85, 0.53}, box.yaw)) == bedId);
    CHECK(ReachableBed(state, near, RotateYaw({-0.25, 0.97}, box.yaw)) == -1);
    const Point headOffset = RotateYaw({box.half.x + 85, box.half.y + 20}, box.yaw);
    const Point nearHead{box.center.x + headOffset.x, box.center.y + headOffset.y};
    CHECK(ReachableBed(state, nearHead,
        RotateYaw({-2 * box.half.x - 85, -2 * box.half.y - 20}, box.yaw)) == bedId);
    CHECK(ReachableBed(state, approach(94), toward) == -1);
    CHECK(BedFocusCandidate(state, approach(94), toward, false, true) == bedId);
    CHECK(BedFocusCandidate(state, approach(94), toward, false, false) == -1);
    UnchangedFailure(sim, [&] { return sim.Sleep(1, approach(91), toward); });
    UnchangedFailure(sim, [&] { return sim.Sleep(1, near, away); });

    OK(sim.Place(Piece::Foundation, -2, 0, 0, CellCenter(-2, 0)));
    OK(sim.Place(Piece::Chest, -2, 0, 0, CellCenter(-2, 0)));
    CHECK(sim.FindNearestStructure(near, Piece::Chest, 280) != -1);
    const Structure* chest = nullptr;
    for (const auto& structure : sim.GetState().structures)
        if (structure.kind == Piece::Chest) chest = &structure;
    CHECK(chest != nullptr);
    const Point chestCenter = StructureFootprint(sim.GetState(), *chest).center;
    CHECK(chestCenter.x != StructureCenter(sim.GetState(), *chest).x
        && chestCenter.y != StructureCenter(sim.GetState(), *chest).y);
    const Point chestDirection{chestCenter.x - near.x, chestCenter.y - near.y};
    CHECK(toward.x * chestDirection.x + toward.y * chestDirection.y
        < BedFacingCosine * std::hypot(chestDirection.x, chestDirection.y));
    CHECK(BedFocusCandidate(sim.GetState(), near, toward, true) == -1);
    CHECK(BedFocusCandidate(sim.GetState(), near, toward, false) == bedId);
    OK(sim.Sleep(1, near, toward));
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
    // Sorting orders what lies below the hotbar row (HomesteadPackRow.h); empty the row first.
    std::array<int, PackRowSize> noRow;
    noRow.fill(-1);
    OK(sim.ArrangePackRow(noRow));
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
    // A world drop stack holds at most what the largest pack (the leather backpack) can.
    malformed.worldDrops.push_back({malformed.nextId, Home, Item::Branch, MaxPackCapacity + 1, 0});
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
            if (sim.GetState().hunger == 100.0 && sim.GetState().energy == 100.0)
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
    // Older unreadable versions are unsupported; later ones came from a newer build.
    for (int version : {1, 2, 3, 4, 5, 6, RetiredTestSaveVersion, SimulationSaveVersion + 1, 999})
    {
        CHECK(sim.Deserialize(Encode(sim.GetState(), version)).code
            == (version > SimulationSaveVersion ? ResultCode::NewerBuild : ResultCode::UnsupportedVersion));
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
            const auto size = sim.Serialize().size();
            CHECK(size >= lastSize - 4);
            lastSize = size;
            // Each gather packs 5 branches and a kindling; empty the pack before it fills.
            if (actions % 16 == 0)
            {
                WorldStock(sim, {{Item::Knife, 1}, {Item::Hatchet, 1}});
                // 256 harvests is far more work than one day's Energy; rest between batches.
                Edit(sim, [](State& state) { state.energy = 100; }, false);
                lastSize = sim.Serialize().size();
            }
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

// Sprint is free (Jenny, round 2): no Energy of its own at any frame rate or day length. She may
// start or keep sprinting from 25 Energy; the awake drain and work are what bring her there.
void SprintEnergyContract()
{
    Simulation sim;
    CHECK(Exertion::SprintFloor == 25.0);
    const std::string before = sim.Serialize();
    const auto revision = sim.GetRevision();
    OK(sim.CanSprint());
    // Asking never changes anything.
    CHECK(sim.Serialize() == before && sim.GetRevision() == revision);

    // A sprinting tick is an ordinary tick: the energy after N seconds matches walking exactly, for
    // 30, 60 and 120 fps and 30, 60 and 120-minute days.
    for (const double dayMinutes : {30.0, 60.0, 120.0})
        for (const int fps : {30, 60, 120})
        {
            Simulation walking;
            OK(walking.SetDayMinutes(dayMinutes));
            Simulation running = walking;
            const double step = 1.0 / fps;
            for (int tick = 0; tick < fps * 20; ++tick)
            {
                OK(running.CanSprint());
                running.Advance(step, Home);
                walking.Advance(step, Home);
            }
            CHECK(running.Serialize() == walking.Serialize());
            // Only the slow awake drain: 20 s is well under a tenth of an Energy point.
            const double hours = 20.0 * 24.0 / (dayMinutes * 60.0);
            CHECK(Close(running.GetState().energy, 100.0 - hours * Exertion::AwakePerHour, 1e-6));
        }

    // The floor: from 25 she may run, below it she can't, and a failed run can't either.
    OK(sim.SetEnergy(25.0));
    OK(sim.CanSprint());
    OK(sim.SetEnergy(24.99));
    const auto tired = sim.CanSprint();
    CHECK(!tired.ok && tired.message == "Too tired to sprint.");
    OK(sim.SetEnergy(20.0));
    CHECK(!sim.CanSprint());
    // Eating brings her back over the floor; nothing turns the toggle back on here (that's the pawn's
    // job, and it never does it by itself), but she may ask again.
    Stock(sim, {{Item::Berries, 2}});
    OK(sim.Eat(Item::Berries));
    OK(sim.Eat(Item::Berries));
    OK(sim.CanSprint());
    // Work that wears her down to the floor stops her sprinting.
    OK(sim.SetEnergy(25.4));
    const auto branch = Node(sim, ResourceKind::Branches);
    OK(sim.Harvest(branch.id, branch.position));
    CHECK(sim.GetState().energy < 25.0 && !sim.CanSprint());
    // Energy survives a save exactly; the toggle itself is never saved.
    Simulation loaded;
    OK(loaded.Deserialize(sim.Serialize()));
    CHECK(loaded.GetState().energy == sim.GetState().energy && !loaded.CanSprint());

    Simulation failed;
    failed.AdvanceGameHours(120, Home);
    CHECK(failed.GetState().failed && !failed.CanSprint());
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
    CHECK(probe.Till(CellToGarden(-3), CellToGarden(-1), CellCenter(-3, -1)).message.find("Too tired") != std::string::npos);
    // Even light work waits until she is back above the exhausted band.
    const int plot = sim.GetState().plots[0].id;
    UnchangedFailure(sim, [&] { return sim.Plant(plot, CellCenter(-2, -1)); });
    OK(sim.SetEnergy(10.5));
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
    // The standing room is the carved-out corner of the ruin, and everything but its salvage and its
    // own loose debris (HomesteadRuinDebris.h) lies outside it.
    const auto& footprint = layout.FindPolygon(Anchor::ManorFootprint)->points;
    CHECK(!PointInPolygon(footprint, spawn));
    int misplaced = 0;
    for (const auto& placement : ProvisionalEstatePlacements().placements)
    {
        // On the estate, or the narrow public-roadside exception (HomesteadEstatePublicRoad.h).
        const bool onEstate = EstatePlacementAllowed(layout, placement);
        const bool inRuin = placement.kind != ResourceKind::SalvagePile && !RuinDebris::Find(placement.id)
            && PointInPolygon(footprint, placement.position);
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
    // She carries only the oil lamp and its flasks (add-oil-lamp).
    CHECK(sim.Count(Item::WateringCan) == 0 && sim.Count(Item::Knife) == 0);
    CHECK(sim.Count(Item::OilLamp) == 1 && sim.Count(Item::OilFlask) == 3 && sim.UsedCapacity() == 4);
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
    const auto tookDown = overflow.Deconstruct(fullChest, chestSite);
    OK(tookDown);
    // What didn't fit is worth a short notice (ResultCode::PackOverflow), not the full story.
    CHECK(tookDown.code == ResultCode::PackOverflow && tookDown.message.rfind("Pack full: ", 0) == 0);
    CHECK(overflow.UsedCapacity() == InventoryCapacity);
    int dropped = 0;
    for (const auto& drop : overflow.GetState().worldDrops) dropped += drop.quantity;
    CHECK(dropped == 67);
}

// Ground stacks never outgrow her pack (120 without the leather backpack), since PickUpDrop takes a
// stack whole: drops in one spot and a chest's spill both split at her capacity.
void GroundStacksStayPickable()
{
    Simulation sim;
    Stock(sim, {{Item::Branch, 100}});
    OK(sim.DropGroup(Group(sim, Item::Branch), 100, Home, Home, sim.GetRevision()));
    OK(sim.GrantItems(Item::Branch, 100));
    OK(sim.DropGroup(Group(sim, Item::Branch), 100, Home, Home, sim.GetRevision()));
    CHECK(sim.GetState().worldDrops.size() == 2);
    for (const auto& drop : sim.GetState().worldDrops) CHECK(drop.quantity == 100);
    OK(sim.PickUpDrop(sim.GetState().worldDrops.front().id, Home));
    CHECK(sim.Count(Item::Branch) == 100);
    // A small drop still joins a stack when it fits.
    OK(sim.DropGroup(Group(sim, Item::Branch), 20, Home, Home, sim.GetRevision()));
    CHECK(sim.GetState().worldDrops.size() == 1 && sim.GetState().worldDrops.front().quantity == 120);

    // A chest of 300 stone taken down with a full pack spills 120 / 120 / 60.
    Simulation spill;
    Stock(spill, {{Item::Branch, 105}, {Item::BrambleCanes, 2}});
    const Point chestSite = CellCenter(2, 0);
    OK(spill.Place(Piece::Chest, 2, 0, 0, chestSite));
    const int chest = StructureIdAt(spill, Piece::Chest, 2, 0);
    OK(spill.Transfer(chest, Item::Branch, spill.Count(Item::Branch), chestSite));
    for (int put = 0; put < 300; put += 100)
    {
        OK(spill.GrantItems(Item::Stone, 100));
        OK(spill.Transfer(chest, Item::Stone, 100, chestSite));
    }
    OK(spill.GrantItems(Item::Fiber, InventoryCapacity - spill.UsedCapacity()));
    CHECK(spill.UsedCapacity() == InventoryCapacity);
    OK(spill.Deconstruct(chest, chestSite));
    std::vector<int> stones;
    for (const auto& drop : spill.GetState().worldDrops)
    {
        CHECK(drop.quantity <= InventoryCapacity);
        if (drop.wearableId == 0 && drop.item == Item::Stone) stones.push_back(drop.quantity);
    }
    std::sort(stones.begin(), stones.end());
    CHECK((stones == std::vector<int>{60, 120, 120}));
    InventoryRoundTrip(spill);
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
    CHECK(waterSim.Count(Item::Water) == PailPortions);
    CHECK(waterSim.UsedCapacity() == InventoryCapacity);
}

void SleepOptionPolicy()
{
    const auto tiredNight = BedSleepOption(21.0, 60.0);
    CHECK(tiredNight && tiredNight->choice == SleepChoice::UntilRested
        && Close(tiredNight->hours, 4.0) && Close(tiredNight->wakeHour, 1.0));
    const auto restedNight = BedSleepOption(25.0, 100.0);
    CHECK(restedNight && restedNight->choice == SleepChoice::UntilMorning
        && Close(restedNight->hours, 5.0) && Close(restedNight->wakeHour, 6.0));
    const auto late = BedSleepOption(22.75, 20.0);
    CHECK(late && late->choice == SleepChoice::UntilRested
        && Close(late->hours, 7.25) && Close(late->wakeHour, 6.0));
    const auto owl = BedSleepOption(5.0, 5.0);
    CHECK(owl && owl->choice == SleepChoice::UntilRested && Close(owl->hours, 1.0) && Close(owl->wakeHour, 6.0));
    const auto midday = BedSleepOption(12.0, 97.0);
    CHECK(midday && midday->choice == SleepChoice::UntilRested && Close(midday->hours, 0.5));
    CHECK(!BedSleepOption(12.0, 100.0));
    CHECK(!BedSleepOption(12.0, 99.6));
    CHECK(BedSleepOption(21.0, 99.6)->choice == SleepChoice::UntilMorning);
    CHECK(!BedSleepOption(6.0, 100.0));
    const auto shortMorning = BedSleepOption(5.875, 100.0);
    CHECK(shortMorning && shortMorning->choice == SleepChoice::UntilMorning
        && Close(shortMorning->hours, 0.125) && Close(shortMorning->wakeHour, 6.0));
    const auto shortRest = BedSleepOption(5.875, 50.0);
    CHECK(shortRest && shortRest->choice == SleepChoice::UntilRested
        && Close(shortRest->hours, 0.125) && Close(shortRest->wakeHour, 6.0));
    CHECK(Close(BedSleepOption(12.0, 0.0)->hours, Exertion::MaxRestHours));
    CHECK(Close(BedSleepOption(18.0, 50.0)->hours, 5.0));
    CHECK(Close(BedSleepOption(18.0, 100.0)->hours, 12.0));
    CHECK(!BedSleepOption(std::numeric_limits<double>::quiet_NaN(), 50.0));
    CHECK(!BedSleepOption(12.0, std::numeric_limits<double>::quiet_NaN()));
    Simulation sleeper;
    BuildingStock(sleeper);
    OK(sleeper.Place(Piece::Bed, -3, 0, 0, Home));
    Edit(sleeper, [](State& state) { state.hour = 21.0; state.energy = 60.0; });
    OK(sleeper.Sleep(BedSleepOption(sleeper.GetState().hour, sleeper.GetState().energy)->hours, Home, {1, 0}));
    CHECK(Close(sleeper.GetState().hour, 25.0) && sleeper.GetState().energy == 100.0);
    OK(sleeper.Sleep(BedSleepOption(sleeper.GetState().hour, sleeper.GetState().energy)->hours, Home, {1, 0}));
    if (!Close(sleeper.GetState().hour, 30.0) || !Close(sleeper.GetState().energy, 100.0))
        std::cerr << "Bed policy at sunrise: hour=" << sleeper.GetState().hour
            << " energy=" << sleeper.GetState().energy << '\n';
    CHECK(Close(sleeper.GetState().hour, 30.0) && Close(sleeper.GetState().energy, 100.0));
    Edit(sleeper, [](State& state) { state.hour = 29.875; state.energy = 50.0; });
    UnchangedFailure(sleeper, [&] { return sleeper.Sleep(0.125, Home, {1, 0}); });
    OK(sleeper.Sleep(shortRest->hours, Home, {1, 0}, true));
    CHECK(Close(sleeper.GetState().hour, 30.0) && Close(sleeper.GetState().energy, 51.25));
    UnchangedFailure(sleeper, [&] { return sleeper.Sleep(0.125, Home, {1, 0}, true); });
    // Rain at any hour (Jenny, 2026-09-30: "let it randomize throughout the day/night cycle"). Each calendar
    // day may draw one spell of 1-8 h starting at any hour, free to run past midnight and past 06:00, with
    // cloud building before it and clearing after; spells never merge; autumn and winter are wetter; the
    // year keeps about the old 5% of hours wet. The hour alone decides it (Simulation/HomesteadRain.h).
    {
        const int days = Calendar::DaysPerYear * 20;
        int spells = 0, nightStarts = 0, pastMidnight = 0, pastRollover = 0;
        double shortest = 1e9, longest = 0.0;
        int startHour[24] = {}, nightRainBySeason[4] = {};
        RainSpell previous;
        bool havePrevious = false;
        CHECK(!RainSpellOfDay(0, previous));   // a new game's first day is dry
        for (int day = 0; day < days; ++day)
        {
            RainSpell spell;
            if (!RainSpellOfDay(day, spell)) continue;
            ++spells;
            const double length = spell.end - spell.start;
            shortest = std::min(shortest, length);
            longest = std::max(longest, length);
            CHECK(length >= Rain::MinSpellHours - 1e-9 && length <= Rain::MaxSpellHours + 1e-9);
            CHECK(spell.buildUp >= Rain::MinCloudHours && spell.buildUp <= Rain::MaxCloudHours
                && spell.clearing >= Rain::MinCloudHours && spell.clearing <= Rain::MaxCloudHours);
            if (havePrevious)   // apart: the sky clears between spells, so none runs longer than the max
                CHECK(spell.start - spell.buildUp >= previous.end + previous.clearing + Rain::MinClearHours - 1e-9);
            previous = spell;
            havePrevious = true;
            const double startOfDay = std::fmod(spell.start, 24.0);
            ++startHour[static_cast<int>(startOfDay)];
            if (startOfDay >= 19.0 || startOfDay < 6.0)
            {
                ++nightStarts;
                ++nightRainBySeason[static_cast<int>(Calendar::DateOfDay(day).season)];
            }
            if (std::floor(spell.end / 24.0) > std::floor(spell.start / 24.0)) ++pastMidnight;
            if (Calendar::DayIndex(spell.end - 1e-6) > spell.day) ++pastRollover;
            // Same answer every time it's asked: nothing random at run time.
            RainSpell again;
            CHECK(RainSpellOfDay(day, again) && again.start == spell.start && again.end == spell.end);
        }
        CHECK(shortest < 1.5 && longest > 7.5);
        for (int hour = 0; hour < 24; ++hour) CHECK(startHour[hour] > 0);   // any hour of the day or night
        CHECK(nightStarts > spells / 4 && pastMidnight > 0 && pastRollover > 0);
        for (int season = 0; season < 4; ++season) CHECK(nightRainBySeason[season] > 0);
        // The share of hours wet, by season (sampled every ten minutes over twenty years).
        double wet[4] = {}, all[4] = {}, wetTotal = 0.0, allTotal = 0.0;
        for (double hour = Calendar::DayStartHour; hour < days * 24.0; hour += 1.0 / 6.0)
        {
            const int season = static_cast<int>(Calendar::DateAt(hour).season);
            const bool raining = IsRainingAt(hour);
            all[season] += 1.0;
            allTotal += 1.0;
            if (raining) { wet[season] += 1.0; wetTotal += 1.0; }
        }
        std::printf("RAIN wet share spring %.4f summer %.4f autumn %.4f winter %.4f year %.4f; %d spells, %d at night\n",
            wet[0] / all[0], wet[1] / all[1], wet[2] / all[2], wet[3] / all[3], wetTotal / allTotal, spells, nightStarts);
        CHECK(wet[1] / all[1] < wet[0] / all[0] && wet[0] / all[0] < wet[2] / all[2] && wet[2] / all[2] < wet[3] / all[3]);
        CHECK(wetTotal / allTotal > 0.04 && wetTotal / allTotal < 0.065);   // the old schedule: 6 h on 20% of days = 5%
        // It depends on the hour alone, so a save and reload keeps the weather, at night as by day.
        RainSpell nightSpell;
        for (double from = 30.0; NextRainSpell(from, nightSpell) && std::fmod(nightSpell.start, 24.0) < 21.0;) from = nightSpell.end;
        Simulation before;
        BuildingStock(before);
        Edit(before, [&nightSpell](State& state) { state.hour = nightSpell.start + 0.2; });
        Simulation after;
        OK(after.Deserialize(before.Serialize()));
        CHECK(before.IsRaining() && after.IsRaining() && after.GetState().hour == before.GetState().hour);
    }
    // One spell's shape: cloud builds over its build-up and clears over its clearing; the rain eases in and
    // out and swells between drizzle and showers without jumps; the ground wets through and dries after.
    {
        RainSpell spell;
        for (double from = 30.0; NextRainSpell(from, spell) && spell.end - spell.start < 5.0;) from = spell.end;
        const double a = spell.start, b = spell.end;
        CHECK(Overcast(a - spell.buildUp - 0.01) == 0.0 && Overcast(a - spell.buildUp * 0.5) > 0.2 && Overcast(a) == 1.0
            && Overcast((a + b) * 0.5) == 1.0 && Overcast(b + spell.clearing * 0.5) > 0.2 && Overcast(b + spell.clearing + 0.01) == 0.0);
        CHECK(RainAmount(a) == 0.0 && RainAmount(a - 0.1) == 0.0 && RainAmount(b) == 0.0 && !IsRainingAt(b) && IsRainingAt(a));
        CHECK(NextRainChange(a - 0.25) == a && NextRainChange(a + 0.25) == b);
        double lowest = 1.0, highest = 0.0, biggestStep = 0.0, previous = RainAmount(a + 0.3);
        for (double h = a + 0.3; h < b - 0.2; h += 1.0 / 60.0)
        {
            const double amount = RainAmount(h);
            lowest = std::min(lowest, amount);
            highest = std::max(highest, amount);
            biggestStep = std::max(biggestStep, std::abs(amount - previous));
            previous = amount;
        }
        CHECK(lowest >= 0.29 && lowest < 0.45 && highest > 0.85 && highest <= 1.0 && biggestStep < 0.1);
        CHECK(GroundWetness(a - 0.01) == 0.0 && GroundWetness(a + 0.6) == 1.0 && GroundWetness(b) == 1.0
            && GroundWetness(b + 2.0) > 0.2 && GroundWetness(b + 2.0) < 0.8);
        RainSpell next;
        if (!NextRainSpell(b, next) || next.start > b + Rain::DryHours) CHECK(GroundWetness(b + Rain::DryHours + 0.01) == 0.0);
    }
    // Rain loudness (Jenny, 2026-09-29: too loud; halve it). Exactly half the previous gain, which was
    // rain^0.7 * ambience * lerp(0.9 outdoors, 0.35 indoors), at every strength and indoors or out; it
    // starts and stops at the same moments, and no rain is silent. Full rain at the default 0.7 ambience:
    // 0.63 -> 0.315 outdoors, 0.245 -> 0.1225 indoors.
    {
        const auto previousGain = [](double rain, double ambience, double indoors)
            { return std::pow(rain, 0.7) * ambience * (0.9 + (0.35 - 0.9) * indoors); };
        for (const double rain : {1.0, 0.8, 0.3, 0.05})
            for (const double indoors : {0.0, 0.5, 1.0})
                for (const double ambience : {0.7, 1.0, 0.2})
                {
                    CHECK(Close(RainAudioGain(rain, ambience, indoors), 0.5 * previousGain(rain, ambience, indoors)));
                    CHECK(RainAudible(rain, ambience, indoors) == (previousGain(rain, ambience, indoors) > 0.001));
                }
        CHECK(Close(RainAudioGain(1.0, 0.7, 0.0), 0.315) && Close(RainAudioGain(1.0, 0.7, 1.0), 0.1225));
        CHECK(RainAudioGain(0.0, 0.7, 0.0) == 0.0 && !RainAudible(0.0, 0.7, 0.0));
        CHECK(RainAudioGain(1.0, 0.0, 0.0) == 0.0 && !RainAudible(1.0, 0.0, 1.0));   // Ambience muted
    }
    // At 05:00, rest stops at 06:00 even when she still needs more sleep; daytime can finish it.
    Simulation owlSim;
    BuildingStock(owlSim);
    OK(owlSim.Place(Piece::Bed, -3, 0, 0, Home));
    Edit(owlSim, [](State& state) { state.hour = 29.0; state.energy = 5.0; state.hunger = 90.0; });
    const auto owlRest = BedSleepOption(owlSim.GetState().hour, owlSim.GetState().energy);
    CHECK(owlRest && Close(owlRest->hours, 1.0));
    OK(owlSim.Sleep(owlRest->hours, Home, {1, 0}));
    CHECK(Close(owlSim.GetState().hour, 30.0));
    CHECK(Close(owlSim.GetState().energy, 15.0) && !owlSim.GetState().failed);
    const auto afterDawn = BedSleepOption(owlSim.GetState().hour, owlSim.GetState().energy);
    if (!afterDawn || !Close(afterDawn->hours, 8.5))
        std::cerr << "After dawn: hour=" << owlSim.GetState().hour
            << " energy=" << owlSim.GetState().energy
            << " offer=" << (afterDawn ? afterDawn->hours : -1) << '\n';
    CHECK(afterDawn && Close(afterDawn->hours, 8.5));
    OK(owlSim.Sleep(afterDawn->hours, Home, {1, 0}));
    CHECK(Close(owlSim.GetState().hour, 38.5) && Close(owlSim.GetState().energy, 100.0));
}

void EstateExhaustionIsNonlethal()
{
    const auto makeEstate = []()
    {
        Simulation estate;
        OK(estate.NewEstateGame(ProvisionalEstateLayout(), ProvisionalEstatePlacements()));
        return estate;
    };
    Simulation sim = makeEstate();
    OK(sim.SetEnergy(0.0));
    const double started = sim.GetState().hour;
    const int dozes = sim.DozeCount();
    CHECK(Exertion::SprintFloor == 25.0 && Exertion::SlowWalkFloor == 10.0);
    CHECK(Close(Exertion::WalkSpeedFactor(0), 0.75));
    CHECK(Close(Exertion::WalkSpeedFactor(9.99), 0.75));
    CHECK(Close(Exertion::WalkSpeedFactor(10), 1.0));
    CHECK(!sim.CanSprint() && sim.CheckExertion(0.5).message == "Too tired.");
    sim.AdvanceGameHours(72.0, ProvisionalEstateLayout().PointOr(Anchor::StandingRoomSpawn, {}));
    CHECK(Close(sim.GetState().hour, started + 72.0) && !sim.GetState().failed
        && sim.GetState().hunger == 100.0 && sim.GetState().energy == 0.0 && sim.DozeCount() == dozes);
    Simulation loaded = makeEstate();
    OK(loaded.Deserialize(sim.Serialize()));
    CHECK(!loaded.GetState().failed && loaded.GetState().energy == 0);
    OK(sim.GrantItems(Item::Berries, 2));
    OK(sim.Eat(Item::Berries));
    CHECK(sim.GetState().energy > 0 && !sim.GetState().failed);
    OK(sim.SetEnergy(9.99));
    CHECK(!sim.CanSprint() && sim.CheckExertion(0.5).message == "Too tired.");
    OK(sim.SetEnergy(10.0));
    CHECK(Close(Exertion::WalkSpeedFactor(sim.GetState().energy), 1.0) && !sim.CanSprint());
    OK(sim.SetEnergy(24.99));
    CHECK(!sim.CanSprint());
    OK(sim.SetEnergy(25.0));
    OK(sim.CanSprint());
    OK(sim.CheckExertion(0.5));

    std::string payload = sim.Serialize();
    payload.erase(0, payload.find('\n') + 1);
    const State& state = sim.GetState();
    payload.replace(0, payload.find('\n'), std::to_string(state.hour) + " " + std::to_string(state.dayMinutes)
        + " 0 0 1 " + std::to_string(state.nextId));
    Simulation recovered = makeEstate();
    OK(recovered.Deserialize(Envelope(payload)));
    CHECK(!recovered.GetState().failed && recovered.GetState().hunger == 100
        && recovered.GetState().energy == 0);
}

void NewestValidRecoveryPreference()
{
    const FHomesteadSavePreference oldRecovery{100, 18, 8};
    const FHomesteadSavePreference newerAuto{200, 10, 2};
    CHECK(IsNewerHomesteadSave(newerAuto, oldRecovery));
    CHECK(!IsNewerHomesteadSave(oldRecovery, newerAuto));
    CHECK(IsNewerHomesteadSave({200, 11, 8}, newerAuto));
    CHECK(!IsNewerHomesteadSave({200, 9, 0}, newerAuto));
    CHECK(IsNewerHomesteadSave({200, 10, 0}, newerAuto));

    const std::pair<FHomesteadSavePreference, bool> files[] = {
        {{300, 19, 0}, false}, // corrupt newest file
        {oldRecovery, true},
        {newerAuto, true},
        {{150, 2, 4}, true}
    };
    const FHomesteadSavePreference* best = nullptr;
    for (const auto& file : files)
        if (file.second && (!best || IsNewerHomesteadSave(file.first, *best)))
            best = &file.first;
    CHECK(best && best->SavedAtUtc == newerAuto.SavedAtUtc
        && best->SavedRevision == newerAuto.SavedRevision);
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
    CHECK(overgrowth >= 60 && salvage == 6 && doorway >= 5 && rearGap >= 2 && teases >= 4);
    CHECK(nearestSalvage < 700.0);
    // Pickable blackberry brambles (540000+): plenty of them, two within sight of the front door,
    // none in the ruin, and none crowding another placement.
    int berries = 0, berriesByDoor = 0;
    const auto& all = ProvisionalEstatePlacements().placements;
    for (const auto& placement : all)
    {
        if (placement.id < 540000 || placement.id >= 550000) continue;
        CHECK(placement.kind == ResourceKind::BerryBush);
        CHECK(PointInPolygon(boundary, placement.position) && !PointInPolygon(manor, placement.position));
        ++berries;
        berriesByDoor += std::hypot(placement.position.x - frontDoor.x, placement.position.y - frontDoor.y) < 2000.0;
        for (const auto& other : all)
            if (other.id != placement.id)
                CHECK(std::hypot(other.position.x - placement.position.x, other.position.y - placement.position.y) >= 300.0);
    }
    CHECK(berries >= 36 && berriesByDoor >= 2);
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
    // The hoe blade comes second (Jenny's playtest): she can start a garden on her first morning.
    CHECK(NextSalvageHead(sim.GetState()) == Item::RustedHoeBlade);
    UnchangedFailure(sim, [&] { return sim.Craft(Recipe::HaftBillhook, at); });
    OK(sim.Harvest(bough, at));
    CHECK(sim.Count(Item::Branch) == 3 && sim.Count(Item::Kindling) == 1);

    // Haft a tool: one rusted head and two branches, by hand, no knife and no station.
    const double energy = sim.GetState().energy;
    OK(sim.Craft(Recipe::HaftBillhook, at));
    CHECK(sim.Count(Item::Billhook) == 1 && sim.Count(Item::RustedBillhookHead) == 0 && sim.Count(Item::Branch) == 1);
    CHECK(Close(sim.GetState().energy, energy - Exertion::CraftEnergy));
    CHECK(sim.GetToolTier(ToolKind::Billhook) == ToolTier::Worn);
    // The hoe blade comes second (Jenny's playtest): she can start a garden on her first morning.
    CHECK(NextSalvageHead(sim.GetState()) == Item::RustedHoeBlade);

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
    // A section this build doesn't know came from a newer build: refused as such, not as corrupt.
    const Result unknown = rejected.Deserialize(Envelope(withoutTools + "shovels 1 0\n"));
    CHECK(!unknown && unknown.code == ResultCode::NewerBuild
        && unknown.message.find("newer build") != std::string::npos);
    OK(rejected.Deserialize(Envelope(withoutTools + "tools 8 0 0 0 0 1 0 3 3\n")));
    CHECK(rejected.GetToolTier(ToolKind::Billhook) == ToolTier::Iron);
}

// The scythe's swish plays once per sweep that cuts something (the controller keys it on MowSweep's
// count, at blade contact): one call mows every tuft in the arc; the same sweep again cuts nothing.
void ScytheSweepMowsEachTuftOnce()
{
    const Point at = ProvisionalEstateLayout().PointOr(Anchor::StandingRoomSpawn, {});
    EstatePlacements table;
    table.bakeVersion = 7;
    int next = EstatePlacementIdBase + 20200;
    const auto add = [&](ResourceKind kind, double dx, double dy)
    {
        table.placements.push_back({next++, kind, {at.x + dx, at.y + dy}, 0, 0, 1, 0});
        return table.placements.back().id;
    };
    add(ResourceKind::TallGrass, 60, 0);
    add(ResourceKind::Weeds, 100, 30);
    add(ResourceKind::TallGrass, 130, -40);
    const int bramble = add(ResourceKind::BrambleThin, 90, 0);
    const int behind = add(ResourceKind::TallGrass, -120, 0);
    Simulation sim;
    OK(sim.NewEstateGame(ProvisionalEstateLayout(), table));
    OK(sim.GrantItems(Item::Scythe, 1));
    const auto arc = sim.ScytheArcTargets(at, {1, 0});
    CHECK(arc.size() == 3);
    const double energy = sim.GetState().energy;
    const int hay = sim.Count(Item::Hay);
    const auto sweep = sim.MowSweep(arc, at);
    CHECK(sweep.mown == 3 && sweep.problem.empty());
    for (const int id : arc) CHECK(PlacedNode(sim, id).cleared);
    CHECK(Close(sim.GetState().energy, energy - 0.9, 1e-9));
    CHECK(sim.Count(Item::Hay) >= hay + 2 && sim.Count(Item::Weeds) >= 1);

    // The same sweep again cuts nothing, spends nothing and names why: no swish.
    const std::string before = sim.Serialize();
    const auto again = sim.MowSweep(arc, at);
    CHECK(again.mown == 0 && !again.problem.empty() && sim.Serialize() == before);
    CHECK(sim.MowSweep({}, at).mown == 0 && sim.Serialize() == before);

    // A target the scythe can't cut is refused on its own; the rest of the sweep still mows.
    const auto mixed = sim.MowSweep({bramble, behind}, at);
    CHECK(mixed.mown == 1 && mixed.problem.find("scythe won't clear") != std::string::npos);
    CHECK(PlacedNode(sim, behind).cleared && !PlacedNode(sim, bramble).cleared);
}

// Hold-to-repeat (Homestead::ToolRepeat): plays a held press the way AHomesteadController does. Each blow
// is counted, the last one clears (ClearOvergrowth), and AfterBlow decides whether another follows.
// heldFor: how many blows the button stays down through (-1: never released); between(blows) runs
// after each blow so a test can drain energy or turn her away.
struct HeldRun
{
    int blows = 0;
    Result cleared;
    Result refusal{true, "", ResultCode::None, 0};
};

HeldRun HoldTool(Simulation& sim, int target, Item tool, Point at, Point facing, int heldFor,
    const std::function<void(int)>& between = {})
{
    HeldRun run;
    int landed = 0;
    for (;;)
    {
        ++run.blows;
        if (++landed >= sim.OvergrowthSwings(target))
        {
            run.cleared = sim.ClearOvergrowth(target, tool, at);
            landed = 0;
        }
        if (between) between(run.blows);
        ToolRepeat::Held now;
        now.held = heldFor < 0 || run.blows < heldFor;
        now.sameTool = sim.Count(tool) > 0;
        now.aimed = sim.FindAimedOvergrowth(at, facing, tool);
        const auto decision = ToolRepeat::AfterBlow(sim, target, tool, at, now);
        if (!decision.more || run.blows > 50)
        {
            run.refusal = decision.refusal;
            return run;
        }
    }
}

void HeldToolRepeatsUntilClear()
{
    const Point at = ProvisionalEstateLayout().PointOr(Anchor::StandingRoomSpawn, {});
    const Point ahead{1, 0};
    EstatePlacements table;
    table.bakeVersion = 7;
    int next = EstatePlacementIdBase + 20500;
    const auto add = [&](ResourceKind kind, double dx, double dy)
    {
        table.placements.push_back({next++, kind, {at.x + dx, at.y + dy}, 0, 0, 1, 0});
        return table.placements.back().id;
    };
    const int stump = add(ResourceKind::StumpMedium, 70, 0);
    const int beside = add(ResourceKind::StumpMedium, 110, 25);
    const int rubble = add(ResourceKind::Rubble, 0, -90);
    const auto fresh = [&](Simulation& sim)
    {
        OK(sim.NewEstateGame(ProvisionalEstateLayout(), table));
        OK(sim.GrantItems(Item::Hatchet, 1));
        OK(sim.GrantItems(Item::Pickaxe, 1));
        OK(sim.SetEnergy(100.0));
    };

    // Only the overgrowth tools repeat; the hoe, can, lamp and food never do.
    CHECK(ToolRepeat::Repeats(Item::Hatchet) && ToolRepeat::Repeats(Item::Billhook)
        && ToolRepeat::Repeats(Item::Pickaxe) && ToolRepeat::Repeats(Item::Scythe));
    CHECK(!ToolRepeat::Repeats(Item::DiggingStick) && !ToolRepeat::Repeats(Item::WateringCan)
        && !ToolRepeat::Repeats(Item::OilLamp) && !ToolRepeat::Repeats(Item::Berries));

    // Held: blow after blow on the stump she aimed at, exactly as many as it needs, cleared once, then stop.
    {
        Simulation sim;
        fresh(sim);
        CHECK(sim.FindAimedOvergrowth(at, ahead, Item::Hatchet) == stump);
        const int needed = sim.OvergrowthSwings(stump);
        CHECK(needed == 5);
        const int wood = sim.Count(Item::Firewood) + sim.Count(Item::Branch) + sim.Count(Item::Kindling);
        const auto run = HoldTool(sim, stump, Item::Hatchet, at, ahead, -1);
        CHECK(run.blows == needed && run.cleared.ok && run.refusal.ok);
        CHECK(PlacedNode(sim, stump).cleared);
        CHECK(sim.Count(Item::Firewood) + sim.Count(Item::Branch) + sim.Count(Item::Kindling) > wood);
        // Never on to the next stump, even though it's now the one ahead of her and still held.
        CHECK(!PlacedNode(sim, beside).cleared);
        CHECK(sim.FindAimedOvergrowth(at, ahead, Item::Hatchet) == beside);
        ToolRepeat::Held still;
        still.held = still.sameTool = true;
        still.aimed = beside;
        CHECK(!ToolRepeat::AfterBlow(sim, stump, Item::Hatchet, at, still).more);
    }
    // A click is exactly one blow, and letting go mid-swing finishes only that blow: nothing clears,
    // nothing is spent, and she stops quietly (the controller then says how many swings are left).
    for (const int heldFor : {0, 1, 3})
    {
        Simulation sim;
        fresh(sim);
        const double energy = sim.GetState().energy;
        const auto run = HoldTool(sim, stump, Item::Hatchet, at, ahead, heldFor);
        CHECK(run.blows == std::max(1, heldFor) && run.refusal.ok && !PlacedNode(sim, stump).cleared);
        CHECK(sim.GetState().energy == energy);
    }
    // Below the 10% energy floor the next blow is refused with Integration's "Too tired." and she stops.
    {
        Simulation sim;
        fresh(sim);
        const auto run = HoldTool(sim, stump, Item::Hatchet, at, ahead, -1,
            [&](int blows) { if (blows == 2) OK(sim.SetEnergy(9.5)); });
        CHECK(run.blows == 2 && !run.refusal.ok && run.refusal.message == "Too tired.");
        CHECK(!PlacedNode(sim, stump).cleared);
    }
    // Turning away, putting the tool down or opening the book ends the run.
    {
        Simulation sim;
        fresh(sim);
        ToolRepeat::Held now;
        now.held = now.sameTool = true;
        now.aimed = stump;
        CHECK(ToolRepeat::AfterBlow(sim, stump, Item::Hatchet, at, now).more);
        now.aimed = -1;
        CHECK(!ToolRepeat::AfterBlow(sim, stump, Item::Hatchet, at, now).more);
        now.aimed = stump;
        now.sameTool = false;
        CHECK(!ToolRepeat::AfterBlow(sim, stump, Item::Hatchet, at, now).more);
        now.sameTool = true;
        now.menuOpen = true;
        CHECK(!ToolRepeat::AfterBlow(sim, stump, Item::Hatchet, at, now).more);
        // The hoe doesn't repeat even if held on something.
        now.menuOpen = false;
        CHECK(!ToolRepeat::AfterBlow(sim, stump, Item::DiggingStick, at, now).more);
    }
    // The pickaxe on rubble: two blows held, one reward.
    {
        Simulation sim;
        fresh(sim);
        const Point down{0, -1};
        CHECK(sim.FindAimedOvergrowth(at, down, Item::Pickaxe) == rubble);
        const int stone = sim.Count(Item::Stone);
        const auto run = HoldTool(sim, rubble, Item::Pickaxe, at, down, -1);
        CHECK(run.blows == sim.OvergrowthSwings(rubble) && run.blows == 2 && run.cleared.ok);
        CHECK(PlacedNode(sim, rubble).cleared && sim.Count(Item::Stone) > stone);
    }

    // The strike clips loop one stroke cycle (axe_fell.FRAMES: loop 44-80, contacts at 34 and 70). A held
    // blow can join only before the clip leaves its last cycle for the recovery.
    const float loopStart = 44.0f / 30.0f, loop = 36.0f / 30.0f, margin = 2.0f / 30.0f;
    CHECK(ToolRepeat::CanAddStroke(34.0f / 30.0f, 1, loopStart, loop, margin));
    CHECK(ToolRepeat::CanAddStroke(41.0f / 30.0f, 1, loopStart, loop, margin));
    CHECK(!ToolRepeat::CanAddStroke(42.0f / 30.0f, 1, loopStart, loop, margin));
    CHECK(!ToolRepeat::CanAddStroke(50.0f / 30.0f, 1, loopStart, loop, margin));
    CHECK(ToolRepeat::CanAddStroke(70.0f / 30.0f, 2, loopStart, loop, margin));
    CHECK(!ToolRepeat::CanAddStroke(79.0f / 30.0f, 2, loopStart, loop, margin));
    CHECK(Close(ToolRepeat::JoinDeadline(3, loopStart, loop, 0.0f), 116.0 / 30.0, 1e-5));
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
    // Below 10 Energy, even a tier-qualified swing is refused without a state change.
    Simulation tired;
    OK(tired.NewEstateGame(ProvisionalEstateLayout(), placements));
    OK(tired.GrantItems(Item::Hatchet, 1));
    OK(tired.SetToolTier(ToolKind::Axe, ToolTier::Iron));
    OK(tired.SetEnergy(9.9));
    UnchangedFailure(tired, [&] { return tired.ClearOvergrowth(large, Item::Hatchet, at); });
    CHECK(tired.CheckOvergrowth(large, Item::Hatchet, at).message == "Too tired.");

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
    // Each pile gives the next missing head, billhook then hoe first, then only scrap.
    const Item order[] = {Item::RustedBillhookHead, Item::RustedHoeBlade, Item::RustedAxeHead,
        Item::RustedScytheBlade, Item::RustedPickHead};
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
    OK(loaded.SetEnergy(30.0));
    OK(loaded.ClearOvergrowth(regrownId, Item::Scythe, PlacedNode(loaded, regrownId).position));
}

void RuinTimbersAreChoppedWithTheAxe()
{
    // The manor's fallen roof timbers looked clearable but were scenery. They're estate placements
    // now (582012-582013), placed after every earlier section so no other id, spot or skip moves,
    // and cut up with the worn axe in a few blows.
    const EstateLayout& layout = ProvisionalEstateLayout();
    const auto& table = ProvisionalEstatePlacements().placements;
    double manorX = 1e18, manorY = 1e18;
    for (const Point& corner : layout.FindPolygon(Anchor::ManorFootprint)->points)
    {
        manorX = std::min(manorX, corner.x);
        manorY = std::min(manorY, corner.y);
    }
    std::size_t lastForage = 0, firstTimber = table.size();
    for (std::size_t i = 0; i < table.size(); ++i)
    {
        if (table[i].id >= 582100 && table[i].id < 582300) lastForage = i;
        if (table[i].kind == ResourceKind::RuinTimbers) firstTimber = std::min(firstTimber, i);
    }
    std::size_t rackAt = table.size();
    for (std::size_t i = 0; i < table.size(); ++i)
        if (table[i].id == 520006) rackAt = i;
    CHECK(firstTimber > lastForage && firstTimber < rackAt && rackAt < table.size());
    // Only the lake trail's forage (582300-582399) comes after the rack.
    for (std::size_t i = rackAt + 1; i < table.size(); ++i) CHECK(table[i].id >= 582300 && table[i].id < 582400);
    Simulation sim;
    OK(sim.NewEstateGame(layout, ProvisionalEstatePlacements()));
    for (int id : {582012, 582013})
    {
        const RuinDebris::Spot* spot = RuinDebris::Find(id);
        CHECK(spot && spot->kind == ResourceKind::RuinTimbers && std::string(spot->mesh) == "RuinFallenTimbers");
        CHECK(RuinDebris::Replaces("RuinFallenTimbers", spot->u, spot->v));
        const ResourceNode& node = PlacedNode(sim, id);
        CHECK(node.kind == ResourceKind::RuinTimbers && !node.cleared);
        CHECK(std::hypot(node.position.x - (manorX + spot->v), node.position.y - (manorY + spot->u)) <= 150.0 + 1e-6);
    }
    CHECK(std::string(ResourceName(ResourceKind::RuinTimbers)) == "Fallen roof timbers");
    CHECK(HandGatherPose(ResourceKind::RuinTimbers) == GatherPose::None);

    // Too heavy to lift by hand, and the billhook and pickaxe don't cut oak: nothing changes.
    const ResourceNode hall = PlacedNode(sim, 582012);
    OK(sim.GrantItems(Item::Billhook, 1));
    OK(sim.GrantItems(Item::Pickaxe, 1));
    for (Item tool : {Item::Count, Item::Billhook, Item::Pickaxe})
        UnchangedFailure(sim, [&] { return sim.ClearOvergrowth(hall.id, tool, hall.position); });
    // The worn axe: three blows, then timber and firewood once.
    OK(sim.GrantItems(Item::Hatchet, 1));
    OK(sim.CheckOvergrowth(hall.id, Item::Hatchet, hall.position));
    CHECK(sim.OvergrowthSwings(hall.id) == 3);
    const int timber = sim.Count(Item::Timber), firewood = sim.Count(Item::Firewood);
    const double energy = sim.GetState().energy;
    OK(sim.ClearOvergrowth(hall.id, Item::Hatchet, hall.position));
    CHECK(PlacedNode(sim, hall.id).cleared);
    CHECK(sim.Count(Item::Timber) >= timber + 1 && sim.Count(Item::Timber) <= timber + 2);
    CHECK(sim.Count(Item::Firewood) >= firewood + 2 && sim.Count(Item::Firewood) <= firewood + 3);
    CHECK(Close(sim.GetState().energy, energy - FindOvergrowth(ResourceKind::RuinTimbers)->energy));
    UnchangedFailure(sim, [&] { return sim.ClearOvergrowth(hall.id, Item::Hatchet, hall.position); });
    OK(sim.SetToolTier(ToolKind::Axe, ToolTier::Iron));
    CHECK(sim.OvergrowthSwings(582013) == 2);

    // Cleared stays cleared across a reload; the other still lies there.
    Simulation loaded;
    loaded.SetLayout(layout);
    loaded.SetPlacements(ProvisionalEstatePlacements());
    OK(loaded.Deserialize(sim.Serialize()));
    CHECK(PlacedNode(loaded, 582012).cleared && !PlacedNode(loaded, 582013).cleared);

    // A save from before (no timbers in its table, a slate heap cleared) loads with both lying there
    // and exactly its own clearances.
    EstatePlacements older = ProvisionalEstatePlacements();
    older.placements.erase(std::remove_if(older.placements.begin(), older.placements.end(),
        [](const EstatePlacement& placement) { return placement.kind == ResourceKind::RuinTimbers; }), older.placements.end());
    Simulation original;
    original.SetPlacements(older);
    OK(original.NewEstateGame(layout, older));
    OK(original.ClearOvergrowth(582000, Item::Count, PlacedNode(original, 582000).position));
    Simulation upgraded;
    upgraded.SetLayout(layout);
    upgraded.SetPlacements(ProvisionalEstatePlacements());
    OK(upgraded.Deserialize(original.Serialize()));
    CHECK(!PlacedNode(upgraded, 582012).cleared && !PlacedNode(upgraded, 582013).cleared && PlacedNode(upgraded, 582000).cleared);
    int cleared = 0;
    for (const auto& node : upgraded.GetState().resources) cleared += node.cleared;
    CHECK(cleared == 1);
}

void WrongOrUnderTierToolIsNonActionable()
{
    // Jenny, 2026-09-29: a wrong or too-worn tool must not look like it's doing anything. Every
    // overgrowth kind on the estate, against every clearing tool at every tier: the swing is allowed
    // only for its own tool at or above the tier it (or its placement) asks for; otherwise the
    // attempt costs no energy, lands no swing and yields nothing.
    Simulation sim;
    OK(sim.NewEstateGame(ProvisionalEstateLayout(), ProvisionalEstatePlacements()));
    const Item tools[] = {Item::Hatchet, Item::Billhook, Item::Scythe, Item::Pickaxe};
    for (Item tool : tools) OK(sim.GrantItems(tool, 1));
    std::set<std::pair<int, int>> seen;
    int refusals = 0;
    for (const ResourceNode& node : std::vector<ResourceNode>(sim.GetState().resources))
    {
        const auto* info = FindOvergrowth(node.kind);
        if (!info || node.cleared || !seen.insert({static_cast<int>(node.kind), static_cast<int>(node.minTier)}).second) continue;
        const ToolTier needed = std::max(info->minTier, node.minTier);
        for (int tier = 0; tier < ToolTierCount; ++tier)
        {
            for (Item tool : tools) OK(sim.SetToolTier(ToolForItem(tool), static_cast<ToolTier>(tier)));
            for (Item tool : tools)
            {
                const bool fits = ToolForItem(tool) == info->tool && static_cast<ToolTier>(tier) >= needed;
                const auto check = sim.CheckOvergrowth(node.id, tool, node.position);
                CHECK(check.ok == fits);
                if (fits) continue;
                ++refusals;
                if (ToolForItem(tool) == info->tool) CHECK(check.code == ResultCode::ToolTier && check.message == NeedsToolMessage(info->tool, needed));
                const int swings = sim.OvergrowthSwings(node.id);
                UnchangedFailure(sim, [&] { return sim.ClearOvergrowth(node.id, tool, node.position); });
                CHECK(sim.OvergrowthSwings(node.id) == swings && !PlacedNode(sim, node.id).cleared);
            }
        }
    }
    CHECK(seen.size() >= 20 && refusals > 100);
}

void EveryHandGatherHasItsOwnPose()
{
    // Jenny, 2026-09-29: the generic slight-knee-bend gather looked wrong. Everything she can take
    // by hand has a real pose (sticks, the stone kneel or the hip pouch, or the reed knife), and the
    // things only a tool clears have none, so no path falls back to the old bend.
    for (int i = 0; i < static_cast<int>(ResourceKind::Count); ++i)
    {
        const auto kind = static_cast<ResourceKind>(i);
        const GatherPose pose = HandGatherPose(kind);
        if (const auto* info = FindOvergrowth(kind)) CHECK(info->byHand == (pose != GatherPose::None));
        else CHECK((kind == ResourceKind::ForestTree) == (pose == GatherPose::None));
    }
    for (auto kind : {ResourceKind::Stones, ResourceKind::SalvagePile, ResourceKind::RubbishHeap, ResourceKind::SlateHeap})
        CHECK(HandGatherPose(kind) == GatherPose::Stones);
    for (auto kind : {ResourceKind::Branches, ResourceKind::FallenBranch, ResourceKind::BrokenCrate,
             ResourceKind::BrokenBarrel, ResourceKind::RottenPlanks})
        CHECK(HandGatherPose(kind) == GatherPose::Sticks);
    for (auto kind : {ResourceKind::BerryBush, ResourceKind::Roots, ResourceKind::Flowers, ResourceKind::Primroses,
             ResourceKind::Bluebells, ResourceKind::WildDaffodils, ResourceKind::WildGarlic, ResourceKind::Weeds,
             ResourceKind::Nettles})
        CHECK(HandGatherPose(kind) == GatherPose::Pouch);
    CHECK(HandGatherPose(ResourceKind::Reeds) == GatherPose::Reeds);
    CHECK(HandGatherPose(ResourceKind::TallGrass) == GatherPose::None && HandGatherPose(ResourceKind::Sapling) == GatherPose::None);
}

void BillhookBlowLandsOnlyOnItsOwnHack()
{
    // Code review 0930: a click during the last hack's follow-through (its clip already past the
    // contact) must not land on the new target. Only a hack started after the press counts, at its own
    // contact; walking up keeps it waiting; a hack that never starts, or stops early, is dropped.
    using namespace SwingTiming;
    const float contact = 1.25f;
    const double grace = 0.4;
    // The old hack (start 4) is at 1.4 s when she clicks (starts recorded as 4): it never lands.
    for (float phase : {1.3f, 1.5f, 1.8f})
        CHECK(Advance(4, 4, true, phase, contact, false, 0.1, grace) == Step::Wait);
    // It ends; her new request was refused while it played, so no new hack: dropped, no blow.
    CHECK(Advance(4, 4, false, -1.0f, contact, false, 0.5, grace) == Step::Drop);
    // The same click, but walking up first: waits through the approach, then its own hack lands.
    CHECK(Advance(4, 4, true, 1.6f, contact, true, 0.0, grace) == Step::Wait);
    CHECK(Advance(4, 4, false, -1.0f, contact, true, 0.0, grace) == Step::Wait);
    CHECK(Advance(4, 5, true, 0.2f, contact, false, 0.1, grace) == Step::Wait);
    CHECK(Advance(4, 5, true, 1.25f, contact, false, 1.2, grace) == Step::Land);
    // A fresh press from rest: waits for the clip, lands at contact.
    CHECK(Advance(9, 9, false, -1.0f, contact, false, 0.1, grace) == Step::Wait);
    CHECK(Advance(9, 10, true, 1.0f, contact, false, 1.0, grace) == Step::Wait);
    CHECK(Advance(9, 10, true, 1.3f, contact, false, 1.3, grace) == Step::Land);
    // Its own hack cancelled before the contact (she moved): dropped.
    CHECK(Advance(9, 10, false, -1.0f, contact, false, 0.9, grace) == Step::Drop);
}

void RuinDebrisIsClearable()
{
    // Jenny saw slate and rubble heaps in the manor that looked clearable but weren't. The loose
    // ones are estate placements now: fallen slates stacked aside by hand, loose granite broken with
    // the worn pickaxe, each cleared once and for good, and none in the way of the standing room,
    // its door or the salvage piles.
    const EstateLayout& layout = ProvisionalEstateLayout();
    const auto& boundary = layout.FindPolygon(Anchor::EstateBoundary)->points;
    double manorX = 1e18, manorY = 1e18;
    for (const Point& corner : layout.FindPolygon(Anchor::ManorFootprint)->points)
    {
        manorX = std::min(manorX, corner.x);
        manorY = std::min(manorY, corner.y);
    }
    Simulation sim;
    OK(sim.NewEstateGame(layout, ProvisionalEstatePlacements()));
    const Point room = layout.PointOr(Anchor::StandingRoomOrigin, {});
    int slate = 0, granite = 0, timbers = 0;
    for (const RuinDebris::Spot& spot : RuinDebris::Spots)
    {
        CHECK(spot.id >= RuinDebris::FirstId && spot.id <= RuinDebris::LastId);
        const ResourceNode& node = PlacedNode(sim, spot.id);
        CHECK(node.kind == spot.kind && !node.cleared);
        CHECK(std::hypot(node.position.x - (manorX + spot.v), node.position.y - (manorY + spot.u)) <= 150.0 + 1e-6);
        CHECK(PointInPolygon(boundary, node.position));
        // Clear of the standing room (U 2400-3000, V 0-600) and its doorway.
        CHECK(std::abs(node.position.x - room.x) > 400.0 || std::abs(node.position.y - room.y) > 400.0);
        const auto* info = FindOvergrowth(node.kind);
        CHECK(info);
        if (node.kind == ResourceKind::SlateHeap) { ++slate; CHECK(info->byHand && IsRubbish(node.kind)); }
        else if (node.kind == ResourceKind::RuinTimbers)
        {
            ++timbers;
            CHECK(!info->byHand && info->tool == ToolKind::Axe && info->minTier == ToolTier::Worn);
        }
        else { ++granite; CHECK(node.kind == ResourceKind::Rubble && info->tool == ToolKind::Pickaxe && info->minTier == ToolTier::Worn); }
    }
    CHECK(slate == 6 && granite == 6 && timbers == 2 && RuinDebris::SpotCount == 14);
    CHECK(std::string(ResourceName(ResourceKind::SlateHeap)) == "Fallen slates");
    // Standing at any salvage pile, the pile itself is what her hands find.
    for (int pile = 520001; pile <= 520005; ++pile)
        CHECK(sim.FindNearestOvergrowth(PlacedNode(sim, pile).position, Overgrowth::Reach, Item::Count) == pile);

    // Slates by hand: stone, and sometimes the lead and nails that came down with them.
    const ResourceNode heap = PlacedNode(sim, 582000);
    const int stone = sim.Count(Item::Stone);
    OK(sim.ClearOvergrowth(heap.id, Item::Count, heap.position));
    CHECK(PlacedNode(sim, heap.id).cleared && sim.Count(Item::Stone) >= stone + 1 && sim.Count(Item::Stone) <= stone + 2);
    CHECK(!sim.ClearOvergrowth(heap.id, Item::Count, heap.position).ok);
    // Granite needs the pickaxe; bare hands are refused and nothing changes.
    const ResourceNode cobbles = PlacedNode(sim, 582006);
    const auto before = sim.GetState().inventory;
    CHECK(!sim.ClearOvergrowth(cobbles.id, Item::Count, cobbles.position).ok);
    CHECK(sim.GetState().inventory == before && !PlacedNode(sim, cobbles.id).cleared);
    OK(sim.GrantItems(Item::Pickaxe, 1));
    OK(sim.ClearOvergrowth(cobbles.id, Item::Pickaxe, cobbles.position));
    CHECK(PlacedNode(sim, cobbles.id).cleared);

    // Cleared heaps stay cleared across a save and reload; the rest are still there.
    Simulation loaded;
    loaded.SetLayout(layout);
    loaded.SetPlacements(ProvisionalEstatePlacements());
    OK(loaded.Deserialize(sim.Serialize()));
    CHECK(PlacedNode(loaded, 582000).cleared && PlacedNode(loaded, 582006).cleared);
    CHECK(!PlacedNode(loaded, 582001).cleared && !PlacedNode(loaded, 582011).cleared);

    // A save from before the heaps were clearable loads with every heap standing.
    EstatePlacements older = ProvisionalEstatePlacements();
    older.placements.erase(std::remove_if(older.placements.begin(), older.placements.end(),
        [](const EstatePlacement& placement) { return placement.id >= RuinDebris::FirstId && placement.id <= RuinDebris::LastId; }),
        older.placements.end());
    Simulation original;
    OK(original.NewEstateGame(layout, older));
    Simulation upgraded;
    upgraded.SetLayout(layout);
    upgraded.SetPlacements(ProvisionalEstatePlacements());
    OK(upgraded.Deserialize(original.Serialize()));
    for (const RuinDebris::Spot& spot : RuinDebris::Spots) CHECK(!PlacedNode(upgraded, spot.id).cleared);
}

void EveryLiveWeedIsOnOpenGround()
{
    // Jenny saw "Weeds [E] Pull" with nothing on the ground. Every pullable weed or nettle on a new
    // estate stands on open estate ground where its clump can be drawn: outside the ruin's footprint,
    // clear of the standing room and every other structure, pulled by hand, and no two stacked on
    // one spot. Pulling one takes it (and its prompt) away for good.
    Simulation sim;
    OK(sim.NewEstateGame(ProvisionalEstateLayout(), ProvisionalEstatePlacements()));
    const EstateLayout& layout = ProvisionalEstateLayout();
    const auto& boundary = layout.FindPolygon(Anchor::EstateBoundary)->points;
    const auto& manor = layout.FindPolygon(Anchor::ManorFootprint)->points;
    std::vector<const ResourceNode*> weeds;
    for (const auto& node : sim.GetState().resources)
    {
        if (node.cleared || (node.kind != ResourceKind::Weeds && node.kind != ResourceKind::Nettles)) continue;
        weeds.push_back(&node);
        CHECK(FindOvergrowth(node.kind) && FindOvergrowth(node.kind)->byHand);
        CHECK(PointInPolygon(boundary, node.position) && !PointInPolygon(manor, node.position));
        const Footprint spot{node.position, {1.0, 1.0}, 0.0};
        for (const auto& piece : sim.GetState().structures) CHECK(!FootprintsOverlap(spot, StructureFootprint(sim.GetState(), piece)));
    }
    CHECK(weeds.size() >= 100);
    for (std::size_t i = 0; i < weeds.size(); ++i)
        for (std::size_t j = i + 1; j < weeds.size(); ++j)
            CHECK(std::hypot(weeds[i]->position.x - weeds[j]->position.x, weeds[i]->position.y - weeds[j]->position.y) >= 20.0);
    const ResourceNode weed = *weeds.front();
    OK(sim.ClearOvergrowth(weed.id, Item::Count, weed.position));
    CHECK(PlacedNode(sim, weed.id).cleared);
    CHECK(sim.FindNearestOvergrowth(weed.position, 1.0, Item::Count) != weed.id);
}

void ManorClearoutField()
{
    // The ground round the ruin (570000+) is thick with clearables of every early kind, a few that
    // need a better tool, and lanes left open to the doors and the salvage piles.
    const EstateLayout& layout = ProvisionalEstateLayout();
    const auto& boundary = layout.FindPolygon(Anchor::EstateBoundary)->points;
    const auto& manor = layout.FindPolygon(Anchor::ManorFootprint)->points;
    const Point room = layout.PointOr(Anchor::StandingRoomOrigin, {});
    const Point frontDoor = EstateManorFrontDoor(layout);
    const auto& all = ProvisionalEstatePlacements().placements;
    const auto wallDistance = [&](Point p)
    {
        double x0 = 1e9, x1 = -1e9, y0 = 1e9, y1 = -1e9;
        for (const Point& corner : manor)
        {
            x0 = std::min(x0, corner.x); x1 = std::max(x1, corner.x);
            y0 = std::min(y0, corner.y); y1 = std::max(y1, corner.y);
        }
        return std::hypot(std::max({0.0, x0 - p.x, p.x - x1}), std::max({0.0, y0 - p.y, p.y - y1}));
    };
    int field = 0, nearHouse = 0, teases = 0, rubbish = 0;
    std::set<ResourceKind> kinds;
    std::set<int> ids;
    for (const auto& placement : all)
    {
        CHECK(ids.insert(placement.id).second);
        if (placement.id < 570000 || placement.id >= 580000) continue;
        const auto* info = FindOvergrowth(placement.kind);
        CHECK(info != nullptr && placement.kind != ResourceKind::SalvagePile);
        CHECK(PointInPolygon(boundary, placement.position) && !PointInPolygon(manor, placement.position));
        // Not in or against the standing room, and clear of the derelict farm's field.
        CHECK(std::abs(placement.position.x - room.x) > 450.0 || std::abs(placement.position.y - room.y) > 450.0);
        CHECK(!(placement.position.x > -22500.0 && placement.position.x < -15900.0
            && placement.position.y > -70800.0 && placement.position.y < -64200.0));
        CHECK(wallDistance(placement.position) < 4000.0);
        for (const auto& other : all)
            if (other.id != placement.id)
                CHECK(std::hypot(other.position.x - placement.position.x, other.position.y - placement.position.y)
                    >= (other.kind == ResourceKind::BerryBush ? 300.0 : 150.0));
        ++field;
        nearHouse += wallDistance(placement.position) < 2000.0;
        teases += info->minTier > ToolTier::Worn;
        rubbish += IsRubbish(placement.kind);
        kinds.insert(placement.kind);
        // The lane out of the front door and the salvage piles outside keep clear ground.
        for (double out = 0.0; out <= 550.0; out += 50.0)
            CHECK(std::hypot(placement.position.x - (frontDoor.x - out), placement.position.y - frontDoor.y) > 160.0);
    }
    for (const auto& pile : all)
        if (pile.kind == ResourceKind::SalvagePile)
            for (const auto& placement : all)
                if (placement.id >= 570000 && placement.id < 580000)
                    CHECK(std::hypot(pile.position.x - placement.position.x, pile.position.y - placement.position.y) > 200.0);
    CHECK(field >= 300 && nearHouse >= 150 && teases >= 10 && teases * 10 < field && rubbish >= 20);
    for (auto kind : {ResourceKind::Weeds, ResourceKind::Nettles, ResourceKind::TallGrass, ResourceKind::BrambleThin,
        ResourceKind::Sapling, ResourceKind::StumpSmall, ResourceKind::StumpMedium, ResourceKind::StumpLarge,
        ResourceKind::SmallRock, ResourceKind::Rubble, ResourceKind::Boulder, ResourceKind::BrokenCrate,
        ResourceKind::BrokenBarrel, ResourceKind::RubbishHeap, ResourceKind::RottenPlanks})
        CHECK(kinds.count(kind) == 1);
    Simulation estate;
    OK(estate.NewEstateGame(layout, ProvisionalEstatePlacements()));
    int present = 0;
    for (const auto& node : estate.GetState().resources) present += node.id >= 570000 && node.id < 580000 && !node.cleared;
    CHECK(present == field);
}

// Weeding by hand kneels and pulls, and commits once at the second root: the controller probes a copy
// first (refusing before she kneels), then runs the real transaction at
// AHomesteadCharacter::PullWeedsCommit; a pull cancelled before then never reaches the simulation.
// This pins the two transactions it routes: a weed node's Harvest (its weeds, Energy once) and a garden
// square's Weed (no yield).
void WeedPullCommitsOnce()
{
    const Point at = ProvisionalEstateLayout().PointOr(Anchor::StandingRoomSpawn, {});
    EstatePlacements table;
    table.bakeVersion = 7;
    table.placements.push_back({EstatePlacementIdBase + 20300, ResourceKind::Weeds, {at.x, at.y + 39.0}, 0, 0, 1, 0});
    table.placements.push_back({EstatePlacementIdBase + 20301, ResourceKind::Nettles, {at.x + 80.0, at.y}, 0, 0, 1, 0});
    const int weed = EstatePlacementIdBase + 20300, nettle = EstatePlacementIdBase + 20301;
    Simulation sim;
    OK(sim.NewEstateGame(ProvisionalEstateLayout(), table));
    const std::string untouched = sim.Serialize();
    const auto revision = sim.GetRevision();
    // The probe on a copy says yes and changes nothing (a cancelled pull leaves exactly this).
    Simulation probe = sim;
    OK(probe.Harvest(weed, at));
    CHECK(sim.Serialize() == untouched && sim.GetRevision() == revision && !PlacedNode(sim, weed).cleared);

    const double energy = sim.GetState().energy;
    const int weeds = sim.Count(Item::Weeds);
    OK(sim.Harvest(weed, at));
    CHECK(PlacedNode(sim, weed).cleared && sim.Count(Item::Weeds) == weeds + 1);
    CHECK(Close(sim.GetState().energy, energy - FindOvergrowth(ResourceKind::Weeds)->energy, 1e-9));
    // Pulled once: the next press is refused up front, with nothing spent.
    Simulation again = sim;
    CHECK(!again.Harvest(weed, at));
    UnchangedFailure(sim, [&] { return sim.Harvest(weed, at); });
    // Nettles pull the same way, 1-2 weeds for their own Energy.
    const double beforeNettle = sim.GetState().energy;
    OK(sim.Harvest(nettle, at));
    CHECK(sim.Count(Item::Weeds) >= weeds + 2 && sim.Count(Item::Weeds) <= weeds + 3);
    CHECK(Close(sim.GetState().energy, beforeNettle - FindOvergrowth(ResourceKind::Nettles)->energy, 1e-9));
    Simulation loaded;
    loaded.SetPlacements(table);
    OK(loaded.Deserialize(sim.Serialize()));
    CHECK(PlacedNode(loaded, weed).cleared && PlacedNode(loaded, nettle).cleared);

    // A garden square: weeding by hand clears its weeds and gives nothing.
    Simulation garden;
    Stock(garden, {{Item::DiggingStick, 1}});
    const Point square = CellCenter(-2, -1);
    OK(garden.Till(CellToGarden(-2), CellToGarden(-1), square));
    const int plot = garden.FindNearestPlot(square, 1);
    CHECK(plot != -1);
    UnchangedFailure(garden, [&] { return garden.Weed(plot, square); });
    Edit(garden, [](State& state) { state.plots[0].weeds = 0.6; }, false);
    const auto stock = garden.GetState().inventory;
    const double gardenEnergy = garden.GetState().energy;
    Simulation gardenProbe = garden;
    OK(gardenProbe.Weed(plot, square));
    CHECK(garden.GetState().plots[0].weeds == 0.6);
    OK(garden.Weed(plot, square));
    CHECK(garden.GetState().inventory == stock && garden.GetState().plots[0].weeds == 0.0);
    CHECK(Close(garden.GetState().energy, gardenEnergy - Exertion::WeedEnergy, 1e-9));
    UnchangedFailure(garden, [&] { return garden.Weed(plot, square); });
}

void WeedPullCommitsOnlyOnItsOwnClip()
{
    // Code review of the 09-29 port: a second pull pressed during the first one's tail (after its commit
    // at the second root, while its clip still plays) committed at once, because the first clip's phase
    // was already past the commit beat. A pull only owns the phase once its own kneel has begun.
    using namespace WeedPull;
    const float first = 54.0f / 30.0f, commit = 102.0f / 30.0f, end = 150.0f / 30.0f;
    const double timeout = 4.5;
    // Weed A: queued, starts (count 0 -> 1), thins at the first root, commits once at the second.
    Pending a{0, 0.0, false};
    CHECK(Advance(a, 0, -1.0f, 0.2, first, commit, timeout) == Step::Wait && !a.started);
    CHECK(Advance(a, 1, -1.0f, 0.4, first, commit, timeout) == Step::Wait && !a.started); // Begun, not yet blended in.
    CHECK(Advance(a, 1, 0.5f, 0.9, first, commit, timeout) == Step::Wait && a.started);
    CHECK(Advance(a, 1, first + 0.1f, 2.4, first, commit, timeout) == Step::Thin);
    CHECK(Advance(a, 1, commit, 3.9, first, commit, timeout) == Step::Commit);
    // Weed B pressed during A's tail (A's clip still at 4 s): it waits through the whole tail.
    Pending b{1, 4.0, false};
    for (float tail = commit; tail < end; tail += 0.1f)
        CHECK(Advance(b, 1, tail, 4.0 + (tail - commit), first, commit, timeout) == Step::Wait && !b.started);
    CHECK(Advance(b, 1, -1.0f, 5.7, first, commit, timeout) == Step::Wait); // A ended, B not begun.
    // B's own kneel begins: its early phase doesn't commit, only its own second root does.
    CHECK(Advance(b, 2, 0.1f, 5.9, first, commit, timeout) == Step::Wait && b.started);
    CHECK(Advance(b, 2, commit - 0.01f, 9.2, first, commit, timeout) == Step::Thin);
    CHECK(Advance(b, 2, commit, 9.3, first, commit, timeout) == Step::Commit);
    // Cancelled after it began (she walked off): dropped, nothing committed.
    Pending c{2, 10.0, false};
    CHECK(Advance(c, 3, 1.0f, 11.0, first, commit, timeout) == Step::Wait);
    CHECK(Advance(c, 3, -1.0f, 11.1, first, commit, timeout) == Step::Drop);
    // Never began, or stuck behind a tail that never ends: dropped after the timeout, never committed.
    Pending d{3, 20.0, false};
    CHECK(Advance(d, 3, -1.0f, 24.0, first, commit, timeout) == Step::Wait);
    CHECK(Advance(d, 3, -1.0f, 24.6, first, commit, timeout) == Step::Drop);
    Pending e{3, 30.0, false};
    CHECK(Advance(e, 3, end, 34.6, first, commit, timeout) == Step::Drop);
}

void ClearoutKindsAndSpoiledGround()
{
    const Point spawn = ProvisionalEstateLayout().PointOr(Anchor::StandingRoomSpawn, {});
    const Point at{spawn.x - 1500, spawn.y + 1500};
    EstatePlacements placements;
    placements.bakeVersion = 12;
    int next = EstatePlacementIdBase + 40000;
    const auto add = [&](ResourceKind kind, double dx, double dy)
    {
        placements.placements.push_back({next++, kind, {at.x + dx, at.y + dy}, 0, 0, 1, 0});
        return next - 1;
    };
    const int crate = add(ResourceKind::BrokenCrate, 0, 0);
    const int barrel = add(ResourceKind::BrokenBarrel, 100, 0);
    const int heap = add(ResourceKind::RubbishHeap, 0, 100);
    const int planks = add(ResourceKind::RottenPlanks, -100, 0);
    const int nettles = add(ResourceKind::Nettles, 0, -100);
    const int mown = add(ResourceKind::Nettles, 50, -150);
    const int weeds = add(ResourceKind::Weeds, -100, -100);
    // A stump beside a garden square, not in it: its spoiled ground still covers the square.
    const Point square = GardenCellCenter(GardenCell(at.x + 1000), GardenCell(at.y));
    const int stump = add(ResourceKind::StumpMedium, square.x - at.x + GardenCellSize * 0.5 + 40.0, square.y - at.y);
    Simulation sim;
    OK(sim.NewEstateGame(ProvisionalEstateLayout(), placements));
    // Rubbish, nettles and weeds come away by hand, each clear yielding once.
    for (int id : {crate, barrel, heap, planks, nettles, weeds}) CHECK(sim.CanHarvest(id));
    const auto cleared = sim.Harvest(crate, at);
    OK(cleared);
    CHECK(cleared.message.find("Cleared the broken crate") == 0 && cleared.code != ResultCode::PackOverflow);
    CHECK(sim.Count(Item::Kindling) >= 2 && sim.Count(Item::Kindling) <= 3);
    UnchangedFailure(sim, [&] { return sim.Harvest(crate, at); });
    OK(sim.Harvest(barrel, at));
    CHECK(sim.Count(Item::ScrapIron) >= 1);
    OK(sim.Harvest(heap, at));
    OK(sim.Harvest(planks, at));
    OK(sim.Harvest(nettles, at));
    OK(sim.Harvest(weeds, at));
    CHECK(sim.Count(Item::Weeds) >= 2);
    // Nettles mow with the scythe too; the stump takes the axe, five worn swings.
    OK(sim.GrantItems(Item::Scythe, 1));
    OK(sim.ClearOvergrowth(mown, Item::Scythe, at));
    CHECK(!sim.CanHarvest(stump));
    UnchangedFailure(sim, [&] { return sim.Harvest(stump, square); });
    OK(sim.GrantItems(Item::Hatchet, 1));
    CHECK(sim.OvergrowthSwings(stump) == 5);
    // Tilling or building on spoiled ground is refused, naming what to clear, until it's cleared.
    OK(sim.GrantItems(Item::DiggingStick, 1));
    const int gx = GardenCell(square.x), gy = GardenCell(square.y);
    const auto refused = sim.Till(gx, gy, square);
    CHECK(!refused.ok && refused.message == "Clear the stump here first.");
    UnchangedFailure(sim, [&] { return sim.Till(gx, gy, square); });
    OK(sim.ClearOvergrowth(stump, Item::Hatchet, square));
    CHECK(sim.Count(Item::Firewood) >= 3);
    OK(sim.Till(gx, gy, square));
    // The spoil radius scales with the obstacle.
    CHECK(FindOvergrowth(ResourceKind::Boulder)->spoil > FindOvergrowth(ResourceKind::SmallRock)->spoil);
    CHECK(FindOvergrowth(ResourceKind::StumpLarge)->spoil > FindOvergrowth(ResourceKind::StumpSmall)->spoil);
    CHECK(FindOvergrowth(ResourceKind::StumpMedium)->minTier == ToolTier::Worn);
    for (auto kind : {ResourceKind::BrokenCrate, ResourceKind::BrokenBarrel, ResourceKind::RubbishHeap, ResourceKind::RottenPlanks})
        CHECK(IsRubbish(kind) && FindOvergrowth(kind)->byHand && FindOvergrowth(kind)->tool == ToolKind::Count);
    CHECK(FindOvergrowth(ResourceKind::Nettles)->tool == ToolKind::Scythe && FindOvergrowth(ResourceKind::Nettles)->byHand);
    CHECK(std::string(ResourceName(ResourceKind::RubbishHeap)) == "Rubbish heap");
    // Saved and reloaded by stable id.
    Simulation loaded;
    loaded.SetPlacements(placements);
    OK(loaded.Deserialize(sim.Serialize()));
    CHECK(PlacedNode(loaded, stump).cleared && PlacedNode(loaded, heap).cleared);
    // A save made before the clear-out was baked loads with any new obstacle on her plot already cleared.
    EstatePlacements rebaked = placements;
    rebaked.placements.push_back({next++, ResourceKind::Nettles, square, 0, 0, 1, 0});
    Simulation later;
    later.SetPlacements(rebaked);
    OK(later.Deserialize(sim.Serialize()));
    CHECK(PlacedNode(later, next - 1).cleared && later.GetState().plots.size() == 1);
    Simulation again;
    again.SetPlacements(rebaked);
    OK(again.Deserialize(later.Serialize()));
    CHECK(PlacedNode(again, next - 1).cleared);
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

void MvpWoodlandPlacements()
{
    // add-mvp-woodland-biome: the west woods carry the MVP's forage (ids 560000+, baked by
    // Scripts/Terrain/mvp_woodland.py), all on the estate and in a new game.
    const EstateLayout& layout = ProvisionalEstateLayout();
    const auto& boundary = layout.FindPolygon(Anchor::EstateBoundary)->points;
    const Point spawn = layout.PointOr(Anchor::StandingRoomSpawn, {});
    int trees = 0, branches = 0, berries = 0, roots = 0, brambles = 0, total = 0;
    double nearest = 1e9;
    for (const auto& placement : ProvisionalEstatePlacements().placements)
    {
        if (placement.id < 560000 || placement.id >= 570000) continue;
        ++total;
        CHECK(PointInPolygon(boundary, placement.position));
        trees += placement.kind == ResourceKind::ForestTree;
        branches += placement.kind == ResourceKind::Branches;
        berries += placement.kind == ResourceKind::BerryBush;
        roots += placement.kind == ResourceKind::Roots;
        brambles += placement.kind == ResourceKind::BrambleThin || placement.kind == ResourceKind::BrambleThicket;
        nearest = std::min(nearest, std::hypot(placement.position.x - spawn.x, placement.position.y - spawn.y));
    }
    CHECK(trees >= 150 && branches >= 30 && berries >= 20 && roots >= 20);
    // The MVP's blocking brambles are clearable overgrowth here.
    CHECK(brambles >= 100);
    // West of the manor, about a minute's walk: the region's near edge is some 200 m out.
    CHECK(nearest > 15000.0 && nearest < 30000.0);
    Simulation estate;
    OK(estate.NewEstateGame(layout, ProvisionalEstatePlacements()));
    const auto& resources = estate.GetState().resources;
    CHECK(std::count_if(resources.begin(), resources.end(),
        [](const ResourceNode& node) { return node.id >= 560000 && node.id < 570000; }) == total);
}

}

// A full stomach doesn't stop a snack that restores Energy she's short of.
void SnackOnFullStomachRestoresEnergy()
{
    Simulation sim;
    Stock(sim, {{Item::Berries, 3}, {Item::Roots, 2}});
    Edit(sim, [](State& state) { state.hunger = 100.0; state.energy = 40.0; });
    const auto ate = sim.Eat(Item::Berries);
    OK(ate);
    CHECK(ate.message == "Ate Berries: Energy +6.");
    CHECK(sim.GetState().hunger == 100.0 && Close(sim.GetState().energy, 46.0) && sim.Count(Item::Berries) == 2);
    // Raw roots are still refused, full or not.
    UnchangedFailure(sim, [&] { return sim.Eat(Item::Roots); });
    // Eating a chosen food group follows the same rule.
    OK(sim.EatGroup(Group(sim, Item::Berries), sim.GetRevision()));
    CHECK(sim.Count(Item::Berries) == 1 && Close(sim.GetState().energy, 52.0));
    // The last few points of Energy still count; the meal stops at 100.
    Edit(sim, [](State& state) { state.hunger = 100.0; state.energy = 97.0; });
    OK(sim.Eat(Item::Berries));
    CHECK(sim.GetState().energy == 100.0 && sim.GetState().hunger == 100.0 && sim.Count(Item::Berries) == 0);
    // Both meters full: nothing to gain, so the food is kept.
    Stock(sim, {{Item::Berries, 1}, {Item::HerbedRoots, 1}});
    Edit(sim, [](State& state) { state.hunger = 100.0; state.energy = 100.0; });
    const auto full = sim.Eat(Item::Berries);
    CHECK(!full.ok && full.message == "You are already full. Save this food for later.");
    UnchangedFailure(sim, [&] { return sim.Eat(Item::HerbedRoots); });
    UnchangedFailure(sim, [&] { return sim.EatGroup(Group(sim, Item::Berries), sim.GetRevision()); });
    // Hungry but rested: food only, and the message says so.
    Edit(sim, [](State& state) { state.hunger = 50.0; state.energy = 100.0; });
    const auto fed = sim.Eat(Item::Berries);
    OK(fed);
    CHECK(fed.message == "Ate Berries: Food +12." && sim.GetState().hunger == 62.0);
    // Full and tired with a cooked dish: Energy only (the §3a table's +40 applies in the woodland too).
    Edit(sim, [](State& state) { state.hunger = 100.0; state.energy = 20.0; });
    const auto dish = sim.Eat(Item::HerbedRoots);
    OK(dish);
    CHECK(dish.message == "Ate Herbed roots: Energy +40." && Close(sim.GetState().energy, 60.0));
    Simulation loaded;
    OK(loaded.Deserialize(sim.Serialize()));
    CHECK(loaded.Serialize() == sim.Serialize());

    Simulation failed;
    Stock(failed, {{Item::Berries, 1}});
    failed.AdvanceGameHours(120, Home);
    CHECK(failed.GetState().failed);
    UnchangedFailure(failed, [&] { return failed.Eat(Item::Berries); });
}

const RecipeIngredientAssessment* Ingredient(const RecipeAssessment& assessment, Item item)
{
    for (const auto& ingredient : assessment.ingredients) if (ingredient.item == item) return &ingredient;
    return nullptr;
}
// Every successful cooked batch burns exactly one kindling; failures and other crafts burn none.
void CookingBurnsOneKindlingPerBatch()
{
    CHECK(std::string(RecipeRequirements(Recipe::RoastedRoots)) ==
        "2 Roots + 1 Kindling; nearby fueled fire (no pot needed)");
    for (const Recipe recipe : {Recipe::RoastedRoots, Recipe::HerbedRoots})
    {
        const bool herbed = recipe == Recipe::HerbedRoots;
        const Item dish = herbed ? Item::HerbedRoots : Item::RoastedRoots;
        Simulation sim;
        BuildingStock(sim);
        const Point firePosition = CellCenter(-3, -1);
        OK(sim.Place(Piece::Fire, -3, -1, 0, firePosition));
        const int fireId = StructureId(sim, Piece::Fire, firePosition);
        OK(sim.AddFuel(fireId, firePosition));
        Stock(sim, {{Item::Roots, 4}, {Item::Flowers, 2}, {Item::Kindling, 1}});
        auto assessment = sim.AssessRecipe(recipe, firePosition);
        const auto* kindling = Ingredient(assessment, Item::Kindling);
        CHECK(assessment.craftable && kindling && kindling->need == 1 && kindling->have == 1 && kindling->met);
        CHECK(std::string(kindling->source) == "Fallen branches, saplings and old boughs");
        const auto fuelOf = [&] {
            for (const auto& piece : sim.GetState().structures) if (piece.id == fireId) return piece.fuelHours;
            return -1.0;
        };
        const double fuel = fuelOf();
        const double hour = sim.GetState().hour;
        OK(sim.Craft(recipe, firePosition));
        CHECK(sim.Count(Item::Kindling) == 0 && sim.Count(Item::Roots) == 2 && sim.Count(dish) == 1);
        CHECK(sim.Count(Item::Flowers) == (herbed ? 1 : 2));
        CHECK(sim.GetState().hour == hour && fuelOf() == fuel);

        // Out of kindling: refused, nothing spent, and the book says why.
        assessment = sim.AssessRecipe(recipe, firePosition);
        kindling = Ingredient(assessment, Item::Kindling);
        CHECK(!assessment.craftable && kindling && kindling->have == 0 && !kindling->met);
        const auto refused = sim.Craft(recipe, firePosition);
        CHECK(!refused.ok && refused.message.find("Kindling") != std::string::npos);
        UnchangedFailure(sim, [&] { return sim.Craft(recipe, firePosition); });
        // Kindling but too few roots, or no fire: nothing is burned.
        Stock(sim, {{Item::Roots, 1}, {Item::Flowers, 2}, {Item::Kindling, 2}});
        UnchangedFailure(sim, [&] { return sim.Craft(recipe, firePosition); });
        Stock(sim, {{Item::Roots, 2}, {Item::Flowers, 2}, {Item::Kindling, 2}});
        UnchangedFailure(sim, [&] { return sim.Craft(recipe, {firePosition.x + 5000.0, firePosition.y}); });
        // A second batch takes the second kindling, and only that one.
        OK(sim.Craft(recipe, firePosition));
        CHECK(sim.Count(Item::Kindling) == 1 && sim.Count(Item::Roots) == 0);
    }

    Simulation hands;
    Stock(hands, {{Item::Branch, 2}, {Item::RustedAxeHead, 1}, {Item::Kindling, 3}, {Item::Timber, 1}});
    OK(hands.Craft(Recipe::HaftAxe, Home));
    OK(hands.Craft(Recipe::SplitFirewood, Home));
    CHECK(hands.Count(Item::Kindling) == 3 && hands.Count(Item::Firewood) == 4);
}

// Hand-gathered fallen branches come with one kindling each time they regrow, woodland and estate alike.
void BranchesYieldRenewableKindling()
{
    Simulation sim;
    Stock(sim, {});
    const auto branch = Node(sim, ResourceKind::Branches);
    CHECK(Close(sim.HarvestCost(branch.id), Exertion::GatherEnergy));
    OK(sim.Harvest(branch.id, branch.position));
    CHECK(sim.Count(Item::Branch) == 5 && sim.Count(Item::Kindling) == 1);
    UnchangedFailure(sim, [&] { return sim.Harvest(branch.id, branch.position); });
    Simulation loaded;
    OK(loaded.Deserialize(sim.Serialize()));
    UnchangedFailure(loaded, [&] { return loaded.Harvest(branch.id, branch.position); });
    // Branches regrow after a day.
    loaded.AdvanceGameHours(23.5, branch.position);
    UnchangedFailure(loaded, [&] { return loaded.Harvest(branch.id, branch.position); });
    loaded.AdvanceGameHours(1.0, branch.position);
    CHECK(!loaded.GetState().failed);
    OK(loaded.SetEnergy(100));
    OK(loaded.Harvest(branch.id, branch.position));
    CHECK(loaded.Count(Item::Branch) == 10 && loaded.Count(Item::Kindling) == 2);

    // A save from before the item stocks carried their width loads, and gathering still gives kindling.
    Simulation migrated;
    Stock(sim, {{Item::Kindling, 4}});
    OK(migrated.Deserialize(Encode(sim.GetState(), PositionalStockSaveVersion)));
    CHECK(migrated.Count(Item::Kindling) == 4);
    const auto other = Node(migrated, ResourceKind::Branches);
    OK(migrated.Harvest(other.id, other.position));
    CHECK(migrated.Count(Item::Kindling) == 5 && migrated.Count(Item::Branch) == 5);

    Simulation estate;
    estate.SetPlacements(ProvisionalEstatePlacements());
    OK(estate.NewEstateGame(ProvisionalEstateLayout(), ProvisionalEstatePlacements()));
    const auto fallen = Node(estate, ResourceKind::Branches);
    const int branches = estate.Count(Item::Branch);
    const int kindling = estate.Count(Item::Kindling);
    OK(estate.Harvest(fallen.id, fallen.position));
    CHECK(estate.Count(Item::Branch) == branches + 5 && estate.Count(Item::Kindling) == kindling + 1);
}

// Jenny's playtest: a bare bramble 285 cm ahead showed no prompt, and a weed at her feet stole the
// focus. The held tool now aims at what it clears, out to the full Overgrowth::Reach, never behind her.
// Architecture review 8df23ba3: with the billhook in hand, a bramble 70 cm behind her kept the prompt
// and took the swing from the one 200 cm straight ahead. The prompt (HeldToolFocus) and the swing
// (FindAimedOvergrowth) now both name the one ahead; the one behind is only named when nothing is.
void HeldToolNeverStrikesBehind()
{
    const Point at = ProvisionalEstateLayout().PointOr(Anchor::StandingRoomSpawn, {});
    EstatePlacements table;
    table.bakeVersion = 7;
    int next = EstatePlacementIdBase + 20400;
    const auto add = [&](ResourceKind kind, double dx, double dy)
    {
        table.placements.push_back({next++, kind, {at.x + dx, at.y + dy}, 0, 0, 1, 0});
        return table.placements.back().id;
    };
    const int behind = add(ResourceKind::BrambleThin, -70, 0);
    const int ahead = add(ResourceKind::BrambleThin, 200, 0);
    const int thicket = add(ResourceKind::BrambleThicket, 0, -70);
    const int aheadNorth = add(ResourceKind::BrambleThin, 0, 220);
    const int berries = add(ResourceKind::BerryBush, 40, 0);
    Simulation sim;
    OK(sim.NewEstateGame(ProvisionalEstateLayout(), table));
    OK(sim.GrantItems(Item::Billhook, 1));
    const Point east{1, 0}, west{-1, 0}, north{0, 1};

    // The nearest-centre focus lands on the one behind (70 cm); held billhook, facing east.
    CHECK(sim.HeldToolFocus(behind, at, east, Item::Billhook) == ahead);
    CHECK(sim.FindAimedOvergrowth(at, east, Item::Billhook) == ahead);
    // Whatever the nearest focus was, the prompt and the swing agree on the one ahead.
    for (const int current : {-1, behind, thicket, ahead})
        CHECK(sim.HeldToolFocus(current, at, east, Item::Billhook) == sim.FindAimedOvergrowth(at, east, Item::Billhook));
    // A forageable she's standing at keeps its prompt (E still gathers it); the swing still aims ahead.
    CHECK(sim.HeldToolFocus(berries, at, east, Item::Billhook) == berries);
    // Pressing clears the one ahead only: the one behind is untouched, Energy spent once.
    const double energy = sim.GetState().energy;
    OK(sim.ClearOvergrowth(sim.FindAimedOvergrowth(at, east, Item::Billhook), Item::Billhook, at));
    CHECK(PlacedNode(sim, ahead).cleared && !PlacedNode(sim, behind).cleared);
    CHECK(Close(sim.GetState().energy, energy - FindOvergrowth(ResourceKind::BrambleThin)->energy, 1e-9));

    // An under-tier thicket 70 cm to her side doesn't block the valid bramble she faces.
    CHECK(sim.HeldToolFocus(thicket, at, north, Item::Billhook) == aheadNorth);
    CHECK(sim.FindAimedOvergrowth(at, north, Item::Billhook) == aheadNorth);
    OK(sim.CheckOvergrowth(aheadNorth, Item::Billhook, at));

    // Nothing ahead any more: the prompt keeps naming the one behind, but the swing has no target,
    // so a press changes nothing there (the controller says "Turn to face it.").
    OK(sim.ClearOvergrowth(aheadNorth, Item::Billhook, at));
    CHECK(sim.FindAimedOvergrowth(at, east, Item::Billhook) == -1);
    CHECK(sim.HeldToolFocus(behind, at, east, Item::Billhook) == behind);
    CHECK(!PlacedNode(sim, behind).cleared);
    // Turning round aims at it; the under-tier thicket is refused silently (ToolTier, nothing changes).
    CHECK(sim.FindAimedOvergrowth(at, west, Item::Billhook) == behind);
    const auto gated = sim.CheckOvergrowth(thicket, Item::Billhook, at);
    CHECK(!gated.ok && gated.code == ResultCode::ToolTier);
    UnchangedFailure(sim, [&] { return sim.ClearOvergrowth(thicket, Item::Billhook, at); });
    // Nothing aimed and no nearest focus: nothing named. A tool with no overgrowth kind aims at nothing.
    CHECK(sim.HeldToolFocus(-1, at, east, Item::Billhook) == -1);
    CHECK(sim.HeldToolFocus(behind, at, east, Item::Count) == behind);
}

void AimedOvergrowthReach()
{
    const Point at = ProvisionalEstateLayout().PointOr(Anchor::StandingRoomSpawn, {});
    EstatePlacements table;
    table.bakeVersion = 7;
    int next = EstatePlacementIdBase + 20000;
    const auto add = [&](ResourceKind kind, double dx, double dy)
    {
        table.placements.push_back({next++, kind, {at.x + dx, at.y + dy}, 0, 0, 1, 0});
        return table.placements.back().id;
    };
    const int bramble = add(ResourceKind::BrambleThin, 285, 0);
    const int weed = add(ResourceKind::Weeds, 50, 20);
    const int grass = add(ResourceKind::TallGrass, 120, -30);
    const int behind = add(ResourceKind::BrambleThin, -150, 40);
    const int beyond = add(ResourceKind::BrambleThin, 0, -301);
    const int thicket = add(ResourceKind::BrambleThicket, -260, 60);
    Simulation sim;
    OK(sim.NewEstateGame(ProvisionalEstateLayout(), table));
    OK(sim.GrantItems(Item::Billhook, 1));
    OK(sim.GrantItems(Item::Scythe, 1));
    const Point east{1, 0}, west{-1, 0}, south{0, -1};

    // The bramble 285 cm ahead is the billhook's target, past the nearer weed and grass.
    CHECK(sim.FindAimedOvergrowth(at, east, Item::Billhook) == bramble);
    OK(sim.CheckOvergrowth(bramble, Item::Billhook, at));
    // Only what the tool clears is aimed at; the scythe takes the weed at her feet in any direction.
    CHECK(sim.FindAimedOvergrowth(at, east, Item::Scythe) == weed);
    CHECK(sim.FindAimedOvergrowth(at, west, Item::Scythe) == weed);
    CHECK(sim.FindAimedOvergrowth({at.x + 90, at.y}, east, Item::Scythe) == grass);
    // Never behind her: turning round aims at the other bramble.
    CHECK(sim.FindAimedOvergrowth(at, west, Item::Billhook) == behind);
    // 301 cm is past the reach the clear itself allows.
    CHECK(sim.FindAimedOvergrowth(at, south, Item::Billhook) == -1);
    CHECK(!sim.CheckOvergrowth(beyond, Item::Billhook, at));
    CHECK(sim.FindAimedOvergrowth(at, east, Item::Hatchet) == -1);
    CHECK(sim.FindAimedOvergrowth(at, east, Item::Count) == -1);
    CHECK(sim.FindAimedOvergrowth(at, {0, 0}, Item::Billhook) == -1);
    CHECK(sim.FindAimedOvergrowth({std::numeric_limits<double>::quiet_NaN(), 0}, east, Item::Billhook) == -1);

    // The billhook is the wrong tool for weeds (hand or scythe), and says so without a tier code.
    const auto wrong = sim.CheckOvergrowth(weed, Item::Billhook, at);
    CHECK(!wrong.ok && wrong.code != ResultCode::ToolTier);
    UnchangedFailure(sim, [&] { return sim.ClearOvergrowth(weed, Item::Billhook, at); });

    // One press clears the thin bramble at 285 cm, and spends energy once.
    double energy = sim.GetState().energy;
    OK(sim.ClearOvergrowth(bramble, Item::Billhook, at));
    CHECK(Close(sim.GetState().energy, energy - FindOvergrowth(ResourceKind::BrambleThin)->energy));
    CHECK(sim.FindAimedOvergrowth(at, east, Item::Billhook) == -1);
    UnchangedFailure(sim, [&] { return sim.ClearOvergrowth(bramble, Item::Billhook, at); });

    // An under-tier thicket is still aimed at, so she's told why; nothing changes and the gates hold.
    OK(sim.ClearOvergrowth(behind, Item::Billhook, at));
    CHECK(sim.FindAimedOvergrowth(at, west, Item::Billhook) == thicket);
    const auto gated = sim.CheckOvergrowth(thicket, Item::Billhook, at);
    CHECK(!gated.ok && gated.code == ResultCode::ToolTier && gated.message == "Needs an iron billhook");
    UnchangedFailure(sim, [&] { return sim.ClearOvergrowth(thicket, Item::Billhook, at); });
    CHECK(FindOvergrowth(ResourceKind::BrambleThin)->swings == (std::array<int, ToolTierCount>{1, 1, 1, 1}));
    CHECK(FindOvergrowth(ResourceKind::BrambleThicket)->swings == (std::array<int, ToolTierCount>{3, 2, 1, 1}));
    CHECK(FindOvergrowth(ResourceKind::BrambleBank)->swings == (std::array<int, ToolTierCount>{4, 3, 2, 1}));
    CHECK(FindOvergrowth(ResourceKind::BrambleThicket)->minTier == ToolTier::Iron);
    CHECK(FindOvergrowth(ResourceKind::BrambleBank)->minTier == ToolTier::Steel);
    Simulation loaded;
    loaded.SetPlacements(table);
    OK(loaded.Deserialize(sim.Serialize()));
    CHECK(PlacedNode(loaded, bramble).cleared && PlacedNode(loaded, behind).cleared);
    CHECK(!PlacedNode(loaded, thicket).cleared && !PlacedNode(loaded, weed).cleared);
    CHECK(loaded.FindAimedOvergrowth(at, east, Item::Billhook) == -1);
    CHECK(loaded.FindAimedOvergrowth(at, east, Item::Scythe) == weed);
}

// A worn billhook fells a sapling in one logical swing (the hack clip already shows both blows):
// one yield of 3-4 branches and a kindling, 1.5 Energy, and it stays down through a reload.
void OneSwingWornSapling()
{
    const Point at = ProvisionalEstateLayout().PointOr(Anchor::StandingRoomSpawn, {});
    EstatePlacements table;
    table.bakeVersion = 7;
    table.placements.push_back({EstatePlacementIdBase + 20100, ResourceKind::Sapling, {at.x, at.y + 800}, 0, 0, 1, 0});
    const int sapling = table.placements.back().id;
    Simulation sim;
    OK(sim.NewEstateGame(ProvisionalEstateLayout(), table));
    OK(sim.GrantItems(Item::Billhook, 1));
    const Point grove{at.x, at.y + 700}, north{0, 1};
    CHECK(FindOvergrowth(ResourceKind::Sapling)->swings == (std::array<int, ToolTierCount>{1, 1, 1, 1}));
    CHECK(sim.FindAimedOvergrowth(grove, north, Item::Billhook) == sapling);
    CHECK(sim.OvergrowthSwings(sapling) == 1);
    const int branches = sim.Count(Item::Branch), kindling = sim.Count(Item::Kindling);
    const double energy = sim.GetState().energy;
    OK(sim.ClearOvergrowth(sapling, Item::Billhook, grove));
    const int gained = sim.Count(Item::Branch) - branches;
    CHECK(gained >= 3 && gained <= 4 && sim.Count(Item::Kindling) == kindling + 1);
    CHECK(Close(sim.GetState().energy, energy - 1.5));
    UnchangedFailure(sim, [&] { return sim.ClearOvergrowth(sapling, Item::Billhook, grove); });
    Simulation loaded;
    loaded.SetPlacements(table);
    OK(loaded.Deserialize(sim.Serialize()));
    CHECK(PlacedNode(loaded, sapling).cleared);
    CHECK(loaded.FindAimedOvergrowth(grove, north, Item::Billhook) == -1);
}

int main()
{
    Run("defaults and input validation", DefaultsAndValidation);
    Run("fixed estate new game and save", FixedEstateNewGameAndSave);
    Run("taking down placed pieces refunds full cost and protects occupied floors", DeconstructRefundsAndFoundationRefusal);
    Run("taking down chests returns contents and drops overflow", DeconstructChestContentsAndOverflow);
    Run("heritage manor pieces cannot be taken down", HeritageManorPiecesCannotBeDeconstructed);
    Run("large chests and water portions do not crowd the pack", ChestCapacityAndWaterSpace);
    Run("sleep option policy", SleepOptionPolicy);
    Run("overgrowth tools, tiers and prompts", OvergrowthTableAndPrompts);
    Run("salvage, hafting and tier-gated clearing by stable id", HaftingBootstrapAndClearing);
    Run("the held tool aims at what it clears, out to the full reach", AimedOvergrowthReach);
    Run("the held tool's prompt and swing never pick a target behind her", HeldToolNeverStrikesBehind);
    Run("a worn billhook fells a sapling in one press", OneSwingWornSapling);
    Run("multi-swing clears, energy reserve and full-pack yields", MultiSwingTiersAndCapacity);
    Run("one scythe sweep mows each tuft once", ScytheSweepMowsEachTuftOnce);
    Run("a held tool strikes its target until it clears, then stops", HeldToolRepeatsUntilClear);
    Run("salvage head order and the scythe's forward arc", SalvageOrderAndScytheArc);
    Run("daily weed creep near remaining overgrowth only", WeedCreepNearOvergrowth);
    Run("manor clear-out field placement", ManorClearoutField);
    Run("every live weed is on open ground", EveryLiveWeedIsOnOpenGround);
    Run("the ruin's loose slate and rubble can be cleared", RuinDebrisIsClearable);
    Run("every hand gather has its own pose", EveryHandGatherHasItsOwnPose);
    Run("a wrong or under-tier tool is non-actionable", WrongOrUnderTierToolIsNonActionable);
    Run("the ruin's fallen roof timbers are chopped with the axe", RuinTimbersAreChoppedWithTheAxe);
    Run("a billhook blow lands only on its own hack", BillhookBlowLandsOnlyOnItsOwnHack);
    Run("clear-out rubbish, nettles, stumps and spoiled ground", ClearoutKindsAndSpoiledGround);
    Run("pre-pivot vitals line without warmth", LegacyVitalsLine);
    Run("playtest skip to morning", SkipToMorning);
    Run("HUD requirements and zero-time action commits", RequirementsMatchTransactions);
    Run("the bedroll takes 4 Branch and 4 Hay", BedrollTakesHay);
    Run("pure structured recipe assessment", StructuredRecipeAssessment);
    Run("a snack on a full stomach restores Energy", SnackOnFullStomachRestoresEnergy);
    Run("each cooked batch burns exactly one kindling", CookingBurnsOneKindlingPerBatch);
    Run("fallen branches give renewable kindling", BranchesYieldRenewableKindling);
    Run("default gameplay walkthrough", GameplayWalkthrough);
    Run("weeds come up once a day, on waking or at 6 AM", DailyWeedPass);
    Run("sporadic daily weeds, stable over reloads", SporadicDailyWeeds);
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
    Run("weeding by hand commits once, and a probe changes nothing", WeedPullCommitsOnce);
    Run("a weed pull commits only on its own clip, never an earlier tail", WeedPullCommitsOnlyOnItsOwnClip);
    Run("weeds in any square offer and take a pull", PullWeedsOnAnySquare);
    Run("berry planting, forgiving growth and recurring harvest", BerryCropCycle);
    Run("crop table, growing days, care modifiers, stages and status", CropTableAndStatus);
    Run("crop-kind persistence and incompatible test-save rejection", CropKindPersistenceAndVersionRejection);
    Run("item stocks carry their width; version 12 saves migrate", ItemStockWidthCompatibility);
    Run("clock, pause and batching consistency", ClockPauseAndBatching);
    Run("sprint is free and needs at least 25 Energy", SprintEnergyContract);
    Run("work spends Energy and time drains it slowly", ActionEnergyContract);
    Run("sleep and failure recovery without cold", SleepAndFailure);
    Run("estate exhaustion never fails or faints, including old saves", EstateExhaustionIsNonlethal);
    Run("newest valid autosave beats older recovery and corrupt newest", NewestValidRecoveryPreference);
    Run("bed reach, facing and focus priority", BedSleepReachAndPriority);
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
    Run("ground stacks stay small enough to pick up whole", GroundStacksStayPickable);
    Run("seeded resource identity and bounded region activation", GeneratedWorldIdentityAndActivation);
    Run("permanent generated felling and persistent renewable timers", GeneratedFellingAndPersistentTimers);
    Run("cross-cell trunk footprint, chosen building sites and reload", GeneratedBuildingFootprintAndReload);
    Run("generated save validation and explicit sparse edit limit", GeneratedSaveValidationAndEditLimit);
    Run("long deterministic negative and positive chunk walk", LongDeterministicChunkWalk);
    Run("mixed distant edits, cache churn and exact reload", MixedPersistentWorldChurn);
    Run("sparse edit scale, payload bounds and atomic rejection", SparseEditScaleAndPayloadBounds);
    Run("MVP woodland placements", MvpWoodlandPlacements);
    Run("garden outline preview matches the hoe and pail", GardenTargetPreview);
    Run("seed outline and plant cue match the sow action", SeedSowPreview);
    std::cout << cases << " scenarios, " << checks << " explicit checks passed.\n";
    return 0;
}
