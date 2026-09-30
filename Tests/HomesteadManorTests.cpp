// add-ruined-manor-and-arrival: the heritage standing room, the reserved manor footprint and the
// new-game names. Kept apart from HomesteadSimulationTests so it builds and runs in seconds.
#include "HomesteadEstate.h"
#include "HomesteadManor.h"
#include "HomesteadOvergrowth.h"
#include "HomesteadSimulation.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

using namespace Homestead;

namespace
{
int checks = 0;
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

bool StartsWith(const std::string& text, const std::string& prefix) { return text.rfind(prefix, 0) == 0; }

Simulation NewEstate()
{
    Simulation sim;
    sim.SetPlacements(ProvisionalEstatePlacements());
    OK(sim.NewEstateGame(ProvisionalEstateLayout(), ProvisionalEstatePlacements()));
    return sim;
}

const Structure* Find(const State& state, Piece kind)
{
    for (const auto& piece : state.structures) if (piece.kind == kind) return &piece;
    return nullptr;
}

int CountOf(const State& state, Piece kind)
{
    int count = 0;
    for (const auto& piece : state.structures) count += piece.kind == kind;
    return count;
}

Point Spawn() { return ProvisionalEstateLayout().PointOr(Anchor::StandingRoomSpawn, {}); }

void SeededStandingRoom()
{
    Simulation sim = NewEstate();
    const State& state = sim.GetState();
    CHECK(sim.DayNumber() == 1 && std::string(sim.SeasonName()) == "Spring" && state.hour == 6.0);
    CHECK(state.heroineName == "Eleanor" && state.familyName == "Cavendish" && state.estateName == "Trevennor");
    CHECK(sim.EstateName() == "Trevennor");
    CHECK(state.buildings.size() == 1);
    const int room = Manor::HeritageBuildingId(state);
    CHECK(room == state.buildings[0].id);
    const Landmark* origin = ProvisionalEstateLayout().FindLandmark(Anchor::StandingRoomOrigin);
    const Point centre = RotateYaw({CellSize, CellSize}, state.buildings[0].yaw);
    CHECK(std::abs(state.buildings[0].origin.x + centre.x - origin->position.x) < 1e-6
        && std::abs(state.buildings[0].origin.y + centre.y - origin->position.y) < 1e-6);
    CHECK(state.journal.size() == 1 && state.journal[0] == Manor::ArrivalEntry);
    CHECK(Manor::JournalTitle(Manor::ArrivalEntry) == "Home at last");
    CHECK(Manor::JournalText(Manor::ArrivalEntry, state).find("Trevennor") != std::string::npos);
    CHECK(CountOf(state, Piece::Foundation) == 4 && CountOf(state, Piece::Roof) == 4);
    CHECK(CountOf(state, Piece::Wall) == 7 && CountOf(state, Piece::Doorway) == 1);
    CHECK(CountOf(state, Piece::Hearth) == 1 && CountOf(state, Piece::Bed) == 1 && CountOf(state, Piece::Chest) == 1);
    for (const auto& piece : state.structures)
    {
        CHECK(piece.heritage && piece.buildingId == room);
        const bool masonry = piece.kind == Piece::Foundation || piece.kind == Piece::Wall
            || piece.kind == Piece::Doorway || piece.kind == Piece::Roof;
        CHECK((piece.skin == StructureSkin::Stone) == masonry);
    }
    const Structure* chest = Find(state, Piece::Chest);
    CHECK(chest->storage[static_cast<int>(Item::WateringCan)] == 1);
    CHECK(chest->storage[static_cast<int>(Item::Branch)] == Manor::SeededBranches);
    // She wakes indoors, under the roof, with the doorway ahead of her.
    const Point spawn = Spawn();
    CHECK(sim.IsSheltered(spawn));
    const Structure* door = Find(state, Piece::Doorway);
    const Point doorCenter = StructureCenter(state, *door);
    const double yaw = ProvisionalEstateLayout().FindLandmark(Anchor::StandingRoomSpawn)->yaw;
    const Point facing = RotateYaw({1.0, 0.0}, yaw);
    const Point toDoor{doorCenter.x - spawn.x, doorCenter.y - spawn.y};
    CHECK(facing.x * toDoor.x + facing.y * toDoor.y > 0.0);
    CHECK(std::abs(facing.x * toDoor.y - facing.y * toDoor.x) < 1.0);
    // Heritage furniture and the always-lit hearth are consistent with ordinary validation.
    Simulation reloaded;
    reloaded.SetPlacements(ProvisionalEstatePlacements());
    OK(reloaded.Deserialize(sim.Serialize()));
    CHECK(reloaded.GetState().structures.size() == state.structures.size());
}

void SleepChestAndHearth()
{
    Simulation sim = NewEstate();
    const State& state = sim.GetState();
    const Structure* bed = Find(state, Piece::Bed);
    const int chestId = Find(state, Piece::Chest)->id;
    const int hearthId = Find(state, Piece::Hearth)->id;
    const Point chestSide = StructureCenter(state, *Find(state, Piece::Chest));
    const Point hearthSide = StructureCenter(state, *Find(state, Piece::Hearth));
    const Point bedSide = StructureCenter(state, *bed);
    const int day = sim.DayNumber();
    sim.SkipToHourOfDay(22.0);
    OK(sim.Sleep(8.0, bedSide));
    CHECK(sim.DayNumber() == day + 1);
    // Take the pail and branches out of the seeded chest.
    OK(sim.Transfer(chestId, Item::WateringCan, -1, chestSide));
    OK(sim.Transfer(chestId, Item::Branch, -Manor::SeededBranches, chestSide));
    CHECK(sim.Count(Item::WateringCan) == 1 && sim.Count(Item::Branch) == Manor::SeededBranches);
    // The hearth cooks like a cookfire, needs no fuel and stays lit.
    CHECK(sim.IsNearFire(hearthSide));
    CHECK(!sim.AddFuel(hearthId, hearthSide));
    sim.AdvanceGameHours(12.0, hearthSide);
    CHECK(sim.IsNearFire(hearthSide));
    OK(sim.GrantItems(Item::Roots, 2));
    // The always-lit hearth still takes one kindling per batch, like any cookfire.
    const int kindling = sim.Count(Item::Kindling);
    OK(sim.GrantItems(Item::Kindling, 1));
    const double hour = sim.GetState().hour;
    OK(sim.Craft(Recipe::RoastedRoots, hearthSide));
    CHECK(sim.Count(Item::Roots) == 0 && sim.Count(Item::RoastedRoots) == 1 && sim.Count(Item::Kindling) == kindling);
    CHECK(sim.GetState().hour == hour && sim.IsNearFire(hearthSide));
    CHECK(!sim.Craft(Recipe::RoastedRoots, hearthSide));
    CHECK(kindling == 0);
    OK(sim.GrantItems(Item::Roots, 2));
    const auto unlit = sim.Craft(Recipe::RoastedRoots, hearthSide);
    CHECK(!unlit.ok && unlit.message.find("Kindling") != std::string::npos && sim.Count(Item::Roots) == 2);
    OK(sim.GrantItems(Item::Kindling, 1));
    OK(sim.Craft(Recipe::RoastedRoots, hearthSide));
    CHECK(sim.Count(Item::Kindling) == 0 && sim.Count(Item::RoastedRoots) == 2);
    const Point faraway{hearthSide.x + 5000.0, hearthSide.y};
    OK(sim.GrantItems(Item::Roots, 2));
    CHECK(!sim.Craft(Recipe::RoastedRoots, faraway));
    CHECK(sim.AssessRecipe(Recipe::RoastedRoots, hearthSide).stationMet);
}

void ManorFootprintReservation()
{
    Simulation sim = NewEstate();
    const LandmarkPolygon* manor = ProvisionalEstateLayout().FindPolygon(Anchor::ManorFootprint);
    CHECK(manor != nullptr);
    // A foundation previewed in the old hall.
    const Point hall{-25000.0, -65000.0};
    CHECK(PointInPolygon(manor->points, hall));
    const auto blocked = sim.CheckPlacement(sim.ResolvePlacement(Piece::Foundation, hall, 0.0, 0), hall);
    CHECK(!blocked && blocked.message == Manor::FootprintBlocked);
    // A foundation snapped on beside the room, into the ruin, is refused too.
    const Point door = StructureCenter(sim.GetState(), *Find(sim.GetState(), Piece::Doorway));
    const Point west{door.x, door.y - 250.0};
    const auto snapped = sim.ResolvePlacement(Piece::Foundation, west, 0.0, 0);
    CHECK(!sim.CheckPlacement(snapped, west));
    // Outside the manor, beyond the clear-out's littered ground, the same foundation is fine.
    const Point lawn{-30500.0, -65000.0};
    CHECK(!PointInPolygon(manor->points, lawn));
    OK(sim.CheckPlacement(sim.ResolvePlacement(Piece::Foundation, lawn, 0.0, 0), lawn));
    // The room itself stays furnishable, and the hearth is never a plan.
    const Point doorCell = StructureCenter(sim.GetState(), *Find(sim.GetState(), Piece::Foundation));
    const auto fire = sim.ResolvePlacement(Piece::Fire, doorCell, 0.0, 0);
    CHECK(fire.buildingId == Manor::HeritageBuildingId(sim.GetState()));
    OK(sim.CheckPlacement(fire, doorCell));
    CHECK(!IsBuildable(Piece::Hearth) && IsBuildable(Piece::Fire));
    CHECK(!sim.CheckPlacement(sim.ResolvePlacement(Piece::Hearth, lawn, 0.0, 0), lawn));
    // Woodland games have no manor.
    Simulation woodland;
    CHECK(!Manor::BlockedByManor(woodland.GetState(), ProvisionalEstateLayout(),
        woodland.ResolvePlacement(Piece::Foundation, hall, 0.0, 0), Footprint{hall, {150, 150}, 0}));
}

void NamesValidationAndPersistence()
{
    CHECK(Manor::TrimName("  Clara \t") == "Clara");
    CHECK(Manor::TrimName("\xC2\xA0Tr\xC3\xA9vose\xC2\xA0") == "Tr\xC3\xA9vose");
    CHECK(Manor::NameLength("Tr\xC3\xA9vose") == 7);
    CHECK(Manor::NameLength("bad\xC3") == -1 && Manor::NameLength("tab\tname") == -1);
    CHECK(Manor::NameProblem("", "estate name") == "Enter an estate name.");
    CHECK(Manor::NameProblem("", "surname") == "Enter a surname.");
    Simulation sim = NewEstate();
    CHECK(!sim.SetNames("   ", "Pendarves", "Trevennor"));
    CHECK(!sim.SetNames("Clara", "", "Trevennor"));
    CHECK(!sim.SetNames("Clara", "Pendarves", std::string(25, 'a')));
    OK(sim.SetNames("Clara", "Pendarves", std::string(24, 'a')));
    OK(sim.SetNames("  Clara ", "Pendarves", " Trevennor"));
    CHECK(sim.GetState().heroineName == "Clara" && sim.GetState().estateName == "Trevennor");
    CHECK(Manor::SaveLabel(sim.GetState(), sim.SeasonName(), sim.DayNumber())
        == "Clara Pendarves \xE2\x80\x94 Trevennor, Spring 1");
    OK(sim.SetNames("\xC3\x89lise", "Tr\xC3\xA9vose", "Chy an Mor"));
    const std::string saved = sim.Serialize();
    Simulation loaded;
    loaded.SetPlacements(ProvisionalEstatePlacements());
    OK(loaded.Deserialize(saved));
    CHECK(loaded.GetState().heroineName == "\xC3\x89lise" && loaded.GetState().familyName == "Tr\xC3\xA9vose");
    CHECK(loaded.EstateName() == "Chy an Mor");
    CHECK(Manor::HeritageBuildingId(loaded.GetState()) != 0);
    for (const auto& piece : loaded.GetState().structures) CHECK(piece.heritage);
    CHECK(Find(loaded.GetState(), Piece::Wall)->skin == StructureSkin::Stone);
    CHECK(loaded.GetState().journal == sim.GetState().journal);
    // A damaged section is rejected rather than half-loaded.
    const auto at = saved.find("manor ");
    CHECK(at != std::string::npos);
    Simulation broken;
    broken.SetPlacements(ProvisionalEstatePlacements());
    std::string damaged = saved;
    damaged.replace(at, 5, "manoz");
    CHECK(!broken.Deserialize(damaged));
    // Woodland saves carry no section and load as before.
    Simulation woodland;
    const std::string plain = woodland.Serialize();
    CHECK(plain.find("manor ") == std::string::npos);
    Simulation plainLoaded;
    OK(plainLoaded.Deserialize(plain));
    CHECK(plainLoaded.GetState().estateName.empty() && plainLoaded.EstateName() == "the estate");
    CHECK(Manor::SaveLabel(plainLoaded.GetState(), "Spring", 1).empty());
    // The woodland playtest aid raises the same room wherever the ground is clear.
    Simulation aid;
    Result raised;
    for (int attempt = 0; attempt < 40 && !raised; ++attempt)
        raised = aid.SeedStandingRoomAt({-800.0 - 450.0 * (attempt % 8), 200.0 - 450.0 * (attempt / 8)}, 30.0 * attempt);
    OK(raised);
    CHECK(Manor::HeritageBuildingId(aid.GetState()) != 0 && CountOf(aid.GetState(), Piece::Hearth) == 1);
    CHECK(aid.GetState().estateName == "Trevennor" && aid.GetState().journal.size() == 1);
    CHECK(!aid.SeedStandingRoomAt({5000.0, 5000.0}, 0.0));
    Simulation aidLoaded;
    OK(aidLoaded.Deserialize(aid.Serialize()));
    CHECK(aidLoaded.GetState().structures.size() == aid.GetState().structures.size());
}

// add-derelict-farm-and-estate-disrepair: the old field and the neglected grounds are thick with
// clearable overgrowth, all on the estate, off the ruin and apart from the piles and each other.
void DerelictFarmAndDisrepair()
{
    const EstateLayout& layout = ProvisionalEstateLayout();
    const DerelictFarmPlan farm = EstateDerelictFarm(layout);
    CHECK(farm.valid && farm.lengthU >= 5000.0 && farm.lengthV >= 5000.0 && farm.ridgeBlocks.size() == 2);
    const auto& boundary = layout.FindPolygon(Anchor::EstateBoundary)->points;
    const auto& manor = layout.FindPolygon(Anchor::ManorFootprint)->points;
    const auto& field = layout.FindPolygon(Anchor::DerelictFarm)->points;
    // The field stays clear of the ruin: its south fence is well north of the rear wall.
    for (const Point& corner : manor) CHECK(!PointInPolygon(field, corner));
    CHECK(farm.southWest.x - (-24100.0) > 1500.0);
    const auto& placements = ProvisionalEstatePlacements().placements;
    int inField = 0, grounds = 0, worn = 0, teases = 0, gate = 0;
    std::vector<Point> piles;
    for (const auto& placement : placements)
        if (placement.kind == ResourceKind::SalvagePile) piles.push_back(placement.position);
    for (size_t i = 0; i < placements.size(); ++i)
    {
        const EstatePlacement& placement = placements[i];
        if (placement.id < 550000 || placement.id >= 560000) continue;
        CHECK(PointInPolygon(boundary, placement.position) && !PointInPolygon(manor, placement.position));
        const OvergrowthInfo* info = FindOvergrowth(placement.kind);
        CHECK(info != nullptr && placement.kind != ResourceKind::SalvagePile);
        worn += info && info->minTier == ToolTier::Worn;
        teases += info && info->minTier > ToolTier::Worn;
        const bool inside = PointInPolygon(field, placement.position);
        inField += inside;
        grounds += !inside;
        const Point gatePoint = layout.PointOr(Anchor::DerelictFarmGate, {});
        gate += std::hypot(placement.position.x - gatePoint.x, placement.position.y - gatePoint.y) < 350.0;
        for (const Point& pile : piles)
            CHECK(std::hypot(placement.position.x - pile.x, placement.position.y - pile.y) > 200.0);
        // Nothing grows inside the heritage standing room or on its door step.
        const Point room = layout.PointOr(Anchor::StandingRoomOrigin, {});
        CHECK(std::hypot(placement.position.x - room.x, placement.position.y - room.y) > 450.0);
        for (size_t j = 0; j < placements.size(); ++j)
            if (j != i)
            {
                CHECK(placements[j].id != placement.id);
                CHECK(std::hypot(placements[j].position.x - placement.position.x,
                    placements[j].position.y - placement.position.y) > 100.0);
            }
    }
    std::cout << "  derelict farm: " << inField << " in the field, " << grounds << " round the grounds, " << teases
              << " iron/steel teases\n";
    CHECK(inField >= 150 && grounds >= 120 && worn > 250 && teases >= 6 && gate >= 3);
    Simulation sim;
    OK(sim.NewEstateGame(layout, ProvisionalEstatePlacements()));
}
}

void StarterChestAndHoeClue()
{
    // Jenny's playtest: a new estate's chest also holds food and a change of clothes, the hoe is
    // the second rusted head she finds, and the game says where to look for it.
    Simulation sim = NewEstate();
    const State& state = sim.GetState();
    const Structure* chest = Find(state, Piece::Chest);
    CHECK(chest->storage[static_cast<int>(Item::WateringCan)] == 1);
    CHECK(chest->storage[static_cast<int>(Item::Branch)] == Manor::SeededBranches);
    CHECK(chest->storage[static_cast<int>(Item::Pasty)] == Manor::StarterPasties && Manor::StarterPasties == 3);
    CHECK(chest->storage[static_cast<int>(Item::Bread)] == Manor::StarterBread && Manor::StarterBread == 2);
    // Pail, branches, pasties, bread, then each outfit piece in order; she still wears only her tunic.
    CHECK(chest->layout.size() == 4 + std::size(Manor::StarterWardrobe));
    CHECK(chest->layout[2].item == Item::Pasty && chest->layout[2].quantity == 3);
    CHECK(chest->layout[3].item == Item::Bread && chest->layout[3].quantity == 2);
    int stored = 0, worn = 0;
    for (std::size_t i = 0; i < std::size(Manor::StarterWardrobe); ++i)
    {
        const LayoutEntry& entry = chest->layout[4 + i];
        const WearableInstance* item = sim.GetWearable(entry.wearableId);
        CHECK(item && item->definition == Manor::StarterWardrobe[i] && item->owner == WearableOwner::Chest
            && item->chestId == chest->id && item->dye == 0);
    }
    for (const auto& item : state.wearables)
    {
        stored += item.owner == WearableOwner::Chest;
        worn += item.owner == WearableOwner::Equipped;
        if (item.owner == WearableOwner::Equipped) CHECK(item.definition == WearableDefinition::LinenTunic);
    }
    CHECK(stored == 7 && worn == 1 && state.wearables.size() == 8);
    CHECK(sim.ChestUsedCapacity(chest->id) == 1 + Manor::SeededBranches + 3 + 2 + 7);
    CHECK(sim.ChestUsedCapacity(chest->id) <= ChestCapacity);
    // Nothing more appears on reload: the chest and wardrobe round-trip exactly.
    Simulation reloaded;
    reloaded.SetPlacements(ProvisionalEstatePlacements());
    OK(reloaded.Deserialize(sim.Serialize()));
    CHECK(reloaded.Serialize() == sim.Serialize());
    const Structure* again = Find(reloaded.GetState(), Piece::Chest);
    CHECK(again->storage[static_cast<int>(Item::Pasty)] == 3 && again->storage[static_cast<int>(Item::Bread)] == 2);
    CHECK(reloaded.GetState().wearables.size() == 8);
    // Loading never restocks it: take the food and clothes out, save and reload, and the chest stays bare.
    Simulation emptied = NewEstate();
    const int chestId = Find(emptied.GetState(), Piece::Chest)->id;
    const Point chestSide = StructureCenter(emptied.GetState(), *Find(emptied.GetState(), Piece::Chest));
    OK(emptied.Transfer(chestId, Item::Pasty, -3, chestSide));
    OK(emptied.Transfer(chestId, Item::Bread, -2, chestSide));
    Simulation bareLoad;
    bareLoad.SetPlacements(ProvisionalEstatePlacements());
    OK(bareLoad.Deserialize(emptied.Serialize()));
    const Structure* bare = Find(bareLoad.GetState(), Piece::Chest);
    CHECK(bare->storage[static_cast<int>(Item::Pasty)] == 0 && bare->storage[static_cast<int>(Item::Bread)] == 0);
    CHECK(bareLoad.Count(Item::Pasty) == 3 && bareLoad.Count(Item::Bread) == 2 && bareLoad.GetState().wearables.size() == 8);    // The hoe blade is the second rusted head, after the billhook.
    State order = state;
    CHECK(NextSalvageHead(order) == Item::RustedBillhookHead);
    ++order.inventory[static_cast<int>(Item::Billhook)];
    CHECK(NextSalvageHead(order) == Item::RustedHoeBlade);
    ++order.inventory[static_cast<int>(Item::RustedHoeBlade)];
    CHECK(NextSalvageHead(order) == Item::RustedAxeHead);
    ++order.inventory[static_cast<int>(Item::Hatchet)];
    CHECK(NextSalvageHead(order) == Item::RustedScytheBlade);
    ++order.inventory[static_cast<int>(Item::Scythe)];
    CHECK(NextSalvageHead(order) == Item::RustedPickHead);
    ++order.inventory[static_cast<int>(Item::Pickaxe)];
    CHECK(NextSalvageHead(order) == Item::Count);
    // Two searches in a new game: billhook first, then the hoe blade.
    Simulation search = NewEstate();
    for (int pile : {520001, 520002})
    {
        Point at{};
        for (const auto& node : search.GetState().resources) if (node.id == pile) at = node.position;
        OK(search.ClearOvergrowth(pile, Item::Count, at));
    }
    CHECK(search.Count(Item::RustedBillhookHead) == 1 && search.Count(Item::RustedHoeBlade) == 1);
    CHECK(search.Count(Item::RustedAxeHead) == 0);
    // Tilling without a hoe says where to look; the journal says where the tools hung.
    const int gx = GardenCell(Spawn().x + 900.0), gy = GardenCell(Spawn().y);
    const Result till = search.Till(gx, gy, GardenCellCenter(gx, gy));
    // She holds the blade now, so it says to haft it; before that it names the nearest unsearched pile.
    CHECK(!till.ok && till.message == "You need a hoe to till. Craft one from your rusted hoe blade and two branches on the Craft page.");
    Simulation fresh = NewEstate();
    const Result lost = fresh.Till(gx, gy, GardenCellCenter(gx, gy));
    CHECK(!lost.ok && StartsWith(lost.message, "You need a hoe to till. Search the old manor's salvage for a hoe blade: there's a pile "));
    CHECK(std::string(SalvageWhereabouts(520006)) == "by the chimney in the west rooms, where Father's tools hung");
    const std::string arrival = Manor::JournalText(Manor::ArrivalEntry, state);
    CHECK(arrival.find("pasties and bread") != std::string::npos && arrival.find("change of clothes") != std::string::npos);
    CHECK(arrival.find("Father's garden tools always hung in the west rooms, by the chimney") != std::string::npos);
}

// The placement table as it stood before the tool rack (main 9a409e07): every id, kind and spot.
std::uint64_t PlacementHashWithout(int skippedId, int& count)
{
    std::uint64_t hash = UINT64_C(14695981039346656037);
    count = 0;
    char line[256];
    for (const auto& p : ProvisionalEstatePlacements().placements)
    {
        // The rack, and the later forage sections appended after it was pinned (roadside 581000-581999,
        // woods and hedges 582100-582299), so the hash still covers exactly the placements older saves know.
        if (p.id == skippedId || (p.id >= 581000 && p.id < 582000) || (p.id >= 582100 && p.id < 582300)) continue;
        std::snprintf(line, sizeof line, "%d %d %.3f %.3f %.3f %.3f %.3f %d\n", p.id, static_cast<int>(p.kind),
            p.position.x, p.position.y, p.z, p.yaw, p.scale, p.minTier);
        for (const char* c = line; *c; ++c) { hash ^= static_cast<unsigned char>(*c); hash *= UINT64_C(1099511628211); }
        ++count;
    }
    return hash;
}

void ToolRackIsSaveSafe()
{
    // Jenny's own save couldn't reach a hoe. Father's tool rack (520006) is a salvage pile appended
    // after every other section, so no earlier id, kind or spot moves: an older save's cleared nodes
    // still name the same things, and the rack waits unsearched with whichever head she lacks.
    int count = 0;
    const std::uint64_t before = PlacementHashWithout(520006, count);
    CHECK(count == 2197 && before == UINT64_C(12311480322052281513));
    const auto& all = ProvisionalEstatePlacements().placements;
    CHECK(all.back().id == 520006 && all.back().kind == ResourceKind::SalvagePile);
    const Point rack = all.back().position;
    CHECK(PointInPolygon(ProvisionalEstateLayout().FindPolygon(Anchor::ManorFootprint)->points, rack));
    for (const auto& other : all)
        if (other.id != 520006) CHECK(std::hypot(other.position.x - rack.x, other.position.y - rack.y) >= 150.0);

    // An older save (no rack in its table) with the billhook pile searched and a nettle pulled.
    EstatePlacements older = ProvisionalEstatePlacements();
    older.placements.pop_back();
    Simulation old;
    old.SetPlacements(older);
    OK(old.NewEstateGame(ProvisionalEstateLayout(), older));
    auto at = [](const Simulation& sim, int id) { for (const auto& n : sim.GetState().resources) if (n.id == id) return n; return ResourceNode{}; };
    OK(old.ClearOvergrowth(520001, Item::Count, at(old, 520001).position));
    CHECK(old.Count(Item::RustedBillhookHead) == 1);
    int nettle = 0;
    for (const auto& n : old.GetState().resources) if (n.kind == ResourceKind::Nettles && n.id >= 570000) { nettle = n.id; break; }
    OK(old.ClearOvergrowth(nettle, Item::Count, at(old, nettle).position));
    const std::string saved = old.Serialize();
    // Loaded by this build: the same nodes are cleared, the rack is new and unsearched.
    Simulation loaded;
    loaded.SetPlacements(ProvisionalEstatePlacements());
    OK(loaded.Deserialize(saved));
    CHECK(at(loaded, 520001).cleared && at(loaded, nettle).cleared && !at(loaded, 520006).cleared);
    int cleared = 0, oldCleared = 0;
    for (const auto& n : loaded.GetState().resources) cleared += n.cleared;
    for (const auto& n : old.GetState().resources) oldCleared += n.cleared;
    CHECK(cleared == oldCleared);
    // Her next search, the rack included, gives the hoe blade; never a second billhook.
    OK(loaded.ClearOvergrowth(520006, Item::Count, at(loaded, 520006).position));
    CHECK(loaded.Count(Item::RustedHoeBlade) == 1 && loaded.Count(Item::RustedBillhookHead) == 1);
    // With every other pile searched and every head owned, the rack gives only scrap.
    Simulation full = NewEstate();
    for (int pile = 520001; pile <= 520005; ++pile) OK(full.ClearOvergrowth(pile, Item::Count, at(full, pile).position));
    const int heads = full.Count(Item::RustedBillhookHead) + full.Count(Item::RustedHoeBlade) + full.Count(Item::RustedAxeHead)
        + full.Count(Item::RustedScytheBlade) + full.Count(Item::RustedPickHead);
    CHECK(heads == 5);
    const int scrap = full.Count(Item::ScrapIron);
    OK(full.ClearOvergrowth(520006, Item::Count, at(full, 520006).position));
    CHECK(full.Count(Item::RustedHoeBlade) == 1 && full.Count(Item::ScrapIron) == scrap + 1);
    // A lost blade (nothing of the hoe anywhere) is exactly what the rack would give.
    State lost = full.GetState();
    lost.inventory[static_cast<int>(Item::RustedHoeBlade)] = 0;
    CHECK(NextSalvageHead(lost) == Item::RustedHoeBlade);
    // Once all six are searched, the refusal says so plainly.
    CHECK(NoHoeMessage(lost, Spawn()) == "You need a hoe to till, and the manor's salvage has all been searched.");
    // The same exhausted estate with the blade set down 12 m off: she's sent to it, not told it's gone.
    State dropped = lost;
    dropped.worldDrops.push_back({dropped.nextId++, {Spawn().x + 1200.0, Spawn().y}, Item::RustedHoeBlade, 1, 0});
    CHECK(NextSalvageHead(dropped) != Item::RustedHoeBlade);
    CHECK(NoHoeMessage(dropped, Spawn()) == "You need a hoe to till. Your rusted hoe blade is lying on the ground "
        "about 12 m away: pick it up and craft a hoe with two branches on the Craft page.");
}

// Architecture review: the till refusal looks where NextSalvageHead does (pack, chests, the ground)
// and sends her to what she already owns before it mentions salvage.
void HoeHintFindsWhatSheOwns()
{
    Simulation sim = NewEstate();
    const State& state = sim.GetState();
    const Structure* chest = Find(state, Piece::Chest);
    const int chestId = chest->id;
    const Point chestSide = StructureCenter(state, *chest);
    const Point far{chestSide.x + 1500.0, chestSide.y};
    CHECK(sim.Count(Item::DiggingStick) == 0 && sim.Count(Item::RustedHoeBlade) == 0);
    CHECK(StartsWith(NoHoeMessage(state, chestSide), "You need a hoe to till. Search the old manor's salvage"));

    // A blade in her pack: craft it.
    OK(sim.GrantItems(Item::RustedHoeBlade, 1));
    CHECK(NoHoeMessage(sim.GetState(), chestSide)
        == "You need a hoe to till. Craft one from your rusted hoe blade and two branches on the Craft page.");
    // Stored in the chest: take it out, and how far off the chest is.
    OK(sim.Transfer(chestId, Item::RustedHoeBlade, 1, chestSide));
    CHECK(sim.Count(Item::RustedHoeBlade) == 0);
    CHECK(NoHoeMessage(sim.GetState(), chestSide) == "You need a hoe to till. Your rusted hoe blade is in a storage "
        "chest right here: take it out and craft a hoe with two branches on the Craft page.");
    CHECK(NoHoeMessage(sim.GetState(), far) == "You need a hoe to till. Your rusted hoe blade is in a storage "
        "chest about 15 m away: take it out and craft a hoe with two branches on the Craft page.");

    // A finished hoe in the chest, or set down on the ground, beats the blade and the salvage.
    State stored = sim.GetState();
    for (auto& piece : stored.structures)
        if (piece.id == chestId) piece.storage[static_cast<int>(Item::DiggingStick)] = 1;
    CHECK(NoHoeMessage(stored, far) == "Your hoe is in a storage chest about 15 m away. Take it out to till.");
    State set = sim.GetState();
    set.worldDrops.push_back({set.nextId++, {far.x + 300.0, far.y}, Item::DiggingStick, 1, 0});
    CHECK(NoHoeMessage(set, far) == "Your hoe is lying on the ground about 3 m away. Pick it up to till.");
    CHECK(NoHoeMessage(set, {far.x + 350.0, far.y}) == "Your hoe is lying on the ground right here. Pick it up to till.");
    // Of two places, the nearer one is named; a wearable drop never counts.
    set.worldDrops.push_back({set.nextId++, {far.x + 300.0, far.y}, Item::DiggingStick, 1, 7});
    CHECK(NoHoeMessage(set, chestSide).find("Your hoe is lying on the ground about 18 m away") == 0);

    // The refusal the Till transaction gives is this message, and it changes nothing.
    const std::string before = sim.Serialize();
    const int gardenX = GardenCell(far.x), gardenY = GardenCell(far.y);
    const Point plot = GardenCellCenter(gardenX, gardenY);
    const auto till = sim.Till(gardenX, gardenY, plot);
    CHECK(!till.ok && till.message == NoHoeMessage(sim.GetState(), plot) && sim.Serialize() == before);
    CHECK(till.message.find("in a storage chest about") != std::string::npos);

    // An old save (the rack not yet in its table) with the blade in its chest loads and gives the same hint.
    EstatePlacements older = ProvisionalEstatePlacements();
    older.placements.pop_back();
    Simulation loaded;
    loaded.SetPlacements(ProvisionalEstatePlacements());
    Simulation old;
    old.SetPlacements(older);
    OK(old.NewEstateGame(ProvisionalEstateLayout(), older));
    OK(old.GrantItems(Item::RustedHoeBlade, 1));
    OK(old.Transfer(Find(old.GetState(), Piece::Chest)->id, Item::RustedHoeBlade, 1, chestSide));
    OK(loaded.Deserialize(old.Serialize()));
    CHECK(NoHoeMessage(loaded.GetState(), chestSide) == NoHoeMessage(sim.GetState(), chestSide));
    CHECK(NextSalvageHead(loaded.GetState()) == Item::RustedBillhookHead);
}

int main()
{
    SeededStandingRoom();
    std::cout << "PASS seeded heritage standing room\n";
    SleepChestAndHearth();
    std::cout << "PASS sleep, chest and hearth cooking\n";
    ManorFootprintReservation();
    std::cout << "PASS manor footprint reservation\n";
    NamesValidationAndPersistence();
    std::cout << "PASS names validation and persistence\n";
    DerelictFarmAndDisrepair();
    std::cout << "PASS derelict farm and estate disrepair\n";
    StarterChestAndHoeClue();
    std::cout << "PASS starter chest, hoe blade second and its clues\n";
    ToolRackIsSaveSafe();
    std::cout << "PASS the tool rack is save-safe and finds her a hoe\n";
    HoeHintFindsWhatSheOwns();
    std::cout << "PASS the till hint sends her to the hoe or blade she owns\n";
    std::cout << checks << " checks passed.\n";
    return 0;
}
