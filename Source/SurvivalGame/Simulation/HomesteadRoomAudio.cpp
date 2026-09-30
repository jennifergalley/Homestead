#include "HomesteadRoomAudio.h"

#include <algorithm>

namespace Homestead
{
namespace RoomAudio
{
namespace
{
double Inside(double indoors) { return std::clamp(indoors, 0.0, 1.0); }
double Mix(double outdoor, double indoor, double indoors) { return outdoor + (indoor - outdoor) * Inside(indoors); }
}

double AmbienceGain(double ambience, double indoors)
{
    return std::clamp(ambience, 0.0, 1.0) * Mix(1.0, AmbienceIndoorGain, indoors);
}

double AmbienceCutoffHz(double indoors) { return Mix(OpenAirCutoffHz, AmbienceIndoorCutoffHz, indoors); }

double HearthGainFor(double gate, double indoors, bool roofed)
{
    const double contained = roofed ? Mix(HearthOutdoorLeak, 1.0, indoors) : 1.0;
    return HearthGain * std::clamp(gate, 0.0, 1.0) * contained;
}
}
}
