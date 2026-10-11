"""Beach shells: a gatherable strandline scatter and the single cockle she lifts.

Real-object research (written before modeling), for a granite-coast English beach, 1851:
- Common cockle (Cerastoderma edule): valves 3-4.5 cm long, height ~0.9 L, strongly inflated
  (one valve ~0.35-0.4 L deep), 22-28 radial ribs carrying small scales near the margin, the
  margin crenulated where the ribs end. Dirty cream to buff with brown growth bands; beach
  shells are often grey-stained from anoxic sand. Inside porcellaneous white-cream, a brown
  posterior blotch, the rib grooves showing only near the margin. Walls ~1.5 mm.
- Common limpet (Patella vulgata): oval base 3.5-5.5 cm (width ~0.83 L), conical, height
  0.4-0.5 L, apex a little anterior, ~30-40 coarse irregular ridges crenulating the margin.
  Chalky grey outside with dark grooves and an eroded, rusty apex; inside glossy grey-green
  with a cream/orange head scar and a horseshoe muscle scar.
- Edible periwinkle (Littorina littorea): 2-3 cm tall, 5-7 whorls with a pointed spire, no
  umbilicus, dark grey-brown with fine dark spiral lines, worn pale apex, chocolate aperture
  with a white lip.
- Blue mussel (Mytilus edulis) half-shells: 4.5-7 cm, terminal pointed umbo, straight ventral
  margin, rounded posterior; glossy blue-black periostracum (brownish young growth), eroded
  violet umbo; inside pearly blue-white nacre with a dark violet margin band. Walls ~0.8 mm.
- Pod razor (Ensis siliqua): single valve 12-20 cm x ~0.15 L, straight, squared rounded
  ends, varnish-glossy olive-brown periostracum flaking at the margin, a diagonal line
  setting off the dorsal-anterior triangle; inside white with a violet flush.
- On the strand, empty valves lie mostly concave-down (stable), some concave-up holding
  sand; winkles lie on their sides; razors half-bury along their length.

Meshes
- SM_BeachShells (+ _LOD1): nine shells in a ~28 cm natural scatter, one mesh. Pivot at the
  sand line (z = 0) under the scatter's bounds centre; shells are bedded 1-5 mm below it.
  Gatherable (the whole mesh disappears when picked); no collision needed.
- SM_SeashellHeld: one 3.5 cm cockle valve with modelled ribs. PIVOT = the pinch point on
  its dorsal (hinge/umbo) margin, mid-wall, 4 mm below the top edge. Local +Z points up out of
  her pinch and the valve hangs below in -Z (the crop-produce convention of
  HomesteadCharacterGather.cpp, MakeFromZX(-Fingers, Across)); the valve plane is XZ so her
  thumb and finger close along Y (outside of the dome toward -Y in Blender, +Y in Unreal).

Everything is generated here: no scanned or downloaded geometry or textures.
"""
import math
import os

import numpy as np
from mathutils import Euler, Matrix, Vector

import homestead_rocks as rocks
import homestead_shore as shore

NAME = "BeachShells"
DESCRIPTION = ("Gatherable strandline shell scatter (cockles, limpets, periwinkles, mussel and razor "
               "valves; ~28 cm) and a held cockle valve (pinch pivot).")
COLLISION = "none"
TRIANGLE_BUDGET = 6000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
DRAFT = os.environ.get("HOMESTEAD_SHORE_DRAFT") == "1"
BAKE = {"size": 512 if DRAFT else 2048, "samples": 16 if DRAFT else 96, "repack": False}
BAKE_MESHES = {"SM_SeashellHeld": {"size": 512 if DRAFT else 1024}, "*": True}
BEAUTY = {"pose": (0, 0, 0), "ground": "origin", "focus": (-0.06, -0.04, 0.008), "detail_distance": 0.3,
          "meshes": {"SM_SeashellHeld": {"pose": (-90, 0, 0), "ground": "lowest", "focus": None,
                                         "detail_distance": 0.14, "detail_fstop": 32.0}}}
REPORT = {}

# kind, size (m), seed, concave-up, (x, y) m, yaw, tilt (rx, ry), sink (m)
SHELLS = [
    ("cockle", 0.034, 1, False, (-0.03, -0.062), 20, (4, -3), 0.0015),
    ("cockle", 0.029, 2, True, (0.042, -0.066), 140, (-6, 5), 0.003),
    ("limpet", 0.043, 3, False, (-0.072, -0.004), -30, (3, 2), 0.0015),
    ("limpet", 0.036, 4, True, (0.088, -0.022), 75, (-42, -5), 0.0025),
    ("winkle", 0.025, 5, False, (-0.118, -0.042), 35, (90, 18), 0.003),
    ("winkle", 0.019, 6, False, (0.082, 0.030), 200, (75, -25), 0.0025),
    ("mussel", 0.057, 7, True, (0.015, -0.006), -55, (5, 6), 0.002),
    ("mussel", 0.048, 8, False, (-0.105, 0.046), 110, (-3, 2), 0.0015),
    ("razor", 0.128, 9, False, (0.0, 0.052), 15, (2, -4), 0.0018),
]


def materials(kit):
    return {
        "cockle": shore.cockle_mat("M_ShellCockle", seed=1.3),
        "limpet": shore.limpet_mat("M_ShellLimpet", seed=2.1),
        "winkle": shore.winkle_mat("M_ShellWinkle", seed=0.7),
        "mussel": shore.mussel_mat("M_ShellMussel", seed=3.3, worn=0.7),
        "razor": shore.razor_mat("M_ShellRazor", seed=0.4),
    }


def make(kind, name, size, seed, mats):
    if kind == "cockle":
        return shore.cockle(name, size, mats["cockle"], seed)
    if kind == "limpet":
        return shore.limpet(name, size, mats["limpet"], seed)
    if kind == "winkle":
        return shore.winkle(name, size, mats["winkle"], seed)
    if kind == "mussel":
        return shore.mussel_valve(name, size, mats["mussel"], seed)
    return shore.razor(name, size, mats["razor"], seed)


def place(obj, flip, position, yaw, tilt, sink):
    P = shore.coords(obj)
    P -= P.mean(axis=0) * np.array([1, 1, 0])
    shore.set_coords(obj, P)
    rot = Euler((math.radians(tilt[0]) + (math.pi if flip else 0.0), math.radians(tilt[1]), math.radians(yaw)),
                "XYZ").to_matrix().to_4x4()
    shore.transform(obj, rot)
    P = shore.coords(obj)
    P += np.array([position[0], position[1], -sink - P[:, 2].min()])
    shore.set_coords(obj, P)
    inner = shore.get_attr(obj.data, "sh_in")
    z = P[:, 2]
    sand = 0.8 * shore.smooth(0.0008, -0.002, z)
    if flip and (inner > 0.5).any():
        zin = z[inner > 0.5].min()
        sand = np.maximum(sand, 0.9 * inner * shore.smooth(zin + 0.0016, zin + 0.0003, z))
    shore.set_attr(obj.data, "sand", sand)
    return obj


def held_cockle(kit):
    mat = shore.cockle_mat("M_HeldCockle", seed=4.2, stain=0.25)
    obj = shore.cockle("SM_SeashellHeld", 0.035, mat, seed=12, detail="high")
    shore.transform(obj, Matrix.Rotation(math.radians(90), 4, "X"))   # dome -> -Y, dorsal -> +Z
    P = shore.coords(obj)
    top = P[np.argmax(P[:, 2])]
    pinch_z = top[2] - 0.004
    tree, bm = shore.bvh_of([obj])
    hits = []
    origin = Vector((top[0], -0.1, pinch_z))
    for _ in range(4):
        hit = tree.ray_cast(origin, Vector((0, 1, 0)))
        if hit[0] is None:
            break
        hits.append(hit[0].y)
        origin = hit[0] + Vector((0, 1e-5, 0))
    bm.free()
    y_mid = 0.5 * (hits[0] + hits[1]) if len(hits) >= 2 else 0.0
    pivot = np.array([top[0], y_mid, pinch_z])
    shore.set_coords(obj, P - pivot)
    shore.set_attr(obj.data, "sand", np.zeros(len(P)))
    REPORT["held"] = {
        "SM_SeashellHeld": "Pivot = pinch point on the dorsal (umbo/hinge) margin, mid-wall, 4 mm below the top "
                           "edge. +Z up out of the pinch, valve hangs in -Z; valve plane XZ, thumb/finger close "
                           "along Y (dome toward -Y in Blender = +Y in Unreal). Length 3.5 cm."}
    return kit.join([obj], "SM_SeashellHeld", pivot=None, unwrap=False, reshade=True, smooth_angle=85)


def build(kit):
    mats = materials(kit)
    parts = []
    for index, (kind, size, seed, flip, pos, yaw, tilt, sink) in enumerate(SHELLS):
        obj = make(kind, f"Shell{index}", size, seed, mats)
        parts.append(place(obj, flip, pos, yaw, tilt, sink))
    allp = np.vstack([shore.coords(o) for o in parts])
    centre = (allp.min(axis=0) + allp.max(axis=0)) / 2
    for obj in parts:
        shore.set_coords(obj, shore.coords(obj) - np.array([centre[0], centre[1], 0.0]))
    scatter = kit.join(parts, "SM_BeachShells", pivot=None, unwrap=False, reshade=True, smooth_angle=85)
    kit.pack_uvs(scatter, margin=0.006)
    lod1 = kit.finalize(rocks.lod(kit, scatter, "SM_BeachShells_LOD1", 0.45), pivot=None, unwrap=False,
                        reshade=True, smooth_angle=85)
    held = held_cockle(kit)
    shore.unwrap(held, angle=75.0, margin=0.008)
    REPORT["scatter"] = {"pivot": "Sand line (z = 0) under the scatter's bounds centre; shells bedded 1-5 mm "
                                  "below it, so place at sand height.",
                         "shells": [f"{k} {s * 100:.1f} cm{' concave-up' if f else ''}"
                                    for k, s, _, f, *_ in SHELLS],
                         "collision": "none (gatherable pickup)"}
    return [scatter, lod1, held]
