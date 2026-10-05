#include "HomesteadTravel.h"
#include "HomesteadEstatePublicRoad.h"
#include "HomesteadShops.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <istream>
#include <ostream>

namespace Homestead
{
namespace
{
struct TravelPlace
{
    const char* anchor;
    const char* label;
    const char* sentenceName;
    double visitReachCm;
};
constexpr std::array<TravelPlace, TravelDestinationCount> TravelPlaces{{
    {Anchor::StandingRoomSpawn, "The manor", "the manor", TravelArrivedCm},
    {Anchor::TownSquare, "Town", "town", TravelTownReachCm},
    {Anchor::MineEntrance, "Mine ruin", "the mine ruin", 1500.0},
    {Anchor::CoveBeach, "The cove", "the cove", 2000.0},
    {Anchor::MillSite, "Mill site", "the mill site", 1500.0},
    {Anchor::EstateGateway, "Estate gateway", "the estate gateway", 1500.0},
    {Anchor::GeneralStoreDoor, "General store", "the general store", 1000.0},
}};
bool TravelValidDestination(TravelDestination destination)
{
    return static_cast<int>(destination) >= 0 && static_cast<int>(destination) < TravelDestinationCount;
}
Result TravelGood(const std::string& text, std::uint64_t revision) { return {true, text, ResultCode::None, revision}; }
Result TravelBad(const std::string& text, std::uint64_t revision, ResultCode code = ResultCode::Invalid)
{
    return {false, text, code, revision};
}
const char* TravelStopName(TravelDestination destination)
{
    return destination == TravelDestination::Manor ? "Manor" : "Town";
}
std::string TravelKilometres(double metres)
{
    if (metres < 950.0) return std::to_string(static_cast<int>(std::lround(metres / 10.0) * 10)) + " m";
    const long tenths = std::lround(metres / 100.0);
    return std::to_string(tenths / 10) + "." + std::to_string(tenths % 10) + " km";
}
}

const char* TravelDestinationName(TravelDestination destination)
{
    return TravelValidDestination(destination) ? TravelPlaces[static_cast<int>(destination)].sentenceName : "that place";
}

const char* TravelDestinationLabel(TravelDestination destination)
{
    return TravelValidDestination(destination) ? TravelPlaces[static_cast<int>(destination)].label : "Unknown location";
}

bool IsTravelUnlocked(const State& state, TravelDestination destination)
{
    if (!state.fixedEstate || !TravelValidDestination(destination)) return false;
    return destination == TravelDestination::Manor
        || std::binary_search(state.discoveredTravel.begin(), state.discoveredTravel.end(), static_cast<int>(destination));
}

bool TravelVisitNear(Point at, TravelDestination destination, const EstateLayout& layout)
{
    if (!TravelValidDestination(destination) || !std::isfinite(at.x) || !std::isfinite(at.y)) return false;
    const TravelPlace& place = TravelPlaces[static_cast<int>(destination)];
    const Landmark* landmark = layout.FindLandmark(place.anchor);
    return landmark && std::hypot(at.x - landmark->position.x, at.y - landmark->position.y) <= place.visitReachCm;
}

Result Simulation::DiscoverTravel(TravelDestination destination, Point player)
{
    if (!state_.fixedEstate || state_.failed)
        return TravelBad("You can't discover places now.", revision_, ResultCode::Unavailable);
    if (!TravelValidDestination(destination)) return TravelBad("Choose a marked location.", revision_);
    if (IsTravelUnlocked(state_, destination)) return TravelBad("Already discovered.", revision_);
    if (!TravelVisitNear(player, destination, Layout())) return TravelBad("Visit this location first.", revision_);
    const int id = static_cast<int>(destination);
    state_.discoveredTravel.insert(std::lower_bound(state_.discoveredTravel.begin(), state_.discoveredTravel.end(), id), id);
    return TravelGood(std::string("Fast Travel Destination Unlocked: ") + TravelDestinationLabel(destination), ++revision_);
}

void TravelDiscovery::WriteSaveSection(std::ostream& output, const State& state)
{
    if (state.discoveredTravel.empty()) return;
    output << SaveTag << ' ' << state.discoveredTravel.size();
    for (int destination : state.discoveredTravel) output << ' ' << destination;
    output << '\n';
}

bool TravelDiscovery::ReadSaveSection(std::istream& input, State& state)
{
    int count = 0;
    if (!state.fixedEstate || !(input >> count) || count < 0 || count >= TravelDestinationCount) return false;
    std::vector<int> destinations;
    for (int index = 0; index < count; ++index)
    {
        int destination = 0;
        if (!(input >> destination) || destination <= 0 || destination >= TravelDestinationCount
            || (!destinations.empty() && destination <= destinations.back())) return false;
        destinations.push_back(destination);
    }
    state.discoveredTravel = std::move(destinations);
    return true;
}

std::string FormatWalkDuration(double gameHours)
{
    const long minutes = std::max(1L, std::lround(gameHours * 60.0));
    const long hours = minutes / 60, rest = minutes % 60;
    if (!hours) return std::to_string(rest) + " min";
    return std::to_string(hours) + " h" + (rest ? " " + std::to_string(rest) + " min" : std::string());
}

bool WithinTravelReachOfRoad(Point at)
{
    const PublicRoad& road = EstatePublicRoad();
    return road.points.size() >= 2 && road.NearestTo(at).distanceCm <= TravelMaxConnectorCm;
}

TravelPlan PlanTravel(const State& state, Point from, TravelDestination destination, const EstateLayout& layout)
{
    TravelPlan plan;
    plan.destination = destination;
    if (!TravelValidDestination(destination)) { plan.error = "Choose a marked location."; return plan; }
    const std::string where = TravelDestinationName(destination);
    if (state.failed) { plan.error = "You need to recover first."; return plan; }
    if (!state.fixedEstate) { plan.error = "There's no road to " + where + " from here."; return plan; }
    if (!std::isfinite(from.x) || !std::isfinite(from.y)
        || std::abs(from.x) > MaxWorldCoordinate || std::abs(from.y) > MaxWorldCoordinate)
    { plan.error = "You can't set out from here."; return plan; }
    if (!std::isfinite(state.dayMinutes) || state.dayMinutes <= 0.0) { plan.error = "The day length is invalid."; return plan; }
    if (!IsTravelUnlocked(state, destination)) { plan.error = "Visit this location first."; return plan; }
    const PublicRoad& road = EstatePublicRoad();
    const bool roadDestination = destination == TravelDestination::Manor || destination == TravelDestination::Town;
    const PublicRoadStop* stop = roadDestination ? road.FindStop(TravelStopName(destination)) : nullptr;
    const Landmark* landmark = layout.FindLandmark(TravelPlaces[static_cast<int>(destination)].anchor);
    if (roadDestination ? road.points.size() < 2 || !stop : !landmark)
    { plan.error = "The way to " + where + " isn't mapped yet."; return plan; }
    if (destination == TravelDestination::Town)
    {
        // Already in town there's no road to walk: inside the store or on its step, or about the square.
        for (const Shop& shop : state.shops)
            if (shop.kind == ShopKind::GeneralStore
                && std::hypot(from.x - shop.counterX, from.y - shop.counterY) <= ShopWaitReach)
            {
                plan.error = "You're already at the general store.";
                return plan;
            }
        if (const auto* square = layout.FindLandmark(Anchor::TownSquare);
            square && std::hypot(from.x - square->position.x, from.y - square->position.y) <= TravelTownReachCm)
        {
            plan.error = "You're already in town.";
            return plan;
        }
    }
    if (roadDestination && (std::hypot(from.x - stop->position.x, from.y - stop->position.y) < TravelArrivedCm
        || (stop->hasArrival && std::hypot(from.x - stop->arrival.x, from.y - stop->arrival.y) < TravelArrivedCm)))
    {
        plan.error = destination == TravelDestination::Manor ? "You're already at the manor." : "You're already in town.";
        return plan;
    }
    if (roadDestination)
    {
        const PublicRoad::Nearest nearest = road.NearestTo(from);
        if (nearest.distanceCm > TravelMaxConnectorCm)
        {
            plan.error = "Make your way back to the road first.";
            return plan;
        }
        plan.connectorMetres = nearest.distanceCm / 100.0;
        plan.roadMetres = road.WalkMetres(nearest.chainage, stop->chainage);
        const double offRoadMetres = stop->hasArrival
            ? std::hypot(stop->arrival.x - stop->position.x, stop->arrival.y - stop->position.y) / 100.0 : 0.0;
        plan.totalMetres = plan.connectorMetres + plan.roadMetres + offRoadMetres;
        plan.arrival = stop->position;
        plan.arrivalZ = stop->z;
        const double ahead = destination == TravelDestination::Town ? -5.0 : 5.0;
        const Point behind = road.At(stop->chainage + ahead);
        plan.arrivalYaw = std::atan2(stop->position.y - behind.y, stop->position.x - behind.x) * 180.0 / 3.14159265358979323846;
        if (stop->hasArrival)
        {
            plan.arrival = stop->arrival;
            plan.arrivalZ = stop->arrivalZ;
            plan.arrivalYaw = stop->arrivalYaw;
        }
    }
    else
    {
        constexpr double LandmarkArrivedCm = 400.0;
        const double distanceCm = std::hypot(from.x - landmark->position.x, from.y - landmark->position.y);
        if (distanceCm <= LandmarkArrivedCm) { plan.error = "You're already here."; return plan; }
        plan.totalMetres = distanceCm / 100.0;
        plan.arrival = landmark->position;
        plan.arrivalZ = landmark->z;
        plan.arrivalYaw = landmark->yaw;
    }
    const double realSeconds = plan.totalMetres * 100.0 / RoadWalkPaceCmPerSecond;
    plan.gameHours = realSeconds * 24.0 / (state.dayMinutes * 60.0);
    plan.arrivalHour = state.hour + plan.gameHours;
    plan.nextDay = std::floor(plan.arrivalHour / 24.0) > std::floor(state.hour / 24.0);
    std::string summary = "Walk to " + where + (roadDestination ? " along the road: " : ": ") + TravelKilometres(plan.totalMetres) + ", about "
        + FormatWalkDuration(plan.gameHours) + ".\nYou'd arrive about " + FormatHour(plan.arrivalHour)
        + (plan.nextDay ? " the next day." : ".");
    if (destination == TravelDestination::Town || destination == TravelDestination::Store)
        for (const Shop& shop : state.shops)
            if (shop.kind == ShopKind::GeneralStore && !IsShopOpen(shop, plan.arrivalHour))
            {
                plan.storeClosedOnArrival = true;
                plan.storeOpenHour = shop.openHour;
                const double opens = NextShopOpening(shop, plan.arrivalHour);
                const Calendar::Date arrives = Calendar::DateAt(plan.arrivalHour), reopens = Calendar::DateAt(opens);
                // "Closed all day" only when she'd arrive in what would be its hours on a closed day: before
                // or after them it's just shut for the night, as the clock-based "the next day" says (review).
                const double ofDay = std::fmod(std::fmod(plan.arrivalHour, 24.0) + 24.0, 24.0);
                plan.storeClosedAllDay = !IsShopDay(plan.arrivalHour) && ofDay >= shop.openHour && ofDay < shop.closeHour;
                if (plan.storeClosedAllDay)
                    summary += std::string("\nYou'd arrive on a ") + Calendar::WeekdayName(arrives.weekday)
                        + ", when the general store is closed all day (it opens " + Calendar::WeekdayName(reopens.weekday)
                        + " at " + FormatHour(opens) + ").";
                else
                {
                    // "(it opens at 8 AM)" means the next 8 AM on the clock; anything later names its day (review:
                    // a Sunday 07:30 arrival waits 25 h, for Monday).
                    const double nextOnClock = std::floor(plan.arrivalHour / 24.0) * 24.0
                        + (ofDay < shop.openHour ? 0.0 : 24.0) + shop.openHour;
                    summary += std::fabs(opens - nextOnClock) < 1e-6
                        ? "\nThe general store will be closed then (it opens at " + FormatHour(opens) + ")."
                        : std::string("\nThe general store will be closed then (it opens ") + Calendar::WeekdayName(reopens.weekday)
                            + " at " + FormatHour(opens) + ").";
                }
            }
    plan.summary = summary;
    plan.ok = true;
    return plan;
}

Result Simulation::WalkRoad(TravelDestination destination, Point from)
{
    const TravelPlan plan = PlanTravel(state_, from, destination, Layout());
    if (!plan.ok)
        return TravelBad(plan.error, revision_, state_.failed ? ResultCode::Unavailable : ResultCode::Invalid);
    const std::string where = TravelDestinationName(destination);
    // Walk it on a copy first: if she'd collapse or doze off on the way, she doesn't set out at all.
    Simulation trial = *this;
    trial.AdvanceGameHours(plan.gameHours, plan.arrival);
    if (trial.state_.failed)
        return TravelBad("You're too hungry to walk to " + where + ". Eat something first.", revision_, ResultCode::Unavailable);
    if (trial.DozeCount() != DozeCount())
        return TravelBad("You're too tired to walk to " + where + ". Rest or eat first.", revision_, ResultCode::Unavailable);
    // AdvanceGameHours passes no time at all past the calendar's supported limit.
    if (trial.state_.hour < state_.hour + plan.gameHours - 1e-3)
        return TravelBad("The calendar has reached its supported limit.", revision_);
    AdvanceGameHours(plan.gameHours, plan.arrival);
    ++revision_;
    return TravelGood("You walk to " + where + ". It's " + FormatHour(state_.hour) + ".", revision_);
}

std::vector<TravelDestination> RoadSignDestinations(const std::string& signName)
{
    if (signName == "ManorRoadSign") return {TravelDestination::Town};
    if (signName == "TownRoadSign") return {TravelDestination::Manor};
    if (signName == "GatewayRoadSign") return {TravelDestination::Town, TravelDestination::Manor};
    return {};
}

const PublicRoadSign* RoadSignNear(Point at, double reachCm)
{
    const PublicRoadSign* nearest = nullptr;
    double best = reachCm;
    for (const PublicRoadSign& sign : EstatePublicRoad().signs)
    {
        const double distance = std::hypot(at.x - sign.position.x, at.y - sign.position.y);
        if (distance <= best && !RoadSignDestinations(sign.name).empty()) { best = distance; nearest = &sign; }
    }
    return nearest;
}

std::string RoadSignLabel(const std::string& signName)
{
    const auto destinations = RoadSignDestinations(signName);
    if (destinations.size() == 2) return "Town / Manor";
    if (destinations.size() == 1) return destinations[0] == TravelDestination::Town ? "To town" : "To the manor";
    return {};
}
}