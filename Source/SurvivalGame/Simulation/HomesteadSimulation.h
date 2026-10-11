#pragma once

#include "HomesteadCalendar.h"
#include "HomesteadDaylight.h"
#include "HomesteadItems.h"
#include "HomesteadFishing.h"
#include "HomesteadRain.h"
#include "HomesteadShops.h"
#include "HomesteadWorldGeneration.h"

#include <array>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <memory>
#include <optional>
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
    // Slate slid off the manor's roofs, heaped in and round the ruin; cleared by hand.
    SlateHeap,
    // The manor's fallen roof timbers, chopped up with the worn axe.
    RuinTimbers,
    Count
};
// First tools are hafted by hand from a salvaged rusted head and two branches.
enum class Recipe : int
{
    HaftAxe, HaftHoe, HaftScythe, HaftBillhook, HaftPickaxe, RoastedRoots, HerbedRoots, SplitFirewood,
    // add-basic-crop-cookfire-recipes.
    RoastedTurnips, StewedCarrots, BakedPotatoes, HerbedBroadBeans,
    CabbagePotatoStew, BerryCompote, StrawberryCompote, RootVegetableHotpot,
    RawFishSlices, GrilledTrout, GrilledPerch, GrilledMackerel,
    FishSoup, FishAndPotatoes, HerbedCarp, MackerelChowder,
    Count
};
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
enum class CropKind : int
{
    Roots, Berries,
    // improve-crops-and-harvest: period crops. Append only (plots save the kind as an int).
    Turnips, Carrots, Potatoes, Cabbage, BroadBeans, Strawberries,
    Count
};

constexpr double CellSize = 300.0;
// Garden squares: each building cell holds 3 x 3 of them, and the middle one shares its centre.
constexpr int GardenCellsPerCell = 3;
constexpr double GardenCellSize = CellSize / GardenCellsPerCell;
constexpr int InventoryCapacity = 120;
constexpr int ChestCapacity = 1200;
// Her pack holds InventoryCapacity until she buys the leather backpack (HomesteadBackpack.h), then
// MaxPackCapacity. Load-time validation that doesn't know her state yet (drops, saved stock) uses the
// maximum; stacks set on the ground are never larger than her own PackCapacity, so she can pick them up.
constexpr int MaxPackCapacity = 240;
struct State;
int PackCapacity(const State& state);
int ContainerCapacity(const State& state, int containerId);
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
    LinenShirt, LinenLongShirt, Trousers, FurCoat, FurBoots, WovenSandals, TurnShoes,
    ScoopTank, CropTop, SkinnyJeans, Leggings, BikerJacket, Sneakers, AnkleBoots, DarkJeans, Count
};
// Outer is a coat worn over whatever covers the torso.
enum class EquipmentSlot : int { Torso, Legs, Apron, Feet, Outer, Count };
enum class WearableOwner : int { Carried, Chest, Equipped, World };
// ToolTier: the target needs a better tool than hers; nothing changed and no energy was spent.
// UnsupportedVersion: a save from an older build this one can't read. NewerBuild: a save written by a
// newer build (a later version, wider item stocks or a section this build doesn't know); the game
// must leave it untouched so that build can still open it.
// PackOverflow: it worked, but what didn't fit in her pack was left on the ground (worth a notice).
enum class ResultCode : int { None, Invalid, StaleRevision, UnsupportedVersion, CorruptSave, Capacity, Unavailable, ToolTier, NewerBuild, PackOverflow };
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

// The hotbar is the first row of her pack (HomesteadPackRow.h): ten cells, each naming one of her
// carried layout entries (a stack's group id, or a garment's id) or empty.
constexpr int PackRowSize = 10;
struct PackRowCell
{
    int groupId = 0;
    int wearableId = 0;
    bool Empty() const { return groupId == 0 && wearableId == 0; }
    bool operator==(const PackRowCell& other) const { return groupId == other.groupId && wearableId == other.wearableId; }
    bool operator!=(const PackRowCell& other) const { return !(*this == other); }
};
using PackRow = std::array<PackRowCell, PackRowSize>;

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

// A decorative (scenery) tree she has felled, keyed by its trunk position in whole centimetres
// (HomesteadTreeFelling.h): the game hour it fell, whether it grows back, and whether she has since
// cleared its permanent stump to bare ground.
struct FelledTree
{
    int xCm = 0;
    int yCm = 0;
    double fellHour = 0.0;
    bool regrows = false;
    bool cleared = false;
};

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
    // A chest's name as she gave it (trimmed UTF-8), empty for the default (HomesteadChests.h).
    std::string customName;
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
    // A regrowing crop (beans, strawberries) picked at least once since sowing: its status counts the
    // regrowth days ("ripening again, day 1 of 3") instead of the first growth. Saved in the optional
    // "picked" section; older saves default to false.
    bool picked = false;
    // Still planted when its crop's seasons ended (Crops::WitherOutOfSeason): it keeps its kind for
    // the dead plant's look, yields nothing and grows no further until the hoe clears it. Saved in
    // the optional "withered" section.
    bool withered = false;
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
    // Real minutes per game day. New games start at 60 ("Balanced"), long enough to walk to town
    // and back while the shops are open; Settings offers 30, 60 and 120, and saves keep their own.
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
    PackRow packRow{};
    // Rows of her pack rotated out of the hotbar (R / LT, RotatePackRow), each kept as it was, gaps and
    // all, so its stacks come back to the same number keys. Cells may name stacks since used up or
    // moved; RotatePackRow drops those as the row comes back. Saved in the optional "packrowsparked" section.
    std::vector<PackRow> parkedRows;
    // Where each stack below the hotbar sits in her pack's grid (place-items-in-exact-slots): slot i
    // names a carried entry, or is an empty gap. Empty until she first drops something on a slot
    // (MoveToPackSlot); until then the grid is the layout's order, packed. Cells naming stacks since used
    // up or moved stay gaps; new stacks take the first gap. Saved in the optional "packslots" section.
    std::vector<PackRowCell> packSlots;
    Generation::WorldDescriptor world{};
    Generation::ChunkCoord activeChunk{};
    std::vector<ResourceEdit> resourceEdits;
    std::vector<UnderbrushEdit> clearedUnderbrush; // Sorted, unique.
    // Round 1 fixed estate: resources come from the baked EstatePlacements (resource id =
    // placement id) instead of the seeded woodland generator, and there are no chunks.
    bool fixedEstate = false;
    int placementBakeVersion = 0;
    std::vector<Parcel> parcels; // Fixed estate only; empty in the seeded woodland.
    Coins money = 0; // HomesteadShops.h; changes only through Sell, Buy and playtest grants.
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
    // The leather backpack (HomesteadBackpack.h): bought once, doubling her pack; and whether it
    // shows on her back (a look only: it never changes capacity).
    bool leatherBackpack = false;
    bool backpackShown = true;
    // Estate only (HomesteadFood.h): the game hour her Well fed runs out; she is Well fed while hour is
    // below it. Saved in the optional "wellfed" section only while active.
    double wellFedUntilHour = 0.0;
    // Sorted destination ids beyond the initially visited Manor, in the optional "travel" section.
    std::vector<int> discoveredTravel;
    // Scenery trees felled with the axe and not yet regrown, sorted by (xCm, yCm), in the optional "felled"
    // section (HomesteadTreeFelling.h).
    std::vector<FelledTree> felledTrees;
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
// What a piece costs to build, and whether it stands on a foundation (walls, doorways and roofs).
Inventory PieceCost(Piece piece);
bool PieceNeedsFoundation(Piece piece);
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
// Energy: time awake drains it slowly; work spends it. On the estate she keeps walking at zero
// Energy rather than fainting, and food or bed rest restores it.
namespace Exertion
{
constexpr double AwakePerHour = 0.6;
constexpr double Reserve = 5.0;
constexpr double SleepPerHour = 10.0;
// Dozing off on the spot: DozeHours of rough sleep at the slower DozePerHour, so she wakes stiff
// and only part rested.
constexpr double DozeHours = 6.0;
constexpr double DozePerHour = 6.0;
constexpr double MinRestHours = 0.25;
constexpr double MinDawnSleepHours = 1e-6; // Tiny positive intervals prevent a zero-time "sleep" just before 06:00.
constexpr double MaxRestHours = 10.0;
constexpr double GatherEnergy = 0.5;
constexpr double ClearEnergy = 1.0;
constexpr double SaplingEnergy = 1.5;
constexpr double FellEnergy = 4.0;
// Clearing a near-home stump to bare ground with a worn axe (Balance, docs/design/balance.md §6): the same
// as the small stump the axe clears, ~3 energy, scaled by the axe's tier like any overgrowth.
constexpr double ClearStumpEnergy = 3.0;
constexpr double WoodyUnderbrushEnergy = 1.5;
constexpr double SoftUnderbrushEnergy = 0.8;
constexpr double CraftEnergy = 0.8;
constexpr double CookEnergy = 0.3;
constexpr double SplitFirewoodEnergy = 1.5;
constexpr double BuildEnergy = 1.5;
constexpr double GarmentEnergy = 0.8;
// Sprinting costs no Energy of its own; below 25 she walks, below 10 she walks at 75% speed.
constexpr double SprintFloor = 25.0;
constexpr double SlowWalkFloor = 10.0;
constexpr double SlowWalkFactor = 0.75;
inline double WalkSpeedFactor(double energy) { return energy < SlowWalkFloor ? SlowWalkFactor : 1.0; }
constexpr double TillEnergy = 2.0;
constexpr double PlantEnergy = 0.4;
constexpr double WaterEnergy = 0.4;
constexpr double WeedEnergy = 0.8;
constexpr double HarvestCropEnergy = 0.6;
constexpr double FillWaterEnergy = 0.3;
constexpr double FuelEnergy = 0.2;
constexpr double DeconstructEnergy = 1.0;
}

// Hunger belongs to the seeded woodland only: there it drains at these rates and at 0 fails her (she
// retries a checkpoint). The estate has no hunger (HomesteadFood.h): `State::hunger` stays at 100 and
// is still saved, until the next planned save-version bump drops it.
namespace Hunger
{
constexpr double AwakePerHour = 2.0;
constexpr double AsleepPerHour = 1.3;
}

// The weather lives in Simulation/HomesteadRain.h (included above): rain spells at any hour, from the clock alone.
// The rain loop's volume multiplier (UHomesteadWeather applies it once, after a fade-in to full): rain
// strength^0.7 times the Ambience setting, 0.9 outdoors and 0.35 indoors (0 outdoors .. 1 indoors),
// times RainLoudness. Jenny, 2026-09-29: the rain was too loud, so RainLoudness halves it (-6 dB).
constexpr double RainOutdoorGain = 0.9;
constexpr double RainIndoorGain = 0.35;
constexpr double RainLoudness = 0.5;
double RainAudioGain(double rain, double ambience, double indoors);
// Whether the loop plays at all: judged before RainLoudness, so it starts and stops at the same moments.
bool RainAudible(double rain, double ambience, double indoors);

// One bed action at a time: pass the night, or restore Energy during the day.
enum class SleepChoice { UntilMorning, UntilRested };
struct SleepOption
{
    SleepChoice choice = SleepChoice::UntilRested;
    double hours = 0.0;
    double wakeHour = 0.0; // Hour of day, 0-24.
};
constexpr double MorningWakeHour = Daylight::MorningSleepHour;
// Evening starts at the earlier of 18:00 and sunset; wake at the earlier of 06:00 and sunrise.
std::optional<SleepOption> BedSleepOption(double hour, double energy,
    double sunrise = Daylight::SunriseHour, double sunset = Daylight::SunsetHour);

struct PreparedWorldRegion
{
    Generation::WorldDescriptor world;
    std::array<const Generation::ChunkBaseline*, 9> chunks{};
};

// Where the public road leads (HomesteadTravel.h).
enum class TravelDestination : int;

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
    Result DiscoverTravel(TravelDestination destination, Point player);
    // The estate's name for HUD and toasts ("Larkhollow"); "the estate" before one is chosen.
    std::string EstateName() const;
    // The baked placements a fixed-estate save is loaded against; set before Deserialize.
    void SetPlacements(const EstatePlacements& placements);
    // On the fixed estate, water comes from the level's authored water bodies (sea, estuary, river):
    // the game supplies the probe. Generated worlds keep the procedural stream test.
    void SetWaterProbe(std::function<bool(Point)> probe) { waterProbe_ = std::move(probe); }
    bool NearWater(Point position) const;
    void SetFishingWaterProbe(std::function<FishingWater(Point)> probe) { fishingWaterProbe_ = std::move(probe); }
    // Test seam: the chance a hooked fish gets away (default Fishing::EscapeChance); clamped to 0-1.
    void SetFishingEscapeChance(double chance);
    FishingWater FishingWaterAt(Point player) const;
    const FishingSession& FishingCast() const { return fishing_; }
    Result CheckFishing(Point player) const;
    Result BeginFishing(Point player);
    Result AdvanceFishing(double seconds, Point player);
    Result FishingPress(Point player);
    Result FishingAnimationContact(FishingContact contact, std::uint64_t token, Point player);
    Result FishingAnimationInterrupted(std::uint64_t token);
    Result CancelFishing();
    // True when a pail waits in a chest or other storage (and so can be fetched to fill).
    bool PailStored() const;
    Result SetActiveWorldRegion(Point player,
        const PreparedWorldRegion* prepared = nullptr);
    Result ResolveGeneratedResource(const Generation::GeneratedEntityKey& key, ResourceNode& out) const;
    int Count(Item item) const;
    int UsedCapacity() const;
    // What her pack holds now: InventoryCapacity, or MaxPackCapacity with the leather backpack.
    int PackCapacity() const;
    int ChestUsedCapacity(int chestId) const;
    std::uint64_t GetRevision() const { return revision_; }
    const WearableInstance* GetWearable(int id) const;
    const InventoryLayout* GetLayout(int containerId) const;
    bool IsRaining() const;
    bool IsNight() const;
    // The calendar date now (HomesteadCalendar.h): "Mon, Spring 12".
    Calendar::Date Today() const { return Calendar::DateAt(state_.hour); }
    // Day of the season, 1-28.
    int DayNumber() const;
    const char* SeasonName() const;
    // What the last season change did, for the game's toast. Step fills it at each season rollover;
    // SeasonChanges() counts them this session (never saved), so the game compares the count.
    struct SeasonChange
    {
        Season from = Season::Spring;
        Season to = Season::Spring;
        int witheredPlots = 0;
    };
    int SeasonChanges() const { return seasonChanges_; }
    const SeasonChange& LastSeasonChange() const { return lastSeasonChange_; }
    // Well fed after a Meal on the estate (HomesteadFood.h), and what work costing `base` costs her
    // now: base x Food::WellFedWorkFactor while Well fed, otherwise base.
    bool IsWellFed() const;
    double WorkCost(double base) const;
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
    // Fells a decorative scenery tree whose trunk stands at `tree` (HomesteadTreeFelling.cpp); the Unreal
    // world supplies the position, the rules live there.
    Result FellSceneryTree(Point tree, Point player);
    // Why she can't fell it now (no axe, too far, too tired, already down, protected), or ok. Spends nothing.
    Result CheckFellSceneryTree(Point tree, Point player) const;
    // Clears the permanent stump a near-home tree leaves (HomesteadTreeFelling.cpp) to bare ground with
    // the axe, for firewood and kindling; the stump never returns. `tree` is the felled trunk's position.
    Result ClearSceneryStump(Point tree, Point player);
    // Why she can't clear it now (no axe, too far, no stump, a stump that grows back, too tired), or ok.
    Result CheckClearSceneryStump(Point tree, Point player) const;
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
    // Side-effect-free: would Till, Water or Weed succeed now? The same refusal, or ok (the world's garden
    // outline shows it before she acts). Till/Water/Weed call these first.
    Result CheckTill(int cellX, int cellY, Point player) const;
    // CheckTill's checks on the ground alone (in reach, free of buildings, resources, spoiling overgrowth and
    // plots, under the plot limit), without the hoe or her energy: whether the square could be tilled.
    Result CheckTillGround(int cellX, int cellY, Point player) const;
    Result CheckWater(int plotId, Point player) const;
    Result CheckWeed(int plotId, Point player) const;
    // Whether Plant(plotId, player, kind) would sow now, with its refusal, changing nothing.
    Result CheckSow(int plotId, Point player, CropKind kind) const;
    Result Plant(int plotId, Point player, CropKind kind = CropKind::Roots, int seedGroupId = 0);
    Result Water(int plotId, Point player);
    Result Weed(int plotId, Point player);
    // Hoes a withered plant out, back to tilled soil (needs the hoe).
    Result ClearWithered(int plotId, Point player);
    // Whether ClearWithered would succeed now, changing nothing.
    Result CheckClearWithered(int plotId, Point player) const;
    Result HarvestCrop(int plotId, Point player);
    Result FillWater(Point player);
    // Tip the water out of the pail (it stays in her pack, empty).
    Result EmptyPail();
    Result AddFuel(int structureId, Point player);
    Result Transfer(int chestId, Item item, int amount, Point player);
    // Wears a carried garment; whatever it displaces (and an apron left with no top) goes to her pack.
    Result EquipWearable(int id, std::uint64_t expectedRevision);
    // As above, or straight from the reachable chest holding it; then what it displaces goes into that chest.
    Result EquipWearable(int id, Point player, std::uint64_t expectedRevision);
    Result UnequipWearable(int id, std::uint64_t expectedRevision);
    Result MoveWearable(int id, int destinationChestId, Point player, std::uint64_t expectedRevision);
    Result CraftGarment(WearableDefinition definition, Point player, std::uint64_t expectedRevision);
    Result RecolorWearable(int id, int dye, Point player, std::uint64_t expectedRevision);
    Result TransferGroup(int chestId, int groupId, int amount, bool toChest, Point player,
        std::uint64_t expectedRevision);
    // Auto-store: in one step, moves her carried goods onto stacks of the same item already in this
    // chest (HomesteadChests.h AutoStores: never tools, the lamp, water or garments). Stacks below the
    // hotbar row go first, then row cells, as far as the chest has room; the rest stays with her.
    Result StoreMatching(int chestId, Point player, std::uint64_t expectedRevision);
    // Names a chest (trimmed; empty puts back the default name). Nothing else changes.
    Result RenameChest(int chestId, const std::string& name, Point player, std::uint64_t expectedRevision);
    Result SplitGroup(int containerId, int groupId, int amount, Point player, std::uint64_t expectedRevision);
    Result MergeGroups(int containerId, int sourceGroupId, int targetGroupId, Point player,
        std::uint64_t expectedRevision);
    Result ReorderEntry(int containerId, int index, int targetIndex, Point player, std::uint64_t expectedRevision);
    Result SplitHalf(int containerId, int groupId, Point player, std::uint64_t expectedRevision);
    // Sorts her pack below the hotbar row; the row stays as she arranged it.
    Result SortPack(std::uint64_t expectedRevision);
    // The hotbar row (HomesteadPackRow.h). Puts one of her carried stacks (or garment `wearableId`),
    // from the row or below it, in `cell`: onto an empty cell it moves, onto the same item it
    // merges, onto anything else the two swap places.
    Result MoveToPackRow(int groupId, int wearableId, int cell, std::uint64_t expectedRevision);
    // Moves what is in `cell` below the row: onto that stack (or garment) there, merging with the
    // same item or else swapping; with no target (0, 0) to the end of her pack.
    Result MoveFromPackRow(int cell, int targetGroupId, int targetWearableId, std::uint64_t expectedRevision);
    // Puts one of her carried stacks (or garment `wearableId`), from the hotbar row or below it, in
    // `slot` of her pack's grid below the row (HomesteadPackRow.h): onto an empty slot it moves, onto
    // the same item it merges, onto anything else the two swap places. Gaps stay where they are.
    Result MoveToPackSlot(int groupId, int wearableId, int slot, std::uint64_t expectedRevision);
    // The hotbar steps on to the next row of her pack, as in Coral Island: the first ten stacks below
    // the row become the row, in order, and the row's stacks go to the end of her pack in cell order,
    // so pressing again carries each row of her pack through the hotbar in turn.
    Result RotatePackRow(std::uint64_t expectedRevision);
    // Takes `amount` of a chest stack straight into `cell` in one step: onto the same item it
    // merges, otherwise it becomes that cell's stack and whatever was there moves below the row.
    Result TransferGroupToPackRow(int chestId, int groupId, int amount, int cell, Point player,
        std::uint64_t expectedRevision);
    // Sets the row from item ids (-1 for empty): each cell takes her first carried stack of that item
    // not already placed, or stays empty. For saves from before the row (their old pinned hotbar)
    // and new games.
    Result ArrangePackRow(const std::array<int, PackRowSize>& items);
    // Test fixtures only (never saved): with this off, new stacks and garments go below the row
    // instead of into its first empty cell, so grid-navigation suites keep their stock in the grid.
    void SetPackRowAutoFill(bool fill) { fillPackRow_ = fill; }
    Result DropGroup(int groupId, int amount, Point position, Point player,
        std::uint64_t expectedRevision);
    Result DropWearable(int wearableId, Point position, Point player,
        std::uint64_t expectedRevision);
    Result PickUpDrop(int dropId, Point player);
    Result Sleep(double hours, Point player, Point facing, bool dawnLimited = false);
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
    // What a swing of `tool` is aimed at: the nearest uncleared overgrowth of that tool's kind whose
    // centre is within Overgrowth::Reach and roughly ahead of her (Overgrowth::AimHalfAngleDegrees), or -1. Tier and energy aren't checked
    // here, so an under-tier target still gets named and refused. Focus and swing both use this.
    int FindAimedOvergrowth(Point player, Point facing, Item tool) const;
    // The resource the prompt names with `tool` in hand, given the nearest resource `current` (-1 for
    // none): a forageable keeps it; otherwise the aimed target wins, even over overgrowth the tool also
    // clears behind her or off to the side, and `current` stays only when nothing is aimed at. The
    // swing always strikes FindAimedOvergrowth, so the prompt and the blow agree.
    int HeldToolFocus(int current, Point player, Point facing, Item tool) const;    // Grass and weeds whose centres lie in the scythe's forward arc (wider at higher tiers).
    std::vector<int> ScytheArcTargets(Point player, Point facing) const;
    // One scythe sweep: each target mown as its own ClearOvergrowth, in order. The presentation plays
    // one swish for the sweep when mown is above zero; problem is the first refusal, if any.
    struct MowSweepResult
    {
        int mown = 0;
        std::string problem;
    };
    MowSweepResult MowSweep(const std::vector<int>& targets, Point player);
    static double ScytheArcRadius(ToolTier tier);
    static double ScytheArcHalfAngle(ToolTier tier);

    void Advance(double realSeconds, Point player, bool paused = false);
    void AdvanceGameHours(double hours, Point player);
    // Playtest aid: jump the clock forward to the next occurrence of hourOfDay (0-24) without
    // simulating the skipped interval, so needs, crops and fires are left as they were.
    void SkipToHourOfDay(double hourOfDay);
    // Playtest aid: let `days` whole days pass hour by hour (crops grow, soil dries, weeds come up at 6 AM,
    // shops sell down, just as if she'd lived them), keeping her fed and rested. With `tend`, every
    // plot is watered and weeded each hour, so crops grow at full speed.
    Result PassDaysForPlaytest(double days, bool tend, Point player);
    // Playtest aid for screenshots: set every planted plot's growth (0-1) directly.
    Result SetCropGrowthForPlaytest(double growth);
    // Whether she may sprint now: not failed and Energy at least Exertion::SprintFloor. Running costs
    // nothing extra; the ordinary awake drain and work costs are what bring her down to the floor.
    Result CanSprint() const;
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
    // The leather backpack: a one-time upgrade at an open General Store (HomesteadBackpack.h).
    Result BuyBackpack(int shopId, Point player);
    // One garment from the General Store's clothing (HomesteadGarmentShop.h), into her pack.
    Result BuyGarment(int shopId,     WearableDefinition definition, Point player);
        // A worn tool upgraded to iron at an open General Store (HomesteadToolUpgrades.h).
        Result BuyToolUpgrade(int shopId, ToolKind tool, Point player);    // Shows or hides the backpack on her back; capacity is unchanged either way.
    Result SetBackpackShown(bool shown);
    // Counts a shopkeeper greeting (a friendship stub).
    Result GreetShopkeeper(int shopId);
    // Waits by a closed shop until it opens: the ordinary passage of time (crops, weather, vitals,
    // the morning sell-down), refused before any time passes if she'd collapse first.
    Result WaitForShop(int shopId, Point player);
    // Walks the public road to the manor or town from `from` (HomesteadTravel.cpp): the ordinary
    // passage of time for the walk's length, refused before any time passes if she'd collapse or
    // doze off on the way. The caller stands her at PlanTravel's arrival point.
    Result WalkRoad(TravelDestination destination, Point from);
    // Playtest aids: adjust the purse; open (or move) a shop with its counter at `counter`.
    // Playtest grant (or take, when negative). Refused, changing nothing, if the purse would go below
    // zero or past MaxMoney; overflow-safe for any int64 amount.
    Result GrantMoney(Coins coins);
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
    bool fillPackRow_ = true;
    std::shared_ptr<const EstateLayout> layout_;
    std::shared_ptr<const EstatePlacements> placements_;
    std::function<bool(Point)> waterProbe_;
    std::function<FishingWater(Point)> fishingWaterProbe_;
    FishingSession fishing_;
    double fishingEscapeChance_ = Fishing::EscapeChance;
    std::uint64_t nextFishingToken_ = 1;
    std::uint64_t revision_ = 0;
    int nextResourceHandle_ = TransientResourceIdBase;
    int dozes_ = 0;
    int seasonChanges_ = 0;
    SeasonChange lastSeasonChange_;
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
    // Once a day at the 6 AM rollover too (HomesteadUpkeep.cpp): ground she has cleared near the manor and
    // farm may grow over again, and the wind may drop a branch under a tree, so there is always some tidying.
    void UpkeepRegrowth(int day);
    // Calendar hooks (HomesteadCalendarHooks.cpp). Step calls OnNewDay once for every 06:00 rollover
    // it crosses, and OnNewDay calls OnNewSeason first when that day begins a season. Features that
    // change with the day or the season add their call there rather than in Step.
    void OnNewDay(const Calendar::Date& today);
    void OnNewSeason(const Calendar::Date& today, Season from);
    // Oil lamp (HomesteadLamp.cpp).
    bool lampInHand_ = false;
    void BurnLamp(double hours, bool sleeping);
    // Gives an estate game the lamp kit once (lamp, full, and flasks) when the pack has room.
    void GrantLampKit();
};
}
