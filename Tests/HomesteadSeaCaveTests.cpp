// Native tests for the sea cave's placement table (Simulation/HomesteadSeaCave, generated into
// HomesteadSeaCave.inc by Scripts/Blender/Recipes/sea_cave.py): the cave frame round-trips, the pool lies
// inside the cave and the dressing stays out of the water, and the controller treats her as in the cave only
// when the landscape is well over her head.
#include "../Source/SurvivalGame/Simulation/HomesteadEstate.h"
#include "../Source/SurvivalGame/Simulation/HomesteadSeaCave.h"

#include <cmath>
#include <cstdio>
#include <cstring>

using namespace Homestead;

namespace
{
int Failures = 0;
void Check(bool ok, const char* what, double value = 0.0)
{
    if (!ok)
    {
        std::printf("FAIL: %s (%g)\n", what, value);
        ++Failures;
    }
}

Point Centroid(const std::vector<Point>& ring)
{
    Point c;
    for (const Point& p : ring)
    {
        c.x += p.x;
        c.y += p.y;
    }
    c.x /= static_cast<double>(ring.size());
    c.y /= static_cast<double>(ring.size());
    return c;
}

bool Outdoors(const SeaCaveProp& prop)
{
    return std::strstr(prop.mesh, "Campion") != nullptr;
}
}

int main()
{
    const SeaCave& cave = EstateSeaCave();
    Check(cave.valid(), "the cave has a footprint");
    Check(cave.shore.size() >= 12, "the pool has a shoreline", static_cast<double>(cave.shore.size()));

    // The frame round-trips, and +X runs inland (away from the mouth at the anchor).
    const Point local{1234.0, -567.0};
    const Point back = cave.ToLocal(cave.ToWorld(local));
    Check(std::fabs(back.x - local.x) < 1e-6 && std::fabs(back.y - local.y) < 1e-6, "the cave frame round-trips");
    const Point inland = cave.ToWorld({1000.0, 0.0});
    Check(std::hypot(inland.x - cave.anchor.x, inland.y - cave.anchor.y) > 999.0, "one metre inland is a metre away");

    // The pool: inside the cave, under its roof, shallow (its surface above the lowest floor, below head height).
    const Point pool = Centroid(cave.shore);
    Check(PointInPolygon(cave.footprint, pool), "the pool lies in the cave");
    for (const Point& p : cave.shore)
        Check(PointInPolygon(cave.footprint, p), "the pool's shore lies in the cave");
    Check(cave.waterZ > cave.floorMinZ && cave.waterZ < cave.floorZ + 100.0, "the pool is shallow", cave.waterZ - cave.floorMinZ);
    Check(cave.lightZ > cave.waterZ + 600.0 && cave.lightZ <= cave.roofMaxZ, "the fissure is high over the pool", cave.lightZ);
    Check(std::hypot(cave.lightX - pool.x, cave.lightY - pool.y) < 300.0, "the fissure is over the pool");

    // Dressing: the indoor pieces stand in the cave, none in the pool.
    int indoor = 0;
    for (const SeaCaveProp& prop : cave.props)
    {
        Check(std::strlen(prop.mesh) > 0, "a dressing piece names its mesh");
        Check(!PointInPolygon(cave.shore, prop.position), "no dressing stands in the pool");
        if (Outdoors(prop)) continue;
        ++indoor;
        if (!PointInPolygon(cave.footprint, prop.position))
            std::printf("  outside the cave: %s at local (%.0f, %.0f)\n", prop.mesh, cave.ToLocal(prop.position).x,
                cave.ToLocal(prop.position).y);
        Check(PointInPolygon(cave.footprint, prop.position), "indoor dressing stands in the cave");
    }
    Check(indoor >= 4, "the cave is dressed", indoor);

    // In the cave only with the landscape well over her head (deep inside), never at the mouth on the beach.
    const double feet = cave.floorZ + 20.0, centre = feet + 90.0;
    Check(InSeaCave(cave, pool, centre, cave.floorZ + 2000.0), "deep inside: in the cave");
    Check(SeaCaveAwareGround(cave, pool, centre, cave.floorZ + 2000.0) == cave.floorMinZ, "deep inside: the floor is the ground");
    Check(!InSeaCave(cave, pool, centre, centre + 50.0), "under a thin roof: on the landscape");
    Check(!InSeaCave(cave, pool, cave.floorMinZ - SeaCaveBelowFloorCm - 10.0, cave.floorZ + 2000.0), "fallen through: not in the cave");
    Check(!InSeaCave(cave, cave.ToWorld({-4000.0, 0.0}), centre, cave.floorZ + 2000.0), "out on the beach: not in the cave");
    Check(SeaCaveAwareGround(cave, cave.ToWorld({-4000.0, 0.0}), centre, 55.0) == 55.0, "outside: the landscape is the ground");
    // The cave takes over before the controller's fall-through check (her centre 200 cm under the surface) would.
    Check(SeaCaveBelowSurfaceCm < 200.0, "the cave takes over before the fall-through check", SeaCaveBelowSurfaceCm);

    if (Failures) std::printf("%d failure(s)\n", Failures);
    else std::printf("HomesteadSeaCaveTests: all passed\n");
    return Failures ? 1 : 0;
}
