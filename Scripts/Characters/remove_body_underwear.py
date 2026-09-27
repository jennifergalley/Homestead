"""Paint the MetaHuman's baked grey underwear out of the heroine's body textures.

MetaHuman Creator bakes a grey sports top and briefs into the body basecolor, normal and
SRMF maps. The primitive tank top is cropped shorter than that top and the low-rise shorts
sit below the briefs' waistband, so the grey showed at her ribs, neckline and hips.

Every underwear pixel is inpainted from the surrounding skin, including those under the
outfit: grey showed through the tank top's back scoop and beside its straps, and wherever a
pose pushes skin through a garment.

Usage (outside Unreal, Python 3 with numpy, Pillow and opencv-python):
    python Scripts/Characters/remove_body_underwear.py <export dir>
The export dir holds T_Body_BC_VT.tga, T_Body_N_VT.tga, T_Body_SRMF_VT.tga and (optionally)
T_Body_Scatter_VT.tga exported from
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


def fill(image, grey, remove, feather=6, patch_at=(0.366, 0.42)):
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
    # Fine detail: high-pass of a clean skin patch (the belly, or the throat on the head),
    # tiled over the image.
    top, left = int(size * patch_at[0]), int(size * patch_at[1])
    patch = image[top:top + size // 36, left:left + size // 6].astype(np.float32)
    detail = patch - cv2.GaussianBlur(patch, (0, 0), size / 1024 * 6)
    reps = (size // detail.shape[0] + 1, size // detail.shape[1] + 1) + ((1,) if detail.ndim == 3 else ())
    filled += np.tile(detail, reps)[:size, :size]
    alpha = cv2.GaussianBlur(remove.astype(np.float32), (0, 0), feather * size / 4096)
    alpha = np.clip(alpha * 2, 0, 1)[..., None]
    out = image.astype(np.float32) * (1 - alpha) + filled * alpha
    return np.clip(out, 0, 255).astype(np.uint8)


def clean_scatter(folder, name="T_Body_Scatter_VT", rows=None):
    """The scatter map paints the underwear black (no subsurface), which left blue-grey skin
    where the top's straps crossed her shoulders. Skin scatter is nearly uniform, so the black
    is inpainted from the surrounding skin."""
    path = folder / f"{name}.tga"
    if not path.exists():
        return
    source = Image.open(path)
    image = np.asarray(source.convert("RGB"))
    dark = (image[..., 0] < 90).astype(np.uint8)
    if rows is not None:
        dark[: int(dark.shape[0] * rows)] = 0
    dark = cv2.dilate(dark, np.ones((5, 5), np.uint8))
    filled = np.dstack([cv2.inpaint(np.ascontiguousarray(image[..., c]), dark, 8, cv2.INPAINT_TELEA)
                        for c in range(3)])
    Image.fromarray(filled, "RGB").save(folder / f"{name}_Clean.tga")
    print(name, f"{dark.mean() * 100:.2f}% repainted")


def process(folder):
    clean_scatter(folder)
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


# The head mesh runs down the neck onto her shoulders, and its bake carries the top's straps
# along the bottom edge of the head UVs. Only that band is searched, so the eyes, brows and lips
# (also low in saturation) are never touched.
FACE_SETS = (
    ("T_Head_LOD1_BC_VT", ("T_Head_LOD1_BC_VT", "T_Head_LOD1_N_VT", "T_Head_LOD1_SRMF_VT"), "T_Head_LOD1_Scatter_VT"),
    ("T_Head_LOD3_BC_VT", ("T_Head_LOD3_BC_VT", "T_Head_LOD3_N_VT", "T_Head_LOD3_SRMF_VT"), "T_Head_LOD3_Scatter_VT"),
    ("T_Head_LOD5to7_BC_VT", ("T_Head_LOD5to7_BC_VT", "T_Head_LOD5to7_N_VT", "T_Head_LOD5to7_SRMF"), "T_Head_LOD5to7_Scatter_VT"),
)
FACE_ROWS = 0.86


def process_face(folder):
    for basecolor_name, names, scatter in FACE_SETS:
        if not (folder / f"{basecolor_name}.tga").exists():
            continue
        clean_scatter(folder, scatter, FACE_ROWS)
        basecolor = np.asarray(Image.open(folder / f"{basecolor_name}.tga").convert("RGB"))
        grey = underwear_mask(basecolor)
        grey[: int(grey.shape[0] * FACE_ROWS)] = False
        for name in names:
            source = Image.open(folder / f"{name}.tga")
            image = np.asarray(source)
            size = image.shape[0]
            grey_n = cv2.resize(grey.astype(np.uint8), (size, size), interpolation=cv2.INTER_NEAREST).astype(bool)
            result = fill(image, grey_n, grey_n, patch_at=(0.83, 0.25))
            Image.fromarray(result, source.mode).save(folder / f"{name}_Clean.tga")
            print(name, source.mode, size, f"{grey_n.mean() * 100:.2f}% repainted")


if __name__ == "__main__":
    # Head textures: python remove_body_underwear.py <export dir> --face
    if len(sys.argv) > 2 and sys.argv[2] == "--face":
        process_face(Path(sys.argv[1]))
    else:
        process(Path(sys.argv[1]))
