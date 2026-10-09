#include "HomesteadSimulation.h"
#include "HomesteadCrops.h"

// The calendar's rollover hooks (rework-farming-calendar-and-period-crafting, design §1). Step calls
// OnNewDay once for each 06:00 rollover it crosses; OnNewDay calls OnNewSeason first when the day is
// the first of a season. Round-2 lanes add their day or season work here, one call each, keeping the
// work itself in their own files:
//   - Harvest (B): nothing; new crops wither through their CropInfo season masks.
//   - Seedsman (C): the daily in-season seed stock belongs in OnNewDay.
//   - Seasons (D): forage renewal windows read Calendar::DateAt at use; add season-start work here.
namespace Homestead
{
static_assert(DayRolloverHour == Calendar::DayStartHour, "The shops' sell-down and the calendar share the 06:00 rollover.");

void Simulation::OnNewDay(const Calendar::Date& today)
{
    if (today.dayOfSeason == 1 && today.dayIndex > 0)
        OnNewSeason(today, Calendar::DateOfDay(today.dayIndex - 1).season);
    // Townsfolk buy down her goods in the shops each morning.
    SellDownShops();
    CreepWeeds(today.dayIndex);
    UpkeepRegrowth(today.dayIndex);
}

void Simulation::OnNewSeason(const Calendar::Date& today, Season from)
{
    lastSeasonChange_ = {};
    lastSeasonChange_.from = from;
    lastSeasonChange_.to = today.season;
    lastSeasonChange_.witheredPlots = Crops::WitherOutOfSeason(state_, today.season);
    ++seasonChanges_;
}
}
