#pragma once

#include "HomesteadItems.h"
#include "HomesteadShops.h"
#include "HomesteadWorldGeneration.h"

#include <array>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace Homestead
{
struct EstateLayout;
struct EstatePlacements;

// Reeds and DeerRemains are retired: they no longer yield anything. The kinds from TallGrass on are
// the estate's overgrowth (see HomesteadOvergrowth.h) and its spring flowers.
enum class ResourceKind : int
{
    Branches, Stones, BerryBush, Roots, Flowers, Reeds, Sapling, ForestTree, DeerRemains,
    TallGrass, Weeds, BrambleThin, BrambleThicket, BrambleBank, FallenBranch,
    StumpSmall, StumpLarge, StumpAncient, FallenLog, GiantLog, Rubble, SmallRock, Boulder, SalvagePile,
    Primroses, Bluebells, WildDaffodils, WildGarlic,
    // add-coral-island-clearout: the manor clear-out's nettles, a middling stump and hand-cleared rubbish.
    Nettles, StumpMedium, BrokenCrate, BrokenBarrel, RubbishHeap, RottenPlanks,
    Count
};
// First tools are hafted by hand from a salvaged rusted head and two branches.
enum class Recipe : int { HaftAxe, HaftHoe, HaftScythe, HaftBillhook, HaftPickaxe, RoastedRoots, HerbedRoots, SplitFirewood, Count };
// One of each tool; its tier belongs to the tool type (State::toolTiers).
enum class ToolKind : int { Axe, Hoe, Pail, Scythe, Billhook, Pickaxe, Count };
enum class ToolTier : int { Worn, Iron, Steel, Master, Count };
constexpr int ToolKindCount = static_cast<int>(ToolKind::Count);
constexpr int ToolTierCount = static_cast<int>(ToolTier::Count);
// Hearth: the standing room's granite fireplace. It cooks like a cookfire and is always lit in
// round 1; it can't be built.
enum class Piece : int { Foundation, Wall, Doorway, Roof, Fire, Bed, Chest, Hearth, Count };
// How a building piece looks; the rules are the same. Stone is the old manor's granite masonry.
enum class StructureSkin : int { Timber, Stone, Count };
enum class CropKind : int { Roots, Berries, Count };

constexpr double CellSize = 300.0;
// Garden squares: each building cell holds 3 x 3 of them, and the middle one shares its centre.
constexpr int GardenCellsPerCell = 3;
constexpr double GardenCellSize = CellSize / GardenCellsPerCell;
constexpr int InventoryCapacity = 120;
constexpr int ChestCapacity = 1200;
constexpr int ContainerCapacity(int containerId) { return containerId > 0 ? ChestCapacity : InventoryCapacity; }
// 12 is the estate pivot: no warmth in the vitals, estate items, tool tiers, parcels, money and the manor.
// 13 writes each item stock (and the equipment slots) with its width first, so appending an Item or a
// slot no longer changes the save format. Enums that name data stay append-only.
constexpr int SimulationSaveVersion = 13;
// Version 12 wrote stocks without a width: exactly the writing build's item count (40 on every build
// of main that wrote version 12), and five equipment slots. The reader measures each stock's line.
constexpr int PositionalStockSaveVersion = 12;
constexpr int PositionalStockMinimumItems = 40;
constexpr int PositionalStockEquipmentSlots = 5;
// Version 11 files came from the woodland MVP and the round-1 test builds, whose item stocks
// changed width without a version bump, so they can't be read reliably and are refused.
constexpr int RetiredTestSaveVersion = 11;
// Saves before this had no fur (one fewer item per stock) and no outer-layer equipment slot.
constexpr int ClothingSaveVersion = 11;
// Saves before this kept every structure on the one world-aligned building grid.
constexpr int FreeBuildingSaveVersion = 10;
// Saves before this stored crop plots on whole building cells.
constexpr int GardenSquareSaveVersion = 9;
// Version 7 saves predate the machete (one fewer item per stock) and cleared underbrush.
constexpr int LegacySimulationSaveVersion = 7;
constexpr double ChestReach = 280.0;
constexpr double MaxWorldCoordinate = 1000000.0;
constexpr int MaxResourceEdits = 16384;
constexpr int MaxUnderbrushEdits = 16384;
constexpr int MaxUnderbrushIndex = 4096;
constexpr int MaxWorldDrops = 512;
constexpr int TransientResourceIdBase = 1000000;
// Fixed-estate placement ids live in [EstatePlacementIdBase, TransientResourceIdBase), apart from
// structure, plot and drop ids.
constexpr int EstatePlacementIdBase = 500000;
// Written in a save's world generation-version slot to mark a fixed-estate game.
constexpr std::uint32_t EstateWorldMarker = 0xE57A7Eu;

enum class WearableDefinition : int
{
    LinenTunic, LinenApron, LeatherShoes, WovenFootwraps,
    LinenShirt, LinenLongShirt, Trousers, FurCoat, FurBoots, WovenSandals, TurnShoes, Count
};
// Outer is a coat worn over whatever covers the torso.
enum class EquipmentSlot : int { Torso, Legs, Apron, Feet, Outer, Count };
enum class WearableOwner : int { Carried, Chest, Equipped, World };
// ToolTier: the target needs a better tool than hers; nothing changed and no energy was spent.
enum class ResultCode : int { None, Invalid, StaleRevision, UnsupportedVersion, CorruptSave, Capacity, Unavailable, ToolTier };
constexpr int EquipmentSlotCount = static_cast<int>(EquipmentSlot::Count);

struct WearableDefinitionInfo
{
    WearableDefinition id;
    const char* key;
    const char* name;
    unsigned slots;
    bool dyeable;
    int fiberCost; // Zero (with no fur) means starter-only, not a free recipe.
    int furCost = 0;
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
    Recipe recipe = Recipe::HaftAxe;
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
    // Tool tier the placement asks for on top of its kind's own minimum (fixed estate only).
    ToolTier minTier = ToolTier::Worn;
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

// A building's own grid, placed anywhere at any heading. Local cell (x, y) spans
// [x, x + 1] x [y, y + 1] CellSize units from `origin`, turned `yaw` degrees (Unreal yaw: +X toward
// +Y). Building 0 is the world-aligned grid (origin 0, yaw 0) that every structure used before
// version 10 saves; it is implicit and never stored.
struct Building
{
    int id = 0;
    Point origin;
    double yaw = 0.0;
};

struct Structure
{
    int id = 0;
    Piece kind = Piece::Foundation;
    int cellX = 0; // Cell in its building's grid.
    int cellY = 0;
    int rotation = 0; // Quarter turns within the building: 0=north (+Y), 1=east (+X), 2=south, 3=west.
    double fuelHours = 0.0;
    Inventory storage{};
    InventoryLayout layout;
    int buildingId = 0;
    StructureSkin skin = StructureSkin::Timber;
    // Part of the old manor (the standing room): not removable until the round-5 rebuild.
    bool heritage = false;
};

// A turned rectangle on the ground: centre, half extents along its own axes, Unreal yaw in degrees.
struct Footprint
{
    Point center;
    Point half;
    double yaw = 0.0;
};

// Where a piece would go. Snapped pieces join an existing building's grid; free-standing ones
// found a new building (buildingId -1) whose cell (0, 0) is centred on the aim point.
struct PlacementTarget
{
    Piece kind = Piece::Foundation;
    int buildingId = 0;
    int cellX = 0;
    int cellY = 0;
    int rotation = 0;
    Building frame;
    bool snapped = false;
    std::string blocker; // Set when no valid site could be resolved at all.
};

struct Plot
{
    int id = 0;
    // Garden-square coordinates (GardenCellSize), not building cells.
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

// A piece of estate land (see HomesteadParcels.h). The polygon comes from the level's landmark
// layout; only `owned` is saved, keyed by the stable `id` (the landmark polygon's name).
struct Parcel
{
    std::string id;
    std::vector<Point> polygon;
    bool owned = false;
    bool forSale = false;
};

struct State
{
    double hour = 6.0;
    double dayMinutes = 60.0;
    double hunger = 85.0;
    double energy = 100.0;
    bool failed = false;
    int nextId = 1;
    Inventory inventory{};
    std::vector<ResourceNode> resources;
    std::vector<Building> buildings; // Free-standing building grids; building 0 is implicit.
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
    // Round 1 fixed Cornish estate: resources come from the baked EstatePlacements (resource id =
    // placement id) instead of the seeded woodland generator, and there are no chunks.
    bool fixedEstate = false;
    int placementBakeVersion = 0;
    std::vector<Parcel> parcels; // Fixed estate only; empty in the seeded woodland.
    Cents money = 0; // HomesteadShops.h; changes only through Sell, Buy and playtest grants.
    std::vector<Shop> shops;
    // Every tool starts worn; the blacksmith (round 3) raises them.
    std::array<ToolTier, ToolKindCount> toolTiers{};
    // Chosen on the new-game Names page (UTF-8); empty on woodland games.
    std::string heroineName;
    std::string familyName;
    std::string estateName;
    // Field-book journal entries, oldest first, by key ("arrival"); see Manor::JournalTitle.
    std::vector<std::string> journal;
    // add-oil-lamp (HomesteadLamp.h): the lamp's oil in game hours, and whether she has had the
    // lamp kit (new estate games start with it; older saves get it once on load).
    double lampOilHours = 0.0;
    bool lampKitGranted = false;
};

const WearableDefinitionInfo* GetWearableDefinition(WearableDefinition definition);
const char* WearableName(WearableDefinition definition);
const char* WearableDescription(WearableDefinition definition);
const char* DyeName(int dye);
const char* GarmentRequirements(WearableDefinition definition);
const char* ResourceName(ResourceKind kind);
const char* RecipeName(Recipe recipe);
const char* PieceName(Piece piece);
const char* CropName(CropKind kind);
const char* RecipeRequirements(Recipe recipe);
const char* PieceRequirements(Piece piece);
// Whether the Build page offers the piece (the hearth belongs to the old house).
bool IsBuildable(Piece piece);
// Beds, chests, cookfires and the hearth: one per building cell, set inside it.
bool IsFurniture(Piece piece);
double StreamX(double y);
bool IsNearWater(Point position);
Point CellCenter(int cellX, int cellY);
int GardenCell(double value);
Point GardenCellCenter(int gardenX, int gardenY);
// Building cell that contains a garden square.
int GardenToCell(int garden);
// The garden square at the middle of a building cell.
inline int CellToGarden(int cell) { return cell * GardenCellsPerCell + GardenCellsPerCell / 2; }
Point PlotCenter(const Plot& plot);
bool PlotInCell(const Plot& plot, int cellX, int cellY);
// Rotates a vector by an Unreal yaw in degrees.
Point RotateYaw(Point value, double yaw);
// The building a structure belongs to; building 0 (the world grid) always resolves.
const Building* FindBuilding(const State& state, int buildingId);
Point BuildingCellCenter(const Building& building, int cellX, int cellY);
// A world point in the building's unrotated cell space (cm from its origin).
Point BuildingLocal(const Building& building, Point world);
// A structure's cell centre in the world, and the Unreal yaw of the piece itself.
Point StructureCenter(const State& state, const Structure& structure);
double StructureYaw(const State& state, const Structure& structure);
double PieceYaw(const Building& building, int rotation);
bool HasFoundation(const State& state, int buildingId, int cellX, int cellY);
// Where furniture sits inside a foundation cell (piece space); off a foundation it is centred.
Point FurnitureOffset(Piece kind);
// The ground a piece covers. Walls and doorways cover their edge; furniture covers only itself.
Footprint PieceFootprint(const Building& building, Piece kind, int cellX, int cellY, int rotation, bool onFoundation);
Footprint StructureFootprint(const State& state, const Structure& structure);
bool FootprintsOverlap(const Footprint& a, const Footprint& b);
// Energy: time awake drains it slowly; work spends it. Work is refused when it would leave her
// below Reserve. Nothing enforces a bedtime: sleep restores SleepPerHour for each hour slept at any
// hour, and only running Energy out forces rest, when she dozes off where she stands.
namespace Exertion
{
constexpr double AwakePerHour = 0.6;
constexpr double Reserve = 5.0;
constexpr double SleepPerHour = 10.0;
// Dozing off on the spot: DozeHours of rough sleep at the slower DozePerHour, so she wakes stiff
// and only part rested.
constexpr double DozeHours = 6.0;
constexpr double DozePerHour = 6.0;
constexpr double NapHours = 1.0;
constexpr double MinRestHours = 1.0;
constexpr double MaxRestHours = 10.0;
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
constexpr double DeconstructEnergy = 1.0;
}

// The spring weather: it rains on the second of every three days, RainStartHour to RainEndHour.
constexpr double RainStartHour = 9.0;
constexpr double RainEndHour = 15.0;
bool IsRainDay(double hour);
bool IsRainingAt(double hour);
// How hard it's raining at `hour`, 0-1 (add-rain-weather): nothing outside the rain window; inside
// it a drizzle (0.3) that swells into passing showers (up to 1) every hour and a half or so, easing
// in over the first quarter hour and out over the last ten minutes.
double RainAmount(double hour);
// Cloud cover at `hour`, 0-1: builds over the half hour before the rain and clears over the half
// hour after it, so the sky greys before a drop falls.
double Overcast(double hour);
constexpr double OvercastLeadHours = 0.5;

// What the bed offers (flexible-sleep): each choice with its length and the hour of day she'd wake.
enum class SleepChoice { UntilMorning, UntilRested, Nap };
struct SleepOption
{
    SleepChoice choice = SleepChoice::UntilRested;
    double hours = 0.0;
    double wakeHour = 0.0; // Hour of day, 0-24.
};
constexpr double MorningWakeHour = 6.75;
// The choices at `hour` with `energy`, the default first: "until morning" (06:45) in the evening and
// at night (18:00-05:00), "until rested" (her Energy deficit at SleepPerHour, a quarter hour up,
// MinRestHours to MaxRestHours; left out when it would wake her within 45 minutes of morning) and a
// NapHours nap (left out when "until rested" is already that short).
std::vector<SleepOption> SleepOptions(double hour, double energy);

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
    // Round 1: a new game on the fixed estate. Each round-1 lane seeds its part from `layout`
    // inside this call (parcels, the heritage standing room, shops, starting money and names).
    Result NewEstateGame(const EstateLayout& layout, const EstatePlacements& placements);
    // The fixed estate's anchors; the provisional layout until the game supplies the level's.
    const EstateLayout& Layout() const;
    void SetLayout(const EstateLayout& layout);
    // The heroine's, her family's and the estate's names (see HomesteadManor.h for the rules).
    Result SetNames(const std::string& heroine, const std::string& family, const std::string& estate);
    // The estate's name for HUD and toasts ("Trevennor"); "the estate" before one is chosen.
    std::string EstateName() const;
    // The baked placements a fixed-estate save is loaded against; set before Deserialize.
    void SetPlacements(const EstatePlacements& placements);
    // On the fixed estate, water comes from the level's authored water bodies (sea, estuary, river):
    // the game supplies the probe. Generated worlds keep the procedural stream test.
    void SetWaterProbe(std::function<bool(Point)> probe) { waterProbe_ = std::move(probe); }
    bool NearWater(Point position) const;
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
    int FindDeconstructTarget(Point aim, double maxDistance) const;

    Result Harvest(int nodeId, Point player);
    Result Clear(int nodeId, Point player);
    // Hack away one underbrush plant at `plant` (validated by the caller's world). Woody shrubs
    // yield a branch, soft growth yields fiber.
    Result ClearUnderbrush(Generation::ChunkCoord chunk, int index, bool woody, Point plant, Point player);
    bool IsUnderbrushCleared(Generation::ChunkCoord chunk, int index) const;
    Result Eat(Item item);
    Result EatGroup(int groupId, std::uint64_t expectedRevision);
    Result Craft(Recipe recipe, Point player);
    // Places on the world grid (building 0).
    Result Place(Piece kind, int cellX, int cellY, int rotation, Point player);
    // Build anywhere: aimed at one of your foundations a piece snaps onto its grid (foundations join
    // the nearest open side, walls take an edge, roofs and furniture take the aimed cell); aimed
    // anywhere else, foundations and furniture stand free at `freeYaw`. Walls and roofs always
    // need a foundation.
    PlacementTarget ResolvePlacement(Piece kind, Point aim, double freeYaw, int rotation) const;
    // Why the target cannot be built now, without spending anything. `quick` checks trees against
    // the loaded region instead of regenerating chunks (for a live preview).
    Result CheckPlacement(const PlacementTarget& target, Point player, bool quick = false) const;
    Result Place(const PlacementTarget& target, Point player);
    Result CheckDeconstruct(int structureId, Point player) const;
    // Take down a placed piece and get its whole build cost back. A chest's contents come with it;
    // anything that does not fit in the pack is set down beside her.
    Result Deconstruct(int structureId, Point player);
    // Whether she carries what the piece costs.
    Result CheckBuildCost(Piece kind) const;
    // Estate land (HomesteadParcels.cpp). The seeded woodland has no parcels and is all hers.
    const Parcel* ParcelAt(Point point) const;
    bool IsOwned(Point point) const;
    // Whether every corner of the target's footprint lies on owned land ("Outside your estate").
    Result CanBuildAt(const PlacementTarget& target) const;
    Point StructureCenter(const Structure& structure) const { return Homestead::StructureCenter(state_, structure); }
    // Playtest kit: one of each early tool not already owned (carried or chested), a bed and two
    // storage chests in clear cells near `anchor` when none exist, and (for new games) seeds.
    Result GrantStarterKit(Point anchor, Point facing, bool includeSeeds);
    // Playtest aid for woodland games: raise the heritage standing room centred on `centre`, its grid
    // turned `yaw` degrees, if nothing stands in the way.
    Result SeedStandingRoomAt(Point centre, double yaw);
    // Playtest aid: put `count` of an item in her pack if there is room.
    Result GrantItems(Item item, int count);
    // Till one garden square (garden coordinates, see GardenCell) with the stone hoe.
    Result Till(int cellX, int cellY, Point player);
    Result Plant(int plotId, Point player, CropKind kind = CropKind::Roots);
    Result Water(int plotId, Point player);
    Result Weed(int plotId, Point player);
    Result HarvestCrop(int plotId, Point player);
    Result FillWater(Point player);
    // Tip the water out of the pail (it stays in her pack, empty).
    Result EmptyPail();
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
    // How many times she has dozed off from exhaustion in this session (never saved); the game
    // compares it to tell her when she wakes.
    int DozeCount() const { return dozes_; }
    // Playtest aid: set her Energy (0-100).
    Result SetEnergy(double energy);
    Result SetDayMinutes(double minutes);

    // Overgrowth clearing (HomesteadOvergrowth.cpp). `tool` is the carried tool she swings, or
    // Item::Count for bare hands (salvage piles, fallen boughs).
    ToolTier GetToolTier(ToolKind tool) const;
    // Playtest aid until the blacksmith sells upgrades.
    Result SetToolTier(ToolKind tool, ToolTier tier);
    // Whether a swing of `tool` at the overgrowth node could clear it now, without changing
    // anything. Fails with ResultCode::ToolTier and "Needs an iron axe" when her tool is too worn.
    Result CheckOvergrowth(int nodeId, Item tool, Point player) const;
    // Swings her current tool tier needs to clear the node (the last one commits).
    int OvergrowthSwings(int nodeId) const;
    double OvergrowthCost(int nodeId) const;
    // Clears the node, spends energy once and grants its yield exactly once. Yield that doesn't
    // fit in her pack is left on the ground as a world drop.
    Result ClearOvergrowth(int nodeId, Item tool, Point player);
    // The uncleared overgrowth `tool` handles nearest to `position`, or -1.
    int FindNearestOvergrowth(Point position, double maxDistance, Item tool) const;
    // Grass and weeds whose centres lie in the scythe's forward arc (wider at higher tiers).
    std::vector<int> ScytheArcTargets(Point player, Point facing) const;
    static double ScytheArcRadius(ToolTier tier);
    static double ScytheArcHalfAngle(ToolTier tier);

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

    // Shops (HomesteadShops.cpp). Trading needs the shop open and her within CounterReach of its
    // counter; each is one transaction moving goods, money and the shop's stock of her goods.
    const Shop* FindShop(int shopId) const;
    const Shop* FindShop(ShopKind kind) const;
    Result CheckShopAccess(int shopId, Point player) const;
    Result Sell(int shopId, Item item, int quantity, Point player);
    // Buys the shop's own goods, or with `fromHeroineStock` her own sold goods back.
    Result Buy(int shopId, Item item, int quantity, bool fromHeroineStock, Point player);
    // Counts a shopkeeper greeting (a friendship stub).
    Result GreetShopkeeper(int shopId);
    // Playtest aids: adjust the purse; open (or move) a shop with its counter at `counter`.
    Result GrantMoney(Cents cents);
    Result PlaceShop(ShopKind kind, Point counter, double yaw = 0.0);
    // Oil lamp (HomesteadLamp.cpp). One lamp, one reservoir of oil wherever the lamp is.
    double LampOil() const { return state_.lampOilHours; }
    // Playtest aid: set the lamp's oil (clamped to its capacity).
    void SetLampOil(double hours);
    // Whether she is holding the lamp out (the selected hotbar tool); set by the game each frame.
    void SetLampInHand(bool inHand) { lampInHand_ = inHand; }
    bool IsLampInHand() const { return lampInHand_ && Count(Item::OilLamp) > 0; }
    // The lamp she set down, if any (a world drop of the lamp).
    const WorldDrop* SetDownLampDrop() const;
    // Lit in her hand (selected, awake) or set down, while it has oil.
    bool IsLampLit(bool sleeping = false) const;
    // Spends a flask to fill the lamp in her pack.
    Result RefillLamp();
    // Sets the carried lamp on dry ground within reach; it keeps burning there.
    Result SetDownLamp(Point position, Point player);
    std::string Serialize() const;
    Result Deserialize(const std::string& data);
    Result Deserialize(const std::string& data, Generation::WorldDescriptor expectedWorld);

private:
    State state_;
    std::shared_ptr<const EstateLayout> layout_;
    std::shared_ptr<const EstatePlacements> placements_;
    std::function<bool(Point)> waterProbe_;
    std::uint64_t revision_ = 0;
    int nextResourceHandle_ = TransientResourceIdBase;
    int dozes_ = 0;
    bool TryAdjust(const Inventory& change);
    Result CheckRevision(std::uint64_t expectedRevision) const;
    // CheckPlacement without the reach and exertion rules (the starter kit places from afar).
    Result CheckSite(const PlacementTarget& target, bool quick) const;
    Result CommitInventory(State&& candidate, const char* message);
    // Charges `cost` Energy when `done` succeeded.
    Result Exert(double cost, Result done);
    // Advances the world up to `hours` and returns the hours actually passed (less when hunger runs
    // out, or awake Energy reaches zero). Sleeping restores `recoveryPerHour` Energy an hour.
    double Step(double hours, Point player, bool sleeping, double recoveryPerHour = Exertion::SleepPerHour);
    // Out of Energy: she sleeps where she stands for DozeHours at DozePerHour. Returns hours slept.
    double DozeOff(Point player);
    // Shops (HomesteadShops.cpp).
    static void SeedEstateShops(State& candidate, const EstateLayout& layout);
    static void RefreshShopCounters(State& candidate, const EstateLayout& layout);
    void SellDownShops();
    void WriteEconomy(std::ostream& body) const;
    static bool ReadEconomy(std::istream& input, State& candidate, std::set<int>& ids);
    // Once a day at the 6 AM rollover: cleared grass and weeds near remaining overgrowth may regrow.
    void CreepWeeds(int day);
    // Oil lamp (HomesteadLamp.cpp).
    bool lampInHand_ = false;
    void BurnLamp(double hours, bool sleeping);
    // Gives an estate game the lamp kit once (lamp, full, and flasks) when the pack has room.
    void GrantLampKit();
};
}
