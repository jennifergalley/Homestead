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

    // The moon never rises: one fixed altitude, and from the end of dusk to the start of dawn the moonlit
    // ground, the displayed ground key and the exposure floor are exactly level (Jenny, 2026-10-10).
    const double fixedAltitude = NightLightAt(0.0).moonAltitude;
    const NightLight deepNight = NightLightAt(0.0);
    for (int minute = 0; minute < 24 * 60; ++minute)
    {
        const NightLight light = NightLightAt(minute / 60.0);
        Check(std::abs(light.moonAltitude - fixedAltitude) < 1e-12, "moon moves", minute);
        Check(light.moonAltitude > 0.7 && light.moonAltitude < 0.8, "moon not high and fixed", light.moonAltitude);
        if (light.daylight < 1e-9)
        {
            Check(std::abs(light.moonGroundLux - deepNight.moonGroundLux) < 1e-9, "night moonlight not level", minute);
            Check(std::abs(NightGroundKey(light) - NightGroundKey(deepNight)) < 1e-9, "night key not level", minute);
            Check(std::abs(light.minExposureEV - deepNight.minExposureEV) < 1e-9, "exposure floor not level", minute);
        }
        // Dusk and dawn blend with the sun: the moon never brightens while the sun's daylight rises, and
        // is never above its full-night level.
        Check(light.moonGroundLux <= deepNight.moonGroundLux + 1e-9, "moon above night level", minute);
        Check(std::abs(light.moonGroundLux - deepNight.moonGroundLux * (1.0 - light.daylight)) < 1e-9, "moon not tied to night", minute);
    }
    // 18:00 is just past sunset and 6:00 is dawn: dusk and dawn are the same blend, mirrored.
    Check(NightLightAt(18.2).moonGroundLux > 0.0 && NightLightAt(18.2).moonGroundLux < deepNight.moonGroundLux, "dusk blend", NightLightAt(18.2).moonGroundLux);
    Check(NightLightAt(5.8).moonGroundLux > 0.0 && NightLightAt(5.8).moonGroundLux < deepNight.moonGroundLux, "dawn blend", NightLightAt(5.8).moonGroundLux);

    // Night reads as night but isn't black: moonlit ground about 1.5-3.5 stops under an adapted grey, at
    // dusk, midnight and pre-dawn alike.
    for (const double hour : {21.0, 0.0, 3.0, 4.5})
    {
        const NightLight light = NightLightAt(hour);
        const double key = NightGroundKey(light);
        Check(key < std::exp2(-1.5), "night ground as bright as day", key);
        Check(key > std::exp2(-3.5), "night ground pitch black", key);
        Check(light.moonGroundLux > 0.1 && light.moonGroundLux < 0.3, "full-moon ground lux", light.moonGroundLux);
    }

    // The moon never glares, and everything is continuous (no pops) minute to minute.
    double previousMoon = NightLightAt(0.0).moonLux;
    for (int minute = 1; minute <= 24 * 60; ++minute)
    {
        const NightLight light = NightLightAt(minute / 60.0);
        Check(light.moonLux <= 0.35, "moon glare", light.moonLux);
        Check(std::abs(light.moonLux - previousMoon) < 0.02, "moon pops", minute);
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

    // Jenny chose set A on 2026-09-30, comparing it with B, C and the old daylight-bright nights
    // (docs/night-ab/2026-09-30, fix-night-brightness 2.3). The defaults stay A: moon 0.2, sky 0.3, floor -1.
    Check(defaults.moonGroundLux == 0.2 && defaults.nightSky == 0.3 && defaults.nightMinExposureEV == -1.0,
          "defaults are Jenny's set A", defaults.moonGroundLux);

    // The console variables start at the tuned defaults (AHomesteadWorld reads them every refresh).
    {
        std::ifstream file(HOMESTEAD_SOURCE_DIR "/Source/SurvivalGame/HomesteadWorldLighting.cpp");
        std::stringstream text;
        text << file.rdbuf();
        const std::string source = text.str();
        Check(!source.empty(), "HomesteadWorldLighting.cpp readable");
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
    // grey by 21:00. The new schedule keeps 21:00 no brighter than any later night hour.
    NightLightTuning old;
    old.moonGroundLux = 2.0 * 0.72;
    old.nightSky = 0.6;
    old.nightMinExposureEV = -2.0;
    Check(NightGroundKey(NightLightAt(21.0, old)) > 0.99, "the old schedule adapted to daylight at 21:00");
    Check(NightGroundKey(NightLightAt(21.0)) <= NightGroundKey(NightLightAt(0.0)) + 1e-9, "21:00 brighter than midnight");

    std::printf("21:00 moon %.3f lux, ground %.3f lux, key %.3f (%.1f stops), floor EV %.2f, sky %.2f\n",
        NightLightAt(21.0).moonLux, NightLightAt(21.0).moonGroundLux, NightGroundKey(NightLightAt(21.0)),
        std::log2(NightGroundKey(NightLightAt(21.0))), NightLightAt(21.0).minExposureEV, NightLightAt(21.0).skyScale);
    std::printf(Failures ? "%d failure(s)\n" : "night light: all checks passed\n", Failures);
    return Failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
