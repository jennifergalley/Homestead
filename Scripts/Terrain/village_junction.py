"""Move the village street's junction up the main road (Balance, balance.md section 9: the street was 21% at its
kink; Jenny scheduled it 2026-10-10). Takes the old street's grade (village-path-and-dressing, a073dc0ea) back out of
the heightfield so the hillside is whole again before town_layout.py lays the new street and town_path.py grades it.

Usage: python Scripts/Terrain/village_junction.py      (once; "villageJunction" in the layout; then the village chain:
       town_layout.py, town_path.py, town_layout.py, public_road.py, weightmaps.py, route_sights.py --top-up,
       village_dress.py, bake_ground.py, bake_estate_map.py)

Writes EstateHeightfield.r16, Estate_Heightmap_4033.png and the work npy where they changed, and removes "townPath"
from estate_layout.json so town_path.py grades the new street.
"""
import json
import os
import subprocess

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, "..", ".."))
WORK = os.environ.get("HOMESTEAD_TERRAIN_WORK", r"E:\TerrainSource\work")
R16 = os.path.join(REPO, "Content", "SurvivalGame", "Estate", "Runtime", "EstateHeightfield.r16")
R16_GIT = "Content/SurvivalGame/Estate/Runtime/EstateHeightfield.r16"
PNG = os.path.join(HERE, "Estate_Heightmap_4033.png")
LAYOUT = os.path.join(HERE, "estate_layout.json")
SIZE, H = 4033, 2016
OLD_STREET = ("a073dc0ea^", "a073dc0ea")     # the commit that graded the old street
OLD_STREET_BOX = (-110.0, -20.0, -470.0, -330.0)   # x0, x1, y0, y1 (m): only its corridor


def git_r16(rev):
    pointer = subprocess.run(["git", "show", f"{rev}:{R16_GIT}"], cwd=REPO, capture_output=True, check=True).stdout
    data = subprocess.run(["git", "lfs", "smudge"], cwd=REPO, input=pointer, capture_output=True, check=True).stdout
    return (np.frombuffer(data, "<u2").reshape(SIZE, SIZE).astype(np.float64) - 32768.0) / 128.0


def main():
    layout = json.load(open(LAYOUT))
    if layout.get("villageJunction"):
        print("village junction: already moved")
        return
    raw = np.fromfile(R16, "<u2").reshape(SIZE, SIZE)
    z = (raw.astype(np.float64) - 32768.0) / 128.0
    delta = git_r16(OLD_STREET[1]) - git_r16(OLD_STREET[0])
    x0, x1, y0, y1 = OLD_STREET_BOX
    box = np.zeros_like(delta, bool)
    box[int(y0) + H:int(y1) + H + 1, int(x0) + H:int(x1) + H + 1] = True
    z = np.where(box, z - delta, z)
    u16 = np.clip(np.round(32768 + z * 128.0), 0, 65535).astype(np.uint16)
    moved = np.argwhere(u16 != raw)
    u16.astype("<u2").tofile(R16)
    Image.fromarray(u16).save(PNG)
    npy = os.path.join(WORK, "game_reshaped_4033.npy")
    if os.path.exists(npy) and len(moved):
        zr = (u16.astype(np.float64) - 32768.0) / 128.0
        zn = np.load(npy)
        r, c = moved[:, 0], moved[:, 1]
        zn[2 * H - c, r] = zr[r, c].astype(zn.dtype)
        np.save(npy, zn)
    layout.pop("townPath", None)
    layout["villageJunction"] = True
    with open(LAYOUT, "w") as fh:
        json.dump(layout, fh, indent=1)
    if len(moved):
        (r0, c0), (r1, c1) = moved.min(0), moved.max(0)
        print(f"village junction: the old street's grade taken out of {len(moved)} vertices; "
              f"ApplyEstateHeightfield rows {r0}-{r1}, cols {c0}-{c1}")


if __name__ == "__main__":
    main()
