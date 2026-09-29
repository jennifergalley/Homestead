#pragma once

#include "HomesteadSimulation.h"

#include <string>

// Walking the public road between the manor and town ("fast travel"): how far and how long the walk
// is from where she stands, measured along the road (HomesteadEstatePublicRoad.h), and the preview
// the Map tab and the road signs show before she agrees. The walk itself is Simulation::WalkRoad
// (HomesteadTravel.cpp): the clock, crops, weather and her vitals run for the whole walk.
namespace Homestead
{
// A conservative ordinary walking pace, cm/s: the legacy heroine's walk speed, under the MetaHuman's
// declared 210 cm/s top walk, because slopes, starts and stops slow a real walk (not measured in
// PIE yet). The full 1.94 km road reads about 7.2 game hours at the default 60-minute day.
constexpr double RoadWalkPaceCmPerSecond = 180.0;
// Closer than this to the destination, she is already there, cm.
constexpr double TravelArrivedCm = 3000.0;
// Farther than this from the road, she walks back to it herself first, cm.
constexpr double TravelMaxConnectorCm = 60000.0;

enum class TravelDestination : int { Manor, Town };

// "the manor" / "town", for sentences ("Walk to town").
const char* TravelDestinationName(TravelDestination destination);

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
    double storeOpenHour = 8.0;
    // The preview: distance, time, arrival and any closed-shop warning, one fact per line.
    std::string summary;
};

// The walk from `from` to `destination` under the saved day length. Fails (with `error`) away
// from the estate's road, when she's already there, or while she needs to recover.
TravelPlan PlanTravel(const State& state, Point from, TravelDestination destination);
// "7 h 12 min", "45 min".
std::string FormatWalkDuration(double gameHours);
}
