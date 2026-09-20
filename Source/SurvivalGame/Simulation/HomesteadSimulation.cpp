#include "HomesteadSimulation.h"

#include <algorithm>
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
constexpr int MaxStock = 120;
constexpr double MaxFuel = 48.0;
constexpr std::size_t MaxSaveBytes = 1024 * 1024;

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
    return FiniteRange(p.x, -4000.0, 4000.0) && FiniteRange(p.y, -4000.0, 4000.0);
}
bool ValidCell(int x, int y) { return x >= -13 && x <= 12 && y >= -13 && y <= 12; }
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
Result Bad(const std::string& text) { return {false, text}; }
Result Failed() { return Bad("You need to recover. Load your recent checkpoint to continue."); }
Inventory Items(std::initializer_list<std::pair<Item, int>> values)
{
    Inventory result{};
    for (const auto& value : values) result[static_cast<int>(value.first)] += value.second;
    return result;
}
bool StockValid(const Inventory& stock)
{
    int total = 0;
    for (int count : stock)
    {
        if (count < 0 || count > MaxStock) return false;
        total += count;
    }
    return total <= InventoryCapacity;
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
    case ResourceKind::Reeds: return Items({{Item::Fiber, 5}});
    case ResourceKind::Sapling: return Items({{Item::Branch, 8}, {Item::Fiber, 2}});
    default: return {};
    }
}
Inventory BuildCost(Piece kind)
{
    switch (kind)
    {
    case Piece::Foundation: return Items({{Item::Branch, -4}, {Item::Stone, -2}});
    case Piece::Wall: return Items({{Item::Branch, -3}, {Item::Fiber, -1}});
    case Piece::Doorway: return Items({{Item::Branch, -4}, {Item::Fiber, -1}});
    case Piece::Roof: return Items({{Item::Branch, -4}, {Item::Fiber, -3}});
    case Piece::Fire: return Items({{Item::Branch, -3}, {Item::Stone, -4}});
    case Piece::Bed: return Items({{Item::Branch, -4}, {Item::Fiber, -4}});
    case Piece::Chest: return Items({{Item::Branch, -5}, {Item::Fiber, -2}});
    default: return {};
    }
}
Inventory CraftChange(Recipe recipe)
{
    switch (recipe)
    {
    case Recipe::Hatchet: return Items({{Item::Branch, -4}, {Item::Stone, -3}, {Item::Fiber, -2}, {Item::Hatchet, 1}});
    case Recipe::DiggingStick: return Items({{Item::Branch, -3}, {Item::Stone, -1}, {Item::DiggingStick, 1}});
    case Recipe::WateringCan: return Items({{Item::Branch, -3}, {Item::Fiber, -2}, {Item::WateringCan, 1}});
    case Recipe::RoastedRoots: return Items({{Item::Roots, -2}, {Item::RoastedRoots, 1}});
    case Recipe::HerbedRoots: return Items({{Item::Roots, -2}, {Item::Flowers, -1}, {Item::HerbedRoots, 1}});
    default: return {};
    }
}
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
bool EdgePiece(Piece kind) { return kind == Piece::Wall || kind == Piece::Doorway; }
bool Furniture(Piece kind) { return kind == Piece::Fire || kind == Piece::Bed || kind == Piece::Chest; }
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
bool HasPiece(const State& state, Piece kind, int x, int y)
{
    for (const auto& piece : state.structures)
        if (piece.kind == kind && piece.cellX == x && piece.cellY == y) return true;
    return false;
}
bool BlockedBySapling(const State& state, int x, int y)
{
    for (const auto& node : state.resources)
        if (!node.cleared && node.kind == ResourceKind::Sapling &&
            Cell(node.position.x) == x && Cell(node.position.y) == y) return true;
    return false;
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
bool ReadStock(std::istream& stream, Inventory& stock)
{
    for (int& value : stock) if (!(stream >> value)) return false;
    return StockValid(stock);
}
void WriteStock(std::ostream& stream, const Inventory& stock)
{
    for (int value : stock) stream << ' ' << value;
    stream << '\n';
}
}

const char* ItemName(Item item)
{
    static const char* names[] = {"Knife", "Branch", "Stone", "Fiber", "Berries", "Roots",
        "Meadow herb", "Seeds", "Crude hatchet", "Digging stick", "Watering can", "Water",
        "Roasted roots", "Herbed roots"};
    return ValidEnum(item, Item::Count) ? names[static_cast<int>(item)] : "Unknown item";
}
const char* ResourceName(ResourceKind kind)
{
    static const char* names[] = {"Fallen branches", "Loose stones", "Berry bush", "Wild roots",
        "Meadow herb", "Stream reeds", "Sapling"};
    return ValidEnum(kind, ResourceKind::Count) ? names[static_cast<int>(kind)] : "Unknown resource";
}
const char* RecipeName(Recipe recipe)
{
    static const char* names[] = {"Crude hatchet", "Digging stick", "Watering can",
        "Roasted roots", "Herbed roots"};
    return ValidEnum(recipe, Recipe::Count) ? names[static_cast<int>(recipe)] : "Unknown recipe";
}
const char* PieceName(Piece piece)
{
    static const char* names[] = {"Foundation", "Wall", "Doorway", "Roof", "Cookfire", "Bed", "Chest"};
    return ValidEnum(piece, Piece::Count) ? names[static_cast<int>(piece)] : "Unknown structure";
}
const char* CropName(CropKind kind)
{
    static const char* names[] = {"Roots", "Berries"};
    return ValidEnum(kind, CropKind::Count) ? names[static_cast<int>(kind)] : "Unknown crop";
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
            result[i] += kind == Recipe::RoastedRoots || kind == Recipe::HerbedRoots
                ? "; nearby fueled fire (no pot needed)" : "; knife required";
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
            if (kind == Piece::Fire) result[i] += "; add a branch after placement to light";
        }
        return result;
    }();
    return ValidEnum(piece, Piece::Count) ? descriptions[static_cast<int>(piece)].c_str() : "Unknown structure";
}
double StreamX(double y) { return 1500.0 + 180.0 * std::sin(y / 800.0); }
bool IsNearWater(Point position)
{
    return ValidPoint(position) && std::abs(position.x - StreamX(position.y)) <= 180.0;
}
Point CellCenter(int cellX, int cellY)
{
    return {(static_cast<double>(cellX) + 0.5) * CellSize,
        (static_cast<double>(cellY) + 0.5) * CellSize};
}

Simulation::Simulation() { NewGame(); }
void Simulation::NewGame()
{
    state_ = State{};
    state_.inventory[static_cast<int>(Item::Knife)] = 1;
    // The first eight patches form a short forage route from the arrival clearing to the stream.
    static const Point patches[] = {{-1100, -400}, {-600, -400}, {-100, -400}, {400, -400},
        {900, -400}, {1150, 200}, {550, 550}, {-300, 550}, {-1900, -1100},
        {-2300, 1000}, {2200, 1600}, {2400, -1700}, {-900, 1900}, {400, -2100}};
    for (const Point patch : patches)
    {
        for (int kind = 0; kind < static_cast<int>(ResourceKind::Count); ++kind)
        {
            Point position{patch.x + (kind % 3) * 100.0, patch.y + (kind / 3) * 100.0};
            if (kind == static_cast<int>(ResourceKind::Sapling))
                position = {patch.x - 100.0, patch.y - 200.0};
            if (kind == static_cast<int>(ResourceKind::Reeds) && patch.x > 800)
                position.x = StreamX(position.y) - 100.0;
            state_.resources.push_back({state_.nextId++, static_cast<ResourceKind>(kind), position, 0.0, false});
        }
    }
}
int Simulation::Count(Item item) const
{
    return ValidEnum(item, Item::Count) ? state_.inventory[static_cast<int>(item)] : 0;
}
int Simulation::UsedCapacity() const
{
    int used = 0;
    for (int count : state_.inventory) used += count;
    return used;
}
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
    state_.inventory = updated;
    return true;
}
bool Simulation::IsNight() const
{
    const double hour = std::fmod(state_.hour, 24.0);
    return hour < 6.0 || hour >= 19.0;
}
bool Simulation::IsRaining() const
{
    const int day = static_cast<int>(state_.hour / 24.0);
    const double hour = std::fmod(state_.hour, 24.0);
    return day % 3 == 1 && hour >= 9.0 && hour < 15.0;
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
    const CellKey start{Cell(position.x), Cell(position.y)};
    std::set<CellKey> floors, roofs;
    std::set<Edge> edges;
    for (const auto& piece : state_.structures)
    {
        if (piece.kind == Piece::Foundation) floors.insert({piece.cellX, piece.cellY});
        if (piece.kind == Piece::Roof) roofs.insert({piece.cellX, piece.cellY});
        if (EdgePiece(piece.kind)) edges.insert(EdgeKey(piece.cellX, piece.cellY, piece.rotation));
    }
    if (!floors.count(start) || !roofs.count(start)) return false;
    std::set<CellKey> visited{start};
    std::queue<CellKey> pending;
    pending.push(start);
    static const int dx[] = {0, 1, 0, -1};
    static const int dy[] = {1, 0, -1, 0};
    while (!pending.empty())
    {
        const auto current = pending.front();
        pending.pop();
        for (int side = 0; side < 4; ++side)
        {
            if (edges.count(EdgeKey(current.first, current.second, side))) continue;
            const CellKey adjacent{current.first + dx[side], current.second + dy[side]};
            if (!floors.count(adjacent) || !roofs.count(adjacent)) return false;
            if (visited.insert(adjacent).second) pending.push(adjacent);
        }
    }
    return true;
}
bool Simulation::IsNearFire(Point position) const
{
    for (const auto& piece : state_.structures)
        if (piece.kind == Piece::Fire && piece.fuelHours > 0.0 &&
            Near(position, CellCenter(piece.cellX, piece.cellY), FireReach)) return true;
    return false;
}
bool Simulation::CanHarvest(int nodeId) const
{
    const auto* node = Find(state_.resources, nodeId);
    return !state_.failed && node && !node->cleared && node->readyAtHour <= state_.hour &&
        (node->kind == ResourceKind::Sapling ? Count(Item::Hatchet) > 0 : Count(Item::Knife) > 0);
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
int Simulation::FindNearestPlot(Point position, double maxDistance) const
{
    if (!ValidPoint(position) || !FiniteRange(maxDistance, 0, 12000)) return -1;
    int nearest = -1;
    double distance = maxDistance * maxDistance;
    for (const auto& plot : state_.plots)
    {
        const double current = DistanceSquared(position, CellCenter(plot.cellX, plot.cellY));
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
        const double current = DistanceSquared(position, CellCenter(piece.cellX, piece.cellY));
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
    if (!Near(player, node->position)) return Bad("Move closer to gather this resource.");
    if (node->readyAtHour > state_.hour) return Bad("This patch needs more time to regrow.");
    if (node->kind == ResourceKind::Sapling && Count(Item::Hatchet) == 0)
        return Bad("Craft a crude hatchet before cutting a sapling.");
    if (node->kind != ResourceKind::Sapling && Count(Item::Knife) == 0)
        return Bad("Take your knife from storage before gathering.");
    const Inventory yield = Yield(node->kind);
    if (!TryAdjust(yield)) return Bad(MissingMessage(yield, state_.inventory));
    node->readyAtHour = state_.hour + Regrowth(node->kind);
    return Good(std::string("Gathered ") + ResourceName(node->kind) + ". This patch will regrow.");
}
Result Simulation::Clear(int nodeId, Point player)
{
    if (state_.failed) return Failed();
    auto* node = Find(state_.resources, nodeId);
    if (!node || node->cleared) return Bad("This patch has already been cleared.");
    if (!Near(player, node->position)) return Bad("Move closer to clear this patch.");
    if (node->kind == ResourceKind::Sapling && Count(Item::Hatchet) == 0)
        return Bad("Craft a crude hatchet before clearing saplings.");
    if (node->kind != ResourceKind::Sapling && Count(Item::Knife) == 0)
        return Bad("Take your knife from storage before clearing.");
    const Inventory yield = node->readyAtHour <= state_.hour ? Yield(node->kind) : Inventory{};
    if (!TryAdjust(yield)) return Bad(MissingMessage(yield, state_.inventory));
    node->cleared = true;
    node->readyAtHour = 0.0;
    return Good("Land cleared permanently. This patch will no longer regrow.");
}
Result Simulation::Eat(Item item)
{
    if (state_.failed) return Failed();
    double nutrition = 0.0;
    if (item == Item::Berries) nutrition = 12.0;
    if (item == Item::RoastedRoots) nutrition = 28.0;
    if (item == Item::HerbedRoots) nutrition = 38.0;
    if (nutrition == 0.0) return Bad("Eat berries, roasted roots, or herbed roots. Raw roots need cooking.");
    if (state_.hunger >= 100.0) return Bad("You are already full. Save this food for later.");
    if (!TryAdjust(Items({{item, -1}}))) return Bad(std::string("Gather or cook some ") + ItemName(item) + " first.");
    state_.hunger = std::min(100.0, state_.hunger + nutrition);
    return Good(std::string("Ate ") + ItemName(item) + ".");
}
Result Simulation::Craft(Recipe recipe, Point player)
{
    if (state_.failed) return Failed();
    if (!ValidEnum(recipe, Recipe::Count) || !ValidPoint(player)) return Bad("Choose a valid recipe and location.");
    const Inventory change = CraftChange(recipe);
    const bool cooking = recipe == Recipe::RoastedRoots || recipe == Recipe::HerbedRoots;
    if (cooking && !IsNearFire(player)) return Bad("Move beside a fueled cookfire to cook roots; no pot is needed.");
    if (!cooking && Count(Item::Knife) == 0) return Bad("Take your knife from storage to craft tools.");
    if (!TryAdjust(change)) return Bad(MissingMessage(change, state_.inventory));
    return Good(std::string("Made ") + RecipeName(recipe) + ".");
}
Result Simulation::Place(Piece kind, int cellX, int cellY, int rotation, Point player)
{
    if (state_.failed) return Failed();
    if (!ValidEnum(kind, Piece::Count) || !ValidCell(cellX, cellY))
        return Bad("Choose a valid structure and building cell.");
    rotation = ((rotation % 4) + 4) % 4;
    if (!Near(player, CellCenter(cellX, cellY), 700.0)) return Bad("Move closer to this building site.");
    if (state_.structures.size() >= MaxObjects || state_.nextId >= std::numeric_limits<int>::max() - 1)
        return Bad("The homestead has reached its structure limit.");
    if (BlockedBySapling(state_, cellX, cellY)) return Bad("Clear the sapling from this cell with a hatchet first.");
    for (const auto& plot : state_.plots)
        if (plot.cellX == cellX && plot.cellY == cellY) return Bad("Keep this crop plot clear of buildings.");
    for (const auto& piece : state_.structures)
    {
        if (EdgePiece(kind) && EdgePiece(piece.kind) &&
            EdgeKey(cellX, cellY, rotation) == EdgeKey(piece.cellX, piece.cellY, piece.rotation))
            return Bad("There is already a wall or doorway along that edge.");
        if (piece.cellX != cellX || piece.cellY != cellY) continue;
        if ((!EdgePiece(kind) && piece.kind == kind) || (Furniture(kind) && Furniture(piece.kind)))
            return Bad("That building space is already occupied.");
    }
    if ((kind == Piece::Roof || EdgePiece(kind)) && !HasPiece(state_, Piece::Foundation, cellX, cellY))
        return Bad("Build a foundation in this cell first.");
    const Inventory cost = BuildCost(kind);
    if (!TryAdjust(cost)) return Bad(MissingMessage(cost, state_.inventory));
    state_.structures.push_back({state_.nextId++, kind, cellX, cellY, rotation, 0.0, {}});
    return Good(std::string("Placed ") + PieceName(kind) + ".");
}
Result Simulation::Till(int cellX, int cellY, Point player)
{
    if (state_.failed) return Failed();
    if (!ValidCell(cellX, cellY) || !Near(player, CellCenter(cellX, cellY))) return Bad("Move closer to a valid garden cell.");
    if (Count(Item::DiggingStick) == 0) return Bad("Craft a digging stick before tilling soil.");
    if (state_.plots.size() >= MaxObjects || state_.nextId >= std::numeric_limits<int>::max() - 1)
        return Bad("The garden has reached its plot limit.");
    if (BlockedBySapling(state_, cellX, cellY)) return Bad("Clear the sapling from this cell before tilling.");
    for (const auto& structure : state_.structures)
        if (structure.cellX == cellX && structure.cellY == cellY) return Bad("Choose soil away from buildings.");
    for (const auto& plot : state_.plots)
        if (plot.cellX == cellX && plot.cellY == cellY) return Bad("This cell is already tilled.");
    state_.plots.push_back({state_.nextId++, cellX, cellY, false, 0.0, 0.35, 0.0});
    return Good("Soil tilled. Plant wild-root seeds or seeds from a foraged berry here.");
}
Result Simulation::Plant(int plotId, Point player, CropKind kind)
{
    if (state_.failed) return Failed();
    if (!ValidEnum(kind, CropKind::Count)) return Bad("Choose roots or berries to plant.");
    auto* plot = Find(state_.plots, plotId);
    if (!plot || !Near(player, CellCenter(plot->cellX, plot->cellY))) return Bad("Move beside a tilled plot to plant.");
    if (plot->planted) return Bad("A crop is already growing here.");
    const bool berries = kind == CropKind::Berries;
    const Item plantingItem = berries ? Item::Berries : Item::Seeds;
    if (!TryAdjust(Items({{plantingItem, -1}})))
        return Bad(berries ? "Gather a berry to plant the seeds from its fruit." : "Gather seeds from wild roots before planting.");
    plot->kind = kind;
    plot->planted = true;
    plot->growth = 0.0;
    return Good(berries ? "Planted the seeds from one berry. Water and weed; this bush will regrow after harvest."
        : "Roots planted. Water and weed to encourage growth.");
}
Result Simulation::Water(int plotId, Point player)
{
    if (state_.failed) return Failed();
    auto* plot = Find(state_.plots, plotId);
    if (!plot || !Near(player, CellCenter(plot->cellX, plot->cellY))) return Bad("Move beside a garden plot to water it.");
    if (Count(Item::WateringCan) == 0) return Bad("Craft a watering can first.");
    if (plot->moisture >= 1.0) return Bad("This soil is already fully watered.");
    if (!TryAdjust(Items({{Item::Water, -1}}))) return Bad("Refill your watering can at the stream.");
    plot->moisture = 1.0;
    return Good("Soil watered.");
}
Result Simulation::Weed(int plotId, Point player)
{
    if (state_.failed) return Failed();
    auto* plot = Find(state_.plots, plotId);
    if (!plot || !Near(player, CellCenter(plot->cellX, plot->cellY))) return Bad("Move beside a garden plot to weed it.");
    if (plot->weeds <= 0.0) return Bad("This plot is already free of weeds.");
    plot->weeds = 0.0;
    return Good("Weeds removed. The crop has more room to grow.");
}
Result Simulation::HarvestCrop(int plotId, Point player)
{
    if (state_.failed) return Failed();
    auto* plot = Find(state_.plots, plotId);
    if (!plot || !Near(player, CellCenter(plot->cellX, plot->cellY))) return Bad("Move beside your crop to harvest.");
    if (!plot->planted || plot->growth < 1.0) return Bad("This crop is not ready to harvest.");
    const bool berries = plot->kind == CropKind::Berries;
    const Inventory yield = berries ? Items({{Item::Berries, 6}}) : Items({{Item::Roots, 4}, {Item::Seeds, 2}});
    if (!TryAdjust(yield)) return Bad(MissingMessage(yield, state_.inventory));
    plot->planted = berries;
    plot->growth = 0.0;
    return Good(berries ? "Harvested six berries. The bush remains planted and will grow more fruit."
        : "Harvested four roots and two seeds. This plot is ready to replant.");
}
Result Simulation::FillWater(Point player)
{
    if (state_.failed) return Failed();
    if (Count(Item::WateringCan) == 0) return Bad("Craft a watering can before collecting water.");
    if (!IsNearWater(player)) return Bad("Walk to the stream to refill your watering can.");
    if (Count(Item::Water) >= 6) return Bad("Your watering can is already full.");
    const Inventory change = Items({{Item::Water, 6 - Count(Item::Water)}});
    if (!TryAdjust(change)) return Bad("Make enough room in your pack for six water portions.");
    return Good("Watering can filled with six water portions.");
}
Result Simulation::AddFuel(int structureId, Point player)
{
    if (state_.failed) return Failed();
    auto* fire = Find(state_.structures, structureId);
    if (!fire || fire->kind != Piece::Fire) return Bad("Choose a cookfire to fuel.");
    if (!Near(player, CellCenter(fire->cellX, fire->cellY))) return Bad("Move closer to fuel this cookfire.");
    if (fire->fuelHours > MaxFuel - 4.0) return Bad("This fire has enough fuel. Add more after it burns down.");
    if (!TryAdjust(Items({{Item::Branch, -1}}))) return Bad("Gather a branch to fuel the fire.");
    fire->fuelHours += 4.0;
    return Good("Added a branch: four more hours of fire.");
}
Result Simulation::Transfer(int chestId, Item item, int amount, Point player)
{
    if (state_.failed) return Failed();
    auto* chest = Find(state_.structures, chestId);
    if (!chest || chest->kind != Piece::Chest) return Bad("Choose a storage chest.");
    if (!Near(player, CellCenter(chest->cellX, chest->cellY))) return Bad("Move closer to this chest.");
    if (!ValidEnum(item, Item::Count) || amount == 0 || amount < -InventoryCapacity || amount > InventoryCapacity)
        return Bad("Choose an item and a transfer amount between one and 120.");
    const int index = static_cast<int>(item);
    Inventory storage = chest->storage;
    storage[index] += amount;
    if (!StockValid(storage)) return Bad(amount > 0 ? "The chest does not have enough space." : "The chest does not contain that many items.");
    const Inventory change = Items({{item, -amount}});
    if (!TryAdjust(change)) return Bad(MissingMessage(change, state_.inventory));
    chest->storage = storage;
    return Good(amount > 0 ? "Items stored in the chest." : "Items taken from the chest.");
}
Result Simulation::SetDayMinutes(double minutes)
{
    if (state_.failed) return Failed();
    if (!FiniteRange(minutes, 1.0, 1440.0)) return Bad("Day length must be between one and 1440 real minutes.");
    state_.dayMinutes = minutes;
    return Good("Day length updated.");
}
void Simulation::SetWarmOutfit(bool enabled) { if (!state_.failed) state_.warmOutfit = enabled; }
void Simulation::Step(double hours, Point player, bool sleeping)
{
    const bool rain = IsRaining();
    const bool sheltered = IsSheltered(player);
    const bool fire = IsNearFire(player);
    double warmthRate = IsNight() ? -6.0 : 3.0;
    if (rain && !sheltered) warmthRate -= 1.5;
    if (state_.warmOutfit && warmthRate < 0) warmthRate += 2.0;
    if (sheltered) warmthRate = std::max(1.0, warmthRate + 7.0);
    if (fire) warmthRate = std::max(6.0, warmthRate + 12.0);
    const double hungerRate = sleeping ? -1.3 : -2.0;
    const double energyRate = sleeping ? 10.0 : -2.4;
    // Stop at the first failed vital, rather than consuming hours beyond the checkpoint boundary.
    double elapsed = hours;
    elapsed = std::min(elapsed, state_.hunger / -hungerRate);
    if (energyRate < 0) elapsed = std::min(elapsed, state_.energy / -energyRate);
    if (warmthRate < 0) elapsed = std::min(elapsed, state_.warmth / -warmthRate);
    state_.hunger = Clamp(state_.hunger + hungerRate * elapsed, 0.0, 100.0);
    state_.energy = Clamp(state_.energy + energyRate * elapsed, 0.0, 100.0);
    state_.warmth = Clamp(state_.warmth + warmthRate * elapsed, 0.0, 100.0);
    for (auto& piece : state_.structures)
        if (piece.kind == Piece::Fire) piece.fuelHours = std::max(0.0, piece.fuelHours - elapsed);
    for (auto& plot : state_.plots)
    {
        plot.moisture = Clamp(plot.moisture + (rain ? 0.3 : -0.025) * elapsed, 0.0, 1.0);
        plot.weeds = Clamp(plot.weeds + 0.009 * elapsed, 0.0, 1.0);
        if (plot.planted)
        {
            const double moistureFactor = 0.15 + 0.85 * plot.moisture;
            const double weedFactor = 1.0 - 0.7 * plot.weeds;
            const double growingHours = plot.kind == CropKind::Berries ? 42.0 : 30.0;
            plot.growth = Clamp(plot.growth + elapsed * moistureFactor * weedFactor / growingHours, 0.0, 1.0);
        }
    }
    state_.hour += elapsed;
    if (state_.hunger <= 1e-10 || state_.energy <= 1e-10 || state_.warmth <= 1e-10)
    {
        if (state_.hunger <= 1e-10) state_.hunger = 0.0;
        if (state_.energy <= 1e-10) state_.energy = 0.0;
        if (state_.warmth <= 1e-10) state_.warmth = 0.0;
        state_.failed = true;
    }
}
void Simulation::Advance(double realSeconds, Point player, bool paused)
{
    if (paused || !FiniteRange(realSeconds, 0.0, 31536000.0)) return;
    AdvanceGameHours(realSeconds * 24.0 / (state_.dayMinutes * 60.0), player);
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
        Step(step, player, false);
        hours -= step;
    }
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
        Step(step, player, true);
        hours -= step;
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
         << state_.energy << ' ' << state_.warmth << ' ' << state_.failed << ' '
         << state_.warmOutfit << ' ' << state_.nextId << '\n';
    WriteStock(body, state_.inventory);
    body << state_.resources.size() << '\n';
    for (const auto& node : state_.resources)
        body << node.id << ' ' << static_cast<int>(node.kind) << ' ' << node.position.x << ' '
             << node.position.y << ' ' << node.readyAtHour << ' ' << node.cleared << '\n';
    body << state_.structures.size() << '\n';
    for (const auto& piece : state_.structures)
    {
        body << piece.id << ' ' << static_cast<int>(piece.kind) << ' ' << piece.cellX << ' '
             << piece.cellY << ' ' << piece.rotation << ' ' << piece.fuelHours;
        WriteStock(body, piece.storage);
    }
    body << state_.plots.size() << '\n';
    for (const auto& plot : state_.plots)
        body << plot.id << ' ' << plot.cellX << ' ' << plot.cellY << ' ' << plot.planted << ' '
             << plot.growth << ' ' << plot.moisture << ' ' << plot.weeds << ' ' << static_cast<int>(plot.kind) << '\n';
    const std::string payload = body.str();
    std::ostringstream output;
    output.imbue(std::locale::classic());
    output << "HOMESTEAD 3 " << payload.size() << ' ' << Checksum(payload) << '\n' << payload;
    return output.str();
}
Result Simulation::Deserialize(const std::string& data)
{
    const auto invalid = [] { return Bad("This save is incomplete, unsupported, or invalid. Your current game was not changed."); };
    if (data.empty() || data.size() > MaxSaveBytes) return invalid();
    const auto newline = data.find('\n');
    if (newline == std::string::npos || newline > 100) return invalid();
    std::istringstream header(data.substr(0, newline));
    header.imbue(std::locale::classic());
    std::string magic;
    int version = 0;
    std::uint64_t size = 0, checksum = 0;
    if (!(header >> magic >> version >> size >> checksum) || magic != "HOMESTEAD" || (version != 2 && version != 3)) return invalid();
    header >> std::ws;
    if (!header.eof()) return invalid();
    const std::string payload = data.substr(newline + 1);
    if (size != payload.size() || Checksum(payload) != checksum) return invalid();
    for (unsigned char c : payload) if (c > 127 || (c < 32 && c != '\n' && c != '\r' && c != '\t')) return invalid();
    std::istringstream input(payload);
    input.imbue(std::locale::classic());
    State candidate;
    if (!(input >> candidate.hour >> candidate.dayMinutes >> candidate.hunger >> candidate.energy >> candidate.warmth) ||
        !ReadBool(input, candidate.failed) || !ReadBool(input, candidate.warmOutfit) || !(input >> candidate.nextId)) return invalid();
    if (!FiniteRange(candidate.hour, 6.0, MaxHour) || !FiniteRange(candidate.dayMinutes, 1.0, 1440.0) ||
        !FiniteRange(candidate.hunger, 0.0, 100.0) || !FiniteRange(candidate.energy, 0.0, 100.0) ||
        !FiniteRange(candidate.warmth, 0.0, 100.0) || candidate.nextId < 1 ||
        candidate.nextId == std::numeric_limits<int>::max()) return invalid();
    const bool critical = candidate.hunger == 0 || candidate.energy == 0 || candidate.warmth == 0;
    if (critical != candidate.failed || !ReadStock(input, candidate.inventory)) return invalid();
    std::set<int> ids;
    const auto acceptId = [&](int id) { return id > 0 && id < candidate.nextId && ids.insert(id).second; };
    int count = 0;
    if (!(input >> count) || count < 0 || count > MaxObjects) return invalid();
    for (int i = 0; i < count; ++i)
    {
        ResourceNode node;
        int kind = 0;
        if (!(input >> node.id >> kind >> node.position.x >> node.position.y >> node.readyAtHour) ||
            !ReadBool(input, node.cleared)) return invalid();
        node.kind = static_cast<ResourceKind>(kind);
        if (!acceptId(node.id) || !ValidEnum(node.kind, ResourceKind::Count) || !ValidPoint(node.position) ||
            !FiniteRange(node.readyAtHour, 0.0, candidate.hour + Regrowth(node.kind)) ||
            (node.readyAtHour != 0.0 && node.readyAtHour < 6.0 + Regrowth(node.kind)) ||
            (node.cleared && node.readyAtHour != 0.0)) return invalid();
        candidate.resources.push_back(node);
    }
    std::set<std::tuple<int, int, int>> cells;
    std::set<Edge> edges;
    std::set<std::pair<int, int>> furniture;
    if (!(input >> count) || count < 0 || count > MaxObjects) return invalid();
    for (int i = 0; i < count; ++i)
    {
        Structure piece;
        int kind = 0;
        if (!(input >> piece.id >> kind >> piece.cellX >> piece.cellY >> piece.rotation >> piece.fuelHours) ||
            !ReadStock(input, piece.storage)) return invalid();
        piece.kind = static_cast<Piece>(kind);
        if (!acceptId(piece.id) || !ValidEnum(piece.kind, Piece::Count) || !ValidCell(piece.cellX, piece.cellY) ||
            piece.rotation < 0 || piece.rotation >= 4 ||
            !FiniteRange(piece.fuelHours, 0.0, MaxFuel) ||
            (piece.kind != Piece::Fire && piece.fuelHours != 0.0) ||
            (piece.kind != Piece::Chest && !Empty(piece.storage)) ||
            BlockedBySapling(candidate, piece.cellX, piece.cellY)) return invalid();
        if (EdgePiece(piece.kind))
        {
            if (!edges.insert(EdgeKey(piece.cellX, piece.cellY, piece.rotation)).second) return invalid();
        }
        else if (!cells.insert({piece.cellX, piece.cellY, kind}).second) return invalid();
        if (Furniture(piece.kind) && !furniture.insert({piece.cellX, piece.cellY}).second) return invalid();
        candidate.structures.push_back(piece);
    }
    for (const auto& piece : candidate.structures)
        if ((piece.kind == Piece::Roof || EdgePiece(piece.kind)) &&
            !HasPiece(candidate, Piece::Foundation, piece.cellX, piece.cellY)) return invalid();
    std::set<std::pair<int, int>> plots;
    if (!(input >> count) || count < 0 || count > MaxObjects) return invalid();
    for (int i = 0; i < count; ++i)
    {
        Plot plot;
        if (!(input >> plot.id >> plot.cellX >> plot.cellY) || !ReadBool(input, plot.planted) ||
            !(input >> plot.growth >> plot.moisture >> plot.weeds)) return invalid();
        if (version == 3)
        {
            int kind = -1;
            if (!(input >> kind)) return invalid();
            plot.kind = static_cast<CropKind>(kind);
        }
        // Version 2 only supported roots; its missing kind is an explicit schema migration.
        if (!acceptId(plot.id) || !ValidCell(plot.cellX, plot.cellY) || !ValidEnum(plot.kind, CropKind::Count) ||
            (!plot.planted && plot.kind != CropKind::Roots) ||
            !FiniteRange(plot.growth, 0.0, 1.0) || !FiniteRange(plot.moisture, 0.0, 1.0) ||
            !FiniteRange(plot.weeds, 0.0, 1.0) || (!plot.planted && plot.growth != 0.0) ||
            !plots.insert({plot.cellX, plot.cellY}).second ||
            BlockedBySapling(candidate, plot.cellX, plot.cellY)) return invalid();
        for (const auto& piece : candidate.structures)
            if (piece.cellX == plot.cellX && piece.cellY == plot.cellY) return invalid();
        candidate.plots.push_back(plot);
    }
    input >> std::ws;
    if (!input.eof()) return invalid();
    state_ = std::move(candidate);
    return Good("Homestead restored. No time passed while you were away.");
}
}
