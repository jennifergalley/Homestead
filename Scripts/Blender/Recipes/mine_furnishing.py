"""Mine house furnishing: the desk, its chair, a ledger and inkwell, the rock pile in the corner, and
the trapdoor's lid (Jenny's card: "rocks piled in a corner, a desk, an oil lamp on the desk ... and a
trapdoor in the floor").

    .\\Scripts\\Blender\\New-Prop.ps1 mine_furnishing -BeautySamples 64

Real-object research:
- The desk is a plain deal table (Jenny): count houses often wrote at a scrubbed kitchen-style table,
  about 1.4 m by 0.8 m and 0.78 m high, turned legs, a shallow apron, the top worn pale where ledgers rubbed.
- The chair a kitchen or Windsor-style chair: elm seat, turned beech legs, spindle back.
- Ledgers: tall quarter-bound books, marbled boards, leather spine and corners; a stoneware ink bottle.
- The rock pile: broken granite from the level, angular blocks and spalls with fresh pale fracture
  faces, the bigger pieces low, heaped against the corner at its natural angle of repose (~35-40 deg).
- The trapdoor: ledged planks 3 cm thick, two strap hinges on top, a drop ring at the free edge.

Pivots (Unreal frame): each piece stands on the floor at its own centre; placed by place_mine.py
from ``layout.dressing()``. The lid's pivot is the hinge line's centre, closed lid lying along +X with
its top at z = 0 (Mine Gameplay rotates it about the hinge). No scanned or downloaded content.
"""
import math
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import homestead_rocks as rocks  # noqa: E402
from mine import layout as L  # noqa: E402

NAME = "MineFurnishing"
DESCRIPTION = "Mine house furnishing: plain table desk, chair, ledger and inkwell, rock pile, trapdoor lid."
COLLISION = "convex"
TRIANGLE_BUDGET = 500000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 48}
# The lid and its collar take the floor's plank photoscan in Unreal over metric UVs (plank_uvs); no bake.
BAKE_MESHES = {"SM_MineTrapdoorLid": False, "SM_MineTrapdoorCollar": False, "*": True}
BEAUTY = {"pose": (0, 0, 0), "ground": "origin", "views": ["hero", "detail"],
          "meshes": {"SM_MineDesk": {"focus": (0.0, 0.0, 0.6), "eye_distance": 3.2},
                     "SM_MineChair": {"focus": (0.0, 0.0, 0.5), "eye_distance": 2.4},
                     "SM_MineLedger": {"focus": (0.0, 0.0, 0.03), "eye_distance": 1.0},
                     "SM_MineRockPile": {"focus": (0.0, 0.0, 0.4), "eye_distance": 3.5},
                     "SM_MineTrapdoorLid": {"focus": (0.6, 0.0, 0.0), "eye_distance": 2.5}}}
REPORT = {"pivot": "Each piece on the floor at its own centre (lid: hinge-line centre, closed along +X)."}


def U(x, y, z):
    return (x, -y, z)


def desk(kit, wood):
    """A plain deal table used as the count-house desk (Jenny: "more of a table, just a flat surface, no
    drawers underneath"): a two-board top with a thumbnail edge on four turned legs joined by a shallow
    apron. Long side along Y."""
    sx, sy, sz = 0.80, 1.40, 0.78
    top_t, apron_h, inset = 0.035, 0.09, 0.07
    parts = []
    for side in (-1, 1):   # two boards, a hairline joint down the middle
        parts.append(kit.box(f"TopBoard{side}", (sx / 2 - 0.002, sy, top_t), U(side * sx / 4, 0.0, sz - top_t / 2),
                             material=wood, bevel=0.008))
    lx, ly = sx / 2 - inset, sy / 2 - inset
    for ax in (-1, 1):
        for ay in (-1, 1):
            pts = [U(ax * lx, ay * ly, z) for z in np.linspace(0.0, sz - top_t, 13)]
            prof = np.linspace(0, 1, 13)
            radii = [0.026 - 0.006 * t + 0.006 * math.exp(-((t - 0.78) / 0.05) ** 2) + 0.004 * math.exp(-((t - 0.15) / 0.04) ** 2)
                     for t in prof]
            radii[-3:] = [0.03, 0.03, 0.03]   # square-ish block under the apron
            parts.append(kit.tube(f"Leg{ax}{ay}", pts, radii=radii, sides=16, material=wood))
    z = sz - top_t - apron_h / 2
    for side in (-1, 1):
        parts.append(kit.box(f"ApronLong{side}", (0.022, 2 * ly - 0.04, apron_h), U(side * lx, 0.0, z), material=wood,
                             bevel=0.004))
        parts.append(kit.box(f"ApronEnd{side}", (2 * lx - 0.04, 0.022, apron_h), U(0.0, side * ly, z), material=wood,
                             bevel=0.004))
    return parts


def chair(kit, elm, beech):
    parts = []
    seat_z = 0.45
    parts.append(kit.box("Seat", (0.42, 0.40, 0.035), U(0.0, 0.0, seat_z), material=elm, bevel=0.012))
    for sx in (-1, 1):
        for sy in (-1, 1):
            top = np.array([sx * 0.16, sy * 0.15, seat_z - 0.02])
            foot = np.array([sx * 0.2, sy * 0.19, 0.0])
            pts = [U(*(foot + (top - foot) * t)) for t in np.linspace(0, 1, 9)]
            radii = [0.017 + 0.006 * math.sin(math.pi * t) ** 2 for t in np.linspace(0, 1, 9)]
            parts.append(kit.tube(f"Leg{sx}{sy}", pts, radii=radii, sides=12, material=beech))
    for sy in (-1, 1):
        parts.append(kit.tube(f"Stretcher{sy}", [U(-0.18, sy * 0.17, 0.17), U(0.18, sy * 0.17, 0.17)], radius=0.011,
                              sides=10, material=beech))
    # Spindle back on the -X side, a bow rail over it.
    for k in range(5):
        y = -0.13 + k * 0.065
        parts.append(kit.tube(f"Spindle{k}", [U(-0.18, y, seat_z), U(-0.21, y * 1.08, seat_z + 0.42)], radius=0.0085,
                              sides=10, material=beech))
    bow = [U(-0.205, 0.17 * math.cos(t), seat_z + 0.40 + 0.06 * math.sin(t)) for t in np.linspace(0, math.pi, 13)]
    parts.append(kit.tube("Bow", [(p[0], p[1], p[2]) for p in bow], radius=0.014, sides=10, material=elm))
    for side in (-1, 1):
        parts.append(kit.tube(f"BowFoot{side}", [U(-0.18, side * 0.17, seat_z), U(-0.205, side * 0.17, seat_z + 0.40)],
                              radius=0.013, sides=10, material=elm))
    return parts


def ledger(kit, leather, marbled, paper, stoneware):
    parts = []
    w, l, t = 0.30, 0.42, 0.045
    parts.append(kit.box("Pages", (w - 0.012, l - 0.012, t - 0.008), U(0.0, 0.0, t / 2), material=paper, bevel=0.002))
    for z in (0.002, t - 0.002):
        parts.append(kit.box("Board", (w, l, 0.004), U(0.0, 0.0, z), material=marbled, bevel=0.001))
    parts.append(kit.box("Spine", (0.012, l, t + 0.004), U(-w / 2, 0.0, t / 2), material=leather, bevel=0.004))
    for sy in (-1, 1):
        parts.append(kit.box("Corner", (0.05, 0.05, t + 0.005), U(w / 2 - 0.022, sy * (l / 2 - 0.022), t / 2), material=leather,
                             bevel=0.002))
    # Ink bottle beside it.
    parts.append(kit.cylinder("InkBottle", 0.03, 0.07, U(0.05, l / 2 + 0.08, 0.035), material=stoneware, sides=18,
                              bevel=0.006))
    parts.append(kit.cylinder("InkNeck", 0.012, 0.02, U(0.05, l / 2 + 0.08, 0.08), material=stoneware, sides=12))
    return parts


def rock_pile(kit, granite, rng):
    """Broken granite heaped against a corner (the corner at -X, +Y of the pile's frame)."""
    R, H = L.ROCK_PILE["radius"], L.ROCK_PILE["height"]
    stones = []
    for i in range(46):
        r = R * math.sqrt(rng.uniform(0.0, 1.0))
        a = rng.uniform(math.pi * 0.5, math.pi * 1.5) if rng.random() < 0.8 else rng.uniform(0, 2 * math.pi)
        x, y = r * math.cos(a) * 0.9 + R * 0.25, -r * math.sin(a) * 0.9 - R * 0.25
        size = rng.uniform(0.09, 0.22) * (1.25 - 0.6 * r / R)
        h = H * max(0.0, 1.0 - math.hypot(x - R * 0.3, y + R * 0.3) / (R * 1.45))
        z = max(size * 0.5, h * rng.uniform(0.55, 1.0) - size * 0.3)
        stones.append({"radii": (size * rng.uniform(1.0, 1.5), size * rng.uniform(0.75, 1.1), size * rng.uniform(0.55, 0.8)),
                       "power": rng.uniform(2.4, 3.2), "lumps": 0.02, "seed": 5000 + i, "center": (x, -y, z),
                       "rotate": (rng.uniform(0, 360), rng.uniform(-25, 25), rng.uniform(-20, 20)),
                       # Broken, not weathered: four to six fracture planes each, sharp-edged.
                       "joints": [(tuple(rng.normal(size=3)), size * rng.uniform(0.45, 0.75), 0.006)
                                  for _ in range(rng.integers(4, 7))],
                       "subdivisions": 5})
    pieces = rocks.scatter("PileStone", stones, granite)
    meshes, _ = rocks.finish(kit, pieces, "SM_MineRockPile", 220000, lod_tris=(60000, 15000), ground_z=0.0,
                             detail=lambda p, n: rocks.relief(p, n, 61, [(0.08, 0.006), (0.02, 0.0015)]), union=False)
    return meshes


def trapdoor_lid(kit, wood, iron):
    """Closed along +X from the hinge (x = 0) with its top at z = 0."""
    lap = L.TRAPDOOR_LAP
    lx, ly, t = L.TRAPDOOR["size"][1] + 2 * lap, L.TRAPDOOR["size"][0] + 2 * lap, 0.032
    parts = []
    y = -ly / 2
    k = 0
    while y < ly / 2 - 1e-6:
        w = min(0.2, ly / 2 - y)
        parts.append(kit.box(f"Plank{k}", (lx, w - 0.004, t), U(lx / 2, y + w / 2, -t / 2), material=wood, bevel=0.004))
        y += w
        k += 1
    for x in (0.18 + lap, lx - 0.18 - lap):     # battens clear of the rebate
        parts.append(kit.box(f"Ledge{x}", (0.12, ly - 2 * lap - 0.08, 0.03), U(x, 0.0, -t - 0.015), material=wood,
                             bevel=0.004))
    # Long dark strap hinges and a big drop ring, so the hatch reads at a glance (Balance, 2026-10-11).
    for sy in (-0.40, 0.40):
        parts.append(kit.box(f"Hinge{sy}", (0.78, 0.065, 0.008), U(0.39, sy, 0.004), material=iron, bevel=0.002))
        parts.append(kit.cylinder(f"Knuckle{sy}", 0.017, 0.12, U(0.0, sy, 0.0), rotation=(90, 0, 0), material=iron, sides=12))
    parts.append(kit.box("RingPlate", (0.10, 0.12, 0.006), U(lx - 0.14, 0.0, 0.003), material=iron, bevel=0.002))
    ring = [U(lx - 0.14 + 0.085 * math.sin(a) + 0.085, 0.085 * math.cos(a), 0.012)
            for a in np.linspace(0, 2 * math.pi, 29)]
    parts.append(kit.tube("Ring", ring, radius=0.011, sides=10, material=iron))
    return parts


COLLAR = {"kerb_w": 0.14, "kerb_h": 0.14, "post": 0.12, "post_h": 0.48, "gap": 0.01}


def trapdoor_collar(kit, wood):
    """A timber kerb round the hatch with short corner posts, as at a shaft head (Jenny, review 7: "add a frame
    of posts/timbers around its edges so the hatch reads clearly"). Centred on the opening, on the floor."""
    half = L.TRAPDOOR["size"][0] / 2 + L.TRAPDOOR_LAP + COLLAR["gap"]
    w, h, post, ph = COLLAR["kerb_w"], COLLAR["kerb_h"], COLLAR["post"], COLLAR["post_h"]
    outer = half + w
    parts = []
    for s in (-1, 1):
        # Kerbs along X run the full width; kerbs along Y sit between them (halved over at the corners).
        parts.append(kit.box(f"KerbX{s}", (2 * outer, w, h), U(0.0, s * (half + w / 2), h / 2), material=wood,
                             bevel=0.008))
        parts.append(kit.box(f"KerbY{s}", (w, 2 * half, h * 0.92), U(s * (half + w / 2), 0.0, h * 0.46),
                             material=wood, bevel=0.008))
    for sx in (-1, 1):
        for sy in (-1, 1):
            c = half + w / 2
            parts.append(kit.box(f"Post{sx}{sy}", (post, post, ph), U(sx * c, sy * c, ph / 2), material=wood,
                                 bevel=0.012))
            parts.append(kit.box(f"Cap{sx}{sy}", (post + 0.02, post + 0.02, 0.03), U(sx * c, sy * c, ph + 0.012),
                                 material=wood, bevel=0.008))
    return parts


def plank_uvs(obj, seed=3):
    """UVs in metres, one layout per loose part (plank, ledge, kerb, post): each face is projected along its
    dominant axis with U along the part's longest in-plane extent (the grain runs along the timber), and each
    part gets its own random patch of the photograph, as the floorboards do (mine_house.photo_uvs)."""
    import bmesh
    rng = np.random.default_rng(seed)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    layer = bm.loops.layers.uv.active or bm.loops.layers.uv.new("UVMap")
    seen = set()
    for face in bm.faces:
        if face.index in seen:
            continue
        part, stack = [], [face]
        seen.add(face.index)
        while stack:
            f = stack.pop()
            part.append(f)
            for e in f.edges:
                for o in e.link_faces:
                    if o.index not in seen:
                        seen.add(o.index)
                        stack.append(o)
        pts = np.array([v.co[:] for f in part for v in f.verts])
        pts[:, 1] *= -1.0
        extent = pts.max(axis=0) - pts.min(axis=0)
        shift = rng.uniform(0.0, 4.0, 3)
        for f in part:
            n = np.abs(np.array(f.normal[:]))
            axis = int(np.argmax(n))
            plane = [a for a in range(3) if a != axis]
            u, v = (plane if extent[plane[0]] >= extent[plane[1]] else plane[::-1])
            for loop in f.loops:
                c = np.array(loop.vert.co[:])
                c[1] *= -1.0
                c = c + shift
                loop[layer].uv = (float(c[u]), float(c[v]))
    bm.to_mesh(obj.data)
    bm.free()
    return obj


def build(kit):
    rng = np.random.default_rng(77)
    pine = kit.mats.aged_wood("M_MineDeskPine", light=(0.24, 0.15, 0.08), dark=(0.08, 0.045, 0.025), roughness=0.62,
                              saw=0.05, grime=0.35, seed=4.0)
    paper = kit.mats.paper("M_MinePaper", color=(0.50, 0.44, 0.32), seed=2.0)
    elm = kit.mats.wood("M_MineChairElm", light=(0.30, 0.20, 0.12), dark=(0.12, 0.075, 0.04), polish=0.3, seed=3.0)
    beech = kit.mats.wood("M_MineChairBeech", light=(0.34, 0.24, 0.15), dark=(0.15, 0.10, 0.06), seed=6.0)
    leather = kit.mats.leather("M_MineLedgerLeather", color=(0.18, 0.08, 0.04), dark=(0.07, 0.03, 0.015))
    marbled = kit.mats.painted_wood("M_MineLedgerBoards", paint=(0.09, 0.07, 0.10), under=(0.20, 0.14, 0.09), wear=0.6,
                                    seed=5.0)
    stoneware = kit.mats.stoneware("M_MineInkBottle", glaze=(0.20, 0.14, 0.08), seed=1.0)
    granite = kit.mats.granite("M_MineRockPile", grain=0.004, scale=0.6, patina=0.2, lichen=0.0, moss=0.0, iron=0.35,
                               streaks=0.0, soil=0.15, soil_height=0.05, enclaves=0.4, film=0.2, seed=13.0)
    lid_wood = kit.mats.aged_wood("M_MineTrapdoorWood", light=(0.22, 0.16, 0.10), dark=(0.07, 0.05, 0.03), saw=0.3,
                                  grime=0.7, seed=9.0)
    iron = kit.mats.wrought_iron("M_MineTrapdoorIron", rust=0.5, wear=0.45, seed=2.0)
    out = [kit.join(desk(kit, pine), "SM_MineDesk", pivot="base"),
           kit.join(chair(kit, elm, beech), "SM_MineChair", pivot="base"),
           kit.join(ledger(kit, leather, marbled, paper, stoneware), "SM_MineLedger", pivot="base")]
    out += rock_pile(kit, granite, rng)
    out.append(plank_uvs(kit.join(trapdoor_lid(kit, lid_wood, iron), "SM_MineTrapdoorLid", pivot=None)))
    collar_wood = kit.mats.aged_wood("M_MineTrapdoorCollar", light=(0.20, 0.15, 0.10), dark=(0.06, 0.045, 0.03),
                                     saw=0.4, grime=0.75, seed=11.0)
    out.append(plank_uvs(kit.join(trapdoor_collar(kit, collar_wood), "SM_MineTrapdoorCollar", pivot=None), seed=5))
    return out
