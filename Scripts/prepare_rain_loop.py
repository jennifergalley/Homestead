"""Cut the estate's rain ambience loop from a real CC0 rain recording.

    python Scripts/prepare_rain_loop.py

Source: "Rain (loopable)" by Ylmir (OpenGameArt, CC0), https://opengameart.org/content/rain-loopable,
file 3.ogg from "Rain OGG.zip" (a steady window-side rain, 45 s). The zip is downloaded into
Assets/Source/opengameart-rain/ (gitignored) when it's missing. Decoding the OGG needs the ffmpeg
binary from the imageio-ffmpeg package (pip install imageio-ffmpeg).

Writes Assets/Audio/Ambience/RainLoop.wav (44.1 kHz, 16-bit stereo): the rumble under 80 Hz is
removed and the hiss over 11 kHz rolled off so it reads as soft rain on grass and leaves, the level
is set to about -24 dBFS RMS, and the tail is crossfaded into the head so the loop has no seam.
UHomesteadWeather plays it with the rain's strength and muffles it indoors. Needs numpy.
"""
import io
import pathlib
import subprocess
import urllib.request
import wave
import zipfile

import numpy as np

ROOT = pathlib.Path(__file__).resolve().parent.parent
SOURCE_DIR = ROOT / 'Assets' / 'Source' / 'opengameart-rain'
ZIP = SOURCE_DIR / 'Rain OGG.zip'
URL = 'https://opengameart.org/sites/default/files/Rain%20OGG.zip'
MEMBER = '3.ogg'
OUT = ROOT / 'Assets' / 'Audio' / 'Ambience' / 'RainLoop.wav'
RATE = 44100
FADE = 2.0
TARGET_RMS_DB = -24.0


def decode(ogg_bytes):
    import imageio_ffmpeg
    ffmpeg = imageio_ffmpeg.get_ffmpeg_exe()
    result = subprocess.run([ffmpeg, '-hide_banner', '-loglevel', 'error', '-i', 'pipe:0', '-ac', '2',
                             '-ar', str(RATE), '-f', 's16le', 'pipe:1'], input=ogg_bytes, capture_output=True, check=True)
    return np.frombuffer(result.stdout, '<i2').astype(np.float64).reshape(-1, 2) / 32768.0


def main():
    if not ZIP.exists():
        SOURCE_DIR.mkdir(parents=True, exist_ok=True)
        request = urllib.request.Request(URL, headers={'User-Agent': 'Mozilla/5.0 Homestead'})
        ZIP.write_bytes(urllib.request.urlopen(request).read())
    with zipfile.ZipFile(ZIP) as archive:
        name = next(n for n in archive.namelist() if n.endswith(MEMBER))
        x = decode(archive.read(name))
    freqs = np.fft.rfftfreq(len(x), 1.0 / RATE)
    shape = np.clip((freqs - 60.0) / 40.0, 0.0, 1.0) * np.clip((15000.0 - freqs) / 4000.0, 0.0, 1.0)
    x = np.stack([np.fft.irfft(np.fft.rfft(x[:, c]) * shape, len(x)) for c in range(2)], -1)
    fade_n = int(RATE * FADE)
    loop = x[:-fade_n].copy()
    ramp = np.linspace(0.0, 1.0, fade_n)[:, None]
    loop[:fade_n] = loop[:fade_n] * ramp + x[-fade_n:] * (1.0 - ramp)
    loop *= 10 ** (TARGET_RMS_DB / 20.0) / np.sqrt(np.mean(loop ** 2))
    peak = np.max(np.abs(loop))
    if peak > 0.9:
        loop *= 0.9 / peak
    print(f'rms {20 * np.log10(np.sqrt(np.mean(loop ** 2))):.1f} dBFS peak {np.max(np.abs(loop)):.3f} {len(loop) / RATE:.1f} s')
    OUT.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(OUT), 'wb') as out:
        out.setnchannels(2)
        out.setsampwidth(2)
        out.setframerate(RATE)
        out.writeframes((np.clip(loop, -1, 1) * 32767).astype('<i2').tobytes())
    print(f'Wrote {OUT}')


if __name__ == '__main__':
    main()
