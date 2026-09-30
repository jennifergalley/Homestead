#include "HomesteadHudTiming.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

using HomesteadHud::ControlsHintWindow;

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

void ControlsHint()
{
    ControlsHintWindow window;
    CHECK(window.Showing() && window.Remaining() == 60.0);
    // Visible frames count; a paused book or shop (hidden strip) freezes the clock.
    for (int frame = 0; frame < 30 * 20; ++frame) window.Advance(1.0 / 30.0, true);
    CHECK(window.Showing() && std::abs(window.shownSeconds - 20.0) < 1e-6);
    window.Advance(600.0, false);
    CHECK(window.Showing() && std::abs(window.shownSeconds - 20.0) < 1e-6);
    // Negative or zero deltas never run it backwards.
    window.Advance(-5.0, true);
    window.Advance(0.0, true);
    CHECK(std::abs(window.shownSeconds - 20.0) < 1e-6);
    window.Advance(39.9, true);
    CHECK(window.Showing());
    window.Advance(0.2, true);
    CHECK(!window.Showing() && window.shownSeconds == 60.0 && window.Remaining() == 0.0);
    // A long hitch can't overshoot, and it stays retired until restarted.
    window.Advance(1000.0, true);
    CHECK(!window.Showing() && window.shownSeconds == 60.0);
    // A new game or "Reset action hints" brings it back for another full minute.
    window.Restart();
    CHECK(window.Showing() && window.Remaining() == 60.0);
}
}

int main()
{
    ControlsHint();
    std::cout << "HUD timing: " << checks << " checks passed.\n";
    return 0;
}
