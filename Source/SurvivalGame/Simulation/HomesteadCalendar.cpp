#include "HomesteadCalendar.h"

#include <cmath>

namespace Homestead
{
namespace Calendar
{
namespace
{
int CalendarFloorDiv(int value, int divisor) { return value >= 0 ? value / divisor : -((-value + divisor - 1) / divisor); }
int CalendarWrap(int value, int modulus) { return ((value % modulus) + modulus) % modulus; }
}

int DayIndex(double hour)
{
    if (!std::isfinite(hour)) return 0;
    // Stepped time drifts by tiny fractions: a sleep that ends "at" 06:00 counts as the new day.
    constexpr double Tolerance = 1e-7;
    return static_cast<int>(std::floor((hour - DayStartHour) / 24.0 + Tolerance));
}

Date DateOfDay(int dayIndex)
{
    Date date;
    date.dayIndex = dayIndex;
    date.seasonIndex = CalendarFloorDiv(dayIndex, DaysPerSeason);
    date.dayOfSeason = dayIndex - date.seasonIndex * DaysPerSeason + 1;
    date.season = static_cast<Season>(CalendarWrap(date.seasonIndex, SeasonsPerYear));
    date.year = FirstYear + CalendarFloorDiv(dayIndex, DaysPerYear);
    date.weekday = static_cast<Weekday>(CalendarWrap(dayIndex, DaysPerWeek));
    date.daysLeftInSeason = DaysPerSeason - date.dayOfSeason + 1;
    return date;
}

const char* SeasonName(Season season)
{
    static const char* names[] = {"Spring", "Summer", "Autumn", "Winter"};
    const int index = static_cast<int>(season);
    return index >= 0 && index < SeasonsPerYear ? names[index] : "Spring";
}

const char* WeekdayName(Weekday day)
{
    static const char* names[] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
    const int index = static_cast<int>(day);
    return index >= 0 && index < DaysPerWeek ? names[index] : "Monday";
}

const char* WeekdayShort(Weekday day)
{
    static const char* names[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    const int index = static_cast<int>(day);
    return index >= 0 && index < DaysPerWeek ? names[index] : "Mon";
}

std::string ShortDate(const Date& date)
{
    return std::string(WeekdayShort(date.weekday)) + ", " + SeasonName(date.season) + " " + std::to_string(date.dayOfSeason);
}

std::string LongDate(const Date& date)
{
    return std::string(SeasonName(date.season)) + " " + std::to_string(date.dayOfSeason) + ", " + std::to_string(date.year);
}

std::string SeasonWarning(const Date& date)
{
    if (date.daysLeftInSeason > WarningDays) return {};
    if (date.daysLeftInSeason <= 1) return std::string("Last day of ") + SeasonName(date.season);
    return std::to_string(date.daysLeftInSeason) + " days left";
}

int DaysLeftInRun(const Date& date, SeasonMask mask)
{
    if (!InSeason(mask, date.season)) return 0;
    if ((mask & AllSeasons) == AllSeasons) return 1 << 20;
    int days = date.daysLeftInSeason;
    for (int next = 1; next < SeasonsPerYear; ++next)
    {
        const auto season = static_cast<Season>(CalendarWrap(static_cast<int>(date.season) + next, SeasonsPerYear));
        if (!InSeason(mask, season)) break;
        days += DaysPerSeason;
    }
    return days;
}

std::string SeasonList(SeasonMask mask)
{
    // Start after a season the crop doesn't grow in, so a run that wraps the year reads in order
    // ("Autumn, Winter and Spring").
    int first = 0;
    if ((mask & AllSeasons) != AllSeasons)
        for (int i = 0; i < SeasonsPerYear; ++i)
            if (!InSeason(mask, static_cast<Season>(i))) { first = (i + 1) % SeasonsPerYear; break; }
    std::string names[SeasonsPerYear];
    int count = 0;
    for (int i = 0; i < SeasonsPerYear; ++i)
    {
        const auto season = static_cast<Season>((first + i) % SeasonsPerYear);
        if (InSeason(mask, season)) names[count++] = SeasonName(season);
    }
    std::string text;
    for (int i = 0; i < count; ++i)
        text += (i == 0 ? "" : i == count - 1 ? " and " : ", ") + names[i];
    return text;
}
}
}
