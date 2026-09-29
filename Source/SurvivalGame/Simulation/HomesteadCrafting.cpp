#include "HomesteadCrafting.h"

#include <algorithm>
#include <cmath>
#include <istream>
#include <map>
#include <ostream>
#include <set>
#include <utility>

namespace Homestead
{
namespace Crafting
{
namespace
{
double CraftingDistanceSquared(Point a, Point b)
{
    const double dx = a.x - b.x, dy = a.y - b.y;
    return dx * dx + dy * dy;
}
bool CraftingFinite(Point p) { return std::isfinite(p.x) && std::isfinite(p.y); }
double CraftingYaw(double yaw)
{
    yaw = std::fmod(yaw, 360.0);
    if (yaw < 0.0) yaw += 360.0;
    return yaw >= 360.0 ? 0.0 : yaw;
}
// The frame whose cell (0, 0) is centred on `centre`, turned `yaw` degrees.
Building CraftingFrameAt(Point centre, double yaw)
{
    const Point corner = RotateYaw(CellCenter(0, 0), yaw);
    return {0, {centre.x - corner.x, centre.y - corner.y}, yaw};
}
}

bool IsStation(Piece piece) { return piece == Piece::Workbench || piece == Piece::Sawhorse; }
bool IsFence(Piece piece) { return piece == Piece::FenceRail || piece == Piece::FenceGate; }
bool IsMovable(Piece piece)
{
    return piece == Piece::Stool || piece == Piece::Table || piece == Piece::Chair || piece == Piece::Shelf;
}

Piece StationFor(Recipe recipe)
{
    switch (recipe)
    {
    case Recipe::SawPlanks: return Piece::Sawhorse;
    case Recipe::MakeFenceSection:
    case Recipe::MakeFieldGate:
    case Recipe::MakeStool:
    case Recipe::MakeTable:
    case Recipe::MakeChair:
    case Recipe::MakeShelf: return Piece::Workbench;
    default: return Piece::Count;
    }
}

const char* StationLabel(Piece station)
{
    return station == Piece::Sawhorse ? "Sawhorse within reach" : "Workbench within reach";
}

CraftCategory CategoryOf(Recipe recipe)
{
    switch (recipe)
    {
    case Recipe::HaftAxe:
    case Recipe::HaftHoe:
    case Recipe::HaftScythe:
    case Recipe::HaftBillhook:
    case Recipe::HaftPickaxe: return CraftCategory::Tools;
    case Recipe::SplitFirewood:
    case Recipe::SawPlanks:
    case Recipe::MakeFenceSection:
    case Recipe::MakeFieldGate: return CraftCategory::Farm;
    case Recipe::MakeStool:
    case Recipe::MakeTable:
    case Recipe::MakeChair:
    case Recipe::MakeShelf: return CraftCategory::Furniture;
    default: return CraftCategory::Cooking;
    }
}

const char* CraftCategoryName(CraftCategory category)
{
    static const char* names[] = {"Tools", "Stations", "Farm", "Furniture", "Cooking"};
    static_assert(sizeof(names) / sizeof(names[0]) == CraftCategoryCount, "Every craft category needs a name.");
    const int index = static_cast<int>(category);
    return index >= 0 && index < CraftCategoryCount ? names[index] : "Craft";
}

std::string StationMissingMessage(Piece station)
{
    const bool saw = station == Piece::Sawhorse;
    std::string cost = PieceRequirements(station);
    cost = cost.substr(0, cost.find(';'));
    return std::string("Needs a ") + (saw ? "sawhorse" : "workbench") + " within reach. Build one from "
        + cost + " on the Build page.";
}

Point FurnitureHalf(Piece piece)
{
    switch (piece)
    {
    case Piece::Workbench: return {92.0, 33.0};
    case Piece::Sawhorse: return {50.0, 34.0};
    case Piece::Stool: return {18.0, 18.0};
    case Piece::Table: return {61.0, 39.0};
    case Piece::Chair: return {23.0, 24.0};
    case Piece::Shelf: return {52.0, 18.0};
    default: return {};
    }
}

Point FenceEnd(const Building& frame, int cellX, int cellY, int rotation, int side)
{
    const Point centre = BuildingCellCenter(frame, cellX, cellY);
    const Point half = RotateYaw({side == 0 ? -FenceSpan * 0.5 : FenceSpan * 0.5, 0.0}, PieceYaw(frame, rotation));
    return {centre.x + half.x, centre.y + half.y};
}

Point FenceEnd(const State& state, const Structure& piece, int side)
{
    const Building* frame = FindBuilding(state, piece.buildingId);
    return FenceEnd(frame ? *frame : Building{}, piece.cellX, piece.cellY, piece.rotation, side);
}

std::unordered_map<int, unsigned> PostOwners(const State& state)
{
    std::unordered_map<int, unsigned> owners;
    // Posts on a coarse grid; each post also checks the neighbouring buckets it might straddle.
    std::map<std::pair<long long, long long>, std::vector<Point>> taken;
    std::vector<const Structure*> fences;
    for (const auto& piece : state.structures)
        if (IsFence(piece.kind)) fences.push_back(&piece);
    std::sort(fences.begin(), fences.end(), [](const Structure* a, const Structure* b) { return a->id < b->id; });
    for (const Structure* piece : fences)
    {
        unsigned mask = 0;
        for (int side = 0; side < 2; ++side)
        {
            const Point post = FenceEnd(state, *piece, side);
            const long long bx = static_cast<long long>(std::floor(post.x / 10.0));
            const long long by = static_cast<long long>(std::floor(post.y / 10.0));
            bool shared = false;
            for (long long dx = -1; dx <= 1 && !shared; ++dx)
                for (long long dy = -1; dy <= 1 && !shared; ++dy)
                {
                    const auto found = taken.find({bx + dx, by + dy});
                    if (found == taken.end()) continue;
                    for (const Point& other : found->second)
                        if (CraftingDistanceSquared(other, post) <= PostMergeDistance * PostMergeDistance) { shared = true; break; }
                }
            if (shared) continue;
            taken[{bx, by}].push_back(post);
            mask |= 1u << side;
        }
        owners[piece->id] = mask;
    }
    return owners;
}

PlacementTarget ResolveFence(const State& state, Piece kind, Point aim, double freeYaw,
    const std::function<bool(const PlacementTarget&)>& buildable)
{
    PlacementTarget target;
    target.kind = kind;
    target.rotation = 0;
    target.buildingId = -1;
    const double yaw = CraftingYaw(std::round(freeYaw / FenceYawStep) * FenceYawStep);
    const Point along = RotateYaw({FenceSpan * 0.5, 0.0}, yaw);
    std::vector<std::pair<double, Point>> candidates;
    for (const auto& piece : state.structures)
    {
        if (!IsFence(piece.kind)) continue;
        for (int side = 0; side < 2; ++side)
        {
            const Point post = FenceEnd(state, piece, side);
            for (int sign : {1, -1})
            {
                const Point centre{post.x + along.x * sign, post.y + along.y * sign};
                const double distance = std::sqrt(CraftingDistanceSquared(centre, aim));
                if (distance < FenceSnapReach) candidates.emplace_back(distance, centre);
            }
        }
    }
    std::stable_sort(candidates.begin(), candidates.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; });
    for (std::size_t i = 0; i < candidates.size(); ++i)
    {
        PlacementTarget snap = target;
        snap.frame = CraftingFrameAt(candidates[i].second, yaw);
        snap.snapped = true;
        if (!buildable || buildable(snap)) return snap;
    }
    if (!candidates.empty())
    {
        target.frame = CraftingFrameAt(candidates.front().second, yaw);
        target.snapped = true;
        return target;
    }
    target.frame = CraftingFrameAt(aim, yaw);
    return target;
}

Point MovableSpot(const Building& building, Piece kind, int cellX, int cellY, int rotation, Point aim)
{
    constexpr double WallInnerFace = CellSize * 0.5 - 14.0;
    constexpr double SpotStep = 5.0;
    const Point local = BuildingLocal(building, aim);
    const Point centre = CellCenter(cellX, cellY);
    const Point half = FurnitureHalf(kind);
    const bool across = (rotation & 1) != 0;
    const double limitX = std::max(0.0, WallInnerFace - (across ? half.y : half.x));
    const double limitY = std::max(0.0, WallInnerFace - (across ? half.x : half.y));
    const Point offset{std::clamp(std::round((local.x - centre.x) / SpotStep) * SpotStep, -limitX, limitX),
        std::clamp(std::round((local.y - centre.y) / SpotStep) * SpotStep, -limitY, limitY)};
    // Building space to piece space: the piece is turned `rotation` quarter turns clockwise.
    const Point turned = RotateYaw(offset, 90.0 * (rotation % 4));
    return {std::round(turned.x), std::round(turned.y)};
}

void WriteSaveSections(std::ostream& output, const State& state)
{
    std::vector<int> open;
    std::vector<const Structure*> spots;
    for (const auto& piece : state.structures)
    {
        if (piece.kind == Piece::FenceGate && piece.open) open.push_back(piece.id);
        if (IsMovable(piece.kind) && (piece.spot.x != 0.0 || piece.spot.y != 0.0)) spots.push_back(&piece);
    }
    if (!open.empty())
    {
        output << GateTag << ' ' << open.size();
        for (int id : open) output << ' ' << id;
        output << '\n';
    }
    if (!spots.empty())
    {
        output << SpotTag << ' ' << spots.size() << '\n';
        for (const Structure* piece : spots) output << piece->id << ' ' << piece->spot.x << ' ' << piece->spot.y << '\n';
    }
}

namespace
{
Structure* FindCraftedPiece(State& state, int id)
{
    for (auto& piece : state.structures)
        if (piece.id == id) return &piece;
    return nullptr;
}
}

bool ReadGates(std::istream& input, State& state)
{
    // The writer never writes an empty section, so any open gate already set means a second copy.
    for (const auto& piece : state.structures)
        if (piece.open) return false;
    long long count = 0;
    if (!(input >> count) || count <= 0 || count > static_cast<long long>(state.structures.size())) return false;
    std::set<int> seen;
    for (long long i = 0; i < count; ++i)
    {
        int id = 0;
        if (!(input >> id) || !seen.insert(id).second) return false;
        Structure* gate = FindCraftedPiece(state, id);
        if (!gate || gate->kind != Piece::FenceGate) return false;
        gate->open = true;
    }
    return true;
}

bool ReadSpots(std::istream& input, State& state)
{
    for (const auto& piece : state.structures)
        if (piece.spot.x != 0.0 || piece.spot.y != 0.0) return false;
    long long count = 0;
    if (!(input >> count) || count <= 0 || count > static_cast<long long>(state.structures.size())) return false;
    std::set<int> seen;
    for (long long i = 0; i < count; ++i)
    {
        int id = 0;
        Point spot;
        if (!(input >> id >> spot.x >> spot.y) || !seen.insert(id).second || !CraftingFinite(spot)
            || std::abs(spot.x) > CellSize * 0.5 || std::abs(spot.y) > CellSize * 0.5
            || (spot.x == 0.0 && spot.y == 0.0)) return false;
        Structure* piece = FindCraftedPiece(state, id);
        if (!piece || !IsMovable(piece->kind)) return false;
        piece->spot = spot;
    }
    return true;
}
}

Result Simulation::ToggleGate(int structureId, Point player)
{
    if (state_.failed) return {false, "You need to recover first.", ResultCode::Unavailable, revision_};
    Structure* gate = nullptr;
    for (auto& piece : state_.structures)
        if (piece.id == structureId) { gate = &piece; break; }
    if (!gate || gate->kind != Piece::FenceGate)
        return {false, "That gate is no longer there.", ResultCode::Invalid, revision_};
    const Point centre = Homestead::StructureCenter(state_, *gate);
    if (!Crafting::CraftingFinite(player)
        || Crafting::CraftingDistanceSquared(centre, player) > Crafting::GateReach * Crafting::GateReach)
        return {false, "Move closer to the gate.", ResultCode::Invalid, revision_};
    gate->open = !gate->open;
    return {true, gate->open ? "Opened the gate." : "Shut the gate.", ResultCode::None, ++revision_};
}
}
