"""Generate the scythe's mowing cue: an airy steel-through-stems "shhhhnk" (original, no third-party audio).

    python Scripts/generate_scythe_sound.py

Writes Assets/Audio/Effects/ScytheSwish.wav (48 kHz, 16-bit mono), peaking at -6 dBFS. The output is
deterministic (fixed seed), so the WAV can be regenerated and compared.

What a scythe sounds like going through standing grass: the long, thin blade moves fast and nearly
flat, so there is no impact. There's a breathy swish of air along the blade that rises as it
accelerates into the swath, a dense run of tiny snaps as it shears dozens of stems in about a
quarter of a second (each a click of a millisecond or two, mostly 2-7 kHz), and the soft rustle of
the cut grass falling over. The "nk" at the end is the thin steel flexing as the stroke stops: a
faint, fast-decaying ring of inharmonic partials, kept well below the swish so it never reads as a
clang. The cue starts at blade contact (AHomesteadController::LandOvergrowthSwing), so the air
lead-in is only a few tens of milliseconds.
"""
import pathlib
import wave

import numpy as np

SR = 48000
SECONDS = 0.72
SEED = 7307
PEAK_DBFS = -6.0
OUT = pathlib.Path(__file__).resolve().parent.parent / 'Assets' / 'Audio' / 'Effects' / 'ScytheSwish.wav'


def smooth_step(t, start, end):
    x = np.clip((t - start) / (end - start), 0, 1)
    return x * x * (3 - 2 * x)


def swept_band_noise(rng, t, lo_start, lo_end, width_octaves):
    """White noise through a band whose centre glides from lo_start to lo_end (Hz) over the sound."""
    n = len(t)
    out = np.zeros(n)
    block = 1024
    hop = block // 2
    window = np.hanning(block)
    noise = rng.standard_normal(n + block)
    f = np.fft.rfftfreq(block, 1 / SR)
    for start in range(0, n, hop):
        centre = lo_start * (lo_end / lo_start) ** min(1.0, start / n)
        octave = np.log2(np.maximum(f, 1) / centre)
        gain = np.exp(-0.5 * (octave / (width_octaves / 2)) ** 2)
        frame = np.fft.irfft(np.fft.rfft(noise[start:start + block] * window) * gain, block)
        end = min(n, start + block)
        out[start:end] += frame[:end - start]
    return out / (np.abs(out).max() + 1e-9)


def stem_snaps(rng, t, start, end, rate):
    """Tiny sharp clicks, one per stem the blade shears, densest in the middle of the stroke."""
    n = len(t)
    clicks = np.zeros(n)
    density = np.sin(np.pi * np.clip((t - start) / (end - start), 0, 1)) ** 1.5
    for i in np.nonzero(rng.random(n) < density * rate / SR)[0]:
        length = int(rng.uniform(0.0006, 0.0022) * SR)
        tone = rng.uniform(2200, 7000)
        s = np.arange(min(length, n - i)) / SR
        clicks[i:i + len(s)] += rng.uniform(0.3, 1.0) * np.sin(2 * np.pi * tone * s) * np.exp(-s / (length / SR / 3))
    return clicks / (np.abs(clicks).max() + 1e-9)


def steel_flex(t, at):
    """The blade's faint "nk": inharmonic partials of a thin steel strip, damped within ~0.1 s."""
    s = np.maximum(t - at, 0)
    ring = sum(a * np.sin(2 * np.pi * f * s) * np.exp(-s / tau)
               for f, a, tau in ((1870, 1.0, 0.070), (3310, 0.55, 0.045), (5260, 0.35, 0.030), (7640, 0.2, 0.020)))
    return np.where(t >= at, ring * smooth_step(t, at, at + 0.004), 0.0)


def build():
    rng = np.random.default_rng(SEED)
    t = np.arange(int(SECONDS * SR)) / SR
    # Air along the blade: rises into the swath, then tails away as the stroke slows.
    air_env = smooth_step(t, 0.0, 0.07) * np.exp(-np.maximum(t - 0.16, 0) / 0.11)
    air = swept_band_noise(rng, t, 1300, 3600, 1.6) * air_env
    # The shear through the stems, from contact to the end of the swath.
    snaps = stem_snaps(rng, t, 0.02, 0.30, 1100)
    # Cut grass falling over: a softer, lower rustle that outlasts the stroke.
    rustle_env = smooth_step(t, 0.08, 0.2) * np.exp(-np.maximum(t - 0.22, 0) / 0.16)
    rustle = swept_band_noise(rng, t, 2400, 900, 2.2) * rustle_env
    ring = steel_flex(t, 0.29)
    x = 0.62 * air + 0.34 * snaps + 0.30 * rustle + 0.07 * ring
    spec = np.fft.rfft(x)
    f = np.fft.rfftfreq(len(x), 1 / SR)
    spec[f < 120] = 0  # nothing low: no thump under the swish
    spec *= 1 / np.sqrt(1 + (f / 9000) ** 6)  # tame the fizz above 9 kHz
    x = np.fft.irfft(spec, len(x))
    fade = int(0.08 * SR)
    x[-fade:] *= np.linspace(1, 0, fade) ** 2
    x[:48] *= np.linspace(0, 1, 48)  # 1 ms fade-in: no click at the start
    return x / (np.abs(x).max() + 1e-9) * 10 ** (PEAK_DBFS / 20)


def write(path, x):
    pcm = np.clip(np.round(x * 32767), -32768, 32767).astype('<i2')
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(pcm.tobytes())
    return pcm


def main():
    pcm = write(OUT, build())
    peak = np.abs(pcm.astype(float)).max() / 32768
    rms = np.sqrt(np.mean((pcm.astype(float) / 32768) ** 2))
    print(f'Wrote {OUT} ({len(pcm) / SR:.2f} s): peak {20 * np.log10(peak):.1f} dBFS, RMS {20 * np.log10(rms):.1f} dBFS')


if __name__ == '__main__':
    main()
