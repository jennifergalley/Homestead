"""Synthesize the standing-room hearth's crackle loop (original to this project).

    python Scripts/generate_hearth_crackle.py

Writes Assets/Audio/Ambience/HearthCrackle.wav (48 kHz, 16-bit mono, 24 s, seamless).

A wood fire's sound has three layers, each built from seeded noise:
- a low, breathing roar of burning gas (brown noise band-passed to 60-400 Hz, slowly modulated);
- a soft hiss of escaping steam and sap (band-passed 2-6 kHz, very quiet);
- crackles: short, sharp broadband clicks in Poisson bursts (a few per second), each a
  1-12 ms decaying noise burst through a random resonance, now and then a bigger "pop" of
  a sap pocket followed by a small shower of ticks.
The tail is crossfaded into the head, so the loop has no seam. Needs numpy.
"""
import pathlib
import wave

import numpy as np

ROOT = pathlib.Path(__file__).resolve().parent.parent
OUT = ROOT / 'Assets' / 'Audio' / 'Ambience' / 'HearthCrackle.wav'
SR = 48000
SECONDS = 24.0
FADE = 1.5


def band(signal, low, high):
    spectrum = np.fft.rfft(signal)
    freqs = np.fft.rfftfreq(len(signal), 1.0 / SR)
    shape = np.clip((freqs - low * 0.7) / (low * 0.3 + 1e-9), 0, 1) * np.clip((high * 1.3 - freqs) / (high * 0.3), 0, 1)
    return np.fft.irfft(spectrum * shape, len(signal))


def smooth_noise(rng, n, rate_hz):
    points = rng.normal(0.0, 1.0, int(n / SR * rate_hz) + 3)
    x = np.linspace(0, len(points) - 3, n)
    i = x.astype(int)
    f = x - i
    return points[i] * (1 - f) + points[i + 1] * f


def click(rng, big=False):
    length = int(SR * (rng.uniform(0.006, 0.022) if big else rng.uniform(0.001, 0.009)))
    t = np.arange(length) / SR
    decay = np.exp(-t / (rng.uniform(0.002, 0.006) if big else rng.uniform(0.0004, 0.0022)))
    burst = rng.normal(0.0, 1.0, length) * decay
    # A little ring from the wood's resonance.
    freq = rng.uniform(900.0, 4200.0)
    burst += 0.6 * np.sin(2 * np.pi * freq * t + rng.uniform(0, 6.3)) * decay
    return burst * (rng.uniform(0.5, 1.0) if big else rng.uniform(0.08, 0.45))


def main():
    rng = np.random.default_rng(1851)
    total = int(SR * (SECONDS + FADE))
    brown = np.cumsum(rng.normal(0.0, 1.0, total))
    brown -= np.convolve(brown, np.ones(4801) / 4801, mode='same')
    roar = band(brown, 60.0, 400.0)
    roar = roar / np.max(np.abs(roar)) * (0.55 + 0.25 * smooth_noise(rng, total, 0.6).clip(-1.5, 1.5) / 1.5)
    hiss = band(rng.normal(0.0, 1.0, total), 2000.0, 6000.0)
    hiss = hiss / np.max(np.abs(hiss)) * 0.05 * (1.0 + 0.5 * smooth_noise(rng, total, 0.3).clip(-1, 1))
    crackle = np.zeros(total)
    activity = 0.6 + 0.4 * smooth_noise(rng, total, 0.25).clip(-1.5, 1.5) / 1.5
    t = 0.0
    while t < SECONDS + FADE - 0.1:
        rate = 5.0 * activity[int(t * SR)]
        t += rng.exponential(1.0 / max(rate, 0.5))
        start = int(t * SR)
        big = rng.random() < 0.08
        events = [click(rng, big)]
        if big:
            events += [click(rng) for _ in range(rng.integers(2, 7))]
        offset = start
        for burst in events:
            end = min(offset + len(burst), total)
            crackle[offset:end] += burst[:end - offset]
            offset += int(SR * rng.uniform(0.004, 0.05))
    mix = 0.5 * roar + 1.6 * hiss + 0.9 * crackle
    loop_n, fade_n = int(SR * SECONDS), int(SR * FADE)
    loop = mix[:loop_n].copy()
    ramp = np.linspace(0.0, 1.0, fade_n)
    loop[:fade_n] = loop[:fade_n] * ramp + mix[loop_n:loop_n + fade_n] * (1.0 - ramp)
    # Soft-limit the sap pops so the bed stays audible: pops end up about 6-8x the RMS.
    loop = loop / np.percentile(np.abs(loop), 99.9)
    loop = np.tanh(loop * 1.4) / np.tanh(1.4) * 0.7
    print(f'rms {np.sqrt(np.mean(loop ** 2)):.3f} peak {np.max(np.abs(loop)):.3f}')
    OUT.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(OUT), 'wb') as out:
        out.setnchannels(1)
        out.setsampwidth(2)
        out.setframerate(SR)
        out.writeframes((loop * 32767).astype('<i2').tobytes())
    print(f'Wrote {OUT} ({SECONDS:.0f} s)')


if __name__ == '__main__':
    main()
