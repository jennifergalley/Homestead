"""Generate the heroine's bare-foot-on-soft-dirt footstep samples (original, no third-party audio).

    python Scripts/generate_bare_footsteps.py

Writes Assets/Audio/Footsteps/BareStepWalk_0N.wav and BareStepRun_0N.wav (48 kHz, 16-bit mono).
The output is deterministic (fixed seeds), so the WAVs can be regenerated and compared.

Bare skin on soft soil has no hard transient: the sole's padding gives a rounded attack of a few
milliseconds, and most energy sits below about 700 Hz. A walking step lands heel then ball of the
foot; a running step lands once on the forefoot and pushes off with a light scuff. Loose soil adds
a faint, short rustle of grains. Each layer is band-limited noise or a decaying low tone shaped by
those timings.
"""
import pathlib
import wave

import numpy as np

SR = 48000
OUT = pathlib.Path(__file__).resolve().parent.parent / 'Assets' / 'Audio' / 'Footsteps'


def band_noise(rng, seconds, lo, hi):
    n = int(seconds * SR)
    spec = np.fft.rfft(rng.standard_normal(n))
    f = np.fft.rfftfreq(n, 1 / SR)
    # Raised-cosine band edges an octave wide avoid ringing.
    rise = np.clip(np.log2(np.maximum(f, 1) / (lo / 2)), 0, 1)
    fall = np.clip(np.log2(hi * 2 / np.maximum(f, 1)), 0, 1)
    shaped = np.fft.irfft(spec * (0.5 - 0.5 * np.cos(np.pi * rise)) * (0.5 - 0.5 * np.cos(np.pi * fall)), n)
    return shaped / (np.abs(shaped).max() + 1e-9)


def envelope(seconds, attack_ms, tau_ms):
    t = np.arange(int(seconds * SR)) / SR
    a = attack_ms / 1000
    rise = np.where(t < a, 0.5 - 0.5 * np.cos(np.pi * np.minimum(t / a, 1)), 1.0)
    return rise * np.exp(-np.maximum(t - a, 0) / (tau_ms / 1000))


def thump(seconds, f0, f1, attack_ms, tau_ms):
    t = np.arange(int(seconds * SR)) / SR
    freq = f1 + (f0 - f1) * np.exp(-t / 0.04)
    return np.sin(2 * np.pi * np.cumsum(freq) / SR) * envelope(seconds, attack_ms, tau_ms)


def grains(rng, seconds, rate, lo, hi, attack_ms, tau_ms):
    n = int(seconds * SR)
    clicks = np.zeros(n)
    count = rng.poisson(rate * seconds)
    clicks[rng.integers(0, n, count)] = rng.exponential(1.0, count) * rng.choice([-1, 1], count)
    spec = np.fft.rfft(clicks)
    f = np.fft.rfftfreq(n, 1 / SR)
    spec[(f < lo) | (f > hi)] *= 0.05
    g = np.fft.irfft(spec, n)
    return g / (np.abs(g).max() + 1e-9) * envelope(seconds, attack_ms, tau_ms)


def place(buffer, layer, at):
    start = int(at * SR)
    end = min(len(buffer), start + len(layer))
    buffer[start:end] += layer[:end - start]


def finish(x):
    spec = np.fft.rfft(x)
    f = np.fft.rfftfreq(len(x), 1 / SR)
    spec *= 1 / np.sqrt(1 + (f / 3200) ** 4)  # gentle low-pass: no bright edge
    spec[f < 35] = 0
    x = np.fft.irfft(spec, len(x))
    fade = int(0.04 * SR)
    x[-fade:] *= np.linspace(1, 0, fade)
    return x / (np.abs(x).max() + 1e-9) * 10 ** (-3 / 20)


def walk_step(seed):
    rng = np.random.default_rng(seed)
    x = np.zeros(int(0.36 * SR))
    heel = 0.004
    place(x, band_noise(rng, 0.2, 110, 900) * envelope(0.2, rng.uniform(5, 9), rng.uniform(22, 32)), heel)
    place(x, 0.22 * thump(0.2, rng.uniform(80, 95), 58, 6, rng.uniform(24, 30)), heel)
    ball = heel + rng.uniform(0.075, 0.11)
    amp = rng.uniform(0.45, 0.6)
    place(x, amp * band_noise(rng, 0.15, 110, 900) * envelope(0.15, 4, rng.uniform(15, 20)), ball)
    place(x, 0.15 * amp * thump(0.15, 100, 70, 4, 18), ball)
    soil = rng.uniform(0.14, 0.18)
    place(x, soil * grains(rng, 0.22, 1500, 600, 2800, 8, 45), heel)
    place(x, 0.6 * soil * grains(rng, 0.15, 1200, 700, 3000, 5, 30), ball)
    return finish(x)


def run_step(seed):
    rng = np.random.default_rng(seed)
    x = np.zeros(int(0.32 * SR))
    land = 0.004
    place(x, band_noise(rng, 0.2, 120, 1000) * envelope(0.2, rng.uniform(4, 6), rng.uniform(24, 32)), land)
    place(x, 0.3 * thump(0.2, rng.uniform(90, 105), 65, 4, rng.uniform(24, 30)), land)
    soil = rng.uniform(0.18, 0.22)
    place(x, soil * grains(rng, 0.22, 2000, 600, 3000, 6, 50), land)
    push = land + rng.uniform(0.11, 0.14)
    place(x, rng.uniform(0.12, 0.18) * band_noise(rng, 0.12, 300, 2000) * envelope(0.12, 10, 25), push)
    return finish(x)


def write(path, x):
    pcm = np.clip(x * 32767, -32768, 32767).astype('<i2')
    with wave.open(str(path), 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(pcm.tobytes())


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    for i in range(6):
        write(OUT / f'BareStepWalk_{i:02d}.wav', walk_step(1000 + i))
    for i in range(4):
        write(OUT / f'BareStepRun_{i:02d}.wav', run_step(2000 + i))
    print(f'Wrote 10 footstep samples to {OUT}')


if __name__ == '__main__':
    main()
