#include "HomesteadSimulation.h"
#include "HomesteadCrops.h"
#include "HomesteadEstate.h"
#include "HomesteadParcels.h"
#include "HomesteadManor.h"
#include "HomesteadLamp.h"
#include "HomesteadCrafting.h"
#include "HomesteadOvergrowth.h"
#include "HomesteadSimulationDetail.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <locale>
#include <queue>
#include <set>
#include <sstream>
#include <tuple>

namespace Homestead
{
namespace
{
constexpr double Reach = 300.0;
constexpr double FireReach = 450.0;
constexpr double MaxHour = 1000000.0;
constexpr double TimeStep = 1.0 / 120.0;
constexpr int MaxObjects = 4096;
constexpr int MaxStock = ChestCapacity;
constexpr double DropReach = 220.0;
constexpr double DropMergeReach = 120.0;
constexpr double MaxFuel = 48.0;
constexpr std::size_t MaxSaveBytes = 8 * 1024 * 1024;

template <typename T> bool ValidEnum(T value, T count)
{
    return static_cast<int>(value) >= 0 && static_cast<int>(value) < static_cast<int>(count);
}
bool FiniteRange(double value, double low, double high)
{
    return std::isfinite(value) && value >= low && value <= high;
}
bool ValidPoint(Point p)
{
    return FiniteRange(p.x, -MaxWorldCoordinate, MaxWorldCoordinate) &&
        FiniteRange(p.y, -MaxWorldCoordinate, MaxWorldCoordinate);
}
bool ValidCell(int x, int y)
{
    return x >= -3333 && x <= 3332 && y >= -3333 && y <= 3332;
}
double DistanceSquared(Point a, Point b)
{
    return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y);
}
bool Near(Point a, Point b, double reach = Reach)
{
    return ValidPoint(a) && DistanceSquared(a, b) <= reach * reach;
}
int Cell(double value) { return static_cast<int>(std::floor(value / CellSize)); }
double Clamp(double value, double low, double high) { return std::max(low, std::min(high, value)); }
Result Good(const std::string& text) { return {true, text}; }
Result Bad(const std::string& text) { return {false, text, ResultCode::Invalid}; }
Result Failed() { return Bad("You need to recover. Load your recent checkpoint to continue."); }
double FoodNutrition(Item item) { return GetItemInfo(item).hunger; }
// Stamina from a meal: a handful of berries is a quick pick-me-up, cooked roots a real rest.
double FoodEnergy(Item item) { return GetItemInfo(item).energy; }
// Applies a meal to hunger and energy and describes what it did, e.g. "Ate Berries: Food +12, Energy +6."
std::string ApplyMeal(State& state, Item item)
{
    const double food = std::min(100.0, state.hunger + FoodNutrition(item)) - state.hunger;
    const double energy = std::min(100.0, state.energy + FoodEnergy(item)) - state.energy;
    state.hunger += food;
    state.energy += energy;
    std::string message = std::string("Ate ") + ItemName(item) + ": Food +" + std::to_string(static_cast<int>(std::lround(food)));
    if (energy >= 0.5) message += ", Energy +" + std::to_string(static_cast<int>(std::lround(energy)));
    return message + ".";
}
Result CanEat(const State& state, Item item)
{
    if (state.failed) return Failed();
    if (FoodNutrition(item) == 0.0) return Bad(item == Item::Roots ? std::string("Raw roots need cooking first.")
        : std::string(ItemName(item)) + " isn't something to eat.");
    if (state.hunger >= 100.0) return Bad("You are already full. Save this food for later.");
    return Good("");
}
Inventory Items(std::initializer_list<std::pair<Item, int>> values)
{
    Inventory result{};
    for (const auto& value : values) result[static_cast<int>(value.first)] += value.second;
    return result;
}
constexpr bool TakesSpace(Item item) { return item != Item::Water; }
bool StockValid(const Inventory& stock, int capacity = InventoryCapacity)
{
    int total = 0;
    for (int i = 0; i < ItemCount; ++i)
    {
        if (stock[i] < 0 || stock[i] > MaxStock) return false;
        if (TakesSpace(static_cast<Item>(i))) total += stock[i];
    }
    return total <= capacity;
}
bool Empty(const Inventory& stock)
{
    return std::all_of(stock.begin(), stock.end(), [](int value) { return value == 0; });
}
template <typename T> T* Find(std::vector<T>& objects, int id)
{
    for (auto& object : objects) if (object.id == id) return &object;
    return nullptr;
}
template <typename T> const T* Find(const std::vector<T>& objects, int id)
{
    for (const auto& object : objects) if (object.id == id) return &object;
    return nullptr;
}
double Regrowth(ResourceKind kind)
{
    switch (kind)
    {
    case ResourceKind::Branches: return 24.0;
    case ResourceKind::Stones: return 48.0;
    case ResourceKind::BerryBush: return 36.0;
    case ResourceKind::Roots: return 48.0;
    case ResourceKind::Flowers: return 24.0;
    case ResourceKind::Reeds: return 24.0;
    case ResourceKind::Sapling: return 168.0;
    case ResourceKind::ForestTree: return 0.0;
    // Another winter-killed deer turns up in the same hollow about ten days later.
    case ResourceKind::DeerRemains: return 240.0;
    // Spring flowers come back in a few days.
    case ResourceKind::Primroses:
    case ResourceKind::Bluebells:
    case ResourceKind::WildDaffodils:
    case ResourceKind::WildGarlic: return 72.0;
    default: return 0.0;
    }
}
Inventory Yield(ResourceKind kind)
{
    switch (kind)
    {
    case ResourceKind::Branches: return Items({{Item::Branch, 5}});
    case ResourceKind::Stones: return Items({{Item::Stone, 4}});
    case ResourceKind::BerryBush: return Items({{Item::Berries, 5}});
    case ResourceKind::Roots: return Items({{Item::Roots, 2}, {Item::Seeds, 2}});
    case ResourceKind::Flowers: return Items({{Item::Flowers, 3}});
    case ResourceKind::ForestTree: return Items({{Item::Timber, 6}, {Item::Branch, 4}});
    case ResourceKind::Primroses: return Items({{Item::Primroses, 3}});
    case ResourceKind::Bluebells: return Items({{Item::Bluebells, 3}});
    case ResourceKind::WildDaffodils: return Items({{Item::WildDaffodils, 3}});
    case ResourceKind::WildGarlic: return Items({{Item::WildGarlic, 3}});
    // Reeds, deer remains and the overgrowth kinds (see HomesteadOvergrowth) yield nothing here.
    default: return {};
    }
}
// Reeds (fibre) and deer remains (fur) are retired from play; the knife that cut them is gone.
bool Retired(ResourceKind kind) { return kind == ResourceKind::Reeds || kind == ResourceKind::DeerRemains; }
// Bramble canes, split and woven as wattle, take the place of the retired fibre lashing.
Inventory BuildCost(Piece kind)
{
    switch (kind)
    {
    case Piece::Foundation: return Items({{Item::Branch, -4}, {Item::Stone, -2}});
    case Piece::Wall: return Items({{Item::Branch, -3}, {Item::BrambleCanes, -1}});
    case Piece::Doorway: return Items({{Item::Branch, -4}, {Item::BrambleCanes, -1}});
    case Piece::Roof: return Items({{Item::Branch, -4}, {Item::BrambleCanes, -3}});
    case Piece::Fire: return Items({{Item::Branch, -3}, {Item::Stone, -4}});
    case Piece::Bed: return Items({{Item::Branch, -4}, {Item::BrambleCanes, -4}});
    case Piece::Chest: return Items({{Item::Branch, -5}, {Item::BrambleCanes, -2}});
    // Period crafting: the stations are built from timber and twine by hand; the rest are made at
    // the workbench and set up whole, so taking one down returns it whole.
    case Piece::Workbench: return Items({{Item::Timber, -6}, {Item::Twine, -4}});
    case Piece::Sawhorse: return Items({{Item::Timber, -4}, {Item::Twine, -2}});
    case Piece::FenceRail: return Items({{Item::FenceSection, -1}});
    case Piece::FenceGate: return Items({{Item::FieldGate, -1}});
    case Piece::Stool: return Items({{Item::Stool, -1}});
    case Piece::Table: return Items({{Item::Table, -1}});
    case Piece::Chair: return Items({{Item::Chair, -1}});
    case Piece::Shelf: return Items({{Item::Shelf, -1}});
    default: return {};
    }
}
Inventory CraftChange(Recipe recipe)
{
    switch (recipe)
    {
    case Recipe::HaftAxe: return Items({{Item::RustedAxeHead, -1}, {Item::Branch, -2}, {Item::Hatchet, 1}});
    case Recipe::HaftHoe: return Items({{Item::RustedHoeBlade, -1}, {Item::Branch, -2}, {Item::DiggingStick, 1}});
    case Recipe::HaftScythe: return Items({{Item::RustedScytheBlade, -1}, {Item::Branch, -2}, {Item::Scythe, 1}});
    case Recipe::HaftBillhook: return Items({{Item::RustedBillhookHead, -1}, {Item::Branch, -2}, {Item::Billhook, 1}});
    case Recipe::HaftPickaxe: return Items({{Item::RustedPickHead, -1}, {Item::Branch, -2}, {Item::Pickaxe, 1}});
    case Recipe::RoastedRoots: return Items({{Item::Roots, -2}, {Item::RoastedRoots, 1}});
    case Recipe::HerbedRoots: return Items({{Item::Roots, -2}, {Item::Flowers, -1}, {Item::HerbedRoots, 1}});
    case Recipe::SplitFirewood: return Items({{Item::Timber, -1}, {Item::Firewood, 4}});
    case Recipe::SawPlanks: return Items({{Item::Timber, -1}, {Item::Planks, Crafting::PlanksPerTimber}});
    case Recipe::MakeFenceSection: return Items({{Item::Planks, -2}, {Item::FenceSection, 1}});
    // The hinges are salvaged scrap iron.
    case Recipe::MakeFieldGate: return Items({{Item::Planks, -4}, {Item::ScrapIron, -1}, {Item::FieldGate, 1}});
    case Recipe::MakeStool: return Items({{Item::Planks, -2}, {Item::Stool, 1}});
    case Recipe::MakeTable: return Items({{Item::Planks, -5}, {Item::Table, 1}});
    case Recipe::MakeChair: return Items({{Item::Planks, -3}, {Item::Chair, 1}});
    // The brackets are scrap iron.
    case Recipe::MakeShelf: return Items({{Item::Planks, -3}, {Item::ScrapIron, -1}, {Item::Shelf, 1}});
    default: return {};
    }
}
bool Hafting(Recipe recipe) { return recipe >= Recipe::HaftAxe && recipe <= Recipe::HaftPickaxe; }
std::string DescribeCost(const Inventory& change)
{
    std::string result;
    for (int i = 0; i < ItemCount; ++i)
    {
        if (change[i] >= 0) continue;
        if (!result.empty()) result += " + ";
        result += std::to_string(-change[i]) + " " + ItemName(static_cast<Item>(i));
    }
    return result;
}
std::string MissingMessage(const Inventory& change, const Inventory& stock)
{
    std::string result = "Gather ";
    bool first = true;
    for (int i = 0; i < ItemCount; ++i)
    {
        if (stock[i] + change[i] >= 0) continue;
        if (!first) result += ", ";
        result += std::to_string(-change[i] - stock[i]) + " " + ItemName(static_cast<Item>(i));
        first = false;
    }
    return first ? "Not enough pack space. Store some items in a chest first." : result + " first.";
}
const char* AcquisitionSource(Item item) { return ItemSource(item); }
bool EdgePiece(Piece kind) { return kind == Piece::Wall || kind == Piece::Doorway; }
bool Furniture(Piece kind) { return IsFurniture(kind); }
using Edge = std::tuple<int, int, int>;
// Canonical shared edges: axis 0 is vertical, axis 1 is horizontal.
Edge EdgeKey(int x, int y, int rotation)
{
    switch (rotation)
    {
    case 0: return {x, y + 1, 1};
    case 1: return {x + 1, y, 0};
    case 2: return {x, y, 1};
    default: return {x, y, 0};
    }
}
bool HasPiece(const State& state, Piece kind, int buildingId, int x, int y)
{
    for (const auto& piece : state.structures)
        if (piece.kind == kind && piece.buildingId == buildingId && piece.cellX == x && piece.cellY == y) return true;
    return false;
}
bool RequiresHatchet(ResourceKind kind)
{
    return kind == ResourceKind::ForestTree;
}
Result GenerationFailure(Generation::Status status)
{
    return {false, std::string(Generation::StatusMessage(status)) + " The world was not changed.",
        status == Generation::Status::UnsupportedVersion ? ResultCode::UnsupportedVersion : ResultCode::Invalid};
}
bool SuppressedTree(const Generation::GeneratedEntity& entity)
{
    if (entity.kind != Generation::EntityKind::ForestTree) return false;
    const Point p{static_cast<double>(entity.xCm), static_cast<double>(entity.yCm)};
    if (DistanceSquared(p, {-1000, 0}) <= 90.0 * 90.0) return true;
    const Point start{-1300, -80};
    const double along = Clamp(((p.x - start.x) * 300.0 + (p.y - start.y) * 80.0) /
        (300.0 * 300.0 + 80.0 * 80.0), 0.0, 1.0);
    return DistanceSquared(p, {start.x + 300.0 * along, start.y + 80.0 * along}) <= 60.0 * 60.0;
}
ResourceKind ResourceType(Generation::EntityKind kind)
{
    switch (kind)
    {
    case Generation::EntityKind::ForestTree: return ResourceKind::ForestTree;
    case Generation::EntityKind::Branches: return ResourceKind::Branches;
    case Generation::EntityKind::Stones: return ResourceKind::Stones;
    case Generation::EntityKind::BerryBush: return ResourceKind::BerryBush;
    case Generation::EntityKind::Roots: return ResourceKind::Roots;
    case Generation::EntityKind::Flowers: return ResourceKind::Flowers;
    case Generation::EntityKind::Reeds: return ResourceKind::Reeds;
    case Generation::EntityKind::Sapling: return ResourceKind::Sapling;
    case Generation::EntityKind::DeerRemains: return ResourceKind::DeerRemains;
    default: return ResourceKind::Count;
    }
}
bool GeneratedNode(const State& state, const Generation::GeneratedEntity& entity, ResourceNode& out)
{
    const Point position{static_cast<double>(entity.xCm), static_cast<double>(entity.yCm)};
    if (!ValidPoint(position) || SuppressedTree(entity)) return false;
    ResourceNode node;
    node.kind = ResourceType(entity.kind);
    if (!ValidEnum(node.kind, ResourceKind::Count)) return false;
    node.position = position;
    node.key = entity.key;
    const auto edit = std::lower_bound(state.resourceEdits.begin(), state.resourceEdits.end(), node.key,
        [](const ResourceEdit& value, const Generation::GeneratedEntityKey& key) { return value.key < key; });
    if (edit != state.resourceEdits.end() && edit->key == node.key)
    {
        node.cleared = edit->cleared;
        node.readyAtHour = edit->readyAtHour;
    }
    out = node;
    return true;
}
Result Materialize(State& candidate, const State* previous, int& nextHandle,
    const PreparedWorldRegion* prepared = nullptr)
{
    if (candidate.fixedEstate) return Good("");
    if (prepared && (candidate.world.seed != prepared->world.seed
        || candidate.world.generationVersion != prepared->world.generationVersion))
        return Bad("Prepared woodland belongs to a different world.");
    std::vector<ResourceNode> resources;
    for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx)
        {
            const Generation::ChunkCoord coord{candidate.activeChunk.x + dx, candidate.activeChunk.y + dy};
            Generation::ChunkBaseline generated;
            const auto* baseline = prepared ? prepared->chunks[(dy + 1) * 3 + dx + 1] : nullptr;
            if (baseline && baseline->chunk != coord)
                return Bad("Prepared woodland chunk does not match the active region.");
            if (!baseline)
            {
                const auto status = Generation::GenerateChunk(candidate.world, coord, generated);
                if (status != Generation::Status::Ok) return GenerationFailure(status);
                baseline = &generated;
            }
            for (const auto& entity : baseline->entities)
            {
                ResourceNode node;
                if (!GeneratedNode(candidate, entity, node)) continue;
                if (previous)
                    for (const auto& old : previous->resources)
                        if (old.key == node.key) { node.id = old.id; break; }
                if (node.id == 0)
                {
                    if (nextHandle == std::numeric_limits<int>::max())
                        return Bad("This session has exhausted transient resource handles. Save and restart.");
                    node.id = nextHandle++;
                }
                resources.push_back(node);
            }
        }
    candidate.resources = std::move(resources);
    return Good("");
}
bool SaveResourceEdit(State& candidate, const ResourceNode& node)
{
    auto edit = std::lower_bound(candidate.resourceEdits.begin(), candidate.resourceEdits.end(), node.key,
        [](const ResourceEdit& value, const Generation::GeneratedEntityKey& key) { return value.key < key; });
    if (edit != candidate.resourceEdits.end() && edit->key == node.key)
        *edit = {node.key, node.cleared, node.readyAtHour};
    else
    {
        if (candidate.resourceEdits.size() >= MaxResourceEdits) return false;
        candidate.resourceEdits.insert(edit, {node.key, node.cleared, node.readyAtHour});
    }
    return true;
}
Result CheckAreaResources(const State& state, double left, double bottom, double size, const char* blocked);
Result CheckGardenResources(const State& state, int gardenX, int gardenY)
{
    return CheckAreaResources(state, gardenX * GardenCellSize, gardenY * GardenCellSize, GardenCellSize,
        "Fell the standing tree or clear the sapling before tilling here.");
}
Result CheckAreaResources(const State& state, double left, double bottom, double size, const char* blockedMessage)
{
    const auto blocks = [&](const ResourceNode& node)
    {
        if (node.cleared) return false;
        return node.kind == ResourceKind::Sapling ?
            node.position.x >= left && node.position.x < left + size
                && node.position.y >= bottom && node.position.y < bottom + size :
            DistanceSquared(node.position, {Clamp(node.position.x, left, left + size),
                Clamp(node.position.y, bottom, bottom + size)}) <= 50.0 * 50.0;
    };
    // The fixed estate has no woodland generator; its trees and overgrowth are the baked placements.
    if (state.fixedEstate)
    {
        for (const auto& node : state.resources)
        {
            if (node.cleared) continue;
            if ((node.kind == ResourceKind::ForestTree || node.kind == ResourceKind::Sapling) && blocks(node))
                return Bad(blockedMessage);
            const bool inside = node.position.x >= left && node.position.x < left + size
                && node.position.y >= bottom && node.position.y < bottom + size;
            if (inside && IsOvergrowth(node.kind)) return Bad(SpoiledGroundMessage(node));
        }
        return Good("");
    }
    Generation::ChunkCoord low, high;
    const auto lowStatus = Generation::ChunkAt(static_cast<std::int64_t>(left - 50),
        static_cast<std::int64_t>(bottom - 50), low);
    const auto highStatus = Generation::ChunkAt(static_cast<std::int64_t>(left + size + 50),
        static_cast<std::int64_t>(bottom + size + 50), high);
    if (lowStatus != Generation::Status::Ok) return GenerationFailure(lowStatus);
    if (highStatus != Generation::Status::Ok) return GenerationFailure(highStatus);
    for (int cy = low.y; cy <= high.y; ++cy)
        for (int cx = low.x; cx <= high.x; ++cx)
        {
            Generation::ChunkBaseline baseline;
            const auto status = Generation::GenerateChunk(state.world, {cx, cy}, baseline);
            if (status != Generation::Status::Ok) return GenerationFailure(status);
            for (const auto& entity : baseline.entities)
            {
                if (entity.kind != Generation::EntityKind::ForestTree && entity.kind != Generation::EntityKind::Sapling) continue;
                ResourceNode node;
                if (!GeneratedNode(state, entity, node) || node.cleared) continue;
                if (blocks(node)) return Bad(blockedMessage);
            }
        }
    return Good("");
}
// Standing trees within 50 cm of the ground a piece covers, or saplings inside it, block it.
// Walls, doorways and roofs are checked against their whole cell, like the foundation beneath.
Result CheckFootprintResources(const State& state, const Footprint& area, bool quick)
{
    const char* blockedMessage = "Fell the standing tree or clear the sapling before using this building cell.";
    const auto blocks = [&](const ResourceNode& node)
    {
        if (node.cleared || (node.kind != ResourceKind::ForestTree && node.kind != ResourceKind::Sapling)) return false;
        const Point local = RotateYaw({node.position.x - area.center.x, node.position.y - area.center.y}, -area.yaw);
        if (node.kind == ResourceKind::Sapling)
            return std::abs(local.x) < area.half.x && std::abs(local.y) < area.half.y;
        const double dx = std::max(0.0, std::abs(local.x) - area.half.x);
        const double dy = std::max(0.0, std::abs(local.y) - area.half.y);
        return dx * dx + dy * dy <= 50.0 * 50.0;
    };
    // The fixed estate's trees are all in state.resources; it has no generated chunks.
    if (quick || state.fixedEstate)
    {
        for (const auto& node : state.resources)
        {
            if (blocks(node)) return Bad(blockedMessage);
            if (!node.cleared && IsOvergrowth(node.kind) && node.kind != ResourceKind::Sapling)
            {
                const Point local = RotateYaw({node.position.x - area.center.x, node.position.y - area.center.y}, -area.yaw);
                if (std::abs(local.x) < area.half.x && std::abs(local.y) < area.half.y)
                    return Bad(SpoiledGroundMessage(node));
            }
        }
        return Good("");
    }
    const Point x = RotateYaw({area.half.x, 0}, area.yaw), y = RotateYaw({0, area.half.y}, area.yaw);
    const double reachX = std::abs(x.x) + std::abs(y.x) + 50.0, reachY = std::abs(x.y) + std::abs(y.y) + 50.0;
    Generation::ChunkCoord low, high;
    const auto lowStatus = Generation::ChunkAt(static_cast<std::int64_t>(area.center.x - reachX),
        static_cast<std::int64_t>(area.center.y - reachY), low);
    const auto highStatus = Generation::ChunkAt(static_cast<std::int64_t>(area.center.x + reachX),
        static_cast<std::int64_t>(area.center.y + reachY), high);
    if (lowStatus != Generation::Status::Ok) return GenerationFailure(lowStatus);
    if (highStatus != Generation::Status::Ok) return GenerationFailure(highStatus);
    for (int cy = low.y; cy <= high.y; ++cy)
        for (int cx = low.x; cx <= high.x; ++cx)
        {
            Generation::ChunkBaseline baseline;
            const auto status = Generation::GenerateChunk(state.world, {cx, cy}, baseline);
            if (status != Generation::Status::Ok) return GenerationFailure(status);
            for (const auto& entity : baseline.entities)
            {
                if (entity.kind != Generation::EntityKind::ForestTree && entity.kind != Generation::EntityKind::Sapling) continue;
                ResourceNode node;
                if (GeneratedNode(state, entity, node) && blocks(node)) return Bad(blockedMessage);
            }
        }
    return Good("");
}
Footprint ResourceFootprint(const Building& building, Piece kind, int cellX, int cellY, int rotation, bool onFoundation,
    Point spot = {})
{
    const bool own = IsFurniture(kind) || Crafting::IsFence(kind);
    return own ? PieceFootprint(building, kind, cellX, cellY, rotation, onFoundation, spot)
        : PieceFootprint(building, Piece::Foundation, cellX, cellY, 0, true);
}
Footprint GardenFootprint(const Plot& plot)
{
    return {PlotCenter(plot), {GardenCellSize * 0.5, GardenCellSize * 0.5}, 0.0};
}
// A slightly shrunken copy, so neighbours that only touch never count as overlapping.
Footprint Inset(Footprint area, double amount = 1.0)
{
    area.half.x = std::max(0.0, area.half.x - amount);
    area.half.y = std::max(0.0, area.half.y - amount);
    return area;
}
std::uint64_t Checksum(const std::string& body)
{
    std::uint64_t hash = UINT64_C(14695981039346656037);
    for (unsigned char c : body) { hash ^= c; hash *= UINT64_C(1099511628211); }
    return hash;
}
bool ReadBool(std::istream& stream, bool& value)
{
    int number = -1;
    if (!(stream >> number) || (number != 0 && number != 1)) return false;
    value = number == 1;
    return true;
}
bool ReadUnsigned(std::istream& stream, std::uint64_t& value)
{
    std::string token;
    if (!(stream >> token) || token.empty() || token.size() > 20) return false;
    std::uint64_t parsed = 0;
    for (char c : token)
    {
        if (c < '0' || c > '9') return false;
        const auto digit = static_cast<std::uint64_t>(c - '0');
        if (parsed > (std::numeric_limits<std::uint64_t>::max() - digit) / 10) return false;
        parsed = parsed * 10 + digit;
    }
    value = parsed;
    return true;
}
// Width first, then one count per item in enum order (version 13 on).
void WriteStock(std::ostream& stream, const Inventory& stock)
{
    stream << ' ' << ItemCount;
    for (int value : stock) stream << ' ' << value;
    stream << '\n';
}
enum class SavedStock { Ok, Invalid, Newer };
// Reads one saved item stock. Version 13 stocks carry their width; a narrower stock (from before
// items were appended) leaves the new items at zero, and a wider one came from a newer build.
// Version 12 stocks are the rest of the current line, as wide as the writing build's catalogue.
// Older versions have the fixed width `legacyWidth`.
SavedStock ReadSavedStock(std::istream& stream, Inventory& stock, int version, int legacyWidth, int capacity)
{
    stock.fill(0);
    if (version > PositionalStockSaveVersion)
    {
        int width = 0;
        if (!(stream >> width) || width < 1) return SavedStock::Invalid;
        if (width > ItemCount) return SavedStock::Newer;
        for (int i = 0; i < width; ++i) if (!(stream >> stock[i])) return SavedStock::Invalid;
    }
    else if (version == PositionalStockSaveVersion)
    {
        std::string line;
        if (!std::getline(stream, line)) return SavedStock::Invalid;
        std::istringstream values(line);
        values.imbue(std::locale::classic());
        std::vector<int> read;
        for (int value = 0; values >> value;) read.push_back(value);
        values >> std::ws;
        if (!values.eof()) return SavedStock::Invalid;
        const int width = static_cast<int>(read.size());
        if (width < PositionalStockMinimumItems) return SavedStock::Invalid;
        if (width > ItemCount) return SavedStock::Newer;
        std::copy(read.begin(), read.end(), stock.begin());
    }
    else
    {
        for (int i = 0; i < legacyWidth; ++i) if (!(stream >> stock[i])) return SavedStock::Invalid;
    }
    return StockValid(stock, capacity) ? SavedStock::Ok : SavedStock::Invalid;
}
constexpr unsigned Slot(EquipmentSlot slot) { return 1u << static_cast<int>(slot); }
constexpr WearableDefinitionInfo Wearables[] = {
    {WearableDefinition::LinenTunic, "linen-tunic", "Linen tunic",
        Slot(EquipmentSlot::Torso) | Slot(EquipmentSlot::Legs), true, 12},
    {WearableDefinition::LinenApron, "linen-apron", "Linen apron", Slot(EquipmentSlot::Apron), true, 6},
    {WearableDefinition::LeatherShoes, "legacy-laceup-shoes", "Leather shoes", Slot(EquipmentSlot::Feet), false, 0},
    {WearableDefinition::WovenFootwraps, "woven-footwraps", "Woven footwraps", Slot(EquipmentSlot::Feet), false, 8},
    {WearableDefinition::LinenShirt, "linen-shirt", "Linen shirt", Slot(EquipmentSlot::Torso), false, 8, 0},
    {WearableDefinition::LinenLongShirt, "long-linen-shirt", "Long-sleeved linen shirt",
        Slot(EquipmentSlot::Torso), false, 12, 0},
    {WearableDefinition::Trousers, "trousers", "Homespun trousers", Slot(EquipmentSlot::Legs), false, 14, 0},
    {WearableDefinition::FurCoat, "fur-coat", "Fur coat", Slot(EquipmentSlot::Outer), false, 4, 6},
    {WearableDefinition::FurBoots, "fur-boots", "Fur boots", Slot(EquipmentSlot::Feet), false, 2, 3},
    {WearableDefinition::WovenSandals, "woven-sandals", "Woven sandals", Slot(EquipmentSlot::Feet), false, 6, 0},
    {WearableDefinition::TurnShoes, "turnshoes", "Turnshoes", Slot(EquipmentSlot::Feet), false, 2, 2}
};
static_assert(sizeof(Wearables) / sizeof(Wearables[0]) == static_cast<int>(WearableDefinition::Count));
bool InContainer(const WearableInstance& item, int container)
{
    return container == 0 ? item.owner == WearableOwner::Carried :
        item.owner == WearableOwner::Chest && item.chestId == container;
}
const Inventory* ContainerStock(const State& state, int container)
{
    if (container == 0) return &state.inventory;
    const auto* chest = Find(state.structures, container);
    return chest && chest->kind == Piece::Chest ? &chest->storage : nullptr;
}
Inventory* ContainerStock(State& state, int container)
{
    if (container == 0) return &state.inventory;
    auto* chest = Find(state.structures, container);
    return chest && chest->kind == Piece::Chest ? &chest->storage : nullptr;
}
const InventoryLayout* ContainerLayout(const State& state, int container)
{
    if (container == 0) return &state.inventoryLayout;
    const auto* chest = Find(state.structures, container);
    return chest && chest->kind == Piece::Chest ? &chest->layout : nullptr;
}
InventoryLayout* ContainerLayout(State& state, int container)
{
    if (container == 0) return &state.inventoryLayout;
    auto* chest = Find(state.structures, container);
    return chest && chest->kind == Piece::Chest ? &chest->layout : nullptr;
}
int ContainerUsed(const State& state, int container)
{
    const auto* stock = ContainerStock(state, container);
    if (!stock) return -1;
    int total = 0;
    for (int i = 0; i < ItemCount; ++i)
        if (TakesSpace(static_cast<Item>(i))) total += (*stock)[i];
    for (const auto& item : state.wearables) if (InContainer(item, container)) ++total;
    return total;
}
Result ContainerAccess(const State& state, int container, Point player)
{
    if (container == 0) return Good("");
    const auto* chest = Find(state.structures, container);
    if (!chest || chest->kind != Piece::Chest) return Bad("Choose an existing storage chest.");
    if (!Near(player, Homestead::StructureCenter(state, *chest), ChestReach))
        return Bad("Move within 280 cm of this chest.");
    return Good("");
}
bool CanAllocate(int next) { return next > 0 && next < std::numeric_limits<int>::max() - 1; }
int InventoryCategory(Item item) { return ItemSortRank(item); }

bool ReconcileLayout(State& state, int container)
{
    auto* layout = ContainerLayout(state, container);
    const auto* stock = ContainerStock(state, container);
    if (!layout || !stock) return false;
    layout->erase(std::remove_if(layout->begin(), layout->end(), [&](const LayoutEntry& entry) {
        if (entry.wearableId == 0) return false;
        const auto* item = Find(state.wearables, entry.wearableId);
        return !item || !InContainer(*item, container);
    }), layout->end());
    for (int i = 0; i < ItemCount; ++i)
    {
        int displayed = 0;
        for (const auto& entry : *layout)
            if (entry.wearableId == 0 && static_cast<int>(entry.item) == i) displayed += entry.quantity;
        int difference = (*stock)[i] - displayed;
        for (auto& entry : *layout)
        {
            if (entry.wearableId != 0 || static_cast<int>(entry.item) != i) continue;
            if (difference > 0) { entry.quantity += difference; difference = 0; }
            else if (difference < 0)
            {
                const int removed = std::min(entry.quantity, -difference);
                entry.quantity -= removed;
                difference += removed;
            }
        }
        if (difference > 0)
        {
            if (!CanAllocate(state.nextGroupId)) return false;
            layout->push_back({state.nextGroupId++, static_cast<Item>(i), difference, 0});
        }
    }
    layout->erase(std::remove_if(layout->begin(), layout->end(), [](const LayoutEntry& entry) {
        return entry.wearableId == 0 && entry.quantity == 0;
    }), layout->end());
    for (const auto& item : state.wearables)
    {
        if (!InContainer(item, container)) continue;
        const auto found = std::find_if(layout->begin(), layout->end(),
            [&](const LayoutEntry& entry) { return entry.wearableId == item.id; });
        if (found == layout->end()) layout->push_back({0, Item::Knife, 0, item.id});
    }
    return true;
}
Result ValidateInventory(const State& state)
{
    if (!CanAllocate(state.nextWearableId) || !CanAllocate(state.nextGroupId) || state.wearables.size() > MaxObjects)
        return Bad("The wardrobe identity allocator is invalid or exhausted.");
    std::set<int> ids, groupIds, displayed;
    std::array<int, EquipmentSlotCount> equipment{};
    for (const auto& item : state.wearables)
    {
        const auto* definition = GetWearableDefinition(item.definition);
        if (item.id <= 0 || item.id >= state.nextWearableId || !ids.insert(item.id).second || !definition ||
            item.dye < 0 || item.dye > (definition->dyeable ? 3 : 0))
            return Bad("A garment has an invalid identity, definition or dye.");
        switch (item.owner)
        {
        case WearableOwner::Carried:
            if (item.chestId != 0) return Bad("A carried garment also names a chest.");
            break;
        case WearableOwner::Chest:
            if (item.chestId <= 0 || !ContainerStock(state, item.chestId))
                return Bad("A garment names a missing chest.");
            break;
        case WearableOwner::Equipped:
            if (item.chestId != 0) return Bad("An equipped garment also names a chest.");
            for (int slot = 0; slot < EquipmentSlotCount; ++slot)
            {
                if ((definition->slots & (1u << slot)) == 0) continue;
                if (equipment[slot] != 0) return Bad("Two garments occupy the same equipment slot.");
                equipment[slot] = item.id;
            }
            break;
        case WearableOwner::World:
            if (item.chestId != 0) return Bad("A world garment also names a chest.");
            break;
        default: return Bad("A garment has an unknown owner.");
        }
    }
    if (equipment != state.equipment ||
        (equipment[static_cast<int>(EquipmentSlot::Apron)] != 0 &&
         equipment[static_cast<int>(EquipmentSlot::Torso)] == 0))
        return Bad("Equipment references or dependent garment layers are invalid.");
    const auto validateContainer = [&](int container) -> Result {
        const auto* stock = ContainerStock(state, container);
        const auto* layout = ContainerLayout(state, container);
        const int capacity = ContainerCapacity(container);
        if (!stock || !layout || !StockValid(*stock, capacity) || ContainerUsed(state, container) > capacity)
            return {false, container == 0 ? "Not enough pack space." : "The chest does not have enough space.", ResultCode::Capacity};
        if (layout->size() > static_cast<std::size_t>(capacity)) return Bad("Inventory layout has too many entries.");
        Inventory total{};
        for (const auto& entry : *layout)
        {
            if (entry.wearableId != 0)
            {
                const auto* item = Find(state.wearables, entry.wearableId);
                if (!item || !InContainer(*item, container) || entry.groupId != 0 || entry.quantity != 0 ||
                    entry.item != Item::Knife || !displayed.insert(entry.wearableId).second)
                    return Bad("A garment layout reference is invalid or duplicated.");
            }
            else
            {
                if (entry.groupId <= 0 || entry.groupId >= state.nextGroupId || !groupIds.insert(entry.groupId).second ||
                    !ValidEnum(entry.item, Item::Count) || entry.quantity <= 0 || entry.quantity > capacity)
                    return Bad("A fungible group has invalid identity, item or quantity.");
                total[static_cast<int>(entry.item)] += entry.quantity;
            }
        }
        if (total != *stock) return Bad("Inventory layout does not partition the stored quantities.");
        return Good("");
    };
    auto result = validateContainer(0);
    if (!result) return result;
    for (const auto& piece : state.structures)
    {
        if (piece.kind == Piece::Chest)
        {
            result = validateContainer(piece.id);
            if (!result) return result;
        }
        else if (!piece.layout.empty()) return Bad("Only chests may contain inventory layout.");
    }
    if (groupIds.size() > MaxObjects) return Bad("The homestead has reached its inventory group limit.");
    std::set<int> dropIds, droppedWearables;
    if (state.worldDrops.size() > MaxWorldDrops) return Bad("The world drop limit is exceeded.");
    for (const auto& drop : state.worldDrops)
    {
        if (drop.id <= 0 || drop.id >= state.nextId || !dropIds.insert(drop.id).second
            || !ValidPoint(drop.position) || std::abs(drop.position.x) > MaxWorldCoordinate
            || std::abs(drop.position.y) > MaxWorldCoordinate)
            return Bad("A world drop has invalid identity or position.");
        if (drop.wearableId == 0)
        {
            if (!ValidEnum(drop.item, Item::Count) || drop.quantity <= 0
                || drop.quantity > InventoryCapacity)
                return Bad("A world item drop has an invalid payload.");
        }
        else
        {
            const auto* wearable = Find(state.wearables, drop.wearableId);
            if (drop.item != Item::Count || drop.quantity != 1 || !wearable
                || wearable->owner != WearableOwner::World
                || !droppedWearables.insert(drop.wearableId).second)
                return Bad("A world garment drop has invalid ownership.");
        }
    }
    for (const auto& item : state.wearables)
        if (item.owner != WearableOwner::Equipped && item.owner != WearableOwner::World
            && !displayed.count(item.id))
            return Bad("A stored garment is missing its layout reference.");
        else if (item.owner == WearableOwner::World && !droppedWearables.count(item.id))
            return Bad("A world garment is missing its drop record.");
    return Good("");
}
void RefreshEquipment(State& state)
{
    state.equipment.fill(0);
    for (const auto& item : state.wearables)
    {
        if (item.owner != WearableOwner::Equipped) continue;
        const auto* definition = GetWearableDefinition(item.definition);
        if (!definition) continue; // Validation rejects unknown definitions before commit.
        for (int slot = 0; slot < EquipmentSlotCount; ++slot)
            if (definition->slots & (1u << slot)) state.equipment[slot] = item.id;
    }
}
void WriteLayout(std::ostream& output, const InventoryLayout& layout)
{
    output << layout.size() << '\n';
    for (const auto& entry : layout)
        output << entry.groupId << ' ' << static_cast<int>(entry.item) << ' ' << entry.quantity << ' ' << entry.wearableId << '\n';
}
bool ReadLayout(std::istream& input, InventoryLayout& layout)
{
    int count = 0;
    if (!(input >> count) || count < 0 || count > ChestCapacity) return false;
    for (int i = 0; i < count; ++i)
    {
        LayoutEntry entry;
        int item = 0;
        if (!(input >> entry.groupId >> item >> entry.quantity >> entry.wearableId)) return false;
        entry.item = static_cast<Item>(item);
        layout.push_back(entry);
    }
    return true;
}
}

namespace Detail
{
bool SaveResourceEdit(State& candidate, const ResourceNode& node) { return Homestead::SaveResourceEdit(candidate, node); }
void EraseResourceEdit(State& candidate, const Generation::GeneratedEntityKey& key)
{
    auto& edits = candidate.resourceEdits;
    const auto edit = std::lower_bound(edits.begin(), edits.end(), key,
        [](const ResourceEdit& value, const Generation::GeneratedEntityKey& wanted) { return value.key < wanted; });
    if (edit != edits.end() && edit->key == key) edits.erase(edit);
}
int PackUsed(const State& state) { return ContainerUsed(state, 0); }
bool AddWorldDrop(State& candidate, Point position, Item item, int quantity)
{
    for (auto& drop : candidate.worldDrops)
        if (drop.wearableId == 0 && drop.item == item && drop.quantity <= InventoryCapacity - quantity
            && DistanceSquared(position, drop.position) <= DropMergeReach * DropMergeReach)
        {
            drop.quantity += quantity;
            return true;
        }
    if (candidate.worldDrops.size() >= MaxWorldDrops || candidate.nextId >= TransientResourceIdBase - 1) return false;
    candidate.worldDrops.push_back({candidate.nextId++, position, item, quantity, 0});
    return true;
}
}

const WearableDefinitionInfo* GetWearableDefinition(WearableDefinition definition)
{
    return ValidEnum(definition, WearableDefinition::Count) ? &Wearables[static_cast<int>(definition)] : nullptr;
}
const char* WearableName(WearableDefinition definition)
{
    const auto* info = GetWearableDefinition(definition);
    return info ? info->name : "Unknown garment";
}
const char* WearableDescription(WearableDefinition definition)
{
    switch (definition)
    {
    case WearableDefinition::LinenTunic: return "A fiber-worked tunic covering torso and legs.";
    case WearableDefinition::LinenApron: return "A separate apron worn over a linen tunic.";
    case WearableDefinition::LeatherShoes: return "Lace-up shoes with socks from older saves. Authored color; not craftable.";
    case WearableDefinition::WovenFootwraps: return "Fiber-woven footwear, worn instead of shoes. Authored color.";
    case WearableDefinition::LinenShirt: return "A short-sleeved shirt of undyed linen, loosely woven for warm days.";
    case WearableDefinition::LinenLongShirt: return "A long-sleeved linen shirt with a drawstring neck.";
    case WearableDefinition::Trousers: return "Close-woven homespun trousers, tied at the waist and snug below the knee.";
    case WearableDefinition::FurCoat: return "A hide coat worn fur-side in, sewn with fiber thread.";
    case WearableDefinition::FurBoots: return "Tall hide boots with a fur lining and a turned-down cuff.";
    case WearableDefinition::WovenSandals: return "Plaited fiber soles tied on with cords. Cool and light.";
    case WearableDefinition::TurnShoes: return "Soft hide shoes sewn inside out and turned, laced at the instep.";
    default: return "Unknown garment";
    }
}
const char* DyeName(int dye)
{
    static const char* names[] = {"Moss", "Wine", "Slate", "Flax"};
    return dye >= 0 && dye < 4 ? names[dye] : "Unknown dye";
}
const char* GarmentRequirements(WearableDefinition definition)
{
    static const auto requirements = [] {
        std::array<std::string, static_cast<int>(WearableDefinition::Count)> result{};
        for (const auto& info : Wearables)
        {
            std::string& text = result[static_cast<int>(info.id)];
            if (info.fiberCost <= 0 && info.furCost <= 0) { text = "Starter footwear; not craftable"; continue; }
            text = std::to_string(info.fiberCost) + " Fiber";
            if (info.furCost > 0) text += " + " + std::to_string(info.furCost) + " Fur";
            text += info.furCost > 0 ? "; knife required; cut and sew the hide" : "; knife required; work fiber into cloth";
        }
        return result;
    }();
    return GetWearableDefinition(definition) ? requirements[static_cast<int>(definition)].c_str() : "Unknown garment";
}
const char* ResourceName(ResourceKind kind)
{
    static const char* names[] = {"Fallen branches", "Loose stones", "Berry bush", "Wild roots",
        "Meadow herb", "Stream reeds", "Sapling", "Forest tree", "Deer remains",
        "Tall grass", "Weeds", "Thin bramble", "Bramble thicket", "Bramble bank", "Fallen bough",
        "Small stump", "Large stump", "Ancient stump", "Fallen log", "Giant log", "Rubble", "Small rock", "Boulder",
        "Salvage pile", "Primroses", "Bluebells", "Wild daffodils", "Wild garlic",
        "Nettles", "Stump", "Broken crate", "Broken barrel", "Rubbish heap", "Rotten planks"};
    static_assert(sizeof(names) / sizeof(names[0]) == static_cast<int>(ResourceKind::Count), "Every resource needs a name.");
    return ValidEnum(kind, ResourceKind::Count) ? names[static_cast<int>(kind)] : "Unknown resource";
}
const char* RecipeName(Recipe recipe)
{
    static const char* names[] = {"Craft an axe", "Craft a hoe", "Craft a scythe", "Craft a billhook", "Craft a pickaxe",
        "Roasted roots", "Herbed roots", "Split firewood",
        "Saw planks", "Fence section", "Field gate", "Stool", "Table", "Chair", "Shelf"};
    static_assert(sizeof(names) / sizeof(names[0]) == static_cast<int>(Recipe::Count), "Every recipe needs a name.");
    return ValidEnum(recipe, Recipe::Count) ? names[static_cast<int>(recipe)] : "Unknown recipe";
}
const char* PieceName(Piece piece)
{
    static const char* names[] = {"Foundation", "Wall", "Doorway", "Roof", "Cookfire", "Bed", "Chest", "Hearth",
        "Workbench", "Sawhorse", "Fence", "Field gate", "Stool", "Table", "Chair", "Shelf"};
    static_assert(sizeof(names) / sizeof(names[0]) == static_cast<int>(Piece::Count), "Every piece needs a name.");
    return ValidEnum(piece, Piece::Count) ? names[static_cast<int>(piece)] : "Unknown structure";
}
const char* CropName(CropKind kind)
{
    return GetCropInfo(kind).name;
}
const char* RecipeRequirements(Recipe recipe)
{
    // Build once from the transaction definitions; returned pointers remain stable for HUD callers.
    static const auto descriptions = [] {
        std::array<std::string, static_cast<int>(Recipe::Count)> result{};
        for (int i = 0; i < static_cast<int>(Recipe::Count); ++i)
        {
            const auto kind = static_cast<Recipe>(i);
            result[i] = DescribeCost(CraftChange(kind));
            if (kind == Recipe::RoastedRoots || kind == Recipe::HerbedRoots)
                result[i] += "; nearby fueled fire (no pot needed)";
            else if (kind == Recipe::SplitFirewood)
                result[i] += "; axe required";
            else if (Crafting::StationFor(kind) == Piece::Sawhorse)
                result[i] += "; at a sawhorse";
            else if (Crafting::StationFor(kind) == Piece::Workbench)
                result[i] += "; at a workbench";
            else
                result[i] += "; by hand, no station";
        }
        return result;
    }();
    return ValidEnum(recipe, Recipe::Count) ? descriptions[static_cast<int>(recipe)].c_str() : "Unknown recipe";
}
const char* PieceRequirements(Piece piece)
{
    static const auto descriptions = [] {
        std::array<std::string, static_cast<int>(Piece::Count)> result{};
        for (int i = 0; i < static_cast<int>(Piece::Count); ++i)
        {
            const auto kind = static_cast<Piece>(i);
            result[i] = DescribeCost(BuildCost(kind));
            if (EdgePiece(kind) || kind == Piece::Roof) result[i] += "; foundation required";
            if (kind == Piece::Fire) result[i] += "; add firewood or a branch after placement to light";
            if (kind == Piece::Hearth) result[i] = "Part of the old house; always lit";
            if (Crafting::IsFence(kind)) result[i] += "; on your own land, joins end to end at the posts";
            if (Crafting::IsStation(kind)) result[i] += "; a station for period crafts";
        }
        return result;
    }();
    return ValidEnum(piece, Piece::Count) ? descriptions[static_cast<int>(piece)].c_str() : "Unknown structure";
}
bool IsBuildable(Piece piece) { return ValidEnum(piece, Piece::Count) && piece != Piece::Hearth; }
bool IsFurniture(Piece piece)
{
    return piece == Piece::Fire || piece == Piece::Bed || piece == Piece::Chest || piece == Piece::Hearth
        || Crafting::IsStation(piece) || Crafting::IsMovable(piece);
}
double StreamX(double y) { return Generation::StreamCenterCm(y); }
bool IsNearWater(Point position)
{
    return ValidPoint(position) && std::abs(position.x - StreamX(position.y)) <= 180.0;
}
int GardenCell(double value) { return static_cast<int>(std::floor(value / GardenCellSize)); }
int GardenToCell(int garden)
{
    return garden >= 0 ? garden / GardenCellsPerCell : -((-garden + GardenCellsPerCell - 1) / GardenCellsPerCell);
}
Point GardenCellCenter(int gardenX, int gardenY)
{
    return {(static_cast<double>(gardenX) + 0.5) * GardenCellSize, (static_cast<double>(gardenY) + 0.5) * GardenCellSize};
}
Point PlotCenter(const Plot& plot) { return GardenCellCenter(plot.cellX, plot.cellY); }
bool PlotInCell(const Plot& plot, int cellX, int cellY)
{
    return GardenToCell(plot.cellX) == cellX && GardenToCell(plot.cellY) == cellY;
}
Point CellCenter(int cellX, int cellY)
{
    return {(static_cast<double>(cellX) + 0.5) * CellSize,
        (static_cast<double>(cellY) + 0.5) * CellSize};
}
Point RotateYaw(Point value, double yaw)
{
    const double radians = yaw * 3.14159265358979323846 / 180.0;
    const double c = std::cos(radians), s = std::sin(radians);
    return {value.x * c - value.y * s, value.x * s + value.y * c};
}
const Building* FindBuilding(const State& state, int buildingId)
{
    static const Building worldGrid{};
    if (buildingId == 0) return &worldGrid;
    for (const auto& building : state.buildings)
        if (building.id == buildingId) return &building;
    return nullptr;
}
Point BuildingCellCenter(const Building& building, int cellX, int cellY)
{
    const Point local = RotateYaw(CellCenter(cellX, cellY), building.yaw);
    return {building.origin.x + local.x, building.origin.y + local.y};
}
Point BuildingLocal(const Building& building, Point world)
{
    return RotateYaw({world.x - building.origin.x, world.y - building.origin.y}, -building.yaw);
}
Point StructureCenter(const State& state, const Structure& structure)
{
    const Building* building = FindBuilding(state, structure.buildingId);
    return BuildingCellCenter(building ? *building : Building{}, structure.cellX, structure.cellY);
}
double PieceYaw(const Building& building, int rotation) { return building.yaw - 90.0 * (rotation % 4); }
double StructureYaw(const State& state, const Structure& structure)
{
    const Building* building = FindBuilding(state, structure.buildingId);
    return PieceYaw(building ? *building : Building{}, structure.rotation);
}
bool HasFoundation(const State& state, int buildingId, int cellX, int cellY)
{
    for (const auto& piece : state.structures)
        if (piece.kind == Piece::Foundation && piece.buildingId == buildingId
            && piece.cellX == cellX && piece.cellY == cellY) return true;
    return false;
}
Point FurnitureOffset(Piece kind)
{
    switch (kind)
    {
    case Piece::Bed: return {95.0, -10.0};
    case Piece::Chest: return {-100.0, -100.0};
    case Piece::Fire: return {-100.0, 95.0};
    // Against the wall on the piece's own edge, its back to the masonry's inner face.
    case Piece::Hearth: return {0.0, 98.0};
    // The workbench stands along the piece's own edge, the sawhorse just in from it.
    case Piece::Workbench: return {0.0, 100.0};
    case Piece::Sawhorse: return {0.0, 50.0};
    default: return {};
    }
}
Footprint PieceFootprint(const Building& building, Piece kind, int cellX, int cellY, int rotation, bool onFoundation,
    Point spot)
{
    const Point center = BuildingCellCenter(building, cellX, cellY);
    const double yaw = PieceYaw(building, rotation);
    const auto at = [&](Point offset, Point half)
    {
        const Point turned = RotateYaw(offset, yaw);
        return Footprint{{center.x + turned.x, center.y + turned.y}, half, yaw};
    };
    switch (kind)
    {
    case Piece::Wall:
    case Piece::Doorway: return at({0.0, 144.0}, {150.0, 8.0});
    case Piece::Bed: return at(onFoundation ? FurnitureOffset(kind) : Point{}, {35.0, 78.0});
    case Piece::Chest: return at(onFoundation ? FurnitureOffset(kind) : Point{}, {35.0, 28.0});
    case Piece::Fire: return at(onFoundation ? FurnitureOffset(kind) : Point{}, {40.0, 40.0});
    case Piece::Hearth: return at(onFoundation ? FurnitureOffset(kind) : Point{}, {85.0, 32.0});
    case Piece::Workbench:
    case Piece::Sawhorse: return at(onFoundation ? FurnitureOffset(kind) : Point{}, Crafting::FurnitureHalf(kind));
    case Piece::Stool:
    case Piece::Table:
    case Piece::Chair:
    case Piece::Shelf: return at(onFoundation ? spot : Point{}, Crafting::FurnitureHalf(kind));
    case Piece::FenceRail:
    case Piece::FenceGate: return at({}, {Crafting::FenceFootprintHalfLength, Crafting::FenceFootprintHalfWidth});
    default: return {center, {CellSize * 0.5, CellSize * 0.5}, building.yaw};
    }
}
Footprint StructureFootprint(const State& state, const Structure& structure)
{
    const Building* building = FindBuilding(state, structure.buildingId);
    return PieceFootprint(building ? *building : Building{}, structure.kind, structure.cellX, structure.cellY,
        structure.rotation, HasFoundation(state, structure.buildingId, structure.cellX, structure.cellY), structure.spot);
}
bool FootprintsOverlap(const Footprint& a, const Footprint& b)
{
    // Separating axes: the two axes of each rectangle.
    const Point delta{b.center.x - a.center.x, b.center.y - a.center.y};
    const Point axes[] = {RotateYaw({1, 0}, a.yaw), RotateYaw({0, 1}, a.yaw),
        RotateYaw({1, 0}, b.yaw), RotateYaw({0, 1}, b.yaw)};
    const auto radius = [](const Footprint& box, Point axis)
    {
        const Point x = RotateYaw({1, 0}, box.yaw), y = RotateYaw({0, 1}, box.yaw);
        return box.half.x * std::abs(x.x * axis.x + x.y * axis.y) + box.half.y * std::abs(y.x * axis.x + y.y * axis.y);
    };
    for (const Point& axis : axes)
        if (std::abs(delta.x * axis.x + delta.y * axis.y) >= radius(a, axis) + radius(b, axis)) return false;
    return true;
}
std::vector<SleepOption> SleepOptions(double hour, double energy)
{
    double current = std::fmod(hour, 24.0);
    if (current < 0.0) current += 24.0;
    const auto wake = [current](double hours) { return std::fmod(current + hours, 24.0); };
    std::vector<SleepOption> options;
    const bool evening = current >= 18.0 || current < 5.0;
    const double toMorning = std::fmod(MorningWakeHour - current + 48.0, 24.0);
    if (evening && toMorning >= Exertion::NapHours)
        options.push_back({SleepChoice::UntilMorning, toMorning, MorningWakeHour});
    const double deficit = Clamp(100.0 - (std::isfinite(energy) ? energy : 0.0), 0.0, 100.0);
    const double rested = Clamp(std::ceil(deficit / Exertion::SleepPerHour * 4.0 - 1e-9) / 4.0,
        Exertion::MinRestHours, Exertion::MaxRestHours);
    if (options.empty() || std::abs(rested - options.front().hours) > 0.75)
        options.push_back({SleepChoice::UntilRested, rested, wake(rested)});
    bool shortOffered = false;
    for (const auto& option : options) shortOffered |= option.hours <= Exertion::NapHours + 0.01;
    if (!shortOffered) options.push_back({SleepChoice::Nap, Exertion::NapHours, wake(Exertion::NapHours)});
    return options;
}

Simulation::Simulation() { NewGame(); }

namespace
{
// Fixed-estate resources reuse the ResourceEdit machinery with a synthetic key: chunk (0, 0) and
// the placement id, which is unique within one bake.
Generation::GeneratedEntityKey EstateKey(int placementId)
{
    Generation::GeneratedEntityKey key{};
    key.localId = static_cast<std::uint32_t>(placementId);
    return key;
}

// Rebuilds the estate's resource nodes from the baked placements plus saved edits.
Result MaterializeEstate(State& candidate, const EstatePlacements& placements)
{
    std::vector<ResourceNode> resources;
    resources.reserve(placements.placements.size());
    std::set<int> seen;
    for (const auto& placement : placements.placements)
    {
        if (placement.id < EstatePlacementIdBase || placement.id >= TransientResourceIdBase
            || !seen.insert(placement.id).second || !ValidPoint(placement.position)
            || static_cast<int>(placement.kind) < 0 || placement.kind >= ResourceKind::Count)
            return Bad("The estate placement table is invalid.");
        ResourceNode node;
        node.id = placement.id;
        node.kind = placement.kind;
        node.position = placement.position;
        node.key = EstateKey(placement.id);
        node.minTier = static_cast<ToolTier>(std::max(0, std::min(placement.minTier, ToolTierCount - 1)));
        const auto edit = std::lower_bound(candidate.resourceEdits.begin(), candidate.resourceEdits.end(), node.key,
            [](const ResourceEdit& value, const Generation::GeneratedEntityKey& key) { return value.key < key; });
        if (edit != candidate.resourceEdits.end() && edit->key == node.key)
        {
            node.cleared = edit->cleared;
            node.readyAtHour = edit->readyAtHour;
        }
        resources.push_back(node);
    }
    candidate.resources = std::move(resources);
    return Good("");
}
}

Result Simulation::NewEstateGame(const EstateLayout& layout, const EstatePlacements& placements)
{
    State candidate;
    candidate.fixedEstate = true;
    candidate.placementBakeVersion = placements.bakeVersion;
    // She arrives in her tunic; clothing is cosmetic on the estate.
    candidate.wearables = {{1, WearableDefinition::LinenTunic, 0, WearableOwner::Equipped, 0}};
    candidate.nextWearableId = 2;
    RefreshEquipment(candidate);
    const auto populated = MaterializeEstate(candidate, placements);
    if (!populated) return populated;
    // Round-1 lanes seed their parts from `layout` here, each in its own helper.
    SeedEstateParcels(candidate, layout);
    SeedEstateShops(candidate, layout);
    candidate.heroineName = Manor::DefaultHeroineName;
    candidate.familyName = Manor::DefaultFamilyName;
    candidate.estateName = Manor::DefaultEstateName;
    candidate.journal.push_back(Manor::ArrivalEntry);
    // The pail is her one starting tool, waiting in the standing room's chest; everything else is
    // hafted from salvage. Test layouts without the room anchor start with no heritage room, so
    // she carries the pail instead.
    if (!Manor::SeedStandingRoom(candidate, layout))
    {
        candidate.inventory[static_cast<int>(Item::WateringCan)] = 1;
        candidate.inventoryLayout.push_back({candidate.nextGroupId++, Item::WateringCan, 1, 0});
    }
    const auto inventory = ValidateInventory(candidate);
    if (!inventory) return inventory;
    layout_ = std::make_shared<const EstateLayout>(layout);
    placements_ = std::make_shared<const EstatePlacements>(placements);
    state_ = std::move(candidate);
    GrantLampKit();
    return {true, "You arrive home to " + state_.estateName + ".", ResultCode::None, ++revision_};
}

Result Simulation::SetNames(const std::string& heroine, const std::string& family, const std::string& estate)
{
    const std::string names[] = {Manor::TrimName(heroine), Manor::TrimName(family), Manor::TrimName(estate)};
    const char* fields[] = {"first name", "surname", "estate name"};
    for (int i = 0; i < 3; ++i)
        if (const std::string problem = Manor::NameProblem(names[i], fields[i]); !problem.empty()) return Bad(problem);
    state_.heroineName = names[0];
    state_.familyName = names[1];
    state_.estateName = names[2];
    return {true, "Welcome home, " + names[0] + ".", ResultCode::None, ++revision_};
}

std::string Simulation::EstateName() const
{
    return state_.estateName.empty() ? std::string("the estate") : state_.estateName;
}

bool Simulation::NearWater(Point position) const
{
    if (state_.fixedEstate)
        return ValidPoint(position) && waterProbe_ && waterProbe_(position);
    return IsNearWater(position);
}

const EstateLayout& Simulation::Layout() const
{
    return layout_ ? *layout_ : ProvisionalEstateLayout();
}

void Simulation::SetLayout(const EstateLayout& layout)
{
    layout_ = std::make_shared<const EstateLayout>(layout);
}

void Simulation::SetPlacements(const EstatePlacements& placements)
{
    placements_ = std::make_shared<const EstatePlacements>(placements);
}
Result Simulation::NewGame() { return NewGame(0); }
Result Simulation::NewGame(std::uint64_t seed)
{
    State candidate;
    candidate.world.seed = seed;
    // She starts barefoot in her tunic and empty-handed; footwear is crafted (woven footwraps).
    candidate.wearables = {{1, WearableDefinition::LinenTunic, 0, WearableOwner::Equipped, 0}};
    candidate.nextWearableId = 2;
    RefreshEquipment(candidate);
    const auto status = Generation::ChunkAt(-1000, 0, candidate.activeChunk);
    if (status != Generation::Status::Ok) return GenerationFailure(status);
    int nextHandle = nextResourceHandle_;
    const auto populated = Materialize(candidate, nullptr, nextHandle);
    if (!populated) return populated;
    state_ = std::move(candidate);
    nextResourceHandle_ = nextHandle;
    return {true, "A new seeded woodland is ready.", ResultCode::None, ++revision_};
}
Result Simulation::SetActiveWorldRegion(Point player,
    const PreparedWorldRegion* prepared)
{
    if (state_.fixedEstate) return {true, "The estate has no streamed regions.", ResultCode::None, revision_};
    if (!ValidPoint(player)) return Bad("Exploration supports coordinates within 1000000 cm of the origin.");
    Generation::ChunkCoord coord;
    const auto status = Generation::ChunkAt(static_cast<std::int64_t>(std::floor(player.x)),
        static_cast<std::int64_t>(std::floor(player.y)), coord);
    if (status != Generation::Status::Ok) return GenerationFailure(status);
    if (prepared && (state_.world.seed != prepared->world.seed
        || state_.world.generationVersion != prepared->world.generationVersion))
        return Bad("Prepared woodland belongs to a different world.");
    if (coord == state_.activeChunk) return {true, "The active region is unchanged.", ResultCode::None, revision_};
    State candidate = state_;
    candidate.activeChunk = coord;
    int nextHandle = nextResourceHandle_;
    const auto populated = Materialize(candidate, &state_, nextHandle, prepared);
    if (!populated) return populated;
    state_ = std::move(candidate);
    nextResourceHandle_ = nextHandle;
    return {true, "Active woodland region updated.", ResultCode::None, ++revision_};
}
Result Simulation::ResolveGeneratedResource(const Generation::GeneratedEntityKey& key, ResourceNode& out) const
{
    Generation::GeneratedEntity entity;
    const auto status = Generation::FindEntity(state_.world, key, entity);
    if (status != Generation::Status::Ok) return GenerationFailure(status);
    ResourceNode node;
    if (!GeneratedNode(state_, entity, node))
        return {false, "This resource is outside the supported world or reserved spawn safety footprint.", ResultCode::Unavailable, revision_};
    for (const auto& active : state_.resources)
        if (active.key == key) { node.id = active.id; break; }
    out = node;
    return {true, "", ResultCode::None, revision_};
}
int Simulation::Count(Item item) const
{
    return ValidEnum(item, Item::Count) ? state_.inventory[static_cast<int>(item)] : 0;
}
int Simulation::UsedCapacity() const
{
    return ContainerUsed(state_, 0);
}
int Simulation::ChestUsedCapacity(int chestId) const
{
    return chestId > 0 ? ContainerUsed(state_, chestId) : -1;
}
const WearableInstance* Simulation::GetWearable(int id) const { return Find(state_.wearables, id); }
const InventoryLayout* Simulation::GetLayout(int containerId) const { return ContainerLayout(state_, containerId); }
bool Simulation::TryAdjust(const Inventory& change)
{
    Inventory updated{};
    for (int i = 0; i < ItemCount; ++i)
    {
        const long long value = static_cast<long long>(state_.inventory[i]) + change[i];
        if (value < 0 || value > InventoryCapacity) return false;
        updated[i] = static_cast<int>(value);
    }
    if (!StockValid(updated)) return false;
    State candidate = state_;
    candidate.inventory = updated;
    if (!ReconcileLayout(candidate, 0) || !ValidateInventory(candidate)) return false;
    // Existing callers retain resource/plot/structure pointers across this helper.
    state_.inventory = candidate.inventory;
    state_.inventoryLayout = std::move(candidate.inventoryLayout);
    state_.nextGroupId = candidate.nextGroupId;
    ++revision_;
    return true;
}
Result Simulation::CheckRevision(std::uint64_t expectedRevision) const
{
    if (expectedRevision != revision_)
        return {false, "Inventory changed. Select the item again before confirming.", ResultCode::StaleRevision, revision_};
    if (state_.failed) return {false, Failed().message, ResultCode::Unavailable, revision_};
    return {true, "", ResultCode::None, revision_};
}
Result Simulation::CommitInventory(State&& candidate, const char* message)
{
    if (!ReconcileLayout(candidate, 0))
        return {false, "Inventory group identities are exhausted.", ResultCode::Unavailable, revision_};
    for (const auto& piece : candidate.structures)
        if (piece.kind == Piece::Chest && !ReconcileLayout(candidate, piece.id))
            return {false, "Storage group identities are exhausted.", ResultCode::Unavailable, revision_};
    auto result = ValidateInventory(candidate);
    if (!result) { result.revision = revision_; return result; }
    state_ = std::move(candidate);
    return {true, message, ResultCode::None, ++revision_};
}

Result Simulation::EquipWearable(int id, std::uint64_t expectedRevision)
{
    const auto ready = CheckRevision(expectedRevision);
    if (!ready) return ready;
    const auto* original = GetWearable(id);
    if (!original || original->owner != WearableOwner::Carried)
        return Bad("Take this garment into your pack before equipping it.");
    const auto* definition = GetWearableDefinition(original->definition);
    if (!definition) return Bad("This garment definition is unavailable.");
    if (original->definition == WearableDefinition::LinenApron &&
        state_.equipment[static_cast<int>(EquipmentSlot::Torso)] == 0)
        return Bad("Equip a linen tunic before wearing an apron.");
    State candidate = state_;
    for (auto& item : candidate.wearables)
    {
        if (item.owner == WearableOwner::Equipped &&
            (GetWearableDefinition(item.definition)->slots & definition->slots) != 0)
            item.owner = WearableOwner::Carried;
    }
    Find(candidate.wearables, id)->owner = WearableOwner::Equipped;
    RefreshEquipment(candidate);
    // Trousers displace a tunic from the legs, which can leave an apron with nothing to tie over.
    if (const int apron = candidate.equipment[static_cast<int>(EquipmentSlot::Apron)];
        apron != 0 && candidate.equipment[static_cast<int>(EquipmentSlot::Torso)] == 0)
    {
        Find(candidate.wearables, apron)->owner = WearableOwner::Carried;
        RefreshEquipment(candidate);
    }
    return CommitInventory(std::move(candidate), "Garment equipped.");
}
Result Simulation::UnequipWearable(int id, std::uint64_t expectedRevision)
{
    const auto ready = CheckRevision(expectedRevision);
    if (!ready) return ready;
    const auto* original = GetWearable(id);
    if (!original || original->owner != WearableOwner::Equipped) return Bad("Choose an equipped garment.");
    State candidate = state_;
    Find(candidate.wearables, id)->owner = WearableOwner::Carried;
    if (GetWearableDefinition(original->definition)->slots & Slot(EquipmentSlot::Torso))
    {
        const int apron = candidate.equipment[static_cast<int>(EquipmentSlot::Apron)];
        if (apron != 0) Find(candidate.wearables, apron)->owner = WearableOwner::Carried;
    }
    RefreshEquipment(candidate);
    return CommitInventory(std::move(candidate), "Garment and any dependent layer moved to your pack.");
}
Result Simulation::MoveWearable(int id, int destinationChestId, Point player, std::uint64_t expectedRevision)
{
    const auto ready = CheckRevision(expectedRevision);
    if (!ready) return ready;
    const auto* original = GetWearable(id);
    if (!original || original->owner == WearableOwner::Equipped)
        return Bad("Unequip this garment into your pack before moving it.");
    if ((original->owner == WearableOwner::Carried && destinationChestId <= 0) ||
        (original->owner == WearableOwner::Chest && destinationChestId != 0))
        return Bad("Move clothing between your pack and one reachable chest.");
    const int chestId = destinationChestId == 0 ? original->chestId : destinationChestId;
    const auto access = ContainerAccess(state_, chestId, player);
    if (!access) return access;
    State candidate = state_;
    auto* item = Find(candidate.wearables, id);
    item->owner = destinationChestId == 0 ? WearableOwner::Carried : WearableOwner::Chest;
    item->chestId = destinationChestId;
    return CommitInventory(std::move(candidate), destinationChestId == 0 ? "Garment taken from chest." : "Garment stored in chest.");
}
Result Simulation::CraftGarment(WearableDefinition definition, Point player, std::uint64_t expectedRevision)
{
    const auto ready = CheckRevision(expectedRevision);
    if (!ready) return ready;
    const auto* info = GetWearableDefinition(definition);
    if (!info || (info->fiberCost <= 0 && info->furCost <= 0) || !ValidPoint(player))
        return Bad("Choose a craftable garment and valid location.");
    if (Count(Item::Knife) == 0) return Bad("Take your knife from storage to work fiber into clothing.");
    if (Count(Item::Fiber) < info->fiberCost)
        return Bad("Gather " + std::to_string(info->fiberCost - Count(Item::Fiber)) + " more Fiber first.");
    if (Count(Item::Fur) < info->furCost)
        return Bad("Gather " + std::to_string(info->furCost - Count(Item::Fur)) +
            " more Fur first. Look for deer remains in the woods.");
    if (!CanAllocate(state_.nextWearableId) || state_.wearables.size() >= MaxObjects)
        return Bad("The homestead has reached its garment identity limit.");
    if (auto rested = CheckExertion(Exertion::GarmentEnergy); !rested) return rested;
    State candidate = state_;
    candidate.inventory[static_cast<int>(Item::Fiber)] -= info->fiberCost;
    candidate.inventory[static_cast<int>(Item::Fur)] -= info->furCost;
    candidate.wearables.push_back({candidate.nextWearableId++, definition, 0, WearableOwner::Carried, 0});
    return Exert(Exertion::GarmentEnergy, CommitInventory(std::move(candidate),
        info->furCost > 0 ? "Made one garment from hide and fiber." : "Made one garment from fiber."));
}
Result Simulation::RecolorWearable(int id, int dye, Point player, std::uint64_t expectedRevision)
{
    const auto ready = CheckRevision(expectedRevision);
    if (!ready) return ready;
    const auto* original = GetWearable(id);
    if (!original || !GetWearableDefinition(original->definition)->dyeable || dye < 0 || dye > 3)
        return Bad("Choose Moss, Wine, Slate or Flax for a tunic or apron.");
    const auto access = ContainerAccess(state_, original->chestId, player);
    if (!access) return access;
    if (original->dye == dye) return Bad("This garment already has that color.");
    State candidate = state_;
    Find(candidate.wearables, id)->dye = dye;
    return CommitInventory(std::move(candidate), "Garment recolored. This is a cosmetic change only.");
}
Result Simulation::TransferGroup(int chestId, int groupId, int amount, bool toChest, Point player,
    std::uint64_t expectedRevision)
{
    const auto ready = CheckRevision(expectedRevision);
    if (!ready) return ready;
    if (chestId <= 0) return Bad("Choose a storage chest.");
    const auto access = ContainerAccess(state_, chestId, player);
    if (!access) return access;
    State candidate = state_;
    const int source = toChest ? 0 : chestId, destination = toChest ? chestId : 0;
    auto* layout = ContainerLayout(candidate, source);
    auto entry = std::find_if(layout->begin(), layout->end(),
        [&](const LayoutEntry& value) { return value.groupId == groupId && value.wearableId == 0; });
    if (entry == layout->end() || amount <= 0 || amount > entry->quantity)
        return Bad("Choose an available quantity from the selected group.");
    const int item = static_cast<int>(entry->item);
    entry->quantity -= amount;
    (*ContainerStock(candidate, source))[item] -= amount;
    (*ContainerStock(candidate, destination))[item] += amount;
    return CommitInventory(std::move(candidate), toChest ? "Selected quantity stored." : "Selected quantity taken.");
}
Result Simulation::SplitGroup(int containerId, int groupId, int amount, Point player, std::uint64_t expectedRevision)
{
    const auto ready = CheckRevision(expectedRevision);
    if (!ready) return ready;
    const auto access = ContainerAccess(state_, containerId, player);
    if (!access) return access;
    State candidate = state_;
    auto* layout = ContainerLayout(candidate, containerId);
    auto entry = std::find_if(layout->begin(), layout->end(),
        [&](const LayoutEntry& value) { return value.groupId == groupId && value.wearableId == 0; });
    if (entry == layout->end() || amount <= 0 || amount >= entry->quantity)
        return Bad("Split a positive quantity smaller than the selected group.");
    if (!CanAllocate(candidate.nextGroupId)) return Bad("Inventory group identities are exhausted.");
    const LayoutEntry split{candidate.nextGroupId++, entry->item, amount, 0};
    entry->quantity -= amount;
    layout->insert(entry + 1, split);
    return CommitInventory(std::move(candidate), "Group split; pack and chest capacity are unchanged.");
}
Result Simulation::MergeGroups(int containerId, int sourceGroupId, int targetGroupId, Point player,
    std::uint64_t expectedRevision)
{
    const auto ready = CheckRevision(expectedRevision);
    if (!ready) return ready;
    const auto access = ContainerAccess(state_, containerId, player);
    if (!access) return access;
    State candidate = state_;
    auto* layout = ContainerLayout(candidate, containerId);
    auto source = std::find_if(layout->begin(), layout->end(),
        [&](const LayoutEntry& value) { return value.groupId == sourceGroupId && value.wearableId == 0; });
    auto target = std::find_if(layout->begin(), layout->end(),
        [&](const LayoutEntry& value) { return value.groupId == targetGroupId && value.wearableId == 0; });
    if (source == layout->end() || target == layout->end() || source == target || source->item != target->item)
        return Bad("Choose two different groups of the same item in this container.");
    target->quantity += source->quantity;
    layout->erase(source);
    return CommitInventory(std::move(candidate), "Groups merged; capacity is unchanged.");
}
Result Simulation::ReorderEntry(int containerId, int index, int targetIndex, Point player, std::uint64_t expectedRevision)
{
    const auto ready = CheckRevision(expectedRevision);
    if (!ready) return ready;
    const auto access = ContainerAccess(state_, containerId, player);
    if (!access) return access;
    State candidate = state_;
    auto* layout = ContainerLayout(candidate, containerId);
    if (index < 0 || targetIndex < 0 || index >= static_cast<int>(layout->size()) ||
        targetIndex >= static_cast<int>(layout->size()) || index == targetIndex)
        return Bad("Choose two different positions in this container.");
    const LayoutEntry entry = (*layout)[index];
    layout->erase(layout->begin() + index);
    layout->insert(layout->begin() + targetIndex, entry);
    return CommitInventory(std::move(candidate), "Inventory order updated.");
}
Result Simulation::SplitHalf(int containerId, int groupId, Point player, std::uint64_t expectedRevision)
{
    const auto ready = CheckRevision(expectedRevision);
    if (!ready) return ready;
    const auto access = ContainerAccess(state_, containerId, player);
    if (!access) return access;
    const auto* layout = ContainerLayout(state_, containerId);
    if (!layout) return Bad("Choose an existing inventory container.");
    const auto entry = std::find_if(layout->begin(), layout->end(),
        [&](const LayoutEntry& value) { return value.groupId == groupId && value.wearableId == 0; });
    if (entry == layout->end() || entry->quantity < 2)
        return Bad("Choose an ordinary stack containing at least two items.");
    return SplitGroup(containerId, groupId, entry->quantity / 2, player, expectedRevision);
}
Result Simulation::SortPack(std::uint64_t expectedRevision)
{
    const auto ready = CheckRevision(expectedRevision);
    if (!ready) return ready;
    State candidate = state_;
    auto& layout = candidate.inventoryLayout;
    for (auto first = layout.begin(); first != layout.end(); ++first)
    {
        if (first->wearableId != 0) continue;
        for (auto duplicate = first + 1; duplicate != layout.end();)
        {
            if (duplicate->wearableId == 0 && duplicate->item == first->item)
            {
                first->quantity += duplicate->quantity;
                duplicate = layout.erase(duplicate);
            }
            else ++duplicate;
        }
    }
    const auto key = [&](const LayoutEntry& entry)
    {
        if (entry.wearableId == 0)
            return std::tuple<int, int, int, int, int>{
                InventoryCategory(entry.item), static_cast<int>(entry.item), 0, 0, entry.groupId};
        const auto* wearable = Find(candidate.wearables, entry.wearableId);
        return std::tuple<int, int, int, int, int>{4,
            wearable ? static_cast<int>(wearable->definition) : std::numeric_limits<int>::max(),
            wearable ? wearable->dye : 0, entry.wearableId, 0};
    };
    std::stable_sort(layout.begin(), layout.end(),
        [&](const LayoutEntry& left, const LayoutEntry& right) { return key(left) < key(right); });
    const auto sameLayout = [&]()
    {
        if (layout.size() != state_.inventoryLayout.size()) return false;
        for (std::size_t index = 0; index < layout.size(); ++index)
        {
            const auto& left = layout[index];
            const auto& right = state_.inventoryLayout[index];
            if (left.groupId != right.groupId || left.item != right.item
                || left.quantity != right.quantity || left.wearableId != right.wearableId)
                return false;
        }
        return true;
    };
    if (sameLayout())
        return {true, "Pack is already sorted.", ResultCode::None, revision_};
    return CommitInventory(std::move(candidate), "Pack sorted.");
}
Result Simulation::DropGroup(int groupId, int amount, Point position, Point player,
    std::uint64_t expectedRevision)
{
    const auto ready = CheckRevision(expectedRevision);
    if (!ready) return ready;
    if (!ValidPoint(position) || std::abs(position.x) > MaxWorldCoordinate
        || std::abs(position.y) > MaxWorldCoordinate || !Near(player, position, DropReach))
        return Bad("Choose safe ground close to you.");
    if (NearWater(position)) return Bad("Choose dry ground for this item.");
    for (const auto& structure : state_.structures)
        if (Near(position, Homestead::StructureCenter(state_, structure), 100.0))
            return Bad("Keep dropped items clear of structures.");
    for (const auto& plot : state_.plots)
        if (Near(position, PlotCenter(plot), 100.0))
            return Bad("Keep dropped items clear of crop plots.");
    State candidate = state_;
    auto entry = std::find_if(candidate.inventoryLayout.begin(), candidate.inventoryLayout.end(),
        [&](const LayoutEntry& value) { return value.groupId == groupId && value.wearableId == 0; });
    if (entry == candidate.inventoryLayout.end() || amount <= 0 || amount > entry->quantity)
        return Bad("Choose an available quantity from the selected carried stack.");
    WorldDrop* merge = nullptr;
    double nearest = DropMergeReach * DropMergeReach;
    for (auto& drop : candidate.worldDrops)
    {
        if (drop.wearableId != 0 || drop.item != entry->item
            || drop.quantity > InventoryCapacity - amount) continue;
        const double distance = DistanceSquared(position, drop.position);
        if (distance <= nearest && (!merge || distance < nearest || drop.id < merge->id))
        { merge = &drop; nearest = distance; }
    }
    if (!merge && candidate.worldDrops.size() >= MaxWorldDrops)
        return Bad("Too many possessions are already resting in the world. Pick one up first.");
    const Item item = entry->item;
    entry->quantity -= amount;
    candidate.inventory[static_cast<int>(item)] -= amount;
    if (merge) merge->quantity += amount;
    else
    {
        if (candidate.nextId >= TransientResourceIdBase - 1)
            return Bad("World drop identities are exhausted.");
        candidate.worldDrops.push_back({candidate.nextId++, position, item, amount, 0});
    }
    return CommitInventory(std::move(candidate), "Item dropped.");
}
Result Simulation::DropWearable(int wearableId, Point position, Point player,
    std::uint64_t expectedRevision)
{
    const auto ready = CheckRevision(expectedRevision);
    if (!ready) return ready;
    if (!ValidPoint(position) || std::abs(position.x) > MaxWorldCoordinate
        || std::abs(position.y) > MaxWorldCoordinate || !Near(player, position, DropReach))
        return Bad("Choose safe ground close to you.");
    if (NearWater(position)) return Bad("Choose dry ground for this garment.");
    for (const auto& structure : state_.structures)
        if (Near(position, Homestead::StructureCenter(state_, structure), 100.0))
            return Bad("Keep dropped garments clear of structures.");
    for (const auto& plot : state_.plots)
        if (Near(position, PlotCenter(plot), 100.0))
            return Bad("Keep dropped garments clear of crop plots.");
    const auto* original = GetWearable(wearableId);
    if (!original || original->owner != WearableOwner::Carried)
        return Bad("Unequip this garment into your pack before dropping it.");
    if (state_.worldDrops.size() >= MaxWorldDrops)
        return Bad("Too many possessions are already resting in the world. Pick one up first.");
    State candidate = state_;
    auto* wearable = Find(candidate.wearables, wearableId);
    wearable->owner = WearableOwner::World;
    wearable->chestId = 0;
    if (candidate.nextId >= TransientResourceIdBase - 1)
        return Bad("World drop identities are exhausted.");
    candidate.worldDrops.push_back({candidate.nextId++, position, Item::Count, 1, wearableId});
    return CommitInventory(std::move(candidate), "Garment dropped.");
}
Result Simulation::PickUpDrop(int dropId, Point player)
{
    if (state_.failed) return Failed();
    const auto found = std::find_if(state_.worldDrops.begin(), state_.worldDrops.end(),
        [dropId](const WorldDrop& drop) { return drop.id == dropId; });
    if (found == state_.worldDrops.end()) return Bad("That dropped possession is no longer available.");
    if (!Near(player, found->position, Reach)) return Bad("Move closer to pick this up.");
    State candidate = state_;
    auto drop = std::find_if(candidate.worldDrops.begin(), candidate.worldDrops.end(),
        [dropId](const WorldDrop& value) { return value.id == dropId; });
    if (drop->wearableId == 0)
    {
        if (TakesSpace(drop->item) && ContainerUsed(candidate, 0) > InventoryCapacity - drop->quantity)
            return {false, "Not enough pack space to pick up the complete stack.", ResultCode::Capacity, revision_};
        candidate.inventory[static_cast<int>(drop->item)] += drop->quantity;
    }
    else
    {
        if (ContainerUsed(candidate, 0) >= InventoryCapacity)
            return {false, "Not enough pack space to pick up this garment.", ResultCode::Capacity, revision_};
        auto* wearable = Find(candidate.wearables, drop->wearableId);
        if (!wearable || wearable->owner != WearableOwner::World)
            return Bad("This dropped garment has invalid ownership.");
        wearable->owner = WearableOwner::Carried;
    }
    candidate.worldDrops.erase(drop);
    return CommitInventory(std::move(candidate), "Dropped possession recovered.");
}
bool Simulation::IsNight() const
{
    const double hour = std::fmod(state_.hour, 24.0);
    return hour < 6.0 || hour >= 19.0;
}
bool IsRainDay(double hour) { return static_cast<long long>(std::floor(hour / 24.0)) % 3 == 1; }
bool IsRainingAt(double hour)
{
    const double ofDay = std::fmod(hour, 24.0);
    return IsRainDay(hour) && ofDay >= RainStartHour && ofDay < RainEndHour;
}
bool Simulation::IsRaining() const { return IsRainingAt(state_.hour); }
double RainAmount(double hour)
{
    if (!IsRainingAt(hour)) return 0.0;
    const double ofDay = std::fmod(hour, 24.0);
    const double day = std::floor(hour / 24.0);
    const auto ease = [](double t) { t = Clamp(t, 0.0, 1.0); return t * t * (3.0 - 2.0 * t); };
    const double envelope = ease((ofDay - RainStartHour) / 0.25) * ease((RainEndHour - ofDay) / (1.0 / 6.0));
    const double wave = 0.5 + 0.5 * std::sin((ofDay - RainStartHour) * 6.2831853 / 1.6 + day * 1.7)
        * (0.8 + 0.2 * std::sin((ofDay - RainStartHour) * 6.2831853 / 0.55 + day * 0.9));
    const double shower = ease((wave - 0.35) / 0.5);
    return envelope * (0.3 + 0.7 * shower);
}
double Overcast(double hour)
{
    // The rain window sits mid-morning, so the cloud's half-hour lead never crosses midnight.
    if (!IsRainDay(hour)) return 0.0;
    const double ofDay = std::fmod(hour, 24.0);
    const auto ease = [](double t) { t = Clamp(t, 0.0, 1.0); return t * t * (3.0 - 2.0 * t); };
    return ease((ofDay - (RainStartHour - OvercastLeadHours)) / OvercastLeadHours)
        * ease((RainEndHour + OvercastLeadHours - ofDay) / OvercastLeadHours);
}
int Simulation::DayNumber() const { return static_cast<int>(state_.hour / 24.0) + 1; }
const char* Simulation::SeasonName() const
{
    static const char* names[] = {"Spring", "Summer", "Autumn", "Winter"};
    return names[((DayNumber() - 1) / 14) % 4];
}
bool Simulation::IsSheltered(Point position) const
{
    if (!ValidPoint(position)) return false;
    using CellKey = std::pair<int, int>;
    std::set<int> candidates;
    for (const auto& piece : state_.structures)
        if (piece.kind == Piece::Foundation) candidates.insert(piece.buildingId);
    for (const int buildingId : candidates)
    {
        const Building* building = FindBuilding(state_, buildingId);
        if (!building) continue;
        const Point local = BuildingLocal(*building, position);
        const CellKey start{Cell(local.x), Cell(local.y)};
        std::set<CellKey> floors, roofs;
        std::set<Edge> edges;
        for (const auto& piece : state_.structures)
        {
            if (piece.buildingId != buildingId) continue;
            if (piece.kind == Piece::Foundation) floors.insert({piece.cellX, piece.cellY});
            if (piece.kind == Piece::Roof) roofs.insert({piece.cellX, piece.cellY});
            if (EdgePiece(piece.kind)) edges.insert(EdgeKey(piece.cellX, piece.cellY, piece.rotation));
        }
        if (!floors.count(start) || !roofs.count(start)) continue;
        std::set<CellKey> visited{start};
        std::queue<CellKey> pending;
        pending.push(start);
        static const int dx[] = {0, 1, 0, -1};
        static const int dy[] = {1, 0, -1, 0};
        bool enclosed = true;
        while (enclosed && !pending.empty())
        {
            const auto current = pending.front();
            pending.pop();
            for (int side = 0; side < 4; ++side)
            {
                if (edges.count(EdgeKey(current.first, current.second, side))) continue;
                const CellKey adjacent{current.first + dx[side], current.second + dy[side]};
                if (!floors.count(adjacent) || !roofs.count(adjacent)) { enclosed = false; break; }
                if (visited.insert(adjacent).second) pending.push(adjacent);
            }
        }
        if (enclosed) return true;
    }
    return false;
}
bool Simulation::IsNearFire(Point position) const
{
    for (const auto& piece : state_.structures)
        if (((piece.kind == Piece::Fire && piece.fuelHours > 0.0) || piece.kind == Piece::Hearth) &&
            Near(position, Homestead::StructureCenter(state_, piece), FireReach)) return true;
    return false;
}
bool Simulation::CanHarvest(int nodeId) const
{
    const auto* node = Find(state_.resources, nodeId);
    if (state_.failed || !node || node->cleared || node->readyAtHour > state_.hour || Retired(node->kind)) return false;
    // Overgrowth is cleared with its tool (ClearOvergrowth); only boughs and salvage piles come away by hand.
    if (const auto* overgrowth = FindOvergrowth(node->kind)) return overgrowth->byHand;
    return !RequiresHatchet(node->kind) || Count(Item::Hatchet) > 0;
}
int Simulation::FindNearestResource(Point position, double maxDistance) const
{
    if (!ValidPoint(position) || !FiniteRange(maxDistance, 0, 12000)) return -1;
    int nearest = -1;
    double distance = maxDistance * maxDistance;
    for (const auto& node : state_.resources)
    {
        const double current = DistanceSquared(position, node.position);
        if (CanHarvest(node.id) && current <= distance)
        {
            if (current == distance && nearest != -1) continue;
            nearest = node.id;
            distance = current;
        }
    }
    return nearest;
}
int Simulation::FindNearestDrop(Point position, double maxDistance) const
{
    if (!ValidPoint(position) || !FiniteRange(maxDistance, 0, 12000)) return -1;
    int nearest = -1;
    double distance = maxDistance * maxDistance;
    for (const auto& drop : state_.worldDrops)
    {
        const double current = DistanceSquared(position, drop.position);
        if (current < distance || (current == distance && (nearest == -1 || drop.id < nearest)))
        { nearest = drop.id; distance = current; }
    }
    return nearest;
}
int Simulation::FindNearestPlot(Point position, double maxDistance) const
{
    if (!ValidPoint(position) || !FiniteRange(maxDistance, 0, 12000)) return -1;
    int nearest = -1;
    double distance = maxDistance * maxDistance;
    for (const auto& plot : state_.plots)
    {
        const double current = DistanceSquared(position, PlotCenter(plot));
        if (current <= distance && !(current == distance && nearest != -1))
        { nearest = plot.id; distance = current; }
    }
    return nearest;
}
int Simulation::FindNearestStructure(Point position, Piece kind, double maxDistance) const
{
    if (!ValidPoint(position) || !ValidEnum(kind, Piece::Count) ||
        !FiniteRange(maxDistance, 0, 12000)) return -1;
    int nearest = -1;
    double distance = maxDistance * maxDistance;
    for (const auto& piece : state_.structures)
    {
        const double current = DistanceSquared(position, Homestead::StructureCenter(state_, piece));
        if (piece.kind == kind && current <= distance && !(current == distance && nearest != -1))
        { nearest = piece.id; distance = current; }
    }
    return nearest;
}
Result Simulation::Harvest(int nodeId, Point player)
{
    if (state_.failed) return Failed();
    auto* node = Find(state_.resources, nodeId);
    if (!node || node->cleared) return Bad("That resource is no longer available.");
    if (node->kind == ResourceKind::ForestTree) return Clear(nodeId, player);
    if (const auto* overgrowth = FindOvergrowth(node->kind))
    {
        if (overgrowth->byHand) return ClearOvergrowth(nodeId, Item::Count, player);
        return Bad(std::string("Use a ") + ToolName(overgrowth->tool) + " to clear this.");
    }
    if (!Near(player, node->position)) return Bad("Move closer to gather this resource.");
    if (Retired(node->kind)) return Bad("There's nothing here worth taking.");
    if (node->readyAtHour > state_.hour) return Bad("Nothing to gather here.");
    const double cost = HarvestCost(nodeId);
    if (auto ready = CheckExertion(cost); !ready) return ready;
    const Inventory yield = Yield(node->kind);
    State candidate = state_;
    auto* updated = Find(candidate.resources, nodeId);
    updated->readyAtHour = state_.hour + Regrowth(node->kind);
    if (!SaveResourceEdit(candidate, *updated)) return Bad("The world has reached its 16384 persistent resource edit limit.");
    for (int i = 0; i < ItemCount; ++i) candidate.inventory[i] += yield[i];
    const std::string message = std::string("Gathered ") + ResourceName(node->kind) + ".";
    return Exert(cost, CommitInventory(std::move(candidate), message.c_str()));
}
Result Simulation::Clear(int nodeId, Point player)
{
    if (state_.failed) return Failed();
    auto* node = Find(state_.resources, nodeId);
    if (!node || node->cleared) return Bad("This patch has already been cleared.");
    if (IsOvergrowth(node->kind)) return Harvest(nodeId, player);
    if (!Near(player, node->position)) return Bad("Move closer to clear this patch.");
    if (RequiresHatchet(node->kind) && Count(Item::Hatchet) == 0)
        return Bad("Craft an axe before felling trees.");
    const double cost = ClearCost(nodeId);
    if (auto ready = CheckExertion(cost); !ready) return ready;
    const Inventory yield = node->readyAtHour <= state_.hour ? Yield(node->kind) : Inventory{};
    State candidate = state_;
    auto* updated = Find(candidate.resources, nodeId);
    updated->cleared = true;
    updated->readyAtHour = 0.0;
    if (!SaveResourceEdit(candidate, *updated)) return Bad("The world has reached its 16384 persistent resource edit limit.");
    for (int i = 0; i < ItemCount; ++i) candidate.inventory[i] += yield[i];
    return Exert(cost, CommitInventory(std::move(candidate), "Land cleared."));
}
bool operator<(const UnderbrushEdit& a, const UnderbrushEdit& b)
{
    return a.chunk < b.chunk || (a.chunk == b.chunk && a.index < b.index);
}
bool Simulation::IsUnderbrushCleared(Generation::ChunkCoord chunk, int index) const
{
    const UnderbrushEdit key{chunk, index};
    return std::binary_search(state_.clearedUnderbrush.begin(), state_.clearedUnderbrush.end(), key);
}
Result Simulation::ClearUnderbrush(Generation::ChunkCoord chunk, int index, bool woody, Point plant, Point player)
{
    if (state_.failed) return Failed();
    if (index < 0 || index >= MaxUnderbrushIndex || !ValidPoint(plant)) return Bad("Choose a bush or bramble to clear.");
    if (Count(Item::Machete) == 0) return Bad("Take your machete from storage to hack through undergrowth.");
    if (!Near(player, plant, 450.0)) return Bad("Move closer to clear this undergrowth.");
    if (IsUnderbrushCleared(chunk, index)) return Bad("This undergrowth has already been cleared.");
    if (static_cast<int>(state_.clearedUnderbrush.size()) >= MaxUnderbrushEdits)
        return Bad("The world has reached its 16384 cleared-undergrowth limit.");
    const double cost = woody ? Exertion::WoodyUnderbrushEnergy : Exertion::SoftUnderbrushEnergy;
    if (auto ready = CheckExertion(cost); !ready) return ready;
    State candidate = state_;
    const UnderbrushEdit key{chunk, index};
    candidate.clearedUnderbrush.insert(
        std::lower_bound(candidate.clearedUnderbrush.begin(), candidate.clearedUnderbrush.end(), key), key);
    // A full pack still clears; the cuttings are just left on the ground.
    ++candidate.inventory[static_cast<int>(woody ? Item::Branch : Item::Fiber)];
    if (ContainerUsed(candidate, 0) > InventoryCapacity) --candidate.inventory[static_cast<int>(woody ? Item::Branch : Item::Fiber)];
    return Exert(cost, CommitInventory(std::move(candidate), "Undergrowth cleared."));
}
Result Simulation::Eat(Item item)
{
    const auto allowed = CanEat(state_, item);
    if (!allowed) return allowed;
    if (!TryAdjust(Items({{item, -1}}))) return Bad(std::string("Gather or cook some ") + ItemName(item) + " first.");
    return Good(ApplyMeal(state_, item));
}
Result Simulation::EatGroup(int groupId, std::uint64_t expectedRevision)
{
    const auto ready = CheckRevision(expectedRevision);
    if (!ready) return ready;
    State candidate = state_;
    auto entry = std::find_if(candidate.inventoryLayout.begin(), candidate.inventoryLayout.end(),
        [&](const LayoutEntry& value) { return value.groupId == groupId && value.wearableId == 0; });
    if (entry == candidate.inventoryLayout.end())
        return Bad("Choose a carried food group. Take stored food into your pack first.");
    const auto allowed = CanEat(candidate, entry->item);
    if (!allowed) return allowed;
    const Item item = entry->item;
    --entry->quantity;
    --candidate.inventory[static_cast<int>(item)];
    const std::string message = ApplyMeal(candidate, item);
    return CommitInventory(std::move(candidate), message.c_str());
}
Result Simulation::Craft(Recipe recipe, Point player)
{
    if (state_.failed) return Failed();
    if (!ValidEnum(recipe, Recipe::Count) || !ValidPoint(player)) return Bad("Choose a valid recipe and location.");
    const Inventory change = CraftChange(recipe);
    const bool cooking = recipe == Recipe::RoastedRoots || recipe == Recipe::HerbedRoots;
    if (cooking && !IsNearFire(player)) return Bad("Move beside a lit cookfire or the hearth to cook roots; no pot is needed.");
    if (recipe == Recipe::SplitFirewood && Count(Item::Hatchet) == 0)
        return Bad("Take your axe from storage to split firewood.");
    const Piece station = Crafting::StationFor(recipe);
    if (station != Piece::Count && FindNearestStructure(player, station, Crafting::StationReach) < 0)
        return Bad(Crafting::StationMissingMessage(station));
    const double cost = cooking ? Exertion::CookEnergy
        : recipe == Recipe::SplitFirewood ? Exertion::SplitFirewoodEnergy
        : station == Piece::Sawhorse ? Crafting::SawEnergy
        : station == Piece::Workbench ? Crafting::JoineryEnergy : Exertion::CraftEnergy;
    if (auto ready = CheckExertion(cost); !ready) return ready;
    if (!TryAdjust(change)) return Bad(MissingMessage(change, state_.inventory));
    if (Hafting(recipe))
    {
        const auto made = std::find_if(change.begin(), change.end(), [](int value) { return value > 0; });
        const Item tool = static_cast<Item>(made - change.begin());
        return Exert(cost, Good(std::string("Crafted a worn ") + ToolName(ToolForItem(tool)) + "."));
    }
    if (station != Piece::Count)
    {
        const auto made = std::find_if(change.begin(), change.end(), [](int value) { return value > 0; });
        const std::string output = CountedName(static_cast<Item>(made - change.begin()), *made);
        return Exert(cost, Good(station == Piece::Sawhorse ? "Sawed a length of timber into " + output + "."
            : "Made " + output + " at the workbench."));
    }
    return Exert(cost, Good(std::string("Made ") + RecipeName(recipe) + "."));
}

RecipeAssessment Simulation::AssessRecipe(Recipe recipe, Point player) const
{
    RecipeAssessment assessment;
    assessment.recipe = recipe;
    if (!ValidEnum(recipe, Recipe::Count) || !ValidPoint(player))
    {
        assessment.blocker = "Choose a valid recipe and location.";
        return assessment;
    }

    const Inventory change = CraftChange(recipe);
    for (int index = 0; index < ItemCount; ++index)
    {
        if (change[index] < 0)
        {
            RecipeIngredientAssessment ingredient;
            ingredient.item = static_cast<Item>(index);
            ingredient.have = state_.inventory[index];
            ingredient.need = -change[index];
            ingredient.source = AcquisitionSource(ingredient.item);
            ingredient.met = ingredient.have >= ingredient.need;
            assessment.ingredients.push_back(ingredient);
        }
        else if (change[index] > 0)
        {
            assessment.output = static_cast<Item>(index);
            assessment.outputCount = change[index];
        }
    }

    const bool cooking = recipe == Recipe::RoastedRoots
        || recipe == Recipe::HerbedRoots;
    const Piece station = Crafting::StationFor(recipe);
    assessment.stationRequired = cooking || station != Piece::Count;
    assessment.stationMet = cooking ? IsNearFire(player)
        : station == Piece::Count || FindNearestStructure(player, station, Crafting::StationReach) >= 0;
    assessment.stationLabel = cooking ? "Fueled cookfire nearby" : station != Piece::Count ? Crafting::StationLabel(station) : "";
    if (recipe == Recipe::SplitFirewood)
        assessment.retainedTool = Item::Hatchet;
    assessment.retainedToolMet = assessment.retainedTool == Item::Count
        || Count(assessment.retainedTool) > 0;

    Simulation probe = *this;
    const Result result = probe.Craft(recipe, player);
    assessment.craftable = result.ok;
    assessment.blocker = result.ok ? std::string() : result.message;
    assessment.capacityMet = result.ok
        || assessment.blocker != "Not enough pack space. Store some items in a chest first.";
    return assessment;
}
namespace
{
constexpr int MaxBuildingCells = 200;
constexpr double SnapReach = 260.0;
constexpr double BuildReach = 700.0;
// The frame the target builds in, or null when the target names no valid site.
const Building* SiteBuilding(const State& state, const PlacementTarget& target)
{
    if (!ValidEnum(target.kind, Piece::Count)) return nullptr;
    if (target.buildingId == 0) return ValidCell(target.cellX, target.cellY) ? FindBuilding(state, 0) : nullptr;
    const Building* building = target.buildingId > 0 ? FindBuilding(state, target.buildingId) : &target.frame;
    if (!building || std::abs(target.cellX) > MaxBuildingCells || std::abs(target.cellY) > MaxBuildingCells) return nullptr;
    if (target.buildingId < 0 && (target.cellX != 0 || target.cellY != 0 || !ValidPoint(target.frame.origin)
        || !FiniteRange(target.frame.yaw, 0.0, 360.0))) return nullptr;
    const Point center = BuildingCellCenter(*building, target.cellX, target.cellY);
    if (!ValidPoint(center) || std::abs(center.x) > MaxWorldCoordinate - CellSize
        || std::abs(center.y) > MaxWorldCoordinate - CellSize) return nullptr;
    return building;
}
}
PlacementTarget Simulation::ResolvePlacement(Piece kind, Point aim, double freeYaw, int rotation) const
{
    PlacementTarget target;
    target.kind = kind;
    target.rotation = ((rotation % 4) + 4) % 4;
    if (!ValidEnum(kind, Piece::Count) || !ValidPoint(aim) || !std::isfinite(freeYaw))
    {
        target.blocker = "Choose a valid structure and building site.";
        return target;
    }
    if (Crafting::IsFence(kind))
        return Crafting::ResolveFence(state_, kind, aim, freeYaw,
            [this](const PlacementTarget& candidate) { return static_cast<bool>(CheckSite(candidate, true)); });
    std::set<std::tuple<int, int, int>> floors;
    for (const auto& piece : state_.structures)
        if (piece.kind == Piece::Foundation) floors.insert({piece.buildingId, piece.cellX, piece.cellY});
    std::vector<std::pair<double, PlacementTarget>> snaps;
    for (const auto& floor : floors)
    {
        const int buildingId = std::get<0>(floor), x = std::get<1>(floor), y = std::get<2>(floor);
        const Building* building = FindBuilding(state_, buildingId);
        if (!building) continue;
        const Point local = BuildingLocal(*building, aim);
        const bool insideFloor = Cell(local.x) == x && Cell(local.y) == y;
        const auto consider = [&](int cellX, int cellY)
        {
            const double distance = std::sqrt(DistanceSquared(local, CellCenter(cellX, cellY)));
            if (distance >= SnapReach && !insideFloor) return;
            PlacementTarget snap = target;
            snap.buildingId = buildingId;
            snap.cellX = cellX;
            snap.cellY = cellY;
            snap.frame = *building;
            snap.snapped = true;
            if (Crafting::IsMovable(kind))
                snap.spot = Crafting::MovableSpot(*building, kind, cellX, cellY, snap.rotation, aim);
            snaps.emplace_back(distance, std::move(snap));
        };
        if (kind == Piece::Foundation)
        {
            static const int dx[] = {0, 1, 0, -1};
            static const int dy[] = {1, 0, -1, 0};
            for (int side = 0; side < 4; ++side)
                if (!floors.count({buildingId, x + dx[side], y + dy[side]})) consider(x + dx[side], y + dy[side]);
        }
        else if (EdgePiece(kind))
        {
            // Walls and doorways take the floor edge nearest the aim, turned to lie along it, so
            // they never need rotating by hand. The floor under the aim wins a shared edge.
            static const double ex[] = {0.0, 0.5, 0.0, -0.5};
            static const double ey[] = {0.5, 0.0, -0.5, 0.0};
            const Point center = CellCenter(x, y);
            for (int side = 0; side < 4; ++side)
            {
                const Point edge{center.x + ex[side] * CellSize, center.y + ey[side] * CellSize};
                const double distance = std::sqrt(DistanceSquared(local, edge)) + (insideFloor ? 0.0 : 1.0);
                if (distance >= SnapReach && !insideFloor) continue;
                PlacementTarget snap = target;
                snap.buildingId = buildingId;
                snap.cellX = x;
                snap.cellY = y;
                snap.rotation = side;
                snap.frame = *building;
                snap.snapped = true;
                snaps.emplace_back(distance, std::move(snap));
            }
        }
        // Furniture joins a floor only when aimed inside it; walls and roofs take the nearest floor.
        else if (!Furniture(kind) || insideFloor) consider(x, y);
    }
    if (!snaps.empty())
    {
        // Prefer the nearest snap that can actually be built, so a taken side yields to a free one.
        std::stable_sort(snaps.begin(), snaps.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });
        for (const auto& snap : snaps)
            if (CheckSite(snap.second, true)) return snap.second;
        return snaps.front().second;
    }

    double yaw = std::fmod(freeYaw, 360.0);
    if (yaw < 0.0) yaw += 360.0;
    if (yaw >= 360.0) yaw = 0.0;
    if (EdgePiece(kind) || kind == Piece::Roof) target.blocker = "Aim at one of your foundations to build this.";
    target.rotation = 0;
    const double quarters = std::round(yaw / 90.0);
    const int gridX = Cell(aim.x), gridY = Cell(aim.y);
    if (std::abs(yaw - quarters * 90.0) < 1e-6 && DistanceSquared(aim, CellCenter(gridX, gridY)) <= 5.0 * 5.0)
    {
        // Squared up within 5 cm of a world grid cell: take exactly that cell on building 0.
        target.buildingId = 0;
        target.cellX = gridX;
        target.cellY = gridY;
        target.rotation = (4 - static_cast<int>(quarters) % 4) % 4;
        return target;
    }
    const Point corner = RotateYaw(CellCenter(0, 0), yaw);
    target.buildingId = -1;
    target.cellX = 0;
    target.cellY = 0;
    target.frame = {0, {aim.x - corner.x, aim.y - corner.y}, yaw};
    return target;
}
Result Simulation::CheckPlacement(const PlacementTarget& target, Point player, bool quick) const
{
    if (state_.failed) return Failed();
    if (!target.blocker.empty()) return Bad(target.blocker);
    const Building* building = SiteBuilding(state_, target);
    if (!building) return Bad("Choose a valid structure and building cell.");
    if (!Near(player, BuildingCellCenter(*building, target.cellX, target.cellY), BuildReach))
        return Bad("Move closer to this building site.");
    return CheckSite(target, quick);
}
Result Simulation::CheckSite(const PlacementTarget& target, bool quick) const
{
    if (!target.blocker.empty()) return Bad(target.blocker);
    const Building* building = SiteBuilding(state_, target);
    if (!building) return Bad("Choose a valid structure and building cell.");
    const Piece kind = target.kind;
    const int cellX = target.cellX, cellY = target.cellY;
    const int rotation = ((target.rotation % 4) + 4) % 4;
    const bool existing = target.buildingId >= 0;
    if (state_.structures.size() >= MaxObjects || state_.nextId >= TransientResourceIdBase - 2
        || (!existing && state_.buildings.size() >= MaxObjects))
        return Bad("The homestead has reached its structure limit.");
    if (auto owned = CanBuildAt(target); !owned) return owned;
    if (!IsBuildable(kind)) return Bad("That belongs to the old house and can't be built.");
    const bool onFoundation = existing && HasPiece(state_, Piece::Foundation, target.buildingId, cellX, cellY);
    const bool movable = Crafting::IsMovable(kind);
    const Footprint ground = ResourceFootprint(*building, kind, cellX, cellY, rotation, onFoundation, target.spot);
    if (Manor::BlockedByManor(state_, Layout(), target, ground)) return Bad(Manor::FootprintBlocked);
    const auto space = CheckFootprintResources(state_, ground, quick);
    if (!space) return space;
    if (const ResourceNode* spoiler = OvergrowthSpoiling(state_, ground)) return Bad(SpoiledGroundMessage(*spoiler));
    for (const auto& plot : state_.plots)
        if (FootprintsOverlap(Inset(ground), Inset(GardenFootprint(plot))))
            return Bad("Keep this crop plot clear of buildings.");
    if (existing)
        for (const auto& piece : state_.structures)
        {
            if (piece.buildingId != target.buildingId) continue;
            if (EdgePiece(kind) && EdgePiece(piece.kind) &&
                EdgeKey(cellX, cellY, rotation) == EdgeKey(piece.cellX, piece.cellY, piece.rotation))
                return Bad("There is already a wall or doorway along that edge.");
            if (piece.cellX != cellX || piece.cellY != cellY) continue;
            // Small furniture shares a floor cell with anything it doesn't overlap (checked below).
            if (movable || (Crafting::IsMovable(piece.kind) && Furniture(kind))) continue;
            if ((!EdgePiece(kind) && piece.kind == kind) || (Furniture(kind) && Furniture(piece.kind)))
                return Bad("That building space is already occupied.");
        }
    if ((kind == Piece::Roof || EdgePiece(kind)) && !onFoundation)
        return Bad("Build a foundation in this cell first.");
    if (existing && (movable || Furniture(kind)))
    {
        const Footprint mine = Inset(PieceFootprint(*building, kind, cellX, cellY, rotation, onFoundation, target.spot));
        for (const auto& piece : state_.structures)
        {
            if (piece.buildingId != target.buildingId || !(Furniture(piece.kind) || (movable && EdgePiece(piece.kind))))
                continue;
            if ((movable || Crafting::IsMovable(piece.kind)) && FootprintsOverlap(mine, Inset(StructureFootprint(state_, piece))))
                return Bad(EdgePiece(piece.kind) ? "That would stand in the wall. Aim further into the room."
                    : "Something already stands there. Aim at a clear part of the floor.");
        }
    }
    if (!EdgePiece(kind))
    {
        const Footprint mine = Inset(PieceFootprint(*building, kind, cellX, cellY, rotation, onFoundation, target.spot));
        for (const auto& piece : state_.structures)
        {
            if ((existing && piece.buildingId == target.buildingId) || EdgePiece(piece.kind)) continue;
            if (FootprintsOverlap(mine, Inset(StructureFootprint(state_, piece))))
                return Bad(Crafting::IsFence(kind) && Crafting::IsFence(piece.kind)
                    ? "That runs into another fence. Turn it, or aim at a free post."
                    : "That overlaps another building. Move it clear, or aim at a foundation to snap on.");
        }
    }
    return Good("");
}
Result Simulation::Place(const PlacementTarget& target, Point player)
{
    const auto ready = CheckPlacement(target, player);
    if (!ready) return ready;
    const Piece kind = target.kind;
    const Inventory cost = BuildCost(kind);
    if (auto rested = CheckExertion(Exertion::BuildEnergy); !rested) return rested;
    if (!TryAdjust(cost)) return Bad(MissingMessage(cost, state_.inventory));
    int buildingId = target.buildingId;
    if (buildingId < 0)
    {
        buildingId = state_.nextId++;
        state_.buildings.push_back({buildingId, target.frame.origin, target.frame.yaw});
    }
    Structure piece{state_.nextId++, kind, target.cellX, target.cellY, ((target.rotation % 4) + 4) % 4, 0.0, {}};
    piece.buildingId = buildingId;
    if (Crafting::IsMovable(kind) && target.buildingId >= 0) piece.spot = target.spot;
    state_.structures.push_back(std::move(piece));
    return Exert(Exertion::BuildEnergy, Good(std::string("Placed ") + PieceName(kind) + "."));
}
Result Simulation::CheckBuildCost(Piece kind) const
{
    if (!ValidEnum(kind, Piece::Count)) return Bad("Choose a valid structure.");
    const Inventory cost = BuildCost(kind);
    for (int i = 0; i < ItemCount; ++i)
        if (state_.inventory[i] + cost[i] < 0) return Bad(MissingMessage(cost, state_.inventory));
    return Good("");
}
Result Simulation::Place(Piece kind, int cellX, int cellY, int rotation, Point player)
{
    PlacementTarget target;
    target.kind = kind;
    target.buildingId = 0;
    target.cellX = cellX;
    target.cellY = cellY;
    target.rotation = rotation;
    return Place(target, player);
}
namespace
{
constexpr double DeconstructReach = 450.0;
double FootprintDistance(const Footprint& box, Point p)
{
    const Point local = RotateYaw({p.x - box.center.x, p.y - box.center.y}, -box.yaw);
    const double dx = std::max(0.0, std::abs(local.x) - box.half.x);
    const double dy = std::max(0.0, std::abs(local.y) - box.half.y);
    return std::sqrt(dx * dx + dy * dy);
}
int DeconstructPriority(Piece kind)
{
    return Furniture(kind) ? 0 : EdgePiece(kind) ? 1 : kind == Piece::Roof ? 2 : 3;
}
double AimTolerance(Piece kind) { return EdgePiece(kind) ? 45.0 : Furniture(kind) ? 30.0 : 0.0; }
Point ClearDropSpot(const State& state, Point player)
{
    const auto clear = [&](Point p)
    {
        if (!ValidPoint(p) || IsNearWater(p)) return false;
        for (const auto& piece : state.structures)
            if (FootprintDistance(StructureFootprint(state, piece), p) < 40.0) return false;
        for (const auto& plot : state.plots)
            if (Near(p, PlotCenter(plot), 100.0)) return false;
        return true;
    };
    if (clear(player)) return player;
    for (double radius : {70.0, 140.0, 200.0})
        for (int step = 0; step < 12; ++step)
        {
            const double angle = step * 3.14159265358979323846 / 6.0;
            const Point p{player.x + std::cos(angle) * radius, player.y + std::sin(angle) * radius};
            if (clear(p)) return p;
        }
    return player;
}
}
int Simulation::FindDeconstructTarget(Point aim, double maxDistance) const
{
    if (!ValidPoint(aim)) return -1;
    int best = -1, bestPriority = 0;
    double bestScore = 0.0;
    for (const auto& piece : state_.structures)
    {
        const double distance = FootprintDistance(StructureFootprint(state_, piece), aim);
        if (distance > maxDistance) continue;
        const double score = std::max(0.0, distance - AimTolerance(piece.kind));
        const int priority = DeconstructPriority(piece.kind);
        if (best < 0 || score < bestScore - 1e-6 || (score <= bestScore + 1e-6 && priority < bestPriority))
        {
            best = piece.id;
            bestScore = score;
            bestPriority = priority;
        }
    }
    return best;
}
Result Simulation::CheckDeconstruct(int structureId, Point player) const
{
    if (state_.failed) return Failed();
    const auto* piece = Find(state_.structures, structureId);
    if (!piece) return Bad("Aim at something you built to take it down.");
    if (piece->heritage) return Bad("This is part of the old house; it can't be taken down.");
    if (!ValidPoint(player) || FootprintDistance(StructureFootprint(state_, *piece), player) > DeconstructReach)
        return Bad("Move closer to take this down.");
    if (piece->kind == Piece::Foundation)
        for (const auto& other : state_.structures)
            if (other.id != piece->id && other.buildingId == piece->buildingId
                && other.cellX == piece->cellX && other.cellY == piece->cellY)
                return Bad("Take down the walls, roof and furnishings on this floor first.");
    return CheckExertion(Exertion::DeconstructEnergy);
}
Result Simulation::Deconstruct(int structureId, Point player)
{
    const auto ready = CheckDeconstruct(structureId, player);
    if (!ready) return ready;
    State candidate = state_;
    const auto found = std::find_if(candidate.structures.begin(), candidate.structures.end(),
        [structureId](const Structure& value) { return value.id == structureId; });
    const Structure piece = *found;
    candidate.structures.erase(found);
    if (piece.buildingId > 0 && std::none_of(candidate.structures.begin(), candidate.structures.end(),
        [&](const Structure& value) { return value.buildingId == piece.buildingId; }))
        candidate.buildings.erase(std::remove_if(candidate.buildings.begin(), candidate.buildings.end(),
            [&](const Building& value) { return value.id == piece.buildingId; }), candidate.buildings.end());

    const Inventory cost = BuildCost(piece.kind);
    const bool chest = piece.kind == Piece::Chest;
    const Point spot = ClearDropSpot(candidate, player);
    const Result crowded = Bad("Too many possessions are already resting in the world. Pick some up before taking this down.");
    int setDown = 0;
    const auto dropItems = [&](Item item, int quantity)
    {
        while (quantity > 0)
        {
            WorldDrop* merge = nullptr;
            for (auto& drop : candidate.worldDrops)
                if (drop.wearableId == 0 && drop.item == item && drop.quantity < InventoryCapacity
                    && Near(spot, drop.position, DropMergeReach)) { merge = &drop; break; }
            int moved = 0;
            if (merge)
            {
                moved = std::min(quantity, InventoryCapacity - merge->quantity);
                merge->quantity += moved;
            }
            else
            {
                if (candidate.worldDrops.size() >= MaxWorldDrops || candidate.nextId >= TransientResourceIdBase - 1)
                    return false;
                moved = std::min(quantity, InventoryCapacity);
                candidate.worldDrops.push_back({candidate.nextId++, spot, item, moved, 0});
            }
            quantity -= moved;
            setDown += moved;
        }
        return true;
    };
    for (const bool contents : {false, true})
        for (int i = 0; i < ItemCount; ++i)
        {
            const int back = contents ? (chest ? piece.storage[i] : 0) : -cost[i];
            if (back <= 0) continue;
            const Item item = static_cast<Item>(i);
            const int kept = TakesSpace(item) ? std::min(back, std::max(0, InventoryCapacity - ContainerUsed(candidate, 0)))
                : std::min(back, InventoryCapacity - candidate.inventory[i]);
            candidate.inventory[i] += kept;
            if (!dropItems(item, back - kept)) return crowded;
        }
    for (auto& wearable : candidate.wearables)
    {
        if (wearable.owner != WearableOwner::Chest || wearable.chestId != piece.id) continue;
        wearable.chestId = 0;
        if (ContainerUsed(candidate, 0) < InventoryCapacity)
        {
            wearable.owner = WearableOwner::Carried;
            continue;
        }
        if (candidate.worldDrops.size() >= MaxWorldDrops || candidate.nextId >= TransientResourceIdBase - 1)
            return crowded;
        wearable.owner = WearableOwner::World;
        candidate.worldDrops.push_back({candidate.nextId++, spot, Item::Count, 1, wearable.id});
        ++setDown;
    }
    std::string message = std::string("Took down the ") + PieceName(piece.kind) + ": " + DescribeCost(cost) + " back";
    if (chest) message += ", and its contents";
    message += ".";
    if (setDown > 0)
        message += " Your pack is full, so " + std::to_string(setDown) + (setDown == 1 ? " thing is" : " things are")
            + " set down beside you.";
    return Exert(Exertion::DeconstructEnergy, CommitInventory(std::move(candidate), message.c_str()));
}
Result Simulation::GrantStarterKit(Point anchor, Point facing, bool includeSeeds)
{
    if (state_.failed) return Failed();
    if (!ValidPoint(anchor)) return Bad("Choose a valid starter-kit location.");
    const auto owned = [this](Item item)
    {
        if (Count(item) > 0) return true;
        for (const auto& piece : state_.structures)
            if (piece.kind == Piece::Chest && piece.storage[static_cast<int>(item)] > 0) return true;
        return false;
    };
    Inventory change{};
    for (Item tool : {Item::Hatchet, Item::DiggingStick, Item::WateringCan, Item::Scythe, Item::Billhook, Item::Pickaxe})
        if (!owned(tool)) change[static_cast<int>(tool)] = 1;
    if (includeSeeds)
    {
        change[static_cast<int>(Item::Seeds)] = 8;
        change[static_cast<int>(Item::Berries)] = 4;
    }
    if (!TryAdjust(change)) return Bad("The pack has no room for the starter tools.");

    int beds = 0, chests = 0;
    for (const auto& piece : state_.structures)
    {
        beds += piece.kind == Piece::Bed;
        chests += piece.kind == Piece::Chest;
    }
    std::vector<Piece> wanted;
    // The estate's standing room already furnishes her.
    if (Manor::HeritageBuildingId(state_) == 0)
    {
        if (beds == 0) wanted.push_back(Piece::Bed);
        for (int i = chests; i < 2; ++i) wanted.push_back(Piece::Chest);
    }
    // Clear cells nearest a spot ahead and to the side of her, never the cell she stands in.
    const int homeX = Cell(anchor.x), homeY = Cell(anchor.y);
    const double length = std::sqrt(facing.x * facing.x + facing.y * facing.y);
    const Point forward = length > 1e-6 ? Point{facing.x / length, facing.y / length} : Point{1.0, 0.0};
    // Unreal is left-handed: facing +X, +Y is to her right.
    const Point target{anchor.x + forward.x * 350.0 - forward.y * 350.0, anchor.y + forward.y * 350.0 + forward.x * 350.0};
    std::vector<std::pair<double, std::pair<int, int>>> cells;
    for (int dy = -3; dy <= 3; ++dy)
        for (int dx = -3; dx <= 3; ++dx)
        {
            const int x = homeX + dx, y = homeY + dy;
            if ((dx == 0 && dy == 0) || !ValidCell(x, y)) continue;
            cells.push_back({DistanceSquared(CellCenter(x, y), target), {x, y}});
        }
    std::sort(cells.begin(), cells.end());
    const auto used = [this](int x, int y)
    {
        for (const auto& piece : state_.structures)
            if (piece.buildingId == 0 && piece.cellX == x && piece.cellY == y) return true;
        for (const auto& plot : state_.plots) if (PlotInCell(plot, x, y)) return true;
        return false;
    };
    int placed = 0;
    for (const auto& cell : cells)
    {
        if (placed == static_cast<int>(wanted.size())) break;
        const int x = cell.second.first, y = cell.second.second;
        PlacementTarget site;
        site.kind = wanted[placed];
        site.cellX = x;
        site.cellY = y;
        site.rotation = 2;
        if (used(x, y) || !CheckSite(site, false)) continue;
        if (state_.structures.size() >= MaxObjects || state_.nextId >= TransientResourceIdBase - 1) break;
        state_.structures.push_back({state_.nextId++, wanted[placed], x, y, 2, 0.0, {}});
        ++placed;
    }
    ++revision_;
    return Good("Your starter tools, bed and storage chests are ready.");
}
Result Simulation::SeedStandingRoomAt(Point origin, double yaw)
{
    if (state_.failed) return Failed();
    if (!ValidPoint(origin) || !std::isfinite(yaw)) return Bad("Choose a valid place for the standing room.");
    if (Manor::HeritageBuildingId(state_) != 0) return Bad("The standing room already stands.");
    EstateLayout layout;
    layout.landmarks.push_back({Anchor::StandingRoomOrigin, origin, 0.0, yaw});
    State candidate = state_;
    const std::size_t first = candidate.structures.size();
    if (!Manor::SeedStandingRoom(candidate, layout)) return Bad("The standing room could not be laid out.");
    // A playtest aid: fell any woodland trees and saplings standing where the room goes.
    if (!candidate.fixedEstate)
    {
        constexpr double Clearance = 600.0;
        Generation::ChunkCoord low, high;
        if (Generation::ChunkAt(static_cast<std::int64_t>(origin.x - Clearance), static_cast<std::int64_t>(origin.y - Clearance), low)
                == Generation::Status::Ok
            && Generation::ChunkAt(static_cast<std::int64_t>(origin.x + Clearance), static_cast<std::int64_t>(origin.y + Clearance), high)
                == Generation::Status::Ok)
            for (int cy = low.y; cy <= high.y; ++cy)
                for (int cx = low.x; cx <= high.x; ++cx)
                {
                    Generation::ChunkBaseline baseline;
                    if (Generation::GenerateChunk(candidate.world, {cx, cy}, baseline) != Generation::Status::Ok) continue;
                    for (const auto& entity : baseline.entities)
                    {
                        ResourceNode node;
                        if ((entity.kind != Generation::EntityKind::ForestTree && entity.kind != Generation::EntityKind::Sapling)
                            || !GeneratedNode(candidate, entity, node) || node.cleared
                            || !Near(node.position, origin, Clearance)) continue;
                        node.cleared = true;
                        node.readyAtHour = 0.0;
                        if (!SaveResourceEdit(candidate, node)) return Bad("The woodland has too many edits to clear the room.");
                        for (auto& live : candidate.resources)
                            if (live.key == node.key) { live.cleared = true; live.readyAtHour = 0.0; }
                    }
                }
    }
    for (std::size_t i = first; i < candidate.structures.size(); ++i)
    {
        const Structure& piece = candidate.structures[i];
        const Building& frame = *FindBuilding(candidate, piece.buildingId);
        const bool floor = HasPiece(candidate, Piece::Foundation, piece.buildingId, piece.cellX, piece.cellY);
        const Footprint ground = ResourceFootprint(frame, piece.kind, piece.cellX, piece.cellY, piece.rotation, floor);
        if (!CheckFootprintResources(candidate, ground, false))
            return Bad("Trees stand where the room would go. Try a clearer spot.");
        for (const auto& plot : candidate.plots)
            if (FootprintsOverlap(Inset(ground), Inset(GardenFootprint(plot))))
                return Bad("A crop plot is in the way of the room.");
        for (std::size_t j = 0; j < first; ++j)
            if (!EdgePiece(piece.kind) && !EdgePiece(candidate.structures[j].kind)
                && FootprintsOverlap(Inset(ground), Inset(StructureFootprint(candidate, candidate.structures[j]))))
                return Bad("Another building is in the way of the room.");
    }
    if (candidate.heroineName.empty())
    {
        candidate.heroineName = Manor::DefaultHeroineName;
        candidate.familyName = Manor::DefaultFamilyName;
        candidate.estateName = Manor::DefaultEstateName;
    }
    if (std::find(candidate.journal.begin(), candidate.journal.end(), Manor::ArrivalEntry) == candidate.journal.end())
        candidate.journal.push_back(Manor::ArrivalEntry);
    const auto inventory = ValidateInventory(candidate);
    if (!inventory) return inventory;
    state_ = std::move(candidate);
    return {true, "The standing room is ready.", ResultCode::None, ++revision_};
}
Result Simulation::GrantItems(Item item, int count)
{
    if (state_.failed) return Failed();
    if (item == Item::Count || count <= 0) return Bad("Choose an item and a positive amount.");
    Inventory change{};
    change[static_cast<int>(item)] = count;
    if (!TryAdjust(change)) return Bad("Not enough pack space.");
    ++revision_;
    return Good(std::string("Added ") + std::to_string(count) + " " + ItemName(item) + ".");
}
Result Simulation::Till(int cellX, int cellY, Point player)
{
    if (state_.failed) return Failed();
    const int buildingX = GardenToCell(cellX), buildingY = GardenToCell(cellY);
    if (!ValidCell(buildingX, buildingY) || !Near(player, GardenCellCenter(cellX, cellY)))
        return Bad("Move closer to a valid garden square.");
    if (Count(Item::DiggingStick) == 0) return Bad("Craft a hoe before tilling soil.");
    if (state_.plots.size() >= MaxObjects || state_.nextId >= TransientResourceIdBase - 1)
        return Bad("The garden has reached its plot limit.");
    const auto space = CheckGardenResources(state_, cellX, cellY);
    if (!space) return space;
    const Footprint square{GardenCellCenter(cellX, cellY), {GardenCellSize * 0.5, GardenCellSize * 0.5}, 0.0};
    if (const ResourceNode* spoiler = OvergrowthSpoiling(state_, square)) return Bad(SpoiledGroundMessage(*spoiler));
    for (const auto& structure : state_.structures)
        if (FootprintsOverlap(Inset(square), Inset(StructureFootprint(state_, structure))))
            return Bad("Choose soil away from buildings.");
    for (const auto& plot : state_.plots)
        if (plot.cellX == cellX && plot.cellY == cellY) return Bad("This cell is already tilled.");
    if (auto ready = CheckExertion(Exertion::TillEnergy); !ready) return ready;
    state_.plots.push_back({state_.nextId++, cellX, cellY, false, 0.0, 0.35, 0.0});
    return Exert(Exertion::TillEnergy, Good("Soil tilled. Choose seeds or a berry on your hotbar to plant here."));
}
Result Simulation::Plant(int plotId, Point player, CropKind kind)
{
    if (state_.failed) return Failed();
    if (!ValidEnum(kind, CropKind::Count)) return Bad("Choose seeds to plant.");
    auto* plot = Find(state_.plots, plotId);
    if (!plot || !Near(player, PlotCenter(*plot))) return Bad("Move beside a tilled plot to plant.");
    if (plot->planted) return Bad("A crop is already growing here.");
    const auto& crop = GetCropInfo(kind);
    if (auto ready = CheckExertion(Exertion::PlantEnergy); !ready) return ready;
    if (!TryAdjust(Items({{crop.seed, -1}})))
        return Bad(kind == CropKind::Berries ? "Gather a berry to plant the seeds from its fruit."
            : kind == CropKind::Roots ? "Gather seeds from wild roots before planting."
            : std::string("You have no ") + ItemName(crop.seed) + " to sow.");
    plot->kind = kind;
    plot->planted = true;
    plot->picked = false;
    plot->growth = 0.0;
    return Exert(Exertion::PlantEnergy, Good(std::string("Planted ") + crop.lower + ". " + ReadyInText(kind)));
}
Result Simulation::Water(int plotId, Point player)
{
    if (state_.failed) return Failed();
    auto* plot = Find(state_.plots, plotId);
    if (!plot || !Near(player, PlotCenter(*plot))) return Bad("Move beside a garden plot to water it.");
    if (Count(Item::WateringCan) == 0) return Bad("Carry your pail to water crops.");
    if (plot->moisture >= 1.0) return Bad("This soil is already fully watered.");
    if (Count(Item::Water) <= 0) return Bad(EmptyPailText);
    if (auto ready = CheckExertion(Exertion::WaterEnergy); !ready) return ready;
    if (!TryAdjust(Items({{Item::Water, -1}}))) return Bad(EmptyPailText);
    plot->moisture = 1.0;
    return Exert(Exertion::WaterEnergy, Good("Soil watered."));
}
Result Simulation::Weed(int plotId, Point player)
{
    if (state_.failed) return Failed();
    auto* plot = Find(state_.plots, plotId);
    if (!plot || !Near(player, PlotCenter(*plot))) return Bad("Move beside a garden plot to weed it.");
    if (plot->weeds <= 0.0) return Bad("This plot is already free of weeds.");
    if (auto ready = CheckExertion(Exertion::WeedEnergy); !ready) return ready;
    plot->weeds = 0.0;
    return Exert(Exertion::WeedEnergy, Good("Weeds removed. The crop has more room to grow."));
}
Result Simulation::HarvestCrop(int plotId, Point player)
{
    if (state_.failed) return Failed();
    auto* plot = Find(state_.plots, plotId);
    if (!plot || !Near(player, PlotCenter(*plot))) return Bad("Move beside your crop to harvest.");
    if (!plot->planted || plot->growth < 1.0)
        return Bad(plot->planted ? PlotStatus(*plot) + "." : std::string("Nothing is growing here yet."));
    const auto& crop = GetCropInfo(plot->kind);
    Inventory yield{};
    yield[static_cast<int>(crop.produce)] += crop.produceCount;
    if (crop.bonus != Item::Count && crop.bonusCount > 0) yield[static_cast<int>(crop.bonus)] += crop.bonusCount;
    if (auto ready = CheckExertion(Exertion::HarvestCropEnergy); !ready) return ready;
    if (!TryAdjust(yield)) return Bad(MissingMessage(yield, state_.inventory));
    const bool regrows = crop.regrowHours > 0.0;
    plot->planted = regrows;
    plot->picked = regrows;
    // A cleared plot keeps the neutral kind (saves require it for bare soil).
    if (!regrows) plot->kind = CropKind::Roots;
    plot->growth = regrows ? std::max(0.0, 1.0 - crop.regrowHours / crop.growHours) : 0.0;
    const auto counted = [](Item item, int count)
    {
        std::string text = CountedName(item, count);
        for (auto& c : text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return text;
    };
    std::string message = "Harvested " + counted(crop.produce, crop.produceCount);
    if (crop.bonus != Item::Count && crop.bonusCount > 0) message += " and " + counted(crop.bonus, crop.bonusCount);
    message += regrows ? ". More will ripen in about " + std::to_string(CropRegrowDays(plot->kind))
            + (CropRegrowDays(plot->kind) == 1 ? " day." : " days.")
        : ". This plot is ready to replant.";
    return Exert(Exertion::HarvestCropEnergy, Good(message));
}
Result Simulation::FillWater(Point player)
{
    if (state_.failed) return Failed();
    if (Count(Item::WateringCan) == 0) return Bad("Carry your pail to collect water.");
    if (!NearWater(player)) return Bad("Walk to the stream to refill your pail.");
    if (Count(Item::Water) >= 6) return Bad("Your pail is already full.");
    const Inventory change = Items({{Item::Water, 6 - Count(Item::Water)}});
    if (auto ready = CheckExertion(Exertion::FillWaterEnergy); !ready) return ready;
    if (!TryAdjust(change)) return Bad("Make enough room in your pack for six water portions.");
    return Exert(Exertion::FillWaterEnergy, Good("Pail filled with six water portions."));
}
Result Simulation::AddFuel(int structureId, Point player)
{
    if (state_.failed) return Failed();
    auto* fire = Find(state_.structures, structureId);
    if (!fire || fire->kind != Piece::Fire) return Bad("Choose a cookfire to fuel.");
    if (!Near(player, Homestead::StructureCenter(state_, *fire))) return Bad("Move closer to fuel this cookfire.");
    if (fire->fuelHours > MaxFuel - 4.0) return Bad("This fire has enough fuel. Add more after it burns down.");
    const Item fuel = Count(Item::Firewood) > 0 ? Item::Firewood : Item::Branch;
    if (auto ready = CheckExertion(Exertion::FuelEnergy); !ready) return ready;
    if (!TryAdjust(Items({{fuel, -1}}))) return Bad("Carry firewood or a branch to fuel the fire.");
    fire->fuelHours += 4.0;
    return Exert(Exertion::FuelEnergy, Good(fuel == Item::Firewood ? "Added firewood: four more hours of fire."
        : "Added a branch: four more hours of fire."));
}
Result Simulation::Transfer(int chestId, Item item, int amount, Point player)
{
    if (state_.failed) return Failed();
    auto* chest = Find(state_.structures, chestId);
    if (!chest || chest->kind != Piece::Chest) return Bad("Choose a storage chest.");
    if (!Near(player, Homestead::StructureCenter(state_, *chest), ChestReach)) return Bad("Move within 280 cm of this chest.");
    if (!ValidEnum(item, Item::Count) || amount == 0 || amount < -ChestCapacity || amount > ChestCapacity)
        return Bad("Choose an item and a transfer amount this pack or chest can hold.");
    const int index = static_cast<int>(item);
    if (amount > 0 && state_.inventory[index] < amount) return Bad("Your pack does not contain that many items.");
    if (amount < 0 && chest->storage[index] < -amount) return Bad("The chest does not contain that many items.");
    State candidate = state_;
    candidate.inventory[index] -= amount;
    Find(candidate.structures, chestId)->storage[index] += amount;
    return CommitInventory(std::move(candidate), amount > 0 ? "Items stored in the chest." : "Items taken from the chest.");
}
Result Simulation::SetDayMinutes(double minutes)
{
    if (state_.failed) return Failed();
    if (!FiniteRange(minutes, 1.0, 1440.0)) return Bad("Day length must be between one and 1440 real minutes.");
    state_.dayMinutes = minutes;
    return Good("Day length updated.");
}
double Simulation::Step(double hours, Point player, bool sleeping, double recoveryPerHour)
{
    (void)player;
    const bool rain = IsRaining();
    const double hungerRate = sleeping ? -1.3 : -2.0;
    const double energyRate = sleeping ? recoveryPerHour : -Exertion::AwakePerHour;
    // Stop at the first failed vital, rather than consuming hours beyond the checkpoint boundary.
    double elapsed = hours;
    elapsed = std::min(elapsed, state_.hunger / -hungerRate);
    if (energyRate < 0) elapsed = std::min(elapsed, state_.energy / -energyRate);
    state_.hunger = Clamp(state_.hunger + hungerRate * elapsed, 0.0, 100.0);
    state_.energy = Clamp(state_.energy + energyRate * elapsed, 0.0, 100.0);
    for (auto& piece : state_.structures)
        if (piece.kind == Piece::Fire) piece.fuelHours = std::max(0.0, piece.fuelHours - elapsed);
    BurnLamp(elapsed, sleeping);
    for (auto& plot : state_.plots)
    {
        plot.moisture = Clamp(plot.moisture + (rain ? 0.3 : -0.025) * elapsed, 0.0, 1.0);
        plot.weeds = Clamp(plot.weeds + 0.009 * elapsed, 0.0, 1.0);
        if (plot.planted)
        {
            const double growingHours = GetCropInfo(plot.kind).growHours;
            const double rate = MoistureGrowthFactor(plot.moisture) * WeedGrowthFactor(plot.weeds) / growingHours;
            plot.growth = Clamp(plot.growth + elapsed * rate, 0.0, 1.0);
        }
    }
    const double before = state_.hour;
    const int dayBefore = static_cast<int>(std::floor((before - DayRolloverHour) / 24.0));
    state_.hour += elapsed;
    // Townsfolk buy down her goods in the shops each morning.
    if (std::floor((state_.hour - DayRolloverHour) / 24.0) > std::floor((before - DayRolloverHour) / 24.0))
        SellDownShops();
    // Only hunger fails her. Energy running out makes her doze off (AdvanceGameHours).
    if (state_.energy <= 1e-10) state_.energy = 0.0;
    if (state_.hunger <= 1e-10)
    {
        state_.hunger = 0.0;
        state_.failed = true;
    }
    if (const int day = static_cast<int>(std::floor((state_.hour - DayRolloverHour) / 24.0)); day > dayBefore) CreepWeeds(day);
    return elapsed;
}
Result Simulation::SetEnergy(double energy)
{
    if (state_.failed) return Failed();
    if (!FiniteRange(energy, 0.0, 100.0)) return Bad("Energy must be between 0 and 100.");
    state_.energy = energy;
    ++revision_;
    return Good("Energy set.");
}
double Simulation::DozeOff(Point player)
{
    double left = std::min(Exertion::DozeHours, MaxHour - state_.hour);
    double slept = 0.0;
    while (left > 1e-12 && !state_.failed)
    {
        double step = std::min(left, TimeStep);
        step = std::min(step, std::floor(state_.hour + 1e-9) + 1.0 - state_.hour);
        for (const auto& piece : state_.structures)
            if (piece.kind == Piece::Fire && piece.fuelHours > 0) step = std::min(step, piece.fuelHours);
        const double done = Step(step, player, true, Exertion::DozePerHour);
        left -= done;
        slept += done;
    }
    ++dozes_;
    ++revision_;
    return slept;
}
void Simulation::Advance(double realSeconds, Point player, bool paused)
{
    if (paused || !FiniteRange(realSeconds, 0.0, 31536000.0)) return;
    AdvanceGameHours(realSeconds * 24.0 / (state_.dayMinutes * 60.0), player);
}
Result Simulation::SpendSprintEnergy(double realSeconds)
{
    if (state_.failed) return Failed();
    if (!FiniteRange(realSeconds, 0.0, 10.0) || realSeconds <= 0)
        return Bad("Sprint requires a positive finite time step.");
    if (state_.energy <= 10.0)
        return Bad("Rest to regain enough energy to sprint.");
    state_.energy = std::max(10.0, state_.energy - 0.35 * realSeconds);
    return {true, "", ResultCode::None, revision_};
}
Result Simulation::CheckExertion(double cost) const
{
    if (state_.failed) return Failed();
    if (state_.energy - cost < Exertion::Reserve) return Bad("You're too exhausted to keep working. Eat something or rest.");
    return {true, "", ResultCode::None, revision_};
}
Result Simulation::Exert(double cost, Result done)
{
    if (done.ok) state_.energy = Clamp(state_.energy - cost, 0.0, 100.0);
    return done;
}
double Simulation::HarvestCost(int nodeId) const
{
    const auto* node = Find(state_.resources, nodeId);
    if (!node) return 0.0;
    if (node->kind == ResourceKind::ForestTree) return Exertion::FellEnergy;
    return node->kind == ResourceKind::Sapling ? Exertion::SaplingEnergy : Exertion::GatherEnergy;
}
double Simulation::ClearCost(int nodeId) const
{
    const auto* node = Find(state_.resources, nodeId);
    if (!node) return 0.0;
    if (node->kind == ResourceKind::ForestTree) return Exertion::FellEnergy;
    return node->kind == ResourceKind::Sapling ? Exertion::SaplingEnergy : Exertion::ClearEnergy;
}
void Simulation::AdvanceGameHours(double hours, Point player)
{
    if (state_.failed || !ValidPoint(player) || !FiniteRange(hours, 0.0, 8760.0) ||
        state_.hour + hours > MaxHour) return;
    while (hours > 1e-12 && !state_.failed)
    {
        double step = std::min(hours, TimeStep);
        // Split at weather/day boundaries and fire expiry so batching does not prolong their effects.
        const double nextHour = std::floor(state_.hour + 1e-9) + 1.0;
        step = std::min(step, nextHour - state_.hour);
        for (const auto& piece : state_.structures)
            if (piece.kind == Piece::Fire && piece.fuelHours > 0) step = std::min(step, piece.fuelHours);
        hours -= Step(step, player, false);
        // Worn out: she dozes off where she stands, and the rough sleep counts against the time asked.
        if (!state_.failed && state_.energy <= 1e-10) hours = std::max(0.0, hours - DozeOff(player));
    }
}
void Simulation::SkipToHourOfDay(double hourOfDay)
{
    if (state_.failed || !FiniteRange(hourOfDay, 0.0, 24.0)) return;
    double target = std::floor(state_.hour / 24.0) * 24.0 + hourOfDay;
    if (target <= state_.hour) target += 24.0;
    if (target > MaxHour) return;
    state_.hour = target;
    ++revision_;
}
Result Simulation::SetCropGrowthForPlaytest(double growth)
{
    if (state_.failed) return Failed();
    if (!FiniteRange(growth, 0.0, 1.0)) return Bad("Growth must be between 0 and 1.");
    int count = 0;
    for (auto& plot : state_.plots)
        if (plot.planted)
        {
            plot.growth = growth;
            ++count;
        }
    ++revision_;
    return Good(std::to_string(count) + (count == 1 ? " crop set to " : " crops set to ")
        + std::to_string(static_cast<int>(std::lround(growth * 100))) + "% grown.");
}
Result Simulation::PassDaysForPlaytest(double days, bool tend, Point player)
{
    if (state_.failed) return Failed();
    if (!FiniteRange(days, 0.0, 60.0) || days <= 0.0) return Bad("Choose between a moment and 60 days.");
    double left = std::min(days * 24.0, MaxHour - state_.hour);
    while (left > 1e-9)
    {
        if (tend)
            for (auto& plot : state_.plots)
            {
                plot.moisture = 1.0;
                plot.weeds = 0.0;
            }
        state_.hunger = 100.0;
        state_.energy = 100.0;
        left -= Step(std::min(1.0, left), player, false);
    }
    state_.hunger = 100.0;
    state_.energy = 100.0;
    ++revision_;
    const int whole = static_cast<int>(std::lround(days));
    return Good(std::to_string(whole) + (whole == 1 ? " day passes" : " days pass") + (tend ? "; the garden was tended." : "."));
}
Result Simulation::Sleep(double hours, Point player)
{
    if (state_.failed) return Failed();
    if (!FiniteRange(hours, 0.25, 12.0)) return Bad("Choose between a quarter hour and twelve hours of sleep.");
    if (FindNearestStructure(player, Piece::Bed, Reach) == -1) return Bad("Place a bed and move beside it before sleeping.");
    if (state_.hour + hours > MaxHour) return Bad("The calendar has reached its supported limit.");
    while (hours > 1e-12 && !state_.failed)
    {
        double step = std::min(hours, TimeStep);
        step = std::min(step, std::floor(state_.hour + 1e-9) + 1.0 - state_.hour);
        for (const auto& piece : state_.structures)
            if (piece.kind == Piece::Fire && piece.fuelHours > 0) step = std::min(step, piece.fuelHours);
        const double done = Step(step, player, true);
        hours -= done;
    }
    if (state_.failed) return Bad("Your rest was interrupted by a critical need. Load your recent checkpoint.");
    return Good("You wake rested. Your garden and fires continued through the night.");
}

std::string Simulation::Serialize() const
{
    std::ostringstream body;
    body.imbue(std::locale::classic());
    body << std::setprecision(std::numeric_limits<double>::max_digits10);
    body << state_.hour << ' ' << state_.dayMinutes << ' ' << state_.hunger << ' '
         << state_.energy << ' ' << state_.failed << ' ' << state_.nextId << '\n';
    WriteStock(body, state_.inventory);
    // The fixed estate stores its placement bake version in the seed slot, marked by the version.
    body << (state_.fixedEstate ? static_cast<std::uint64_t>(state_.placementBakeVersion) : state_.world.seed) << ' '
         << (state_.fixedEstate ? EstateWorldMarker : state_.world.generationVersion) << ' '
         << state_.activeChunk.x << ' ' << state_.activeChunk.y << '\n';
    body << state_.resourceEdits.size() << '\n';
    for (const auto& edit : state_.resourceEdits)
        body << edit.key.chunk.x << ' ' << edit.key.chunk.y << ' ' << edit.key.localId << ' '
             << edit.cleared << ' ' << edit.readyAtHour << '\n';
    body << state_.buildings.size() << '\n';
    for (const auto& building : state_.buildings)
        body << building.id << ' ' << building.origin.x << ' ' << building.origin.y << ' ' << building.yaw << '\n';
    body << state_.structures.size() << '\n';
    for (const auto& piece : state_.structures)
    {
        body << piece.id << ' ' << static_cast<int>(piece.kind) << ' ' << piece.buildingId << ' ' << piece.cellX << ' '
             << piece.cellY << ' ' << piece.rotation << ' ' << piece.fuelHours;
        WriteStock(body, piece.storage);
        WriteLayout(body, piece.layout);
    }
    body << state_.plots.size() << '\n';
    for (const auto& plot : state_.plots)
        body << plot.id << ' ' << plot.cellX << ' ' << plot.cellY << ' ' << plot.planted << ' '
             << plot.growth << ' ' << plot.moisture << ' ' << plot.weeds << ' ' << static_cast<int>(plot.kind) << '\n';
    body << state_.worldDrops.size() << '\n';
    for (const auto& drop : state_.worldDrops)
        body << drop.id << ' ' << drop.position.x << ' ' << drop.position.y << ' '
             << static_cast<int>(drop.item) << ' ' << drop.quantity << ' ' << drop.wearableId << '\n';
    body << state_.nextWearableId << ' ' << state_.nextGroupId << '\n' << state_.wearables.size() << '\n';
    for (const auto& item : state_.wearables)
        body << item.id << ' ' << static_cast<int>(item.definition) << ' ' << item.dye << ' '
             << static_cast<int>(item.owner) << ' ' << item.chestId << '\n';
    body << EquipmentSlotCount;
    for (int id : state_.equipment) body << ' ' << id;
    body << '\n';
    WriteLayout(body, state_.inventoryLayout);
    body << state_.clearedUnderbrush.size() << '\n';
    for (const auto& plant : state_.clearedUnderbrush)
        body << plant.chunk.x << ' ' << plant.chunk.y << ' ' << plant.index << '\n';
    // Optional trailing sections; saves without them still load.
    WriteParcelOwnership(body, state_);
    WriteEconomy(body);
    // Tagged trailing section: tool tiers.
    body << "tools " << ToolKindCount;
    for (const ToolTier tier : state_.toolTiers) body << ' ' << static_cast<int>(tier);
    body << '\n';
    // Optional tagged trailing sections; saves without them still load.
    if (Manor::HasSaveSection(state_)) Manor::WriteSaveSection(body, state_);
    Lamp::WriteSaveSection(body, state_);
    Crops::WriteSaveSection(body, state_);
    Crafting::WriteSaveSections(body, state_);
    const std::string payload = body.str();
    std::ostringstream output;
    output.imbue(std::locale::classic());
    output << "HOMESTEAD " << SimulationSaveVersion << ' ' << payload.size() << ' ' << Checksum(payload) << '\n' << payload;
    return output.str();
}
Result Simulation::Deserialize(const std::string& data)
{
    const auto invalid = [&] { return Result{false,
        "This save is corrupt or incomplete. Your current game was not changed.", ResultCode::CorruptSave, revision_}; };
    if (data.empty() || data.size() > MaxSaveBytes) return invalid();
    const auto newline = data.find('\n');
    if (newline == std::string::npos || newline > 100) return invalid();
    std::istringstream header(data.substr(0, newline));
    header.imbue(std::locale::classic());
    std::string magic;
    int version = 0;
    std::uint64_t size = 0, checksum = 0;
    if (!(header >> magic >> version >> size >> checksum) || magic != "HOMESTEAD") return invalid();
    header >> std::ws;
    if (!header.eof()) return invalid();
    if (version == RetiredTestSaveVersion) return {false,
        "This save is from an earlier test build and can't be opened by this one. Start a new game; no save was changed.",
        ResultCode::UnsupportedVersion, revision_};
    const Result newer{false,
        "This save comes from a newer build of the game. Open it with that build; no save was changed.",
        ResultCode::NewerBuild, revision_};
    if (version > SimulationSaveVersion) return newer;
    if (version != SimulationSaveVersion && version != PositionalStockSaveVersion && version != FreeBuildingSaveVersion
        && version != GardenSquareSaveVersion && version != LegacySimulationSaveVersion
        && version != GardenSquareSaveVersion - 1) return {false,
        "This test save uses an incompatible version. Start a new woodland with this build; no save was changed.",
        ResultCode::UnsupportedVersion, revision_};
    const int storedItems = version == LegacySimulationSaveVersion ? static_cast<int>(Item::Machete)
        : version < ClothingSaveVersion ? static_cast<int>(Item::Fur) : ItemCount;
    const int storedSlots = version < ClothingSaveVersion ? static_cast<int>(EquipmentSlot::Outer)
        : PositionalStockEquipmentSlots;
    const std::string payload = data.substr(newline + 1);
    if (size != payload.size() || Checksum(payload) != checksum) return invalid();
    for (unsigned char c : payload) if (c > 127 || (c < 32 && c != '\n' && c != '\r' && c != '\t')) return invalid();
    std::istringstream input(payload);
    input.imbue(std::locale::classic());
    State candidate;
    // The first line lost warmth and the warm-outfit flag with the estate pivot; older lines still
    // carry them (8 fields instead of 6) and they are read and discarded.
    std::string vitals;
    if (!std::getline(input, vitals)) return invalid();
    std::istringstream line(vitals);
    line.imbue(std::locale::classic());
    std::vector<std::string> fields;
    for (std::string field; line >> field;) fields.push_back(field);
    const bool legacyVitals = fields.size() == 8;
    if (!legacyVitals && fields.size() != 6) return invalid();
    std::istringstream vitalsInput(vitals);
    vitalsInput.imbue(std::locale::classic());
    double legacyWarmth = 50.0;
    bool legacyWarmOutfit = false;
    if (!(vitalsInput >> candidate.hour >> candidate.dayMinutes >> candidate.hunger >> candidate.energy)
        || (legacyVitals && !(vitalsInput >> legacyWarmth)) || !ReadBool(vitalsInput, candidate.failed)
        || (legacyVitals && !ReadBool(vitalsInput, legacyWarmOutfit)) || !(vitalsInput >> candidate.nextId)) return invalid();
    if (!FiniteRange(candidate.hour, 6.0, MaxHour) || !FiniteRange(candidate.dayMinutes, 1.0, 1440.0) ||
        !FiniteRange(candidate.hunger, 0.0, 100.0) || !FiniteRange(candidate.energy, 0.0, 100.0) ||
        !FiniteRange(legacyWarmth, 0.0, 100.0) || candidate.nextId < 1 ||
        candidate.nextId >= TransientResourceIdBase) return invalid();
    const bool critical = candidate.hunger == 0 || candidate.energy == 0 || legacyWarmth == 0;
    if (critical != candidate.failed) return invalid();
    switch (ReadSavedStock(input, candidate.inventory, version, storedItems, InventoryCapacity))
    {
    case SavedStock::Ok: break;
    case SavedStock::Newer: return newer;
    default: return invalid();
    }
    std::set<int> ids;
    const auto acceptId = [&](int id) { return id > 0 && id < candidate.nextId && ids.insert(id).second; };
    int count = 0;
    std::uint64_t generationVersion = 0;
    if (!ReadUnsigned(input, candidate.world.seed) || !ReadUnsigned(input, generationVersion)) return invalid();
    candidate.fixedEstate = generationVersion == EstateWorldMarker;
    if (candidate.fixedEstate)
    {
        if (!placements_ || candidate.world.seed != static_cast<std::uint64_t>(placements_->bakeVersion)) return {false,
            "This test save belongs to a different estate layout. Start a new game with this build; no save was changed.",
            ResultCode::UnsupportedVersion, revision_};
        candidate.placementBakeVersion = placements_->bakeVersion;
        candidate.world.seed = 0;
        generationVersion = Generation::WorldGenerationVersion;
    }
    if (generationVersion != Generation::WorldGenerationVersion) return {false,
        "This save uses an unsupported world generation version. No terrain or saved changes were regenerated.",
        ResultCode::UnsupportedVersion, revision_};
    candidate.world.generationVersion = static_cast<std::uint32_t>(generationVersion);
    if (!(input >> candidate.activeChunk.x >> candidate.activeChunk.y) ||
        candidate.activeChunk.x < -417 || candidate.activeChunk.x > 416 ||
        candidate.activeChunk.y < -417 || candidate.activeChunk.y > 416) return invalid();
    if (!(input >> count) || count < 0 || count > MaxResourceEdits) return invalid();
    for (int i = 0; i < count; ++i)
    {
        ResourceEdit edit;
        std::uint64_t localId = 0;
        if (!(input >> edit.key.chunk.x >> edit.key.chunk.y) || !ReadUnsigned(input, localId) ||
            localId > std::numeric_limits<std::uint32_t>::max() ||
            !ReadBool(input, edit.cleared) || !(input >> edit.readyAtHour)) return invalid();
        edit.key.localId = static_cast<std::uint32_t>(localId);
        if (!candidate.resourceEdits.empty() && !(candidate.resourceEdits.back().key < edit.key)) return invalid();
        if (candidate.fixedEstate)
        {
            const auto& all = placements_->placements;
            const auto placement = std::find_if(all.begin(), all.end(),
                [&](const EstatePlacement& value) { return value.id == static_cast<int>(edit.key.localId); });
            if (placement == all.end() || edit.key.chunk.x != 0 || edit.key.chunk.y != 0) continue;
            if (!FiniteRange(edit.readyAtHour, 0.0, candidate.hour + Regrowth(placement->kind))
                || (edit.cleared && edit.readyAtHour != 0.0)) return invalid();
            candidate.resourceEdits.push_back(edit);
            continue;
        }
        Generation::GeneratedEntity entity;
        const auto found = Generation::FindEntity(candidate.world, edit.key, entity);
        // A candidate the generator has since retired (e.g. a tree now under a granite knob) drops quietly.
        if (found == Generation::Status::NotFound) continue;
        if (found != Generation::Status::Ok) return invalid();
        ResourceNode node;
        if (!GeneratedNode(candidate, entity, node) ||
            !FiniteRange(edit.readyAtHour, 0.0, candidate.hour + Regrowth(node.kind)) ||
            (edit.readyAtHour != 0.0 && edit.readyAtHour < 6.0 + Regrowth(node.kind)) ||
            (edit.cleared && edit.readyAtHour != 0.0) ||
            (!edit.cleared && (edit.readyAtHour == 0.0 || node.kind == ResourceKind::ForestTree))) return invalid();
        candidate.resourceEdits.push_back(edit);
    }
    std::set<std::tuple<int, int, int, int>> cells;
    std::set<std::pair<int, Edge>> edges;
    std::set<std::tuple<int, int, int>> furniture;
    if (version >= FreeBuildingSaveVersion)
    {
        if (!(input >> count) || count < 0 || count > MaxObjects) return invalid();
        for (int i = 0; i < count; ++i)
        {
            Building building;
            if (!(input >> building.id >> building.origin.x >> building.origin.y >> building.yaw)
                || !acceptId(building.id) || !ValidPoint(building.origin)
                || !FiniteRange(building.yaw, 0.0, 360.0) || building.yaw >= 360.0) return invalid();
            candidate.buildings.push_back(building);
        }
    }
    if (!(input >> count) || count < 0 || count > MaxObjects) return invalid();
    for (int i = 0; i < count; ++i)
    {
        Structure piece;
        int kind = 0;
        if (!(input >> piece.id >> kind) ||
            (version >= FreeBuildingSaveVersion && !(input >> piece.buildingId)) ||
            !(input >> piece.cellX >> piece.cellY >> piece.rotation >> piece.fuelHours)) return invalid();
        const SavedStock storage = ReadSavedStock(input, piece.storage, version, storedItems, ChestCapacity);
        if (storage == SavedStock::Newer) return newer;
        if (storage != SavedStock::Ok || !ReadLayout(input, piece.layout)) return invalid();
        piece.kind = static_cast<Piece>(kind);
        PlacementTarget site;
        site.kind = piece.kind;
        site.buildingId = piece.buildingId;
        site.cellX = piece.cellX;
        site.cellY = piece.cellY;
        if (!acceptId(piece.id) || !ValidEnum(piece.kind, Piece::Count) || piece.buildingId < 0 ||
            !SiteBuilding(candidate, site) ||
            piece.rotation < 0 || piece.rotation >= 4 ||
            !FiniteRange(piece.fuelHours, 0.0, MaxFuel) ||
            (piece.kind != Piece::Fire && piece.fuelHours != 0.0) ||
            (piece.kind != Piece::Chest && !Empty(piece.storage))) return invalid();
        if (EdgePiece(piece.kind))
        {
            if (!edges.insert({piece.buildingId, EdgeKey(piece.cellX, piece.cellY, piece.rotation)}).second) return invalid();
        }
        else if (!Crafting::IsMovable(piece.kind) && !cells.insert({piece.buildingId, piece.cellX, piece.cellY, kind}).second)
            return invalid();
        if (Furniture(piece.kind) && !Crafting::IsMovable(piece.kind)
            && !furniture.insert({piece.buildingId, piece.cellX, piece.cellY}).second) return invalid();
        candidate.structures.push_back(piece);
    }
    for (const auto& piece : candidate.structures)
    {
        if ((piece.kind == Piece::Roof || EdgePiece(piece.kind)) &&
            !HasPiece(candidate, Piece::Foundation, piece.buildingId, piece.cellX, piece.cellY)) return invalid();
        const Building* building = FindBuilding(candidate, piece.buildingId);
        if (!CheckFootprintResources(candidate, ResourceFootprint(*building, piece.kind, piece.cellX, piece.cellY,
            piece.rotation, HasPiece(candidate, Piece::Foundation, piece.buildingId, piece.cellX, piece.cellY)), false))
            return invalid();
    }
    std::set<std::pair<int, int>> plots;
    if (!(input >> count) || count < 0 || count > MaxObjects) return invalid();
    for (int i = 0; i < count; ++i)
    {
        Plot plot;
        if (!(input >> plot.id >> plot.cellX >> plot.cellY) || !ReadBool(input, plot.planted) ||
            !(input >> plot.growth >> plot.moisture >> plot.weeds)) return invalid();
        int kind = -1;
        if (!(input >> kind)) return invalid();
        plot.kind = static_cast<CropKind>(kind);
        // Older saves planted a whole building cell; it becomes that cell's middle garden square.
        if (version < GardenSquareSaveVersion)
        {
            if (!ValidCell(plot.cellX, plot.cellY)) return invalid();
            plot.cellX = CellToGarden(plot.cellX);
            plot.cellY = CellToGarden(plot.cellY);
        }
        if (!acceptId(plot.id) || !ValidCell(GardenToCell(plot.cellX), GardenToCell(plot.cellY))
            || !ValidEnum(plot.kind, CropKind::Count) ||
            (!plot.planted && plot.kind != CropKind::Roots) ||
            !FiniteRange(plot.growth, 0.0, 1.0) || !FiniteRange(plot.moisture, 0.0, 1.0) ||
            !FiniteRange(plot.weeds, 0.0, 1.0) || (!plot.planted && plot.growth != 0.0) ||
            !plots.insert({plot.cellX, plot.cellY}).second ||
            !CheckGardenResources(candidate, plot.cellX, plot.cellY)) return invalid();
        for (const auto& piece : candidate.structures)
            if (FootprintsOverlap(Inset(GardenFootprint(plot)),
                Inset(ResourceFootprint(*FindBuilding(candidate, piece.buildingId), piece.kind, piece.cellX, piece.cellY,
                    piece.rotation, HasPiece(candidate, Piece::Foundation, piece.buildingId, piece.cellX, piece.cellY)))))
                return invalid();
        candidate.plots.push_back(plot);
    }
    if (!(input >> count) || count < 0 || count > MaxWorldDrops) return invalid();
    for (int i = 0; i < count; ++i)
    {
        WorldDrop drop;
        int item = -1;
        if (!(input >> drop.id >> drop.position.x >> drop.position.y
            >> item >> drop.quantity >> drop.wearableId)) return invalid();
        drop.item = static_cast<Item>(item);
        if (!acceptId(drop.id) || !ValidPoint(drop.position)
            || std::abs(drop.position.x) > MaxWorldCoordinate
            || std::abs(drop.position.y) > MaxWorldCoordinate)
            return invalid();
        candidate.worldDrops.push_back(drop);
    }
    if (!(input >> candidate.nextWearableId >> candidate.nextGroupId >> count) || count < 0 || count > MaxObjects)
        return invalid();
    for (int i = 0; i < count; ++i)
    {
        WearableInstance item;
        int definition = -1, owner = -1;
        if (!(input >> item.id >> definition >> item.dye >> owner >> item.chestId)) return invalid();
        item.definition = static_cast<WearableDefinition>(definition);
        item.owner = static_cast<WearableOwner>(owner);
        candidate.wearables.push_back(item);
    }
    candidate.equipment.fill(0);
    int savedSlots = storedSlots;
    if (version > PositionalStockSaveVersion)
    {
        if (!(input >> savedSlots) || savedSlots < 1) return invalid();
        if (savedSlots > EquipmentSlotCount) return newer;
    }
    for (int slot = 0; slot < savedSlots; ++slot) if (!(input >> candidate.equipment[slot])) return invalid();
    if (!ReadLayout(input, candidate.inventoryLayout)) return invalid();
    if (version != LegacySimulationSaveVersion)
    {
        if (!(input >> count) || count < 0 || count > MaxUnderbrushEdits) return invalid();
        for (int i = 0; i < count; ++i)
        {
            UnderbrushEdit plant;
            if (!(input >> plant.chunk.x >> plant.chunk.y >> plant.index) ||
                plant.index < 0 || plant.index >= MaxUnderbrushIndex ||
                (!candidate.clearedUnderbrush.empty() && !(candidate.clearedUnderbrush.back() < plant)))
                return invalid();
            candidate.clearedUnderbrush.push_back(plant);
        }
    }
    // Optional trailing sections. The parcels themselves come from the current layout.
    if (candidate.fixedEstate) SeedEstateParcels(candidate, Layout());
    if (!ReadParcelOwnership(input, candidate)) return invalid();
    if (!ReadEconomy(input, candidate, ids)) return invalid();
    RefreshShopCounters(candidate, Layout());
    input >> std::ws;
    // Optional tagged trailing sections, each introduced by its tag word.
    while (!input.eof())
    {
        std::string tag;
        if (!(input >> tag)) return invalid();
        if (tag == "tools")
        {
            // Tiers for tools this build doesn't know are ignored; missing ones stay worn.
            if (!(input >> count) || count < 0 || count > 64) return invalid();
            for (int i = 0; i < count; ++i)
            {
                int tier = -1;
                if (!(input >> tier) || tier < 0 || tier >= ToolTierCount) return invalid();
                if (i < ToolKindCount) candidate.toolTiers[i] = static_cast<ToolTier>(tier);
            }
        }
        else if (tag == Manor::SaveTag) { if (!Manor::ReadSaveSection(input, candidate)) return invalid(); }
        else if (tag == Lamp::SaveTag) { if (!Lamp::ReadSaveSection(input, candidate)) return invalid(); }
        else if (tag == Crops::SaveTag) { if (!Crops::ReadSaveSection(input, candidate)) return invalid(); }
        else if (tag == Crafting::GateTag) { if (!Crafting::ReadGates(input, candidate)) return invalid(); }
        else if (tag == Crafting::SpotTag) { if (!Crafting::ReadSpots(input, candidate)) return invalid(); }
        // A section this build doesn't know came from a newer build; it can't be skipped safely.
        else return newer;
        input >> std::ws;
    }
    if (!input.eof()) return invalid();
    const auto inventory = ValidateInventory(candidate);
    if (!inventory) return {false, inventory.message + " Your current game was not changed.", ResultCode::CorruptSave, revision_};
    int nextHandle = nextResourceHandle_;
    const bool sameWorld = candidate.world.seed == state_.world.seed &&
        candidate.world.generationVersion == state_.world.generationVersion;
    if (candidate.fixedEstate)
    {
        const auto estate = MaterializeEstate(candidate, *placements_);
        if (!estate) return estate;
        // Overgrowth baked after this save was made (the manor clear-out) can land on her plots or
        // under her buildings: there it counts as already cleared.
        for (auto& node : candidate.resources)
        {
            if (node.cleared || !IsOvergrowth(node.kind)) continue;
            const Footprint spot{node.position, {1.0, 1.0}, 0.0};
            bool covered = false;
            for (const auto& plot : candidate.plots) covered = covered || FootprintsOverlap(spot, GardenFootprint(plot));
            for (const auto& piece : candidate.structures)
                covered = covered || (!piece.heritage && FootprintsOverlap(spot, StructureFootprint(candidate, piece)));
            if (!covered) continue;
            node.cleared = true;
            node.readyAtHour = 0.0;
            if (!SaveResourceEdit(candidate, node)) return invalid();
        }
    }
    const auto populated = Materialize(candidate, sameWorld ? &state_ : nullptr, nextHandle);
    if (!populated) return populated;
    state_ = std::move(candidate);
    nextResourceHandle_ = nextHandle;
    // Saves from before the lamp get its kit once.
    GrantLampKit();
    return {true, "Homestead restored. No time passed while you were away.", ResultCode::None, ++revision_};
}
Result Simulation::Deserialize(const std::string& data, Generation::WorldDescriptor expectedWorld)
{
    Simulation candidate = *this;
    const auto result = candidate.Deserialize(data);
    if (!result) return result;
    if (candidate.state_.world.seed != expectedWorld.seed ||
        candidate.state_.world.generationVersion != expectedWorld.generationVersion)
        return {false, "This save does not match the expected world seed and generation version. Your current game was not changed.",
            ResultCode::Invalid, revision_};
    *this = std::move(candidate);
    return result;
}
}
