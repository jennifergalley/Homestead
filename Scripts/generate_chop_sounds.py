"""Cut the hatchet-on-trunk chop samples from CC0 recordings.

    python Scripts/generate_chop_sounds.py

Reads Assets/Source/oga-wood-chop/ (gitignored; download chop-tree-fall.ogg from
https://opengameart.org/content/tree-chop-fall-thud and wood_hammer_02.ogg from
https://opengameart.org/content/100-cc0-metal-and-wood-sfx, both CC0) and writes Assets/Audio/Effects/ChopA..C.wav (48 kHz, 16-bit
mono), plus TreeFall.wav for the trunk landing. Needs ffmpeg (imageio-ffmpeg's bundled binary works) to decode the Ogg sources.

An axe biting a standing trunk is a dull, woody thock: most energy sits at 150-400 Hz, with a
short 1-3 kHz bite and a decay of about 100 ms. The old Kenney wood taps were 16 ms clicks, which
read as a hard stone knock. Each chop is onset-aligned, band-limited (70 Hz - ~5 kHz), faded out
and peak-normalised to -3 dBFS; the game sets the playback level.
"""
import pathlib
import subprocess

import numpy as np

SR = 48000
ROOT = pathlib.Path(__file__).resolve().parent.parent
SOURCE = ROOT / 'Assets' / 'Source' / 'oga-wood-chop'
OUT = ROOT / 'Assets' / 'Audio' / 'Effects'


def ffmpeg():
    try:
        import imageio_ffmpeg
        return imageio_ffmpeg.get_ffmpeg_exe()
    except ImportError:
        return 'ffmpeg'


def load(path):
    raw = subprocess.run([ffmpeg(), '-v', 'error', '-i', str(path), '-ac', '1', '-ar', str(SR), '-f', 'f64le', '-'],
                         capture_output=True, check=True).stdout
    return np.frombuffer(raw, np.float64)


def shape(x, start, length, pitch=1.0, lowpass=5500.0):
    s = x[int(start * SR):int((start + length) * SR)].copy()
    onset = int(np.argmax(np.abs(s) > 0.2 * np.abs(s).max()))
    s = s[max(0, onset - int(0.003 * SR)):]
    if pitch != 1.0:
        s = np.interp(np.arange(0, len(s) - 1, pitch), np.arange(len(s)), s)
    spec = np.fft.rfft(s)
    f = np.fft.rfftfreq(len(s), 1 / SR)
    spec *= 1 / np.sqrt(1 + (f / lowpass) ** 4) * (f / 70) ** 2 / (1 + (f / 70) ** 2)
    s = np.fft.irfft(spec, len(s))
    t = np.arange(len(s)) / len(s)
    s *= np.minimum(1, np.arange(len(s)) / (0.002 * SR))
    s *= np.where(t < 0.35, 1, 0.5 * (1 + np.cos(np.pi * (t - 0.35) / 0.65)))
    return s / np.abs(s).max() * 0.7


def write(name, s):
    OUT.mkdir(parents=True, exist_ok=True)
    subprocess.run([ffmpeg(), '-y', '-v', 'error', '-f', 'f64le', '-ar', str(SR), '-ac', '1', '-i', '-',
                    '-c:a', 'pcm_s16le', str(OUT / f'{name}.wav')], input=s.tobytes(), check=True)


def main():
    tree = load(SOURCE / 'chop-tree-fall.ogg')
    hammer = load(SOURCE / 'wood_hammer_02.ogg')
    write('ChopA', shape(tree, 0.02, 0.26))
    write('ChopB', shape(tree, 0.02, 0.26, pitch=1.07, lowpass=4800))
    write('ChopC', shape(hammer, 0.0, 0.22, pitch=0.9, lowpass=4500))
    # The trunk hitting the forest floor: the recording's own landing (1.6 s in), a soft ~0.8 s thump.
    write('TreeFall', shape(tree, 1.55, 1.05, lowpass=3000))


if __name__ == '__main__':
    main()
