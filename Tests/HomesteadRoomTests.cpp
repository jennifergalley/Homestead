// Portable tests for the standing room's sound (HomesteadRoomAudio).
#include "HomesteadRoomAudio.h"
#include "HomesteadSimulation.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

using namespace Homestead;

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
bool Close(double a, double b, double tolerance = 1e-9) { return std::abs(a - b) <= tolerance; }
}

int main()
{
    using namespace RoomAudio;
    // Ambience: the user's setting outdoors, 0.4 of it indoors, easing in between; muted stays muted.
    for (const double setting : {0.7, 1.0, 0.25})
    {
        CHECK(Close(AmbienceGain(setting, 0.0), setting));
        CHECK(Close(AmbienceGain(setting, 1.0), setting * AmbienceIndoorGain));
        CHECK(Close(AmbienceGain(setting, 0.5), setting * 0.7));
        double previous = 2.0;
        for (double indoors = 0.0; indoors <= 1.0; indoors += 0.1)
        {
            const double gain = AmbienceGain(setting, indoors);
            CHECK(gain <= previous + 1e-12 && gain > 0.0);
            previous = gain;
        }
    }
    CHECK(AmbienceGain(0.0, 0.0) == 0.0 && AmbienceGain(0.0, 1.0) == 0.0);
    CHECK(Close(AmbienceGain(0.7, -1.0), 0.7) && Close(AmbienceGain(0.7, 2.0), 0.28));   // clamped
    CHECK(Close(AmbienceCutoffHz(0.0), OpenAirCutoffHz) && Close(AmbienceCutoffHz(1.0), AmbienceIndoorCutoffHz));
    CHECK(AmbienceCutoffHz(0.5) < OpenAirCutoffHz && AmbienceCutoffHz(0.5) > AmbienceIndoorCutoffHz);

    // Hearth: louder than the old 0.2 in its room, silent without a line of sight, and a roofed hearth
    // heard from outside keeps only the leak; an open-air hearth isn't contained.
    CHECK(Close(HearthGainFor(1.0, 1.0, true), HearthGain) && HearthGain > 0.2 && HearthGain < 0.4);
    CHECK(Close(HearthGainFor(1.0, 0.0, true), HearthGain * HearthOutdoorLeak));
    CHECK(HearthGainFor(1.0, 0.0, true) < 0.2);                  // quieter outdoors than it ever was
    CHECK(HearthGainFor(0.0, 1.0, true) == 0.0 && HearthGainFor(0.0, 0.0, false) == 0.0);
    CHECK(Close(HearthGainFor(1.0, 0.0, false), HearthGain));
    CHECK(Close(HearthGainFor(0.5, 1.0, true), 0.5 * HearthGain));

    std::cout << "room audio: " << checks << " checks passed\n";
    return EXIT_SUCCESS;
}
