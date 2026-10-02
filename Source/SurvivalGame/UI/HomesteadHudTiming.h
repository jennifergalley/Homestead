#pragma once

#include <algorithm>

// Portable HUD timing rules (no engine types, so native tests cover them).
namespace HomesteadHud
{
// The top-left controls strip is a first-minute reminder: it shows for the first WindowSeconds of
// real time it has actually been on screen after boot, a new game or "Reset action hints". Time in a
// paused book or shop (or anywhere else the strip is hidden) doesn't count, so she never misses it.
struct ControlsHintWindow
{
    static constexpr double WindowSeconds = 60.0;
    double shownSeconds = 0.0;

    void Restart() { shownSeconds = 0.0; }
    void Advance(double realSeconds, bool onScreen)
    {
        if (onScreen && realSeconds > 0.0) shownSeconds = std::min(WindowSeconds, shownSeconds + realSeconds);
    }
    bool Showing() const { return shownSeconds < WindowSeconds; }
    double Remaining() const { return WindowSeconds - shownSeconds; }
};
}
