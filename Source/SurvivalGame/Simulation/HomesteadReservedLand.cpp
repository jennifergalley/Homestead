#include "HomesteadReservedLand.h"
#include "HomesteadEstate.h"

namespace Homestead
{
namespace
{
struct ReservedName
{
    const char* id;
    const char* label;
    bool faintest;
};

// Jenny-approved period names (shrink-estate-map design, decision 5). VillageGrowth is outline only.
constexpr ReservedName ReservedNames[] = {
    {"Neighbour.Penhallow", "Ashgrove", false},
    {"Neighbour.Tregarthen", "Brackenburn", false},
    {"Neighbour.Polwhele", "Thornley", false},
    {"Common.VillageGreen", "the village green", false},
    {"Common.Allotments", "allotments", false},
    {"Common.ChapelSands", "Gull Sands", false},
    {"Common.VillageGrowth", "", true},
};

bool IsReservedName(const std::string& name)
{
    return name.rfind(Anchor::NeighbourPrefix, 0) == 0 || name.rfind(Anchor::CommonPrefix, 0) == 0;
}

const ReservedName* FindReservedName(const std::string& name)
{
    for (const ReservedName& entry : ReservedNames)
        if (name == entry.id) return &entry;
    return nullptr;
}
}

const char* ReservedLandLabel(const std::string& name)
{
    if (!IsReservedName(name)) return nullptr;
    const ReservedName* entry = FindReservedName(name);
    return entry ? entry->label : "";
}

bool IsFaintestReservedLand(const std::string& name)
{
    const ReservedName* entry = FindReservedName(name);
    return entry && entry->faintest;
}

std::vector<ReservedOutline> ReservedOutlinesFromLayout(const EstateLayout& layout)
{
    std::vector<ReservedOutline> outlines;
    for (const LandmarkPolygon& polygon : layout.polygons)
    {
        const char* label = ReservedLandLabel(polygon.name);
        if (!label || polygon.points.size() < 3) continue;
        outlines.push_back({polygon.name, label, IsFaintestReservedLand(polygon.name), polygon.points});
    }
    return outlines;
}
}
