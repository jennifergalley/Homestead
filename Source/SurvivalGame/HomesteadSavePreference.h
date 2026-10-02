#pragma once

#include <cstdint>

struct FHomesteadSavePreference
{
    std::int64_t SavedAtUtc = 0;
    std::int64_t SavedRevision = 0;
    int SlotOrder = 0;
};

// Revision resolves saves written in the same second; the slot order breaks otherwise equal ties.
constexpr bool IsNewerHomesteadSave(FHomesteadSavePreference Candidate, FHomesteadSavePreference Best)
{
    if (Candidate.SavedAtUtc != Best.SavedAtUtc) return Candidate.SavedAtUtc > Best.SavedAtUtc;
    if (Candidate.SavedRevision != Best.SavedRevision) return Candidate.SavedRevision > Best.SavedRevision;
    return Candidate.SlotOrder < Best.SlotOrder;
}
