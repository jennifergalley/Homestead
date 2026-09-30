#pragma once

#include "CoreMinimal.h"
#include "Simulation/HomesteadSimulation.h"

namespace HomesteadWorldKeys
{
constexpr uint64 Seed = 1469598103934665603ull;

inline uint64 Mix(uint64 Hash, uint64 Value)
{
    return Hash ^ (Value + 0x9e3779b97f4a7c15ull + (Hash << 6) + (Hash >> 2));
}

inline uint64 Bits(double Value)
{
    uint64 Out = 0;
    FMemory::Memcpy(&Out, &Value, sizeof(Out));
    return Out;
}

inline uint64 Pair(int32 A, int32 B)
{
    return (static_cast<uint64>(static_cast<uint32>(A)) << 32) | static_cast<uint32>(B);
}

// A resource visual's signature: what it is, where, and whether it's shown cleared.
inline FString ResourceSignature(Homestead::ResourceKind Kind, Homestead::Point Position, bool bCleared)
{
    uint64 Hash = Mix(Seed, static_cast<uint64>(Kind));
    Hash = Mix(Hash, Bits(Position.x));
    Hash = Mix(Hash, Bits(Position.y));
    Hash = Mix(Hash, bCleared ? 1 : 0);
    return FString::Printf(TEXT("%016llx"), static_cast<unsigned long long>(Hash));
}
}
