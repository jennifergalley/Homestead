"""Held tinned sheet-iron watering can for the Homestead hotbar.

Real-object research (written before modeling):
- Mid-19th-century British garden watering cans were made from tinned sheet iron:
  cylindrical or slightly oval bodies with soldered seams, folded/rolled rims, a long
  tapered spout ending in a perforated "rose", and strap handles riveted to the body.
  Tinplate dulls quickly: grey metal, bright rubbed high spots, solder lines, white
  water staining, orange rust freckles at seams and dents from being knocked against
  stone floors.
- Period cans of a hand-tool size are about 30 cm high to the rim, 22-25 cm diameter,
  with a long spout and a top strap handle plus a rear pouring handle. The rose is a
  shallow round plate with many small holes.
- PIVOT/AXES FOR ANIMATION: this is authored like ``water_pail.py`` for hand attach.
  The origin is the middle of the TOP carrying-handle grip where the fist closes; the
  can hangs below it (-Z). The grip segment runs along Y. The spout points +X, matching
  the wooden pail's pouring lip +X. Base is 0.405 m below the grip; spout tip is at
  (+0.345, 0.0, -0.180) from the pivot. Collision is none.
  Original procedural geometry/materials only.
"""
import math

from mathutils import Matrix, Vector, noise

NAME = "TinWateringCan"
DESCRIPTION = "1850s tinned sheet-iron watering can hotbar tool; pivot at top handle grip; spout +X."
COLLISION = "none"
TRIANGLE_BUDGET = 15000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 96, "maps": ("basecolor", "roughness", "normal", "ao", "metallic")}

BODY_H = 0.300
BODY_R = 0.116
GRIP_Z = 0.405
SPOUT_TIP = Vector((0.345, 0.0, 0.225))
GRIP = Vector((0.0, 0.0, GRIP_Z))
BEAUTY = {"pose": (0, 0, -55), "focus": (0.22 - GRIP.x, -0.02 - GRIP.y, 0.20 - GRIP.z),
          "detail_distance": 0.34}
REPORT = {"attach": {
    "units": "cm in exported mesh frame; origin is top carrying-handle grip centre; grip runs Y; spout points +X; can hangs -Z",
    "grip_height_above_base_cm": round(GRIP_Z * 100, 2),
    "spout_tip_offset_cm": [round((SPOUT_TIP.x - GRIP.x) * 100, 2),
                             round((SPOUT_TIP.y - GRIP.y) * 100, 2),
                             round((SPOUT_TIP.z - GRIP.z) * 100, 2)],
    "body_height_cm": round(BODY_H * 100, 2),
    "body_diameter_cm": round(BODY_R * 2 * 100, 2),
}, "pivot": "top grip centre, not bottom; collision none"}


def smoothstep(a, b, x):
    if a == b:
        return 1.0 if x >= b else 0.0
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3 - 2 * t)


def _circle_tube(kit, name, radius, z, tube_radius, mat, sides=72, wire_sides=10,
                 squash_y=1.0, xoff=0.0, yoff=0.0):
    pts = [Vector((xoff + math.cos(math.tau * i / sides) * radius,
                   yoff + math.sin(math.tau * i / sides) * radius * squash_y, z))
           for i in range(sides + 1)]
    return kit.tube(name, pts, radius=tube_radius, sides=wire_sides, material=mat)


def _lathe(kit, name, profile, mat, sides=72, cap_bottom=False, cap_top=False):
    rows, coords = [], []
    for z, r in profile:
        row, pco = [], []
        for i in range(sides):
            a = math.tau * i / sides
            # Tiny ovaling: sheet iron bodies are not perfectly round.
            oval = 1.0 + 0.025 * math.cos(2 * a + 0.35)
            x = r * oval * math.cos(a)
            y = r * (1.0 - 0.020 * math.cos(2 * a + 0.35)) * math.sin(a)
            row.append((x, y, z))
            pco.append((x, y, z))
        rows.append(row)
        coords.append(pco)
    obj = kit.loft(name, rows, material=mat, coords=coords, cap_start=cap_bottom, cap_end=cap_top)
    return kit.recalc_normals(obj)


def body_mesh(kit, tin):
    profile = [(0.000, 0.090), (0.010, 0.111), (0.025, 0.119), (0.075, 0.121),
               (0.155, 0.118), (0.235, 0.112), (0.275, 0.086), (0.300, 0.052)]
    obj = _lathe(kit, "CanBody", profile, tin, sides=72, cap_bottom=True, cap_top=False)

    def dented(co, pco):
        relief = 0.0005 * noise.noise(Vector((pco.x * 60, pco.y * 60, pco.z * 26)))
        for cx, cy, cz, sx, sy, sz, depth in ((0.072, -0.056, 0.125, 0.035, 0.022, 0.040, 0.0028),
                                              (-0.061, 0.070, 0.192, 0.030, 0.025, 0.034, 0.0020),
                                              (0.010, -0.104, 0.052, 0.050, 0.018, 0.026, 0.0018)):
            d = ((co.x - cx) / sx) ** 2 + ((co.y - cy) / sy) ** 2 + ((co.z - cz) / sz) ** 2
            relief -= depth * math.exp(-d)
        return relief

    kit.displace(obj, dented)
    return obj


def tapered_spout(kit, tin):
    pts = [Vector((0.096, 0.0, 0.165)), Vector((0.170, 0.0, 0.190)),
           Vector((0.260, 0.0, 0.215)), SPOUT_TIP]
    radii = [0.033, 0.028, 0.022, 0.017]
    spout = kit.tube("TaperedSpout", pts, radii=radii, sides=32, material=tin)
    def seam(co, pco):
        # A shallow lengthwise solder seam under the spout.
        a = math.atan2(pco.y, pco.x)
        return 0.00055 * smoothstep(0.32, 0.0, abs(a + math.pi / 2))
    kit.displace(spout, seam)
    return spout


def rose(kit, tin, dark):
    parts = [kit.cylinder("SpoutRosePlate", 0.043, 0.010, location=(SPOUT_TIP.x + 0.008, 0, SPOUT_TIP.z),
                          rotation=(0, 90, 0), material=tin, sides=40, bevel=0.0012, bevel_segments=1),
             kit.cylinder("SpoutRoseRim", 0.046, 0.006, location=(SPOUT_TIP.x + 0.003, 0, SPOUT_TIP.z),
                          rotation=(0, 90, 0), material=tin, sides=40, bevel=0.001, bevel_segments=1)]
    # Dark recessed holes on the face, arranged in hand-punched rings.
    idx = 0
    for ring, count in ((0.000, 1), (0.012, 6), (0.023, 12), (0.033, 16)):
        for i in range(count):
            a = math.tau * i / count if count > 1 else 0
            y = math.cos(a) * ring
            z = SPOUT_TIP.z + math.sin(a) * ring
            parts.append(kit.cylinder(f"RoseHole_{idx:02d}", 0.0025 if ring else 0.0032, 0.0012,
                                      location=(SPOUT_TIP.x + 0.014, y, z), rotation=(0, 90, 0),
                                      material=dark, sides=8))
            idx += 1
    return parts


def top_handle(kit, tin):
    parts = []
    for yi, y in enumerate((-0.052, 0.052)):
        pts = [Vector((-0.090, y, 0.260)), Vector((-0.070, y, 0.325)),
               Vector((-0.020, y, 0.387)), Vector((0.000, y, GRIP_Z)),
               Vector((0.052, y, 0.360)), Vector((0.076, y, 0.292))]
        parts.append(kit.tube(f"TopHandleArch_{yi}", pts, radius=0.0052, sides=12, material=tin))
    parts.append(kit.tube("TopHandleGrip", [Vector((0.0, -0.062, GRIP_Z)), Vector((0.0, 0.062, GRIP_Z))],
                          radius=0.0080, sides=16, material=tin))
    for y in (-0.052, 0.052):
        parts.append(kit.cylinder(f"TopHandleRearRivet_{y}", 0.006, 0.003, (-0.093, y, 0.257),
                                  rotation=(0, 90, 0), material=tin, sides=14, bevel=0.0005))
        parts.append(kit.cylinder(f"TopHandleFrontRivet_{y}", 0.006, 0.003, (0.078, y, 0.290),
                                  rotation=(0, 90, 0), material=tin, sides=14, bevel=0.0005))
    return parts


def rear_handle(kit, tin):
    parts = []
    for y in (-0.036, 0.036):
        parts.append(kit.tube(f"RearHandleSide_{y}", [Vector((-0.106, y, 0.075)), Vector((-0.170, y, 0.165)),
                                                      Vector((-0.112, y, 0.255))],
                              radius=0.0062, sides=12, material=tin))
    parts.append(kit.tube("RearHandleGrip", [Vector((-0.175, -0.038, 0.165)), Vector((-0.175, 0.038, 0.165))],
                          radius=0.0075, sides=14, material=tin))
    for z in (0.075, 0.255):
        parts.append(kit.tube(f"RearHandleRivetBar_{z}", [Vector((-0.106, -0.048, z)), Vector((-0.106, 0.048, z))],
                              radius=0.0022, sides=8, material=tin))
    return parts


def build(kit):
    m = kit.mats
    tin = m.tinplate("M_TinWateringCanDullTinplate", base=(0.30, 0.31, 0.29), rust=0.50, seed=510.0)
    solder = m.tinplate("M_TinWateringCanSolderSeams", base=(0.36, 0.35, 0.31), rust=0.58, seed=511.0)
    dark = kit.material("M_TinWateringCanDarkHole", (0.012, 0.011, 0.010), roughness=0.95)
    # Opaque metal: no subsurface inherited into the baked material.
    for mat in (tin, solder, dark):
        if mat.node_tree and "Principled BSDF" in mat.node_tree.nodes and "Subsurface Weight" in mat.node_tree.nodes["Principled BSDF"].inputs:
            mat.node_tree.nodes["Principled BSDF"].inputs["Subsurface Weight"].default_value = 0.0

    parts = [body_mesh(kit, tin),
             _circle_tube(kit, "BottomRolledSeam", 0.112, 0.014, 0.0020, solder, sides=72),
             _circle_tube(kit, "BodyBellySolderLine", 0.118, 0.152, 0.0013, solder, sides=72),
             _circle_tube(kit, "ShoulderRolledRim", 0.054, 0.300, 0.0024, solder, sides=56),
             tapered_spout(kit, tin)]
    parts.extend(rose(kit, tin, dark))
    parts.extend(top_handle(kit, tin))
    parts.extend(rear_handle(kit, tin))
    # Vertical solder seam on the back of the can and white water scale below the rose.
    parts.append(kit.tube("RearVerticalSolderSeam", [Vector((-0.004, 0.119, 0.020)), Vector((-0.006, 0.113, 0.255))],
                          radius=0.0017, sides=8, material=solder))
    scale_mat = kit.material("M_TinWateringCanWhiteWaterScale", (0.38, 0.36, 0.30), roughness=0.96)
    parts.append(kit.tube("SpoutWaterStain", [Vector((0.122, -0.020, 0.145)), Vector((0.220, -0.028, 0.178)),
                                              Vector((0.312, -0.018, 0.205))],
                          radius=0.0016, sides=5, material=scale_mat))

    for part in parts:
        part.data.transform(part.matrix_world)
        part.matrix_world = Matrix.Identity(4)
        part.data.transform(Matrix.Translation(-GRIP))
        part.data.update()
    return kit.join(parts, "SM_TinWateringCan", pivot=None, unwrap=False, reshade=True, smooth_angle=58)
