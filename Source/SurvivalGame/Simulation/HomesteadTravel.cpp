#include "HomesteadTravel.h"
#include "HomesteadEstatePublicRoad.h"
#include "HomesteadShops.h"

#include <algorithm>
#include <cmath>

namespace Homestead
{
namespace
{
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
    return destination == TravelDestination::Manor ? "the manor" : "town";
}

std::string FormatWalkDuration(double gameHours)
{
    const long minutes = std::max(1L, std::lround(gameHours * 60.0));
    const long hours = minutes / 60, rest = minutes % 60;
    if (!hours) return std::to_string(rest) + " min";
    return std::to_string(hours) + " h" + (rest ? " " + std::to_string(rest) + " min" : std::string());
}

TravelPlan PlanTravel(const State& state, Point from, TravelDestination destination, const EstateLayout& layout)
{
    TravelPlan plan;
    plan.destination = destination;
    const std::string where = TravelDestinationName(destination);
    if (state.failed) { plan.error = "You need to recover first."; return plan; }
    if (!state.fixedEstate) { plan.error = "There's no road to " + where + " from here."; return plan; }
    if (!std::isfinite(from.x) || !std::isfinite(from.y)) { plan.error = "You can't set out from here."; return plan; }
    if (!std::isfinite(state.dayMinutes) || state.dayMinutes <= 0.0) { plan.error = "The day length is invalid."; return plan; }
    const PublicRoad& road = EstatePublicRoad();
    const PublicRoadStop* stop = road.FindStop(TravelStopName(destination));
    if (road.points.size() < 2 || !stop) { plan.error = "The road to " + where + " isn't mapped yet."; return plan; }
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
    if (std::hypot(from.x - stop->position.x, from.y - stop->position.y) < TravelArrivedCm)
    {
        plan.error = destination == TravelDestination::Manor ? "You're already at the manor." : "You're already in town.";
        return plan;
    }
    const PublicRoad::Nearest nearest = road.NearestTo(from);
    if (nearest.distanceCm > TravelMaxConnectorCm)
    {
        plan.error = "Make your way back to the road first.";
        return plan;
    }
    plan.connectorMetres = nearest.distanceCm / 100.0;
    plan.roadMetres = road.WalkMetres(nearest.chainage, stop->chainage);
    plan.totalMetres = plan.connectorMetres + plan.roadMetres;
    const double realSeconds = plan.totalMetres * 100.0 / RoadWalkPaceCmPerSecond;
    plan.gameHours = realSeconds * 24.0 / (state.dayMinutes * 60.0);
    plan.arrival = stop->position;
    plan.arrivalZ = stop->z;
    // Face the way she was walking: toward the town end, or back toward the manor.
    const double ahead = destination == TravelDestination::Town ? -5.0 : 5.0;
    const Point behind = road.At(stop->chainage + ahead);
    plan.arrivalYaw = std::atan2(stop->position.y - behind.y, stop->position.x - behind.x) * 180.0 / 3.14159265358979323846;
    plan.arrivalHour = state.hour + plan.gameHours;
    plan.nextDay = std::floor(plan.arrivalHour / 24.0) > std::floor(state.hour / 24.0);
    std::string summary = "Walk to " + where + " along the road: " + TravelKilometres(plan.totalMetres) + ", about "
        + FormatWalkDuration(plan.gameHours) + ".\nYou'd arrive about " + FormatHour(plan.arrivalHour)
        + (plan.nextDay ? " the next day." : ".");
    if (destination == TravelDestination::Town)
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