"""Oil flask: a stoppered tin flask of lamp oil sold at the Homestead general store.

Real-object research (written before modeling):
- Nineteenth-century household lamp oil was commonly sold or stored in small tinned
  sheet-iron cans/flasks and stoppered bottles. A hand-sized store flask is about
  5.5-6.5 in / 14-16.5 cm tall, with a short neck, cork stopper, soldered side seam,
  folded top/bottom rims and often a plain paper label band.
- Authored dimensions here: 15.4 cm overall height including cork; flattened oval tin
  body 9.2 cm wide x 5.4 cm deep x 12.3 cm high, short neck and 1.8 cm cork.
  Pivot is bottom-centre (z=0), Z up, -Y forward. Blender export mirrors Y into Unreal.
- Materials: dull tinplate with solder and rust freckles, aged rag-paper label with
  illegible ink bars, and dark oily cork. Original procedural geometry/materials only.
"""
import math
import random

import bpy
from mathutils import Vector, noise

NAME = "OilFlask"
DESCRIPTION = "Stoppered flattened tin flask of lamp oil with cork and aged paper label (original)."
COLLISION = "convex"
TRIANGLE_BUDGET = 5000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 96, "maps": ("basecolor", "roughness", "normal", "ao", "metallic")}
BEAUTY = {"pose": (0, 0, -28), "focus": (0.0, -0.01, 0.085), "ground": "origin", "detail_distance": 0.28}
REPORT = {"notes": {"pivot": "bottom-centre, z=0; Z up, -Y forward (Blender FBX export mirrors Y into Unreal)",
                      "dimensions_cm": [9.4, 5.8, 16.0]}}


def smoothstep(a, b, x):
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3 - 2 * t)


def oval_point(a, rx, ry, z):
    return Vector((math.cos(a) * rx, math.sin(a) * ry, z))


def oval_loft(kit, name, levels, mat, sides=64, cap_bottom=True, cap_top=True, offset=0.0):
    rows, coords = [], []
    for z, rx, ry in levels:
        row, pco = [], []
        for i in range(sides):
            a = math.tau * i / sides
            row.append(tuple(oval_point(a, rx + offset, ry + offset * 0.58, z)))
            pco.append((math.cos(a) * rx, math.sin(a) * ry, z))
        rows.append(row)
        coords.append(pco)
    return kit.loft(name, rows, material=mat, coords=coords, cap_start=cap_bottom, cap_end=cap_top)


def oval_tube(kit, name, rx, ry, z, tube_radius, mat, sides=40, wire_sides=8):
    pts = [oval_point(math.tau * i / sides, rx, ry, z) for i in range(sides + 1)]
    return kit.tube(name, pts, radius=tube_radius, sides=wire_sides, material=mat)


def label_band(kit, paper):
    sides = 48
    z0, z1 = 0.055, 0.094
    rx, ry = 0.047, 0.0275
    verts, coords, faces = [], [], []
    for row_index, z in enumerate((z0, z1)):
        for i in range(sides):
            a = math.tau * i / sides
            edge_noise = 0.00055 * noise.noise(Vector((i * 0.37, row_index * 6.3, 0.0)))
            # A small dog-eared lift on the front right of the paper label.
            corner = math.exp(-((math.atan2(math.sin(a + 0.80), math.cos(a + 0.80))) / 0.22) ** 2) if row_index else 0.0
            p = oval_point(a, rx, ry, z + edge_noise - 0.0005 * corner)
            p += Vector((math.cos(a) * 0.00025, math.sin(a) * (0.00025 + 0.0017 * corner), 0.0008 * corner))
            verts.append(tuple(p))
            coords.append((math.cos(a) * rx, math.sin(a) * ry, z))
    for i in range(sides):
        faces.append((i, (i + 1) % sides, sides + (i + 1) % sides, sides + i))
    data = bpy.data.meshes.new("PaperLabelBand")
    data.from_pydata(verts, [], faces)
    data.update()
    data.uv_layers.new(name="UVMap")
    kit.tag_coords(data, [Vector(c) for c in coords])
    obj = bpy.data.objects.new("PaperLabelBand", data)
    bpy.context.scene.collection.objects.link(obj)
    data.materials.append(paper)
    kit.recalc_normals(obj)
    return obj


def label_paper(kit):
    """Aged paper with its illegible store printing baked into the material, not raised
    geometry. pcoord is the oval band position; the front is negative Y."""
    g = kit.mats.Graph("M_OilFlaskPaper")
    p = g.coord()
    x, y, z = g.separate(p)
    fibre = g.noise(g.vmath("MULTIPLY", p, (1.0, 1.0, 0.35)), scale=80.0, detail=6.0,
                    roughness=0.62).outputs["Fac"]
    stain = g.noise(g.vmath("ADD", p, (3.0, 1.0, 7.0)), scale=18.0, detail=5.0,
                    roughness=0.7).outputs["Fac"]
    edge = g.math("MAXIMUM", g.remap(z, 0.060, 0.055, 0.0, 0.75), g.remap(z, 0.089, 0.094, 0.0, 0.75))
    base = g.mix((0.39, 0.32, 0.20), (0.57, 0.49, 0.32), g.remap(fibre, 0.34, 0.72))
    base = g.mix(base, (0.24, 0.18, 0.105), g.math("MAXIMUM", g.remap(stain, 0.64, 0.80, 0.0, 0.40), edge))
    front = g.remap(y, -0.010, -0.026, 0.0, 1.0)
    xgate = g.math("MULTIPLY", g.remap(x, -0.034, -0.026), g.remap(x, 0.034, 0.026))
    # Broken horizontal print lines and a simple border, all slightly blotched.
    bands = None
    for zc, half, width in ((0.062, 0.00055, 0.62), (0.069, 0.00065, 0.78),
                            (0.076, 0.00060, 0.72), (0.083, 0.00055, 0.86),
                            (0.090, 0.00050, 0.58)):
        line = g.remap(g.math("ABSOLUTE", g.math("SUBTRACT", z, zc)), half * 2.6, half, 0.0, width)
        bands = line if bands is None else g.math("MAXIMUM", bands, line)
    border_z = g.math("MAXIMUM",
                      g.remap(g.math("ABSOLUTE", g.math("SUBTRACT", z, 0.059)), 0.0018, 0.0006),
                      g.remap(g.math("ABSOLUTE", g.math("SUBTRACT", z, 0.091)), 0.0018, 0.0006))
    border_x = g.math("MAXIMUM",
                      g.remap(g.math("ABSOLUTE", g.math("SUBTRACT", x, -0.035)), 0.004, 0.0012),
                      g.remap(g.math("ABSOLUTE", g.math("SUBTRACT", x, 0.035)), 0.004, 0.0012))
    border = g.math("MAXIMUM", g.math("MULTIPLY", border_z, xgate),
                    g.math("MULTIPLY", border_x, g.remap(z, 0.058, 0.062)))
    ink_noise = g.noise(g.vmath("ADD", p, (9.0, 2.0, 1.0)), scale=95.0, detail=4.0).outputs["Fac"]
    broken = g.remap(ink_noise, 0.37, 0.66, 0.15, 1.0)
    ink_mask = g.math("MULTIPLY", front, g.math("MULTIPLY", g.math("MAXIMUM", bands, border), broken))
    base = g.mix(base, (0.055, 0.046, 0.035), ink_mask)
    g.set("Base Color", base)
    g.set("Roughness", g.math("ADD", 0.78, g.math("MULTIPLY", stain, 0.14)))
    height = g.math("ADD", g.math("MULTIPLY", fibre, 0.25), g.math("MULTIPLY", edge, -0.35))
    g.set("Normal", g.bump(height, strength=0.32, distance=0.00055))
    return g.mat


def build(kit):
    m = kit.mats
    tin = m.tinplate("M_OilFlaskTinplate", base=(0.44, 0.44, 0.40), rust=0.20, seed=6.0)
    cork = m.cork("M_OilFlaskCork", seed=4.0)
    paper = label_paper(kit)
    solder = m.tinplate("M_OilFlaskSolder", base=(0.52, 0.51, 0.46), rust=0.12, seed=9.0)
    for mat in (tin, cork, paper, solder):
        mat.node_tree.nodes["Principled BSDF"].inputs["Subsurface Weight"].default_value = 0.0

    body_levels = [(0.004, 0.033, 0.019), (0.011, 0.043, 0.025), (0.025, 0.046, 0.027),
                   (0.105, 0.046, 0.027), (0.123, 0.036, 0.020), (0.132, 0.016, 0.016)]
    body = oval_loft(kit, "FlattenedTinBody", body_levels, tin, sides=48, cap_bottom=True, cap_top=True)

    def body_relief(co, pco):
        relief = 0.00055 * noise.noise(Vector((pco.x * 75, pco.y * 95, pco.z * 34)))
        # Broad shallow dents pressed into the front sheet, not separate blobs.
        for cx, cz, sx, sz, depth in ((0.028, 0.108, 0.020, 0.018, 0.0012),
                                      (-0.030, 0.036, 0.017, 0.014, 0.0009)):
            front = smoothstep(-0.006, -0.022, co.y)
            d = ((co.x - cx) / sx) ** 2 + ((co.z - cz) / sz) ** 2
            relief -= depth * math.exp(-d) * front
        return relief

    kit.displace(body, body_relief)

    parts = [body,
             oval_tube(kit, "BottomFoldedRim", 0.043, 0.025, 0.011, 0.0016, solder),
             oval_tube(kit, "ShoulderSolderRim", 0.039, 0.023, 0.119, 0.0012, solder),
             kit.cylinder("TinNeck", 0.014, 0.020, (0, 0, 0.136), material=tin, sides=28,
                          radius_top=0.0125, bevel=0.0008, bevel_segments=1),
             kit.cylinder("NeckRolledLip", 0.016, 0.0042, (0, 0, 0.146), material=solder, sides=28,
                          bevel=0.0007, bevel_segments=1),
             kit.cylinder("CorkStopper", 0.0118, 0.018, (0, 0, 0.155), material=cork, sides=24,
                          radius_top=0.0105, bevel=0.0012, bevel_segments=2),
             label_band(kit, paper)]

    # Soldered vertical back seam and a couple of dents.
    parts.append(kit.tube("BackSolderSeam", [Vector((0, 0.0279, 0.014)), Vector((0, 0.0284, 0.116))],
                          radius=0.0012, sides=10, material=solder))
    flask = kit.join(parts, "SM_OilFlask", pivot="base", unwrap=False, reshade=True, smooth_angle=50)
    return flask
