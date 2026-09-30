#pragma once

// The estate's night light schedule (presentation tuning kept in plain C++ so native tests can pin its
// shape). AHomesteadWorld::UpdateLighting feeds these to the moon, the sky light and auto-exposure.
//
// Jenny, 2026-09-29: "at 9 PM the scene brightens and the moon lights like the sun". The old schedule
// held the moon at a fixed 2 lux while it climbed from the horizon at dusk to 46 degrees by 21:00, so
// the moonlit ground brightened threefold after dark; with auto-exposure allowed down to EV100 -2 the
// camera adapted to it like daylight. This one keeps the moonlit ground level from dusk to dawn and far
// enough below the exposure floor that night stays night, with lamps and the hearth reading bright.

namespace Homestead
{
struct NightLightTuning
{
    // Moonlit ground (lux on level ground) at full night: a bright full moon gives about 0.1-0.3 lux.
    double moonGroundLux = 0.2;
    // Sky light (the real-time sky capture) at full night, a multiplier on its day intensity.
    double nightSky = 0.3;
    // Auto-exposure min brightness at full night (EV100). Higher keeps night darker: with the moonlit
    // ground well below it, the camera can't adapt the moon up to a daylight grey.
    double nightMinExposureEV = -1.0;
    // Below this altitude (sine, about 11.5 degrees) the moon's intensity stops rising to compensate, so a
    // low moon at dusk rakes walls and faces at no more than moonGroundLux / 0.2 = 1 lux. The price: the
    // moonlit ground firms up from about 0.11 to 0.2 lux as the moon clears 11.5 degrees (18:23-18:50).
    double moonCompensationFloor = 0.2;
};

struct NightLight
{
    double sunElevation = 0.0;   // sine of the sun's altitude
    double daylight = 0.0;       // 0 night .. 1 day
    double moonAltitude = 0.0;   // sine of the moon's altitude (negative below the horizon)
    double moonLux = 0.0;        // the moon's directional light intensity
    double moonGroundLux = 0.0;  // what that gives on level ground: moonLux * max(altitude, 0)
    double skyScale = 1.0;       // sky light multiplier (before any overcast scaling)
    double minExposureEV = 0.0;  // auto-exposure min brightness (EV100)
};

// The sun and moon follow the same day circle the world draws (sun pitch -elevation * 65 degrees, moon
// opposite), so the moon is up exactly when the sun is down.
NightLight NightLightAt(double hour, const NightLightTuning& tuning = {});

// Rough displayed brightness of moonlit level ground relative to an adapted daylight grey (1), for
// tests: auto-exposure adapts to the scene (1) until it hits the night floor, then the image darkens.
double NightGroundKey(const NightLight& light);
}
