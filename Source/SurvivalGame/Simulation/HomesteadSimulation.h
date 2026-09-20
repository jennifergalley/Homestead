#pragma once

#include <array>
#include <string>
#include <vector>

namespace Homestead
{
enum class Item : int
{
    Knife, Branch, Stone, Fiber, Berries, Roots, Flowers, Seeds,
    Hatchet, DiggingStick, WateringCan, Water, RoastedRoots, HerbedRoots, Count
};
enum class ResourceKind : int { Branches, Stones, BerryBush, Roots, Flowers, Reeds, Sapling, Count };
enum class Recipe : int { Hatchet, DiggingStick, WateringCan, RoastedRoots, HerbedRoots, Count };
enum class Piece : int { Foundation, Wall, Doorway, Roof, Fire, Bed, Chest, Count };
enum class CropKind : int { Roots, Berries, Count };

constexpr int ItemCount = static_cast<int>(Item::Count);
constexpr double CellSize = 300.0;
constexpr int InventoryCapacity = 120;

struct Point { double x = 0.0; double y = 0.0; };
using Inventory = std::array<int, ItemCount>;

struct Result
{
    bool ok = false;
    std::string message;
    explicit operator bool() const { return ok; }
};

struct ResourceNode
{
    int id = 0;
    ResourceKind kind = ResourceKind::Branches;
    Point position;
    double readyAtHour = 0.0;
    bool cleared = false;
};

struct Structure
{
    int id = 0;
    Piece kind = Piece::Foundation;
    int cellX = 0;
    int cellY = 0;
    int rotation = 0; // Quarter turns: 0=north (+Y), 1=east (+X), 2=south, 3=west.
    double fuelHours = 0.0;
    Inventory storage{};
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
};

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

class Simulation
{
public:
    Simulation();
    const State& GetState() const { return state_; }
    void NewGame();
    int Count(Item item) const;
    int UsedCapacity() const;
    bool IsRaining() const;
    bool IsNight() const;
    int DayNumber() const;
    const char* SeasonName() const;
    bool IsSheltered(Point position) const;
    bool IsNearFire(Point position) const;
    bool CanHarvest(int nodeId) const;
    int FindNearestResource(Point position, double maxDistance) const;
    int FindNearestPlot(Point position, double maxDistance) const;
    int FindNearestStructure(Point position, Piece kind, double maxDistance) const;

    Result Harvest(int nodeId, Point player);
    Result Clear(int nodeId, Point player);
    Result Eat(Item item);
    Result Craft(Recipe recipe, Point player);
    Result Place(Piece kind, int cellX, int cellY, int rotation, Point player);
    Result Till(int cellX, int cellY, Point player);
    Result Plant(int plotId, Point player, CropKind kind = CropKind::Roots);
    Result Water(int plotId, Point player);
    Result Weed(int plotId, Point player);
    Result HarvestCrop(int plotId, Point player);
    Result FillWater(Point player);
    Result AddFuel(int structureId, Point player);
    Result Transfer(int chestId, Item item, int amount, Point player);
    Result Sleep(double hours, Point player);
    Result SetDayMinutes(double minutes);
    void SetWarmOutfit(bool enabled);
    void Advance(double realSeconds, Point player, bool paused = false);
    void AdvanceGameHours(double hours, Point player);

    std::string Serialize() const;
    Result Deserialize(const std::string& data);

private:
    State state_;
    bool TryAdjust(const Inventory& change);
    void Step(double hours, Point player, bool sleeping);
};
}
