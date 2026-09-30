// Portable tests for typing short names in the field book (UI/HomesteadTextEdit.h).
#include "HomesteadTextEdit.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

using namespace HomesteadTextEdit;

namespace
{
int checks = 0;
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

constexpr int Limit = 24;
constexpr std::uint32_t High = 0xD83C, Low = 0xDF3F; // U+1F33F HERB

struct Draft
{
    std::vector<char16_t> units;
    Typer typer;
    void Type(std::uint32_t unit)
    {
        std::uint32_t out[2] = {};
        const int count = typer.Type(unit, CodePoints(units.data(), static_cast<int>(units.size())), Limit, out);
        for (int i = 0; i < count; ++i) units.push_back(static_cast<char16_t>(out[i]));
    }
    void Backspace()
    {
        const int chop = BackspaceUnits(units.data(), static_cast<int>(units.size()));
        units.resize(units.size() - static_cast<std::size_t>(chop));
        typer.Reset();
    }
    int Length() const { return CodePoints(units.data(), static_cast<int>(units.size())); }
    bool Whole() const
    {
        for (std::size_t i = 0; i < units.size(); ++i)
        {
            if (IsHighSurrogate(units[i]) && (i + 1 >= units.size() || !IsLowSurrogate(units[i + 1]))) return false;
            if (IsLowSurrogate(units[i]) && (i == 0 || !IsHighSurrogate(units[i - 1]))) return false;
        }
        return true;
    }
};

void PairsStayWhole()
{
    Draft draft;
    for (int i = 0; i < Limit - 1; ++i) draft.Type('a');
    CHECK(draft.Length() == 23 && draft.units.size() == 23);
    // The 24th character is an emoji: both halves go in together, counted once.
    draft.Type(High);
    CHECK(draft.units.size() == 23);
    draft.Type(Low);
    CHECK(draft.units.size() == 25 && draft.Length() == Limit && draft.Whole());
    // Full: another emoji (or a letter) is refused whole.
    draft.Type(High);
    draft.Type(Low);
    draft.Type('b');
    CHECK(draft.units.size() == 25 && draft.Whole());
    // Backspace removes the whole emoji, then a letter.
    draft.Backspace();
    CHECK(draft.units.size() == 23 && draft.Length() == 23 && draft.Whole());
    draft.Backspace();
    CHECK(draft.units.size() == 22);
}

void OrphansAreDropped()
{
    Draft draft;
    draft.Type(Low);
    CHECK(draft.units.empty());
    // A high half followed by an ordinary letter: the half is dropped, the letter kept.
    draft.Type(High);
    draft.Type('x');
    CHECK(draft.units.size() == 1 && draft.units[0] == 'x');
    // A second high half replaces the first; its pair then goes in whole.
    draft.Type(High);
    draft.Type(0xD83D);
    draft.Type(0xDE00);
    CHECK(draft.units.size() == 3 && draft.units[1] == 0xD83D && draft.Whole());
    // Controls never go in.
    for (const std::uint32_t control : {0u, 8u, 9u, 10u, 13u, 27u, 127u}) draft.Type(control);
    CHECK(draft.units.size() == 3);
    // Backspace on a lone trailing unit (never produced here, but from a pasted string) takes one.
    const char16_t stray[] = {u'a', static_cast<char16_t>(Low)};
    CHECK(BackspaceUnits(stray, 2) == 1);
    const char16_t pair[] = {static_cast<char16_t>(High), static_cast<char16_t>(Low)};
    CHECK(BackspaceUnits(pair, 2) == 2 && BackspaceUnits(pair, 0) == 0);
    CHECK(CodePoints(pair, 2) == 1 && CodePoints(stray, 2) == 2);
}
}

int main()
{
    PairsStayWhole();
    OrphansAreDropped();
    std::cout << "Text edit: " << checks << " checks passed.\n";
    return 0;
}
