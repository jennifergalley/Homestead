"""Oak post-and-rail handrail for the cove route: level and raked bays, end and corner posts
(add-cove-route-kit).

Real-object research (written before modeling):
- Estate and harbour-path rails of the period were riven or sawn oak: square posts about 4-5 in (10-12 cm)
  set 5-6 ft (1.5-1.8 m) apart, a rounded hand rail about 3 ft (90-100 cm) up and a lower rail, the rails
  tenoned or spiked into the posts; on steps the rails run parallel to the nosing line and the posts
  stand plumb.
- By the sea oak silvers grey within a few years, checks along the grain, and goes green with algae low
  down; hand rails are polished darker where hands slide. Wrought-iron shoe straps protect post feet set
  in stone.

Pieces and pivots (meters, Z up, the engine's frame): SM_CoveRail_Level and SM_CoveRail_Rake26/28/30, one
bay 1.70 m in plan: its downhill post at the pivot (the post's foot on the path or the step nosing line),
the rails running up +X to where the next bay's post stands (the next bay supplies that post; a run ends
with SM_CoveRail_EndPost). Raked bays rise at their pitch along +X; Water's builder scales a bay's X and Z
together to its plan length so the pitch holds. SM_CoveRail_CornerPost stands where a rail turns.
+Y points to the drop, but every piece is symmetric across its rail line (the XZ plane): posts, rails,
shoe straps, nails, checks and wear match on both faces, with no text and no one-sided detail, so Water
mirrors the far-side raked bays with scale Y = -1 (a raked bay can't be turned round).
No collision on the mesh: the game adds one pawn-only blocker along each railed edge.
Original procedural geometry and materials only.
"""
import math

from mathutils import Vector

NAME = "CoveRail"
DESCRIPTION = ("Silvered oak post-and-rail handrail: level bay, raked bays at 26/28/30 degrees, end and corner "
               "posts, all symmetric across the rail line for Y-mirroring (original). Pivot = downhill post's foot.")
COLLISION = "none"
TRIANGLE_BUDGET = 16000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 96, "maps": ("basecolor", "roughness", "normal", "ao", "metallic")}
BEAUTY = {"pose": (0, 0, 25), "focus": (0.0, 0.0, 0.92)}
REPORT = {"pivot": "foot of the downhill post on the path or nosing line; +X along the path, +Y to the drop",
          "bay_plan_m": 1.70, "rail_height_m": 0.95, "pitches_deg": [0, 26, 28, 30],
          "symmetric_across_xz": True}

BAY = 1.70
POST = 0.12
POST_BURY = 0.22            # set into the ground or the step below its foot
POST_TOP = 1.02
RAIL_Z = 0.95               # top of the hand rail above the path or nosing line
RAIL_R = 0.032
MID_Z = 0.50
MID_R = 0.024
PITCHES = (0, 26, 28, 30)


def materials(kit):
    oak = kit.mats.wood("M_CoveRailOak", light=(0.30, 0.28, 0.25), dark=(0.10, 0.095, 0.085), grain=1.1,
                        roughness=0.8, weathering=0.75, grime=0.25, seed=61.0, relief=1.2)
    hand = kit.mats.wood("M_CoveRailHand", light=(0.26, 0.22, 0.17), dark=(0.09, 0.075, 0.06), grain=1.0,
                         roughness=0.6, weathering=0.45, grime=0.2, seed=62.0, polish=0.6, polish_center=0.0,
                         polish_length=2.0)
    iron = kit.mats.wrought_iron("M_CoveRailIron", rust=0.6, wear=0.3, seed=63.0)
    return oak, hand, iron


def post(kit, name, x, oak, iron, cap=True):
    """A square oak post centred on the rail line at x: bedded below the pivot, a pyramid cap, and a
    wrought-iron shoe strap on each face across the rail line (both faces, so it mirrors)."""
    parts = []
    height = POST_TOP + POST_BURY
    body = kit.box(name, (POST, POST, height), location=(x, 0.0, POST_TOP - height * 0.5), material=oak,
                   bevel=0.008, bevel_segments=2)
    parts.append(body)
    if cap:
        parts.append(kit.cylinder(name + "Cap", POST * 0.72, 0.045, location=(x, 0.0, POST_TOP + 0.022),
                                  rotation=(0, 0, 45), material=oak, sides=4, radius_top=0.012))
    for side in (-1.0, 1.0):
        strap_y = side * (POST * 0.5 + 0.002)
        parts.append(kit.box(name + "Shoe%+d" % side, (0.05, 0.004, 0.24), location=(x, strap_y, 0.02),
                             material=iron, bevel=0.0012))
        for z in (-0.06, 0.1):
            parts.append(kit.cylinder(name + "Nail%+d_%.2f" % (side, z), 0.006, 0.005,
                                      location=(x, strap_y + side * 0.002, z), rotation=(90, 0, 0),
                                      material=iron, sides=8, radius_top=0.004))
    return parts


def rail(kit, name, z0, pitch, radius, mat, length=BAY):
    """A rounded rail along +X at height z0 above the line, rising at ``pitch``, into both posts' centres."""
    rise = math.tan(math.radians(pitch))
    pts = [(x, 0.0, z0 + x * rise) for x in [length * k / 10 for k in range(11)]]
    return kit.tube(name, pts, radius=radius, sides=18, material=mat)


def build_bay(kit, pitch, oak, hand, iron):
    parts = post(kit, "Post", 0.0, oak, iron)
    parts.append(rail(kit, "HandRail", RAIL_Z - RAIL_R, pitch, RAIL_R, hand))
    parts.append(rail(kit, "MidRail", MID_Z, pitch, MID_R, oak))
    name = "SM_CoveRail_Level" if pitch == 0 else "SM_CoveRail_Rake%d" % pitch
    return kit.join(parts, name, pivot=None, unwrap=True, reshade=True, smooth_angle=40)


def build_end_post(kit, oak, iron):
    return kit.join(post(kit, "EndPost", 0.0, oak, iron), "SM_CoveRail_EndPost", pivot=None, unwrap=True,
                    reshade=True, smooth_angle=40)


def build_corner_post(kit, oak, iron):
    # A heavier post where the rail turns, square to the incoming run.
    parts = post(kit, "CornerPost", 0.0, oak, iron)
    for part in parts:
        kit.warp(part, lambda co: Vector((co.x * 1.2, co.y * 1.2, co.z)))
    return kit.join(parts, "SM_CoveRail_CornerPost", pivot=None, unwrap=True, reshade=True, smooth_angle=40)


def build(kit):
    oak, hand, iron = materials(kit)
    meshes = [build_bay(kit, pitch, oak, hand, iron) for pitch in PITCHES]
    meshes += [build_end_post(kit, oak, iron), build_corner_post(kit, oak, iron)]
    return meshes
