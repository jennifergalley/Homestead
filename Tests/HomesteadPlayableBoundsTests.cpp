// Native tests for the map shrink's edges (shrink-estate-map decisions 5 and 7): reserved land outlines and
// names, parcels ignoring them, the PlayableBounds rectangle, the road-end gate and its refusal's rate limit.
#include "HomesteadEstate.h"
#include "HomesteadParcels.h"
#include "HomesteadPlayableBounds.h"
#include "HomesteadReservedLand.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

using namespace Homestead;

namespace
{
int Failures = 0;
void Check(bool ok, const char* what)
{
    if (!ok)
    {
        std::printf("FAIL: %s\n", what);
        ++Failures;
    }
}
bool Near(double a, double b, double tolerance = 1e-6) { return std::abs(a - b) <= tolerance; }

EstateLayout ReservedLayout()
{
    EstateLayout layout;
    const std::vector<Point> square{{0, 0}, {1000, 0}, {1000, 1000}, {0, 1000}};
    layout.polygons = {
        {Anchor::EstateBoundary, square},
        {std::string(Anchor::ForSaleParcelPrefix) + "CarnWood", square},
        {std::string(Anchor::NeighbourPrefix) + "Penhallow", square},
        {std::string(Anchor::NeighbourPrefix) + "Tregarthen", square},
        {std::string(Anchor::NeighbourPrefix) + "Polwhele", square},
        {std::string(Anchor::CommonPrefix) + "VillageGreen", square},
        {std::string(Anchor::CommonPrefix) + "Allotments", square},
        {std::string(Anchor::CommonPrefix) + "ChapelSands", square},
        {std::string(Anchor::CommonPrefix) + "VillageGrowth", square},
        {std::string(Anchor::CommonPrefix) + "Unnamed", square},
        {std::string(Anchor::NeighbourPrefix) + "Sliver", {{0, 0}, {1, 1}}},
        {Anchor::PlayableBounds, {{-90000.0, -136000.0}, {56000.0, -136000.0}, {56000.0, 16000.0}, {-90000.0, 16000.0}}},
    };
    return layout;
}

void ReservedLandIsNotAParcel()
{
    for (const EstateLayout* layout : {&ProvisionalEstateLayout()})
        for (const Parcel& parcel : ParcelsFromLayout(*layout))
        {
            Check(parcel.id.rfind(Anchor::NeighbourPrefix, 0) != 0, "a Neighbour polygon became a parcel");
            Check(parcel.id.rfind(Anchor::CommonPrefix, 0) != 0, "a Common polygon became a parcel");
            Check(parcel.id != Anchor::PlayableBounds, "PlayableBounds became a parcel");
        }
    const auto parcels = ParcelsFromLayout(ReservedLayout());
    Check(parcels.size() == 2, "only the home estate and the for-sale wood are parcels");
    Check(parcels.size() == 2 && parcels[0].owned && parcels[1].forSale && parcels[1].id == "ForSale.CarnWood",
        "home owned, Hollin Wood for sale");
}

void ReservedLandNames()
{
    struct Expected { const char* id; const char* label; bool faintest; };
    const Expected expected[] = {
        {"Neighbour.Penhallow", "Ashgrove", false},
        {"Neighbour.Tregarthen", "Brackenburn", false},
        {"Neighbour.Polwhele", "Thornley", false},
        {"Common.VillageGreen", "the village green", false},
        {"Common.Allotments", "allotments", false},
        {"Common.ChapelSands", "Gull Sands", false},
        {"Common.VillageGrowth", "", true},
        {"Common.Unnamed", "", false},
    };
    const auto outlines = ReservedOutlinesFromLayout(ReservedLayout());
    Check(outlines.size() == sizeof(expected) / sizeof(expected[0]), "every reserved polygon with an area is outlined");
    for (std::size_t i = 0; i < outlines.size() && i < sizeof(expected) / sizeof(expected[0]); ++i)
    {
        Check(outlines[i].id == expected[i].id, expected[i].id);
        Check(outlines[i].label == expected[i].label, expected[i].label);
        Check(outlines[i].faintest == expected[i].faintest, "only the village growth is faintest");
        Check(outlines[i].polygon.size() == 4, "the outline keeps its ring");
        for (const char* banned : {"eserved", "uture", ".", "Neighbour", "Common"})
            Check(outlines[i].label.find(banned) == std::string::npos, "labels never say reserved/future or show ids");
    }
    Check(ReservedLandLabel(Anchor::EstateBoundary) == nullptr, "the estate isn't reserved land");
    Check(ReservedLandLabel("ForSale.TopField") == nullptr, "for-sale land isn't reserved land");
    Check(ReservedLandLabel(Anchor::PlayableBounds) == nullptr, "the playable bounds aren't reserved land");
    for (const ReservedOutline& outline : ReservedOutlinesFromLayout(ProvisionalEstateLayout()))
        Check(outline.id.rfind(Anchor::NeighbourPrefix, 0) == 0 || outline.id.rfind(Anchor::CommonPrefix, 0) == 0,
            "the provisional layout's outlines are all reserved land");
}

void PlayableBoundsRectangle()
{
    const PlayableRect rect = EstatePlayableRect(ReservedLayout());
    Check(rect.valid, "the rectangle is read from the layout");
    Check(Near(rect.minX, -90000.0) && Near(rect.maxX, 56000.0) && Near(rect.minY, -136000.0) && Near(rect.maxY, 16000.0),
        "its extent matches the polygon");
    Check(rect.Contains({0.0, 0.0}) && !rect.Contains({60000.0, 0.0}) && !rect.Contains({0.0, 17000.0}), "contains");
    Check(!EstatePlayableRect(EstateLayout{}).valid, "no polygon, no rectangle");
    EstateLayout flat;
    flat.polygons = {{Anchor::PlayableBounds, {{0, 0}, {100, 0}, {200, 0}}}};
    Check(!EstatePlayableRect(flat).valid, "a rectangle needs an area");
    const PlayableRect provisional = EstatePlayableRect(ProvisionalEstateLayout());
    if (provisional.valid)
    {
        const EstateLayout& layout = ProvisionalEstateLayout();
        for (const char* anchor : {Anchor::StandingRoomSpawn, Anchor::TownSquare, Anchor::CoveBeach, Anchor::MineEntrance})
            if (layout.FindLandmark(anchor)) Check(provisional.Contains(layout.PointOr(anchor, {})), anchor);
    }
}

void RoadEndGateStandsAtTheRoadsEnd()
{
    const std::vector<Point> road{{-6000.0, 10000.0}, {-6888.0, 11261.0}, {-7119.0, 11588.0}, {-7119.0, 11588.0}};
    const RoadEndGate gate = EstateRoadEndGate(EstateLayout{}, road);
    Check(gate.valid && !gate.fromLandmark, "the gate falls back to the road's end");
    const double dx = -7119.0 - -6888.0, dy = 11588.0 - 11261.0, length = std::hypot(dx, dy);
    Check(Near(gate.yaw, std::atan2(dy, dx) * 180.0 / 3.14159265358979323846, 1e-6), "it faces along the last segment");
    Check(Near(gate.position.x, -7119.0 - dx / length * RoadEndGateInsetCm, 1e-6)
        && Near(gate.position.y, 11588.0 - dy / length * RoadEndGateInsetCm, 1e-6), "inset back along the road");
    Check(!EstateRoadEndGate(EstateLayout{}, {{0, 0}}).valid, "one point gives no gate");
    Check(!EstateRoadEndGate(EstateLayout{}, {{0, 0}, {0, 0}}).valid, "no heading gives no gate");
    EstateLayout placed;
    placed.landmarks = {{RoadEndGateAnchor, {-7000.0, 11400.0}, 6000.0, 125.0}};
    const RoadEndGate authored = EstateRoadEndGate(placed, road);
    Check(authored.valid && authored.fromLandmark && Near(authored.position.x, -7000.0) && Near(authored.yaw, 125.0),
        "the landmark wins");
}

void RoadEndRefusalIsRateLimited()
{
    Check(RoadEndRefusalDue(5.0, -1.0), "the first bump shows the refusal");
    Check(!RoadEndRefusalDue(5.0 + RoadEndRefusalCooldownSeconds * 0.5, 5.0), "not again straight away");
    Check(RoadEndRefusalDue(5.0 + RoadEndRefusalCooldownSeconds, 5.0), "again after the cooldown");
    Check(RoadEndRefusalDue(1.0, 5.0), "a clock that went back (a new world) resets it");
    const std::string message = RoadEndRefusalMessage;
    Check(message == "The road to Marlbury \xE2\x80\x93 another day.", "the agreed words, with an en dash");
    Check(message.find("Truro") == std::string::npos, "no real town");
}
}

int main()
{
    ReservedLandIsNotAParcel();
    ReservedLandNames();
    PlayableBoundsRectangle();
    RoadEndGateStandsAtTheRoadsEnd();
    RoadEndRefusalIsRateLimited();
    std::printf(Failures ? "%d failure(s)\n" : "playable bounds: all checks passed\n", Failures);
    return Failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
