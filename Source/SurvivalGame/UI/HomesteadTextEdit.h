#pragma once

#include <cstdint>

// Portable UTF-16 typing rules for short names typed into the field book (the chest's name). Slate
// delivers one UTF-16 unit per character event, so a character outside the Basic Multilingual Plane
// (an emoji) arrives as a high surrogate and then a low one. These keep a draft from ever holding
// half a pair and count its length in code points, as Homestead::Manor::NameLength does.
namespace HomesteadTextEdit
{
inline bool IsHighSurrogate(std::uint32_t unit) { return unit >= 0xD800 && unit <= 0xDBFF; }
inline bool IsLowSurrogate(std::uint32_t unit) { return unit >= 0xDC00 && unit <= 0xDFFF; }

// Code points in `length` UTF-16 units (a pair counts once; a stray half counts as one).
template <typename Unit>
int CodePoints(const Unit* units, int length)
{
    int count = 0;
    for (int i = 0; i < length; ++i)
    {
        if (IsHighSurrogate(static_cast<std::uint32_t>(units[i])) && i + 1 < length
            && IsLowSurrogate(static_cast<std::uint32_t>(units[i + 1]))) ++i;
        ++count;
    }
    return count;
}

// Units Backspace removes from the end: a whole surrogate pair, else one.
template <typename Unit>
int BackspaceUnits(const Unit* units, int length)
{
    if (length <= 0) return 0;
    return length >= 2 && IsLowSurrogate(static_cast<std::uint32_t>(units[length - 1]))
        && IsHighSurrogate(static_cast<std::uint32_t>(units[length - 2])) ? 2 : 1;
}

// Feeds typed units one at a time. A high surrogate waits here until its low half arrives, and
// only then is the pair appended, so the draft never holds half a character.
struct Typer
{
    std::uint32_t pendingHigh = 0;

    void Reset() { pendingHigh = 0; }
    // Given the draft's current code points and the limit, writes the units to append to `out`
    // and returns how many (0, 1 or 2). Control characters, an orphan low surrogate and a
    // character past the limit append nothing.
    int Type(std::uint32_t unit, int codePoints, int maxCodePoints, std::uint32_t out[2])
    {
        if (IsHighSurrogate(unit)) { pendingHigh = unit; return 0; }
        if (IsLowSurrogate(unit))
        {
            const std::uint32_t high = pendingHigh;
            pendingHigh = 0;
            if (!high || codePoints >= maxCodePoints) return 0;
            out[0] = high;
            out[1] = unit;
            return 2;
        }
        // Anything else abandons a high surrogate whose low half never came.
        pendingHigh = 0;
        if (unit < 32 || unit == 127 || codePoints >= maxCodePoints) return 0;
        out[0] = unit;
        return 1;
    }
};
}
