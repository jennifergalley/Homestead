"""Generate the creek's ripple textures and its flowing-water loop.

    python Scripts/generate_creek_assets.py

Textures (original to this project, synthesized from filtered noise) go to Assets/Environment/Creek:
  T_CreekRipples_N.png  512x512 tileable normal map (DirectX green). Ripples are stretched along V,
                        the direction the water flows, like the chevrons over a pebbly stream bed.
  T_CreekFoam.png       512x512 tileable grayscale flecks of foam and bubbles.

The loop goes to Assets/Audio/Ambience/CreekLoop.wav (48 kHz, 16-bit mono). It is cut from
"Small brook, stream, water sound" by SamsterBirdies (Freesound 584269, CC0); download the HQ
preview into Assets/Source/freesound-brook/SmallBrook.mp3 (gitignored). The rumble under 90 Hz
and the hiss over 9 kHz are rolled off so the brook reads as a soft burble, and the end is
crossfaded into the start so it loops without a seam. Needs numpy, Pillow and ffmpeg
(imageio-ffmpeg's bundled binary works).
"""
import pathlib
import subprocess

import numpy as np
from PIL import Image

ROOT = pathlib.Path(__file__).resolve().parent.parent
TEXTURES = ROOT / 'Assets' / 'Environment' / 'Creek'
AUDIO = ROOT / 'Assets' / 'Audio' / 'Ambience'
BROOK = ROOT / 'Assets' / 'Source' / 'freesound-brook' / 'SmallBrook.mp3'
SIZE = 512
SR = 48000


def ffmpeg():
    try:
        import imageio_ffmpeg
        return imageio_ffmpeg.get_ffmpeg_exe()
    except ImportError:
        return 'ffmpeg'


def band_noise(rng, low, high, stretch_v=1.0, power=1.0):
    """Tileable noise whose features span low..high cycles per tile; stretch_v > 1 elongates along V."""
    k = np.fft.fftfreq(SIZE) * SIZE
    ku, kv = np.meshgrid(k, k)
    radius = np.sqrt(ku ** 2 + (kv * stretch_v) ** 2)
    band = np.exp(-((np.log(np.maximum(radius, 1e-6)) - np.log(np.sqrt(low * high))) / np.log(high / low) * 2) ** 2)
    spectrum = band / np.maximum(radius, 1) ** power * np.exp(2j * np.pi * rng.random((SIZE, SIZE)))
    spectrum[0, 0] = 0
    field = np.real(np.fft.ifft2(spectrum))
    return field / np.abs(field).max()


def ripples():
    rng = np.random.default_rng(58426)
    height = band_noise(rng, 4, 14, 2.2, 0.6) + 0.45 * band_noise(rng, 12, 40, 1.6, 0.4)
    # Gradient on the torus so the map tiles; DirectX normals flip green.
    du = (np.roll(height, -1, 1) - np.roll(height, 1, 1)) * 0.5
    dv = (np.roll(height, -1, 0) - np.roll(height, 1, 0)) * 0.5
    strength = 18.0
    normal = np.dstack([-du * strength, dv * strength, np.ones_like(height)])
    normal /= np.linalg.norm(normal, axis=2, keepdims=True)
    return Image.fromarray(np.uint8(np.clip((normal * 0.5 + 0.5) * 255 + 0.5, 0, 255)), 'RGB')


def foam():
    rng = np.random.default_rng(26958)
    base = band_noise(rng, 10, 48, 1.8, 0.3)
    base /= base.std()
    flecks = np.clip((base - 1.2) / 1.3, 0, 1) ** 1.5
    veil = np.clip(band_noise(rng, 2, 6, 2.5, 0.2) * 0.5 + 0.5, 0, 1)
    return Image.fromarray(np.uint8(np.clip(flecks * (0.35 + 0.65 * veil), 0, 1) * 255), 'L')


def load(path):
    raw = subprocess.run([ffmpeg(), '-v', 'error', '-i', str(path), '-ac', '1', '-ar', str(SR), '-f', 'f64le', '-'],
                         capture_output=True, check=True).stdout
    return np.frombuffer(raw, np.float64)


def creek_loop():
    x = load(BROOK)
    start, length, fade = 8.0, 36.0, 4.0
    s = x[int(start * SR):int((start + length + fade) * SR)].copy()
    spec = np.fft.rfft(s)
    f = np.fft.rfftfreq(len(s), 1 / SR)
    spec *= (f / 90) ** 4 / (1 + (f / 90) ** 4) / np.sqrt(1 + (f / 9000) ** 4)
    s = np.fft.irfft(spec, len(s))
    n, x_len = int(length * SR), int(fade * SR)
    t = np.linspace(0, np.pi / 2, x_len)
    out = s[:n].copy()
    out[:x_len] = s[:x_len] * np.sin(t) + s[n:n + x_len] * np.cos(t)
    return out / np.abs(out).max() * 0.7


def main():
    TEXTURES.mkdir(parents=True, exist_ok=True)
    ripples().save(TEXTURES / 'T_CreekRipples_N.png')
    foam().save(TEXTURES / 'T_CreekFoam.png')
    AUDIO.mkdir(parents=True, exist_ok=True)
    loop = creek_loop()
    subprocess.run([ffmpeg(), '-y', '-v', 'error', '-f', 'f64le', '-ar', str(SR), '-ac', '1', '-i', '-',
                    '-c:a', 'pcm_s16le', str(AUDIO / 'CreekLoop.wav')], input=loop.tobytes(), check=True)
    print('Wrote', TEXTURES / 'T_CreekRipples_N.png', TEXTURES / 'T_CreekFoam.png', AUDIO / 'CreekLoop.wav')


if __name__ == '__main__':
    main()
