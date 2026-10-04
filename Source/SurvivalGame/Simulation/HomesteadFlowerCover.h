#pragma once

#include "HomesteadEstate.h"

#include <unordered_set>

namespace Homestead
{
// Derived presentation reservations; rebuilt from authoritative plots/layout only when they change.
class FlowerCoverMask
{
public:
    FlowerCoverMask(const State& state, const EstateLayout& layout);
    bool Excludes(Point centre, double radiusCm) const;

private:
    std::unordered_set<std::uint64_t> plots_;
    std::vector<std::vector<Point>> landmarks_;
};
}
