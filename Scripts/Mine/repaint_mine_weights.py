"""Repaint the landscape's paint layers over the mine plain as open pasture.

Usage (from the repo root, editor running with -AllowPython):
    python Scripts\\Mine\\repaint_mine_weights.py            # writes the patched weightmaps to Saved/Ground/MineWeights
    pyfile Scripts\\Mine\\repaint_mine_weights.py            # in the editor: dry run, apply over RECT, save the touched proxies

Jenny (mine house review): "fix the brown dirt lines around the mine building still where the moat used to be, make it
green, level, and covered with grass like the plains". The lines were CliffRock (and a little Moorland) weight that
Scripts/Terrain/weightmaps.py painted on the steep banks of the old cut yard; regrade_mine_site.py --plain levelled the
ground, but the paint stayed. Within PLAIN_M of the house every layer but Pasture is cleared, blending back to the
existing weights by BLEND_M[1], so the rectangle's edges match the landscape around it.
"""
import os
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = Path(os.environ.get("HOMESTEAD_TERRAIN_WORK", r"E:\TerrainSource\work")) / "weights"
OUT = ROOT / "Saved" / "Ground" / "MineWeights"
LAYERS = ["Pasture", "WoodlandFloor", "Moorland", "DuneSand", "Beach", "CliffRock", "DirtRoad"]
RECT = (1550, 1240, 1662, 1348)    # vertex rectangle (X = 2016 + x m, Y = 2016 + y m): regrade_mine_site.py --plain's
BLEND_M = (34.0, 50.0)             # metres past the walls: all pasture inside the first, the old weights by the second
H = 2016


def write_maps():
    import numpy as np
    from PIL import Image
    sys.path.insert(0, str(ROOT / "Scripts" / "Blender" / "Recipes"))
    from mine import layout as L
    maps = {n: np.asarray(Image.open(SOURCE / f"{n}.png")).astype(np.float64) for n in LAYERS}
    x0, y0, x1, y1 = RECT
    Y, X = np.mgrid[y0:y1 + 1, x0:x1 + 1]
    hx, hy, _ = L.HOUSE_WORLD
    ox = np.abs(X - H - hx) - L.OUT_X / 2
    oy = np.abs(Y - H - hy) - L.OUT_Y / 2
    past = np.hypot(np.maximum(ox, 0), np.maximum(oy, 0))
    t = np.clip((past - BLEND_M[0]) / (BLEND_M[1] - BLEND_M[0]), 0, 1)
    t = t * t * (3 - 2 * t)
    win = (slice(y0, y1 + 1), slice(x0, x1 + 1))
    for n in LAYERS:
        target = 255.0 if n == "Pasture" else 0.0
        maps[n][win] = maps[n][win] * t + target * (1 - t)
    # Keep the sum at 255 after rounding: Pasture takes the remainder.
    rounded = {n: np.rint(maps[n]).clip(0, 255) for n in LAYERS}
    rest = sum(rounded[n] for n in LAYERS if n != "Pasture")
    rounded["Pasture"][win] = (255 - rest[win]).clip(0, 255)
    OUT.mkdir(parents=True, exist_ok=True)
    for n in LAYERS:
        Image.fromarray(rounded[n].astype(np.uint8)).save(OUT / f"{n}.png")
    print(f"wrote {len(LAYERS)} weightmaps to {OUT}")


def apply_in_editor():
    import unreal
    lib = unreal.HomesteadEstateAuthoringLibrary
    names = [unreal.Name(n) for n in LAYERS]
    x0, y0, x1, y1 = RECT
    print("before:", lib.apply_estate_weightmaps(str(OUT), names, x0, y0, x1, y1, 63, True))
    print("apply:", lib.apply_estate_weightmaps(str(OUT), names, x0, y0, x1, y1, 63, False))
    print("after:", lib.apply_estate_weightmaps(str(OUT), names, x0, y0, x1, y1, 63, True))
    # The edit layer holds the new weights, but the rendered weightmaps only update when the landscape merges its
    # layers; saving straight away kept the old paint in PIE. Force the merge, then save.
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Landscape)[0].force_layers_full_update()
    # Save only the landscape proxies over the rectangle (never blanket-save: preview actors would be saved too).
    lo = unreal.Vector((x0 - H) * 100.0, (y0 - H) * 100.0, -1e6)
    hi = unreal.Vector((x1 - H) * 100.0, (y1 - H) * 100.0, 1e6)
    pkgs = []
    for p in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.LandscapeStreamingProxy):
        o, e = p.get_actor_bounds(False)
        if o.x + e.x >= lo.x and o.x - e.x <= hi.x and o.y + e.y >= lo.y and o.y - e.y <= hi.y:
            pkgs.append(p.get_outermost())
    print("saved", unreal.EditorLoadingAndSavingUtils.save_packages(pkgs, False), [str(p.get_name()) for p in pkgs])


if __name__ == "__main__":
    try:
        import unreal  # noqa: F401
        apply_in_editor()
    except ImportError:
        write_maps()
