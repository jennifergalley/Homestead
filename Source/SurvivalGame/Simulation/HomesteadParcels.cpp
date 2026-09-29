#include "HomesteadParcels.h"
#include "HomesteadCrafting.h"
#include "HomesteadEstate.h"

#include <cctype>
#include <istream>
#include <ostream>
#include <set>
#include <string>

namespace Homestead
{
namespace
{
constexpr std::size_t MaxParcelIdLength = 128;
constexpr const char* ParcelSectionTag = "parcels";

bool StartsWith(const std::string& text, const char* prefix)
{
    return text.rfind(prefix, 0) == 0;
}

// Save ids are whitespace-free tokens; the layout's polygon names normally already are.
std::string ParcelId(const std::string& name)
{
    std::string id = name.substr(0, MaxParcelIdLength);
    for (char& c : id)
        if (static_cast<unsigned char>(c) <= ' ' || static_cast<unsigned char>(c) > '~') c = '_';
    return id;
}
}

std::vector<Parcel> ParcelsFromLayout(const EstateLayout& layout)
{
    std::vector<Parcel> parcels;
    for (const auto& polygon : layout.polygons)
    {
        const bool home = polygon.name == Anchor::EstateBoundary;
        const bool forSale = StartsWith(polygon.name, Anchor::ForSaleParcelPrefix);
        if ((!home && !forSale) || polygon.points.size() < 3 || parcels.size() >= MaxParcels) continue;
        Parcel parcel;
        parcel.id = ParcelId(polygon.name);
        parcel.polygon = polygon.points;
        parcel.owned = home;
        parcel.forSale = forSale;
        bool duplicate = false;
        for (const auto& other : parcels) duplicate = duplicate || other.id == parcel.id;
        if (!duplicate) parcels.push_back(std::move(parcel));
    }
    return parcels;
}

void SeedEstateParcels(State& state, const EstateLayout& layout)
{
    std::vector<Parcel> parcels = ParcelsFromLayout(layout);
    for (auto& parcel : parcels)
        for (const auto& previous : state.parcels)
            if (previous.id == parcel.id) parcel.owned = previous.owned;
    state.parcels = std::move(parcels);
}

const Parcel* FindParcelAt(const std::vector<Parcel>& parcels, Point point)
{
    for (const auto& parcel : parcels)
        if (PointInPolygon(parcel.polygon, point)) return &parcel;
    return nullptr;
}

bool InOwnedParcel(const std::vector<Parcel>& parcels, Point point)
{
    for (const auto& parcel : parcels)
        if (parcel.owned && PointInPolygon(parcel.polygon, point)) return true;
    return false;
}

void FootprintCorners(const Footprint& area, Point (&corners)[4])
{
    const Point x = RotateYaw({area.half.x, 0.0}, area.yaw);
    const Point y = RotateYaw({0.0, area.half.y}, area.yaw);
    corners[0] = {area.center.x + x.x + y.x, area.center.y + x.y + y.y};
    corners[1] = {area.center.x - x.x + y.x, area.center.y - x.y + y.y};
    corners[2] = {area.center.x - x.x - y.x, area.center.y - x.y - y.y};
    corners[3] = {area.center.x + x.x - y.x, area.center.y + x.y - y.y};
}

bool FootprintOwned(const std::vector<Parcel>& parcels, const Footprint& area)
{
    Point corners[4];
    FootprintCorners(area, corners);
    for (const Point& corner : corners)
        if (!InOwnedParcel(parcels, corner)) return false;
    return true;
}

void WriteParcelOwnership(std::ostream& out, const State& state)
{
    if (state.parcels.empty()) return;
    out << ParcelSectionTag << ' ' << state.parcels.size() << '\n';
    for (const auto& parcel : state.parcels) out << parcel.id << ' ' << (parcel.owned ? 1 : 0) << '\n';
}

bool ReadParcelOwnership(std::istream& in, State& state)
{
    in >> std::ws;
    if (in.eof()) return true;
    const auto start = in.tellg();
    std::string tag;
    if (!(in >> tag)) return false;
    if (tag != ParcelSectionTag)
    {
        in.clear();
        in.seekg(start);
        return true;
    }
    int count = 0;
    if (!(in >> count) || count < 0 || count > MaxParcels) return false;
    std::set<std::string> seen;
    for (int i = 0; i < count; ++i)
    {
        std::string id;
        int owned = -1;
        if (!(in >> id >> owned) || (owned != 0 && owned != 1) || id.size() > MaxParcelIdLength
            || !seen.insert(id).second) return false;
        for (auto& parcel : state.parcels)
            if (parcel.id == id) parcel.owned = owned == 1;
    }
    return true;
}

const Parcel* Simulation::ParcelAt(Point point) const
{
    return FindParcelAt(state_.parcels, point);
}

bool Simulation::IsOwned(Point point) const
{
    return state_.parcels.empty() || InOwnedParcel(state_.parcels, point);
}

Result Simulation::CanBuildAt(const PlacementTarget& target) const
{
    if (state_.parcels.empty()) return {true, ""};
    const Building* building = target.buildingId < 0 ? &target.frame : FindBuilding(state_, target.buildingId);
    if (!building || static_cast<int>(target.kind) < 0 || target.kind >= Piece::Count)
        return {false, "Choose a valid structure and building cell.", ResultCode::Invalid};
    const bool onFoundation = target.buildingId >= 0 && HasFoundation(state_, target.buildingId, target.cellX, target.cellY);
    // Walls, doorways and roofs stand on their foundation's cell, so they count that ground.
    const bool furniture = IsFurniture(target.kind) || Crafting::IsFence(target.kind);
    Footprint area = furniture
        ? PieceFootprint(*building, target.kind, target.cellX, target.cellY, ((target.rotation % 4) + 4) % 4, onFoundation,
            target.spot)
        : PieceFootprint(*building, Piece::Foundation, target.cellX, target.cellY, 0, true);
    // Pieces flush against the boundary still count as inside.
    area.half = {area.half.x > 1.0 ? area.half.x - 1.0 : 0.0, area.half.y > 1.0 ? area.half.y - 1.0 : 0.0};
    if (!FootprintOwned(state_.parcels, area)) return {false, OutsideEstateMessage, ResultCode::Invalid};
    return {true, ""};
}
}
