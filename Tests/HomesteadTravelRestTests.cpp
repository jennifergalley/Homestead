#include "HomesteadBed.h"
#include "HomesteadCrops.h"
#include "HomesteadEstate.h"
#include "HomesteadEstatePublicRoad.h"
#include "HomesteadTravel.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <sstream>

namespace
{
int TravelRestChecks = 0;
void Check(bool condition, const char* expression, int line)
{
    ++TravelRestChecks;
    if (!condition) { std::cerr << "FAIL " << line << ": " << expression << '\n'; std::exit(1); }
}
#define CHECK(expression) Check(static_cast<bool>(expression), #expression, __LINE__)
bool Close(double a, double b) { return std::abs(a - b) < 1e-5; }

Homestead::Simulation Estate()
{
    Homestead::Simulation sim;
    Homestead::EstatePlacements empty;
    empty.bakeVersion = Homestead::ProvisionalEstatePlacements().bakeVersion;
    CHECK(sim.NewEstateGame(Homestead::ProvisionalEstateLayout(), empty));
    return sim;
}

std::string Reseal(const std::string& saved, const std::string& payload)
{
    std::uint64_t hash = UINT64_C(14695981039346656037);
    for (unsigned char c : payload) { hash ^= c; hash *= UINT64_C(1099511628211); }
    std::istringstream header(saved.substr(0, saved.find('\n')));
    std::string magic, version;
    header >> magic >> version;
    return magic + " " + version + " " + std::to_string(payload.size()) + " " + std::to_string(hash) + "\n" + payload;
}

void SleepTiming()
{
    for (double energy : {0.0, 20.0, 60.0, 99.9, 100.0})
    {
        for (double hour : {18.0, 21.0, 23.99, 24.0, 29.875})
        {
            const auto offer = Homestead::BedSleepOption(hour, energy);
            CHECK(offer && offer->choice == Homestead::SleepChoice::UntilMorning);
            CHECK(Close(hour + offer->hours, 30.0) && Close(offer->wakeHour, 6.0));
        }
    }
    const auto early = Homestead::BedSleepOption(16.5, 60.0, 5.0, 16.0);
    CHECK(early && early->choice == Homestead::SleepChoice::UntilMorning
        && Close(early->hours, 12.5) && Close(early->wakeHour, 5.0));
    const auto lateSun = Homestead::BedSleepOption(18.0, 60.0, 7.0, 20.0);
    CHECK(lateSun && Close(lateSun->hours, 12.0) && Close(lateSun->wakeHour, 6.0));
    const auto day = Homestead::BedSleepOption(12.0, 60.0);
    CHECK(day && day->choice == Homestead::SleepChoice::UntilRested && Close(day->hours, 4.0));
    CHECK(!Homestead::BedSleepOption(6.0, 100.0));
    CHECK(!Homestead::BedSleepOption(12.0, 60.0, std::numeric_limits<double>::quiet_NaN(), 18.0));
    CHECK(!Homestead::BedSleepOption(12.0, 60.0, 18.0, 6.0));
    CHECK(Close(Homestead::Daylight::SolarElevation(6.0), 0.0));
    CHECK(Close(Homestead::Daylight::SolarElevation(18.0), 0.0));
    Homestead::Simulation sim = Estate();
    const auto bed = std::find_if(sim.GetState().structures.begin(), sim.GetState().structures.end(),
        [](const Homestead::Structure& piece) { return piece.kind == Homestead::Piece::Bed; });
    CHECK(bed != sim.GetState().structures.end());
    const Homestead::Point at = sim.StructureCenter(*bed);
    CHECK(Homestead::ReachableBed(sim.GetState(), at, {1.0, 0.0}) != -1);
    sim.SkipToHourOfDay(21.0);
    CHECK(sim.SetEnergy(60.0));
    const auto offer = Homestead::BedSleepOption(sim.GetState().hour, sim.GetState().energy);
    const double before = sim.GetState().hour;
    CHECK(offer && sim.Sleep(offer->hours, at, {1.0, 0.0}, true));
    CHECK(Close(sim.GetState().hour, before + 9.0) && Close(sim.GetState().energy, 100.0));
}

void DiscoveryAndSaves()
{
    Homestead::Simulation sim = Estate();
    const auto& layout = sim.Layout();
    const auto from = layout.PointOr(Homestead::Anchor::StandingRoomSpawn, {});
    CHECK(Homestead::IsTravelUnlocked(sim.GetState(), Homestead::TravelDestination::Manor));
    const std::string legacy = sim.Serialize();
    CHECK(legacy.find("\ntravel ") == std::string::npos);
    const char* anchors[] = {Homestead::Anchor::StandingRoomSpawn, Homestead::Anchor::TownSquare,
        Homestead::Anchor::MineEntrance, Homestead::Anchor::CoveBeach, Homestead::Anchor::MillSite,
        Homestead::Anchor::EstateGateway, Homestead::Anchor::GeneralStoreDoor};
    static_assert(sizeof(anchors) / sizeof(anchors[0]) == Homestead::TravelDestinationCount);
    for (int index = 1; index < Homestead::TravelDestinationCount; ++index)
    {
        const auto destination = static_cast<Homestead::TravelDestination>(index);
        const auto revision = sim.GetRevision();
        const std::string before = sim.Serialize();
        CHECK(!Homestead::PlanTravel(sim.GetState(), from, destination, layout).ok);
        CHECK(!sim.WalkRoad(destination, from));
        CHECK(!sim.DiscoverTravel(destination, {std::numeric_limits<double>::quiet_NaN(), 0.0}));
        CHECK(sim.Serialize() == before && sim.GetRevision() == revision);
        const auto* place = layout.FindLandmark(anchors[index]);
        CHECK(place != nullptr && Homestead::TravelVisitNear(place->position, destination, layout));
        const auto discovered = sim.DiscoverTravel(destination, place->position);
        CHECK(discovered && discovered.message == std::string("Fast Travel Destination Unlocked: ")
            + Homestead::TravelDestinationLabel(destination));
        CHECK(Homestead::IsTravelUnlocked(sim.GetState(), destination));
        const auto plan = Homestead::PlanTravel(sim.GetState(), from, destination, layout);
        CHECK(plan.ok && plan.gameHours > 0.0);
        Homestead::Simulation traveller = sim;
        CHECK(traveller.WalkRoad(destination, from));
        CHECK(Close(traveller.GetState().hour, sim.GetState().hour + plan.gameHours));
        CHECK(!sim.DiscoverTravel(destination, place->position));
    }
    CHECK(sim.Serialize().find("\ntravel 6 1 2 3 4 5 6\n") != std::string::npos);
    Homestead::Simulation loaded = Estate();
    CHECK(loaded.Deserialize(sim.Serialize()));
    CHECK(loaded.GetState().discoveredTravel == sim.GetState().discoveredTravel);
    CHECK(loaded.Serialize() == sim.Serialize());
    CHECK(loaded.Deserialize(legacy));
    CHECK(loaded.GetState().discoveredTravel.empty()
        && Homestead::IsTravelUnlocked(loaded.GetState(), Homestead::TravelDestination::Manor));
    CHECK(!Homestead::IsTravelUnlocked(loaded.GetState(), Homestead::TravelDestination::Town));
    const std::string payload = legacy.substr(legacy.find('\n') + 1);
    for (const char* tail : {"travel -1\n", "travel 7 1 2 3 4 5 6 7\n", "travel 1 0\n",
        "travel 1 7\n", "travel 1 -1\n", "travel 2 1 1\n", "travel 2 2 1\n", "travel 1\n",
        "travel 1 1\ntravel 1 2\n", "travel 0\ntravel 0\n"})
    {
        const auto revision = sim.GetRevision();
        const auto saved = sim.Serialize();
        const auto result = sim.Deserialize(Reseal(legacy, payload + tail));
        CHECK(!result && result.code == Homestead::ResultCode::CorruptSave);
        CHECK(sim.Serialize() == saved && sim.GetRevision() == revision);
    }
    for (const auto destination : Homestead::RoadSignDestinations("ManorRoadSign"))
    {
        CHECK(destination == Homestead::TravelDestination::Town);
        const auto* sign = Homestead::EstatePublicRoad().FindSign("ManorRoadSign");
        CHECK(sign && !Homestead::PlanTravel(loaded.GetState(), sign->position, destination).ok);
        CHECK(!loaded.WalkRoad(destination, sign->position));
        CHECK(Homestead::PlanTravel(sim.GetState(), sign->position, destination).ok);
    }
    const auto* town = Homestead::EstatePublicRoad().FindStop("Town");
    const auto* manor = Homestead::EstatePublicRoad().FindStop("Manor");
    CHECK(town && manor);
    const auto home = Homestead::PlanTravel(sim.GetState(), town->position, Homestead::TravelDestination::Manor);
    CHECK(home.ok && home.arrival.x == manor->arrival.x && home.arrival.y == manor->arrival.y);
    CHECK(home.arrival.x == -26340.0 && home.arrival.y == -64920.0);
    CHECK(!sim.DiscoverTravel(static_cast<Homestead::TravelDestination>(99), from));
}

void PlantPotato(Homestead::Simulation& sim)
{
    const auto spawn = sim.Layout().PointOr(Homestead::Anchor::StandingRoomSpawn, {});
    const int x = Homestead::GardenCell(spawn.x) + 10, y = Homestead::GardenCell(spawn.y) + 10;
    const auto at = Homestead::GardenCellCenter(x, y);
    CHECK(sim.GrantItems(Homestead::Item::DiggingStick, 1));
    CHECK(sim.GrantItems(Homestead::Item::SeedPotato, 1));
    CHECK(sim.Till(x, y, at));
    CHECK(sim.Plant(sim.FindNearestPlot(at, 1.0), at, Homestead::CropKind::Potatoes));
    CHECK(sim.GetState().plots.size() == 1);
}

void SundayWait()
{
    const auto& layout = Homestead::ProvisionalEstateLayout();
    const auto door = layout.PointOr(Homestead::Anchor::GeneralStoreDoor, {});
    for (double time : {6.0, 7.0, 12.0, 19.0, 23.0})
    {
        Homestead::Simulation sim = Estate();
        for (int index = 0; index < 6; ++index) sim.SkipToHourOfDay(6.0);
        if (time > 6.0) sim.SkipToHourOfDay(time);
        CHECK(sim.Today().weekday == Homestead::Weekday::Sunday);
        PlantPotato(sim);
        CHECK(sim.SetEnergy(0.0));
        const auto* shop = sim.FindShop(Homestead::ShopKind::GeneralStore);
        CHECK(shop && Homestead::CanWaitForShop(*shop, sim.GetState().hour));
        const int id = shop->id;
        const double opening = Homestead::NextShopOpening(*shop, sim.GetState().hour);
        const auto saved = sim.Serialize();
        const auto revision = sim.GetRevision();
        CHECK(!sim.WaitForShop(id, {door.x + 10000.0, door.y}));
        CHECK(!sim.WaitForShop(id, {std::numeric_limits<double>::quiet_NaN(), door.y}));
        CHECK(!sim.WaitForShop(id + 999, door));
        CHECK(sim.Serialize() == saved && sim.GetRevision() == revision);
        Homestead::Simulation ordinary = sim;
        ordinary.AdvanceGameHours(opening - sim.GetState().hour + 1e-6, door);
        CHECK(sim.WaitForShop(id, door));
        CHECK(sim.Today().weekday == Homestead::Weekday::Monday && Close(sim.GetState().hour, opening));
        CHECK(Homestead::IsShopOpen(*sim.FindShop(id), sim.GetState().hour));
        CHECK(sim.Serialize() == ordinary.Serialize());
        CHECK(sim.GetState().energy == 0.0 && sim.DozeCount() == ordinary.DozeCount());
        CHECK(sim.GetState().plots.front().growth > 0.0);
        CHECK(Close(sim.GetState().plots.front().growth, ordinary.GetState().plots.front().growth));
        CHECK(Close(sim.GetState().plots.front().moisture, ordinary.GetState().plots.front().moisture));
    }
    // Last Sunday of Spring: Monday opening includes the authoritative Summer rollover.
    Homestead::Simulation seasonal = Estate();
    CHECK(seasonal.PassDaysForPlaytest(27.0, false, door));
    seasonal.SkipToHourOfDay(12.0);
    CHECK(seasonal.Today().weekday == Homestead::Weekday::Sunday);
    PlantPotato(seasonal);
    const auto* shop = seasonal.FindShop(Homestead::ShopKind::GeneralStore);
    CHECK(shop && seasonal.Today().season == Homestead::Season::Spring);
    CHECK(seasonal.WaitForShop(shop->id, door));
    CHECK(seasonal.Today().weekday == Homestead::Weekday::Monday
        && seasonal.Today().season == Homestead::Season::Summer && seasonal.SeasonChanges() == 1);
    CHECK(seasonal.GetState().plots.front().withered && seasonal.LastSeasonChange().witheredPlots == 1);
}
}

int main()
{
    SleepTiming();
    DiscoveryAndSaves();
    SundayWait();
    std::cout << TravelRestChecks << " travel/rest checks passed.\n";
}
