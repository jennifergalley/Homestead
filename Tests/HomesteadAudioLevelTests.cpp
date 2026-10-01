// The loudness standard's gate (docs/audio-checks.md, "Loudness standard"): every cue sits inside its
// category's band against the forest bed, every cue has a measurement, every sound file under Assets/Audio
// has a cue (a category), and the gains the gameplay code keeps elsewhere match the catalogue.
// Re-measure with Scripts/Audio/Measure-Loudness.py after adding or changing a sound.
#include "HomesteadAudioLevels.h"
#include "HomesteadAudioMeasurements.h"
#include "HomesteadRoomAudio.h"
#include "HomesteadSimulation.h"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <set>
#include <string>

namespace
{
using namespace Homestead::AudioLevels;

int Failures = 0;
void Check(bool condition, const std::string& message)
{
    if (!condition)
    {
        std::cerr << "FAIL " << message << '\n';
        ++Failures;
    }
}

const Measurement* Find(const char* source)
{
    for (const Measurement& measured : Measurements)
        if (std::strcmp(measured.source, source) == 0) return &measured;
    return nullptr;
}

double BusVolume(Bus bus)
{
    switch (bus)
    {
    case Bus::Effects: return DefaultEffectsVolume;
    case Bus::Ambience: return DefaultAmbienceVolume;
    case Bus::Music: return DefaultMusicVolume;
    default: return 1.0;
    }
}

// The cue's level at its in-game gain, as Measure-Loudness.py computes it.
double Effective(const Cue& cue, const Measurement& measured)
{
    const Band& band = Bands[static_cast<int>(cue.category)];
    if (cue.category == Category::Music) return MusicTargetLufs + 20.0 * std::log10(BusVolume(cue.bus));
    const double base = band.integrated ? measured.integratedLufs : measured.momentaryMaxLufs;
    return base + 20.0 * std::log10(cue.gain * BusVolume(cue.bus));
}
}

int main()
{
    const Cue& reference = Cues[0];
    Check(std::strcmp(reference.use, "Forest bed (reference)") == 0, "the first cue row must be the forest bed reference");
    const Measurement* referenceMeasured = Find(reference.source);
    Check(referenceMeasured != nullptr, "the forest bed has no measurement");
    if (!referenceMeasured) return 1;
    const double referenceLufs = Effective(reference, *referenceMeasured);

    std::set<std::string> used;
    for (const Cue& cue : Cues)
    {
        used.insert(cue.source);
        const std::string name = std::string(cue.use) + " (" + cue.source + ")";
        Check(static_cast<int>(cue.category) >= 0 && cue.category < Category::Count, name + " has no category");
        Check(cue.category == Category::Music ? cue.gain == 0.0 : cue.gain > 0.0 && cue.gain <= 1.0,
            name + " has an invalid gain");
        const Measurement* measured = Find(cue.source);
        Check(measured != nullptr, name + " has no measurement: run Scripts/Audio/Measure-Loudness.py");
        if (!measured) continue;
        const Band& band = Bands[static_cast<int>(cue.category)];
        const double relative = Effective(cue, *measured) - referenceLufs;
        Check(relative >= band.minLu - 0.05 && relative <= band.maxLu + 0.05,
            name + " is " + std::to_string(relative) + " LU over the forest bed; its " + CategoryNames[static_cast<int>(cue.category)]
                + " band is " + std::to_string(band.minLu) + " to " + std::to_string(band.maxLu));
    }
    for (const Measurement& measured : Measurements)
        Check(used.count(measured.source) == 1, std::string(measured.source) + " is measured but no cue uses it");

    // Every sound file the game ships from Assets/Audio has a cue row, so it has a category and a band.
    const std::filesystem::path audio = std::filesystem::path(HOMESTEAD_REPO_ROOT) / "Assets" / "Audio";
    Check(std::filesystem::is_directory(audio), "Assets/Audio not found at " + audio.string());
    if (std::filesystem::is_directory(audio))
        for (const auto& entry : std::filesystem::recursive_directory_iterator(audio))
        {
            if (!entry.is_regular_file()) continue;
            const std::string extension = entry.path().extension().string();
            if (extension != ".wav" && extension != ".ogg" && extension != ".mp3") continue;
            const std::string relative = "Audio/" + std::filesystem::relative(entry.path(), audio).generic_string();
            Check(used.count(relative) == 1, relative + " has no cue row (no category) in HomesteadAudioLevels.h");
        }

    // Each music track's stated loudness (which sets its gain) matches its measurement within 2 LU.
    for (const MusicTrack& track : MusicTracks)
    {
        bool found = false;
        for (const Cue& cue : Cues)
            if (cue.category == Category::Music && std::strstr(cue.source, (std::string("/") + track.name + ".").c_str()))
                if (const Measurement* measured = Find(cue.source))
                {
                    found = true;
                    Check(std::abs(measured->integratedLufs - track.loudnessLufs) <= 2.0,
                        std::string(track.name) + "'s stated loudness is off its measurement by more than 2 LU");
                }
        Check(found, std::string(track.name) + " has no music cue row");
    }

    // The bed gains the gameplay code keeps elsewhere are the ones measured here.
    Check(std::abs(Homestead::RoomAudio::HearthGain - HearthCrackleGain) < 1e-9, "RoomAudio::HearthGain changed: update the catalogue");
    Check(std::abs(Homestead::RainOutdoorGain * Homestead::RainLoudness - RainFullGain) < 1e-9,
        "RainOutdoorGain x RainLoudness changed: update RainFullGain");

    if (Failures) std::cerr << Failures << " loudness check(s) failed\n";
    else std::cout << "PASS " << (sizeof(Cues) / sizeof(Cues[0])) << " cues in band against the forest bed (" << referenceLufs << " LUFS)\n";
    return Failures ? 1 : 0;
}
