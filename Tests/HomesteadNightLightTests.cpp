// Native tests for the estate's night light schedule (Simulation/HomesteadNightLight).
#include "../Source/SurvivalGame/Simulation/HomesteadNightLight.h"

#include <cmath>
#include <initializer_list>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>

using namespace Homestead;

namespace
{
int Failures = 0;
void Check(bool ok, const char* what, double value = 0.0)
{
    if (!ok)
    {
        std::printf("FAIL: %s (%g)\n", what, value);
        ++Failures;
    }
}
}

int main()
{
    // Day is untouched: no moon, full sky, no exposure floor.
    const NightLight noon = NightLightAt(12.0);
    Check(noon.daylight > 0.999, "noon daylight", noon.daylight);
    Check(noon.moonLux == 0.0 && noon.moonGroundLux == 0.0, "no moon at noon", noon.moonLux);
    Check(std::abs(noon.skyScale - 1.0) < 1e-9, "noon sky", noon.skyScale);
    Check(noon.minExposureEV == 0.0, "noon exposure floor", noon.minExposureEV);

    // Moonrise (18:23-18:50) may firm the moonlit ground up by less than a stop; from 18:50 to midnight the
    // moonlit ground and the displayed ground key never rise (the old moon at a fixed 2 lux brightened
    // threefold between 18:30 and 21:00 as it climbed, and the camera adapted it to daylight).
    const double duskKey = NightGroundKey(NightLightAt(18.0 + 23.0 / 60.0));
    Check(NightGroundKey(NightLightAt(18.9)) < duskKey * 2.0, "moonrise brightens a stop or more", NightGroundKey(NightLightAt(18.9)));
    double lastGround = 1e9, lastKey = 1e9, lastExposure = 1e9;
    for (int minute = 18 * 60 + 50; minute <= 24 * 60; ++minute)
    {
        const NightLight light = NightLightAt(minute / 60.0);
        Check(light.moonGroundLux <= lastGround + 1e-9, "moonlit ground rises after dusk", minute);
        Check(NightGroundKey(light) <= lastKey + 1e-9, "night brightens after dusk", minute);
        Check(light.minExposureEV <= lastExposure + 1e-9, "exposure floor rises after dusk", minute);
        lastGround = light.moonGroundLux;
        lastKey = NightGroundKey(light);
        lastExposure = light.minExposureEV;
    }

    // Night reads as night but isn't black: moonlit ground about 1.5-3.5 stops under an adapted grey.
    for (const double hour : {21.0, 0.0, 3.0})
    {
        const NightLight light = NightLightAt(hour);
        const double key = NightGroundKey(light);
        Check(key < std::exp2(-1.5), "night ground as bright as day", key);
        Check(key > std::exp2(-3.5), "night ground pitch black", key);
        Check(light.moonGroundLux > 0.1 && light.moonGroundLux < 0.3, "full-moon ground lux", light.moonGroundLux);
    }

    // The moon never glares at the horizon, and everything is continuous (no pops) minute to minute.
    double previousMoon = NightLightAt(0.0).moonLux;
    for (int minute = 1; minute <= 24 * 60; ++minute)
    {
        const NightLight light = NightLightAt(minute / 60.0);
        Check(light.moonLux <= 1.01, "moon glare", light.moonLux);
        // A game minute is about a second at the default day length, so the moon's 5-minute rise over
        // the horizon (at most 0.25 lux a minute) is a fade of several seconds, not a pop.
        Check(std::abs(light.moonLux - previousMoon) < 0.25, "moon pops", minute);
        previousMoon = light.moonLux;
    }

    // The sky light: full at noon, the night level at night, and a smooth blend through dusk.
    const NightLightTuning defaults;
    Check(std::abs(NightLightAt(0.0).skyScale - defaults.nightSky) < 1e-9, "night sky at midnight", NightLightAt(0.0).skyScale);
    Check(std::abs(NightLightAt(21.0).skyScale - defaults.nightSky) < 1e-9, "night sky at 21:00", NightLightAt(21.0).skyScale);
    double lastSky = 2.0;
    for (int minute = 12 * 60; minute <= 21 * 60; ++minute)
    {
        const double sky = NightLightAt(minute / 60.0).skyScale;
        Check(sky <= lastSky + 1e-9 && sky >= defaults.nightSky - 1e-9 && sky <= 1.0 + 1e-9, "sky blend through dusk", minute);
        Check(lastSky > 1.5 || lastSky - sky < 0.02, "sky pops at dusk", minute);
        lastSky = sky;
    }

    // The console variables start at the tuned defaults (AHomesteadWorld reads them every refresh).
    {
        std::ifstream file(HOMESTEAD_SOURCE_DIR "/Source/SurvivalGame/HomesteadWorld.cpp");
        std::stringstream text;
        text << file.rdbuf();
        const std::string source = text.str();
        Check(!source.empty(), "HomesteadWorld.cpp readable");
        auto cvarDefault = [&](const char* name) {
            const std::regex pattern(std::string("TEXT\\(\"") + name + "\"\\),\\s*(-?[0-9.]+)f");
            std::smatch match;
            return std::regex_search(source, match, pattern) ? std::stod(match[1].str()) : 1e9;
        };
        Check(std::abs(cvarDefault("homestead.NightMoonLux") - defaults.moonGroundLux) < 1e-6, "NightMoonLux default", cvarDefault("homestead.NightMoonLux"));
        Check(std::abs(cvarDefault("homestead.NightSky") - defaults.nightSky) < 1e-6, "NightSky default", cvarDefault("homestead.NightSky"));
        Check(std::abs(cvarDefault("homestead.NightMinExposure") - defaults.nightMinExposureEV) < 1e-6, "NightMinExposure default", cvarDefault("homestead.NightMinExposure"));
        Check(source.find("Homestead::NightLightAt(Hour, NightTuning)") != std::string::npos, "UpdateLighting uses NightLightAt");
    }

    // The old schedule's failure, reproduced: fixed 2 lux, EV -2 floor, and the scene adapts to a daylight
    // grey by 21:00. The new schedule keeps 21:00 darker than 18:45.
    const NightLightTuning old{2.0 * 0.72, 0.6, -2.0, 1.0};
    Check(NightGroundKey(NightLightAt(21.0, old)) > 0.99, "the old schedule adapted to daylight at 21:00");
    Check(NightGroundKey(NightLightAt(21.0)) <= NightGroundKey(NightLightAt(18.75)) + 1e-9, "21:00 no brighter than 18:45");

    std::printf("21:00 moon %.3f lux, ground %.3f lux, key %.3f (%.1f stops), floor EV %.2f, sky %.2f\n",
        NightLightAt(21.0).moonLux, NightLightAt(21.0).moonGroundLux, NightGroundKey(NightLightAt(21.0)),
        std::log2(NightGroundKey(NightLightAt(21.0))), NightLightAt(21.0).minExposureEV, NightLightAt(21.0).skyScale);
    std::printf(Failures ? "%d failure(s)\n" : "night light: all checks passed\n", Failures);
    return Failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
