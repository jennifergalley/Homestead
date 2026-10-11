"""Beach campfire kit (BeachCampfire): a ring of sea-worn granite cobbles with an ash bed, a driftwood
tepee laid ready, the same stack burning, its ember bed and the flame tongues.

    .\\Scripts\\Blender\\New-Prop.ps1 beach_campfire

Real-object research (written before modeling):
- A beach fire above the strandline on a granite coast is ringed with whatever cobbles lie to hand:
  rounded to well-rounded granite "boulders" of 15-25 cm, flattened (discoidal) by surf, polished
  smooth with fine crescent percussion marks, often carrying feldspar megacrysts (Cornish granite).
  They are bedded a third to half into the dry sand. Inner faces soot black-brown and take a
  rust-buff heat tinge; the outer faces stay clean, with dry sand clinging round the sand line.
- Inside the ring: a shallow bed of grey-white wood ash (light, powdery, crusted), black charcoal
  crumbs and lumps, and a couple of charred driftwood stubs left from earlier fires.
- Driftwood on such a coast is sea-bleached silver-grey to pale buff, bark long gone, the soft
  earlywood eroded so the grain stands as ridges, ends rounded by tumbling, with long drying checks
  and sand in the crevices. Branch pieces of 3-7 cm thickness and 30-60 cm length are what gets laid.
- A tepee lay: 6-8 sticks leaning together over a tuft of tinder. Dry marram blades (pale straw,
  inrolled) and dried bladderwrack (black-brown crinkly straps with salt bloom) both serve.
- Burning, a tepee collapses inward within half an hour into a low star of half-burnt sticks whose
  inner ends sit on the coal bed. Wood that is burning is black with "alligator" char (blocks 1-2 cm
  split across and along the grain), ash-white on the hottest blocks; behind the char front the wood
  is scorched brown, and the outer ends often stay pale. Ends in the coals glow orange-red.
- Flames off driftwood are yellow-orange at the base, deep orange to red at the tips, 35-70 cm tall
  over a small fire.

Kit layout (Unreal local cm; Blender Y imports as Unreal -Y): every mesh shares ONE origin, the
bottom centre of the pit at sand level (0, 0, 0). Nothing is re-pivoted, so placing all five meshes
at the same transform composes the fire; per-mesh offsets are all zero. Ring ~96 cm across; stones
sink up to 3.5 cm below the sand line. Unlit state: SM_BeachFirePit + SM_CampfireStack.
Lit state: SM_BeachFirePit + SM_CampfireBurning + SM_CampfireEmbers + SM_CampfireFlames.
Everything is generated here: no scanned or downloaded geometry or textures.
"""
import math
import os
import random
import sys
from pathlib import Path

import bmesh
import bpy
from mathutils import Matrix, Vector, noise as mnoise

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import homestead_rocks as rocks  # noqa: E402
from homestead_materials import Graph  # noqa: E402

NAME = "BeachCampfire"
DESCRIPTION = ("Beach campfire kit on a shared origin (pit bottom centre at sand level): granite cobble ring with "
               "ash bed, unlit driftwood tepee with marram/wrack tinder, burning collapsed stack, emissive ember "
               "bed and flame tongues (original).")
COLLISION = "none"
TRIANGLE_BUDGET = 12000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
DRAFT = os.environ.get("HOMESTEAD_DRAFT") == "1"
BAKE = {"size": 1024 if DRAFT else 2048, "samples": 16 if DRAFT else 96}
BAKE_MESHES = {"SM_BeachFirePit": {"repack": False}, "SM_CampfireStack": True, "SM_CampfireBurning": True}
BEAUTY = {"pose": (0, 0, 0), "ground": "origin", "views": ["hero", "detail"], "meshes": {
    "SM_BeachFirePit": {"focus": (0.0, 0.30, 0.06), "detail_distance": 0.75},
    "SM_CampfireStack": {"focus": (0.0, 0.0, 0.12), "detail_distance": 0.7},
    "SM_CampfireBurning": {"focus": (0.0, 0.0, 0.1), "detail_distance": 0.6},
    "SM_CampfireEmbers": {"focus": (0.0, 0.0, 0.05), "detail_distance": 0.5},
    "SM_CampfireFlames": {"focus": (0.0, 0.0, 0.3)},
}}

SEED = 1851
RING_RADIUS = 0.385          # stone centres (m)
STONE_COUNT = 10
STONE_CUT_Z = -0.035         # stones are trimmed flat this far below the sand line (hidden)
FIRE_CENTRE = (0.0, 0.0, 0.08)
STONE_TRIS = 750
FLAME_SIDES, FLAME_RINGS = 16, 16

REPORT = {
    "unreal_frame": "Unreal local cm: Blender (x, y, z) imports as Unreal (x, -y, z).",
    "pivot": "All five meshes share one origin: bottom centre of the pit at sand level (0, 0, 0). No mesh is "
             "re-pivoted; place them all at the same transform.",
    "mesh_offsets_cm": {name: [0, 0, 0] for name in (
        "SM_BeachFirePit", "SM_CampfireStack", "SM_CampfireBurning", "SM_CampfireEmbers", "SM_CampfireFlames")},
    "states": {"unlit": ["SM_BeachFirePit", "SM_CampfireStack"],
               "lit": ["SM_BeachFirePit", "SM_CampfireBurning", "SM_CampfireEmbers", "SM_CampfireFlames"]},
    "collision_note": "No collision on any part (walkable, ankle-high ring; the heroine stands at its edge).",
    "flames_uv": ("SM_CampfireFlames: closed tapered tongues; UV V runs 0 at each tongue's base to 1 at its tip, "
                  "U runs around the tongue inside that tongue's own strip [k/N, (k+1)/N]."),
    "placement": "Set on flat sand at terrain height; the cobbles extend 3.5 cm below the origin.",
}


def smoothstep(a, b, x):
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3.0 - 2.0 * t)


def ash_height(x, y):
    """Top of the ash bed (m): a low heap in the middle, ~1 cm at the ring, tucking under the stones."""
    r = math.hypot(x, y)
    base = 0.006 + 0.02 * math.exp(-(r / 0.15) ** 2)
    wobble = 0.025 * mnoise.noise(Vector((x * 5.0, y * 5.0, 9.7)))
    edge = -0.022 * smoothstep(0.25, 0.35, r + wobble)
    lumps = 0.005 * mnoise.noise(Vector((x * 9.0, y * 9.0, 1.3))) + 0.0025 * mnoise.noise(Vector((x * 24.0, y * 24.0, 4.2))) + \
        0.0012 * mnoise.noise(Vector((x * 41.0, y * 41.0, 7.1)))
    return base + edge + lumps


def triangles(obj):
    return sum(len(p.vertices) - 2 for p in obj.data.polygons)


def _decimate(kit, obj, target):
    if triangles(obj) <= target:
        return obj
    mod = obj.modifiers.new("Decimate", "DECIMATE")
    mod.ratio = target / triangles(obj)
    mod.use_collapse_triangulate = True
    return kit.apply_modifiers(obj)


def _set_vector_attr(obj, name, value):
    attr = obj.data.attributes.get(name) or obj.data.attributes.new(name, "FLOAT_VECTOR", "POINT")
    attr.data.foreach_set("vector", list(value) * len(obj.data.vertices))


def _shrink_uvs(obj, material_name, factor, margin=0.003):
    """Scale the UVs of faces using ``material_name`` about their centre by ``factor`` and repack,
    so a large flat part (the ash bed) doesn't starve the cobbles of texels."""
    index = next(i for i, slot in enumerate(obj.material_slots) if slot.material and slot.material.name == material_name)
    layer = obj.data.uv_layers.active.data
    loops = [li for poly in obj.data.polygons if poly.material_index == index for li in poly.loop_indices]
    cu = sum(layer[li].uv.x for li in loops) / len(loops)
    cv = sum(layer[li].uv.y for li in loops) / len(loops)
    for li in loops:
        uv = layer[li].uv
        layer[li].uv = (cu + (uv.x - cu) * factor, cv + (uv.y - cv) * factor)
    rocks._select(obj)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.select_all(action="SELECT")
    bpy.ops.uv.pack_islands(margin=margin, rotate=True)
    bpy.ops.object.mode_set(mode="OBJECT")


def _blob(kit, name, centre, radii, axis, seed, material, subdiv=1, jitter=0.22):
    """Angular lump (coal, ember) from a jittered icosphere, its local Z along ``axis``."""
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=subdiv, radius=1.0)
    offset = Vector((seed * 3.17, seed * 1.71, seed * 2.33))
    for vert in bm.verts:
        n = vert.co.normalized()
        bump = 1.0 + jitter * mnoise.noise(n * 1.7 + offset) + jitter * 0.5 * mnoise.noise(n * 4.1 + offset)
        vert.co = Vector((n.x * radii[0], n.y * radii[1], n.z * radii[2])) * bump
    data = bpy.data.meshes.new(name)
    bm.to_mesh(data)
    bm.free()
    data.uv_layers.new(name="UVMap")
    obj = bpy.data.objects.new(name, data)
    bpy.context.scene.collection.objects.link(obj)
    data.materials.append(material)
    rot = Vector((0, 0, 1)).rotation_difference(Vector(axis).normalized()).to_matrix().to_4x4()
    spin = Matrix.Rotation(seed * 2.399, 4, "Z")
    data.transform(Matrix.Translation(Vector(centre)) @ rot @ spin)
    kit.tag_coords(data)
    return obj


# ------------------------------------------------------------------ materials

class _Overlay(Graph):
    """A Graph on an existing material, for layering effects over a library material."""

    def __init__(self, mat):  # noqa: super().__init__ would clear the tree
        self.mat = mat
        self.tree = mat.node_tree
        self.out = next(n for n in self.tree.nodes if n.type == "OUTPUT_MATERIAL")
        self.bsdf = next(n for n in self.tree.nodes if n.type == "BSDF_PRINCIPLED")

    def current(self, socket_name):
        socket = self.bsdf.inputs[socket_name]
        return socket.links[0].from_socket if socket.is_linked else socket.default_value


def _fmix(g, fac, a, b):
    return g.node("ShaderNodeMix", data_type="FLOAT", Factor=fac, A=a, B=b).outputs[0]


def stone_material(kit):
    """Surf-rounded Cornish granite (feldspar megacrysts), sooted on the faces toward the fire,
    a rust-buff heat tinge round the soot, sand at the foot outside and ash dust inside."""
    mat = kit.mats.granite("M_FirePitStone", grain=0.0032, scale=0.22, patina=0.35, lichen=0.05, moss=0.0,
                           iron=0.2, streaks=0.0, soil=0.0, enclaves=0.25, megacrysts=0.55, film=0.25,
                           spots=0.2, seed=7.0, offset_attr="stone_offset", tint_attr="stone_tint")
    g = _Overlay(mat)
    color, rough = g.current("Base Color"), g.current("Roughness")
    p = g.coord()
    px, py, pz = g.separate(p)
    normal = g.node("ShaderNodeTexCoord").outputs["Normal"]
    radial = g.vmath("NORMALIZE", g.combine(px, py, 0.0))
    inward = g.math("MULTIPLY", g.vmath("DOT_PRODUCT", normal, radial), -1.0)
    blot = g.noise(p, scale=11.0, detail=5.0, roughness=0.6).outputs["Fac"]
    tinge = g.math("MULTIPLY", g.remap(inward, -0.35, 0.25), g.remap(pz, 0.0, 0.03))
    color = g.mix(color, (0.86, 0.70, 0.55), g.math("MULTIPLY", tinge, 0.7), blend="MULTIPLY")
    soot = g.math("MULTIPLY", g.remap(inward, -0.2, 0.4), g.remap(pz, -0.004, 0.015))
    soot = g.math("MULTIPLY", soot, g.remap(pz, 0.16, 0.07, 0.7, 1.0))
    soot = g.math("MULTIPLY", soot, g.remap(blot, 0.25, 0.6, 0.55, 1.0))
    soot.node.label = "CAMPFIRE_SOOT"
    sooty = g.mix((0.055, 0.045, 0.036), (0.018, 0.016, 0.014), g.remap(blot, 0.35, 0.65))
    color = g.mix(color, sooty, g.math("MULTIPLY", soot, 0.95))
    grit = g.noise(p, scale=380.0, detail=2.0).outputs["Fac"]
    jitter = g.noise(p, scale=28.0, detail=3.0).outputs["Fac"]
    line = g.math("ADD", pz, g.math("MULTIPLY", g.math("SUBTRACT", jitter, 0.5), 0.016))
    foot = g.remap(line, 0.014, 0.0)
    sand = g.mix((0.40, 0.345, 0.25), (0.25, 0.21, 0.15), g.remap(grit, 0.35, 0.7))
    dust = g.mix(sand, (0.33, 0.32, 0.30), g.remap(inward, -0.1, 0.4))
    color = g.mix(color, dust, foot)
    g.set("Base Color", color)
    rough = g.math("MULTIPLY", rough, 0.9)
    g.set("Roughness", _fmix(g, g.math("MAXIMUM", soot, foot), rough, 0.95))
    return mat


def driftwood_material(name, char=None, seed=0.0):
    """Sea-bleached driftwood: silver-grey with eroded ridged grain, drying checks, sand in the
    grooves and ash dust where it lies in the pit. ``char`` = (full_m, none_m) chars it within
    ``full_m`` of the fire centre (object space) fading to scorched brown by ``none_m``; ``"all"``
    chars everything (charcoal). Char is alligator-cracked, with an ash-white skin on hot blocks."""
    g = Graph(name)
    p = g.vmath("ADD", g.coord(), (seed * 1.37, seed * 2.11, seed * 0.73))
    x, y, z = g.separate(p)
    obj = g.node("ShaderNodeTexCoord").outputs["Object"]
    ox, oy, oz = g.separate(obj)
    warp = g.noise(p, scale=7.0, detail=2.0).outputs["Fac"]
    fib_vec = g.combine(g.math("ADD", g.math("MULTIPLY", x, 70.0), g.math("MULTIPLY", warp, 2.0)),
                        g.math("MULTIPLY", y, 70.0), g.math("MULTIPLY", z, 3.5))
    fibres = g.noise(fib_vec, scale=2.0, detail=8.0, roughness=0.62).outputs["Fac"]
    groove = g.remap(g.math("ABSOLUTE", g.math("SUBTRACT", fibres, 0.5)), 0.0, 0.05, 1.0, 0.0)
    tone = g.noise(g.combine(g.math("MULTIPLY", x, 14.0), g.math("MULTIPLY", y, 14.0),
                             g.math("MULTIPLY", z, 1.2)), scale=2.0, detail=4.0).outputs["Fac"]
    color = g.ramp(g.math("ADD", g.math("MULTIPLY", fibres, 0.6), g.math("MULTIPLY", tone, 0.4)),
                   [(0.28, (0.15, 0.14, 0.125)), (0.5, (0.26, 0.25, 0.23)), (0.72, (0.34, 0.33, 0.31))])
    streaks = g.noise(g.combine(g.math("MULTIPLY", x, 40.0), g.math("MULTIPLY", y, 40.0),
                                g.math("MULTIPLY", z, 1.6)), scale=1.5, detail=5.0).outputs["Fac"]
    color = g.mix(color, (0.15, 0.135, 0.11), g.remap(streaks, 0.52, 0.7, 0.0, 0.75))
    warm = g.noise(obj, scale=3.5, detail=3.0).outputs["Fac"]
    color = g.mix(color, (0.31, 0.25, 0.18), g.remap(warm, 0.55, 0.72, 0.0, 0.6))
    checks = g.voronoi(g.combine(g.math("MULTIPLY", x, 30.0), g.math("MULTIPLY", y, 30.0),
                                 g.math("MULTIPLY", z, 0.9)), scale=1.6, feature="DISTANCE_TO_EDGE")
    crack = g.math("MULTIPLY", g.remap(checks.outputs["Distance"], 0.0, 0.018, 1.0, 0.0),
                   g.remap(g.noise(p, scale=6.0).outputs["Fac"], 0.45, 0.6))
    color = g.mix(color, (0.07, 0.065, 0.058), g.math("MULTIPLY", groove, 0.7))
    color = g.mix(color, (0.035, 0.03, 0.026), crack)
    sandy = g.noise(p, scale=160.0, detail=2.0).outputs["Fac"]
    color = g.mix(color, (0.40, 0.345, 0.25),
                  g.math("MULTIPLY", g.math("MAXIMUM", groove, crack), g.remap(sandy, 0.55, 0.7, 0.0, 0.7)))
    dust = g.math("MULTIPLY", g.remap(oz, 0.045, 0.012), g.remap(g.noise(obj, scale=30.0).outputs["Fac"], 0.35, 0.6))
    color = g.mix(color, (0.36, 0.355, 0.34), g.math("MULTIPLY", dust, 0.8))
    rough = g.remap(fibres, 0.3, 0.7, 0.82, 0.93)
    height = g.math("SUBTRACT", g.math("MULTIPLY", fibres, 0.6),
                    g.math("ADD", g.math("MULTIPLY", groove, 0.5), g.math("MULTIPLY", crack, 1.2)))
    if char is not None:
        if char == "all":
            charm, heat = 1.0, 0.6
        else:
            full, none = char
            d = g.vmath("DISTANCE", obj, FIRE_CENTRE)
            front = g.noise(obj, scale=9.0, detail=4.0).outputs["Fac"]
            d = g.math("ADD", d, g.math("MULTIPLY", g.math("SUBTRACT", front, 0.5), 0.09))
            charm = g.remap(d, none, full)
            heat = g.remap(d, full + 0.03, full - 0.07)
            scorch = g.remap(d, none + 0.05, none - 0.02)
            color = g.mix(color, (0.05, 0.03, 0.017), g.math("MULTIPLY", scorch, 0.92))
            rough = _fmix(g, scorch, rough, 0.8)
        # Alligator char: blocks split across and along the grain (Chebychev cells in the
        # unrolled surface: arc length around x along the grain).
        jit = g.noise(p, scale=60.0, detail=2.0).outputs["Color"]
        rx, ry, _ = g.separate(g.coord())
        arc = g.math("MULTIPLY", g.math("ARCTAN2", ry, rx), 0.025)
        flat = g.vmath("ADD", g.combine(g.math("MULTIPLY", arc, 1.0 / 0.011), g.math("MULTIPLY", z, 1.0 / 0.016), seed),
                       g.scale(g.vmath("SUBTRACT", jit, (0.5, 0.5, 0.5)), 0.5))
        f1 = g.node("ShaderNodeTexVoronoi", Vector=flat, Scale=1.0, Randomness=0.6, feature="F1",
                    distance="CHEBYCHEV", voronoi_dimensions="3D")
        f2 = g.node("ShaderNodeTexVoronoi", Vector=flat, Scale=1.0, Randomness=0.6, feature="F2",
                    distance="CHEBYCHEV", voronoi_dimensions="3D")
        gap = g.math("SUBTRACT", f2.outputs["Distance"], f1.outputs["Distance"])
        cell = g.channel(f1.outputs["Color"], 0)
        crack_c = g.remap(gap, 0.0, 0.1, 1.0, 0.0)
        dome = g.remap(gap, 0.0, 0.45)
        sheen = g.noise(p, scale=150.0, detail=3.0).outputs["Fac"]
        coal = g.mix((0.014, 0.013, 0.012), (0.04, 0.038, 0.036),
                     g.remap(g.math("ADD", g.math("MULTIPLY", cell, 0.6), g.math("MULTIPLY", sheen, 0.4)), 0.3, 0.8))
        coal = g.mix(coal, (0.003, 0.003, 0.003), crack_c)
        ash_n = g.noise(p, scale=55.0, detail=4.0).outputs["Fac"]
        ash = g.remap(g.math("ADD", g.math("MULTIPLY", ash_n, 0.6), g.math("MULTIPLY", cell, 0.4)), 0.56, 0.66)
        ash = g.math("MULTIPLY", g.math("MULTIPLY", ash, heat), g.math("SUBTRACT", 1.0, crack_c))
        ash_col = g.mix((0.24, 0.235, 0.225), (0.40, 0.395, 0.38), g.remap(sheen, 0.4, 0.65))
        coal = g.mix(coal, ash_col, g.math("MULTIPLY", ash, 0.92))
        color = g.mix(color, coal, charm)
        rough = _fmix(g, charm, rough, _fmix(g, ash, g.remap(sheen, 0.3, 0.7, 0.5, 0.75), 0.95))
        height = _fmix(g, charm, height, g.math("SUBTRACT", g.math("MULTIPLY", dome, 0.9),
                                                g.math("MULTIPLY", crack_c, 1.4)))
    tint = g.node("ShaderNodeAttribute", attribute_name="piece_tint", attribute_type="GEOMETRY").outputs["Color"]
    color = g.mix(color, tint, 1.0, blend="MULTIPLY")
    g.set("Base Color", color)
    g.set("Roughness", rough)
    g.set("Normal", g.bump(height, strength=0.8, distance=0.003))
    return g.mat


def ash_material():
    """Grey-white wood ash with darker drifts, charcoal crumbs and a burnt-sand rim."""
    g = Graph("M_FirePitAsh")
    p = g.node("ShaderNodeTexCoord").outputs["Object"]
    px, py, pz = g.separate(p)
    drift = g.noise(p, scale=6.0, detail=6.0, roughness=0.6).outputs["Fac"]
    powder = g.noise(p, scale=140.0, detail=3.0).outputs["Fac"]
    color = g.ramp(g.math("ADD", g.math("MULTIPLY", drift, 0.75), g.math("MULTIPLY", powder, 0.25)),
                   [(0.3, (0.07, 0.066, 0.062)), (0.45, (0.17, 0.165, 0.158)), (0.58, (0.27, 0.265, 0.255)),
                    (0.7, (0.37, 0.365, 0.35))])
    # Charcoal crumbs and burnt-sand grains: irregular thresholded fBm at three sizes.
    crumb = None
    for scale, lo, amount in ((38.0, 0.7, 1.0), (95.0, 0.72, 0.9), (260.0, 0.74, 0.7)):
        n = g.noise(p, scale=scale, detail=3.0, roughness=0.65, distortion=0.4).outputs["Fac"]
        m = g.math("MULTIPLY", g.remap(n, lo, lo + 0.035), amount)
        crumb = m if crumb is None else g.math("MAXIMUM", crumb, m)
    patch = g.noise(p, scale=4.0, detail=3.0).outputs["Fac"]
    crumb = g.math("MULTIPLY", crumb, g.remap(patch, 0.35, 0.6, 0.35, 1.0))
    r = g.vmath("LENGTH", g.combine(px, py, 0.0))
    # Grey-white ash heaped in the middle; round it the sand is blackened with soot and char
    # crumbs, with drifts of ash blown across it, fading to clean sand under the stones.
    heap = g.remap(g.math("ADD", r, g.math("MULTIPLY", g.math("SUBTRACT", drift, 0.5), 0.14)), 0.24, 0.12)
    streak = g.noise(p, scale=9.0, detail=5.0, roughness=0.6).outputs["Fac"]
    blown = g.math("MULTIPLY", g.remap(streak, 0.56, 0.72), 0.6)
    burnt = g.mix((0.035, 0.031, 0.027), (0.10, 0.088, 0.07), g.remap(powder, 0.3, 0.75))
    floor = g.mix(burnt, color, g.math("MAXIMUM", heap, blown))
    rim = g.remap(g.math("ADD", r, g.math("MULTIPLY", g.math("SUBTRACT", drift, 0.5), 0.07)), 0.23, 0.31)
    sand = g.mix((0.36, 0.31, 0.225), (0.24, 0.20, 0.145), g.remap(powder, 0.35, 0.7))
    color = g.mix(floor, sand, rim)
    brown_n = g.noise(p, scale=170.0, detail=2.0).outputs["Fac"]
    color = g.mix(color, (0.12, 0.075, 0.04), g.math("MULTIPLY", g.remap(brown_n, 0.7, 0.74), 0.8))
    color = g.mix(color, (0.016, 0.015, 0.014), g.math("MULTIPLY", crumb, g.math("SUBTRACT", 1.0, g.math("MULTIPLY", rim, 0.7))))
    g.set("Base Color", color)
    g.set("Roughness", g.remap(powder, 0.3, 0.7, 0.88, 0.98))
    height = g.math("ADD", g.math("MULTIPLY", drift, 0.4), g.math("MULTIPLY", powder, 0.35))
    height = g.math("ADD", height, g.math("MULTIPLY", crumb, 0.6))
    crust = g.voronoi(g.vmath("ADD", p, g.scale(g.noise(p, scale=12.0).outputs["Color"], 0.02)), scale=28.0,
                      feature="DISTANCE_TO_EDGE").outputs["Distance"]
    height = g.math("SUBTRACT", height, g.math("MULTIPLY", g.remap(crust, 0.0, 0.05, 1.0, 0.0),
                                               g.math("MULTIPLY", heap, 0.5)))
    g.set("Normal", g.bump(height, strength=0.85, distance=0.004))
    return g.mat


def grass_material():
    """Dry marram blades: inrolled straw, pale to grey-brown, fine longitudinal streaks."""
    g = Graph("M_KindlingMarram")
    p = g.coord()
    x, y, z = g.separate(p)
    obj = g.node("ShaderNodeTexCoord").outputs["Object"]
    streak = g.noise(g.combine(g.math("MULTIPLY", x, 500.0), g.math("MULTIPLY", y, 500.0),
                               g.math("MULTIPLY", z, 5.0)), scale=2.0, detail=4.0).outputs["Fac"]
    blade = g.noise(obj, scale=22.0, detail=2.0).outputs["Fac"]
    color = g.ramp(blade, [(0.3, (0.24, 0.20, 0.135)), (0.48, (0.42, 0.36, 0.235)),
                           (0.62, (0.48, 0.43, 0.30)), (0.75, (0.31, 0.30, 0.20))])
    color = g.mix(color, (0.7, 0.66, 0.6), g.remap(streak, 0.4, 0.65, 0.0, 0.6), blend="MULTIPLY")
    tip = g.remap(z, 0.12, 0.3)
    color = g.mix(color, (0.30, 0.26, 0.20), g.math("MULTIPLY", tip, 0.5))
    g.set("Base Color", color)
    g.set("Roughness", g.remap(streak, 0.3, 0.7, 0.55, 0.7))
    g.set("Normal", g.bump(streak, strength=0.3, distance=0.0005))
    return g.mat


def wad_material():
    """A crumpled wad of dry marram: straw strands running every way, dark gaps between them."""
    g = Graph("M_KindlingWad")
    p = g.node("ShaderNodeTexCoord").outputs["Object"]
    strands = None
    for k, stretch in enumerate(((260.0, 260.0, 12.0), (12.0, 260.0, 260.0), (260.0, 12.0, 260.0),
                                 (180.0, 30.0, 180.0), (30.0, 180.0, 180.0), (180.0, 180.0, 30.0))):
        warp = g.noise(p, scale=14.0 + 5 * k, detail=2.0).outputs["Color"]
        v = g.vmath("MULTIPLY", g.vmath("ADD", p, g.scale(warp, 0.03)), stretch)
        n = g.noise(v, scale=1.0, detail=2.0).outputs["Fac"]
        ridge = g.remap(g.math("ABSOLUTE", g.math("SUBTRACT", n, 0.5)), 0.0, 0.065, 1.0, 0.0)
        strands = ridge if strands is None else g.math("MAXIMUM", strands, ridge)
    blade = g.noise(p, scale=30.0, detail=2.0).outputs["Fac"]
    straw = g.ramp(blade, [(0.3, (0.24, 0.20, 0.135)), (0.5, (0.42, 0.36, 0.235)),
                           (0.65, (0.47, 0.42, 0.29)), (0.78, (0.30, 0.29, 0.19))])
    color = g.mix((0.07, 0.058, 0.04), straw, g.math("MULTIPLY", strands, 0.9))
    g.set("Base Color", color)
    g.set("Roughness", 0.7)
    g.set("Normal", g.bump(strands, strength=0.9, distance=0.003))
    return g.mat


def wrack_material():
    """Dried bladderwrack: olive-black crinkled straps with a white salt bloom."""
    g = Graph("M_KindlingWrack")
    p = g.node("ShaderNodeTexCoord").outputs["Object"]
    tone = g.noise(p, scale=60.0, detail=5.0).outputs["Fac"]
    color = g.mix((0.035, 0.03, 0.018), (0.10, 0.08, 0.04), g.remap(tone, 0.35, 0.7))
    salt = g.voronoi(p, scale=420.0, feature="F1")
    bloom = g.math("MULTIPLY", g.remap(salt.outputs["Distance"], 0.22, 0.1),
                   g.remap(g.noise(p, scale=25.0).outputs["Fac"], 0.58, 0.7))
    color = g.mix(color, (0.42, 0.41, 0.38), g.math("MULTIPLY", bloom, 0.85))
    g.set("Base Color", color)
    g.set("Roughness", _fmix(g, bloom, g.remap(tone, 0.3, 0.7, 0.45, 0.65), 0.9))
    g.set("Normal", g.bump(g.math("ADD", tone, bloom), strength=0.4, distance=0.001))
    return g.mat


def ember_material():
    """Glowing coals for review (the game may override it): dark crusted blocks with orange-red
    cracks, some blocks glowing through, grey ash skin on cooler crusts."""
    g = Graph("M_CampfireEmbers")
    p = g.node("ShaderNodeTexCoord").outputs["Object"]
    warp = g.noise(p, scale=30.0, detail=3.0).outputs["Color"]
    pv = g.vmath("ADD", p, g.scale(g.vmath("SUBTRACT", warp, (0.5, 0.5, 0.5)), 0.014))
    edge = g.voronoi(pv, scale=1.0 / 0.012, feature="DISTANCE_TO_EDGE").outputs["Distance"]
    crack = g.math("MULTIPLY", g.remap(edge, 0.0, 0.05, 1.0, 0.0),
                   g.remap(g.noise(p, scale=35.0, detail=2.0).outputs["Fac"], 0.42, 0.55))
    glow_n = g.noise(p, scale=18.0, detail=5.0, roughness=0.6).outputs["Fac"]
    glow_area = g.remap(glow_n, 0.5, 0.66)
    skin = g.math("MULTIPLY", g.remap(g.noise(p, scale=45.0, detail=4.0).outputs["Fac"], 0.52, 0.64),
                  g.math("SUBTRACT", 1.0, glow_area))
    hot = g.math("MULTIPLY", g.math("MAXIMUM", g.math("MULTIPLY", crack, 0.85), glow_area),
                 g.math("SUBTRACT", 1.0, g.math("MULTIPLY", skin, 0.85)))
    base = g.mix((0.025, 0.023, 0.021), (0.32, 0.31, 0.30), skin)
    g.set("Base Color", base)
    g.set("Roughness", 0.85)
    heat = g.math("ADD", g.math("MULTIPLY", hot, 0.7), g.math("MULTIPLY", glow_n, 0.3))
    g.set("Emission Color", g.ramp(heat, [(0.0, (0.45, 0.02, 0.003)), (0.45, (1.0, 0.09, 0.008)),
                                          (0.8, (1.0, 0.28, 0.03)), (1.0, (1.0, 0.5, 0.12))]))
    g.set("Emission Strength", g.math("MULTIPLY", hot, 4.0))
    g.mat.diffuse_color = (1.0, 0.25, 0.04, 1.0)
    return g.mat


def flame_material():
    """Review flame: emissive yellow-white base to red tips along UV V, alpha fading at the
    silhouette (facing) and at base and tip. The game applies its own additive flame material."""
    mat = bpy.data.materials.new("M_CampfireFlame")
    mat.use_nodes = True
    tree = mat.node_tree
    tree.nodes.clear()
    new = tree.nodes.new
    out = new("ShaderNodeOutputMaterial")
    uv = new("ShaderNodeUVMap")
    uv.uv_map = "UVMap"
    sep = new("ShaderNodeSeparateXYZ")
    tree.links.new(uv.outputs["UV"], sep.inputs["Vector"])
    v = sep.outputs["Y"]
    ramp = new("ShaderNodeValToRGB")
    stops = [(0.0, (1.0, 0.62, 0.22)), (0.18, (1.0, 0.48, 0.1)), (0.5, (1.0, 0.26, 0.035)),
             (0.8, (0.9, 0.11, 0.015)), (1.0, (0.5, 0.04, 0.008))]
    while len(ramp.color_ramp.elements) < len(stops):
        ramp.color_ramp.elements.new(0.5)
    for element, (pos, col) in zip(ramp.color_ramp.elements, stops):
        element.position, element.color = pos, (*col, 1.0)
    tree.links.new(v, ramp.inputs["Fac"])
    emit = new("ShaderNodeEmission")
    tree.links.new(ramp.outputs["Color"], emit.inputs["Color"])
    emit.inputs["Strength"].default_value = 5.0
    weight = new("ShaderNodeLayerWeight")
    weight.inputs["Blend"].default_value = 0.4

    def math(op, a, b=0.0, clamp=True):
        node = new("ShaderNodeMath")
        node.operation, node.use_clamp = op, clamp
        for socket, value in zip(node.inputs, (a, b)):
            if isinstance(value, bpy.types.NodeSocket):
                tree.links.new(value, socket)
            else:
                socket.default_value = value
        return node.outputs[0]

    def ramp_range(value, lo, hi):
        node = new("ShaderNodeMapRange")
        node.interpolation_type = "SMOOTHSTEP"
        tree.links.new(value, node.inputs["Value"])
        node.inputs["From Min"].default_value, node.inputs["From Max"].default_value = lo, hi
        return node.outputs["Result"]

    edge = math("POWER", math("SUBTRACT", 1.0, weight.outputs["Facing"]), 1.6)
    alpha = math("MULTIPLY", math("MULTIPLY", edge, ramp_range(v, 0.0, 0.08)),
                 math("SUBTRACT", 1.0, ramp_range(v, 0.62, 1.0)))
    alpha = math("MULTIPLY", alpha, 0.8)
    clear = new("ShaderNodeBsdfTransparent")
    mix = new("ShaderNodeMixShader")
    tree.links.new(alpha, mix.inputs[0])
    tree.links.new(clear.outputs[0], mix.inputs[1])
    tree.links.new(emit.outputs[0], mix.inputs[2])
    tree.links.new(mix.outputs[0], out.inputs["Surface"])
    mat.diffuse_color = (1.0, 0.45, 0.1, 0.8)
    if hasattr(mat, "surface_render_method"):
        mat.surface_render_method = "BLENDED"
    return mat


# ------------------------------------------------------------------ pit

def stone_layout(rng):
    """Cobbles round the ring, long axes along it, packed by their lengths with small gaps."""
    specs = []
    for _ in range(STONE_COUNT):
        length = rng.uniform(0.16, 0.25)
        specs.append(dict(length=length, width=length * rng.uniform(0.6, 0.88),
                          height=length * rng.uniform(0.42, 0.7), gap=rng.uniform(0.006, 0.04),
                          power=rng.uniform(1.95, 2.7), angular=rng.random() < 0.35))
    circumference = 2 * math.pi * RING_RADIUS
    span = sum(s["length"] * 0.94 + s["gap"] for s in specs)
    scale = circumference / span
    angle = rng.uniform(0, math.tau)
    for spec in specs:
        half = spec["length"] * 0.94 * scale / 2 / RING_RADIUS
        angle += half
        spec["angle"] = angle
        angle += half + spec["gap"] * scale / RING_RADIUS
    return specs


def make_stone(kit, index, spec, rng, mat):
    a = spec["angle"]
    rr = RING_RADIUS + rng.uniform(-0.015, 0.02) + (spec["width"] - 0.13) * 0.3
    h = spec["height"]
    sink = rng.uniform(0.32, 0.45)
    centre = (rr * math.cos(a), rr * math.sin(a), h / 2 - sink * h)
    rot = rocks.rotation(math.degrees(a) + 90 + rng.uniform(-14, 14), rng.uniform(-8, 8), rng.uniform(-10, 4))
    joints = [((0, 0, -1), h / 2 * rng.uniform(0.72, 0.85), 0.03)]
    if spec["angular"]:
        # A sub-angular cobble keeps a worn fracture face or two from the cliff it fell from.
        for _ in range(2):
            n = (rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(-0.2, 0.8))
            joints.append((n, spec["width"] / 2 * rng.uniform(0.7, 0.85), 0.025))
    field = rocks.boulder((spec["length"] / 2, spec["width"] / 2, h / 2), power=spec["power"],
                          lumps=rng.uniform(0.025, 0.055), lump_scale=1.1, seed=SEED + 31 * index,
                          center=centre, rotate=rot, joints=joints)
    field = rocks.cut(field, (0, 0, -1), -STONE_CUT_Z, rounding=0.012)
    obj = rocks.solid(f"Cobble{index}", field, center=centre, subdivisions=5, material=mat,
                      r_max=spec["length"] * 1.5)
    rocks.displace(obj, lambda P, N: rocks.relief(P, N, SEED + 7 * index, [(0.05, 0.0012), (0.015, 0.0003)]))
    _decimate(kit, obj, STONE_TRIS)
    kit.tag_coords(obj.data)
    _set_vector_attr(obj, "stone_offset", (rng.uniform(-5, 5), rng.uniform(-5, 5), rng.uniform(-5, 5)))
    tint = rng.choice(((1.06, 0.97, 0.9), (0.92, 0.92, 0.93), (0.8, 0.79, 0.78), (1.0, 0.96, 0.9),
                       (0.88, 0.86, 0.84)))
    _set_vector_attr(obj, "stone_tint", tint)
    return obj


def make_ash_bed(kit, mat, rings=18, segments=52, radius=0.37):
    verts = [(0.0, 0.0, ash_height(0.0, 0.0))]
    for i in range(1, rings + 1):
        r = radius * (i / rings) ** 0.85
        for j in range(segments):
            a = math.tau * (j + 0.5 * (i % 2)) / segments
            x, y = r * math.cos(a), r * math.sin(a)
            verts.append((x, y, ash_height(x, y)))
    faces = [(0, 1 + (j + 1) % segments, 1 + j)[::-1] for j in range(segments)]
    for i in range(rings - 1):
        r0, r1 = 1 + i * segments, 1 + (i + 1) * segments
        for j in range(segments):
            faces.append((r0 + j, r1 + j, r1 + (j + 1) % segments, r0 + (j + 1) % segments))
    obj = kit.mesh("AshBed", verts, faces, material=mat)
    obj.data.update()
    if obj.data.polygons[0].normal.z < 0:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.reverse_faces(bm, faces=bm.faces[:])
        bm.to_mesh(obj.data)
        bm.free()
        obj.data.update()
    return obj


def make_coals(kit, rng, mat, count=24):
    parts = []
    for k in range(count):
        r = 0.27 * math.sqrt(rng.random()) + 0.02
        a = rng.uniform(0, math.tau)
        x, y = r * math.cos(a), r * math.sin(a)
        size = rng.choice([0.006, 0.008, 0.01, 0.013, 0.016, 0.021, 0.026])
        radii = (size, size * rng.uniform(0.6, 0.9), size * rng.uniform(0.45, 0.7))
        z = ash_height(x, y) + radii[2] * rng.uniform(0.1, 0.5)
        axis = (rng.uniform(-0.3, 0.3), rng.uniform(-0.3, 0.3), 1.0)
        coal = _blob(kit, f"Coal{k}", (x, y, z), radii, axis, k + 3, mat, subdiv=3, jitter=0.2)
        _decimate(kit, coal, 48 if size < 0.012 else 90)
        _set_vector_attr(coal, "piece_tint", (1.0, 1.0, 1.0))
        parts.append(coal)
    return parts


# ------------------------------------------------------------------ driftwood

def driftwood(kit, name, start, end, radius, seed, mat, sides=14, bend=None, burn=0.0, burn_at="end",
              ring_spacing=0.026, branch=None, tip=None):
    """One sea-worn branch from ``start`` to ``end``: bent in an S, tapering, knobbly with worn knots,
    oval and ridged along the grain. The start is tumbled round; the far end (``tip``) is "round",
    "point" (a worn branch tip) or "broken" (a blunt snapped end). ``burn`` (fraction of length) burns
    an end (``burn_at`` = "end", "start" or "both") down to a ragged char stump. Returns (parts, burnt tips)."""
    rng = random.Random(seed)
    tip = tip or rng.choice(("round", "point", "broken"))
    bend = rng.uniform(0.035, 0.075) if bend is None else bend
    burnt_start = bool(burn) and burn_at in ("start", "both")
    burnt_end = bool(burn) and burn_at in ("end", "both")
    start, end = Vector(start), Vector(end)
    axis = end - start
    length = axis.length
    d = axis / length
    ref = Vector((0, 0, 1)) if abs(d.z) < 0.9 else Vector((1, 0, 0))
    s1 = d.cross(ref).normalized()
    s2 = d.cross(s1).normalized()
    b = rng.uniform(0, math.tau)
    bend_dir = s1 * math.cos(b) + s2 * math.sin(b)
    b2 = b + rng.uniform(1.2, 2.0)
    wave_dir = s1 * math.cos(b2) + s2 * math.sin(b2)
    wave = rng.uniform(0.3, 0.6) * bend
    phase = Vector((rng.uniform(0, 50), rng.uniform(0, 50), rng.uniform(0, 50)))
    taper = rng.uniform(0.6, 0.85)
    n = max(10, int(length / ring_spacing))
    burn_len = burn * length
    pts, radii, arcs = [], [], []
    for i in range(n + 1):
        t = 0.5 - 0.5 * math.cos(math.pi * i / n)
        s = t * length
        envelope = math.sin(math.pi * t)
        off = bend_dir * (bend * length * envelope) + wave_dir * (wave * length * math.sin(math.tau * t))
        off += s1 * (0.025 * length * envelope * mnoise.noise(phase + Vector((t * 3.1, 0, 0))))
        off += s2 * (0.025 * length * envelope * mnoise.noise(phase + Vector((0, t * 3.1, 3))))
        r = radius * (1 - (1 - taper) * t)
        r *= 1 + 0.2 * mnoise.noise(phase + Vector((0, 0, s * 6))) + 0.07 * mnoise.noise(phase + Vector((s * 23, 1, 0)))
        worn = radius * 0.9
        if not burnt_start:
            r *= math.sqrt(max(0.0, 1 - (1 - min(1.0, s / worn)) ** 2))
        if not burnt_end:
            u = length - s
            if tip == "round":
                r *= math.sqrt(max(0.0, 1 - (1 - min(1.0, u / worn)) ** 2))
            elif tip == "point":
                r *= 0.35 + 0.65 * smoothstep(0.0, length * 0.3, u)
                r *= math.sqrt(max(0.0, 1 - (1 - min(1.0, u / (worn * 0.4))) ** 2))
            else:
                r *= 0.82 + 0.18 * min(1.0, u / (radius * 0.25))
        for burnt, u in ((burnt_end, length - s), (burnt_start, s)):
            if burnt:
                f = smoothstep(burn_len, 0.0, u)
                r *= 1 - f * (0.42 + 0.14 * mnoise.noise(phase + Vector((s * 25, 4, 0))))
        pts.append(start + d * s + off)
        radii.append(max(r, radius * 0.16))
        arcs.append(s)
    obj = kit.tube(name, pts, radii=radii, sides=sides, material=mat, roll=rng.uniform(0, math.tau))
    rid = Vector((rng.uniform(0, 40), rng.uniform(0, 40), rng.uniform(0, 40)))
    oval_angle, oval = rng.uniform(0, math.pi), rng.uniform(0.06, 0.13)
    knots = [(rng.uniform(0.15, 0.85) * length, rng.uniform(-math.pi, math.pi), rng.uniform(0.12, 0.22),
              radius * rng.uniform(0.8, 1.4)) for _ in range(rng.randint(1, 3))]

    def relief(co, pco):
        rl = math.hypot(pco.x, pco.y)
        if rl < 1e-6:
            return 0.0
        a = math.atan2(pco.y, pco.x)
        q = Vector((math.cos(a) * 2.4, math.sin(a) * 2.4, pco.z * 3.0)) + rid
        out = rl * (0.10 * mnoise.noise(q) + 0.05 * mnoise.noise(q * 2.7) + oval * math.cos(2 * (a - oval_angle)))
        for ks, ka, kamp, kw in knots:
            da = math.atan2(math.sin(a - ka), math.cos(a - ka))
            out += rl * kamp * math.exp(-((pco.z - ks) / kw) ** 2 - (da / 0.9) ** 2)
        return out
    kit.displace(obj, relief)
    tint = rng.choice(((1.0, 1.0, 1.0), (1.1, 1.07, 1.02), (0.97, 0.9, 0.8), (0.82, 0.8, 0.78), (1.05, 0.98, 0.88)))
    _set_vector_attr(obj, "piece_tint", tint)
    parts = [obj]
    if branch:
        # A short worn side-branch stub.
        t, length_b, angle = branch
        root = pts[min(range(len(pts)), key=lambda k: abs(arcs[k] - t * length))]
        out = (s1 * math.cos(angle) + s2 * math.sin(angle)) * 0.8 + d * 0.6
        out.normalize()
        rb = radius * 0.5
        stub_pts = [root - out * radius * 0.5 + out * (length_b + radius * 0.5) * k / 6 for k in range(7)]
        stub_r = [rb * f for f in (1.0, 1.0, 0.97, 0.93, 0.88, 0.72, 0.35)]
        stub = kit.tube(name + "Stub", stub_pts, radii=stub_r, sides=8, material=mat)
        _set_vector_attr(stub, "piece_tint", tint)
        parts.append(stub)
    tips = []
    if burnt_end:
        tips.append({"point": pts[-1], "dir": d, "radius": radii[-1]})
    if burnt_start:
        tips.append({"point": pts[0], "dir": -d, "radius": radii[0]})
    return parts, tips


def make_stubs(kit, mat):
    """Two charred stubs left from earlier fires, half-sunk in the ash with their burnt ends inward."""
    parts = []
    for k, (a_deg, r_out, r_in, swing, radius, seed) in enumerate(((205, 0.30, 0.15, 18, 0.026, 41),
                                                                    (62, 0.29, 0.17, -40, 0.022, 42))):
        a, s = math.radians(a_deg), math.radians(a_deg + swing)
        p0 = Vector((r_out * math.cos(a), r_out * math.sin(a), 0))
        p1 = Vector((r_in * math.cos(s), r_in * math.sin(s), 0))
        p0.z = ash_height(p0.x, p0.y) + radius * 0.35
        p1.z = ash_height(p1.x, p1.y) + radius * 0.25
        pieces, _ = driftwood(kit, f"Stub{k}", p0, p1, radius, seed, mat, sides=12, bend=0.02, burn=0.45,
                              ring_spacing=0.016)
        parts += pieces
    return parts


# ------------------------------------------------------------------ stacks

def stack_layout(rng):
    """Seven sticks leaning together over the tinder; two shorter ones lean lower."""
    pieces = []
    for i in range(7):
        a = math.tau * i / 7 + rng.uniform(-0.2, 0.2)
        short = i in (2, 5)
        rb = rng.uniform(0.19, 0.24) if not short else rng.uniform(0.2, 0.25)
        radius = rng.uniform(0.017, 0.027) if not short else rng.uniform(0.014, 0.019)
        if i == 0:
            radius = 0.031
        base = Vector((rb * math.cos(a), rb * math.sin(a), 0))
        base.z = ash_height(base.x, base.y) + radius * 0.6
        ap = a + math.pi + rng.uniform(-0.7, 0.7)
        reach = 0.03 if not short else 0.07
        apex = Vector((reach * math.cos(ap), reach * math.sin(ap),
                       rng.uniform(0.37, 0.44) if not short else rng.uniform(0.21, 0.26)))
        direction = (apex - base).normalized()
        length = (apex - base).length + (rng.uniform(0.04, 0.09) if not short else 0.01)
        branch = (rng.uniform(0.35, 0.6), rng.uniform(0.035, 0.06), rng.uniform(0, math.tau)) \
            if i in (0, 3, 6) else None
        pieces.append(dict(start=base, end=base + direction * length, radius=radius, seed=SEED + 100 + i,
                           angle=a, rb=rb, branch=branch))
    return pieces


def make_tinder(kit, rng, grass, wrack, wad):
    """Tinder: a crumpled wad of dry marram with loose blades tangled over it and straying out,
    and a few dried bladderwrack straps draped on top."""
    parts = []
    cz = ash_height(0.0, 0.0)
    centre = Vector((0.0, 0.0, cz + 0.028))
    radii = Vector((0.055, 0.05, 0.036))
    core = _blob(kit, "TinderWad", centre, radii, (0.1, -0.05, 1.0), 5, wad, subdiv=4, jitter=0.3)
    parts.append(_decimate(kit, core, 220))
    for k in range(64):
        th, el = rng.uniform(0, math.tau), rng.uniform(-0.1, 1.2)
        shell = rng.uniform(0.95, 1.06)
        pos = centre + Vector((math.cos(th) * math.cos(el) * radii.x, math.sin(th) * math.cos(el) * radii.y,
                               math.sin(el) * radii.z)) * shell
        heading = Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(-0.6, 0.6))).normalized()
        step = rng.uniform(0.012, 0.019)
        pts = [pos.copy()]
        for i in range(6):
            heading = (heading + Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(-1, 1))) * 0.9).normalized()
            rel = Vector(((pos.x - centre.x) / radii.x, (pos.y - centre.y) / radii.y, (pos.z - centre.z) / radii.z))
            q = rel.length
            n = Vector((rel.x / radii.x, rel.y / radii.y, rel.z / radii.z)).normalized()
            heading = (heading - n * heading.dot(n) + n * (shell - q) * 1.6).normalized()
            pos = pos + heading * step
            pos.z = max(pos.z, ash_height(pos.x, pos.y) + 0.002)
            pts.append(pos.copy())
        widths = [0.0017 * (1 - 0.5 * i / 6) for i in range(7)]
        parts.append(kit.tube(f"Marram{k}", pts, radii=widths, sides=3, material=grass, roll=rng.uniform(0, 6)))
    for k in range(7):
        a = rng.uniform(0, math.tau)
        reach = rng.uniform(0.13, 0.2)
        lift = rng.uniform(0.02, 0.1)
        start = Vector((0.02 * math.cos(a + 2), 0.02 * math.sin(a + 2), cz + 0.02))
        sag = rng.uniform(-0.03, 0.03)
        pts = []
        for i in range(9):
            t = i / 8
            p = start + Vector((math.cos(a) * reach * t, math.sin(a) * reach * t,
                                lift * math.sin(t * 1.4) + sag * t * t))
            p.z = max(p.z, ash_height(p.x, p.y) + 0.002)
            pts.append(p)
        widths = [0.0018 * (1 - 0.6 * i / 8) for i in range(9)]
        parts.append(kit.tube(f"MarramStray{k}", pts, radii=widths, sides=3, material=grass, roll=rng.uniform(0, 6)))
    for k in range(2):
        length = rng.uniform(0.1, 0.14)
        width = rng.uniform(0.007, 0.009)
        ph = rng.uniform(0, 10)
        pts = [Vector((-length / 2 + length * i / 15, 0, 0)) for i in range(16)]
        strap = kit.tube(f"Wrack{k}", pts, radius=width / 2, sides=6, material=wrack)

        def crinkle(co, length=length, ph=ph):
            u = co.x / (length / 2)
            return Vector((co.x, co.y + 0.008 * math.sin(co.x * 19 + ph),
                           co.z * 0.25 + 0.004 * math.sin(co.x * 52 + ph) + 0.002 * mnoise.noise(co * 90)
                           - 0.045 * u * u))
        kit.warp(strap, crinkle)
        a = rng.uniform(0, math.tau)
        off = rng.uniform(0.0, 0.035)
        strap.location = (off * math.cos(a), off * math.sin(a), centre.z + radii.z - 0.002)
        strap.rotation_euler = (rng.uniform(-0.15, 0.15), rng.uniform(-0.12, 0.12), rng.uniform(0, math.tau))
        parts.append(strap)
    return parts


def burning_layout(specs, rng):
    """The tepee fallen in: three remnants still leaning, three fallen into a star with their inner ends on
    the coals, and a broken length lying across the middle."""
    pieces = []
    for i, s in enumerate(specs):
        a = s["angle"]
        if i in (0, 3, 6):
            base = s["start"] + Vector((math.cos(a), math.sin(a), 0)) * 0.01
            ap = a + math.pi + rng.uniform(-0.5, 0.5)
            top = Vector((0.04 * math.cos(ap), 0.04 * math.sin(ap), rng.uniform(0.17, 0.24)))
            pieces.append(dict(start=base, end=top, radius=s["radius"], seed=s["seed"], burn=0.32, burn_at="end"))
        elif i in (1, 2, 4):
            r_out = s["rb"] + rng.uniform(0.06, 0.09)
            base = Vector((r_out * math.cos(a), r_out * math.sin(a), 0))
            base.z = ash_height(base.x, base.y) + s["radius"] * 0.6
            ia = a + rng.uniform(-0.3, 0.3)
            r_in = rng.uniform(0.02, 0.05)
            inner = Vector((r_in * math.cos(ia), r_in * math.sin(ia), rng.uniform(0.055, 0.07)))
            pieces.append(dict(start=base, end=inner, radius=s["radius"], seed=s["seed"], burn=0.4, burn_at="end"))
        elif i == 5:
            pieces.append(dict(start=Vector((-0.075, 0.035, 0.05)), end=Vector((0.065, -0.03, 0.058)),
                               radius=s["radius"] * 1.05, seed=s["seed"], burn=0.3, burn_at="both"))
            pieces.append(dict(start=Vector((0.02, 0.1, 0.035)), end=Vector((0.11, 0.02, 0.04)),
                               radius=0.016, seed=s["seed"] + 50, burn=0.35, burn_at="both"))
    return pieces


def make_embers(kit, rng, mat, tips):
    # A low glowing bed under the coals, so the fire glows through the gaps between them.
    bed = _blob(kit, "EmberBed", (0.0, 0.0, ash_height(0.0, 0.0) - 0.004), (0.09, 0.085, 0.02), (0, 0, 1), 7, mat,
                subdiv=3, jitter=0.18)
    parts = [_decimate(kit, bed, 260)]
    for k in range(26):
        r = 0.13 * math.sqrt(rng.random())
        a = rng.uniform(0, math.tau)
        x, y = r * math.cos(a), r * math.sin(a)
        size = rng.choice([0.01, 0.013, 0.016, 0.019, 0.023, 0.027])
        radii = (size, size * rng.uniform(0.65, 0.95), size * rng.uniform(0.45, 0.7))
        z = ash_height(x, y) + 0.028 * math.exp(-(r / 0.09) ** 2) + radii[2] * 0.3
        axis = (rng.uniform(-0.4, 0.4), rng.uniform(-0.4, 0.4), 1.0)
        parts.append(_decimate(kit, _blob(kit, f"Ember{k}", (x, y, z), radii, axis, k + 11, mat, subdiv=3, jitter=0.3), 64))
    for k, tip in enumerate(tips):
        tr = max(tip["radius"], 0.008)
        centre = tip["point"] - tip["dir"] * tr * 0.5
        parts.append(_decimate(kit, _blob(kit, f"EmberTip{k}", centre, (tr * 1.25, tr * 1.25, tr * 1.6), tip["dir"],
                                          k + 61, mat, subdiv=3, jitter=0.2), 64))
    return parts


# ------------------------------------------------------------------ flames

FLAMES = [  # base (m), height, half-width, lean at the tip (dx, dy), twist (rad)
    ((0.0, 0.0, 0.045), 0.68, 0.08, (0.04, -0.03), 1.1),
    ((0.06, 0.035, 0.045), 0.52, 0.062, (0.11, 0.06), -0.9),
    ((-0.06, 0.03, 0.045), 0.47, 0.062, (-0.1, 0.07), 0.8),
    ((0.02, -0.07, 0.04), 0.40, 0.052, (0.04, -0.11), -0.7),
    ((-0.05, -0.05, 0.04), 0.36, 0.048, (-0.08, -0.07), 0.9),
]


def make_tongue(kit, k, spec, mat):
    base, height, width, lean, twist = spec
    phase = k * 1.7 + 0.4
    rows = []
    for i in range(FLAME_RINGS + 1):
        t = i / FLAME_RINGS
        sway = 0.035 * t * t
        cx = base[0] + lean[0] * t ** 1.4 + sway * math.sin(t * math.pi * 2.2 + phase)
        cy = base[1] + lean[1] * t ** 1.4 + sway * math.cos(t * math.pi * 1.9 + phase)
        cz = base[2] + height * t
        # Full and rounded low down, licking to a soft point; a slight waist where the tongue tears.
        w = width * (0.45 + 0.55 * smoothstep(0, 0.25, t)) * (1 - t) ** 0.8
        w *= 1 - 0.12 * math.exp(-((t - 0.58) / 0.1) ** 2)
        w = max(w, 0.002)
        ang = twist * t + phase
        ax, ay = Vector((math.cos(ang), math.sin(ang), 0)), Vector((-math.sin(ang), math.cos(ang), 0))
        ring = []
        for j in range(FLAME_SIDES):
            a = math.tau * j / FLAME_SIDES
            rr = w * (1 + 0.13 * math.sin(3 * a + phase + t * 5) + 0.07 * math.sin(5 * a - t * 9 + phase)
                      + 0.06 * mnoise.noise(Vector((math.cos(a) * 1.5, math.sin(a) * 1.5, t * 4 + phase))))
            ring.append(Vector((cx, cy, cz)) + ax * math.cos(a) * rr + ay * math.sin(a) * rr * 0.7)
        rows.append(ring)
    obj = kit.loft(f"Tongue{k}", rows, material=mat, cap_start=True, cap_end=True)
    # UVs: V = 0 at the base ring .. 1 at the tip; U around the tongue inside its own strip.
    count, sides = FLAME_RINGS + 1, FLAME_SIDES
    u0, u1 = (k + 0.02) / len(FLAMES), (k + 0.98) / len(FLAMES)
    layer = obj.data.uv_layers["UVMap"].data
    body = (count - 1) * sides
    for index, poly in enumerate(obj.data.polygons):
        seam = index < body and index % sides == sides - 1
        for loop_index, vertex_index in zip(poly.loop_indices, poly.vertices):
            if vertex_index >= count * sides:
                v = 0.0 if vertex_index == count * sides else 1.0
                layer[loop_index].uv = ((u0 + u1) / 2, v)
                continue
            ring, side = divmod(vertex_index, sides)
            u = 1.0 if seam and side == 0 else side / sides
            layer[loop_index].uv = (u0 + (u1 - u0) * u, ring / (count - 1))
    return obj


# ------------------------------------------------------------------ build

def build(kit):
    rng = random.Random(SEED)
    stone_mat = stone_material(kit)
    pit_parts = [make_stone(kit, i, spec, rng, stone_mat) for i, spec in enumerate(stone_layout(rng))]
    pit_parts.append(make_ash_bed(kit, ash_material()))
    pit_parts += make_coals(kit, rng, driftwood_material("M_FirePitCharcoal", char="all", seed=3.0))
    pit_parts += make_stubs(kit, driftwood_material("M_FirePitStub", char=(0.24, 0.36), seed=5.0))
    pit = kit.join(pit_parts, "SM_BeachFirePit", pivot=None, unwrap=False, reshade=True, smooth_angle=50)
    rocks.unwrap(pit, angle=62.0, margin=0.003, buried=(-0.012, 0.3))
    _shrink_uvs(pit, "M_FirePitAsh", 0.6)

    specs = stack_layout(rng)
    wood = driftwood_material("M_CampfireDriftwood", seed=1.0)
    parts = []
    for i, s in enumerate(specs):
        parts += driftwood(kit, f"Stick{i}", s["start"], s["end"], s["radius"], s["seed"], wood,
                           branch=s["branch"])[0]
    parts += make_tinder(kit, rng, grass_material(), wrack_material(), wad_material())
    stack = kit.join(parts, "SM_CampfireStack", pivot=None, unwrap=False, reshade=True, smooth_angle=60)

    charred = driftwood_material("M_CampfireCharred", char=(0.18, 0.27), seed=1.0)
    parts, tips = [], []
    for i, s in enumerate(burning_layout(specs, rng)):
        pieces, burnt = driftwood(kit, f"Burnt{i}", s["start"], s["end"], s["radius"], s["seed"], charred,
                                  burn=s["burn"], burn_at=s["burn_at"])
        parts += pieces
        tips += burnt
    burning = kit.join(parts, "SM_CampfireBurning", pivot=None, unwrap=False, reshade=True, smooth_angle=60)

    embers = kit.join(make_embers(kit, rng, ember_material(), tips), "SM_CampfireEmbers", pivot=None,
                      unwrap=False, reshade=True, smooth_angle=40)
    kit.pack_uvs(embers, margin=0.01)

    flame_mat = flame_material()
    flames = kit.join([make_tongue(kit, k, spec, flame_mat) for k, spec in enumerate(FLAMES)],
                      "SM_CampfireFlames", pivot=None, unwrap=False, reshade=True, smooth_angle=180)

    REPORT["fire_cm"] = {
        "flame_base_centre": [0.0, 0.0, 7.0],
        "flame_top": [round(FLAMES[0][0][0] * 100 + FLAMES[0][3][0] * 100, 1),
                      round(-(FLAMES[0][0][1] + FLAMES[0][3][1]) * 100, 1),
                      round((FLAMES[0][0][2] + FLAMES[0][1]) * 100, 1)],
        "ember_bed_radius": 16.0,
        "light_suggested": [0.0, 0.0, 45.0],
    }
    meshes = [pit, stack, burning, embers, flames]
    print("HOMESTEAD_CAMPFIRE tris", {o.name: triangles(o) for o in meshes})
    return meshes
