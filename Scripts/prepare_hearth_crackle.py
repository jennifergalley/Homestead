"""Cut the standing-room hearth's crackle loop from a real CC0 fireplace recording.

    python Scripts/prepare_hearth_crackle.py

Source: "Fireplace Sound loop" by PagDev (OpenGameArt, CC0),
https://opengameart.org/content/fireplace-sound-loop. Download fire.wav into
Assets/Source/opengameart-fireplace/fire.wav (gitignored); the script downloads it there if it's missing.

Writes Assets/Audio/Ambience/HearthCrackle.wav (44.1 kHz, 16-bit mono). The rumble under 60 Hz
is removed, the air over 9 kHz is rolled off so the fire reads as a warm, soft crackle rather than
hiss, and the tail is crossfaded into the head so the loop has no seam. The earlier synthesized loop
sounded like static in play. Needs numpy.
"""
import pathlib
import urllib.request
import wave

import numpy as np

ROOT = pathlib.Path(__file__).resolve().parent.parent
SOURCE = ROOT / 'Assets' / 'Source' / 'opengameart-fireplace' / 'fire.wav'
URL = 'https://opengameart.org/sites/default/files/fire.wav'
OUT = ROOT / 'Assets' / 'Audio' / 'Ambience' / 'HearthCrackle.wav'
FADE = 1.5


def load(path):
    with wave.open(str(path)) as w:
        width, channels, rate = w.getsampwidth(), w.getnchannels(), w.getframerate()
        raw = w.readframes(w.getnframes())
    data = np.frombuffer(raw, {2: '<i2', 4: '<i4'}[width]).astype(np.float64) / 2 ** (8 * width - 1)
    return data.reshape(-1, channels).mean(axis=1), rate


def main():
    if not SOURCE.exists():
        SOURCE.parent.mkdir(parents=True, exist_ok=True)
        urllib.request.urlretrieve(URL, SOURCE)
    x, rate = load(SOURCE)
    spectrum = np.fft.rfft(x)
    freqs = np.fft.rfftfreq(len(x), 1.0 / rate)
    shape = np.clip((freqs - 60.0) / 60.0, 0.0, 1.0) * np.clip((14000.0 - freqs) / 5000.0, 0.0, 1.0)
    x = np.fft.irfft(spectrum * shape, len(x))
    fade_n = int(rate * FADE)
    loop = x[:-fade_n].copy()
    ramp = np.linspace(0.0, 1.0, fade_n)
    loop[:fade_n] = loop[:fade_n] * ramp + x[-fade_n:] * (1.0 - ramp)
    loop = loop / np.max(np.abs(loop)) * 0.7
    print(f'rms {np.sqrt(np.mean(loop ** 2)):.3f} peak {np.max(np.abs(loop)):.3f} {len(loop) / rate:.1f} s')
    OUT.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(OUT), 'wb') as out:
        out.setnchannels(1)
        out.setsampwidth(2)
        out.setframerate(rate)
        out.writeframes((loop * 32767).astype('<i2').tobytes())
    print(f'Wrote {OUT}')


if __name__ == '__main__':
    main()
