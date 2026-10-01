"""Granite edge kerbs for the cove route's paths and ramps (add-cove-route-kit).

Real-object research (written before modeling):
- Estate path edging of the period was split granite set on edge: pieces about 1 m long, 12-15 cm thick,
  standing 15-20 cm proud with their lower third bedded in the ground, the top pitch-faced or lightly
  dressed and the ends butted with a finger's gap. On a bend short pieces follow the curve.
- Weathering: rounded top arrises, lichen and moss low on the drop side, soil stain at the ground line.

Pieces and pivots (meters, Z up, given in the engine's frame; the FBX export mirrors Y, so the recipe builds\nthe drop side on -Y): SM_CoveKerb_Straight (1.0 m), SM_CoveKerb_Curve15
(1.0 m along an arc turning 15 degrees toward +Y; mirror it with scale Y = -1 for a turn the other way)
and SM_CoveKerb_End (a 0.5 m piece with a rounded end at +X). Pivot on the path-side top edge at the
piece's centre (its start for the end piece): +X along the path, +Y toward the drop. The stone stands
0.20 m above the path (pivot z) and is bedded 0.10 m below it: 0.30 m tall, 0.15 m thick.
Collision is a simple box; the game's blocker refuses step-up (ECB_No).
Original procedural geometry and materials only.
"""
import math
import random

from mathutils import Vector, noise

NAME = "CoveKerb"
DESCRIPTION = ("Split granite edge kerbs: 1 m straight, 1 m 15-degree curve, 0.5 m end (original). Pivot on the "
               "path-side top edge, +X along the path, +Y to the drop.")
COLLISION = "box"
TRIANGLE_BUDGET = 13000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 96, "maps": ("basecolor", "roughness", "normal", "ao")}
BEAUTY = {"pose": (0, 0, 30), "focus": (0.0, 0.07, -0.1)}
REPORT = {"pivot": "path-side top edge at the centre; +X along the path, +Y to the drop",
          "proud_m": 0.20, "bedded_m": 0.10, "thick_m": 0.15}

PROUD = 0.20
BEDDED = 0.10
THICK = 0.15
GAP = 0.008


def section(kit):
    return kit.mats.granite("M_CoveKerbGranite", grain=0.004, patina=0.7, lichen=0.4, moss=0.2, iron=0.25,
                            soil=0.4, soil_height=0.12, relief=0.8, north=(0.0, 1.0, 0.0), seed=51.0)


def kerb_stone(kit, name, length, mat, seed, end_round=False):
    """A straight kerb along +X from -length/2 to +length/2 (or 0..length for the end piece)."""
    rng = random.Random(seed)
    height = PROUD + BEDDED
    x0 = 0.0 if end_round else -length * 0.5
    stone = kit.box(name, (length - GAP, THICK, height), location=(x0 + length * 0.5, -THICK * 0.5, -height * 0.5),
                    material=mat, bevel=0.018, bevel_segments=3)
    # Top face at the pivot (0.20 m above the path), bedded 0.10 m below the path: z -0.30 .. 0.
    kit.subdivide(stone, levels=3, smooth=False)
    phase = Vector((rng.uniform(0, 40), rng.uniform(0, 40), rng.uniform(0, 40)))
    end_x = x0 + length

    def dress(co, pcoord):
        d = 0.0015 * noise.noise(co * 8.0 + phase) + 0.0007 * noise.noise(co * 35.0 + phase)
        if end_round:
            # A rounded end: pull the corners of the +X end in on a quarter circle.
            over = co.x - (end_x - THICK * 0.5)
            if over > 0:
                off = co.y + THICK * 0.5
                limit = math.sqrt(max(0.0, (THICK * 0.5) ** 2 - off * off))
                d -= max(0.0, over - limit) * 0.8
        return d
    kit.displace(stone, dress)
    return stone


def build_straight(kit, mat):
    return kit.join([kerb_stone(kit, "KerbStraight", 1.0, mat, 1)], "SM_CoveKerb_Straight", pivot=None,
                    unwrap=True, reshade=True, smooth_angle=35)


def build_curve(kit, mat):
    stone = kerb_stone(kit, "KerbCurve", 1.0, mat, 2)
    radius = 1.0 / math.radians(15.0)

    def bend(co):
        # The path-side edge (y = 0) follows an arc of radius R centred on the drop side (-Y here, +Y in the
        # engine), turning toward the drop.
        a = co.x / radius
        r = radius + co.y
        return Vector((r * math.sin(a), -(radius - r * math.cos(a)), co.z))
    kit.warp(stone, bend)
    return kit.join([stone], "SM_CoveKerb_Curve15", pivot=None, unwrap=True, reshade=True, smooth_angle=35)


def build_end(kit, mat):
    return kit.join([kerb_stone(kit, "KerbEnd", 0.5, mat, 3, end_round=True)], "SM_CoveKerb_End", pivot=None,
                    unwrap=True, reshade=True, smooth_angle=35)


def build(kit):
    mat = section(kit)
    return [build_straight(kit, mat), build_curve(kit, mat), build_end(kit, mat)]
