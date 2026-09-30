"""Cut granite steps and landings for the cove route (add-cove-route-kit; Water's route data in
Source/SurvivalGame/Simulation/HomesteadEstateCoveRoute.inc).

Real-object research (written before modeling):
- Cornish estate and coastal steps of the period were cut from moorland granite: blocks 15-20 cm thick,
  dressed on the tread and the riser face with a punch and a claw tool, the arrises softened and chipped,
  laid on rubble with the back of each block under the nosing of the one above.
- A century of feet dishes the middle of each tread, polishes it and rounds the nosing where people step;
  the untrodden ends keep their tool marks, salt bloom and grey-green lichen, and moss in the joints.
- Landings between flights are one or two long slabs; where a stair turns, a wedge-shaped stone fills
  the outside of the turn.

Pieces and pivots (meters, Z up, given in the engine's frame; the FBX export mirrors Y, so the recipe builds\nthe wedge toward -Y):
- SM_CoveStep_A/B/C: one tread block, 1.50 m wide (the 1.4 m clear width plus 5 cm bedding each side),
  0.36-0.38 m from nosing to back, 0.20 m thick with a 5 cm skirt below it (0.25 m), so a 30-34 cm going
  laps the next block by a few cm. Pivot on the top face at the centre of the front nosing, +X up the
  flight (the back of the block), Y across.
- SM_CoveLanding: a 1.2 m landing of two slabs; SM_CoveLandingSlab: one 0.6 m slab, for Water to tile the
  1.23-2.30 m landings (each run scaled by at most +-10%). Pivot on the top face at the centre of the
  downhill edge, +X uphill.
- SM_CoveLandingWedge: fills the outside of a corner landing's turn: a 21-degree sector 1.5 m long from
  its apex. Pivot at the apex (the inner corner) on the top face, +X along the incoming leg; scale Y by
  tan(18)/tan(21) for the 18-degree turn.
Original procedural geometry and materials only.
"""
import math
import random

from mathutils import Vector, noise

NAME = "CoveSteps"
DESCRIPTION = ("Cut granite treads (3 variants), a 1.2 m landing, a 0.6 m landing slab and a 21-degree corner "
               "wedge for the cove route (original). Pivots on the top face at the front nosing / downhill edge "
               "centre, +X up the flight.")
COLLISION = "convex"
TRIANGLE_BUDGET = 24000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 96, "maps": ("basecolor", "roughness", "normal", "ao")}
BEAUTY = {"pose": (0, 0, 35), "focus": (0.15, 0.0, 0.0)}
REPORT = {"pivot": "top face, centre of the front nosing (treads) or downhill edge (landings); +X up the flight",
          "tread_width_m": 1.5, "block_depth_m": 0.25, "skirt_m": 0.05}

WIDTH = 1.50
THICK = 0.20
SKIRT = 0.05
NOSING_OVER = 0.012          # the nosing's round overhangs the riser a little
TREADS = {"A": (0.37, 11), "B": (0.36, 12), "C": (0.38, 13)}
LANDING = 1.20
SLAB = 0.60
WEDGE_DEG = 21.0
WEDGE_LEN = 1.50


def granite(kit, name, seed):
    return kit.mats.granite(name, grain=0.004, patina=0.65, lichen=0.35, moss=0.15, iron=0.2, streaks=0.2,
                            soil=0.0, relief=0.7, north=(0.0, 1.0, 0.0), seed=seed)


def dressed_block(kit, name, length, width, material, seed, dish=True):
    """A granite block with its top face at z = 0 and its front (downhill) edge at x = 0: softened arrises,
    tool-dressed faces, a trodden dish in the middle of the tread and chipped corners."""
    rng = random.Random(seed)
    depth = THICK + SKIRT
    block = kit.box(name, (length + NOSING_OVER, width, depth),
                    location=((length - NOSING_OVER) * 0.5, 0.0, -depth * 0.5), material=material,
                    bevel=0.014, bevel_segments=3)
    kit.subdivide(block, levels=3, smooth=False)
    phase = Vector((rng.uniform(0, 50), rng.uniform(0, 50), rng.uniform(0, 50)))

    def dress(co, pcoord):
        p = co * 9.0 + phase
        tooled = 0.0012 * noise.noise(p) + 0.0006 * noise.noise(co * 40.0 + phase)
        wear = 0.0
        if dish and co.z > -0.004:
            # Feet land about a third of the going back from the nosing, across the middle 60% of the width.
            along = math.exp(-((co.x - 0.35 * length) / (0.28 * length)) ** 2)
            across = math.exp(-(co.y / (0.30 * width)) ** 2)
            wear = -0.006 * along * across
        # Chipped arrises: a few irregular losses where the corners took knocks.
        edge = min(abs(co.x + NOSING_OVER), abs(co.x - length), abs(abs(co.y) - width * 0.5), abs(co.z))
        chip = -0.004 * max(0.0, noise.noise(co * 6.0 + phase * 1.7) - 0.35) if edge < 0.02 else 0.0
        return tooled + wear + chip
    kit.displace(block, dress)
    return block


def build_tread(kit, key, mat):
    length, seed = TREADS[key]
    return kit.join([dressed_block(kit, "Tread" + key, length, WIDTH, mat, seed)],
                    "SM_CoveStep_" + key, pivot=None, unwrap=True, reshade=True, smooth_angle=35)


def build_landing(kit, mat):
    parts = [dressed_block(kit, "LandingA", SLAB - 0.002, WIDTH, mat, 21, dish=False)]
    second = dressed_block(kit, "LandingB", SLAB - 0.002, WIDTH, mat, 22, dish=False)
    kit.warp(second, lambda co: co + Vector((SLAB, 0.0, 0.0)))
    parts.append(second)
    return kit.join(parts, "SM_CoveLanding", pivot=None, unwrap=True, reshade=True, smooth_angle=35)


def build_slab(kit, mat):
    return kit.join([dressed_block(kit, "Slab", SLAB - 0.002, WIDTH, mat, 31, dish=False)],
                    "SM_CoveLandingSlab", pivot=None, unwrap=True, reshade=True, smooth_angle=35)


def build_wedge(kit, mat):
    """A sector of granite from the apex: rows of the sector's arc lofted from a thin sliver at the apex."""
    depth = THICK + SKIRT
    arc = 10
    rows = []
    for k in range(1, 13):
        r = WEDGE_LEN * k / 12
        top, bottom = [], []
        for j in range(arc + 1):
            a = math.radians(WEDGE_DEG) * j / arc
            top.append((r * math.cos(a), -r * math.sin(a), 0.0))
            bottom.append((r * math.cos(a), -r * math.sin(a), -depth))
        rows.append(top + list(reversed(bottom)))
    wedge = kit.loft("Wedge", rows, material=mat, cap_start=True, cap_end=True)
    kit.recalc_normals(wedge)   # built toward -Y, which winds the rings the other way

    def dress(co, pcoord):
        return 0.0012 * noise.noise(co * 9.0 + Vector((7.0, 3.0, 1.0)))
    kit.displace(wedge, dress)
    return kit.join([wedge], "SM_CoveLandingWedge", pivot=None, unwrap=True, reshade=True, smooth_angle=35)


def build(kit):
    mat = granite(kit, "M_CoveGranite", 40.0)
    meshes = [build_tread(kit, key, mat) for key in TREADS]
    meshes += [build_landing(kit, mat), build_slab(kit, mat), build_wedge(kit, mat)]
    return meshes
