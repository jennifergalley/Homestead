// Portable tests for period crafting (rework-farming-calendar-and-period-crafting, lane E): the
// workbench and sawhorse stations, planks, fences and gates, and small furniture.
#include "HomesteadCrafting.h"
#include "HomesteadEstate.h"
#include "HomesteadItems.h"
#include "HomesteadParcels.h"
#include "HomesteadSimulation.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

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

void Okay(const Result& result, int line)
{
    ++checks;
    if (!result.ok)
    {
        std::cerr << "FAIL line " << line << ": " << result.message << '\n';
        std::exit(1);
    }
}
#define OK(expression) Okay(expression, __LINE__)

bool Near(double a, double b, double tolerance = 1e-6) { return std::abs(a - b) < tolerance; }
bool Near(Point a, Point b, double tolerance = 0.01) { return Near(a.x, b.x, tolerance) && Near(a.y, b.y, tolerance); }

// A 200 m square estate that is all hers, a for-sale strip north of it, and the heritage standing
// room (2 x 2 cells, grid heading 0) centred at (10000, 10000).
EstateLayout TestLayout()
{
    EstateLayout layout;
    layout.landmarks = {
        {Anchor::StandingRoomSpawn, {10000.0, 10000.0}, 0.0, 0.0},
        {Anchor::StandingRoomOrigin, {10000.0, 10000.0}, 0.0, 0.0},
        {Anchor::TownSquare, {100000.0, 100000.0}, 0.0, 0.0},
    };
    layout.polygons = {
        {Anchor::EstateBoundary, {{0.0, 0.0}, {20000.0, 0.0}, {20000.0, 20000.0}, {0.0, 20000.0}}},
        {std::string(Anchor::ForSaleParcelPrefix) + "North",
            {{0.0, 20000.0}, {20000.0, 20000.0}, {20000.0, 30000.0}, {0.0, 30000.0}}},
    };
    return layout;
}

EstatePlacements NoPlacements()
{
    EstatePlacements placements;
    placements.bakeVersion = 7;
    return placements;
}

Simulation NewTestEstate()
{
    Simulation sim;
    OK(sim.NewEstateGame(TestLayout(), NoPlacements()));
    return sim;
}

Simulation Reload(const Simulation& sim, const std::string& saved)
{
    Simulation loaded;
    loaded.SetLayout(TestLayout());
    loaded.SetPlacements(NoPlacements());
    OK(loaded.Deserialize(saved));
    (void)sim;
    return loaded;
}

std::uint64_t Checksum(const std::string& body)
{
    std::uint64_t hash = UINT64_C(14695981039346656037);
    for (unsigned char c : body) { hash ^= c; hash *= UINT64_C(1099511628211); }
    return hash;
}

// Re-wraps an edited payload in a valid envelope, so only the parser's rules are tested.
std::string Rewrap(const std::string& save, const std::string& payload)
{
    std::istringstream header(save.substr(0, save.find('\n')));
    std::string magic, version;
    header >> magic >> version;
    return magic + " " + version + " " + std::to_string(payload.size()) + " " + std::to_string(Checksum(payload)) + "\n" + payload;
}

std::string Payload(const std::string& save) { return save.substr(save.find('\n') + 1); }

int CountOf(const Simulation& sim, Piece kind)
{
    int count = 0;
    for (const auto& piece : sim.GetState().structures) count += piece.kind == kind;
    return count;
}

const Structure* Last(const Simulation& sim)
{
    return sim.GetState().structures.empty() ? nullptr : &sim.GetState().structures.back();
}

void BuildStation(Simulation& sim, Piece station, Point at)
{
    OK(sim.GrantItems(Item::Timber, 6));
    OK(sim.GrantItems(Item::Twine, 4));
    OK(sim.Place(sim.ResolvePlacement(station, at, 0.0, 0), at));
}

void WorkbenchIsTheStation()
{
    Simulation sim = NewTestEstate();
    const Point yard{5000.0, 5000.0};
    OK(sim.GrantItems(Item::Planks, 10));
    const auto stock = sim.GetState().inventory;
    const double energy = sim.GetState().energy;
    // Away from a workbench the recipe says so, and nothing is spent.
    const RecipeAssessment away = sim.AssessRecipe(Recipe::MakeFenceSection, yard);
    CHECK(away.stationRequired && !away.stationMet && away.stationLabel == "Workbench within reach");
    CHECK(!away.craftable && away.blocker.find("Needs a workbench within reach") == 0);
    CHECK(away.blocker.find("6 Timber + 4 Twine") != std::string::npos);
    const Result refused = sim.Craft(Recipe::MakeFenceSection, yard);
    CHECK(!refused.ok && refused.message == away.blocker);
    CHECK(sim.GetState().inventory == stock && sim.GetState().energy == energy);
    // Build one from timber and twine, by hand; then the recipe works beside it.
    BuildStation(sim, Piece::Workbench, yard);
    CHECK(CountOf(sim, Piece::Workbench) == 1);
    const RecipeAssessment beside = sim.AssessRecipe(Recipe::MakeFenceSection, yard);
    CHECK(beside.stationRequired && beside.stationMet && beside.craftable);
    OK(sim.Craft(Recipe::MakeFenceSection, yard));
    CHECK(sim.Count(Item::Planks) == 8 && sim.Count(Item::FenceSection) == 1);
    // Out of reach again.
    CHECK(!sim.Craft(Recipe::MakeStool, {yard.x + Crafting::StationReach + 200.0, yard.y}).ok);
    // Each workbench recipe makes its piece.
    OK(sim.GrantItems(Item::ScrapIron, 2));
    OK(sim.Craft(Recipe::MakeFieldGate, yard));
    OK(sim.Craft(Recipe::MakeShelf, yard));
    CHECK(sim.Count(Item::FieldGate) == 1 && sim.Count(Item::Shelf) == 1 && sim.Count(Item::ScrapIron) == 0);
    CHECK(sim.Count(Item::Planks) == 1);
    // Hafting a tool stays possible by hand, far from any station.
    const Point field{15000.0, 15000.0};
    OK(sim.GrantItems(Item::RustedAxeHead, 1));
    OK(sim.GrantItems(Item::Branch, 2));
    CHECK(!sim.AssessRecipe(Recipe::HaftAxe, field).stationRequired);
    OK(sim.Craft(Recipe::HaftAxe, field));
    CHECK(std::string(RecipeRequirements(Recipe::MakeTable)) == "5 Plank; at a workbench");
    CHECK(std::string(PieceRequirements(Piece::Workbench)).find("6 Timber + 4 Twine") == 0);
}

void SawhorseSawsPlanks()
{
    Simulation sim = NewTestEstate();
    const Point yard{6000.0, 6000.0};
    OK(sim.GrantItems(Item::Timber, 2));
    CHECK(sim.Craft(Recipe::SawPlanks, yard).message.find("Needs a sawhorse within reach") == 0);
    CHECK(sim.Count(Item::Timber) == 2);
    OK(sim.GrantItems(Item::Timber, 4));
    OK(sim.GrantItems(Item::Twine, 2));
    OK(sim.Place(sim.ResolvePlacement(Piece::Sawhorse, yard, 0.0, 0), yard));
    CHECK(sim.Count(Item::Timber) == 2 && sim.Count(Item::Twine) == 0);
    const double energy = sim.GetState().energy;
    const Result sawn = sim.Craft(Recipe::SawPlanks, yard);
    OK(sawn);
    CHECK(sawn.message == "Sawed a length of timber into 4 planks.");
    CHECK(sim.Count(Item::Timber) == 1 && sim.Count(Item::Planks) == 4);
    CHECK(Near(sim.GetState().energy, energy - Crafting::SawEnergy));
    CHECK(sim.AssessRecipe(Recipe::SawPlanks, yard).stationLabel == "Sawhorse within reach");
}

void FencesSnapEndToEnd()
{
    Simulation sim = NewTestEstate();
    OK(sim.GrantItems(Item::FenceSection, 6));
    // Headings come in whole 15 degree steps.
    CHECK(Near(sim.ResolvePlacement(Piece::FenceRail, {5000, 5000}, 37.0, 0).frame.yaw, 30.0));
    CHECK(Near(sim.ResolvePlacement(Piece::FenceRail, {5000, 5000}, 38.0, 0).frame.yaw, 45.0));
    CHECK(Near(sim.ResolvePlacement(Piece::FenceRail, {5000, 5000}, -10.0, 0).frame.yaw, 345.0));
    // The first section stands free, centred on the aim.
    const Point start{5000.0, 5000.0};
    const PlacementTarget first = sim.ResolvePlacement(Piece::FenceRail, start, 2.0, 0);
    CHECK(!first.snapped && first.buildingId == -1);
    OK(sim.Place(first, start));
    const Structure a = *Last(sim);
    CHECK(Near(sim.StructureCenter(a), start));
    CHECK(Near(Crafting::FenceEnd(sim.GetState(), a, 0), {4880.0, 5000.0}));
    CHECK(Near(Crafting::FenceEnd(sim.GetState(), a, 1), {5120.0, 5000.0}));
    // Aimed a little past its end, the next one joins at the post.
    const PlacementTarget second = sim.ResolvePlacement(Piece::FenceRail, {5230.0, 5040.0}, 0.0, 0);
    CHECK(second.snapped);
    OK(sim.Place(second, start));
    const Structure b = *Last(sim);
    CHECK(Near(Crafting::FenceEnd(sim.GetState(), b, 0), Crafting::FenceEnd(sim.GetState(), a, 1)));
    // Turning the corner: a quarter turn from the far post runs north.
    const PlacementTarget corner = sim.ResolvePlacement(Piece::FenceRail, {5370.0, 5110.0}, 90.0, 0);
    CHECK(corner.snapped);
    OK(sim.Place(corner, {5360.0, 5000.0}));
    const Structure c = *Last(sim);
    CHECK(Near(Crafting::FenceEnd(sim.GetState(), c, 0), {5360.0, 5000.0}));
    CHECK(Near(Crafting::FenceEnd(sim.GetState(), c, 1), {5360.0, 5240.0}));
    // A 15 degree bend joins too.
    const Point bentAim = Crafting::FenceEnd(sim.GetState(), a, 0);
    const PlacementTarget bent = sim.ResolvePlacement(Piece::FenceRail, {bentAim.x - 110.0, bentAim.y + 30.0}, 165.0, 0);
    CHECK(bent.snapped);
    OK(sim.Place(bent, start));
    // Four sections, five posts: shared posts are drawn once.
    const auto owners = Crafting::PostOwners(sim.GetState());
    CHECK(owners.size() == 4);
    int posts = 0;
    for (const auto& entry : owners) posts += ((entry.second & 1u) ? 1 : 0) + ((entry.second & 2u) ? 1 : 0);
    CHECK(posts == 5);
    CHECK(owners.at(a.id) == 3u && owners.at(b.id) == 2u && owners.at(c.id) == 2u);
    // The same bay again is refused; with nowhere free near the aim it names the clash.
    const Result again = sim.CheckPlacement(sim.ResolvePlacement(Piece::FenceRail, start, 0.0, 0), start);
    CHECK(!again.ok);
    // Taking one down returns the section whole.
    const int before = sim.Count(Item::FenceSection);
    OK(sim.Deconstruct(c.id, sim.StructureCenter(c)));
    CHECK(sim.Count(Item::FenceSection) == before + 1);
}

void FencesNeedOwnedLand()
{
    Simulation sim = NewTestEstate();
    OK(sim.GrantItems(Item::FenceSection, 2));
    const auto stock = sim.GetState().inventory;
    const std::size_t pieces = sim.GetState().structures.size();
    // On the for-sale land north of the estate, and straddling its west edge.
    for (const Point aim : {Point{10000.0, 25000.0}, Point{60.0, 10000.0}})
    {
        const PlacementTarget target = sim.ResolvePlacement(Piece::FenceRail, aim, 0.0, 0);
        const Result refused = sim.CheckPlacement(target, aim);
        CHECK(!refused.ok && refused.message == OutsideEstateMessage);
        CHECK(!sim.Place(target, aim).ok);
    }
    CHECK(sim.GetState().inventory == stock && sim.GetState().structures.size() == pieces);
    // Just inside the edge, turned along it, it builds.
    const PlacementTarget inside = sim.ResolvePlacement(Piece::FenceRail, {20.0, 10000.0}, 90.0, 0);
    OK(sim.Place(inside, {20.0, 10000.0}));
}

void GatesOpenShutAndPersist()
{
    Simulation sim = NewTestEstate();
    OK(sim.GrantItems(Item::FenceSection, 1));
    OK(sim.GrantItems(Item::FieldGate, 1));
    const Point start{7000.0, 7000.0};
    OK(sim.Place(sim.ResolvePlacement(Piece::FenceRail, start, 0.0, 0), start));
    const int fence = Last(sim)->id;
    const PlacementTarget gateSite = sim.ResolvePlacement(Piece::FenceGate, {7240.0, 7000.0}, 0.0, 0);
    CHECK(gateSite.snapped);
    OK(sim.Place(gateSite, start));
    const Structure gate = *Last(sim);
    CHECK(gate.kind == Piece::FenceGate && !gate.open);
    const Point at = sim.StructureCenter(gate);
    // Only from beside it, and never a fence.
    CHECK(!sim.ToggleGate(gate.id, {at.x + 2000.0, at.y}).ok);
    CHECK(!sim.ToggleGate(fence, start).ok);
    const std::uint64_t revision = sim.GetRevision();
    const Result opened = sim.ToggleGate(gate.id, at);
    OK(opened);
    CHECK(opened.message == "Opened the gate." && sim.GetRevision() > revision);
    const std::string saved = sim.Serialize();
    CHECK(saved.find("\ngates 1 " + std::to_string(gate.id) + "\n") != std::string::npos);
    Simulation loaded = Reload(sim, saved);
    bool open = false;
    for (const auto& piece : loaded.GetState().structures) if (piece.id == gate.id) open = piece.open;
    CHECK(open);
    const Result shut = loaded.ToggleGate(gate.id, at);
    CHECK(shut.ok && shut.message == "Shut the gate.");
    // All shut: the section is left out, and a save without it loads with every gate shut.
    const std::string closed = loaded.Serialize();
    CHECK(closed.find("gates") == std::string::npos);
    Simulation reloaded = Reload(loaded, closed);
    for (const auto& piece : reloaded.GetState().structures) CHECK(!piece.open);
    // Malformed or repeated sections are refused and the current game kept.
    const std::string payload = Payload(saved);
    const std::string line = "gates 1 " + std::to_string(gate.id) + "\n";
    const auto at_ = payload.find(line);
    CHECK(at_ != std::string::npos);
    const std::string without = payload.substr(0, at_) + payload.substr(at_ + line.size());
    for (const std::string bad : {std::string("gates 1 ") + std::to_string(fence) + "\n", std::string("gates 0\n"),
             line + line, std::string("gates 2 ") + std::to_string(gate.id) + " " + std::to_string(gate.id) + "\n",
             std::string("gates 1 999999\n"), std::string("gates x\n")})
    {
        Simulation refused;
        refused.SetLayout(TestLayout());
        refused.SetPlacements(NoPlacements());
        CHECK(!refused.Deserialize(Rewrap(saved, without + bad)).ok);
    }
    // Deconstructing an open gate returns it whole.
    Simulation taken = Reload(sim, saved);
    OK(taken.Deconstruct(gate.id, at));
    CHECK(taken.Count(Item::FieldGate) == 1);
}

// The heritage room's building and a floor cell's centre in it.
int RoomBuilding(const Simulation& sim)
{
    for (const auto& piece : sim.GetState().structures)
        if (piece.heritage && piece.buildingId > 0) return piece.buildingId;
    return 0;
}

void FurnishTheStandingRoom()
{
    Simulation sim = NewTestEstate();
    const int room = RoomBuilding(sim);
    CHECK(room > 0);
    const Building* frame = FindBuilding(sim.GetState(), room);
    CHECK(frame != nullptr);
    // Cell (0, 0) is the doorway cell, free of the hearth, bed and chest.
    const Point cell = BuildingCellCenter(*frame, 0, 0);
    OK(sim.GrantItems(Item::Table, 1));
    OK(sim.GrantItems(Item::Chair, 1));
    OK(sim.GrantItems(Item::Stool, 2));
    OK(sim.GrantItems(Item::Shelf, 1));
    // The table where she aims, in 5 cm steps.
    const PlacementTarget table = sim.ResolvePlacement(Piece::Table, {cell.x + 22.0, cell.y + 31.0}, 0.0, 0);
    CHECK(table.snapped && table.buildingId == room && table.cellX == 0 && table.cellY == 0);
    CHECK(Near(table.spot, {20.0, 30.0}));
    OK(sim.Place(table, cell));
    const Structure placedTable = *Last(sim);
    CHECK(Near(placedTable.spot, {20.0, 30.0}));
    // A chair in the same cell, clear of it.
    const PlacementTarget chair = sim.ResolvePlacement(Piece::Chair, {cell.x + 20.0, cell.y - 60.0}, 0.0, 0);
    OK(sim.Place(chair, cell));
    // A stool on top of the table is refused.
    const PlacementTarget onTable = sim.ResolvePlacement(Piece::Stool, {cell.x + 20.0, cell.y + 30.0}, 0.0, 0);
    CHECK(!sim.CheckPlacement(onTable, cell).ok);
    // Aimed at the wall, it is kept inside the room.
    const PlacementTarget byWall = sim.ResolvePlacement(Piece::Stool, {cell.x - 140.0, cell.y - 5.0}, 0.0, 0);
    CHECK(byWall.buildingId == room && byWall.spot.x > -150.0 + 14.0);
    OK(sim.Place(byWall, cell));
    // A turned shelf's spot is in piece space.
    const PlacementTarget shelf = sim.ResolvePlacement(Piece::Shelf, {cell.x - 80.0, cell.y + 90.0}, 0.0, 1);
    CHECK(shelf.rotation == 1);
    OK(sim.CheckPlacement(shelf, cell));
    // It all stays after a reload, spots and all.
    const std::string saved = sim.Serialize();
    CHECK(saved.find("\nspots 3\n") != std::string::npos);
    Simulation loaded = Reload(sim, saved);
    CHECK(CountOf(loaded, Piece::Table) == 1 && CountOf(loaded, Piece::Chair) == 1 && CountOf(loaded, Piece::Stool) == 1);
    for (const auto& piece : loaded.GetState().structures)
        if (piece.id == placedTable.id) CHECK(Near(piece.spot, {20.0, 30.0}));
    // A spot on anything but small furniture, or out of range, is refused.
    const std::string payload = Payload(saved);
    const auto at = payload.find("spots 3\n");
    const std::string head = payload.substr(0, at);
    const int roomFloor = [&] {
        for (const auto& piece : sim.GetState().structures)
            if (piece.kind == Piece::Foundation && piece.buildingId == room) return piece.id;
        return 0;
    }();
    for (const std::string bad : {std::string("spots 1\n") + std::to_string(roomFloor) + " 10 10\n",
             std::string("spots 1\n") + std::to_string(placedTable.id) + " 400 0\n",
             std::string("spots 1\n") + std::to_string(placedTable.id) + " nan 0\n",
             std::string("spots 0\n")})
    {
        Simulation refused;
        refused.SetLayout(TestLayout());
        refused.SetPlacements(NoPlacements());
        CHECK(!refused.Deserialize(Rewrap(saved, head + bad)).ok);
    }
    // Off a floor, furniture stands free where it's aimed.
    const Point yard{5000.0, 5000.0};
    const PlacementTarget outside = sim.ResolvePlacement(Piece::Stool, yard, 0.0, 0);
    CHECK(outside.buildingId == -1);
    OK(sim.Place(outside, yard));
}

const char* filter = nullptr;
void Run(const char* name, void (*test)())
{
    if (filter && !std::strstr(name, filter)) return;
    test();
    ++cases;
    std::cout << "PASS " << name << '\n';
}
}

int main(int argc, char** argv)
{
    if (argc > 1) filter = argv[1];
    Run("workbench recipes need a workbench; hafting stays by hand", WorkbenchIsTheStation);
    Run("the sawhorse saws one timber into four planks", SawhorseSawsPlanks);
    Run("fences snap end to end at the posts", FencesSnapEndToEnd);
    Run("fences need owned land", FencesNeedOwnedLand);
    Run("gates open, shut and persist", GatesOpenShutAndPersist);
    Run("small furniture furnishes the standing room", FurnishTheStandingRoom);
    std::cout << cases << " scenarios, " << checks << " explicit checks passed.\n";
    return 0;
}
