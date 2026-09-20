"""Validate only the game's captured submix; no microphone or system-audio access."""
import argparse
import array
import json
import math
import sys
import wave
from pathlib import Path


def analyze(path):
    count = 0
    sum_squares = 0.0
    peak = 0
    clipped = 0
    with wave.open(str(path), "rb") as wav:
        if wav.getcomptype() != "NONE" or wav.getsampwidth() != 2:
            raise ValueError("Expected the engine's uncompressed 16-bit PCM submix export.")
        channels, rate, frames = wav.getnchannels(), wav.getframerate(), wav.getnframes()
        if channels < 1 or rate < 8000 or frames < rate * 10:
            raise ValueError("Game audio capture is absent, too short, or invalid.")
        while block := wav.readframes(32768):
            samples = array.array("h", block)
            if sys.byteorder != "little":
                samples.byteswap()
            for sample in samples:
                magnitude = abs(sample)
                peak = max(peak, magnitude)
                clipped += magnitude >= 32760
                sum_squares += sample * sample
            count += len(samples)
    rms = math.sqrt(sum_squares / count) / 32768
    result = {
        "source": "Game master submix only; no microphone/system audio",
        "channels": channels,
        "sample_rate": rate,
        "duration_seconds": frames / rate,
        "rms": rms,
        "peak": peak / 32768,
        "near_clipped_sample_fraction": clipped / count,
        "listening_review": "Not established by waveform analysis",
    }
    if rms < 0.0001:
        raise ValueError(f"Game submix is effectively silent: RMS={rms}")
    if clipped / count > 0.0001:
        raise ValueError(f"Game mix has excessive near-clipped samples: {clipped / count}")
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("file", type=Path)
    args = parser.parse_args()
    result = analyze(args.file)
    args.file.with_suffix(".json").write_text(json.dumps(result, indent=2), encoding="utf-8")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
