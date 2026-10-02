"""Generate the billhook's cane-cutting cues: a hooked slash, a woody fibrous snap and the leaves falling
(original, no third-party audio).

    python Scripts/generate_billhook_sound.py

Writes Assets/Audio/Effects/CaneCutA.wav, CaneCutB.wav and CaneCutC.wav (48 kHz, 16-bit mono), each matched to the
other effects' loudness: its loudest 100 ms at TARGET_RMS_DBFS (the chops and the scythe sit at -17 to -19), never
peaking above PEAK_CEILING_DBFS. Deterministic (fixed seeds), so the WAVs can be regenerated and compared.

What a billhook does to a bramble or hazel cane (Jenny, 2026-09-30: "Clearing canes with the billhook needs a new
sound"): the hooked blade comes in fast and short, a brief airy slash; at contact the cane snaps, a sharp click with a
hollow woody knock (a few damped resonances between 500 Hz and 1.6 kHz, the cane's short length ringing) and a quick
tearing run of fibre clicks as the bark and pith give; then the leaves and side shoots rustle as the cut cane drops.
The cue starts at blade contact (AHomesteadController::LandOvergrowthSwing, on the machete hack's MacheteClearSeconds),
so the slash lead-in is only a few milliseconds.
"""
import pathlib
import wave

import numpy as np

SR = 48000
SECONDS = 0.62
TARGET_RMS_DBFS = -17.5
PEAK_CEILING_DBFS = -3.0
OUT_DIR = pathlib.Path(__file__).resolve().parent.parent / 'Assets' / 'Audio' / 'Effects'
# (name, seed, cane resonances Hz, snap at s, fibre clicks, rustle level)
VARIANTS = (
    ('CaneCutA', 4101, (640.0, 1020.0, 1540.0), 0.012, 9, 0.34),
    ('CaneCutB', 4102, (560.0, 910.0, 1380.0), 0.010, 12, 0.30),
    ('CaneCutC', 4103, (720.0, 1180.0, 1660.0), 0.014, 7, 0.38),
)


def smooth_step(t, start, end):
    x = np.clip((t - start) / (end - start), 0, 1)
    return x * x * (3 - 2 * x)


def swept_band_noise(rng, n, f_start, f_end, width_octaves):
    """White noise through a band whose centre glides from f_start to f_end (Hz) over the sound."""
    out = np.zeros(n)
    block, hop = 1024, 512
    window = np.hanning(block)
    noise = rng.standard_normal(n + block)
    f = np.fft.rfftfreq(block, 1 / SR)
    for start in range(0, n, hop):
        centre = f_start * (f_end / f_start) ** min(1.0, start / n)
        gain = np.exp(-0.5 * (np.log2(np.maximum(f, 1) / centre) / (width_octaves / 2)) ** 2)
        frame = np.fft.irfft(np.fft.rfft(noise[start:start + block] * window) * gain, block)
        end = min(n, start + block)
        out[start:end] += frame[:end - start]
    return out / (np.abs(out).max() + 1e-9)


def snap(rng, t, at, resonances):
    """The cane breaking: a sharp click, a hollow woody knock and a brief bright crack of splitting fibre."""
    s = np.maximum(t - at, 0)
    on = t >= at
    click_len = 0.0012
    click = np.where(on & (s < click_len), rng.standard_normal(len(t)) * (1 - s / click_len), 0.0)
    knock = sum(a * np.sin(2 * np.pi * f * rng.uniform(0.97, 1.03) * s + rng.uniform(0, 6.28)) * np.exp(-s / tau)
                for f, a, tau in zip(resonances, (1.0, 0.6, 0.35), (0.030, 0.020, 0.012)))
    crack = swept_band_noise(rng, len(t), 4200, 2200, 1.4) * np.exp(-s / 0.014)
    return np.where(on, 0.9 * click + 0.55 * knock + 0.6 * crack, 0.0)


def fibre_tear(rng, t, at, count):
    """The bark and pith giving way after the snap: a short, thinning run of tiny clicks."""
    out = np.zeros(len(t))
    for k in range(count):
        when = at + 0.004 + rng.exponential(0.018) * (1 + k / count)
        i = int(when * SR)
        length = int(rng.uniform(0.0004, 0.0016) * SR)
        if i + length >= len(t):
            continue
        u = np.arange(length) / SR
        tone = rng.uniform(1800, 6000)
        out[i:i + length] += rng.uniform(0.25, 0.8) * (1 - k / (count + 2)) * np.sin(2 * np.pi * tone * u) * np.exp(-u / (length / SR / 3))
    return out


def leaf_rustle(rng, t, start, level):
    """Leaves and side shoots brushing as the cut cane drops: grainy, airy, slowly settling."""
    n = len(t)
    body = swept_band_noise(rng, n, 3600, 1500, 2.0)
    grains = np.convolve(np.abs(rng.standard_normal(n)), np.hanning(int(0.012 * SR)), 'same')
    grains /= grains.max() + 1e-9
    env = smooth_step(t, start, start + 0.06) * np.exp(-np.maximum(t - start - 0.06, 0) / 0.17)
    return level * body * (0.35 + 0.65 * grains) * env


def build(seed, resonances, at, fibres, rustle_level):
    rng = np.random.default_rng(seed)
    t = np.arange(int(SECONDS * SR)) / SR
    # The hooked slash: a short airy swish arriving at contact.
    slash = swept_band_noise(rng, len(t), 1600, 4200, 1.5) * smooth_step(t, 0.0, at) * np.exp(-np.maximum(t - at, 0) / 0.035)
    x = (0.40 * slash + snap(rng, t, at, resonances) + 0.45 * fibre_tear(rng, t, at, fibres)
         + leaf_rustle(rng, t, at + 0.06, rustle_level))
    spec = np.fft.rfft(x)
    f = np.fft.rfftfreq(len(x), 1 / SR)
    spec[f < 150] = 0                               # no thump: a cane is light
    spec *= 1 / np.sqrt(1 + (f / 10000) ** 6)       # tame the fizz above 10 kHz
    x = np.fft.irfft(spec, len(x))
    fade = int(0.1 * SR)
    x[-fade:] *= np.linspace(1, 0, fade) ** 2
    x[:24] *= np.linspace(0, 1, 24)                 # 0.5 ms fade-in: no click at the start
    window = int(0.1 * SR)
    loudest = np.sqrt(np.convolve(x * x, np.ones(window) / window, 'valid').max())
    x *= 10 ** (TARGET_RMS_DBFS / 20) / (loudest + 1e-12)
    peak = np.abs(x).max()
    ceiling = 10 ** (PEAK_CEILING_DBFS / 20)
    if peak > ceiling:
        # Soft-limit the snap's click rather than turning the whole cue down (it's a millisecond long).
        x = ceiling * np.tanh(x / ceiling)
    return x


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
    for name, seed, resonances, at, fibres, rustle in VARIANTS:
        pcm = write(OUT_DIR / f'{name}.wav', build(seed, resonances, at, fibres, rustle)).astype(float) / 32768
        window = int(0.1 * SR)
        loudest = np.sqrt(np.convolve(pcm * pcm, np.ones(window) / window, 'valid').max())
        print(f'Wrote {name}.wav ({len(pcm) / SR:.2f} s): peak {20 * np.log10(np.abs(pcm).max()):.1f} dBFS, '
              f'loudest-100ms RMS {20 * np.log10(loudest):.1f} dBFS')


if __name__ == '__main__':
    main()
