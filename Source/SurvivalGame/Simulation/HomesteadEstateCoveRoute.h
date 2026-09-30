#pragma once

#include "HomesteadEstate.h"

#include <vector>

// The cove route: an on-foot path from the manor's fallen south front door down to the sand at the head of
// the cove, with granite steps down the steep valley side (Scripts/Terrain/cove_route.py designs it, grades
// the heightfield to it and generates HomesteadEstateCoveRoute.inc). The runtime builder places Props'
// add-cove-route-kit pieces from this data; the pivots and axes below are the kit's.
//
// World frame as HomesteadEstate.h: Unreal centimetres, +X north, +Y east; yaw in degrees from +X toward +Y.
namespace Homestead
{
struct CoveRouteStation
{
    Point position;           // on the centreline (cm)
    double walkZ = 0.0;       // what she stands on: the path, or the tread or landing there (cm)
    double bedZ = 0.0;        // the graded ground (cm): under a tread or landing, a few cm below it
    double metres = 0.0;      // along the route from the front door
    bool onSteps = false;
};

// A straight flight. Tread i's pivot (top face, centre of its front nosing) is start + i * (going along
// yaw, rise); +X runs up the flight. The top tread is level with the landing or path above it.
struct CoveRouteFlight
{
    Point start;              // tread 0's pivot (cm)
    double z = 0.0;           // tread 0's top (cm)
    double yaw = 0.0;         // up the flight
    double rise = 0.0;        // cm
    double going = 0.0;       // cm
    int treads = 0;
    double railPitch = 0.0;   // the raked rail bay it takes: 26, 28 or 30 degrees

    Point TreadPivot(int i) const;
    double TreadZ(int i) const { return z + i * rise; }
    double PitchDegrees() const;
};

// A granite landing: pivot at its front (downhill) edge's centre on its top; +X up the steps.
struct CoveRouteLanding
{
    Point position;
    double z = 0.0;
    double yaw = 0.0;
    double length = 0.0;      // cm along yaw
};

// A 1 m kerb piece: pivot on its path-side top edge; +X along the path, +Y toward the drop.
struct CoveRouteKerb
{
    Point position;
    double z = 0.0;
    double yaw = 0.0;
};

// An oak rail bay: pivot at its downhill post's foot on the path (on a flight, on the nosing line); +X along
// the path (uphill when raked), +Y toward the drop. A mirrored bay has the drop on its -Y: scale it by -1 in
// Y (a raked bay turned round would slope the wrong way).
struct CoveRouteRail
{
    Point position;
    double z = 0.0;
    double yaw = 0.0;
    double pitch = 0.0;       // 0 level, else 26, 28 or 30 degrees
    double length = 0.0;      // cm in plan
    bool mirrored = false;
};

// The graded heightfield under a station (cove_route.py reads it back from the r16 it cut): on the centreline
// and CoveRouteGroundSampleCm either side (left and right looking along the route).
struct CoveRouteGround
{
    double metres = 0.0;
    double leftZ = 0.0;
    double centreZ = 0.0;
    double rightZ = 0.0;
};

// The graded ground just beyond a flight's foot and head (cove_route.py's END_SAMPLE_M along its axis, on the
// axis and END_SIDE_M either side): where she steps on and off it.
struct CoveRouteFlightEnds
{
    double foot[3] = {0.0, 0.0, 0.0};
    double head[3] = {0.0, 0.0, 0.0};
};

// Where two stair legs meet at a corner landing: the turning point on the landing's top, the landing's leg's
// up direction (the landing runs CoveRouteCornerHalfCm past the point along it) and the other leg's.
struct CoveRouteCorner
{
    Point position;
    double z = 0.0;
    double landingYaw = 0.0;
    double otherYaw = 0.0;
};

// "To the Cove": pivot at the post's foot; the arm points along yaw.
struct CoveRouteFingerpost
{
    Point position;
    double z = 0.0;
    double yaw = 0.0;
};

struct CoveRoute
{
    std::vector<CoveRouteStation> stations;
    std::vector<CoveRouteGround> ground;      // one per station, in step
    std::vector<CoveRouteFlight> flights;
    std::vector<CoveRouteFlightEnds> flightEnds;   // one per flight, in step
    std::vector<CoveRouteLanding> landings;
    std::vector<CoveRouteCorner> corners;
    std::vector<CoveRouteKerb> kerbs;
    std::vector<CoveRouteRail> rails;
    std::vector<CoveRouteFingerpost> fingerposts;

    double Length() const { return stations.empty() ? 0.0 : stations.back().metres; }
    int Steps() const;
    // Nearest centreline station: its index and distance (cm).
    struct Nearest
    {
        int station = -1;
        double distanceCm = 0.0;
    };
    Nearest NearestTo(Point world) const;
};

const CoveRoute& EstateCoveRoute();

// Props' kit limits (add-cove-route-kit design.md), in cm and degrees.
constexpr double CoveRouteMinRiseCm = 15.0;
constexpr double CoveRouteMaxRiseCm = 17.0;
constexpr double CoveRouteMinGoingCm = 30.0;
constexpr double CoveRouteMaxGoingCm = 35.0;
constexpr int CoveRouteMaxRisers = 12;
constexpr double CoveRouteMinLandingCm = 120.0;
constexpr double CoveRouteClearWidthCm = 140.0;
constexpr double CoveRouteTreadWidthCm = 150.0;
constexpr double CoveRouteCornerHalfCm = 75.0;   // cove_route.py CORNER_HALF_M
constexpr double CoveRouteGroundSampleCm = 75.0;
// The route's own limit: steeper than this is on steps.
constexpr double CoveRouteMaxPathGrade = 1.0 / 7.0;
}
