#pragma once

#include "HomesteadWorldGeneration.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace Homestead
{
enum class Item : int
{
    Knife, Branch, Stone, Fiber, Berries, Roots, Flowers, Seeds,
    Hatchet, DiggingStick, WateringCan, Water, RoastedRoots, HerbedRoots,
    Timber, Firewood, Machete, Count
};
enum class ResourceKind : int { Branches, Stones, BerryBush, Roots, Flowers, Reeds, Sapling, ForestTree, Count };
enum class Recipe : int { Hatchet, DiggingStick, WateringCan, RoastedRoots, HerbedRoots, SplitFirewood, Count };
enum class Piece : int { Foundation, Wall, Doorway, Roof, Fire, Bed, Chest, Count };
enum class CropKind : int { Roots, Berries, Count };

constexpr int ItemCount = static_cast<int>(Item::Count);
constexpr double CellSize = 300.0;
constexpr int InventoryCapacity = 120;
constexpr int SimulationSaveVersion = 8;
// Version 7 saves predate the machete (one fewer item per stock) and cleared underbrush.
constexpr int LegacySimulationSaveVersion = 7;
constexpr double ChestReach = 280.0;
constexpr double MaxWorldCoordinate = 1000000.0;
constexpr int MaxResourceEdits = 16384;
constexpr int MaxUnderbrushEdits = 16384;
constexpr int MaxUnderbrushIndex = 4096;
constexpr int MaxWorldDrops = 512;
constexpr int TransientResourceIdBase = 1000000;

enum class WearableDefinition : int { LinenTunic, LinenApron, LeatherShoes, WovenFootwraps, Count };
enum class EquipmentSlot : int { Torso, Legs, Apron, Feet, Count };
enum class WearableOwner : int { Carried, Chest, Equipped, World };
enum class ResultCode : int { None, Invalid, StaleRevision, UnsupportedVersion, CorruptSave, Capacity, Unavailable };
constexpr int EquipmentSlotCount = static_cast<int>(EquipmentSlot::Count);

struct WearableDefinitionInfo
{
    WearableDefinition id;
    const char* key;
    const char* name;
    unsigned slots;
    bool dyeable;
    int fiberCost; // Zero means starter-only, not a free recipe.
};

struct WearableInstance
{
    int id = 0;
    WearableDefinition definition = WearableDefinition::LinenTunic;
    int dye = 0;
    WearableOwner owner = WearableOwner::Carried;
    int chestId = 0;
};

struct LayoutEntry
{
    int groupId = 0;
    Item item = Item::Knife;
    int quantity = 0;
    int wearableId = 0;
};
using InventoryLayout = std::vector<LayoutEntry>;

struct Point { double x = 0.0; double y = 0.0; };
using Inventory = std::array<int, ItemCount>;

struct Result
{
    bool ok = false;
    std::string message;
    ResultCode code = ResultCode::None;
    std::uint64_t revision = 0;
    explicit operator bool() const { return ok; }
};

struct RecipeIngredientAssessment
{
    Item item = Item::Branch;
    int have = 0;
    int need = 0;
    const char* source = "";
    bool met = false;
};

struct RecipeAssessment
{
    Recipe recipe = Recipe::Hatchet;
    Item output = Item::Count;
    int outputCount = 0;
    std::vector<RecipeIngredientAssessment> ingredients;
    Item retainedTool = Item::Count;
    bool retainedToolMet = true;
    bool stationRequired = false;
    bool stationMet = true;
    bool capacityMet = true;
    bool craftable = false;
    std::string blocker;
};

struct ResourceNode
{
    int id = 0;
    ResourceKind kind = ResourceKind::Branches;
    Point position;
    double readyAtHour = 0.0;
    bool cleared = false;
    Generation::GeneratedEntityKey key{};
};

struct ResourceEdit
{
    Generation::GeneratedEntityKey key{};
    bool cleared = false;
    double readyAtHour = 0.0;
};

// A decorative underbrush plant hacked away with the machete: the Unreal world's per-chunk
// generator index, which depends only on the world seed and chunk.
struct UnderbrushEdit
{
    Generation::ChunkCoord chunk{};
    int index = 0;
};
bool operator<(const UnderbrushEdit& a, const UnderbrushEdit& b);

struct Structure
{
    int id = 0;
    Piece kind = Piece::Foundation;
    int cellX = 0;
    int cellY = 0;
    int rotation = 0; // Quarter turns: 0=north (+Y), 1=east (+X), 2=south, 3=west.
    double fuelHours = 0.0;
    Inventory storage{};
    InventoryLayout layout;
};

struct Plot
{
    int id = 0;
    int cellX = 0;
    int cellY = 0;
    bool planted = false;
    double growth = 0.0;
    double moisture = 0.0;
    double weeds = 0.0;
    CropKind kind = CropKind::Roots;
};

struct WorldDrop
{
    int id = 0;
    Point position;
    Item item = Item::Count;
    int quantity = 0;
    int wearableId = 0;
};

struct State
{
    double hour = 6.0;
    double dayMinutes = 60.0;
    double hunger = 85.0;
    double energy = 100.0;
    double warmth = 90.0;
    bool failed = false;
    bool warmOutfit = false;
    int nextId = 1;
    Inventory inventory{};
    std::vector<ResourceNode> resources;
    std::vector<Structure> structures;
    std::vector<Plot> plots;
    std::vector<WorldDrop> worldDrops;
    int nextWearableId = 3;
    int nextGroupId = 1;
    std::vector<WearableInstance> wearables;
    std::array<int, EquipmentSlotCount> equipment{};
    InventoryLayout inventoryLayout;
    Generation::WorldDescriptor world{};
    Generation::ChunkCoord activeChunk{};
    std::vector<ResourceEdit> resourceEdits;
    std::vector<UnderbrushEdit> clearedUnderbrush; // Sorted, unique.
};

const WearableDefinitionInfo* GetWearableDefinition(WearableDefinition definition);
const char* WearableName(WearableDefinition definition);
const char* WearableDescription(WearableDefinition definition);
const char* DyeName(int dye);
const char* GarmentRequirements(WearableDefinition definition);
const char* ItemName(Item item);
const char* ResourceName(ResourceKind kind);
const char* RecipeName(Recipe recipe);
const char* PieceName(Piece piece);
const char* CropName(CropKind kind);
const char* RecipeRequirements(Recipe recipe);
const char* PieceRequirements(Piece piece);
double StreamX(double y);
bool IsNearWater(Point position);
Point CellCenter(int cellX, int cellY);

// Energy: time awake drains it slowly; work spends it. Work is refused when it would leave her
// below Reserve, so exertion alone never collapses her.
namespace Exertion
{
constexpr double AwakePerHour = 0.6;
constexpr double Reserve = 5.0;
constexpr double GatherEnergy = 0.5;
constexpr double ClearEnergy = 1.0;
constexpr double SaplingEnergy = 1.5;
constexpr double FellEnergy = 4.0;
constexpr double WoodyUnderbrushEnergy = 1.5;
constexpr double SoftUnderbrushEnergy = 0.8;
constexpr double CraftEnergy = 0.8;
constexpr double CookEnergy = 0.3;
constexpr double SplitFirewoodEnergy = 1.5;
constexpr double BuildEnergy = 1.5;
constexpr double GarmentEnergy = 0.8;
constexpr double TillEnergy = 2.0;
constexpr double PlantEnergy = 0.4;
constexpr double WaterEnergy = 0.4;
constexpr double WeedEnergy = 0.8;
constexpr double HarvestCropEnergy = 0.6;
constexpr double FillWaterEnergy = 0.3;
constexpr double FuelEnergy = 0.2;
}

struct PreparedWorldRegion
{
    Generation::WorldDescriptor world;
    std::array<const Generation::ChunkBaseline*, 9> chunks{};
};

class Simulation
{
public:
    Simulation();
    const State& GetState() const { return state_; }
    Result NewGame();
    Result NewGame(std::uint64_t seed);
    Result SetActiveWorldRegion(Point player,
        const PreparedWorldRegion* prepared = nullptr);
    Result ResolveGeneratedResource(const Generation::GeneratedEntityKey& key, ResourceNode& out) const;
    int Count(Item item) const;
    int UsedCapacity() const;
    int ChestUsedCapacity(int chestId) const;
    std::uint64_t GetRevision() const { return revision_; }
    const WearableInstance* GetWearable(int id) const;
    const InventoryLayout* GetLayout(int containerId) const;
    bool IsRaining() const;
    bool IsNight() const;
    int DayNumber() const;
    const char* SeasonName() const;
    bool IsSheltered(Point position) const;
    bool IsNearFire(Point position) const;
    RecipeAssessment AssessRecipe(Recipe recipe, Point player) const;
    bool CanHarvest(int nodeId) const;
    int FindNearestResource(Point position, double maxDistance) const;
    int FindNearestPlot(Point position, double maxDistance) const;
    int FindNearestStructure(Point position, Piece kind, double maxDistance) const;
    int FindNearestDrop(Point position, double maxDistance) const;

    Result Harvest(int nodeId, Point player);
    Result Clear(int nodeId, Point player);
    // Hack away one underbrush plant at `plant` (validated by the caller's world). Woody shrubs
    // yield a branch, soft growth yields fiber.
    Result ClearUnderbrush(Generation::ChunkCoord chunk, int index, bool woody, Point plant, Point player);
    bool IsUnderbrushCleared(Generation::ChunkCoord chunk, int index) const;
    Result Eat(Item item);
    Result EatGroup(int groupId, std::uint64_t expectedRevision);
    Result Craft(Recipe recipe, Point player);
    Result Place(Piece kind, int cellX, int cellY, int rotation, Point player);
    // Playtest kit: one of each early tool not already owned (carried or chested), a bed and two
    // storage chests in clear cells near `anchor` when none exist, and (for new games) seeds.
    Result GrantStarterKit(Point anchor, Point facing, bool includeSeeds);
    // Playtest aid: put `count` of an item in her pack if there is room.
    Result GrantItems(Item item, int count);
    Result Till(int cellX, int cellY, Point player);
    Result Plant(int plotId, Point player, CropKind kind = CropKind::Roots);
    Result Water(int plotId, Point player);
    Result Weed(int plotId, Point player);
    Result HarvestCrop(int plotId, Point player);
    Result FillWater(Point player);
    Result AddFuel(int structureId, Point player);
    Result Transfer(int chestId, Item item, int amount, Point player);
    Result EquipWearable(int id, std::uint64_t expectedRevision);
    Result UnequipWearable(int id, std::uint64_t expectedRevision);
    Result MoveWearable(int id, int destinationChestId, Point player, std::uint64_t expectedRevision);
    Result CraftGarment(WearableDefinition definition, Point player, std::uint64_t expectedRevision);
    Result RecolorWearable(int id, int dye, Point player, std::uint64_t expectedRevision);
    Result TransferGroup(int chestId, int groupId, int amount, bool toChest, Point player,
        std::uint64_t expectedRevision);
    Result SplitGroup(int containerId, int groupId, int amount, Point player, std::uint64_t expectedRevision);
    Result MergeGroups(int containerId, int sourceGroupId, int targetGroupId, Point player,
        std::uint64_t expectedRevision);
    Result ReorderEntry(int containerId, int index, int targetIndex, Point player, std::uint64_t expectedRevision);
    Result SplitHalf(int containerId, int groupId, Point player, std::uint64_t expectedRevision);
    Result SortPack(std::uint64_t expectedRevision);
    Result DropGroup(int groupId, int amount, Point position, Point player,
        std::uint64_t expectedRevision);
    Result DropWearable(int wearableId, Point position, Point player,
        std::uint64_t expectedRevision);
    Result PickUpDrop(int dropId, Point player);
    Result Sleep(double hours, Point player);
    Result SetDayMinutes(double minutes);
    void SetWarmOutfit(bool enabled);
    void Advance(double realSeconds, Point player, bool paused = false);
    void AdvanceGameHours(double hours, Point player);
    // Playtest aid: jump the clock forward to the next occurrence of hourOfDay (0-24) without
    // simulating the skipped interval, so needs, crops and fires are left as they were.
    void SkipToHourOfDay(double hourOfDay);
    Result SpendSprintEnergy(double realSeconds);
    // Whether she has the Energy for work costing `cost` (see Exertion); ok when she does.
    Result CheckExertion(double cost) const;
    // What harvesting or clearing a node would cost her.
    double HarvestCost(int nodeId) const;
    double ClearCost(int nodeId) const;

    std::string Serialize() const;
    Result Deserialize(const std::string& data);
    Result Deserialize(const std::string& data, Generation::WorldDescriptor expectedWorld);

private:
    State state_;
    std::uint64_t revision_ = 0;
    int nextResourceHandle_ = TransientResourceIdBase;
    bool TryAdjust(const Inventory& change);
    Result CheckRevision(std::uint64_t expectedRevision) const;
    Result CommitInventory(State&& candidate, const char* message);
    // Charges `cost` Energy when `done` succeeded.
    Result Exert(double cost, Result done);
    void Step(double hours, Point player, bool sleeping);
};
}
