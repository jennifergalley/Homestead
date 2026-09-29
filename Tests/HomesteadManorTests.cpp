// add-ruined-manor-and-arrival: the heritage standing room, the reserved manor footprint and the
// new-game names. Kept apart from HomesteadSimulationTests so it builds and runs in seconds.
#include "HomesteadEstate.h"
#include "HomesteadManor.h"
#include "HomesteadOvergrowth.h"
#include "HomesteadSimulation.h"

#include <cmath>
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
    OK(sim.Craft(Recipe::RoastedRoots, hearthSide));
    CHECK(sim.Count(Item::Roots) == 0 && sim.Count(Item::RoastedRoots) == 1);
    CHECK(!sim.Craft(Recipe::RoastedRoots, hearthSide));
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
    CHECK(!till.ok && till.message == "You need a hoe to till. Search the salvage in the old manor for a hoe blade.");
    const std::string arrival = Manor::JournalText(Manor::ArrivalEntry, state);
    CHECK(arrival.find("pasties and bread") != std::string::npos && arrival.find("change of clothes") != std::string::npos);
    CHECK(arrival.find("Father's garden tools always hung in the west rooms, by the chimney") != std::string::npos);
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
    std::cout << checks << " checks passed.\n";
    return 0;
}
