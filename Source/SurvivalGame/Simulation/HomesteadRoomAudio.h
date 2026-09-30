#pragma once

// How the outdoors and the hearth sound from inside a room (Jenny, 2026-09-29: the hearth is too quiet,
// and the birds and creek play at full level indoors). Pure mixes the Unreal audio applies each frame;
// `indoors` is UHomesteadWeather's eased roof/shelter check at the camera, 0 outdoors .. 1 indoors,
// the same one the rain uses.
namespace Homestead
{
namespace RoomAudio
{
// The woodland loop and the creek through stone walls: about -8 dB and dulled, not gone.
constexpr double AmbienceIndoorGain = 0.4;
constexpr double OpenAirCutoffHz = 20000.0;
constexpr double AmbienceIndoorCutoffHz = 1800.0;
// The hearth's crackle: 0.2 was lost under the room tone; 0.32 is about +4 dB.
constexpr double HearthGain = 0.32;
// A roofed hearth heard from outside its room (through an open door) keeps only this share, so the
// fire stays in its room rather than carrying into the ruin's range.
constexpr double HearthOutdoorLeak = 0.3;

// The ambience loop's volume multiplier: the Ambience setting, ducked indoors.
double AmbienceGain(double ambience, double indoors);
// Low-pass cutoff for the ambience and creek, from open air to muffled indoors.
double AmbienceCutoffHz(double indoors);
// The crackle's volume multiplier. gate: 0..1, the eased line of sight from the listener to the fire;
// roofed: the hearth stands under a roof (its room), so it is contained when the listener is outdoors.
double HearthGainFor(double gate, double indoors, bool roofed);
}
}
