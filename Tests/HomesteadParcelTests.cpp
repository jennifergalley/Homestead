#include "HomesteadEstate.h"
#include "HomesteadParcels.h"
#include "HomesteadSimulation.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
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
void Run(const char* name, void (*test)())
{
    test();
    ++cases;
    std::cout << "PASS " << name << '\n';
}

// A test estate: a 200 m square home parcel west of a 100 m for-sale square that shares its east
// edge, with the town far outside both.
EstateLayout SquareLayout()
{
    EstateLayout layout;
    layout.landmarks = {
        {Anchor::StandingRoomSpawn, {10000.0, 10000.0}, 0.0, 0.0},
        {Anchor::TownSquare, {100000.0, 100000.0}, 0.0, 0.0},
    };
    layout.polygons = {
        {Anchor::EstateBoundary, {{0.0, 0.0}, {20000.0, 0.0}, {20000.0, 20000.0}, {0.0, 20000.0}}},
        {Anchor::ManorFootprint, {{9000.0, 9000.0}, {9500.0, 9000.0}, {9500.0, 9500.0}}},
        {std::string(Anchor::ForSaleParcelPrefix) + "East",
            {{0.0, 20000.0}, {20000.0, 20000.0}, {20000.0, 30000.0}, {0.0, 30000.0}}},
        {std::string(Anchor::ForSaleParcelPrefix) + "Sliver", {{0.0, 0.0}, {1.0, 1.0}}},
        {std::string(Anchor::ForSaleParcelPrefix) + "East", {{-5.0, -5.0}, {-1.0, -5.0}, {-1.0, -1.0}}},
        {std::string(Anchor::ForSaleParcelPrefix) + "Old Mill", {{-9000.0, 0.0}, {-1000.0, 0.0}, {-1000.0, 5000.0}}},
    };
    return layout;
}

EstatePlacements NoPlacements()
{
    EstatePlacements placements;
    placements.bakeVersion = 7;
    return placements;
}

std::uint64_t Checksum(const std::string& body)
{
    std::uint64_t hash = UINT64_C(14695981039346656037);
    for (unsigned char c : body) { hash ^= c; hash *= UINT64_C(1099511628211); }
    return hash;
}

// Re-wraps an edited save payload in a valid envelope, so only the parser's rules are tested.
std::string Rewrap(const std::string& save, const std::string& payload)
{
    std::string header = save.substr(0, save.find('\n'));
    const std::string version = header.substr(0, header.find(' ', 10));
    return version + " " + std::to_string(payload.size()) + " " + std::to_string(Checksum(payload)) + "\n" + payload;
}

std::string Payload(const std::string& save) { return save.substr(save.find('\n') + 1); }

std::string Replace(std::string text, const std::string& from, const std::string& to)
{
    const auto at = text.find(from);
    CHECK(at != std::string::npos);
    return text.replace(at, from.size(), to);
}

void Stock(Simulation& sim)
{
    for (Item item : {Item::Branch, Item::Stone, Item::Fiber})
        OK(sim.GrantItems(item, 30));
}

void PolygonEdgeCases()
{
    const std::vector<Point> square{{0, 0}, {10, 0}, {10, 10}, {0, 10}};
    CHECK(PointInPolygon(square, {5, 5}));
    CHECK(!PointInPolygon(square, {15, 5}));
    CHECK(!PointInPolygon(square, {5, -0.001}));
    CHECK(!PointInPolygon(square, {-5, 5}));
    // Concave "L": the notch is outside even though it lies inside the bounding box.
    const std::vector<Point> ell{{0, 0}, {10, 0}, {10, 4}, {4, 4}, {4, 10}, {0, 10}};
    CHECK(PointInPolygon(ell, {2, 8}));
    CHECK(PointInPolygon(ell, {8, 2}));
    CHECK(!PointInPolygon(ell, {8, 8}));
    // A ray through a vertex and along a horizontal edge counts once.
    CHECK(PointInPolygon(ell, {2, 4}));
    CHECK(!PointInPolygon(ell, {12, 4}));
    CHECK(!PointInPolygon(ell, {12, 0}));
    // Clockwise and counter-clockwise rings agree.
    const std::vector<Point> reversed{{0, 10}, {10, 10}, {10, 0}, {0, 0}};
    CHECK(PointInPolygon(reversed, {5, 5}) && !PointInPolygon(reversed, {5, 11}));
    // Degenerate rings contain nothing.
    CHECK(!PointInPolygon({}, {0, 0}));
    CHECK(!PointInPolygon({{0, 0}, {10, 10}}, {5, 5}));
    CHECK(!PointInPolygon({{0, 0}, {10, 0}, {20, 0}}, {5, 0}));
    // A point on an edge two neighbours share belongs to exactly one of them.
    const std::vector<Point> left{{0, 0}, {10, 0}, {10, 10}, {0, 10}};
    const std::vector<Point> right{{10, 0}, {20, 0}, {20, 10}, {10, 10}};
    for (double y : {0.5, 5.0, 9.5})
        CHECK(PointInPolygon(left, {10, y}) != PointInPolygon(right, {10, y}));
}

void ParcelsComeFromTheLayout()
{
    const auto parcels = ParcelsFromLayout(SquareLayout());
    // The manor footprint, the sliver and the duplicate id are not parcels.
    CHECK(parcels.size() == 3);
    CHECK(parcels[0].id == Anchor::EstateBoundary && parcels[0].owned && !parcels[0].forSale);
    CHECK(parcels[1].id == "ForSale.East" && !parcels[1].owned && parcels[1].forSale);
    CHECK(parcels[1].polygon.size() == 4);
    CHECK(parcels[2].id == "ForSale.Old_Mill");
    CHECK(FindParcelAt(parcels, {10000, 25000}) == &parcels[1]);
    CHECK(FindParcelAt(parcels, {100000, 100000}) == nullptr);
    CHECK(InOwnedParcel(parcels, {10000, 10000}));
    CHECK(!InOwnedParcel(parcels, {10000, 25000}));
    // The provisional layout the lanes develop against: home owned, woodland for sale.
    const auto provisional = ParcelsFromLayout(ProvisionalEstateLayout());
    const EstateLayout& layout = ProvisionalEstateLayout();
    CHECK(provisional.size() >= 2 && provisional[0].owned);
    for (std::size_t i = 1; i < provisional.size(); ++i) CHECK(provisional[i].forSale && !provisional[i].owned);
    CHECK(InOwnedParcel(provisional, layout.PointOr(Anchor::StandingRoomSpawn, {})));
    // The compact map (shrink-estate-map): her cove and the mine ruin are hers; the mill by the ford is past the village.
    CHECK(InOwnedParcel(provisional, layout.PointOr(Anchor::CoveBeach, {})));
    CHECK(InOwnedParcel(provisional, layout.PointOr(Anchor::MineEntrance, {})));
    CHECK(!InOwnedParcel(provisional, layout.PointOr(Anchor::MillSite, {})));
    CHECK(!InOwnedParcel(provisional, layout.PointOr(Anchor::TownSquare, {})));
    // The town road meets the estate drive at its junction, inside the owned land; the square is outside it.
    // Turned footprints report their true corners.
    Point corners[4];
    FootprintCorners({{0, 0}, {10, 5}, 90.0}, corners);
    for (const Point& corner : corners)
        CHECK(std::abs(std::abs(corner.x) - 5.0) < 1e-9 && std::abs(std::abs(corner.y) - 10.0) < 1e-9);
}

void NewEstateOwnership()
{
    Simulation sim;
    OK(sim.NewEstateGame(SquareLayout(), NoPlacements()));
    CHECK(sim.GetState().parcels.size() == 3);
    CHECK(sim.IsOwned({10000, 10000}));
    CHECK(!sim.IsOwned({10000, 25000}));
    CHECK(!sim.IsOwned({100000, 100000}));
    const Parcel* sale = sim.ParcelAt({10000, 25000});
    CHECK(sale && sale->forSale && !sale->owned);
    CHECK(sim.ParcelAt({100000, 100000}) == nullptr);
    // The seeded woodland has no parcels: everything is hers and building is unrestricted.
    Simulation woodland;
    CHECK(woodland.GetState().parcels.empty());
    CHECK(woodland.IsOwned({900000, -900000}));
    CHECK(woodland.Serialize().find("parcels") == std::string::npos);
}

void BuildingNeedsOwnedLand()
{
    Simulation sim;
    OK(sim.NewEstateGame(SquareLayout(), NoPlacements()));
    Stock(sim);
    const auto stock = sim.GetState().inventory;
    const double energy = sim.GetState().energy;
    // A free foundation (300 cm) centred 100 cm inside the west edge pokes 50 cm outside.
    const Point straddle{100.0, 10000.0};
    const PlacementTarget outside = sim.ResolvePlacement(Piece::Foundation, straddle, 0.0, 0);
    const Result refused = sim.CheckPlacement(outside, straddle);
    CHECK(!refused.ok && refused.message == OutsideEstateMessage);
    CHECK(sim.CanBuildAt(outside).message == OutsideEstateMessage);
    const Result placed = sim.Place(outside, straddle);
    CHECK(!placed.ok && placed.message == OutsideEstateMessage);
    CHECK(sim.GetState().inventory == stock && sim.GetState().energy == energy);
    CHECK(sim.GetState().structures.empty() && sim.GetState().buildings.empty());
    // Wholly inside, with its edge flush against the boundary, it builds.
    const Point flush{150.0, 10000.0};
    const PlacementTarget inside = sim.ResolvePlacement(Piece::Foundation, flush, 0.0, 0);
    OK(sim.CanBuildAt(inside));
    OK(sim.Place(inside, flush));
    CHECK(sim.GetState().structures.size() == 1);
    // A turned foundation near a corner: only its rotated corners decide.
    const Point corner{180.0, 180.0};
    OK(sim.CanBuildAt(sim.ResolvePlacement(Piece::Foundation, corner, 0.0, 0)));
    CHECK(sim.CanBuildAt(sim.ResolvePlacement(Piece::Foundation, corner, 45.0, 0)).message == OutsideEstateMessage);
    OK(sim.CanBuildAt(sim.ResolvePlacement(Piece::Foundation, {400.0, 400.0}, 45.0, 0)));
    // For-sale land and open country are both refused.
    for (Point aim : {Point{10000, 25000}, Point{100000, 100000}})
    {
        const PlacementTarget target = sim.ResolvePlacement(Piece::Foundation, aim, 0.0, 0);
        CHECK(sim.CheckPlacement(target, aim).message == OutsideEstateMessage);
    }
    // Furniture stands alone too.
    const Point far{10000, 25000};
    CHECK(sim.CheckPlacement(sim.ResolvePlacement(Piece::Chest, far, 0.0, 0), far).message == OutsideEstateMessage);
    OK(sim.CheckPlacement(sim.ResolvePlacement(Piece::Chest, {5000, 5000}, 0.0, 0), {5000, 5000}));
    // A wall on the flush foundation's outer (west) edge is on her land with its floor.
    const Point westEdge{10.0, 10000.0};
    const PlacementTarget wall = sim.ResolvePlacement(Piece::Wall, westEdge, 0.0, 0);
    CHECK(wall.snapped);
    OK(sim.CanBuildAt(wall));
    OK(sim.CheckPlacement(wall, flush));
    // Nothing in the woodland is restricted.
    Simulation woodland;
    OK(woodland.CanBuildAt(woodland.ResolvePlacement(Piece::Foundation, {-900.0, 0.0}, 0.0, 0)));
}

void ForagingOutsideStillWorks()
{
    EstatePlacements placements = NoPlacements();
    const Point town{100000, 100000};
    placements.placements = {{EstatePlacementIdBase + 1, ResourceKind::Branches, town, 0, 0, 1, 0}};
    Simulation sim;
    OK(sim.NewEstateGame(SquareLayout(), placements));
    CHECK(!sim.IsOwned(town));
    const int before = sim.Count(Item::Branch);
    OK(sim.GrantItems(Item::Knife, 1));
    OK(sim.Harvest(EstatePlacementIdBase + 1, town));
    CHECK(sim.Count(Item::Branch) > before);
}

void OwnershipSavesAndReloads()
{
    const EstateLayout layout = SquareLayout();
    Simulation sim;
    OK(sim.NewEstateGame(layout, NoPlacements()));
    const std::string saved = sim.Serialize();
    CHECK(Payload(saved).find("parcels 3\nEstateBoundary 1\nForSale.East 0\nForSale.Old_Mill 0\n") != std::string::npos);
    // Plain reload keeps the defaults.
    Simulation loaded;
    loaded.SetLayout(layout);
    loaded.SetPlacements(NoPlacements());
    OK(loaded.Deserialize(saved));
    CHECK(loaded.GetState().parcels.size() == 3 && loaded.IsOwned({10000, 10000}) && !loaded.IsOwned({10000, 25000}));
    // A bought parcel stays bought, and building there is allowed.
    const std::string bought = Rewrap(saved, Replace(Payload(saved), "ForSale.East 0", "ForSale.East 1"));
    OK(loaded.Deserialize(bought));
    CHECK(loaded.IsOwned({10000, 25000}));
    const Point east{10000, 25000};
    OK(loaded.CanBuildAt(loaded.ResolvePlacement(Piece::Foundation, east, 0.0, 0)));
    CHECK(loaded.Serialize().find("ForSale.East 1") != std::string::npos);
    // The polygons come from the current layout, not the save: moving the east parcel moves it.
    EstateLayout moved = layout;
    moved.polygons[2].points = {{0, 40000}, {20000, 40000}, {20000, 50000}, {0, 50000}};
    Simulation remapped;
    remapped.SetLayout(moved);
    remapped.SetPlacements(NoPlacements());
    OK(remapped.Deserialize(bought));
    CHECK(!remapped.IsOwned({10000, 25000}) && remapped.IsOwned({10000, 45000}));
    // Saves written before parcels existed load with the new-game defaults.
    const std::string legacy = Rewrap(saved, Replace(Payload(saved),
        "parcels 3\nEstateBoundary 1\nForSale.East 0\nForSale.Old_Mill 0\n", ""));
    Simulation older;
    older.SetLayout(layout);
    older.SetPlacements(NoPlacements());
    OK(older.Deserialize(legacy));
    CHECK(older.GetState().parcels.size() == 3 && older.IsOwned({10000, 10000}) && !older.IsOwned({10000, 25000}));
    // Ids the layout no longer has are ignored; a parcel missing from the save keeps its default.
    const std::string stale = Rewrap(saved, Replace(Payload(saved), "ForSale.East 0", "ForSale.Gone 1"));
    OK(older.Deserialize(stale));
    CHECK(!older.IsOwned({10000, 25000}));
    // Malformed sections are corrupt and leave the game untouched.
    const auto revision = older.GetRevision();
    for (const std::string& bad : {
        Replace(Payload(saved), "ForSale.East 0", "ForSale.East 2"),
        Replace(Payload(saved), "ForSale.East 0", "EstateBoundary 0"),
        Replace(Payload(saved), "parcels 3", "parcels 4"),
        Replace(Payload(saved), "parcels 3", "parcels -1")})
    {
        const Result result = older.Deserialize(Rewrap(saved, bad));
        CHECK(!result.ok && result.code == ResultCode::CorruptSave);
        CHECK(older.GetRevision() == revision);
    }
    // An unknown trailing word reads as a section from a newer build: refused, the game untouched.
    const Result unknown = older.Deserialize(Rewrap(saved, Payload(saved) + "trailing\n"));
    CHECK(!unknown.ok && unknown.code == ResultCode::NewerBuild);
    CHECK(older.GetRevision() == revision);
}
}

int main()
{
    Run("point-in-polygon edge cases", PolygonEdgeCases);
    Run("parcels come from the layout polygons", ParcelsComeFromTheLayout);
    Run("a new estate game owns only the home estate", NewEstateOwnership);
    Run("building needs owned land and consumes nothing when refused", BuildingNeedsOwnedLand);
    Run("foraging outside the estate still works", ForagingOutsideStillWorks);
    Run("parcel ownership saves and reloads", OwnershipSavesAndReloads);
    std::cout << cases << " scenarios, " << checks << " explicit checks passed.\n";
    return 0;
}
