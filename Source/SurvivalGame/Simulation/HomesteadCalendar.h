#pragma once

#include <string>

// The estate calendar (rework-farming-calendar-and-period-crafting, design §1). Four 28-day seasons,
// seven named weekdays and years from Monday, Spring 1, 1851. Everything derives from State::hour
// (a running game-hour count, 6.0 at a new game) and the 06:00 day rollover; nothing is saved.
namespace Homestead
{
enum class Season : int { Spring, Summer, Autumn, Winter, Count };
enum class Weekday : int { Monday, Tuesday, Wednesday, Thursday, Friday, Saturday, Sunday, Count };

// A set of seasons, e.g. SeasonBit(Season::Spring) | SeasonBit(Season::Summer).
using SeasonMask = unsigned;
constexpr SeasonMask SeasonBit(Season season) { return 1u << static_cast<unsigned>(season); }
constexpr SeasonMask AllSeasons = 0xFu;
constexpr bool InSeason(SeasonMask mask, Season season) { return (mask & SeasonBit(season)) != 0; }

namespace Calendar
{
constexpr int DaysPerSeason = 28;
constexpr int SeasonsPerYear = 4;
constexpr int DaysPerYear = DaysPerSeason * SeasonsPerYear;
constexpr int DaysPerWeek = 7;
constexpr int FirstYear = 1851;
// The day turns over at 06:00, when she wakes and the shops sell down (HomesteadShops.h).
constexpr double DayStartHour = 6.0;
// The HUD warns in a season's last WarningDays days ("3 days left").
constexpr int WarningDays = 3;

struct Date
{
    int dayIndex = 0;        // whole days since Spring 1, 1851 (0-based)
    int dayOfSeason = 1;     // 1-28
    Season season = Season::Spring;
    int year = FirstYear;
    Weekday weekday = Weekday::Monday;
    int daysLeftInSeason = DaysPerSeason; // counting today: 3 on day 26, 1 on day 28
    int seasonIndex = 0;     // whole seasons since Spring 1851 (changes exactly when the season does)
};

// Day 0 is Spring 1, 1851; a day runs 06:00 to 06:00, so 02:00 still belongs to the day before.
int DayIndex(double hour);
Date DateOfDay(int dayIndex);
inline Date DateAt(double hour) { return DateOfDay(DayIndex(hour)); }

const char* SeasonName(Season season);   // "Spring"
const char* WeekdayName(Weekday day);    // "Monday"
const char* WeekdayShort(Weekday day);   // "Mon"
// "Mon, Spring 12" (the HUD).
std::string ShortDate(const Date& date);
// "Spring 12, 1851" (save labels, the journal).
std::string LongDate(const Date& date);
// "3 days left" / "Last day of Spring"; empty outside the season's last WarningDays days.
std::string SeasonWarning(const Date& date);
// Days from `date` (counting it) until the last day of the unbroken run of seasons in `mask` that
// contains it; 0 when `date` is out of the mask. A mask of every season never ends (a large number).
int DaysLeftInRun(const Date& date, SeasonMask mask);
// "Spring and Summer", "Autumn, Winter and Spring".
std::string SeasonList(SeasonMask mask);
}
}
