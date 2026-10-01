"""Measure every game sound against the loudness standard and flag anything out of its band.

Usage (from the repo root; needs the fetched sources, Scripts/Fetch-Assets.ps1):
    python Scripts/Audio/Measure-Loudness.py            # measure, report, rewrite the generated header and docs table
    python Scripts/Audio/Measure-Loudness.py --check    # measure and report only; exit 1 on any problem

Reads the cue table, gains, bus defaults and bands from Source/SurvivalGame/Simulation/HomesteadAudioLevels.h,
measures each source file (EBU R128 K-weighting: integrated, momentary max over 400 ms, short-term max over 3 s),
applies the in-game gain (cue gain x default slider), and compares it with the reference: the forest bed's
integrated loudness at its in-game gain. Beds and music are judged by integrated loudness, one-shots by
momentary max. Writes Source/SurvivalGame/Simulation/HomesteadAudioMeasurements.h (read by the native
test HomesteadAudioLevelTests) and the table between the loudness markers in docs/audio-checks.md.

Exit 1 when a cue is out of band, a sound file under Assets/Audio has no cue (no category), or a source is missing.
Needs: pip install soundfile pyloudnorm numpy
"""
import argparse
import math
import pathlib
import re
import sys

import numpy as np
import pyloudnorm
import soundfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
ASSETS = ROOT / "Assets"
LEVELS = ROOT / "Source" / "SurvivalGame" / "Simulation" / "HomesteadAudioLevels.h"
GENERATED = ROOT / "Source" / "SurvivalGame" / "Simulation" / "HomesteadAudioMeasurements.h"
DOCS = ROOT / "docs" / "audio-checks.md"
DOC_START = "<!-- loudness-table:start -->"
DOC_END = "<!-- loudness-table:end -->"
MOMENTARY_SECONDS = 0.4
SHORT_TERM_SECONDS = 3.0
HOP_SECONDS = 0.01
SILENT_LUFS = -70.0  # below the R128 absolute gate: written for clips too short to integrate
REFERENCE_USE = "Forest bed (reference)"
BUSES = {"Effects": "DefaultEffectsVolume", "Ambience": "DefaultAmbienceVolume", "Music": "DefaultMusicVolume"}


def parse_levels(text):
    """The header's constants, bands and cue rows."""
    constants = {}
    for name, value in re.findall(r"constexpr\s+(?:float|double)\s+(\w+)\s*=\s*(-?[\d.]+)f?\s*;", text):
        constants.setdefault(name, float(value))
    gains = dict(re.findall(r"constexpr\s+float\s+(\w+)\s*=\s*(-?[\d.]+)f\s*;",
                            text[text.index("namespace Gain"):]))
    gains = {name: float(value) for name, value in gains.items()}
    names = text[text.index("CategoryNames["):]
    categories = re.findall(r'"(\w+)"', names[:names.index("};")])
    bands_text = text[text.index("Bands[static_cast<int>(Category::Count)]"):]
    bands = [(float(lo), float(hi), flag == "true") for lo, hi, flag in
             re.findall(r"\{\s*(-?[\d.]+),\s*(-?[\d.]+),\s*(true|false)\}", bands_text)[:len(categories)]]
    cues = []
    for use, source, category, bus, gain in re.findall(
            r'\{"([^"]+)",\s*"([^"]+)",\s*Category::(\w+),\s*Bus::(\w+),\s*([^}]+?)\}', text):
        gain = gain.strip()
        if gain.startswith("Gain::"):
            value = gains[gain[len("Gain::"):]]
        elif gain in constants:
            value = constants[gain]
        else:
            value = float(gain)
        cues.append({"use": use, "source": source, "category": category, "bus": bus, "gain": value})
    music = {name: float(value) for name, value in
             re.findall(r'\{"(\w+)",\s*(-?[\d.]+)\}', text[text.index("MusicTracks[]"):text.index("};", text.index("MusicTracks[]"))])}
    for cue in cues:
        if cue["category"] == "Music":
            cue["stated"] = music.get(pathlib.Path(cue["source"]).stem)
    return constants, dict(zip(categories, bands)), cues


def k_weighted_power(path):
    data, rate = soundfile.read(str(path), always_2d=True)
    meter = pyloudnorm.Meter(rate)
    weighted = data.copy()
    for stage in meter._filters.values():  # the R128 pre-filter and RLB high-pass, per channel
        weighted = np.stack([stage.apply_filter(weighted[:, c]) for c in range(weighted.shape[1])], 1)
    return data, rate, meter, np.sum(weighted ** 2, axis=1)


def window_max(power, rate, seconds):
    width = int(seconds * rate)
    if len(power) <= width:
        return -0.691 + 10 * math.log10(power.sum() / width + 1e-20)
    sums = np.concatenate([[0.0], np.cumsum(power)])
    hop = max(1, int(HOP_SECONDS * rate))
    best = max(sums[i + width] - sums[i] for i in range(0, len(power) - width + 1, hop)) / width
    return -0.691 + 10 * math.log10(best + 1e-20)


def measure(path):
    data, rate, meter, power = k_weighted_power(path)
    integrated = SILENT_LUFS
    if len(data) / rate >= MOMENTARY_SECONDS:
        value = meter.integrated_loudness(data)
        if math.isfinite(value):
            integrated = value
    return {"integrated": integrated, "momentary": window_max(power, rate, MOMENTARY_SECONDS),
            "short_term": window_max(power, rate, SHORT_TERM_SECONDS)}


def effective(cue, levels, constants, music_target):
    """The cue's level at its in-game gain: integrated for beds and music, momentary max for one-shots."""
    measured = levels[cue["source"]]
    bus = constants[BUSES[cue["bus"]]] if cue["bus"] in BUSES else 1.0
    if cue["category"] == "Music":
        # Measured, moved by the gain the game applies from the stated loudness, so a wrong statement shows.
        return measured["integrated"] + (music_target - cue["stated"]) + 20 * math.log10(bus)
    integrated_band = cue["integrated"]
    base = measured["integrated"] if integrated_band else measured["momentary"]
    return base + 20 * math.log10(cue["gain"] * bus)


def write_generated(levels):
    lines = ["#pragma once", "",
             "// Generated by Scripts/Audio/Measure-Loudness.py from the source files; do not edit by hand.",
             "// LUFS at unity gain, EBU R128 K-weighted: integrated (-70 when too short to gate), momentary max",
             "// (400 ms) and short-term max (3 s). Read by Tests/HomesteadAudioLevelTests.cpp.",
             "namespace Homestead", "{", "namespace AudioLevels", "{", "struct Measurement", "{",
             "    const char* source;", "    double integratedLufs;", "    double momentaryMaxLufs;",
             "    double shortTermMaxLufs;", "};", "inline constexpr Measurement Measurements[] = {"]
    for source in sorted(levels):
        m = levels[source]
        lines.append(f'    {{"{source}", {m["integrated"]:.1f}, {m["momentary"]:.1f}, {m["short_term"]:.1f}}},')
    lines += ["};", "}", "}", ""]
    GENERATED.write_text("\n".join(lines), encoding="utf-8", newline="\r\n")


def write_docs(rows):
    text = DOCS.read_text(encoding="utf-8")
    table = ["| Use | Source | Category | Gain | Level (LUFS) | Over the forest bed (LU) | Band (LU) |",
             "| --- | --- | --- | --- | --- | --- | --- |"]
    for row in rows:
        table.append(f'| {row["use"]} | `{row["source"].split("/")[-1]}` | {row["category"]} | {row["gain"]} | '
                     f'{row["level"]:.1f} | {row["relative"]:+.1f}{"" if row["ok"] else " **out**"} | '
                     f'{row["band"][0]:+.0f} to {row["band"][1]:+.0f} |')
    block = DOC_START + "\n" + "\n".join(table) + "\n" + DOC_END
    if DOC_START in text and DOC_END in text:
        text = text[:text.index(DOC_START)] + block + text[text.index(DOC_END) + len(DOC_END):]
    else:
        text = text.rstrip() + "\n\n" + block + "\n"
    DOCS.write_text(text, encoding="utf-8", newline="\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="report only; don't rewrite the header or docs")
    args = parser.parse_args()

    constants, bands, cues = parse_levels(LEVELS.read_text(encoding="utf-8"))
    problems = []
    sources = {cue["source"] for cue in cues}
    tracked = {p.relative_to(ASSETS).as_posix() for p in (ASSETS / "Audio").rglob("*")
               if p.suffix.lower() in (".wav", ".ogg", ".mp3")}
    for orphan in sorted(tracked - sources):
        problems.append(f"{orphan} has no cue row (no category) in HomesteadAudioLevels.h")
    levels = {}
    for source in sorted(sources):
        path = ASSETS / source
        if not path.is_file():
            problems.append(f"{source} is missing" + (" (run Scripts/Fetch-Assets.ps1)" if source.startswith("Source/") else ""))
            continue
        levels[source] = measure(path)
    if problems and len(levels) < len(sources):
        print("\n".join(problems))
        return 1

    for cue in cues:
        lo, hi, integrated = bands[cue["category"]]
        cue["integrated"] = integrated
    reference_cue = next(cue for cue in cues if cue["use"] == REFERENCE_USE)
    music_target = constants["MusicTargetLufs"]
    reference = effective(reference_cue, levels, constants, music_target)
    rows = []
    for cue in cues:
        if cue["category"] == "Music":
            if cue.get("stated") is None:
                problems.append(f'{cue["source"]} has no MusicTracks loudness')
                continue
            if abs(levels[cue["source"]]["integrated"] - cue["stated"]) > 0.5:
                problems.append(f'{cue["source"]} measures {levels[cue["source"]]["integrated"]:.1f} LUFS but MusicTracks '
                                f'states {cue["stated"]:.1f}: set it to the measurement')
        lo, hi, _ = bands[cue["category"]]
        level = effective(cue, levels, constants, music_target)
        relative = level - reference
        ok = lo - 0.05 <= relative <= hi + 0.05
        if not ok:
            problems.append(f'{cue["use"]} ({cue["source"]}) is {relative:+.1f} LU; {cue["category"]} band is {lo:+.0f} to {hi:+.0f}')
        rows.append({**cue, "level": level, "relative": relative, "band": (lo, hi), "ok": ok})

    print(f"Reference: forest bed at in-game gain, {reference:.1f} LUFS integrated")
    for row in rows:
        flag = "" if row["ok"] else "  <-- OUT OF BAND"
        print(f'{row["use"]:28s} {row["source"].split("/")[-1]:24s} {row["category"]:11s} gain {row["gain"]:<5g} '
              f'{row["level"]:6.1f} LUFS  {row["relative"]:+5.1f} LU  band {row["band"][0]:+.0f}..{row["band"][1]:+.0f}{flag}')
    if not args.check:
        write_generated(levels)
        write_docs(rows)
    if problems:
        print("\n".join(["", "Problems:"] + problems))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
