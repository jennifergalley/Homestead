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
double FoodNutrition(Item item)
{
    switch (item)
    {
    case Item::Berries: return 12.0;
    case Item::RoastedRoots: return 28.0;
    case Item::HerbedRoots: return 38.0;
    default: return 0.0;
    }
}
Result CanEat(const State& state, Item item)
{
    if (state.failed) return Failed();
    if (FoodNutrition(item) == 0.0) return Bad("Eat berries, roasted roots, or herbed roots. Raw roots need cooking.");
    if (state.hunger >= 100.0) return Bad("You are already full. Save this food for later.");
    return Good("");
}
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
    case ResourceKind::ForestTree: return 0.0;
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
    case ResourceKind::ForestTree: return Items({{Item::Timber, 6}, {Item::Branch, 4}});
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
    case Recipe::SplitFirewood: return Items({{Item::Timber, -1}, {Item::Firewood, 4}});
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
const char* AcquisitionSource(Item item)
{
    switch (item)
    {
    case Item::Branch: return "Fallen branches";
    case Item::Stone: return "Loose stones";
    case Item::Fiber: return "Reeds near water";
    case Item::Roots: return "Wild roots";
    case Item::Flowers: return "Meadow herb patches";
    case Item::Timber: return "Mature trees with Hatchet";
    default: return "";
    }
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
bool RequiresHatchet(ResourceKind kind)
{
    return kind == ResourceKind::Sapling || kind == ResourceKind::ForestTree;
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
Result CheckBuildingResources(const State& state, int x, int y)
{
    const double left = static_cast<double>(x) * CellSize;
    const double bottom = static_cast<double>(y) * CellSize;
    Generation::ChunkCoord low, high;
    const auto lowStatus = Generation::ChunkAt(static_cast<std::int64_t>(left - 50),
        static_cast<std::int64_t>(bottom - 50), low);
    const auto highStatus = Generation::ChunkAt(static_cast<std::int64_t>(left + CellSize + 50),
        static_cast<std::int64_t>(bottom + CellSize + 50), high);
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
                const bool blocked = node.kind == ResourceKind::Sapling ?
                    Cell(node.position.x) == x && Cell(node.position.y) == y :
                    DistanceSquared(node.position, {Clamp(node.position.x, left, left + CellSize),
                        Clamp(node.position.y, bottom, bottom + CellSize)}) <= 50.0 * 50.0;
                if (blocked) return Bad("Fell the standing tree or clear the sapling before using this building cell.");
            }
        }
    return Good("");
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
constexpr unsigned Slot(EquipmentSlot slot) { return 1u << static_cast<int>(slot); }
constexpr WearableDefinitionInfo Wearables[] = {
    {WearableDefinition::LinenTunic, "linen-tunic", "Linen tunic",
        Slot(EquipmentSlot::Torso) | Slot(EquipmentSlot::Legs), true, 12},
    {WearableDefinition::LinenApron, "linen-apron", "Linen apron", Slot(EquipmentSlot::Apron), true, 6},
    {WearableDefinition::LeatherShoes, "legacy-laceup-shoes", "Leather shoes", Slot(EquipmentSlot::Feet), false, 0},
    {WearableDefinition::WovenFootwraps, "woven-footwraps", "Woven footwraps", Slot(EquipmentSlot::Feet), false, 8}
};
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
    for (int count : *stock) total += count;
    for (const auto& item : state.wearables) if (InContainer(item, container)) ++total;
    return total;
}
Result ContainerAccess(const State& state, int container, Point player)
{
    if (container == 0) return Good("");
    const auto* chest = Find(state.structures, container);
    if (!chest || chest->kind != Piece::Chest) return Bad("Choose an existing storage chest.");
    if (!Near(player, CellCenter(chest->cellX, chest->cellY), ChestReach))
        return Bad("Move within 280 cm of this chest.");
    return Good("");
}
bool CanAllocate(int next) { return next > 0 && next < std::numeric_limits<int>::max() - 1; }
int InventoryCategory(Item item)
{
    switch (item)
    {
    case Item::Knife:
    case Item::Hatchet:
    case Item::DiggingStick:
    case Item::WateringCan:
        return 0;
    case Item::Branch:
    case Item::Stone:
    case Item::Fiber:
    case Item::Timber:
    case Item::Firewood:
        return 1;
    case Item::Berries:
    case Item::Roots:
    case Item::Flowers:
    case Item::RoastedRoots:
    case Item::HerbedRoots:
        return 2;
    case Item::Seeds:
    case Item::Water:
        return 3;
    default:
        return 5;
    }
}

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
        if (!stock || !layout || !StockValid(*stock) || ContainerUsed(state, container) > InventoryCapacity)
            return {false, container == 0 ? "Not enough pack space." : "The chest does not have enough space.", ResultCode::Capacity};
        if (layout->size() > InventoryCapacity) return Bad("Inventory layout has too many entries.");
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
                    !ValidEnum(entry.item, Item::Count) || entry.quantity <= 0 || entry.quantity > InventoryCapacity)
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
    if (!(input >> count) || count < 0 || count > InventoryCapacity) return false;
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
    case WearableDefinition::LinenTunic: return "A fiber-worked tunic covering torso and legs. Cosmetic clothing; no warmth bonus.";
    case WearableDefinition::LinenApron: return "A separate apron worn over a linen tunic. Cosmetic clothing; no warmth bonus.";
    case WearableDefinition::LeatherShoes: return "Starter lace-up shoes with socks. Authored color; not craftable.";
    case WearableDefinition::WovenFootwraps: return "Fiber-woven footwear, worn instead of shoes. Authored color; no warmth bonus.";
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
            result[static_cast<int>(info.id)] = info.fiberCost > 0 ?
                std::to_string(info.fiberCost) + " Fiber; knife required; work fiber into cloth" : "Starter footwear; not craftable";
        return result;
    }();
    return GetWearableDefinition(definition) ? requirements[static_cast<int>(definition)].c_str() : "Unknown garment";
}
const char* ItemName(Item item)
{
    static const char* names[] = {"Knife", "Branch", "Stone", "Fiber", "Berries", "Roots",
        "Meadow herb", "Seeds", "Crude hatchet", "Digging stick", "Watering can", "Water",
        "Roasted roots", "Herbed roots", "Timber", "Firewood"};
    return ValidEnum(item, Item::Count) ? names[static_cast<int>(item)] : "Unknown item";
}
const char* ResourceName(ResourceKind kind)
{
    static const char* names[] = {"Fallen branches", "Loose stones", "Berry bush", "Wild roots",
        "Meadow herb", "Stream reeds", "Sapling", "Forest tree"};
    return ValidEnum(kind, ResourceKind::Count) ? names[static_cast<int>(kind)] : "Unknown resource";
}
const char* RecipeName(Recipe recipe)
{
    static const char* names[] = {"Crude hatchet", "Digging stick", "Watering can",
        "Roasted roots", "Herbed roots", "Split firewood"};
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
            if (kind == Recipe::RoastedRoots || kind == Recipe::HerbedRoots)
                result[i] += "; nearby fueled fire (no pot needed)";
            else if (kind == Recipe::SplitFirewood)
                result[i] += "; crude hatchet required";
            else
                result[i] += "; knife required";
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
        }
        return result;
    }();
    return ValidEnum(piece, Piece::Count) ? descriptions[static_cast<int>(piece)].c_str() : "Unknown structure";
}
double StreamX(double y) { return Generation::StreamCenterCm(y); }
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
Result Simulation::NewGame() { return NewGame(0); }
Result Simulation::NewGame(std::uint64_t seed)
{
    State candidate;
    candidate.world.seed = seed;
    candidate.inventory[static_cast<int>(Item::Knife)] = 1;
    candidate.inventoryLayout.push_back({candidate.nextGroupId++, Item::Knife, 1, 0});
    candidate.wearables = {
        {1, WearableDefinition::LinenTunic, 0, WearableOwner::Equipped, 0},
        {2, WearableDefinition::LeatherShoes, 0, WearableOwner::Equipped, 0}};
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
    if (original->definition == WearableDefinition::LinenTunic)
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
    if (!info || info->fiberCost <= 0 || !ValidPoint(player)) return Bad("Choose a craftable garment and valid location.");
    if (Count(Item::Knife) == 0) return Bad("Take your knife from storage to work fiber into clothing.");
    if (Count(Item::Fiber) < info->fiberCost)
        return Bad("Gather " + std::to_string(info->fiberCost - Count(Item::Fiber)) + " more Fiber first.");
    if (!CanAllocate(state_.nextWearableId) || state_.wearables.size() >= MaxObjects)
        return Bad("The homestead has reached its garment identity limit.");
    State candidate = state_;
    candidate.inventory[static_cast<int>(Item::Fiber)] -= info->fiberCost;
    candidate.wearables.push_back({candidate.nextWearableId++, definition, 0, WearableOwner::Carried, 0});
    return CommitInventory(std::move(candidate), "Made one garment from fiber.");
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
    if (IsNearWater(position)) return Bad("Choose dry ground for this item.");
    for (const auto& structure : state_.structures)
        if (Near(position, CellCenter(structure.cellX, structure.cellY), 100.0))
            return Bad("Keep dropped items clear of structures.");
    for (const auto& plot : state_.plots)
        if (Near(position, CellCenter(plot.cellX, plot.cellY), 100.0))
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
    if (IsNearWater(position)) return Bad("Choose dry ground for this garment.");
    for (const auto& structure : state_.structures)
        if (Near(position, CellCenter(structure.cellX, structure.cellY), 100.0))
            return Bad("Keep dropped garments clear of structures.");
    for (const auto& plot : state_.plots)
        if (Near(position, CellCenter(plot.cellX, plot.cellY), 100.0))
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
        if (ContainerUsed(candidate, 0) > InventoryCapacity - drop->quantity)
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
        (RequiresHatchet(node->kind) ? Count(Item::Hatchet) > 0 : Count(Item::Knife) > 0);
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
    if (node->kind == ResourceKind::ForestTree) return Clear(nodeId, player);
    if (!Near(player, node->position)) return Bad("Move closer to gather this resource.");
    if (node->readyAtHour > state_.hour) return Bad("Nothing to gather here.");
    if (node->kind == ResourceKind::Sapling && Count(Item::Hatchet) == 0)
        return Bad("Craft a crude hatchet before cutting a sapling.");
    if (node->kind != ResourceKind::Sapling && Count(Item::Knife) == 0)
        return Bad("Take your knife from storage before gathering.");
    const Inventory yield = Yield(node->kind);
    State candidate = state_;
    auto* updated = Find(candidate.resources, nodeId);
    updated->readyAtHour = state_.hour + Regrowth(node->kind);
    if (!SaveResourceEdit(candidate, *updated)) return Bad("The world has reached its 16384 persistent resource edit limit.");
    for (int i = 0; i < ItemCount; ++i) candidate.inventory[i] += yield[i];
    const std::string message = std::string("Gathered ") + ResourceName(node->kind) + ".";
    return CommitInventory(std::move(candidate), message.c_str());
}
Result Simulation::Clear(int nodeId, Point player)
{
    if (state_.failed) return Failed();
    auto* node = Find(state_.resources, nodeId);
    if (!node || node->cleared) return Bad("This patch has already been cleared.");
    if (!Near(player, node->position)) return Bad("Move closer to clear this patch.");
    if (RequiresHatchet(node->kind) && Count(Item::Hatchet) == 0)
        return Bad("Craft a crude hatchet before felling trees or clearing saplings.");
    if (!RequiresHatchet(node->kind) && Count(Item::Knife) == 0)
        return Bad("Take your knife from storage before clearing.");
    const Inventory yield = node->readyAtHour <= state_.hour ? Yield(node->kind) : Inventory{};
    State candidate = state_;
    auto* updated = Find(candidate.resources, nodeId);
    updated->cleared = true;
    updated->readyAtHour = 0.0;
    if (!SaveResourceEdit(candidate, *updated)) return Bad("The world has reached its 16384 persistent resource edit limit.");
    for (int i = 0; i < ItemCount; ++i) candidate.inventory[i] += yield[i];
    return CommitInventory(std::move(candidate), "Land cleared.");
}
Result Simulation::Eat(Item item)
{
    const auto allowed = CanEat(state_, item);
    if (!allowed) return allowed;
    if (!TryAdjust(Items({{item, -1}}))) return Bad(std::string("Gather or cook some ") + ItemName(item) + " first.");
    state_.hunger = std::min(100.0, state_.hunger + FoodNutrition(item));
    return Good(std::string("Ate ") + ItemName(item) + ".");
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
    candidate.hunger = std::min(100.0, candidate.hunger + FoodNutrition(item));
    const std::string message = std::string("Ate ") + ItemName(item) + ".";
    return CommitInventory(std::move(candidate), message.c_str());
}
Result Simulation::Craft(Recipe recipe, Point player)
{
    if (state_.failed) return Failed();
    if (!ValidEnum(recipe, Recipe::Count) || !ValidPoint(player)) return Bad("Choose a valid recipe and location.");
    const Inventory change = CraftChange(recipe);
    const bool cooking = recipe == Recipe::RoastedRoots || recipe == Recipe::HerbedRoots;
    if (cooking && !IsNearFire(player)) return Bad("Move beside a fueled cookfire to cook roots; no pot is needed.");
    if (recipe == Recipe::SplitFirewood && Count(Item::Hatchet) == 0)
        return Bad("Take your crude hatchet from storage to split firewood.");
    if (!cooking && recipe != Recipe::SplitFirewood && Count(Item::Knife) == 0)
        return Bad("Take your knife from storage to craft tools.");
    if (!TryAdjust(change)) return Bad(MissingMessage(change, state_.inventory));
    return Good(std::string("Made ") + RecipeName(recipe) + ".");
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
    assessment.stationRequired = cooking;
    assessment.stationMet = !cooking || IsNearFire(player);
    if (recipe == Recipe::SplitFirewood)
        assessment.retainedTool = Item::Hatchet;
    else if (!cooking)
        assessment.retainedTool = Item::Knife;
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
Result Simulation::Place(Piece kind, int cellX, int cellY, int rotation, Point player)
{
    if (state_.failed) return Failed();
    if (!ValidEnum(kind, Piece::Count) || !ValidCell(cellX, cellY))
        return Bad("Choose a valid structure and building cell.");
    rotation = ((rotation % 4) + 4) % 4;
    if (!Near(player, CellCenter(cellX, cellY), 700.0)) return Bad("Move closer to this building site.");
    if (state_.structures.size() >= MaxObjects || state_.nextId >= TransientResourceIdBase - 1)
        return Bad("The homestead has reached its structure limit.");
    const auto space = CheckBuildingResources(state_, cellX, cellY);
    if (!space) return space;
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
    for (Item tool : {Item::Knife, Item::Hatchet, Item::DiggingStick, Item::WateringCan})
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
    if (beds == 0) wanted.push_back(Piece::Bed);
    for (int i = chests; i < 2; ++i) wanted.push_back(Piece::Chest);
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
        for (const auto& piece : state_.structures) if (piece.cellX == x && piece.cellY == y) return true;
        for (const auto& plot : state_.plots) if (plot.cellX == x && plot.cellY == y) return true;
        return false;
    };
    int placed = 0;
    for (const auto& cell : cells)
    {
        if (placed == static_cast<int>(wanted.size())) break;
        const int x = cell.second.first, y = cell.second.second;
        if (used(x, y) || !CheckBuildingResources(state_, x, y)) continue;
        if (state_.structures.size() >= MaxObjects || state_.nextId >= TransientResourceIdBase - 1) break;
        state_.structures.push_back({state_.nextId++, wanted[placed], x, y, 2, 0.0, {}});
        ++placed;
    }
    ++revision_;
    return Good("Your starter tools, bed and storage chests are ready.");
}
Result Simulation::Till(int cellX, int cellY, Point player)
{
    if (state_.failed) return Failed();
    if (!ValidCell(cellX, cellY) || !Near(player, CellCenter(cellX, cellY))) return Bad("Move closer to a valid garden cell.");
    if (Count(Item::DiggingStick) == 0) return Bad("Craft a digging stick before tilling soil.");
    if (state_.plots.size() >= MaxObjects || state_.nextId >= TransientResourceIdBase - 1)
        return Bad("The garden has reached its plot limit.");
    const auto space = CheckBuildingResources(state_, cellX, cellY);
    if (!space) return space;
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
    return Good(berries ? "Planted berry seeds."
        : "Roots planted.");
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
    const Item fuel = Count(Item::Firewood) > 0 ? Item::Firewood : Item::Branch;
    if (!TryAdjust(Items({{fuel, -1}}))) return Bad("Carry firewood or a branch to fuel the fire.");
    fire->fuelHours += 4.0;
    return Good(fuel == Item::Firewood ? "Added firewood: four more hours of fire."
        : "Added a branch: four more hours of fire.");
}
Result Simulation::Transfer(int chestId, Item item, int amount, Point player)
{
    if (state_.failed) return Failed();
    auto* chest = Find(state_.structures, chestId);
    if (!chest || chest->kind != Piece::Chest) return Bad("Choose a storage chest.");
    if (!Near(player, CellCenter(chest->cellX, chest->cellY), ChestReach)) return Bad("Move within 280 cm of this chest.");
    if (!ValidEnum(item, Item::Count) || amount == 0 || amount < -InventoryCapacity || amount > InventoryCapacity)
        return Bad("Choose an item and a transfer amount between one and 120.");
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
void Simulation::SkipToHourOfDay(double hourOfDay)
{
    if (state_.failed || !FiniteRange(hourOfDay, 0.0, 24.0)) return;
    double target = std::floor(state_.hour / 24.0) * 24.0 + hourOfDay;
    if (target <= state_.hour) target += 24.0;
    if (target > MaxHour) return;
    state_.hour = target;
    ++revision_;
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
    body << state_.world.seed << ' ' << state_.world.generationVersion << ' '
         << state_.activeChunk.x << ' ' << state_.activeChunk.y << '\n';
    body << state_.resourceEdits.size() << '\n';
    for (const auto& edit : state_.resourceEdits)
        body << edit.key.chunk.x << ' ' << edit.key.chunk.y << ' ' << edit.key.localId << ' '
             << edit.cleared << ' ' << edit.readyAtHour << '\n';
    body << state_.structures.size() << '\n';
    for (const auto& piece : state_.structures)
    {
        body << piece.id << ' ' << static_cast<int>(piece.kind) << ' ' << piece.cellX << ' '
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
    for (int id : state_.equipment) body << id << ' ';
    body << '\n';
    WriteLayout(body, state_.inventoryLayout);
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
    if (version != SimulationSaveVersion) return {false,
        "This test save uses an incompatible version. Start a new woodland with this build; no save was changed.",
        ResultCode::UnsupportedVersion, revision_};
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
        candidate.nextId >= TransientResourceIdBase) return invalid();
    const bool critical = candidate.hunger == 0 || candidate.energy == 0 || candidate.warmth == 0;
    if (critical != candidate.failed || !ReadStock(input, candidate.inventory)) return invalid();
    std::set<int> ids;
    const auto acceptId = [&](int id) { return id > 0 && id < candidate.nextId && ids.insert(id).second; };
    int count = 0;
    std::uint64_t generationVersion = 0;
    if (!ReadUnsigned(input, candidate.world.seed) || !ReadUnsigned(input, generationVersion)) return invalid();
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
        Generation::GeneratedEntity entity;
        if (Generation::FindEntity(candidate.world, edit.key, entity) != Generation::Status::Ok) return invalid();
        ResourceNode node;
        if (!GeneratedNode(candidate, entity, node) ||
            !FiniteRange(edit.readyAtHour, 0.0, candidate.hour + Regrowth(node.kind)) ||
            (edit.readyAtHour != 0.0 && edit.readyAtHour < 6.0 + Regrowth(node.kind)) ||
            (edit.cleared && edit.readyAtHour != 0.0) ||
            (!edit.cleared && (edit.readyAtHour == 0.0 || node.kind == ResourceKind::ForestTree))) return invalid();
        candidate.resourceEdits.push_back(edit);
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
            !ReadStock(input, piece.storage) || !ReadLayout(input, piece.layout)) return invalid();
        piece.kind = static_cast<Piece>(kind);
        if (!acceptId(piece.id) || !ValidEnum(piece.kind, Piece::Count) || !ValidCell(piece.cellX, piece.cellY) ||
            piece.rotation < 0 || piece.rotation >= 4 ||
            !FiniteRange(piece.fuelHours, 0.0, MaxFuel) ||
            (piece.kind != Piece::Fire && piece.fuelHours != 0.0) ||
            (piece.kind != Piece::Chest && !Empty(piece.storage)) ||
            !CheckBuildingResources(candidate, piece.cellX, piece.cellY)) return invalid();
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
        int kind = -1;
        if (!(input >> kind)) return invalid();
        plot.kind = static_cast<CropKind>(kind);
        if (!acceptId(plot.id) || !ValidCell(plot.cellX, plot.cellY) || !ValidEnum(plot.kind, CropKind::Count) ||
            (!plot.planted && plot.kind != CropKind::Roots) ||
            !FiniteRange(plot.growth, 0.0, 1.0) || !FiniteRange(plot.moisture, 0.0, 1.0) ||
            !FiniteRange(plot.weeds, 0.0, 1.0) || (!plot.planted && plot.growth != 0.0) ||
            !plots.insert({plot.cellX, plot.cellY}).second ||
            !CheckBuildingResources(candidate, plot.cellX, plot.cellY)) return invalid();
        for (const auto& piece : candidate.structures)
            if (piece.cellX == plot.cellX && piece.cellY == plot.cellY) return invalid();
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
    for (int& id : candidate.equipment) if (!(input >> id)) return invalid();
    if (!ReadLayout(input, candidate.inventoryLayout)) return invalid();
    input >> std::ws;
    if (!input.eof()) return invalid();
    const auto inventory = ValidateInventory(candidate);
    if (!inventory) return {false, inventory.message + " Your current game was not changed.", ResultCode::CorruptSave, revision_};
    int nextHandle = nextResourceHandle_;
    const bool sameWorld = candidate.world.seed == state_.world.seed &&
        candidate.world.generationVersion == state_.world.generationVersion;
    const auto populated = Materialize(candidate, sameWorld ? &state_ : nullptr, nextHandle);
    if (!populated) return populated;
    state_ = std::move(candidate);
    nextResourceHandle_ = nextHandle;
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
