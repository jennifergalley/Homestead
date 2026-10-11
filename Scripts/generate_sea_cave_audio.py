"""Generate the sea cave's ambience loops (original, no third-party audio).

    python Scripts/generate_sea_cave_audio.py

Writes Assets/Audio/Ambience/SeaCaveDrips.wav (48 s) and SeaCaveSurf.wav (64 s), 48 kHz 16-bit mono loops that
wrap without a seam (every layer, the reverb tail included, is generated circularly). Deterministic (fixed
seeds), so the WAVs can be regenerated and compared.

Drips: a falling drop's sound is the small air bubble it traps ringing (a Minnaert resonance): a sine of
1-3 kHz whose pitch rises as the bubble shrinks, decaying in a few tens of milliseconds, after a faint tick.
Seeps drip at a steady rate each, so three sources keep their own periods (with a little jitter) at different
distances. Surf: heard from inside, the sea is a low roar: noise below a few hundred hertz swelling with each
wave (about every 9-12 s), a soft thump when one fills the mouth, and the hiss of its backwash. Both share a
cave impulse response: dense early reflections and a ~2.2 s tail that loses its highs.
"""
import pathlib
import wave

import numpy as np

SR = 48000
OUT = pathlib.Path(__file__).resolve().parent.parent / 'Assets' / 'Audio' / 'Ambience'
DRIPS_SECONDS = 48.0
SURF_SECONDS = 64.0
RT60 = 2.2                   # the chamber's reverb time (s)
# (period s, jitter s, gain, distance wet share): a seep over the pool, one by the wall, one far off.
DRIP_SOURCES = [(2.7, 0.25, 1.0, 0.45), (4.1, 0.6, 0.65, 0.6), (7.3, 1.4, 0.4, 0.8)]


def lowpass(x, hz):
    spec = np.fft.rfft(x)
    f = np.fft.rfftfreq(len(x), 1 / SR)
    return np.fft.irfft(spec / np.sqrt(1 + (f / hz) ** 4), len(x))


def highpass(x, hz):
    spec = np.fft.rfft(x)
    f = np.fft.rfftfreq(len(x), 1 / SR)
    return np.fft.irfft(spec * (f / hz) ** 2 / np.sqrt(1 + (f / hz) ** 4), len(x))


def cave_impulse(rng, seconds=3.0):
    n = int(seconds * SR)
    t = np.arange(n) / SR
    tail = rng.standard_normal(n) * np.exp(-6.91 * t / RT60)
    # Highs die faster than lows in rock and damp air: blend toward a darker copy along the tail.
    dark = lowpass(tail, 1800.0)
    w = np.clip(t / 0.6, 0, 1)
    ir = (1 - w) * tail + w * dark
    ir[:int(0.012 * SR)] = 0.0
    for delay, gain in ((0.011, 0.5), (0.019, 0.38), (0.027, 0.3), (0.041, 0.22), (0.057, 0.18)):
        ir[int(delay * SR)] += gain
    return ir / np.sqrt(np.sum(ir ** 2))


def circular_convolve(x, ir):
    n = len(x)
    padded = np.zeros(n)
    padded[:len(ir)] = ir[:n]
    return np.fft.irfft(np.fft.rfft(x) * np.fft.rfft(padded), n)


def drop(rng):
    seconds = 0.12
    t = np.arange(int(seconds * SR)) / SR
    f0 = rng.uniform(900.0, 2600.0)
    rise = rng.uniform(0.3, 0.7)
    tau = rng.uniform(0.012, 0.035)
    freq = f0 * (1 + rise * (1 - np.exp(-t / 0.02)))
    phase = 2 * np.pi * np.cumsum(freq) / SR
    ring = np.sin(phase) * np.exp(-t / tau) * np.clip(t / 0.0015, 0, 1)
    tick = highpass(rng.standard_normal(len(t)), 3000.0) * np.exp(-t / 0.0012) * 0.25
    return ring + tick


def drips(rng):
    n = int(DRIPS_SECONDS * SR)
    dry = np.zeros(n)
    wet_in = np.zeros(n)
    for period, jitter, gain, wet_share in DRIP_SOURCES:
        count = max(1, int(round(DRIPS_SECONDS / period)))
        step = DRIPS_SECONDS / count           # a whole number of drips per loop keeps the rhythm across the seam
        for k in range(count):
            at = int(((k * step + rng.uniform(-jitter, jitter)) % DRIPS_SECONDS) * SR)
            d = drop(rng) * gain * rng.uniform(0.75, 1.0)
            idx = (at + np.arange(len(d))) % n
            dry[idx] += d * (1 - wet_share)
            wet_in[idx] += d * wet_share
    wet = circular_convolve(wet_in, cave_impulse(rng)) * 0.9
    return dry + wet


def surf(rng):
    n = int(SURF_SECONDS * SR)
    t = np.arange(n) / SR
    roar = lowpass(rng.standard_normal(n), 260.0)
    hiss = lowpass(highpass(rng.standard_normal(n), 900.0), 3500.0)
    swell = np.zeros(n)
    backwash = np.zeros(n)
    thumps = np.zeros(n)
    at = 0.0
    while at < SURF_SECONDS:
        period = rng.uniform(9.0, 12.5)
        rise, fall = rng.uniform(1.5, 2.5), rng.uniform(4.0, 6.0)
        size = rng.uniform(0.6, 1.0)
        dt = (t - at) % SURF_SECONDS
        shape = np.where(dt < rise, np.sin(0.5 * np.pi * dt / rise) ** 2, np.exp(-(dt - rise) / fall))
        shape *= dt < rise + 4 * fall
        swell += size * shape
        bw = np.exp(-((dt - rise - 1.6) / 1.2) ** 2)
        backwash += 0.5 * size * bw
        if size > 0.75:
            k = int(((at + rise) % SURF_SECONDS) * SR)
            boom_t = np.arange(int(0.9 * SR)) / SR
            boom = np.sin(2 * np.pi * rng.uniform(55.0, 80.0) * boom_t) * np.exp(-boom_t / 0.25) * np.clip(boom_t / 0.03, 0, 1)
            thumps[(k + np.arange(len(boom))) % n] += boom * size
        at += period
    dry = roar * (0.25 + swell) + 0.12 * hiss * backwash + 0.6 * thumps
    wet = circular_convolve(dry, cave_impulse(rng)) * 0.8
    return 0.55 * dry + wet


def write(path, x, peak=0.7):
    x = x - x.mean()
    x = x / (np.abs(x).max() + 1e-9) * peak
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), 'wb') as f:
        f.setnchannels(1)
        f.setsampwidth(2)
        f.setframerate(SR)
        f.writeframes((x * 32767).astype('<i2').tobytes())


if __name__ == '__main__':
    write(OUT / 'SeaCaveDrips.wav', drips(np.random.default_rng(2610)), peak=0.6)
    write(OUT / 'SeaCaveSurf.wav', surf(np.random.default_rng(2611)), peak=0.7)
    print('wrote', OUT / 'SeaCaveDrips.wav', 'and', OUT / 'SeaCaveSurf.wav')
