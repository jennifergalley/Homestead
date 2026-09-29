// Calendar, gentle hunger and crop seasons (rework-farming-calendar-and-period-crafting, lane A).
#include "HomesteadCalendar.h"
#include "HomesteadCrops.h"
#include "HomesteadEstate.h"
#include "HomesteadSimulation.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

using namespace Homestead;

namespace
{
int checks = 0;
int cases = 0;
void Check(bool condition, const char* expression, int line)
{
    ++checks;
    if (!condition)
    {
        std::cerr << "FAIL line " << line << ": " << expression << '\n';
        std::exit(1);
    }
}
#define CHECK(expression) Check(static_cast<bool>(expression), #expression, __LINE__)
void Okay(const Result& result, int line)
{
    ++checks;
    if (!result.ok)
    {
        std::cerr << "FAIL line " << line << ": " << result.message << '\n';
        std::exit(1);
    }
}
#define OK(expression) Okay(expression, __LINE__)
void Run(const char* name, void (*test)())
{
    test();
    ++cases;
    std::cout << "PASS " << name << '\n';
}
bool Close(double a, double b, double tolerance = 1e-6) { return std::abs(a - b) <= tolerance; }

// A plain 200 m square estate with no placements, so any ground near the spawn can be tilled.
constexpr Point Spawn{10000.0, 10000.0};
EstateLayout SquareLayout()
{
    EstateLayout layout;
    layout.landmarks = {
        {Anchor::StandingRoomSpawn, Spawn, 0.0, 0.0},
        {Anchor::TownSquare, {100000.0, 100000.0}, 0.0, 0.0},
    };
    layout.polygons = {
        {Anchor::EstateBoundary, {{0.0, 0.0}, {20000.0, 0.0}, {20000.0, 20000.0}, {0.0, 20000.0}}},
    };
    return layout;
}

Simulation Estate()
{
    EstatePlacements none;
    none.bakeVersion = 7;
    Simulation sim;
    OK(sim.NewEstateGame(SquareLayout(), none));
    sim.SetWaterProbe([](Point) { return true; });
    return sim;
}

std::uint64_t Checksum(const std::string& body)
{
    std::uint64_t hash = UINT64_C(14695981039346656037);
    for (unsigned char c : body) { hash ^= c; hash *= UINT64_C(1099511628211); }
    return hash;
}
// Re-wraps an edited save payload in a valid envelope.
std::string Rewrap(const std::string& save, const std::string& payload)
{
    std::string header = save.substr(0, save.find('\n'));
    const std::string version = header.substr(0, header.find(' ', 10));
    return version + " " + std::to_string(payload.size()) + " " + std::to_string(Checksum(payload)) + "\n" + payload;
}
std::string Payload(const std::string& save) { return save.substr(save.find('\n') + 1); }

const Plot& PlotById(const Simulation& sim, int id)
{
    for (const auto& plot : sim.GetState().plots)
        if (plot.id == id) return plot;
    CHECK(false);
    return sim.GetState().plots.front();
}

void EatUntilFed(Simulation& sim)
{
    while (sim.GetState().hunger < 90.0)
    {
        if (sim.Count(Item::Berries) == 0) OK(sim.GrantItems(Item::Berries, 4));
        OK(sim.Eat(Item::Berries));
    }
}

// The hour of the given 1-based day of a season in 1851 (Spring = 0), at `hourOfDay`.
double HourOf(int season, int day, double hourOfDay)
{
    return (season * Calendar::DaysPerSeason + day - 1) * 24.0 + hourOfDay;
}

void CalendarMath()
{
    using namespace Calendar;
    CHECK(DayIndex(6.0) == 0 && DayIndex(29.99) == 0 && DayIndex(30.0) == 1);
    // 02:00 still belongs to the day before; the day turns at 06:00.
    CHECK(DayIndex(24.0 + 2.0) == 0);
    const Date first = DateAt(6.0);
    CHECK(first.dayIndex == 0 && first.dayOfSeason == 1 && first.season == Season::Spring);
    CHECK(first.year == 1851 && first.weekday == Weekday::Monday && first.daysLeftInSeason == 28);
    CHECK(ShortDate(first) == "Mon, Spring 1" && LongDate(first) == "Spring 1, 1851");
    CHECK(SeasonWarning(first).empty());
    CHECK(ShortDate(DateOfDay(6)) == "Sun, Spring 7" && ShortDate(DateOfDay(7)) == "Mon, Spring 8");
    CHECK(ShortDate(DateOfDay(11)) == "Fri, Spring 12");
    // The end-of-season warning: nothing on day 25, "3 days left" on 26, the last day named.
    CHECK(SeasonWarning(DateOfDay(24)).empty());
    CHECK(DateOfDay(25).dayOfSeason == 26 && DateOfDay(25).daysLeftInSeason == 3);
    CHECK(SeasonWarning(DateOfDay(25)) == "3 days left" && SeasonWarning(DateOfDay(26)) == "2 days left");
    CHECK(SeasonWarning(DateOfDay(27)) == "Last day of Spring");
    CHECK(DateOfDay(28).season == Season::Summer && DateOfDay(28).dayOfSeason == 1);
    CHECK(DateOfDay(56).season == Season::Autumn && DateOfDay(84).season == Season::Winter);
    // Winter 28, 1851 rolls over into Spring 1, 1852.
    const Date lastOfYear = DateOfDay(DaysPerYear - 1);
    CHECK(LongDate(lastOfYear) == "Winter 28, 1851" && SeasonWarning(lastOfYear) == "Last day of Winter");
    CHECK(LongDate(DateAt((DaysPerYear) * 24.0 + 6.0)) == "Spring 1, 1852");
    CHECK(LongDate(DateAt((DaysPerYear) * 24.0 + 5.99)) == "Winter 28, 1851");
    CHECK(DateOfDay(DaysPerYear).seasonIndex == 4 && DateOfDay(DaysPerYear - 1).seasonIndex == 3);
    // A year is 16 weeks, so every year starts on a Monday.
    CHECK(DateOfDay(DaysPerYear).weekday == Weekday::Monday);
    CHECK(std::string(WeekdayName(Weekday::Sunday)) == "Sunday" && std::string(SeasonName(Season::Autumn)) == "Autumn");
    // Season runs: the days left count today and every following season in the mask.
    const SeasonMask spring = SeasonBit(Season::Spring);
    const SeasonMask springSummer = spring | SeasonBit(Season::Summer);
    const SeasonMask autumnWinter = SeasonBit(Season::Autumn) | SeasonBit(Season::Winter);
    CHECK(DaysLeftInRun(DateOfDay(25), spring) == 3 && DaysLeftInRun(DateOfDay(25), springSummer) == 31);
    CHECK(DaysLeftInRun(DateOfDay(25), autumnWinter) == 0);
    CHECK(DaysLeftInRun(DateOfDay(84 + 24), autumnWinter) == 4);
    CHECK(DaysLeftInRun(DateOfDay(0), AllSeasons) > DaysPerYear);
    CHECK(SeasonList(springSummer) == "Spring and Summer" && SeasonList(autumnWinter) == "Autumn and Winter");
    CHECK(SeasonList(springSummer | SeasonBit(Season::Autumn)) == "Spring, Summer and Autumn");
    CHECK(SeasonList(autumnWinter | spring) == "Autumn, Winter and Spring");
    CHECK(SeasonList(SeasonBit(Season::Winter)) == "Winter");
}

void NewGameDateAndDayLength()
{
    Simulation sim = Estate();
    CHECK(Calendar::ShortDate(sim.Today()) == "Mon, Spring 1");
    CHECK(sim.DayNumber() == 1 && std::string(sim.SeasonName()) == "Spring");
    // A new game's day lasts 30 real minutes: 15 real minutes unpaused pass 12 game hours.
    CHECK(sim.GetState().dayMinutes == 30.0);
    const double before = sim.GetState().hour;
    sim.Advance(15.0 * 60.0, Spawn);
    CHECK(Close(sim.GetState().hour - before, 12.0));
    // Settings keep the longer days, and the choice saves with the game.
    OK(sim.SetDayMinutes(60.0));
    Simulation loaded;
    EstatePlacements none;
    none.bakeVersion = 7;
    loaded.SetLayout(SquareLayout());
    loaded.SetPlacements(none);
    OK(loaded.Deserialize(sim.Serialize()));
    CHECK(loaded.GetState().dayMinutes == 60.0);
    // The rain schedule keeps every third day, keyed off the calendar day.
    CHECK(!IsRainingAt(HourOf(0, 1, 12.0)) && IsRainingAt(HourOf(0, 2, 12.0)) && !IsRainingAt(HourOf(0, 3, 12.0)));
}

void SeasonRolloverHook()
{
    Simulation sim = Estate();
    OK(sim.PassDaysForPlaytest(27.0, false, Spawn));
    CHECK(Calendar::ShortDate(sim.Today()) == "Sun, Spring 28" && sim.SeasonChanges() == 0);
    OK(sim.PassDaysForPlaytest(0.5, false, Spawn));
    CHECK(sim.SeasonChanges() == 0);
    OK(sim.PassDaysForPlaytest(0.5, false, Spawn));
    CHECK(sim.SeasonChanges() == 1 && Calendar::LongDate(sim.Today()) == "Summer 1, 1851");
    CHECK(sim.LastSeasonChange().from == Season::Spring && sim.LastSeasonChange().to == Season::Summer);
    CHECK(sim.LastSeasonChange().witheredPlots == 0);
    // On through the year: Winter 28, 1851 turns over into Spring 1, 1852.
    OK(sim.PassDaysForPlaytest(60.0, false, Spawn));
    OK(sim.PassDaysForPlaytest(24.0, false, Spawn));
    CHECK(sim.Today().dayIndex == 28 + 84 && Calendar::LongDate(sim.Today()) == "Spring 1, 1852");
    CHECK(sim.SeasonChanges() == 4 && sim.LastSeasonChange().from == Season::Winter
        && sim.LastSeasonChange().to == Season::Spring);
    CHECK(sim.Today().weekday == Weekday::Monday);
}

void GentleHunger()
{
    Simulation sim = Estate();
    OK(sim.GrantStarterKit(Spawn, {1.0, 0.0}, false));
    const Structure* bed = nullptr;
    for (const auto& piece : sim.GetState().structures) if (piece.kind == Piece::Bed) bed = &piece;
    CHECK(bed != nullptr);
    const Point bedSide = sim.StructureCenter(*bed);
    CHECK(sim.GetHungerState() == HungerState::Fed && Close(sim.WorkCost(2.0), 2.0));
    // Two and a half days without food: she's famished but never fails, and time keeps passing.
    sim.AdvanceGameHours(60.0, bedSide);
    CHECK(!sim.GetState().failed && sim.GetState().hunger == 0.0);
    CHECK(sim.GetHungerState() == HungerState::Famished && Close(sim.WorkCost(2.0), 3.0));
    const double hour = sim.GetState().hour;
    sim.AdvanceGameHours(72.0, bedSide);
    CHECK(!sim.GetState().failed && Close(sim.GetState().hour, hour + 72.0));
    // Famished sleep recovers at half the rate.
    OK(sim.SetEnergy(20.0));
    OK(sim.Sleep(2.0, bedSide));
    CHECK(Close(sim.GetState().energy, 20.0 + 2.0 * Exertion::SleepPerHour * 0.5));
    // Work costs half as much again: tilling a square.
    OK(sim.GrantItems(Item::DiggingStick, 1));
    const int gx = GardenCell(Spawn.x) - 20, gy = GardenCell(Spawn.y) - 20;
    OK(sim.SetEnergy(50.0));
    OK(sim.Till(gx, gy, GardenCellCenter(gx, gy)));
    CHECK(Close(sim.GetState().energy, 50.0 - Exertion::TillEnergy * 1.5));
    // A saved famished estate game loads as it is, not failed.
    {
        Simulation loaded;
        EstatePlacements none;
        none.bakeVersion = 7;
        loaded.SetLayout(SquareLayout());
        loaded.SetPlacements(none);
        OK(loaded.Deserialize(sim.Serialize()));
        CHECK(!loaded.GetState().failed && loaded.GetState().hunger == 0.0);
        // An estate save from before gentle hunger, failed for hunger alone, carries on unfailed.
        const std::string saved = sim.Serialize();
        std::string body = Payload(saved);
        const auto lineEnd = body.find('\n');
        std::string vitals = body.substr(0, lineEnd);
        std::string fields[6];
        {
            std::size_t start = 0;
            for (int i = 0; i < 6; ++i)
            {
                const auto space = vitals.find(' ', start);
                fields[i] = vitals.substr(start, space == std::string::npos ? std::string::npos : space - start);
                start = space + 1;
            }
        }
        CHECK(fields[2] == "0" && fields[4] == "0");
        fields[4] = "1";
        vitals = fields[0];
        for (int i = 1; i < 6; ++i) vitals += " " + fields[i];
        body.replace(0, lineEnd, vitals);
        OK(loaded.Deserialize(Rewrap(saved, body)));
        CHECK(!loaded.GetState().failed);
    }
    // A little food lifts her to Hungry: three-quarter recovery and a quarter more work.
    OK(sim.GrantItems(Item::Berries, 1));
    OK(sim.Eat(Item::Berries));
    CHECK(sim.GetHungerState() == HungerState::Hungry && Close(sim.WorkCost(2.0), 2.5));
    OK(sim.SetEnergy(20.0));
    OK(sim.Sleep(2.0, bedSide));
    CHECK(sim.GetHungerState() == HungerState::Hungry);
    CHECK(Close(sim.GetState().energy, 20.0 + 2.0 * Exertion::SleepPerHour * 0.75));
    // Eating a meal lifts the penalty at once.
    EatUntilFed(sim);
    CHECK(sim.GetHungerState() == HungerState::Fed);
    OK(sim.SetEnergy(50.0));
    OK(sim.Till(gx + 1, gy, GardenCellCenter(gx + 1, gy)));
    CHECK(Close(sim.GetState().energy, 50.0 - Exertion::TillEnergy));
    OK(sim.SetEnergy(20.0));
    OK(sim.Sleep(2.0, bedSide));
    CHECK(Close(sim.GetState().energy, 20.0 + 2.0 * Exertion::SleepPerHour));
    // The seeded woodland keeps its legacy rule: no penalties, and hunger at 0 still fails her.
    Simulation woodland;
    CHECK(woodland.GetHungerState() == HungerState::Fed);
    woodland.AdvanceGameHours(60.0, {-750, 150});
    CHECK(woodland.GetState().failed && woodland.GetState().hunger == 0.0);
    CHECK(woodland.GetHungerState() == HungerState::Fed);
}

void CropSeasonsAndWithering()
{
    CHECK(GrowsIn(CropKind::Potatoes, Season::Spring) && !GrowsIn(CropKind::Potatoes, Season::Summer));
    CHECK(GrowsIn(CropKind::Carrots, Season::Summer) && !GrowsIn(CropKind::Turnips, Season::Spring));
    CHECK(OutOfSeasonText(CropKind::Turnips) == "Turnips grow in Autumn and Winter.");
    CHECK(OutOfSeasonText(CropKind::Cabbage) == "Cabbage grows in Autumn and Winter.");
    CHECK(OutOfSeasonText(CropKind::BroadBeans) == "Broad beans grow in Spring.");
    // Cabbage sown on Winter 25 can't make its nine days; on Autumn 20 it can.
    CHECK(TooLateText(CropKind::Cabbage, GetCropInfo(CropKind::Cabbage).growHours, HourOf(3, 25, 10.0))
        == "Won't ripen before Winter ends.");
    CHECK(TooLateText(CropKind::Cabbage, GetCropInfo(CropKind::Cabbage).growHours, HourOf(2, 20, 10.0)).empty());
    // Potatoes (six days) sown on Spring 22 in the morning just make it; on Spring 23 they don't.
    CHECK(TooLateText(CropKind::Potatoes, 144.0, HourOf(0, 22, 10.0)).empty());
    CHECK(TooLateText(CropKind::Potatoes, 144.0, HourOf(0, 23, 10.0)) == "Won't ripen before Spring ends.");
    CHECK(Close(HoursLeftInSeasons(CropKind::Potatoes, HourOf(0, 28, 6.0)), 24.0));
    CHECK(HoursLeftInSeasons(CropKind::Turnips, HourOf(0, 1, 6.0)) == 0.0);

    Simulation sim = Estate();
    OK(sim.GrantItems(Item::DiggingStick, 1));
    OK(sim.GrantItems(Item::WateringCan, 1));
    OK(sim.GrantItems(Item::SeedPotato, 3));
    OK(sim.GrantItems(Item::CarrotSeed, 1));
    OK(sim.GrantItems(Item::TurnipSeed, 1));
    const int gx = GardenCell(Spawn.x) + 10, gy = GardenCell(Spawn.y) + 10;
    int plots[4] = {};
    for (int i = 0; i < 4; ++i)
    {
        const Point square = GardenCellCenter(gx + i, gy);
        OK(sim.Till(gx + i, gy, square));
        plots[i] = sim.FindNearestPlot(square, 1);
    }
    const auto at = [&](int i) { return GardenCellCenter(gx + i, gy); };
    // Spring: turnips are refused, naming their seasons; early potatoes go in.
    const Result refused = sim.Plant(plots[0], at(0), CropKind::Turnips);
    CHECK(!refused.ok && refused.message == "Turnips grow in Autumn and Winter.");
    CHECK(sim.Count(Item::TurnipSeed) == 1);
    const Result early = sim.Plant(plots[0], at(0), CropKind::Potatoes);
    OK(early);
    CHECK(early.message == "Planted potatoes. Ready in about 6 days if watered.");
    // Spring 26: carrots still have Summer ahead; potatoes are sown too late, with a warning.
    OK(sim.PassDaysForPlaytest(25.0, true, at(0)));
    CHECK(sim.DayNumber() == 26 && IsRipe(PlotById(sim, plots[0])));
    OK(sim.Plant(plots[1], at(1), CropKind::Carrots));
    const Result late = sim.Plant(plots[2], at(2), CropKind::Potatoes);
    OK(late);
    CHECK(late.message == "Planted potatoes. Won't ripen before Spring ends.");
    CHECK(PlotStatus(PlotById(sim, plots[2]), sim.GetState().hour).find("Won't ripen before Spring ends.") != std::string::npos);
    CHECK(PlotStatus(PlotById(sim, plots[1]), sim.GetState().hour).find("Won't ripen") == std::string::npos);
    OK(sim.Plant(plots[3], at(3), CropKind::Potatoes));
    // Summer 1: both potato plots and the ripe early one wither; the carrots grow on.
    OK(sim.PassDaysForPlaytest(3.0, true, at(0)));
    CHECK(Calendar::LongDate(sim.Today()) == "Summer 1, 1851");
    CHECK(sim.SeasonChanges() == 1 && sim.LastSeasonChange().witheredPlots == 3);
    for (int i : {0, 2, 3})
    {
        const Plot& plot = PlotById(sim, plots[i]);
        CHECK(plot.planted && plot.withered && !IsRipe(plot) && StageOf(plot) == CropStage::Withered);
        CHECK(PlotStatus(plot) == "Potatoes: withered. Clear it with the hoe");
    }
    const double carrotGrowth = PlotById(sim, plots[1]).growth;
    CHECK(!PlotById(sim, plots[1]).withered && carrotGrowth > 0.4);
    // A withered plant yields nothing, takes no water, and doesn't grow.
    const int potatoes = sim.Count(Item::Potato);
    CHECK(!sim.HarvestCrop(plots[0], at(0)).ok && sim.Count(Item::Potato) == potatoes);
    CHECK(!sim.Plant(plots[0], at(0), CropKind::Carrots).ok);
    OK(sim.FillWater(at(0)));
    CHECK(!sim.Water(plots[0], at(0)).ok);
    const double witheredGrowth = PlotById(sim, plots[2]).growth;
    OK(sim.PassDaysForPlaytest(1.0, true, at(0)));
    CHECK(PlotById(sim, plots[2]).growth == witheredGrowth && PlotById(sim, plots[1]).growth > carrotGrowth);
    // The withered state saves; saves without the section load every plot living.
    {
        const std::string saved = sim.Serialize();
        EstatePlacements none;
        none.bakeVersion = 7;
        Simulation loaded;
        loaded.SetLayout(SquareLayout());
        loaded.SetPlacements(none);
        OK(loaded.Deserialize(saved));
        CHECK(loaded.Serialize() == saved && PlotById(loaded, plots[2]).withered);
        std::string body = Payload(saved);
        const auto tag = body.find(std::string(Crops::WitheredSaveTag) + " 3 ");
        CHECK(tag != std::string::npos);
        const std::string line = body.substr(tag, body.find('\n', tag) + 1 - tag);
        body.erase(tag, line.size());
        OK(loaded.Deserialize(Rewrap(saved, body)));
        CHECK(!PlotById(loaded, plots[2]).withered);
        // A repeated or unknown id is refused.
        CHECK(loaded.Deserialize(Rewrap(saved, body + line + line)).code == ResultCode::CorruptSave);
    }
    // The hoe clears a withered plant back to tilled soil, ready for a summer crop.
    const Result cleared = sim.Weed(plots[0], at(0));
    OK(cleared);
    CHECK(cleared.message == "Hoed out the withered potatoes. The soil is ready to plant.");
    CHECK(!PlotById(sim, plots[0]).planted && !PlotById(sim, plots[0]).withered);
    OK(sim.ClearWithered(plots[2], at(2)));
    CHECK(!sim.ClearWithered(plots[2], at(2)).ok);
    OK(sim.GrantItems(Item::CarrotSeed, 1));
    OK(sim.Plant(plots[0], at(0), CropKind::Carrots));
    // Out of season in Summer: no potatoes.
    OK(sim.GrantItems(Item::SeedPotato, 1));
    const Result summerPotatoes = sim.Plant(plots[2], at(2), CropKind::Potatoes);
    CHECK(!summerPotatoes.ok && summerPotatoes.message == "Potatoes grow in Spring.");
}
}

int main()
{
    Run("calendar math: 28-day seasons, weekdays and years", CalendarMath);
    Run("a new game starts Mon, Spring 1 with 30-minute days", NewGameDateAndDayLength);
    Run("the season rollover hook fires once per season", SeasonRolloverHook);
    Run("hunger slows her but never fails the estate", GentleHunger);
    Run("crops grow in season and wither when it ends", CropSeasonsAndWithering);
    std::cout << cases << " scenarios, " << checks << " explicit checks passed.\n";
    return 0;
}
