#pragma once

#include "HomesteadSimulation.h"

#include <string>
#include <vector>

// Reserved land on the map (shrink-estate-map decision 5): the layout's Neighbour.* estates and Common.*
// communal land. They are drawn as faint dashed outlines with their period names and have no gameplay:
// ParcelsFromLayout never turns them into parcels. Player-facing names never say "reserved" or "future".
namespace Homestead
{
struct EstateLayout;

struct ReservedOutline
{
    std::string id;           // the layout polygon's name, e.g. "Neighbour.Penhallow"
    std::string label;        // what the map writes at its centroid; empty for an unlabelled outline
    bool faintest = false;    // drawn fainter still (Common.VillageGrowth)
    std::vector<Point> polygon;
};

// The map's name for a reserved polygon: its label (possibly empty) when `name` is a Neighbour.* or
// Common.* polygon, else null. Unknown reserved names get an empty label rather than their raw id.
const char* ReservedLandLabel(const std::string& name);
// True for Common.VillageGrowth-style outlines drawn fainter than the rest.
bool IsFaintestReservedLand(const std::string& name);
// Every Neighbour.* and Common.* polygon with at least three points, in layout order.
std::vector<ReservedOutline> ReservedOutlinesFromLayout(const EstateLayout& layout);
}
