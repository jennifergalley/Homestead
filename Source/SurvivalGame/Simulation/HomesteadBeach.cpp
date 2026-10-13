#include "HomesteadBeach.h"
#include "HomesteadCalendar.h"

#include <cmath>

namespace Homestead
{
namespace Beach
{
double NextMorning(double hour)
{
    double next = (std::floor((hour - Calendar::DayStartHour) / 24.0) + 1.0) * 24.0 + Calendar::DayStartHour;
    // Floating-point guard: the result is always strictly ahead, by at most a day.
    if (next <= hour) next += 24.0;
    return next;
}

double RefillHour(double pickHour)
{
    const double earliest = pickHour + MinRefillHours;
    double refill = std::ceil((earliest - Calendar::DayStartHour) / 24.0) * 24.0 + Calendar::DayStartHour;
    // Floating-point guard: never earlier than the minimum bare time.
    if (refill < earliest) refill += 24.0;
    return refill;
}

Inventory DailyYield(ResourceKind kind)
{
    Inventory yield{};
    auto give = [&](Item item, int count) { yield[static_cast<int>(item)] = count; };
    switch (kind)
    {
    case ResourceKind::Seashells: give(Item::Seashell, 2); break;
    case ResourceKind::Driftwood: give(Item::Driftwood, 3); break;
    case ResourceKind::BeachPebbles: give(Item::Stone, 2); break;
    case ResourceKind::MusselBed: give(Item::Mussels, 3); break;
    case ResourceKind::LaverRocks: give(Item::Laver, 2); break;
    default: break;
    }
    return yield;
}
}
}
