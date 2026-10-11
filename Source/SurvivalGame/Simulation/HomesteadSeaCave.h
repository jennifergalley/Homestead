#pragma once

#include "HomesteadSimulation.h"

#include <vector>

// The sea cave in the cove's west cliff (openspec add-sea-cave): one granite mesh under the estate's landscape,
// entered through a hole painted in it at the mouth. Where it sits, its outline, its pool and the fissure's
// light come from HomesteadSeaCave.inc, which Scripts/Blender/Recipes/sea_cave.py writes when it fits the cave
// to the ground. Scenery only: nothing here is saved.
//
// World frame as HomesteadEstate.h: Unreal centimetres, +X north, +Y east; yaw in degrees from +X toward +Y.
// The cave's own frame: +X inland along the cave, +Y to the right looking in, Z up from the mouth's floor.
namespace Homestead
{
struct SeaCaveProp
{
    const char* mesh = "";        // the static mesh's object path
    Point position;               // world cm
    double z = 0.0;               // world cm, the mesh's ground line
    double yaw = 0.0;             // world
    double scale = 1.0;
};

struct SeaCave
{
    Point anchor;                 // the mouth's floor on the cliff foot (cm)
    double floorZ = 0.0;          // its height (cm)
    double yaw = 0.0;             // the cave's +X in the world
    double waterZ = 0.0;          // the pool's surface (world cm)
    double floorMinZ = 0.0;       // the lowest floor (the pool's bed), world cm
    double roofMaxZ = 0.0;        // the highest roof (world cm)
    double lightX = 0.0, lightY = 0.0, lightZ = 0.0;   // the fissure's top, world cm
    std::vector<Point> footprint; // the cave's outline (world cm)
    std::vector<Point> shore;     // the pool's waterline, a closed loop (world cm)
    std::vector<SeaCaveProp> props;   // reused rocks, ferns and flowers dressing it

    bool valid() const { return footprint.size() >= 3; }
    Point ToWorld(Point local) const;
    Point ToLocal(Point world) const;
};

const SeaCave& EstateSeaCave();

// She counts as in the cave this far (cm, her capsule's centre) below the landscape's surface over it: the
// landscape above the cave is its roof, so the cave's floor, not the landscape, is the ground under her.
// Below the controller's fall-through check (200 cm under the surface for her centre, about 290 cm for her
// feet), so walking in never looks like falling through the ground.
constexpr double SeaCaveBelowSurfaceCm = 150.0;
// ... and no more than this below the cave's lowest floor.
constexpr double SeaCaveBelowFloorCm = 250.0;

// Whether a body at `point`, height `z` (cm), with the landscape surface at `surfaceZ` there, is inside the cave.
bool InSeaCave(const SeaCave& cave, Point point, double z, double surfaceZ);
// The ground to hold her against: the landscape outside, the cave's lowest floor inside it.
double SeaCaveAwareGround(const SeaCave& cave, Point point, double z, double surfaceZ);
}
