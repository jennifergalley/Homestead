#pragma once

#include <cstdint>
#include <vector>

namespace Homestead
{
// Music is an occasional visitor, not a soundtrack (Jenny, 2026-09-29): the first piece starts 45-120 s
// into a session and each piece is followed by 3-8 minutes of ambience alone. Unit is a uniform draw in
// [0, 1] (FMath::FRand in production), so the ranges are testable.
namespace MusicPacing
{
constexpr float FirstDelayMinSeconds = 45.0f;
constexpr float FirstDelayMaxSeconds = 120.0f;
constexpr float GapMinSeconds = 180.0f;
constexpr float GapMaxSeconds = 480.0f;
inline float Between(float Unit, float Min, float Max)
{
    const float T = Unit < 0.0f ? 0.0f : Unit > 1.0f ? 1.0f : Unit;
    return Min + (Max - Min) * T;
}
inline float FirstDelay(float Unit) { return Between(Unit, FirstDelayMinSeconds, FirstDelayMaxSeconds); }
inline float Gap(float Unit) { return Between(Unit, GapMinSeconds, GapMaxSeconds); }
}

// Shuffle bag over music track indices: each bag plays every track once in random order, and a
// new bag never starts with the track that just played (or, for the first bag, the track the
// previous launch last started) when another track exists. Engine-free so it can be unit tested
// with a fixed seed; production seeds it from process entropy.
class MusicShuffleBag
{
public:
    void Reset(int TrackCount, int AvoidFirst, std::uint64_t Seed)
    {
        Count = TrackCount > 0 ? TrackCount : 0;
        Last = AvoidFirst >= 0 && AvoidFirst < Count ? AvoidFirst : -1;
        State = Seed ? Seed : 0x9E3779B97F4A7C15ull;
        Order.clear();
        Position = 0;
    }

    // Next track index, or -1 when there are no tracks.
    int Next()
    {
        if (Count == 0) return -1;
        if (Position >= Order.size()) Refill();
        Last = Order[Position++];
        return Last;
    }

    int Size() const { return Count; }

private:
    std::uint64_t Random()
    {
        // SplitMix64.
        std::uint64_t Z = (State += 0x9E3779B97F4A7C15ull);
        Z = (Z ^ (Z >> 30)) * 0xBF58476D1CE4E5B9ull;
        Z = (Z ^ (Z >> 27)) * 0x94D049BB133111EBull;
        return Z ^ (Z >> 31);
    }

    void Refill()
    {
        Order.resize(static_cast<std::size_t>(Count));
        for (int Index = 0; Index < Count; ++Index) Order[static_cast<std::size_t>(Index)] = Index;
        for (int Index = Count - 1; Index > 0; --Index)
        {
            const int Swap = static_cast<int>(Random() % static_cast<std::uint64_t>(Index + 1));
            const int Held = Order[static_cast<std::size_t>(Index)];
            Order[static_cast<std::size_t>(Index)] = Order[static_cast<std::size_t>(Swap)];
            Order[static_cast<std::size_t>(Swap)] = Held;
        }
        if (Count > 1 && Order[0] == Last)
        {
            const std::size_t Swap = 1 + static_cast<std::size_t>(Random() % static_cast<std::uint64_t>(Count - 1));
            Order[0] = Order[Swap];
            Order[Swap] = Last;
        }
        Position = 0;
    }

    std::vector<int> Order;
    std::size_t Position = 0;
    int Count = 0;
    int Last = -1;
    std::uint64_t State = 0;
};
}
