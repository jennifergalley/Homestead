"""Paint the MetaHuman's baked grey underwear out of the heroine's body textures.

MetaHuman Creator bakes a grey sports top and briefs into the body basecolor, normal and
SRMF maps. The primitive tank top is cropped shorter than that top and the low-rise shorts
sit below the briefs' waistband, so the grey showed at her ribs, neckline and hips.

Every underwear pixel is inpainted from the surrounding skin, including those under the
outfit: grey showed through the tank top's back scoop and beside its straps, and wherever a
pose pushes skin through a garment.

Usage (outside Unreal, Python 3 with numpy, Pillow and opencv-python):
    python Scripts/Characters/remove_body_underwear.py <export dir>
The export dir holds T_Body_BC_VT.tga, T_Body_N_VT.tga and T_Body_SRMF_VT.tga exported from
/Game/Characters/Heroine_MH/Assembled/Heroine/Body/Baked. Cleaned copies are written next to
them as *_Clean.tga for reimport over the originals.
"""
import sys
from pathlib import Path

import cv2
import numpy as np
from PIL import Image

Image.MAX_IMAGE_PIXELS = None
ROOT = Path(__file__).resolve().parents[2]
WORK = 1024


def underwear_mask(basecolor):
    """Grey (unsaturated) pixels are underwear; skin always has red well above blue."""
    rgb = basecolor.astype(np.int16)
    grey = (rgb[..., 0] - rgb[..., 2]) < 18
    grey = cv2.morphologyEx(grey.astype(np.uint8), cv2.MORPH_OPEN, np.ones((3, 3), np.uint8))
    # Grow over the darker binding lines and the anti-aliased rim.
    return cv2.dilate(grey, np.ones((9, 9), np.uint8)).astype(bool)


def removal_mask(size, grey):
    """Every underwear pixel goes, covered or not. The tank top's back scoop and narrow straps
    leave skin showing where the baked top was wider, and the coverage mask (fitted to the
    garment surface) can't see those gaps; skin also reads better than grey cloth wherever a
    pose pushes the body through a garment."""
    return grey.copy()


def fill(image, grey, remove, feather=6):
    """Inpaint every underwear pixel at low resolution (so the fill only sees skin), add back
    skin-scale detail borrowed from the belly, and composite it where the skin shows."""
    size = image.shape[0]
    small = cv2.resize(image, (WORK, WORK), interpolation=cv2.INTER_AREA)
    small_mask = cv2.resize(grey.astype(np.uint8) * 255, (WORK, WORK), interpolation=cv2.INTER_NEAREST)
    small_mask = cv2.dilate(small_mask, np.ones((3, 3), np.uint8))
    channels = small.shape[2]
    filled = np.dstack([cv2.inpaint(np.ascontiguousarray(small[..., c]), small_mask, 12, cv2.INPAINT_TELEA)
                        for c in range(channels)])
    filled = cv2.resize(filled, (size, size), interpolation=cv2.INTER_CUBIC).astype(np.float32)
    # Fine detail: high-pass of a clean belly patch, tiled over the image.
    top, left = int(size * 0.366), int(size * 0.42)
    patch = image[top:top + size // 36, left:left + size // 6].astype(np.float32)
    detail = patch - cv2.GaussianBlur(patch, (0, 0), size / 1024 * 6)
    reps = (size // detail.shape[0] + 1, size // detail.shape[1] + 1) + ((1,) if detail.ndim == 3 else ())
    filled += np.tile(detail, reps)[:size, :size]
    alpha = cv2.GaussianBlur(remove.astype(np.float32), (0, 0), feather * size / 4096)
    alpha = np.clip(alpha * 2, 0, 1)[..., None]
    out = image.astype(np.float32) * (1 - alpha) + filled * alpha
    return np.clip(out, 0, 255).astype(np.uint8)


def process(folder):
    basecolor = np.asarray(Image.open(folder / "T_Body_BC_VT.tga").convert("RGB"))
    grey = underwear_mask(basecolor)
    for name in ("T_Body_BC_VT", "T_Body_N_VT", "T_Body_SRMF_VT"):
        source = Image.open(folder / f"{name}.tga")
        image = np.asarray(source)
        size = image.shape[0]
        grey_n = cv2.resize(grey.astype(np.uint8), (size, size), interpolation=cv2.INTER_NEAREST).astype(bool)
        remove = removal_mask(size, grey_n)
        result = fill(image, grey_n, remove)
        Image.fromarray(result, source.mode).save(folder / f"{name}_Clean.tga")
        print(name, source.mode, size, f"{remove.mean() * 100:.2f}% repainted")


if __name__ == "__main__":
    process(Path(sys.argv[1]))
