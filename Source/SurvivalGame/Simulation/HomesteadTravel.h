#pragma once

#include "HomesteadEstate.h"
#include "HomesteadSimulation.h"

#include <string>
#include <vector>

// Walking the public road between the manor and town ("fast travel"): how far and how long the walk
// is from where she stands, measured along the road (HomesteadEstatePublicRoad.h), and the preview
// the Map tab and the road signs show before she agrees. The walk itself is Simulation::WalkRoad
// (HomesteadTravel.cpp): the clock, crops, weather and her vitals run for the whole walk.
namespace Homestead
{
// A conservative ordinary walking pace, cm/s: the legacy heroine's walk speed, under the MetaHuman's
// declared 210 cm/s top walk, because slopes, starts and stops slow a real walk (not measured in
// PIE yet). The full 852 m road reads about 3.2 game hours at the default 60-minute day.
constexpr double RoadWalkPaceCmPerSecond = 180.0;
// Closer than this to the destination, she is already there, cm.
constexpr double TravelArrivedCm = 3000.0;
// Farther than this from the road, she walks back to it herself first, cm.
constexpr double TravelMaxConnectorCm = 60000.0;
// Within this of the town square she is already in town (the square, its streets and the general
// store 26 m off it), cm; beside the store (ShopWaitReach of its counter) she's "at the store".
constexpr double TravelTownReachCm = 4500.0;

enum class TravelDestination : int { Manor, Town, Mine, Cove, Mill, Gateway, Store, Count };
constexpr int TravelDestinationCount = static_cast<int>(TravelDestination::Count);

// "the manor" / "town", for sentences ("Walk to town").
const char* TravelDestinationName(TravelDestination destination);
const char* TravelDestinationLabel(TravelDestination destination);
bool IsTravelUnlocked(const State& state, TravelDestination destination);
bool TravelVisitNear(Point at, TravelDestination destination, const EstateLayout& layout);
namespace TravelDiscovery
{
constexpr const char* SaveTag = "travel";
void WriteSaveSection(std::ostream& output, const State& state);
bool ReadSaveSection(std::istream& input, State& state);
}

struct TravelPlan
{
    bool ok = false;
    std::string error;
    TravelDestination destination = TravelDestination::Town;
    // Where she arrives: on the road's centreline at the destination stop, facing along the road
    // the way she was walking (Unreal yaw, degrees).
    Point arrival;
    double arrivalZ = 0.0;
    double arrivalYaw = 0.0;
    double connectorMetres = 0.0; // From where she stands to the nearest point of the road.
    double roadMetres = 0.0;      // Along the road from there to the stop.
    double totalMetres = 0.0;
    double gameHours = 0.0;
    double arrivalHour = 0.0;     // Absolute State::hour when she gets there.
    // She'd arrive after midnight.
    bool nextDay = false;
    bool storeClosedOnArrival = false;
    // She'd arrive on a day the shops keep closed (ShopClosedDay), in what would be their hours.
    bool storeClosedAllDay = false;
    double storeOpenHour = 8.0;
    // The preview: distance, time, arrival and any closed-shop warning, one fact per line.
    std::string summary;
};

// The walk from `from` to `destination` under the saved day length. Fails (with `error`) away
// from the estate's road, when she's already there (in town or at the general store, for Town),
// or while she needs to recover.
TravelPlan PlanTravel(const State& state, Point from, TravelDestination destination,
    const EstateLayout& layout = ProvisionalEstateLayout());
// Whether `at` is close enough to the public road for a walk to start from it (TravelMaxConnectorCm). A save
// made where the road no longer runs (the old town site) fails this, and the load puts her back at the manor.
bool WithinTravelReachOfRoad(Point at);
// "7 h 12 min", "45 min".
std::string FormatWalkDuration(double gameHours);

struct PublicRoadSign;
// The road signs (HomesteadEstatePublicRoad.h PublicRoad::signs) are wooden fingerposts that only point the way:
// the manor's sign points toward the village, the village's toward the manor, the gateway's both ways. They don't
// travel; walking is the Map tab's and the places she has visited.
// The words painted on it ("To the village", "To the manor", "Village / Manor"); empty for any other name.
std::string RoadSignLabel(const std::string& signName);
// The yaw (degrees, 0..360) its upper arm points along: the road's heading toward the village for the manor and
// gateway signs, toward the manor for the village's. The gateway's second arm points the opposite way.
double RoadSignArmYaw(const PublicRoadSign& sign);
}
