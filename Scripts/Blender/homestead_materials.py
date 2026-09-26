"""Procedural PBR materials for Homestead assets (Blender shader node networks).

Every material reads the ``pcoord`` point attribute written by the kit's
primitives: part-local coordinates at rest, with Z along the part's length for
cylinders and tubes. Shading therefore follows each part (grain runs along a
haft, bands wind around a cord) even after parts are joined and displaced.
The networks are designed to be baked (``kit.bake``) into game textures.
"""
import bpy


class Graph:
    """Tiny helper for building shader node trees in code."""

    def __init__(self, name):
        mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
        if mat.node_tree is None:
            mat.use_nodes = True
        self.mat = mat
        self.tree = mat.node_tree
        self.tree.nodes.clear()
        self.out = self.node("ShaderNodeOutputMaterial")
        self.bsdf = self.node("ShaderNodeBsdfPrincipled")
        self.link(self.bsdf.outputs["BSDF"], self.out.inputs["Surface"])
        mat["homestead_procedural"] = True

    def node(self, kind, **settings):
        node = self.tree.nodes.new(kind)
        for key, value in settings.items():
            if key in node.inputs:
                socket = node.inputs[key]
                if isinstance(value, bpy.types.NodeSocket):
                    self.link(value, socket)
                else:
                    socket.default_value = value
            else:
                setattr(node, key, value)
        return node

    def link(self, source, target):
        self.tree.links.new(source, target)

    def set(self, socket_name, value):
        socket = self.bsdf.inputs[socket_name]
        if isinstance(value, bpy.types.NodeSocket):
            self.link(value, socket)
        else:
            socket.default_value = value

    # --- sources
    def coord(self, scale=(1, 1, 1)):
        attr = self.node("ShaderNodeAttribute", attribute_name="pcoord", attribute_type="GEOMETRY")
        if scale == (1, 1, 1):
            return attr.outputs["Vector"]
        mapping = self.node("ShaderNodeMapping", Vector=attr.outputs["Vector"])
        mapping.inputs["Scale"].default_value = scale
        return mapping.outputs["Vector"]

    def uv(self):
        return self.node("ShaderNodeUVMap", uv_map="UVMap").outputs["UV"]

    def separate(self, vector):
        node = self.node("ShaderNodeSeparateXYZ", Vector=vector)
        return node.outputs["X"], node.outputs["Y"], node.outputs["Z"]

    def combine(self, x=0.0, y=0.0, z=0.0):
        node = self.node("ShaderNodeCombineXYZ")
        for socket, value in zip(("X", "Y", "Z"), (x, y, z)):
            if isinstance(value, bpy.types.NodeSocket):
                self.link(value, node.inputs[socket])
            else:
                node.inputs[socket].default_value = value
        return node.outputs["Vector"]

    # --- textures
    def noise(self, vector, scale=5.0, detail=4.0, roughness=0.5, distortion=0.0, dims="3D"):
        return self.node("ShaderNodeTexNoise", Vector=vector, Scale=scale, Detail=detail,
                         Roughness=roughness, Distortion=distortion, noise_dimensions=dims)

    def voronoi(self, vector, scale=5.0, randomness=1.0, feature="F1", dims="3D"):
        return self.node("ShaderNodeTexVoronoi", Vector=vector, Scale=scale, Randomness=randomness,
                         feature=feature, voronoi_dimensions=dims)

    def wave(self, vector, scale=5.0, distortion=0.0, detail=2.0, kind="BANDS", direction="Z",
             profile="SIN"):
        node = self.node("ShaderNodeTexWave", Vector=vector, Scale=scale, Distortion=distortion,
                         Detail=detail, wave_type=kind, wave_profile=profile)
        if kind == "BANDS":
            node.bands_direction = direction
        else:
            node.rings_direction = direction
        return node

    # --- math
    def math(self, op, a, b=0.0, clamp=False):
        node = self.node("ShaderNodeMath", operation=op, use_clamp=clamp)
        for socket, value in zip(node.inputs, (a, b)):
            if isinstance(value, bpy.types.NodeSocket):
                self.link(value, socket)
            else:
                socket.default_value = value
        return node.outputs[0]

    def vmath(self, op, a, b=(0, 0, 0)):
        node = self.node("ShaderNodeVectorMath", operation=op)
        for socket, value in zip(node.inputs, (a, b)):
            if isinstance(value, bpy.types.NodeSocket):
                self.link(value, socket)
            else:
                socket.default_value = value
        return node.outputs["Value"] if op in ("LENGTH", "DOT_PRODUCT", "DISTANCE") else node.outputs["Vector"]

    def remap(self, value, from_min, from_max, to_min=0.0, to_max=1.0, clamp=True):
        return self.node("ShaderNodeMapRange", Value=value, **{
            "From Min": from_min, "From Max": from_max, "To Min": to_min, "To Max": to_max},
            clamp=clamp).outputs["Result"]

    def ramp(self, fac, stops):
        """stops: [(position, (r, g, b)), ...] in linear color."""
        node = self.node("ShaderNodeValToRGB", Fac=fac)
        elements = node.color_ramp.elements
        while len(elements) < len(stops):
            elements.new(0.5)
        for element, (position, color) in zip(elements, stops):
            element.position = position
            element.color = (*color[:3], 1.0)
        return node.outputs["Color"]

    def mix(self, a, b, fac, blend="MIX"):
        node = self.node("ShaderNodeMix", data_type="RGBA", blend_type=blend)
        sockets = [s for s in node.inputs if s.type == "RGBA"]
        factor = node.inputs["Factor"]
        for socket, value in ((factor, fac), (sockets[0], a), (sockets[1], b)):
            if isinstance(value, bpy.types.NodeSocket):
                self.link(value, socket)
            else:
                socket.default_value = value if socket is factor else (*value[:3], 1.0)
        return next(s for s in node.outputs if s.type == "RGBA")

    def bump(self, height, strength=0.5, distance=0.002, normal=None):
        node = self.node("ShaderNodeBump", Height=height, Strength=strength, Distance=distance)
        if normal is not None:
            self.link(normal, node.inputs["Normal"])
        return node.outputs["Normal"]


# ------------------------------------------------------------------ materials

def wood(name, light=(0.42, 0.29, 0.17), dark=(0.20, 0.12, 0.06), grain=1.0, roughness=0.62,
         weathering=0.0, grime=0.0, seed=0.0, polish=0.0, polish_center=0.0, polish_length=0.06,
         relief=1.0):
    """Stripped/seasoned wood: fine long grain along Z, growth-ring banding,
    long tonal streaks, pores, optional grey weathering and dark handling grime.
    ``polish`` burnishes a band of part-local Z (``polish_center`` +- ``polish_length``)
    darker and glossier, as where a hand has gripped a tool handle for years."""
    g = Graph(name)
    p = g.coord((1.0, 1.0, 1.0))
    x, y, z = g.separate(p)
    warp = g.noise(p, scale=3.0 / grain, detail=3.0).outputs["Fac"]
    # Stretch along Z so features read as long fibres.
    stretched = g.combine(g.math("ADD", g.math("MULTIPLY", x, 38.0 / grain), g.math("MULTIPLY", warp, 2.5)),
                          g.math("MULTIPLY", y, 38.0 / grain), g.math("ADD", g.math("MULTIPLY", z, 1.6 / grain), seed))
    fibres = g.noise(stretched, scale=6.0, detail=8.0, roughness=0.62).outputs["Fac"]
    rings = g.wave(g.combine(x, y, g.math("MULTIPLY", z, 0.08)), scale=22.0 / grain, distortion=6.0,
                   detail=3.0, kind="RINGS", direction="Z").outputs["Fac"]
    tone = g.math("ADD", g.math("MULTIPLY", fibres, 0.65), g.math("MULTIPLY", rings, 0.35))
    color = g.ramp(tone, [(0.25, dark), (0.62, light), (0.85, tuple(min(1, c * 1.18) for c in light))])
    streaks = g.noise(g.combine(g.math("MULTIPLY", x, 12.0), g.math("MULTIPLY", y, 12.0),
                                g.math("MULTIPLY", z, 0.7)), scale=3.0, detail=3.0).outputs["Fac"]
    color = g.mix(color, (0.72, 0.66, 0.60), g.remap(streaks, 0.45, 0.75), blend="MULTIPLY")
    if weathering:
        grey = g.noise(p, scale=9.0 / grain, detail=4.0).outputs["Fac"]
        mask = g.remap(grey, 0.42, 0.62, 0.0, weathering)
        color = g.mix(color, (0.30, 0.27, 0.23), mask)
    if grime:
        dirt = g.noise(p, scale=30.0, detail=6.0, roughness=0.7).outputs["Fac"]
        color = g.mix(color, (0.07, 0.05, 0.035), g.remap(dirt, 0.4, 0.75, 0.0, grime))
    pores = g.noise(stretched, scale=38.0, detail=2.0).outputs["Fac"]
    rough = g.remap(fibres, 0.3, 0.7, roughness - 0.08, roughness + 0.1)
    if polish:
        worn = g.math("ABSOLUTE", g.math("SUBTRACT", z, polish_center))
        wobble = g.noise(p, scale=14.0, detail=3.0).outputs["Fac"]
        worn = g.math("ADD", worn, g.math("MULTIPLY", g.math("SUBTRACT", wobble, 0.5), polish_length * 0.6))
        mask = g.remap(worn, polish_length, polish_length * 0.25, 0.0, polish)
        # Hand oil darkens and closes the grain; the pores stay dark.
        color = g.mix(color, (0.62, 0.52, 0.44), mask, blend="MULTIPLY")
        rough = g.math("MULTIPLY", rough, g.math("SUBTRACT", 1.0, g.math("MULTIPLY", mask, 0.5)))
        pores = g.math("MULTIPLY", pores, g.math("SUBTRACT", 1.0, g.math("MULTIPLY", mask, 0.6)))
    g.set("Base Color", color)
    g.set("Roughness", rough)
    height = g.math("ADD", g.math("MULTIPLY", fibres, 1.0), g.math("MULTIPLY", pores, 0.4))
    g.set("Normal", g.bump(height, strength=0.35 * relief, distance=0.0015))
    return g.mat


def bark(name, light=(0.23, 0.19, 0.15), dark=(0.055, 0.045, 0.035), scale=1.0, roughness=0.9,
         lichen=0.0):
    """Furrowed bark: vertical plates split by dark fissures, optional lichen."""
    g = Graph(name)
    p = g.coord()
    x, y, z = g.separate(p)
    warp = g.noise(p, scale=4.0 / scale, detail=3.0).outputs["Fac"]
    plates_vec = g.combine(g.math("MULTIPLY", x, 1.0), g.math("MULTIPLY", y, 1.0),
                           g.math("ADD", g.math("MULTIPLY", z, 0.32), g.math("MULTIPLY", warp, 0.03)))
    cells = g.voronoi(plates_vec, scale=26.0 / scale, feature="DISTANCE_TO_EDGE").outputs["Distance"]
    fissure = g.remap(cells, 0.0, 0.12)
    grit = g.noise(p, scale=90.0 / scale, detail=6.0, roughness=0.7).outputs["Fac"]
    tone = g.math("MULTIPLY", fissure, g.remap(grit, 0.3, 0.7, 0.7, 1.1))
    color = g.ramp(tone, [(0.05, dark), (0.55, tuple(c * 0.75 for c in light)), (1.0, light)])
    if lichen:
        patches = g.noise(p, scale=7.0 / scale, detail=5.0).outputs["Fac"]
        mask = g.remap(patches, 0.62, 0.7, 0.0, lichen)
        color = g.mix(color, (0.38, 0.42, 0.30), mask)
    g.set("Base Color", color)
    g.set("Roughness", roughness)
    height = g.math("ADD", fissure, g.math("MULTIPLY", grit, 0.25))
    g.set("Normal", g.bump(height, strength=0.8, distance=0.004 * scale))
    return g.mat


def flint(name, body=(0.025, 0.022, 0.02), light=(0.075, 0.065, 0.055), cortex=(0.34, 0.29, 0.22),
          cortex_amount=0.35, cortex_below_x=None):
    """Knapped flint: near-black glassy body with smoky mottling and banding, fine
    ripple marks around flake scars, and a chalky cortex rind. ``cortex_below_x``
    confines the rind to part-local x below that value (e.g. an axe's butt)."""
    g = Graph(name)
    p = g.coord()
    cloud = g.noise(p, scale=45.0, detail=6.0, roughness=0.6).outputs["Fac"]
    band = g.wave(p, scale=24.0, distortion=5.0, detail=2.0, kind="BANDS", direction="DIAGONAL").outputs["Fac"]
    tone = g.math("ADD", g.math("MULTIPLY", cloud, 0.7), g.math("MULTIPLY", band, 0.3))
    stone = g.ramp(tone, [(0.3, body), (0.75, light)])
    rind = g.noise(p, scale=28.0, detail=4.0).outputs["Fac"]
    rind_mask = g.remap(rind, 0.66 - 0.2 * cortex_amount, 0.69 - 0.2 * cortex_amount)
    if cortex_below_x is not None:
        px, _, _ = g.separate(p)
        region = g.remap(px, cortex_below_x + 0.006, cortex_below_x - 0.006)
        rind_mask = g.math("MULTIPLY", g.math("MAXIMUM", rind_mask, g.math("MULTIPLY", region, 0.85)), region)
    speck = g.noise(p, scale=400.0, detail=2.0).outputs["Fac"]
    # Cortex is buff chalk stained by soil: iron-brown blotches, darker in pits.
    stain = g.noise(p, scale=90.0, detail=5.0).outputs["Fac"]
    rind_color = g.mix(cortex, tuple(c * 0.55 for c in (cortex[0], cortex[1] * 0.9, cortex[2] * 0.75)), stain)
    rind_color = g.mix(rind_color, tuple(c * 0.7 for c in cortex), speck)
    g.set("Base Color", g.mix(stone, rind_color, rind_mask))
    g.set("Roughness", g.remap(rind_mask, 0.0, 1.0, 0.22, 0.9))
    g.set("Subsurface Weight", 0.08)
    g.set("Subsurface Radius", (0.02, 0.015, 0.01))
    # Faint concentric ripple marks around each flake scar (Voronoi cell centers);
    # the cortex gets a pitted, granular relief instead.
    cells = g.voronoi(p, scale=55.0, feature="F1").outputs["Distance"]
    ripples = g.math("SINE", g.math("MULTIPLY", cells, 90.0))
    pits = g.noise(p, scale=700.0, detail=3.0).outputs["Fac"]
    glass = g.math("MULTIPLY", ripples, g.math("MULTIPLY", g.math("SUBTRACT", 1.0, rind_mask), 0.035))
    chalk = g.math("MULTIPLY", rind_mask, g.math("ADD", speck, g.math("MULTIPLY", pits, 0.8)))
    g.set("Normal", g.bump(g.math("ADD", glass, chalk), strength=0.3, distance=0.0006))
    return g.mat


def rawhide(name, color=(0.42, 0.27, 0.12), strands=3, twist=90.0):
    """Twisted rawhide/sinew cord for tube parts: pcoord (x, y, length) gives
    angle around the cord and distance along it; helical strand grooves wind
    ``twist`` times per meter."""
    g = Graph(name)
    p = g.coord()
    x, y, z = g.separate(p)
    angle = g.math("ARCTAN2", y, x)
    phase = g.math("ADD", g.math("MULTIPLY", z, twist * 6.2832), g.math("MULTIPLY", angle, strands))
    groove = g.math("ABSOLUTE", g.math("SINE", phase))
    fibre = g.noise(g.combine(g.math("MULTIPLY", angle, 3.0), g.math("MULTIPLY", z, 2.0), 0.0),
                    scale=60.0, detail=6.0).outputs["Fac"]
    tone = g.math("MULTIPLY", g.remap(groove, 0.0, 0.5, 0.55, 1.0), g.remap(fibre, 0.3, 0.7, 0.8, 1.12))
    base = g.ramp(tone, [(0.4, tuple(c * 0.45 for c in color)), (0.9, color), (1.0, tuple(min(1, c * 1.25) for c in color))])
    g.set("Base Color", base)
    g.set("Roughness", g.remap(groove, 0.0, 1.0, 0.75, 0.5))
    g.set("Subsurface Weight", 0.15)
    g.set("Subsurface Radius", (0.006, 0.003, 0.0015))
    g.set("Normal", g.bump(g.math("ADD", groove, g.math("MULTIPLY", fibre, 0.3)), strength=0.6, distance=0.0012))
    return g.mat


def leaf(name, color=(0.07, 0.16, 0.03), vein=(0.18, 0.26, 0.07), tip=(0.11, 0.17, 0.04),
         roughness=0.45, translucency=0.3):
    """Leaf tissue for UV-mapped blades (U across the blade 0..1 with the midrib at
    0.5, V from base 0 to tip 1): midrib and paired lateral veins, base-to-tip
    tint, mottling and a waxy sheen. Designed for shared, overlapping leaf UVs."""
    g = Graph(name)
    uv = g.uv()
    u, v, _ = g.separate(uv)
    across = g.math("ABSOLUTE", g.math("SUBTRACT", u, 0.5))
    midrib = g.remap(across, 0.0, 0.03, 1.0, 0.0)
    lateral_phase = g.math("SUBTRACT", g.math("MULTIPLY", v, 18.0), g.math("MULTIPLY", across, 22.0))
    lateral = g.remap(g.math("ABSOLUTE", g.math("SINE", g.math("MULTIPLY", lateral_phase, 3.1416))),
                      0.93, 1.0, 0.0, 0.6)
    veins = g.math("MAXIMUM", midrib, lateral)
    mottle = g.noise(uv, scale=14.0, detail=5.0).outputs["Fac"]
    tissue = g.mix(color, tip, g.remap(v, 0.4, 1.0))
    tissue = g.mix(tissue, tuple(c * 0.78 for c in color), g.remap(mottle, 0.4, 0.7, 0.0, 0.6))
    g.set("Base Color", g.mix(tissue, vein, veins))
    g.set("Roughness", roughness)
    g.set("Subsurface Weight", translucency)
    g.set("Subsurface Radius", (0.01, 0.02, 0.005))
    height = g.math("SUBTRACT", g.math("MULTIPLY", mottle, 0.15), g.math("MULTIPLY", veins, 0.6))
    g.set("Normal", g.bump(height, strength=0.4, distance=0.0006))
    return g.mat


def stem(name, color=(0.12, 0.16, 0.05), dark=(0.05, 0.06, 0.02), roughness=0.5):
    """Green/brown plant stem with fine longitudinal striations (tube parts)."""
    g = Graph(name)
    p = g.coord()
    x, y, z = g.separate(p)
    angle = g.math("ARCTAN2", y, x)
    stripes = g.noise(g.combine(g.math("MULTIPLY", angle, 6.0), g.math("MULTIPLY", z, 0.6), 0.0),
                      scale=12.0, detail=4.0).outputs["Fac"]
    g.set("Base Color", g.ramp(stripes, [(0.3, dark), (0.7, color)]))
    g.set("Roughness", roughness)
    g.set("Subsurface Weight", 0.1)
    g.set("Normal", g.bump(stripes, strength=0.3, distance=0.0008))
    return g.mat


def daub(name, clay=(0.30, 0.22, 0.14), dry=(0.46, 0.38, 0.27), straw=(0.52, 0.42, 0.22)):
    """Clay-and-straw daub: lumpy trowelled surface, drying cracks, chopped straw."""
    g = Graph(name)
    p = g.coord()
    lumps = g.noise(p, scale=9.0, detail=6.0, roughness=0.6).outputs["Fac"]
    cracks = g.voronoi(p, scale=14.0, feature="DISTANCE_TO_EDGE").outputs["Distance"]
    crack_mask = g.remap(cracks, 0.0, 0.015, 1.0, 0.0)
    fibres = g.noise(g.vmath("MULTIPLY", p, (60.0, 6.0, 60.0)), scale=4.0, detail=2.0).outputs["Fac"]
    straw_mask = g.remap(fibres, 0.72, 0.76)
    color = g.mix(clay, dry, g.remap(lumps, 0.35, 0.7))
    color = g.mix(color, straw, straw_mask)
    color = g.mix(color, tuple(c * 0.4 for c in clay), crack_mask)
    g.set("Base Color", color)
    g.set("Roughness", 0.92)
    height = g.math("SUBTRACT", lumps, g.math("MULTIPLY", crack_mask, 0.5))
    g.set("Normal", g.bump(height, strength=0.7, distance=0.006))
    return g.mat


def steel(name, bevel=0.0035, scale_from=0.022, patina=0.6, rust=0.35, seed=0.0):
    """Hand-forged carbon-steel blade. pcoord is (thickness, distance from the cutting
    edge, length) in meters (see the machete recipe). Three zones: a bright honed
    bevel within ``bevel`` of the edge, a ground flat carrying a mottled grey-brown
    plant-sap patina, and black forge scale with hammer marks beyond ``scale_from``
    (towards the spine). Rust sits in pits and scale flakes; the honed bevel is clean.
    Albedo stays in measured iron ranges (bare steel ~0.56 linear), metallic 1 on
    bare steel, lower on scale and 0 on rust."""
    g = Graph(name)
    p = g.coord()
    x, d, z = g.separate(p)
    seeded = g.vmath("ADD", p, (seed * 0.37, seed * 0.11, seed * 0.53))
    wob = g.noise(seeded, scale=45.0, detail=3.0).outputs["Fac"]
    shoulder = g.math("ADD", d, g.math("MULTIPLY", g.math("SUBTRACT", wob, 0.5), 0.0014))
    bevel_mask = g.remap(shoulder, bevel, bevel - 0.0005)
    big = g.noise(seeded, scale=9.0, detail=4.0).outputs["Fac"]
    ragged = g.noise(seeded, scale=48.0, detail=6.0, roughness=0.7).outputs["Fac"]
    reach = g.math("ADD", d, g.math("MULTIPLY", g.math("SUBTRACT", big, 0.5), 0.016))
    reach = g.math("ADD", reach, g.math("MULTIPLY", g.math("SUBTRACT", ragged, 0.5), 0.009))
    zone = g.remap(reach, scale_from - 0.0015, scale_from + 0.003)
    flakes = g.noise(seeded, scale=55.0, detail=5.0, roughness=0.65).outputs["Fac"]
    cover = g.remap(flakes, 0.31, 0.36)
    scale_mask = g.math("MULTIPLY", zone, cover)
    # Where scale has flaked off, the steel underneath is dull grey and pitted, not bright.
    hole = g.math("MULTIPLY", zone, g.math("SUBTRACT", 1.0, cover))
    # Scratches: honing strokes run diagonally across the bevel, grinding marks across the flats.
    diag = g.combine(g.math("ADD", g.math("MULTIPLY", d, 0.8), g.math("MULTIPLY", z, 0.6)),
                     g.math("SUBTRACT", g.math("MULTIPLY", z, 0.8), g.math("MULTIPLY", d, 0.6)), x)
    hone = g.noise(g.vmath("MULTIPLY", diag, (40.0, 2600.0, 40.0)), scale=1.0, detail=2.0).outputs["Fac"]
    grind = g.noise(g.vmath("MULTIPLY", p, (60.0, 25.0, 1800.0)), scale=1.0, detail=3.0,
                    distortion=0.8).outputs["Fac"]
    # Pits: fine pitting everywhere off the bevel, a few larger rust blooms.
    pit_noise = g.noise(seeded, scale=520.0, detail=2.0).outputs["Fac"]
    # Pitting clusters where moisture sat, and under flaked scale.
    cluster = g.noise(g.vmath("ADD", seeded, (1.3, 4.1, 2.2)), scale=14.0, detail=3.0).outputs["Fac"]
    cluster = g.math("MAXIMUM", g.remap(cluster, 0.48, 0.62), g.math("MULTIPLY", hole, 0.8))
    pits = g.math("MULTIPLY", g.remap(pit_noise, 0.66, 0.75), g.math("SUBTRACT", 1.0, bevel_mask))
    pits = g.math("MULTIPLY", pits, cluster)
    bloom = g.noise(g.vmath("ADD", seeded, (3.1, 1.7, 0.4)), scale=16.0, detail=5.0, roughness=0.7).outputs["Fac"]
    bloom = g.remap(bloom, 0.68 - 0.1 * rust, 0.76 - 0.1 * rust, 0.0, rust)
    rust_mask = g.math("MINIMUM", g.math("ADD", g.math("MULTIPLY", pits, 0.9),
                                        g.math("MULTIPLY", bloom, g.math("SUBTRACT", 1.0, bevel_mask))), 1.0)
    rust_mask = g.math("MAXIMUM", rust_mask, g.math("MULTIPLY", hole, g.remap(pit_noise, 0.45, 0.6, 0.15, 0.7)))
    # Colors (linear).
    tint = g.noise(g.vmath("ADD", seeded, (7.0, 0.0, 0.0)), scale=22.0, detail=4.0).outputs["Fac"]
    # Noise Fac clusters tightly around 0.5, so masks remap a narrow band for real contrast.
    tan_patina = g.remap(tint, 0.44, 0.60, 0.0, patina)
    blue_patina = g.remap(big, 0.47, 0.60, 0.0, patina * 0.85)
    ground = g.mix((0.50, 0.49, 0.475), (0.21, 0.19, 0.16), tan_patina)
    ground = g.mix(ground, (0.12, 0.125, 0.14), blue_patina)
    # Sap patina wiped along the blade by cutting strokes, and fine oxide freckling.
    streak = g.noise(g.vmath("MULTIPLY", seeded, (40.0, 70.0, 5.0)), scale=1.0, detail=4.0).outputs["Fac"]
    streaks = g.remap(streak, 0.50, 0.60, 0.0, patina * 0.6)
    ground = g.mix(ground, (0.19, 0.16, 0.12), streaks)
    freckle = g.noise(g.vmath("ADD", seeded, (2.0, 5.0, 1.0)), scale=110.0, detail=3.0).outputs["Fac"]
    ground = g.mix(ground, (0.24, 0.225, 0.20), g.remap(freckle, 0.52, 0.64, 0.0, patina * 0.5))
    patina_mask = g.math("MINIMUM", g.math("ADD", g.math("ADD", tan_patina, blue_patina), streaks), 1.0)
    ground = g.mix(ground, (0.86, 0.86, 0.86), g.remap(grind, 0.3, 0.75, 0.0, 0.15), blend="MULTIPLY")
    honed = g.mix((0.60, 0.595, 0.585), (0.50, 0.495, 0.49), g.remap(hone, 0.35, 0.7))
    forge = g.mix((0.035, 0.036, 0.04), (0.075, 0.072, 0.07), g.remap(flakes, 0.4, 0.8))
    rust_color = g.mix((0.075, 0.034, 0.017), (0.17, 0.068, 0.026), g.remap(pit_noise, 0.68, 0.82))
    color = g.mix(ground, forge, scale_mask)
    color = g.mix(color, (0.085, 0.08, 0.075), hole)
    color = g.mix(color, honed, bevel_mask)
    color = g.mix(color, rust_color, rust_mask)
    g.set("Base Color", color)
    # Thin oxide films read slightly less metallic; scale much less; rust not at all.
    metal = g.math("SUBTRACT", 1.0, g.math("MULTIPLY", patina_mask, 0.15))
    metal = g.math("SUBTRACT", metal, g.math("MULTIPLY", scale_mask, 0.55))
    metal = g.math("SUBTRACT", metal, g.math("MULTIPLY", hole, 0.4))
    metal = g.math("MAXIMUM", metal, bevel_mask)
    g.set("Metallic", g.math("MULTIPLY", metal, g.math("SUBTRACT", 1.0, rust_mask)))
    rough = g.remap(grind, 0.3, 0.7, 0.48, 0.58)
    rough = g.math("ADD", rough, g.math("MULTIPLY", patina_mask, 0.12))
    rough = g.math("ADD", g.math("MULTIPLY", rough, g.math("SUBTRACT", 1.0, zone)),
                   g.math("MULTIPLY", zone, 0.64))
    honed_rough = g.remap(hone, 0.3, 0.7, 0.16, 0.28)
    rough = g.math("ADD", g.math("MULTIPLY", rough, g.math("SUBTRACT", 1.0, bevel_mask)),
                   g.math("MULTIPLY", honed_rough, bevel_mask))
    rough = g.math("ADD", g.math("MULTIPLY", rough, g.math("SUBTRACT", 1.0, rust_mask)),
                   g.math("MULTIPLY", rust_mask, 0.9))
    g.set("Roughness", rough)
    # Relief: shallow hammer dishes under the scale, grinding lines, pits and rust crust.
    cells = g.voronoi(seeded, scale=115.0, feature="SMOOTH_F1").outputs["Distance"]
    hammer = g.math("MULTIPLY", g.math("MULTIPLY", cells, cells),
                    g.math("ADD", 0.25, g.math("MULTIPLY", scale_mask, 0.75)))
    hammer = g.math("MULTIPLY", hammer, g.math("SUBTRACT", 1.0, bevel_mask))
    height = g.math("ADD", g.math("MULTIPLY", hammer, 0.9), g.math("MULTIPLY", grind, 0.02))
    height = g.math("ADD", height, g.math("MULTIPLY", hone, g.math("MULTIPLY", bevel_mask, 0.006)))
    height = g.math("SUBTRACT", height, g.math("MULTIPLY", pits, 0.35))
    height = g.math("ADD", height, g.math("MULTIPLY", bloom, g.math("MULTIPLY", flakes, 0.3)))
    g.set("Normal", g.bump(height, strength=0.7, distance=0.0025))
    return g.mat


def brass(name, polished=(0.78, 0.58, 0.30), tarnish=(0.20, 0.15, 0.075), wear=0.5):
    """Old brass hardware (rivets, liners): peened faces rubbed bright, dark
    brown-green tarnish in the texture and around the rims."""
    g = Graph(name)
    p = g.coord()
    blot = g.noise(p, scale=900.0, detail=4.0, roughness=0.6).outputs["Fac"]
    rub = g.remap(blot, 0.55 - 0.2 * wear, 0.62 - 0.2 * wear)
    g.set("Base Color", g.mix(tarnish, polished, rub))
    g.set("Metallic", g.remap(rub, 0.0, 1.0, 0.55, 1.0))
    g.set("Roughness", g.remap(rub, 0.0, 1.0, 0.62, 0.3))
    dents = g.noise(p, scale=2400.0, detail=2.0).outputs["Fac"]
    g.set("Normal", g.bump(g.math("ADD", dents, g.math("MULTIPLY", blot, 0.4)), strength=0.25,
                           distance=0.0002))
    return g.mat


def leather(name, color=(0.30, 0.19, 0.10), dark=(0.12, 0.07, 0.035), roughness=0.74,
            creases=1.0, soil_below=None, soil=0.5, handled=None, stains=0.0, seed=0.0):
    """Soft smoke-tanned hide (buckskin): mottled smoke colour, a fine suede nap,
    crease lines that collect dirt, optional soil grime below part-local Z
    ``soil_below`` and a burnished, grubbier band ``handled=(z_center, span)``
    where fingers work the drawstring. ``stains`` adds faint berry-juice spots."""
    g = Graph(name)
    p = g.coord()
    x, y, z = g.separate(p)
    seeded = g.vmath("ADD", p, (seed * 0.41, seed * 0.23, seed * 0.19))
    smoke = g.noise(seeded, scale=7.0, detail=5.0, roughness=0.6).outputs["Fac"]
    mottle = g.noise(seeded, scale=38.0, detail=4.0).outputs["Fac"]
    nap = g.noise(seeded, scale=1400.0, detail=3.0, roughness=0.7).outputs["Fac"]
    # Noise Fac clusters around 0.5: narrow remaps give the smoke mottling real contrast.
    tone = g.math("ADD", g.math("MULTIPLY", smoke, 0.65), g.math("MULTIPLY", mottle, 0.35))
    base = g.ramp(tone, [(0.40, dark), (0.52, color), (0.62, tuple(min(1.0, c * 1.15) for c in color))])
    base = g.mix(base, (0.88, 0.87, 0.85), g.remap(nap, 0.4, 0.62), blend="MULTIPLY")
    # Soft suede has no grain pattern: broad soft wrinkles running with the gathers, plus
    # fine horizontal compression crinkles. Smooth fields, never ridged (ridges read as marble).
    wrinkle = g.noise(g.vmath("MULTIPLY", seeded, (1.0, 1.0, 0.35)), scale=55.0, detail=3.0).outputs["Fac"]
    crinkle = g.noise(g.vmath("MULTIPLY", seeded, (1.0, 1.0, 3.5)), scale=140.0, detail=2.0,
                      distortion=0.4).outputs["Fac"]
    crease = g.math("ADD", g.math("MULTIPLY", g.remap(wrinkle, 0.42, 0.58, 1.0, 0.0), 0.7 * creases),
                    g.math("MULTIPLY", g.remap(crinkle, 0.44, 0.56, 1.0, 0.0), 0.3 * creases))
    base = g.mix(base, tuple(c * 0.6 for c in dark), g.math("MULTIPLY", crease, 0.22))
    grime = g.math("MULTIPLY", crease, 0.0)
    if soil_below is not None:
        dirt = g.noise(seeded, scale=24.0, detail=5.0).outputs["Fac"]
        low = g.math("ADD", z, g.math("MULTIPLY", g.math("SUBTRACT", dirt, 0.5), 0.05))
        grime = g.math("MAXIMUM", grime, g.remap(low, soil_below, soil_below - 0.05, 0.0, soil))
    if handled is not None:
        center, span = handled
        near = g.math("ABSOLUTE", g.math("SUBTRACT", z, center))
        grime = g.math("MAXIMUM", grime, g.remap(near, span, span * 0.3, 0.0, 0.45))
    base = g.mix(base, (0.07, 0.05, 0.035), grime)
    if stains:
        spot = g.noise(g.vmath("ADD", seeded, (5.0, 2.0, 9.0)), scale=30.0, detail=3.0).outputs["Fac"]
        base = g.mix(base, (0.075, 0.045, 0.045), g.remap(spot, 0.62, 0.67, 0.0, stains))
    g.set("Base Color", base)
    rough = g.remap(nap, 0.3, 0.7, roughness - 0.05, roughness + 0.06)
    if handled is not None:
        rough = g.math("SUBTRACT", rough, g.math("MULTIPLY", g.remap(near, span, span * 0.3), 0.18))
    g.set("Roughness", rough)
    g.set("Sheen Weight", 0.25)
    g.set("Sheen Roughness", 0.6)
    height = g.math("SUBTRACT", g.math("MULTIPLY", nap, 0.2), g.math("MULTIPLY", crease, 0.5))
    height = g.math("ADD", height, g.math("MULTIPLY", mottle, 0.15))
    g.set("Normal", g.bump(height, strength=0.4, distance=0.0012))
    return g.mat

def blackberry(name, color=(0.016, 0.007, 0.013), glint=(0.040, 0.011, 0.028), crevice=(0.012, 0.003, 0.007),
               red=0.0, drupelet=0.0030, seed=0.0):
    """Aggregate berry (blackberry/raspberry) skin. pcoord is berry-local rest position
    in meters (offset per berry for variety). Each Voronoi cell of ~``drupelet`` metres is
    one drupelet: a glossy rounded dome with a faint purple-red body glow and a tiny dry
    style scar, separated by dark, rough crevices. ``red`` (0..1) shifts some drupelets
    towards the dark red of a not-quite-ripe berry. Ripe blackberries reflect only a few
    percent, so albedo stays very low; the readable shape comes from the specular domes."""
    g = Graph(name)
    p = g.coord()
    seeded = g.vmath("ADD", p, (seed * 0.031, seed * 0.017, seed * 0.023))
    scale = 1.0 / drupelet
    cell = g.voronoi(seeded, scale=scale, randomness=0.35, feature="F1")
    centre = cell.outputs["Distance"]
    edge = g.voronoi(seeded, scale=scale, randomness=0.35, feature="DISTANCE_TO_EDGE").outputs["Distance"]
    # Packed spheres: a round dome sqrt(1 - (d / R)^2) about each seed, with R larger
    # than the cells so neighbouring domes meet in sharp creases instead of flat gaps.
    ratio = g.math("DIVIDE", centre, 0.68)
    dome = g.math("SQRT", g.math("SUBTRACT", 1.0, g.math("MULTIPLY", ratio, ratio), clamp=True))
    dome = g.math("MULTIPLY", dome, g.remap(edge, 0.0, 0.03, 0.7, 1.0))
    tint = g.separate(cell.outputs["Color"])[0]
    body = g.mix(color, glint, g.remap(tint, 0.2, 0.9))
    if red:
        body = g.mix(body, (0.16, 0.012, 0.022), g.remap(tint, 0.85 - 0.6 * red, 1.0, 0.0, red))
    base = g.mix(crevice, body, g.remap(dome, 0.0, 0.4))
    scar = g.remap(centre, 0.07, 0.02)
    base = g.mix(base, (0.05, 0.035, 0.028), g.math("MULTIPLY", scar, 0.7))
    g.set("Base Color", base)
    g.set("Roughness", g.remap(dome, 0.1, 0.8, 0.62, 0.2))
    g.set("Coat Weight", 0.25)
    g.set("Coat Roughness", 0.12)
    g.set("Subsurface Weight", 0.15)
    g.set("Subsurface Radius", (0.004, 0.0008, 0.002))
    height = g.math("SUBTRACT", dome, g.math("MULTIPLY", scar, 0.25))
    g.set("Normal", g.bump(height, strength=1.0, distance=0.0009))
    return g.mat


def leaf_pcoord(name, color=(0.045, 0.085, 0.022), vein=(0.10, 0.15, 0.05), tip=(0.06, 0.10, 0.03),
                roughness=0.5, translucency=0.25, rugose=1.0, serrate_dark=0.0):
    """``leaf`` driven by pcoord = (u across 0..1 with the midrib at 0.5, v base 0..tip 1, 0)
    instead of UVs, so it survives the bake repack. Adds the quilted, sunken-vein
    (rugose) relief of bramble leaves between the laterals."""
    g = Graph(name)
    u, v, _ = g.separate(g.coord())
    uv = g.combine(u, v, 0.0)
    across = g.math("ABSOLUTE", g.math("SUBTRACT", u, 0.5))
    midrib = g.remap(across, 0.0, 0.025, 1.0, 0.0)
    lateral_phase = g.math("SUBTRACT", g.math("MULTIPLY", v, 9.0), g.math("MULTIPLY", across, 7.0))
    wave = g.math("ABSOLUTE", g.math("SINE", g.math("MULTIPLY", lateral_phase, 3.1416)))
    lateral = g.remap(wave, 0.94, 1.0, 0.0, 0.75)
    veins = g.math("MAXIMUM", midrib, lateral)
    mottle = g.noise(uv, scale=18.0, detail=5.0).outputs["Fac"]
    fine = g.voronoi(uv, scale=60.0, feature="DISTANCE_TO_EDGE").outputs["Distance"]
    tissue = g.mix(color, tip, g.remap(v, 0.5, 1.0))
    tissue = g.mix(tissue, tuple(c * 0.75 for c in color), g.remap(mottle, 0.42, 0.62, 0.0, 0.7))
    tissue = g.mix(tissue, vein, veins)
    if serrate_dark:
        rim = g.remap(across, 0.40, 0.5, 0.0, serrate_dark)
        tissue = g.mix(tissue, (0.06, 0.05, 0.02), rim)
    g.set("Base Color", tissue)
    g.set("Roughness", g.remap(mottle, 0.4, 0.6, roughness - 0.06, roughness + 0.06))
    g.set("Subsurface Weight", translucency)
    g.set("Subsurface Radius", (0.01, 0.02, 0.005))
    quilt = g.math("MULTIPLY", g.remap(wave, 0.5, 1.0, 1.0, 0.0), rugose)
    height = g.math("ADD", g.math("MULTIPLY", quilt, 0.5), g.math("MULTIPLY", g.remap(fine, 0.0, 0.12), 0.15))
    height = g.math("SUBTRACT", height, g.math("MULTIPLY", veins, 0.6))
    g.set("Normal", g.bump(height, strength=0.45, distance=0.0005))
    return g.mat


def root(name, skin=(0.46, 0.33, 0.18), dark=(0.33, 0.22, 0.12), soil=(0.14, 0.10, 0.065),
         dry_soil=(0.27, 0.21, 0.145), dirt=0.55, ring_spacing=0.0045, seed=0.0):
    """Freshly pulled taproot skin (wild carrot / yampah): cream-tan with fine
    transverse growth rings and lenticel scars, faint longitudinal wrinkles, and
    damp soil smeared into the creases and in patches (``dirt`` 0..1), drying paler
    at its thin edges. pcoord is the tube frame (x, y, arclength from the crown)."""
    g = Graph(name)
    p = g.coord()
    x, y, z = g.separate(p)
    seeded = g.vmath("ADD", p, (seed * 0.21, seed * 0.13, seed * 0.07))
    angle = g.math("ARCTAN2", y, x)
    wob = g.noise(seeded, scale=90.0, detail=3.0).outputs["Fac"]
    phase = g.math("ADD", g.math("DIVIDE", z, ring_spacing), g.math("MULTIPLY", wob, 3.0))
    ring = g.math("ABSOLUTE", g.math("SINE", g.math("MULTIPLY", phase, 3.1416)))
    groove = g.remap(ring, 0.12, 0.0)
    broken = g.noise(g.combine(g.math("MULTIPLY", angle, 1.3), g.math("MULTIPLY", z, 60.0), 3.0),
                     scale=3.0, detail=2.0).outputs["Fac"]
    groove = g.math("MULTIPLY", groove, g.remap(broken, 0.5, 0.6))
    streak = g.noise(g.combine(g.math("MULTIPLY", angle, 5.0), g.math("MULTIPLY", z, 25.0), seed),
                     scale=4.0, detail=4.0).outputs["Fac"]
    lent = g.voronoi(g.vmath("MULTIPLY", seeded, (1.0, 1.0, 0.35)), scale=700.0, feature="F1").outputs["Distance"]
    lenticel = g.remap(lent, 0.12, 0.05)
    base = g.mix(dark, skin, g.remap(streak, 0.25, 0.68))
    base = g.mix(base, tuple(c * 0.6 for c in dark), g.math("MAXIMUM", g.math("MULTIPLY", groove, 0.5),
                                                               g.math("MULTIPLY", lenticel, 0.6)))
    patch = g.noise(seeded, scale=60.0, detail=6.0, roughness=0.65).outputs["Fac"]
    grain = g.noise(seeded, scale=2200.0, detail=2.0).outputs["Fac"]
    film = g.noise(seeded, scale=14.0, detail=3.0).outputs["Fac"]
    cover = g.math("ADD", g.remap(patch, 0.56 - 0.12 * dirt, 0.70 - 0.12 * dirt),
                   g.math("MULTIPLY", groove, 0.6 * dirt))
    cover = g.math("MAXIMUM", cover, g.remap(film, 0.35, 0.65, 0.05 * dirt, 0.4 * dirt))
    cover = g.math("MINIMUM", cover, 1.0)
    earth = g.mix(soil, dry_soil, g.remap(patch, 0.6 - 0.14 * dirt, 0.52 - 0.14 * dirt))
    earth = g.mix(earth, tuple(c * 0.6 for c in soil), g.remap(grain, 0.45, 0.6))
    base = g.mix(base, earth, cover)
    g.set("Base Color", base)
    g.set("Roughness", g.math("ADD", g.math("MULTIPLY", cover, 0.4), g.remap(streak, 0.3, 0.7, 0.46, 0.56)))
    g.set("Subsurface Weight", 0.08)
    height = g.math("SUBTRACT", g.math("MULTIPLY", cover, 0.5), g.math("MULTIPLY", groove, 0.6))
    height = g.math("ADD", height, g.math("MULTIPLY", g.math("MULTIPLY", grain, cover), 0.35))
    height = g.math("SUBTRACT", height, g.math("MULTIPLY", lenticel, 0.25))
    g.set("Normal", g.bump(height, strength=0.6, distance=0.0006))
    return g.mat


def soil(name, damp=(0.12, 0.085, 0.055), dry=(0.26, 0.20, 0.135), seed=0.0):
    """Clinging clods of forest loam: crumbly grit, tiny pale mineral grains and
    fibrous organic bits, damp and dark in the core, paler where it has dried."""
    g = Graph(name)
    p = g.coord()
    seeded = g.vmath("ADD", p, (seed * 0.3, seed * 0.2, seed * 0.1))
    crumb = g.voronoi(seeded, scale=900.0, feature="F1").outputs["Distance"]
    lumps = g.noise(seeded, scale=240.0, detail=5.0, roughness=0.7).outputs["Fac"]
    grit = g.noise(seeded, scale=3000.0, detail=2.0).outputs["Fac"]
    base = g.mix(damp, dry, g.remap(lumps, 0.42, 0.62))
    base = g.mix(base, tuple(c * 0.55 for c in damp), g.remap(crumb, 0.35, 0.6))
    base = g.mix(base, (0.36, 0.33, 0.28), g.remap(grit, 0.68, 0.72, 0.0, 0.6))
    g.set("Base Color", base)
    g.set("Roughness", 0.95)
    height = g.math("ADD", g.math("MULTIPLY", g.remap(crumb, 0.6, 0.0), 0.6), g.math("MULTIPLY", lumps, 0.4))
    g.set("Normal", g.bump(height, strength=0.8, distance=0.0008))
    return g.mat
