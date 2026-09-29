"""Pickaxe rocks: three sizes of Cornish moorstone, from knee-high to waist-high, part-buried in turf.

Reference: the moorstone of Bodmin Moor, Carnmenellis and West Penwith, before and during the
19th-century granite trade. Coarse porphyritic biotite granite, with white K-feldspar megacrysts
(2-4 cm) in a grey groundmass. The corestones weathered free of the growan (decomposed granite):
blocky shapes with edges rounded by spheroidal weathering, joint faces still broad and flat.
They sit a quarter to a third buried in the turf. Clean Atlantic air crusts their tops with
lichen (grey-green Parmelia, white Ochrolechia, yellow map lichen), and moss fills the shaded
north foot. Moorstone was split where it lay: "tare and feather" (plug and feathers) splitting
from about 1800 leaves a straight split face with a row of half drill holes along its top edge.
An abandoned line of holes on a boulder's top shows where the next split would have gone.

Meshes:
  Small   knee-high (~0.3 m above the turf, 0.65 m long; the SmallRock node). A sub-angular block with
          one freshly broken corner.
  Medium  thigh-high (~0.65 m above the turf, 1.4 m long); a spare boulder variant. A rounded loaf with a natural joint crack across
          its crown and spalled plates.
  Large   waist-high (~1.0 m tall, 1.7 m long); the iron-pick tier. A blocky moorstone with one end
          split off by plug and feathers: a paler, planar split face with half drill holes along its
          top edge, and an unfinished row of drill holes across its top.

Everything is generated here: no scanned or downloaded geometry or textures.
Units are meters; the ground line is at z = 0 (origin), +Y is north (moss side).
"""
import numpy as np

import homestead_rocks as rocks

NAME = "GranitePickRocks"
DESCRIPTION = ("Three pickaxe rocks (Small knee-high, Medium thigh-high, Large waist-high with a plug-and-feather "
               "split face) of part-buried Cornish moorstone; Nanite; LOD1/LOD2 fallback.")
COLLISION = "convex"
TRIANGLE_BUDGET = 320000
BAKE = {"size": 2048, "samples": 64, "repack": False}
BAKE_MESHES = {"SM_GranitePickRock_Large": {"size": 4096}, "*": True}
BEAUTY = {"pose": (0, 0, 0), "ground": "origin", "views": ["hero", "detail", "eye"],
          "meshes": {"SM_GranitePickRock_Small": {"focus": (0.13, -0.13, 0.18), "eye_distance": 2.2},
                     "SM_GranitePickRock_Medium": {"focus": (0.1, -0.3, 0.5), "eye_distance": 4.0},
                     "SM_GranitePickRock_Large": {"focus": (-0.72, -0.2, 0.5), "eye_distance": 5.5}}}
REPORT = {"nanite": True}
NOTES = {}


def _face_weight(points, normal, n, d, reach=0.03):
    """1 on the face of joint plane (n, d), fading off its rounded edge."""
    return np.exp(-(np.maximum(d - points @ n, 0) / reach) ** 2) * np.clip(normal @ n, 0, 1)


def _drill_rows(points, normal, face_n, face_d, spacing, radius, deep, seed):
    """Half drill holes along the top edge of a split face (the plug-and-feather row the split ran
    along). Returns (depth along -N, on_face weight)."""
    n = rocks.unit(face_n)
    on = _face_weight(points, normal, n, face_d, 0.02)
    face = on > 0.6
    u_dir = rocks.unit(np.cross((0.0, 0.0, 1.0), n))
    u = points @ u_dir
    depth = np.zeros(len(points))
    if not face.any():
        return depth, on
    uf, zf = u[face], points[face, 2]
    lo, hi = uf.min(), uf.max()
    rng = np.random.default_rng(seed)
    centres = np.arange(lo + spacing * 0.6, hi - spacing * 0.4, spacing) + rng.uniform(-0.01, 0.01)
    for c in centres:
        near = np.abs(uf - c) < radius * 2.5
        if not near.any():
            continue
        top = zf[near].max()
        length = deep * rng.uniform(0.85, 1.15)
        du = np.abs(u - c)
        groove = np.sqrt(np.clip(radius ** 2 - du ** 2, 0, None))
        # Rounded hole bottom and a slight flare where the drill entered.
        z_in = rocks.smoothstep(top - length, top - length + radius * 1.5, points[:, 2])
        depth = np.maximum(depth, groove * z_in * on)
    return depth, on


def _top_holes(points, normal, holes, radius, deep):
    """Drilled holes (no split yet) on an upward face: ``holes`` of (x, y)."""
    depth = np.zeros(len(points))
    up = rocks.smoothstep(0.45, 0.75, normal[:, 2])
    for x, y in holes:
        near = np.hypot(points[:, 0] - x, points[:, 1] - y)
        depth = np.maximum(depth, deep * (1.0 - rocks.smoothstep(radius * 0.65, radius * 1.1, near)) * up)
    return depth


ROCKS = {
    "Small": dict(
        radii=(0.325, 0.235, 0.235), power=2.9, lumps=0.025, seed=4101, ground_z=-0.09, tris=70000,
        lods=(14000, 3500),
        faces=[((0.0, 0.05, -1.0), 0.17, 0.033, True),       # buried base
               ((0.1, -0.15, 1.0), 0.205, 0.045, True),      # crown joint
               ((0.85, -0.55, 0.35), 0.273, 0.008, False),   # freshly broken corner
               ((-1.0, 0.25, 0.1), 0.286, 0.045, True),
               ((0.2, 1.0, 0.0), 0.2, 0.04, True)],
        material=dict(lichen=0.95, moss=0.25, iron=0.3, patina=0.75, seed=41.0)),
    "Medium": dict(
        radii=(0.74, 0.52, 0.5), power=2.6, lumps=0.03, seed=4102, ground_z=-0.2, tris=160000,
        lods=(30000, 7000),
        faces=[((0.0, 0.0, -1.0), 0.36, 0.08, True),
               ((-0.2, 0.1, 1.0), 0.44, 0.12, True),
               ((1.0, 0.2, 0.1), 0.66, 0.1, True),
               ((-1.0, -0.1, 0.2), 0.66, 0.08, True),
               ((0.3, -1.0, 0.25), 0.46, 0.04, False)],      # an old spall scar, still paler
        crack=((1.0, 0.25, 0.0), 0.12, 0.018, 0.06),
        plates=dict(cell=0.4, thickness=0.024, coverage=0.3),
        material=dict(lichen=1.1, moss=0.3, iron=0.35, patina=0.8, seed=42.0)),
    "Large": dict(
        radii=(1.05, 0.72, 0.68), power=3.0, lumps=0.025, seed=4103, ground_z=-0.3, tris=280000,
        lods=(50000, 10000),
        faces=[((0.0, 0.05, -1.0), 0.5, 0.12, True),
               ((0.06, -0.1, 1.0), 0.66, 0.14, True),
               ((0.05, 1.0, 0.05), 0.62, 0.12, True),
               ((0.1, -1.0, 0.1), 0.64, 0.12, True),
               ((1.0, 0.15, 0.05), 0.96, 0.12, True)],
        split=((-1.0, -0.22, 0.02), 0.72),                    # plug-and-feather split face (west end)
        holes=[(-0.05, y) for y in (-0.36, -0.21, -0.06, 0.09, 0.24)],
        crack=((1.0, 0.25, 0.0), 0.45, 0.02, 0.07),
        plates=dict(cell=0.6, thickness=0.03, coverage=0.3),
        material=dict(lichen=1.15, moss=0.32, iron=0.4, patina=0.85, seed=43.0)),
}


def rock(kit, key, spec):
    m = spec["material"]
    size = max(spec["radii"]) * 2
    mat = kit.mats.granite(f"M_GranitePickRock_{key}", grain=0.004, scale=size, patina=m["patina"],
                           lichen=m["lichen"], moss=m["moss"], iron=m["iron"], streaks=0.35, soil=0.5,
                           soil_height=0.06 + 0.05 * size, enclaves=0.4, megacrysts=0.85, film=0.7, spots=1.1,
                           seed=m["seed"])
    joints = [(n, d, r) for n, d, r, _ in spec["faces"]]
    field = rocks.boulder(radii=spec["radii"], power=spec["power"], lumps=spec["lumps"], lump_scale=1.2,
                          seed=spec["seed"], joints=joints)
    if "split" in spec:
        n, d = spec["split"]
        field = rocks.cut(field, n, d, rounding=0.012, wobble=0.004, wobble_scale=3.0, seed=spec["seed"])

    def post(points, normal):
        if "crack" not in spec:
            return points
        n, d, width, deep = spec["crack"]
        depth = rocks.crack(points, n, d, width, deep, seed=spec["seed"] + 1, wobble=0.03, wobble_scale=2.5,
                            taper=(2, -0.05, 0.25))
        return points - normal * depth[:, None]

    solid = rocks.solid(f"Pick{key}", field, subdivisions=8, post=post, material=mat, r_max=size * 2)

    def detail(points, normal):
        fresh = np.zeros(len(points))
        for n, d, _, old in spec["faces"]:
            if not old:
                fresh = np.maximum(fresh, _face_weight(points, normal, rocks.unit(n), d))
        cut = np.zeros(len(points))
        if "split" in spec:
            n, d = spec["split"]
            cut, on = _drill_rows(points, normal, n, d, spacing=0.15, radius=0.017, deep=0.11,
                                  seed=spec["seed"])
            # Split about 70 years ago: paler than the weathered rind, but greying.
            fresh = np.maximum(fresh, on * 0.75)
        if "holes" in spec:
            cut = cut + _top_holes(points, normal, spec["holes"], 0.019, 0.08)
        spall = np.zeros(len(points))
        if "plates" in spec:
            p = spec["plates"]
            spall, spalled = rocks.plates(points, normal, seed=spec["seed"] + 2, cell=p["cell"],
                                          thickness=p["thickness"], coverage=p["coverage"], width=0.018,
                                          wobble=0.28, bias=lambda c: 0.3 * np.clip(c[:, 2] / 0.4, -1, 1))
            spall = spall * (1.0 - fresh)
            fresh = np.maximum(fresh, spalled * 0.45 * (1.0 - fresh))
        chips, chipped = rocks.plates(points, normal, seed=spec["seed"] + 3, cell=0.12, thickness=0.008,
                                      coverage=0.12, width=0.008, wobble=0.3)
        smooth_face = 1.0 - 0.7 * fresh
        grain = rocks.relief(points, normal, spec["seed"] + 4,
                             [(0.4, 0.006), (0.08, 0.0022), (0.02, 0.0009)]) * smooth_face
        fresh = np.clip(np.maximum(fresh, chipped * 0.5), 0, 1)
        return grain - spall - chips - cut, {"fresh": fresh}

    meshes, sink = rocks.finish(kit, [solid], f"SM_GranitePickRock_{key}", spec["tris"], spec["lods"],
                                ground_z=spec["ground_z"], detail=detail)
    NOTES[key] = {"sink_depth_m": round(sink, 3)}
    return meshes


def build(kit):
    meshes = []
    for key, spec in ROCKS.items():
        meshes += rock(kit, key, spec)
    NOTES.update({
        "placement": "Origin is the ground line; place at terrain height (each is already part-sunk; see "
                     "sink_depth_m). Add ~0.5 x slope x radius on slopes.",
        "north": "+Y (moss side)",
        "collision": "Convex (simple) per mesh; they're blocking obstacles.",
        "tiers": "Small/Medium: stone pick. Large: iron pick (the split face and drill holes mark it quarry-able).",
        "traces": "Nanite meshes don't answer LineTraceComponent; trace the world and test Hit.GetComponent().",
    })
    return meshes
