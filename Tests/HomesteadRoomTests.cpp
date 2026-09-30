// Portable tests for the standing room's sound and door (HomesteadRoomAudio, HomesteadDoor).
#include "HomesteadDoor.h"
#include "HomesteadEstate.h"
#include "HomesteadManor.h"
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
double Dot(Point a, Point b) { return a.x * b.x + a.y * b.y; }
Point Minus(Point a, Point b) { return {a.x - b.x, a.y - b.y}; }
double Length(Point a) { return std::hypot(a.x, a.y); }
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

    // The standing room's door: one heritage stone doorway, on the room's west side, opening west.
    Simulation sim;
    const Result started = sim.NewEstateGame(ProvisionalEstateLayout(), ProvisionalEstatePlacements());
    CHECK(started.ok);
    const State& state = sim.GetState();
    const Structure* doorway = nullptr;
    int leaves = 0;
    for (const Structure& piece : state.structures)
        if (Door::HasLeaf(piece))
        {
            ++leaves;
            doorway = &piece;
        }
    CHECK(leaves == 1 && doorway != nullptr);
    const Point room = ProvisionalEstateLayout().PointOr(Anchor::StandingRoomOrigin, {});
    const Door::Leaf leaf = Door::LeafFor(state, *doorway);
    CHECK(Close(Length(leaf.outward), 1.0, 1e-9));
    CHECK(leaf.outward.y < -0.99);                               // +Y is east: the door opens west
    const double wallOut = Dot(Minus(leaf.opening, room), leaf.outward);
    CHECK(wallOut > 290.0 && wallOut < 320.0);                   // the room is 6 m across; outer face 3.08 m out
    CHECK(Close(Length(Minus(leaf.hinge, leaf.opening)), std::hypot(Door::OpeningHalfWidthCm, 3.0), 1e-6));
    // Shut, the latch edge closes the opening's far side, inside the reveal.
    const Point shut = Door::LatchEdge(leaf, 0.0);
    CHECK(std::abs(Dot(Minus(shut, leaf.opening), leaf.outward)) < 6.0);
    CHECK(Close(Length(Minus(shut, leaf.hinge)), std::hypot(Door::LeafWidthCm, Door::LeafThicknessCm * 0.5), 1e-6));
    // Open, it swings out of the room (never in over the bed or hearth) and clears the opening.
    const double shutDepth = Dot(Minus(shut, leaf.opening), leaf.outward);
    for (double openness = 0.05; openness <= 1.0; openness += 0.05)
    {
        const Point edge = Door::LatchEdge(leaf, Door::Angle(openness));
        CHECK(Dot(Minus(edge, leaf.opening), leaf.outward) > shutDepth);
    }
    const Point open = Door::LatchEdge(leaf, Door::Angle(1.0));
    CHECK(Dot(Minus(open, leaf.opening), leaf.outward) > 120.0);
    CHECK(Close(Door::LeafYaw(leaf, Door::Angle(1.0)) - leaf.closedYaw, Door::MaxOpenDeg));

    // Near: opens within 2.2 m, stays open until she's 3 m away (no chatter on the threshold).
    CHECK(Door::WantsOpen(false, 150.0) && !Door::WantsOpen(false, 250.0));
    CHECK(Door::WantsOpen(true, 250.0) && !Door::WantsOpen(true, 320.0));
    // Swing: fully open in 0.9 s, shut in 1.4 s, eased at both ends, and never past either stop.
    double openness = 0.0;
    for (int frame = 0; frame < 54; ++frame) openness = Door::Step(openness, true, 1.0 / 60.0);
    CHECK(Close(openness, 1.0, 1e-9));
    openness = Door::Step(openness, true, 1.0);
    CHECK(openness == 1.0);
    for (int frame = 0; frame < 60; ++frame) openness = Door::Step(openness, false, 1.0 / 60.0);
    CHECK(openness > 0.0 && openness < 0.4);
    for (int frame = 0; frame < 30; ++frame) openness = Door::Step(openness, false, 1.0 / 60.0);
    CHECK(openness == 0.0);
    CHECK(Door::Angle(0.0) == 0.0 && Close(Door::Angle(1.0), Door::MaxOpenDeg) && Close(Door::Angle(0.5), 50.0));
    CHECK(Door::Angle(0.05) < 0.05 * Door::MaxOpenDeg);          // eases off the stop
    // Nothing about the door is saved: a save round trip leaves the structures exactly as they were.
    Simulation loaded;
    loaded.SetLayout(ProvisionalEstateLayout());
    loaded.SetPlacements(ProvisionalEstatePlacements());
    CHECK(loaded.Deserialize(sim.Serialize()).ok);
    CHECK(loaded.Serialize() == sim.Serialize());

    std::cout << "room audio and door: " << checks << " checks passed\n";
    return EXIT_SUCCESS;
}
