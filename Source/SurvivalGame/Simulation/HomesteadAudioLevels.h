#pragma once

// The loudness standard (docs/audio-checks.md, "Loudness standard"): every sound's category and in-game
// gain, measured against the forest ambience bed so a new cue is balanced from the start instead of by
// complaint. Jenny, 2026-09-30: "Is there any way we can get better at audio balancing sounds from the
// get-go by balancing it against the ambience sounds?"
//
// The playback code reads its gains from here. Scripts/Audio/Measure-Loudness.py parses the Cues table
// below, measures each source file and writes HomesteadAudioMeasurements.h; the native test
// HomesteadAudioLevelTests fails when a cue sits outside its category's band, or a sound file has no cue.
namespace Homestead
{
namespace AudioLevels
{
enum class Category : int
{
    AmbienceBed, // looping beds: the forest (the reference) and the creek
    Weather,     // the rain loop
    Hearth,      // the hearth's crackle, heard in its room
    Music,
    Footstep,
    Pickup,      // small success feedback: gathering, sowing, a sale at the counter
    UI,          // field book and menu clicks
    Door,        // reserved: no door sound yet
    ToolImpact,  // chops, the pickaxe ping, cane cuts, crafting strikes, a felled trunk landing
    Swish,       // the scythe's sweep
    Count
};

// Which mixer setting scales the cue (the player's Settings sliders; defaults below). Direct: none.
enum class Bus : int { Effects, Ambience, Music, Direct };

// Default slider levels (AHomesteadController::EffectsVolume, AmbienceVolume, MusicVolume).
constexpr double DefaultEffectsVolume = 0.8;
constexpr double DefaultAmbienceVolume = 0.7;
constexpr double DefaultMusicVolume = 0.65;

// Bands in LU over the reference: the forest bed's integrated loudness at its in-game gain (about
// -49 LUFS). Beds and music are judged by integrated loudness; one-shots by momentary max (400 ms),
// which is what a short hit sounds like against a steady bed. Cozy mix: steps and clicks sit at or under
// the birds, pickups just over, the creek, hearth and music around +10, tool hits clearly above, the
// scythe's swish a few LU under the chops it works between.
struct Band
{
    double minLu;
    double maxLu;
    bool integrated;
};
inline constexpr Band Bands[static_cast<int>(Category::Count)] = {
    {-3.0, 12.0, true},   // AmbienceBed
    {12.0, 22.0, true},   // Weather
    {4.0, 12.0, true},    // Hearth
    {6.0, 12.0, true},    // Music
    {-8.0, 5.0, false},   // Footstep
    {2.0, 10.0, false},   // Pickup
    {-10.0, 0.0, false},  // UI
    {6.0, 14.0, false},   // Door
    {11.0, 25.0, false},  // ToolImpact
    {14.0, 19.0, false},  // Swish
};
inline constexpr const char* CategoryNames[static_cast<int>(Category::Count)] = {
    "AmbienceBed", "Weather", "Hearth", "Music", "Footstep", "Pickup", "UI", "Door", "ToolImpact", "Swish"};

// Effect gains (times the Effects slider), named by use.
namespace Gain
{
constexpr float Default = 0.12f;         // PlayEffect's default: Notify's success cues (GrassStep, WoodTap)
constexpr float UIClick = 0.08f;         // opening a page, confirming
constexpr float UIClickSoft = 0.06f;     // moving a selection
constexpr float UIClickFaint = 0.05f;    // hotbar and shop cursor, cancelling
constexpr float FootstepWalk = 0.04f;    // bare feet, +/-15% per step
constexpr float FootstepRun = 0.07f;
constexpr float ShodStep = 0.12f;        // the non-MetaHuman heroine's grass step
constexpr float ShopSale = 0.16f;        // the counter's wood tap on a sale or purchase
constexpr float Ping = 0.12f;            // the pickaxe's stone ping (PlayStrikeCue)
constexpr float FinalPing = 0.15f;       // its breaking strike, +2 dB
constexpr float CraftStrike = 0.16f;     // the craft page's hammer beats
constexpr float CaneCut = 0.9f;          // the billhook (Jenny: almost too quiet at the chops' 0.75)
constexpr float Chop = 0.75f;            // hatchet and billhook-fallback chops on overgrowth
constexpr float FellChop = 0.8f;         // chops into a standing tree
constexpr float FellChopHeavy = 0.65f;   // ChopC, the loudest of the three, on a tree
constexpr float WoodTap = 0.6f;          // the chop fallback when the chop cues aren't imported
constexpr float FellTap = 0.55f;         // a felling stroke's fallback tap
constexpr float FinalTap = 0.12f;        // other tools' last strike: a small "cleared" tap
constexpr float TreeFall = 0.7f;         // the trunk landing
constexpr float ScytheSwish = 0.25f;     // Jenny: twice as loud as needed; -6 dB, then -4 dB more to the band
}

// Bed and loop gains (applied by their own code; listed so they are measured with everything else).
constexpr double CreekGain = 0.35;        // AHomesteadController::CreekGain, times the Ambience slider
constexpr double RainFullGain = 0.45;     // RainOutdoorGain x RainLoudness at full rain, times Ambience
constexpr double HearthCrackleGain = 0.32; // RoomAudio::HearthGain, inside the room

// Music: every track is matched to this integrated loudness at full slider (the per-track gain is
// 10^((MusicTargetLufs - loudness) / 20)), from each track's measured integrated loudness.
constexpr double MusicTargetLufs = -35.0;
struct MusicTrack
{
    const char* name;
    double loudnessLufs;
};
inline constexpr MusicTrack MusicTracks[] = {
    {"EveningHarp", -21.0}, {"WhispersOfTheGlen", -14.7}, {"MedievalTheme", -15.7}, {"ANewTown", -15.6}};

// One row per use of a sound. `source` is relative to Assets/ (tracked under Audio/, or fetched under
// Source/ by Fetch-Assets.ps1). Music rows carry gain 0: they play at MusicTargetLufs. Keep one row per
// line in this exact shape: Measure-Loudness.py reads it.
struct Cue
{
    const char* use;
    const char* source;
    Category category;
    Bus bus;
    double gain;
};
// clang-format off
inline constexpr Cue Cues[] = {
    {"Forest bed (reference)", "Source/forest-ambience/ForestAmbience.mp3", Category::AmbienceBed, Bus::Ambience, 1.0},
    {"Creek", "Audio/Ambience/CreekLoop.wav", Category::AmbienceBed, Bus::Ambience, CreekGain},
    {"Rain, full", "Audio/Ambience/RainLoop.wav", Category::Weather, Bus::Ambience, RainFullGain},
    {"Hearth crackle", "Audio/Ambience/HearthCrackle.wav", Category::Hearth, Bus::Direct, HearthCrackleGain},
    {"Music: EveningHarp", "Source/evening-harp/EveningHarp.mp3", Category::Music, Bus::Music, 0.0},
    {"Music: WhispersOfTheGlen", "Source/whispers-of-the-glen/WhispersOfTheGlen.mp3", Category::Music, Bus::Music, 0.0},
    {"Music: MedievalTheme", "Source/medieval-theme/MedievalTheme.mp3", Category::Music, Bus::Music, 0.0},
    {"Music: ANewTown", "Source/a-new-town/ANewTown.mp3", Category::Music, Bus::Music, 0.0},
    {"Bare step, walk", "Audio/Footsteps/BareStepWalk_00.wav", Category::Footstep, Bus::Effects, Gain::FootstepWalk},
    {"Bare step, walk", "Audio/Footsteps/BareStepWalk_01.wav", Category::Footstep, Bus::Effects, Gain::FootstepWalk},
    {"Bare step, walk", "Audio/Footsteps/BareStepWalk_02.wav", Category::Footstep, Bus::Effects, Gain::FootstepWalk},
    {"Bare step, walk", "Audio/Footsteps/BareStepWalk_03.wav", Category::Footstep, Bus::Effects, Gain::FootstepWalk},
    {"Bare step, walk", "Audio/Footsteps/BareStepWalk_04.wav", Category::Footstep, Bus::Effects, Gain::FootstepWalk},
    {"Bare step, walk", "Audio/Footsteps/BareStepWalk_05.wav", Category::Footstep, Bus::Effects, Gain::FootstepWalk},
    {"Bare step, run", "Audio/Footsteps/BareStepRun_00.wav", Category::Footstep, Bus::Effects, Gain::FootstepRun},
    {"Bare step, run", "Audio/Footsteps/BareStepRun_01.wav", Category::Footstep, Bus::Effects, Gain::FootstepRun},
    {"Bare step, run", "Audio/Footsteps/BareStepRun_02.wav", Category::Footstep, Bus::Effects, Gain::FootstepRun},
    {"Bare step, run", "Audio/Footsteps/BareStepRun_03.wav", Category::Footstep, Bus::Effects, Gain::FootstepRun},
    {"Shod grass step", "Source/kenney-impact/GrassStepA.ogg", Category::Footstep, Bus::Effects, Gain::ShodStep},
    {"Shod grass step", "Source/kenney-impact/GrassStepB.ogg", Category::Footstep, Bus::Effects, Gain::ShodStep},
    {"Gather, sow, till", "Source/kenney-impact/GrassStepA.ogg", Category::Pickup, Bus::Effects, Gain::Default},
    {"Gather, sow, till", "Source/kenney-impact/GrassStepB.ogg", Category::Pickup, Bus::Effects, Gain::Default},
    {"Pick up, clear by hand", "Source/kenney-impact/WoodTapA.ogg", Category::Pickup, Bus::Effects, Gain::Default},
    {"Pick up, clear by hand", "Source/kenney-impact/WoodTapB.ogg", Category::Pickup, Bus::Effects, Gain::Default},
    {"Overgrowth cleared tap", "Source/kenney-impact/WoodTapB.ogg", Category::Pickup, Bus::Effects, Gain::FinalTap},
    {"Shop sale", "Source/kenney-impact/WoodTapA.ogg", Category::Pickup, Bus::Effects, Gain::ShopSale},
    {"Shop purchase", "Source/kenney-impact/WoodTapB.ogg", Category::Pickup, Bus::Effects, Gain::ShopSale},
    {"UI click", "Source/kenney-interface/UIClick.ogg", Category::UI, Bus::Effects, Gain::UIClick},
    {"UI click, soft", "Source/kenney-interface/UIClick.ogg", Category::UI, Bus::Effects, Gain::UIClickSoft},
    {"UI click, faint", "Source/kenney-interface/UIClick.ogg", Category::UI, Bus::Effects, Gain::UIClickFaint},
    {"Pickaxe ping", "Source/kenney-impact/CraftStrikeA.ogg", Category::ToolImpact, Bus::Effects, Gain::Ping},
    {"Pickaxe ping", "Source/kenney-impact/CraftStrikeB.ogg", Category::ToolImpact, Bus::Effects, Gain::Ping},
    {"Pickaxe ping", "Source/kenney-impact/CraftStrikeC.ogg", Category::ToolImpact, Bus::Effects, Gain::Ping},
    {"Pickaxe final ping", "Source/kenney-impact/CraftStrikeA.ogg", Category::ToolImpact, Bus::Effects, Gain::FinalPing},
    {"Craft beat", "Source/kenney-impact/CraftStrikeA.ogg", Category::ToolImpact, Bus::Effects, Gain::CraftStrike},
    {"Craft beat", "Source/kenney-impact/CraftStrikeB.ogg", Category::ToolImpact, Bus::Effects, Gain::CraftStrike},
    {"Craft beat", "Source/kenney-impact/CraftStrikeC.ogg", Category::ToolImpact, Bus::Effects, Gain::CraftStrike},
    {"Billhook cane cut", "Audio/Effects/CaneCutA.wav", Category::ToolImpact, Bus::Effects, Gain::CaneCut},
    {"Billhook cane cut", "Audio/Effects/CaneCutB.wav", Category::ToolImpact, Bus::Effects, Gain::CaneCut},
    {"Billhook cane cut", "Audio/Effects/CaneCutC.wav", Category::ToolImpact, Bus::Effects, Gain::CaneCut},
    {"Chop", "Audio/Effects/ChopA.wav", Category::ToolImpact, Bus::Effects, Gain::Chop},
    {"Chop", "Audio/Effects/ChopB.wav", Category::ToolImpact, Bus::Effects, Gain::Chop},
    {"Chop", "Audio/Effects/ChopC.wav", Category::ToolImpact, Bus::Effects, Gain::Chop},
    {"Felling chop", "Audio/Effects/ChopA.wav", Category::ToolImpact, Bus::Effects, Gain::FellChop},
    {"Felling chop", "Audio/Effects/ChopB.wav", Category::ToolImpact, Bus::Effects, Gain::FellChop},
    {"Felling chop, heavy", "Audio/Effects/ChopC.wav", Category::ToolImpact, Bus::Effects, Gain::FellChopHeavy},
    {"Chop fallback tap", "Source/kenney-impact/WoodTapA.ogg", Category::ToolImpact, Bus::Effects, Gain::WoodTap},
    {"Felling fallback tap", "Source/kenney-impact/WoodTapB.ogg", Category::ToolImpact, Bus::Effects, Gain::FellTap},
    {"Tree falls", "Audio/Effects/TreeFall.wav", Category::ToolImpact, Bus::Effects, Gain::TreeFall},
    {"Scythe swish", "Audio/Effects/ScytheSwish.wav", Category::Swish, Bus::Effects, Gain::ScytheSwish},
};
// clang-format on
}
}
