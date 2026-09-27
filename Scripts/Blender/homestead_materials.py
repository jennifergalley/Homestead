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

    def ramp(self, fac, stops, interpolation="LINEAR"):
        """stops: [(position, (r, g, b)), ...] in linear color."""
        node = self.node("ShaderNodeValToRGB", Fac=fac)
        node.color_ramp.interpolation = interpolation
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

    def ao(self, distance=0.2, samples=16, local=True):
        """Cycles ambient occlusion (1 = open, 0 = enclosed) for cavity masks."""
        node = self.node("ShaderNodeAmbientOcclusion", Distance=distance, samples=samples,
                         only_local=local)
        return node.outputs["AO"]

    def channel(self, color, index=0):
        node = self.node("ShaderNodeSeparateColor", Color=color)
        return node.outputs[index]

    def scale(self, vector, factor):
        return self.vmath("MULTIPLY", vector, factor if isinstance(factor, (tuple, list)) else (factor,) * 3)


# ------------------------------------------------------------------ materials

def wood(name, light=(0.42, 0.29, 0.17), dark=(0.20, 0.12, 0.06), grain=1.0, roughness=0.62,
         weathering=0.0, grime=0.0, seed=0.0, polish=0.0, polish_center=0.0, polish_length=0.06,
         relief=1.0, char_above=None, char_band=0.05, soot_band=0.12):
    """Stripped/seasoned wood: fine long grain along Z, growth-ring banding,
    long tonal streaks, pores, optional grey weathering and dark handling grime.
    ``polish`` burnishes a band of part-local Z (``polish_center`` +- ``polish_length``)
    darker and glossier, as where a hand has gripped a tool handle for years.
    ``polish_center`` may be a list for tools held with two hands (one band per grip).
    ``char_above`` chars the wood above that part-local Z (a torch stake under its head):
    black alligator-cracked charcoal over ``char_band`` m, with brown scorch and soot
    reaching ``soot_band`` m further down."""
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
        centres = polish_center if isinstance(polish_center, (list, tuple)) else (polish_center,)
        worn = None
        for centre in centres:
            distance = g.math("ABSOLUTE", g.math("SUBTRACT", z, centre))
            worn = distance if worn is None else g.math("MINIMUM", worn, distance)
        wobble = g.noise(p, scale=14.0, detail=3.0).outputs["Fac"]
        worn = g.math("ADD", worn, g.math("MULTIPLY", g.math("SUBTRACT", wobble, 0.5), polish_length * 0.6))
        mask = g.remap(worn, polish_length, polish_length * 0.25, 0.0, polish)
        # Hand oil darkens and closes the grain; the pores stay dark.
        color = g.mix(color, (0.62, 0.52, 0.44), mask, blend="MULTIPLY")
        rough = g.math("MULTIPLY", rough, g.math("SUBTRACT", 1.0, g.math("MULTIPLY", mask, 0.5)))
        pores = g.math("MULTIPLY", pores, g.math("SUBTRACT", 1.0, g.math("MULTIPLY", mask, 0.6)))
    height = g.math("ADD", g.math("MULTIPLY", fibres, 1.0), g.math("MULTIPLY", pores, 0.4))
    if char_above is not None:
        edge = g.noise(p, scale=40.0, detail=3.0).outputs["Fac"]
        zc = g.math("ADD", z, g.math("MULTIPLY", g.math("SUBTRACT", edge, 0.5), char_band * 0.8))
        soot = g.remap(zc, char_above - char_band - soot_band, char_above - char_band)
        char = g.remap(zc, char_above - char_band, char_above)
        color = g.mix(color, (0.34, 0.24, 0.16), soot, blend="MULTIPLY")
        cells = g.voronoi(g.vmath("MULTIPLY", p, (1.0, 1.0, 0.55)), scale=180.0,
                          feature="DISTANCE_TO_EDGE").outputs["Distance"]
        crack = g.remap(cells, 0.0, 0.07, 1.0, 0.0)
        sheen = g.noise(p, scale=90.0, detail=3.0).outputs["Fac"]
        coal = g.mix((0.018, 0.016, 0.015), (0.045, 0.042, 0.04), g.remap(sheen, 0.4, 0.7))
        coal = g.mix(coal, (0.006, 0.005, 0.005), crack)
        color = g.mix(color, coal, char)
        rough = g.node("ShaderNodeMix", data_type="FLOAT", Factor=char, A=rough,
                       B=g.remap(sheen, 0.3, 0.7, 0.62, 0.9)).outputs[0]
        height = g.node("ShaderNodeMix", data_type="FLOAT", Factor=char, A=height,
                        B=g.math("SUBTRACT", g.math("MULTIPLY", sheen, 0.3), crack)).outputs[0]
    g.set("Base Color", color)
    g.set("Roughness", rough)
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


def painted_wood(name, paint=(0.055, 0.095, 0.065), under=(0.18, 0.11, 0.055), wear=0.45,
                 grime=0.45, seed=0.0):
    """Scuffed oil-painted pine for shop fixtures. pcoord is part-local meters:
    fine wood grain shows through the paint, with worn brown undercoat, scratches,
    rubbed hand-polished bands and soot/grime in recesses."""
    g = Graph(name)
    p = g.coord()
    x, y, z = g.separate(p)
    seeded = g.vmath("ADD", p, (seed * 0.37, seed * 0.19, seed * 0.11))
    grain_vec = g.combine(g.math("MULTIPLY", x, 28.0), g.math("MULTIPLY", y, 28.0),
                          g.math("ADD", g.math("MULTIPLY", z, 1.7), seed))
    fibres = g.noise(grain_vec, scale=8.0, detail=8.0, roughness=0.62).outputs["Fac"]
    rings = g.wave(g.combine(x, y, g.math("MULTIPLY", z, 0.10)), scale=24.0,
                   distortion=8.0, detail=3.0, kind="RINGS", direction="Z").outputs["Fac"]
    wood_tone = g.math("ADD", g.math("MULTIPLY", fibres, 0.65), g.math("MULTIPLY", rings, 0.35))
    exposed = g.ramp(wood_tone, [(0.22, tuple(c * 0.48 for c in under)),
                                 (0.58, under),
                                 (0.90, tuple(min(1.0, c * 1.35) for c in under))])
    brush = g.noise(g.combine(g.math("MULTIPLY", x, 6.0), g.math("MULTIPLY", y, 9.0),
                              g.math("MULTIPLY", z, 0.45)), scale=10.0, detail=5.0,
                    roughness=0.58).outputs["Fac"]
    paint_col = g.mix(tuple(c * 0.72 for c in paint), tuple(min(1.0, c * 1.25) for c in paint),
                      g.remap(brush, 0.25, 0.75))
    scratches = g.noise(g.combine(g.math("MULTIPLY", x, 70.0), g.math("MULTIPLY", y, 70.0),
                                  g.math("MULTIPLY", z, 5.0)), scale=25.0, detail=4.0,
                        roughness=0.7).outputs["Fac"]
    scratch_mask = g.remap(scratches, 0.72, 0.82, 0.0, wear)
    long_rubs = g.wave(g.combine(g.math("MULTIPLY", x, 1.5), g.math("MULTIPLY", y, 1.5), z),
                       scale=18.0, distortion=9.0, detail=2.0, kind="BANDS",
                       direction="Z").outputs["Fac"]
    rub_mask = g.remap(long_rubs, 0.76, 0.92, 0.0, wear * 0.55)
    mask = g.math("MAXIMUM", scratch_mask, rub_mask)
    color = g.mix(paint_col, exposed, mask)
    dirt = g.noise(seeded, scale=38.0, detail=6.0, roughness=0.7).outputs["Fac"]
    ao = g.math("SUBTRACT", 1.0, g.ao(distance=0.05, samples=16))
    grime_mask = g.math("MAXIMUM", g.remap(dirt, 0.58, 0.78, 0.0, grime * 0.55),
                        g.math("MULTIPLY", ao, grime))
    color = g.mix(color, (0.035, 0.030, 0.024), grime_mask)
    g.set("Base Color", color)
    rough = g.math("ADD", 0.62, g.math("MULTIPLY", brush, 0.20))
    rough = g.math("SUBTRACT", rough, g.math("MULTIPLY", mask, 0.22))
    rough = g.math("MAXIMUM", rough, g.math("MULTIPLY", grime_mask, 0.88))
    g.set("Roughness", rough)
    height = g.math("ADD", g.math("MULTIPLY", fibres, 0.45), g.math("MULTIPLY", brush, 0.28))
    height = g.math("SUBTRACT", height, g.math("MULTIPLY", mask, 0.20))
    g.set("Normal", g.bump(height, strength=0.45, distance=0.0016))
    return g.mat


def aged_wood(name, light=(0.34, 0.23, 0.13), dark=(0.12, 0.075, 0.038), roughness=0.82,
              saw=0.5, grime=0.25, seed=0.0):
    """Rough stained or unfinished pine/deal boards: saw kerfs across the grain,
    torn fibres, dark checks and dirty handling marks."""
    g = Graph(name)
    p = g.coord()
    x, y, z = g.separate(p)
    seeded = g.vmath("ADD", p, (seed * 0.31, seed * 0.13, seed * 0.41))
    length = g.combine(g.math("MULTIPLY", x, 24.0), g.math("MULTIPLY", y, 24.0),
                       g.math("MULTIPLY", z, 1.4))
    fibres = g.noise(length, scale=7.0, detail=8.0, roughness=0.64).outputs["Fac"]
    rings = g.wave(g.combine(x, y, g.math("MULTIPLY", z, 0.08)), scale=20.0,
                   distortion=7.0, detail=3.0, kind="RINGS", direction="Z").outputs["Fac"]
    tone = g.math("ADD", g.math("MULTIPLY", fibres, 0.62), g.math("MULTIPLY", rings, 0.38))
    color = g.ramp(tone, [(0.2, dark), (0.6, light), (0.92, tuple(min(1.0, c * 1.25) for c in light))])
    saw_phase = g.math("ADD", g.math("MULTIPLY", x, 115.0), g.math("MULTIPLY", y, 19.0))
    kerf = g.math("ABSOLUTE", g.math("SINE", saw_phase))
    kerf_mask = g.remap(kerf, 0.91, 1.0, 0.0, saw)
    color = g.mix(color, tuple(c * 0.72 for c in dark), g.math("MULTIPLY", kerf_mask, 0.35))
    checks = g.voronoi(g.vmath("MULTIPLY", seeded, (1.0, 1.0, 0.2)), scale=18.0,
                       feature="DISTANCE_TO_EDGE").outputs["Distance"]
    crack = g.remap(checks, 0.0, 0.012, 0.45, 0.0)
    color = g.mix(color, (0.045, 0.030, 0.018), crack)
    dirt = g.noise(seeded, scale=45.0, detail=6.0, roughness=0.7).outputs["Fac"]
    color = g.mix(color, (0.060, 0.045, 0.030), g.remap(dirt, 0.55, 0.78, 0.0, grime))
    g.set("Base Color", color)
    rough = g.math("ADD", roughness, g.math("MULTIPLY", kerf_mask, 0.10))
    rough = g.math("MAXIMUM", rough, g.math("MULTIPLY", crack, 0.94))
    g.set("Roughness", rough)
    height = g.math("ADD", g.math("MULTIPLY", fibres, 0.50), g.math("MULTIPLY", kerf_mask, 0.35))
    height = g.math("SUBTRACT", height, g.math("MULTIPLY", crack, 0.4))
    g.set("Normal", g.bump(height, strength=0.55, distance=0.0022))
    return g.mat


def hessian(name, base=(0.34, 0.285, 0.19), dark=(0.13, 0.105, 0.070), dust=0.15, seed=0.0):
    """Plain woven jute/hessian: over-under warp and weft ribs, slubs, frayed dark
    holes and optional pale flour dust caught on high threads."""
    g = Graph(name)
    p = g.coord()
    x, y, z = g.separate(p)
    seeded = g.vmath("ADD", p, (seed * 0.21, seed * 0.47, seed * 0.16))
    warp = g.math("ABSOLUTE", g.math("SINE", g.math("MULTIPLY", x, 1050.0)))
    weft = g.math("ABSOLUTE", g.math("SINE", g.math("MULTIPLY", z, 920.0)))
    ribs = g.math("MAXIMUM", g.remap(warp, 0.90, 1.0), g.remap(weft, 0.90, 1.0))
    slub = g.noise(seeded, scale=80.0, detail=5.0, roughness=0.68).outputs["Fac"]
    color = g.mix(dark, base, g.remap(g.math("ADD", g.math("MULTIPLY", ribs, 0.42),
                                           g.math("MULTIPLY", slub, 0.58)), 0.18, 0.9))
    holes = g.voronoi(g.vmath("MULTIPLY", seeded, (1.2, 0.6, 1.0)), scale=58.0,
                      feature="F1").outputs["Distance"]
    hole_mask = g.remap(holes, 0.045, 0.018, 0.0, 0.28)
    color = g.mix(color, (0.045, 0.036, 0.026), hole_mask)
    dust_mask = g.math("MULTIPLY", dust, g.remap(g.noise(seeded, scale=24.0, detail=4.0).outputs["Fac"],
                                                 0.52, 0.78))
    color = g.mix(color, (0.72, 0.66, 0.54), dust_mask)
    g.set("Base Color", color)
    g.set("Roughness", 0.93)
    height = g.math("ADD", g.math("MULTIPLY", ribs, 0.8), g.math("MULTIPLY", slub, 0.35))
    height = g.math("SUBTRACT", height, g.math("MULTIPLY", hole_mask, 0.35))
    g.set("Normal", g.bump(height, strength=0.38, distance=0.0007))
    return g.mat


def paper(name, color=(0.58, 0.50, 0.36), string_shadow=0.25, seed=0.0):
    """Brown rag paper with fibre flecks and shallow creases for wrapped packets."""
    g = Graph(name)
    p = g.coord()
    seeded = g.vmath("ADD", p, (seed * 0.17, seed * 0.23, seed * 0.31))
    fibre = g.noise(seeded, scale=95.0, detail=5.0, roughness=0.7).outputs["Fac"]
    broad = g.noise(seeded, scale=12.0, detail=4.0).outputs["Fac"]
    base = g.mix(tuple(c * 0.72 for c in color), tuple(min(1.0, c * 1.18) for c in color),
                 g.remap(broad, 0.25, 0.75))
    flecks = g.remap(fibre, 0.65, 0.82, 0.0, 0.45)
    base = g.mix(base, (0.18, 0.13, 0.08), flecks)
    crease = g.voronoi(g.vmath("MULTIPLY", seeded, (1.0, 1.0, 0.4)), scale=21.0,
                       feature="DISTANCE_TO_EDGE").outputs["Distance"]
    crease_mask = g.remap(crease, 0.0, 0.011, string_shadow, 0.0)
    g.set("Base Color", g.mix(base, (0.16, 0.115, 0.07), crease_mask))
    g.set("Roughness", 0.88)
    g.set("Normal", g.bump(g.math("ADD", g.math("MULTIPLY", fibre, 0.25),
                                  g.math("MULTIPLY", crease_mask, -0.45)),
                           strength=0.42, distance=0.0009))
    return g.mat


def stoneware(name, glaze=(0.36, 0.32, 0.25), clay=(0.22, 0.12, 0.065), seed=0.0):
    """Salt-glazed stoneware: warm clay body, tan/grey glaze mottling, iron specks
    and a darker unglazed foot."""
    g = Graph(name)
    p = g.coord()
    x, y, z = g.separate(p)
    seeded = g.vmath("ADD", p, (seed * 0.43, seed * 0.12, seed * 0.29))
    mottle = g.noise(seeded, scale=34.0, detail=6.0, roughness=0.6).outputs["Fac"]
    speck = g.noise(seeded, scale=430.0, detail=2.0).outputs["Fac"]
    color = g.ramp(mottle, [(0.18, tuple(c * 0.65 for c in glaze)),
                            (0.55, glaze),
                            (0.9, tuple(min(1.0, c * 1.28) for c in glaze))])
    foot = g.remap(z, 0.045, 0.0)
    color = g.mix(color, clay, foot)
    color = g.mix(color, (0.12, 0.065, 0.035), g.remap(speck, 0.78, 0.88, 0.0, 0.45))
    g.set("Base Color", color)
    g.set("Roughness", g.math("ADD", 0.48, g.math("MULTIPLY", foot, 0.35)))
    g.set("Normal", g.bump(g.math("ADD", g.math("MULTIPLY", mottle, 0.28),
                                  g.math("MULTIPLY", speck, 0.12)),
                           strength=0.22, distance=0.0008))
    return g.mat


def tinplate(name, base=(0.50, 0.49, 0.45), rust=0.28, seed=0.0):
    """Dull nineteenth-century tinplate: grey metal with solder seams, rubbed edges,
    tea/cocoa staining and small rust freckles. Include a metallic bake map."""
    g = Graph(name)
    p = g.coord()
    seeded = g.vmath("ADD", p, (seed * 0.37, seed * 0.07, seed * 0.19))
    blotch = g.noise(seeded, scale=35.0, detail=5.0, roughness=0.65).outputs["Fac"]
    scratches = g.noise(g.vmath("MULTIPLY", seeded, (35.0, 35.0, 6.0)), scale=9.0,
                        detail=4.0).outputs["Fac"]
    color = g.mix(tuple(c * 0.72 for c in base), tuple(min(1.0, c * 1.2) for c in base),
                  g.remap(blotch, 0.25, 0.75))
    rust_noise = g.noise(seeded, scale=95.0, detail=5.0, roughness=0.7).outputs["Fac"]
    rust_mask = g.remap(rust_noise, 0.68, 0.82, 0.0, rust)
    color = g.mix(color, (0.28, 0.13, 0.045), rust_mask)
    rub = g.remap(scratches, 0.72, 0.88, 0.0, 0.35)
    color = g.mix(color, (0.62, 0.60, 0.55), rub)
    g.set("Base Color", color)
    g.set("Metallic", g.math("SUBTRACT", 1.0, rust_mask))
    g.set("Roughness", g.math("ADD", 0.43, g.math("MULTIPLY", rust_mask, 0.42)))
    g.set("Normal", g.bump(g.math("ADD", g.math("MULTIPLY", blotch, 0.3),
                                  g.math("MULTIPLY", scratches, 0.2)),
                           strength=0.25, distance=0.0006))
    return g.mat


def green_glass(name, tint=(0.10, 0.16, 0.12), grime=0.25, seed=0.0):
    """Dark green hand-blown bottle glass represented as baked opaque tinted glass:
    uneven thickness, seed bubbles, rubbed shoulders and dusty punt."""
    g = Graph(name)
    p = g.coord()
    seeded = g.vmath("ADD", p, (seed * 0.29, seed * 0.41, seed * 0.17))
    thickness = g.noise(seeded, scale=18.0, detail=5.0).outputs["Fac"]
    bubbles = g.voronoi(seeded, scale=90.0, feature="F1").outputs["Distance"]
    bubble_mask = g.remap(bubbles, 0.08, 0.02, 0.0, 0.55)
    color = g.mix(tuple(c * 0.55 for c in tint), tuple(min(1.0, c * 1.45) for c in tint),
                  g.remap(thickness, 0.25, 0.75))
    color = g.mix(color, (0.42, 0.48, 0.40), bubble_mask)
    dust = g.noise(seeded, scale=55.0, detail=5.0).outputs["Fac"]
    color = g.mix(color, (0.18, 0.16, 0.13), g.remap(dust, 0.62, 0.82, 0.0, grime))
    g.set("Base Color", color)
    g.set("Roughness", g.remap(thickness, 0.2, 0.8, 0.16, 0.42))
    g.set("Normal", g.bump(g.math("ADD", g.math("MULTIPLY", thickness, 0.3),
                                  g.math("MULTIPLY", bubble_mask, -0.2)),
                           strength=0.18, distance=0.0008))
    return g.mat


def candle_wax(name, color=(0.78, 0.70, 0.54), soot=0.15, seed=0.0):
    """Hand-dipped tallow/beeswax: creamy uneven wax, drips, wick soot and finger dents."""
    g = Graph(name)
    p = g.coord()
    x, y, z = g.separate(p)
    seeded = g.vmath("ADD", p, (seed * 0.13, seed * 0.17, seed * 0.19))
    mottle = g.noise(seeded, scale=38.0, detail=5.0, roughness=0.6).outputs["Fac"]
    angle = g.math("ARCTAN2", y, x)
    drip = g.noise(g.combine(g.math("MULTIPLY", angle, 2.0), g.math("MULTIPLY", z, 9.0), seed),
                   scale=12.0, detail=4.0).outputs["Fac"]
    color = g.mix(tuple(c * 0.85 for c in color), color, g.remap(mottle, 0.25, 0.75))
    color = g.mix(color, (0.12, 0.10, 0.08), g.remap(drip, 0.73, 0.86, 0.0, soot))
    g.set("Base Color", color)
    g.set("Roughness", 0.64)
    g.set("Normal", g.bump(g.math("ADD", g.math("MULTIPLY", mottle, 0.25),
                                  g.math("MULTIPLY", drip, 0.40)),
                           strength=0.25, distance=0.0007))
    return g.mat


# ------------------------------------------------------------------ granite

# Sierra Nevada granodiorite/granite, linear albedo. Plagioclase is chalky white, quartz a
# smoky translucent grey, K-feldspar cream to faint pink, biotite and hornblende black.
GRANITE_MINERALS = [
    (0.00, (0.02, 0.019, 0.018)),    # biotite
    (0.11, (0.036, 0.040, 0.034)),   # hornblende
    (0.17, (0.30, 0.305, 0.31)),     # quartz
    (0.33, (0.52, 0.515, 0.50)),     # plagioclase
    (0.80, (0.52, 0.48, 0.43)),      # K-feldspar
]
GRANITE_MEAN = (0.37, 0.36, 0.345)


def _granite_grains(g, vector, grain, w=None):
    """Interlocking mineral grains: returns (color, roughness, relief) sockets.

    Coarse, blocky feldspar grains (plagioclase, K-feldspar) with the spaces between
    them filled by finer quartz, biotite and hornblende, as in a real hypidiomorphic
    granodiorite; grain edges are warped so no cell polygon survives. ``w`` switches
    the textures to 4D (seamless torus-mapped tiles)."""
    dims = "4D" if w is not None else "3D"
    extra = {"W": w} if w is not None else {}
    warp = g.node("ShaderNodeTexNoise", Vector=vector, Scale=1.0 / (grain * 2.2), Detail=3.0,
                  noise_dimensions=dims, **extra).outputs["Color"]
    warped = g.vmath("ADD", vector, g.scale(g.vmath("SUBTRACT", warp, (0.5, 0.5, 0.5)), grain * 1.1))
    coarse = g.node("ShaderNodeTexVoronoi", Vector=warped, Scale=1.0 / (grain * 1.5), Randomness=1.0,
                    voronoi_dimensions=dims, feature="F1", **extra)
    fine = g.node("ShaderNodeTexVoronoi", Vector=warped, Scale=1.0 / (grain * 0.62), Randomness=1.0,
                  voronoi_dimensions=dims, feature="F1", **extra)
    pick_c = g.channel(coarse.outputs["Color"], 0)
    pick_f = g.channel(fine.outputs["Color"], 0)
    felsic = g.remap(pick_c, 0.349, 0.351)
    plag, kspar = GRANITE_MINERALS[3][1], GRANITE_MINERALS[4][1]
    big = g.ramp(pick_c, [(0.0, plag), (0.82, kspar)], interpolation="CONSTANT")
    small = g.ramp(pick_f, [(0.0, GRANITE_MINERALS[0][1]), (0.2, GRANITE_MINERALS[1][1]),
                            (0.3, GRANITE_MINERALS[2][1]), (0.75, plag)], interpolation="CONSTANT")
    color = g.mix(small, big, felsic)
    shade = g.math("ADD", g.math("MULTIPLY", felsic, g.channel(coarse.outputs["Color"], 1)),
                   g.math("MULTIPLY", g.math("SUBTRACT", 1.0, felsic), g.channel(fine.outputs["Color"], 1)))
    color = g.mix(color, (0.86, 0.86, 0.85), g.remap(shade, 0.0, 1.0, 0.0, 0.7), blend="MULTIPLY")
    cloud = g.node("ShaderNodeTexNoise", Vector=vector, Scale=1.0 / (grain * 0.6), Detail=2.0,
                   noise_dimensions=dims, **extra).outputs["Fac"]
    color = g.mix(color, (0.9, 0.9, 0.9), g.remap(cloud, 0.4, 0.65), blend="MULTIPLY")
    # Tiny biotite books scattered through the felsic grains.
    specks = g.node("ShaderNodeTexVoronoi", Vector=warped, Scale=1.0 / (grain * 0.4), Randomness=1.0,
                    voronoi_dimensions=dims, feature="F1", **extra)
    speck = g.math("MULTIPLY", g.remap(g.channel(specks.outputs["Color"], 2), 0.935, 0.94),
                   g.remap(specks.outputs["Distance"], 0.4, 0.28))
    color = g.mix(color, (0.018, 0.017, 0.016), speck)
    # Per mineral: quartz and mica glossier; feldspar stands proud of the weathered surface.
    mineral = g.math("ADD", g.math("MULTIPLY", felsic, 1.0),
                     g.math("MULTIPLY", g.math("SUBTRACT", 1.0, felsic), g.math("MULTIPLY", pick_f, 1.0)))
    rough = g.channel(g.ramp(mineral, [(0.0, (0.46,) * 3), (0.3, (0.64,) * 3), (0.75, (0.84,) * 3)],
                             interpolation="CONSTANT"), 0)
    rough = g.node("ShaderNodeMix", data_type="FLOAT", Factor=speck, A=rough, B=0.45).outputs[0]
    relief = g.channel(g.ramp(mineral, [(0.0, (0.35,) * 3), (0.3, (0.75,) * 3), (0.75, (1.0,) * 3)],
                              interpolation="CONSTANT"), 0)
    relief = g.math("ADD", relief, g.math("MULTIPLY", g.math("SUBTRACT", shade, 0.5), 0.25))
    relief = g.math("ADD", relief, g.math("MULTIPLY", cloud, 0.4))
    relief = g.math("SUBTRACT", relief, g.math("MULTIPLY", speck, 0.4))
    return color, rough, relief

def granite(name, grain=0.0045, grains=True, scale=1.0, patina=0.7, lichen=0.45, moss=0.12,
            iron=0.25, streaks=0.3, soil=0.2, soil_height=0.12, enclaves=0.4, megacrysts=0.0,
            relief=1.0, north=(0.0, 1.0, 0.0), seed=0.0, fresh="fresh", spots=1.0, film=0.5,
            offset_attr=None, tint_attr=None):
    """Weathered Sierra Nevada granite for rocks meshed in meters with pcoord = object
    coordinates and the ground line at z = 0.

    - ``grain``: mean crystal size (m). ``grains=False`` replaces the crystals by their
      average colour, for big rocks whose bake texels are coarser than the crystals
      (pair them with the shared tiling GraniteDetail maps).
    - ``scale``: rock size (m) that sizes the macro weathering features.
    - ``patina``: grey-buff weathering rind; ``lichen``/``moss`` coverage (moss prefers
      cavities and faces toward ``north`` near the ground); ``iron`` rust stains;
      ``streaks`` dark water streaks down steep faces; ``soil`` dirt up to
      ``soil_height`` m above the ground line (and everything buried below it);
      ``enclaves`` dark mafic inclusions; ``megacrysts`` Cathedral Peak style K-feldspar
      phenocrysts; ``relief`` multiplies the bump strength.
    - ``fresh``: name of a 0..1 point attribute marking newly spalled surfaces (written by
      ``homestead_rocks.sheets`` recipes); they keep a cleaner, paler face with less rind
      and lichen. Missing attributes read as 0 (all weathered).
    - ``offset_attr`` / ``tint_attr``: optional per-point vector attributes for masonry built
      from many stones: the offset (m) shifts every noise pattern so neighbouring stones
      don't continue each other's markings, and the tint (~1, 1, 1) multiplies each stone's
      colour. The soil line and facing masks still use the shared object coordinates.
    """
    g = Graph(name)
    p = g.coord()
    ps = g.vmath("ADD", p, (seed * 7.13, seed * 3.37, seed * 5.71))
    if offset_attr:
        shift = g.node("ShaderNodeAttribute", attribute_name=offset_attr, attribute_type="GEOMETRY")
        ps = g.vmath("ADD", ps, shift.outputs["Vector"])
    px, py, pz = g.separate(p)
    normal = g.node("ShaderNodeTexCoord").outputs["Normal"]
    nx, ny, nz = g.separate(normal)
    north_facing = g.vmath("DOT_PRODUCT", normal, north)
    steep = g.remap(g.math("ABSOLUTE", nz), 0.85, 0.35)
    upward = g.remap(nz, 0.1, 0.8)
    spalled = g.node("ShaderNodeAttribute", attribute_name=fresh, attribute_type="GEOMETRY").outputs["Fac"]
    weathered = g.math("SUBTRACT", 1.0, g.math("MULTIPLY", spalled, 0.85))
    joint_face = g.node("ShaderNodeAttribute", attribute_name="joint", attribute_type="GEOMETRY").outputs["Fac"]

    if grains:
        color, rough, height = _granite_grains(g, ps, grain)
    else:
        speck = g.noise(ps, scale=1.0 / (grain * 3.0), detail=3.0).outputs["Fac"]
        color = g.mix(GRANITE_MEAN, tuple(c * 0.82 for c in GRANITE_MEAN), g.remap(speck, 0.4, 0.62))
        rough = g.remap(speck, 0.3, 0.7, 0.72, 0.84)
        height = speck

    # Megacrysts and mafic enclaves.
    if megacrysts:
        mega = g.node("ShaderNodeTexVoronoi", Vector=g.scale(ps, (1.0, 1.0, 1.6)), Scale=1.0 / 0.055,
                      Randomness=1.0, feature="F1", distance="CHEBYCHEV")
        mega_mask = g.math("MULTIPLY", g.remap(g.channel(mega.outputs["Color"], 0), 1.0 - 0.14 * megacrysts,
                                               1.0 - 0.14 * megacrysts + 0.01),
                           g.remap(mega.outputs["Distance"], 0.3, 0.24))
        color = g.mix(color, (0.50, 0.455, 0.40), mega_mask)
        height = g.math("ADD", height, g.math("MULTIPLY", mega_mask, 0.3))
    if enclaves:
        warp = g.noise(ps, scale=4.0, detail=3.0).outputs["Color"]
        evec = g.vmath("ADD", g.scale(ps, (1.0, 0.7, 2.6)), g.scale(warp, 0.1))
        encl = g.node("ShaderNodeTexVoronoi", Vector=evec, Scale=1.0 / 0.5, Randomness=1.0, feature="F1")
        encl_mask = g.math("MULTIPLY", g.remap(g.channel(encl.outputs["Color"], 1), 1.0 - 0.06 * enclaves,
                                               1.0 - 0.06 * enclaves + 0.004),
                           g.remap(encl.outputs["Distance"], 0.3, 0.25))
        fine = g.noise(ps, scale=1.0 / (grain * 0.5), detail=2.0).outputs["Fac"]
        color = g.mix(color, g.mix((0.10, 0.10, 0.095), (0.2, 0.2, 0.19), g.remap(fine, 0.35, 0.65)),
                      g.math("MULTIPLY", encl_mask, 0.85))
    if tint_attr:
        tint = g.node("ShaderNodeAttribute", attribute_name=tint_attr, attribute_type="GEOMETRY")
        color = g.mix(color, tint.outputs["Color"], 1.0, blend="MULTIPLY")

    # Weathering rind: crystals lose contrast and the surface warms to grey-buff.
    zone = g.noise(ps, scale=1.1 / scale, detail=4.0, roughness=0.55).outputs["Fac"]
    rind = g.math("MULTIPLY", g.remap(zone, 0.3, 0.7, patina * 0.5, patina), weathered)
    color = g.mix(color, (0.27, 0.265, 0.25), g.math("MULTIPLY", rind, 0.65))
    color = g.mix(color, (0.93, 0.9, 0.84), g.math("MULTIPLY", rind, 0.6), blend="MULTIPLY")
    mottle = g.noise(ps, scale=4.5 / scale, detail=5.0, roughness=0.6).outputs["Fac"]
    color = g.mix(color, (0.72, 0.72, 0.7), g.remap(mottle, 0.4, 0.72, 0.0, 0.9), blend="MULTIPLY")
    aged = g.noise(ps, scale=0.8 / scale, detail=3.0).outputs["Fac"]
    color = g.mix(color, (0.68, 0.68, 0.66), g.math("MULTIPLY", g.remap(aged, 0.45, 0.68), weathered),
                  blend="MULTIPLY")
    light = g.noise(ps, scale=2.3 / scale, detail=3.0).outputs["Fac"]
    color = g.mix(color, (1.12, 1.1, 1.06), g.math("MULTIPLY", g.remap(light, 0.55, 0.75), weathered),
                  blend="MULTIPLY")

    # Iron-oxide stains: blotches round weathering biotite, and rusty runs down steep faces.
    if iron:
        blotch = g.noise(ps, scale=2.2 / scale, detail=5.0, roughness=0.62).outputs["Fac"]
        runs = g.noise(g.combine(g.math("MULTIPLY", px, 9.0 / scale), g.math("MULTIPLY", py, 9.0 / scale),
                                 g.math("MULTIPLY", pz, 0.7 / scale)), scale=1.4, detail=4.0).outputs["Fac"]
        rust = g.math("MAXIMUM", g.remap(blotch, 0.6, 0.78),
                      g.math("MULTIPLY", g.remap(runs, 0.55, 0.78), steep))
        color = g.mix(color, (0.78, 0.5, 0.3), g.math("MULTIPLY", g.math("MULTIPLY", rust, iron), weathered),
                      blend="MULTIPLY")

    # Dark water streaks (cyanobacteria/lichen) running down from rims on steep faces.
    # Old joint faces (``joint`` attribute) carry a rusty iron-oxide film.
    oxide = g.noise(ps, scale=3.0 / scale, detail=5.0, roughness=0.6).outputs["Fac"]
    rust_film = g.math("MULTIPLY", joint_face, g.remap(oxide, 0.3, 0.6, 0.45, 0.95))
    color = g.mix(color, (0.86, 0.74, 0.6), rust_film, blend="MULTIPLY")
    streak_mask = None
    if streaks:
        lines = g.noise(g.combine(g.math("MULTIPLY", px, 16.0 / scale), g.math("MULTIPLY", py, 16.0 / scale),
                                  g.math("MULTIPLY", pz, 0.45 / scale)), scale=1.3, detail=5.0).outputs["Fac"]
        wet = g.noise(ps, scale=0.9 / scale, detail=2.0).outputs["Fac"]
        streak_mask = g.math("MULTIPLY", g.math("MULTIPLY", g.remap(lines, 0.5, 0.68), steep),
                             g.remap(wet, 0.42, 0.6))
        streak_mask = g.math("MULTIPLY", streak_mask, g.math("MULTIPLY", weathered, streaks))
        color = g.mix(color, (0.05, 0.05, 0.045), g.math("MULTIPLY", streak_mask, 0.8))

    # Cavities hold dirt and moss.
    hollow = g.remap(g.ao(distance=0.06 * scale + 0.01, samples=16), 0.45, 0.95, 1.0, 0.0)
    color = g.mix(color, (0.6, 0.55, 0.48), g.math("MULTIPLY", hollow, 0.6), blend="MULTIPLY")
    cavity = g.remap(g.ao(distance=0.035, samples=12), 0.35, 0.9, 1.0, 0.0)
    color = g.mix(color, (0.55, 0.5, 0.44), g.math("MULTIPLY", cavity, 0.6), blend="MULTIPLY")

    # Old surfaces carry a patchy dark film of lichen and cyanobacteria: grey-olive mottling
    # that makes weathered Sierra boulders read mid-grey rather than white from a distance.
    if film:
        blot = g.noise(g.vmath("ADD", ps, g.scale(g.noise(ps, scale=1.5 / scale, detail=2.0).outputs["Color"],
                                                   0.25 * scale)), scale=2.2 / scale, detail=6.0,
                       roughness=0.62).outputs["Fac"]
        film_mask = g.math("MULTIPLY", g.remap(blot, 0.43, 0.57),
                           g.math("MULTIPLY", g.math("SUBTRACT", 1.0, g.math("MULTIPLY", spalled, 0.5)), film))
        film_mask = g.math("MULTIPLY", film_mask, g.math("ADD", 0.45, g.math("MULTIPLY", upward, 0.55)))
        color = g.mix(color, (0.42, 0.43, 0.39), film_mask, blend="MULTIPLY")

    # Crustose lichens. Colonies are irregular, coalescing crusts (a thresholded fBm, not
    # discs) in species zones: grey-green and pale grey crusts, chartreuse map lichen, and
    # small black crust/rock-tripe spots. Most grow on tops and cooler (north) faces.
    lichen_mask = None
    if lichen:
        lwarp = g.noise(ps, scale=9.0, detail=3.0).outputs["Color"]
        ls = max(1.0, scale / 1.5) ** 0.5   # colonies grow bigger on big, old rocks
        lvec = g.vmath("ADD", ps, g.scale(g.vmath("SUBTRACT", lwarp, (0.5, 0.5, 0.5)), 0.08 * ls))
        crustfield = g.noise(lvec, scale=1.0 / (0.11 * ls), detail=7.0, roughness=0.62).outputs["Fac"]
        colony = g.noise(ps, scale=1.3 / scale, detail=3.0).outputs["Fac"]
        exposure = g.math("ADD", g.math("MULTIPLY", upward, 0.55),
                          g.math("ADD", g.math("MULTIPLY", g.remap(north_facing, -0.3, 0.9), 0.3), 0.15))
        density = g.math("MULTIPLY", g.math("MULTIPLY", g.remap(colony, 0.3, 0.7, 0.2, 1.0), exposure),
                         g.math("MULTIPLY", weathered, lichen))
        threshold = g.math("SUBTRACT", 0.7, g.math("MULTIPLY", density, 0.45))
        edge = g.math("SUBTRACT", crustfield, threshold)
        lichen_mask = g.remap(edge, 0.0, 0.012)
        lichen_mask = g.math("MULTIPLY", lichen_mask, g.math("SUBTRACT", 1.0, g.math("MULTIPLY", cavity, 0.8)))
        species = g.noise(ps, scale=1.0 / 0.3, detail=2.0).outputs["Color"]
        pick = g.channel(species, 0)
        crust = g.ramp(pick, [(0.0, (0.2, 0.215, 0.18)), (0.36, (0.37, 0.385, 0.34)),
                              (0.58, (0.25, 0.275, 0.12)), (0.64, (0.14, 0.14, 0.125))], interpolation="CONSTANT")
        age = g.remap(edge, 0.0, 0.08, 0.75, 1.0)
        crust = g.mix(crust, (0.8, 0.8, 0.78), g.math("SUBTRACT", 1.0, age), blend="MULTIPLY")
        areoles = g.node("ShaderNodeTexVoronoi", Vector=lvec, Scale=1.0 / 0.004,
                         feature="DISTANCE_TO_EDGE").outputs["Distance"]
        crust = g.mix(crust, (0.55, 0.55, 0.52), g.remap(areoles, 0.0, 0.06, 0.45, 0.0), blend="MULTIPLY")
        # Map lichen colonies have a black prothallus rim.
        rim = g.math("MULTIPLY", g.remap(edge, 0.0, 0.004), g.remap(edge, 0.016, 0.006))
        is_map = g.math("MULTIPLY", g.remap(pick, 0.575, 0.585), g.remap(pick, 0.64, 0.63))
        crust = g.mix(crust, (0.03, 0.03, 0.028), g.math("MULTIPLY", rim, is_map))
        # Map lichen is areolate: yellow-green islands separated by the black prothallus.
        crust = g.mix(crust, (0.035, 0.035, 0.03),
                      g.math("MULTIPLY", g.remap(areoles, 0.0, 0.07, 1.0, 0.0), g.math("MULTIPLY", is_map, 0.9)))
        color = g.mix(color, crust, g.math("MULTIPLY", lichen_mask, 0.92))
        dots = g.node("ShaderNodeTexVoronoi", Vector=lvec, Scale=1.0 / (0.045 * ls), Randomness=1.0, feature="F1")
        spot_size = g.remap(g.channel(dots.outputs["Color"], 1), 0.0, 1.0, 0.08, 0.32)
        spot_on = g.remap(g.math("ADD", g.channel(dots.outputs["Color"], 2),
                                 g.math("MULTIPLY", density, 0.12 * spots)), 1.0 - 0.05 * spots,
                          1.0 - 0.05 * spots + 0.005)
        ragged = g.noise(lvec, scale=1.0 / 0.006, detail=4.0, roughness=0.7).outputs["Fac"]
        spot_edge = g.math("SUBTRACT", g.math("ADD", spot_size, g.math("MULTIPLY",
                           g.math("SUBTRACT", ragged, 0.5), 0.9)), dots.outputs["Distance"])
        spot_mask = g.math("MULTIPLY", g.remap(spot_edge, 0.0, 0.03), spot_on)
        color = g.mix(color, (0.045, 0.042, 0.036), g.math("MULTIPLY", spot_mask, g.remap(ragged, 0.3, 0.6, 0.75, 1.0)))
        lichen_mask = g.math("MAXIMUM", lichen_mask, spot_mask)

    # Moss cushions in crevices and on shaded faces near the ground.
    moss_mask = None
    if moss:
        clump = g.noise(ps, scale=9.0 / scale, detail=5.0, roughness=0.65).outputs["Fac"]
        shade = g.math("MULTIPLY", g.remap(north_facing, 0.05, 0.75),
                       g.remap(pz, min(0.9 * scale, 1.2), 0.05 * min(scale, 1.5)))
        where = g.math("MAXIMUM", g.math("MULTIPLY", g.math("MULTIPLY", hollow, shade), 1.4),
                       g.math("MULTIPLY", shade, g.remap(nz, -0.3, 0.4)))
        where = g.math("MINIMUM", where, 1.0)
        value = g.math("ADD", g.math("MULTIPLY", where, 0.5 + moss * 2.5),
                       g.math("MULTIPLY", g.math("SUBTRACT", clump, 0.5), 0.6))
        moss_mask = g.math("MULTIPLY", g.remap(value, 0.55, 0.64), g.remap(pz, -0.02, 0.02))
        tuft = g.noise(ps, scale=260.0, detail=4.0).outputs["Fac"]
        green = g.mix((0.035, 0.05, 0.016), (0.075, 0.09, 0.03), g.remap(tuft, 0.35, 0.7))
        green = g.mix(green, (0.10, 0.085, 0.04), g.remap(clump, 0.55, 0.75, 0.0, 0.6))
        color = g.mix(color, green, moss_mask)
    # Soil splash and the buried base.
    if soil:
        jitter = g.noise(ps, scale=12.0, detail=4.0).outputs["Fac"]
        line = g.math("ADD", pz, g.math("MULTIPLY", g.math("SUBTRACT", jitter, 0.5), soil_height * 0.8))
        dirt = g.math("MAXIMUM", g.remap(line, soil_height, 0.0, 0.0, soil),
                      g.remap(pz, 0.01, -0.03))
        color = g.mix(color, (0.085, 0.064, 0.045), dirt)
    else:
        dirt = None

    g.set("Base Color", color)
    roughness = rough
    for mask, value in ((streak_mask, 0.66), (lichen_mask, 0.9), (moss_mask, 0.95), (dirt, 0.95)):
        if mask is not None:
            roughness = g.node("ShaderNodeMix", data_type="FLOAT", Factor=mask, A=roughness, B=value).outputs[0]
    g.set("Roughness", roughness)

    grit = g.noise(ps, scale=1.0 / 0.0012, detail=2.0).outputs["Fac"]
    pitting = g.noise(ps, scale=1.0 / 0.012, detail=4.0, roughness=0.6).outputs["Fac"]
    fine = g.math("ADD", height, g.math("MULTIPLY", grit, 0.35))
    if lichen_mask is not None:
        fine = g.math("ADD", fine, g.math("MULTIPLY", lichen_mask, 0.6))
    if moss_mask is not None:
        fine = g.math("ADD", fine, g.math("MULTIPLY", moss_mask, g.math("MULTIPLY", tuft, 3.0)))
    coarse = g.math("ADD", g.math("MULTIPLY", pitting, 1.0),
                    g.math("MULTIPLY", g.noise(ps, scale=1.0 / 0.05, detail=4.0).outputs["Fac"], 1.5))
    g.math("ADD", fine, 0.0).node.label = "HOMESTEAD_HEIGHT"
    # Where crystals show through: 1 on bare rock, 0 under lichen, moss and soil. Big rocks
    # bake it (map role "mask") to gate the shared tiling crystal detail.
    cover = g.math("MAXIMUM", 0.0, 0.0)
    for mask in (lichen_mask, moss_mask, dirt):
        if mask is not None:
            cover = g.math("MAXIMUM", cover, mask)
    g.math("SUBTRACT", 1.0, g.math("MINIMUM", g.math("MULTIPLY", cover, 0.7), 1.0)).node.label = "HOMESTEAD_MASK"
    bumped = g.bump(coarse, strength=0.6 * relief, distance=0.006)
    g.set("Normal", g.bump(fine, strength=0.55 * relief, distance=0.0007, normal=bumped))
    return g.mat


def granite_detail(name, tile=1.0, grain=0.0045):
    """Seamless tiling crystal detail for the shared GraniteDetail maps. UV 0..1 on a
    ``tile`` x ``tile`` m plane is mapped onto a flat (Clifford) torus in 4D, so the
    grain texture repeats with no seam and no stretching. Base colour is normalised
    so its mean is linear 0.5 (multiply by 2 over a macro colour)."""
    g = Graph(name)
    u, v, _ = g.separate(g.uv())
    radius = tile / (2 * 3.14159265)
    au, av = g.math("MULTIPLY", u, 6.2831853), g.math("MULTIPLY", v, 6.2831853)
    vector = g.combine(g.math("MULTIPLY", g.math("COSINE", au), radius),
                       g.math("MULTIPLY", g.math("SINE", au), radius),
                       g.math("MULTIPLY", g.math("COSINE", av), radius))
    w = g.math("MULTIPLY", g.math("SINE", av), radius)
    color, rough, height = _granite_grains(g, vector, grain, w=w)
    g.set("Base Color", g.mix(color, tuple(0.5 / c for c in GRANITE_MEAN), 1.0, blend="MULTIPLY"))
    g.set("Roughness", rough)
    g.math("ADD", height, 0.0).node.label = "HOMESTEAD_HEIGHT"
    g.set("Normal", g.bump(height, strength=0.6, distance=0.0007))
    return g.mat


# ------------------------------------------------------------------ masonry, roofing, torches

def lime_mortar(name, color=(0.22, 0.21, 0.185), dirt=(0.09, 0.08, 0.065), grime=0.6, seed=0.0):
    """Weathered lime mortar pointing between fieldstones: off-white lime binder with
    visible sand and small grit, trowel-struck lumps, fine shrinkage cracks, rain-washed
    grime and green-black algae in the damp lower joints. pcoord in meters, ground at z = 0.
    Lime ages from near-white to a dull oatmeal grey (linear ~0.35-0.45)."""
    g = Graph(name)
    p = g.coord()
    ps = g.vmath("ADD", p, (seed * 3.1, seed * 1.7, seed * 2.3))
    _, _, z = g.separate(p)
    lumps = g.noise(ps, scale=28.0, detail=5.0, roughness=0.6).outputs["Fac"]
    sand = g.noise(ps, scale=900.0, detail=2.0).outputs["Fac"]
    grit = g.voronoi(ps, scale=260.0, feature="F1")
    grit_mask = g.math("MULTIPLY", g.remap(g.channel(grit.outputs["Color"], 0), 0.86, 0.87),
                       g.remap(grit.outputs["Distance"], 0.45, 0.3))
    base = g.mix(tuple(c * 0.86 for c in color), color, g.remap(lumps, 0.35, 0.68))
    base = g.mix(base, (0.82, 0.8, 0.76), g.remap(sand, 0.3, 0.7, 0.0, 0.5), blend="MULTIPLY")
    grit_color = g.mix((0.10, 0.095, 0.09), (0.28, 0.25, 0.21), g.channel(grit.outputs["Color"], 1))
    base = g.mix(base, grit_color, grit_mask)
    cracks = g.voronoi(ps, scale=55.0, feature="DISTANCE_TO_EDGE").outputs["Distance"]
    crack_mask = g.remap(cracks, 0.0, 0.025, 1.0, 0.0)
    base = g.mix(base, tuple(c * 0.45 for c in color), g.math("MULTIPLY", crack_mask, 0.7))
    stain = g.noise(g.vmath("MULTIPLY", ps, (6.0, 6.0, 1.2)), scale=3.0, detail=4.0).outputs["Fac"]
    low = g.remap(z, 0.9, 0.0)
    base = g.mix(base, dirt, g.math("MULTIPLY", g.remap(stain, 0.45, 0.7, 0.0, grime),
                                     g.math("ADD", 0.35, g.math("MULTIPLY", low, 0.65))))
    algae = g.noise(ps, scale=9.0, detail=5.0).outputs["Fac"]
    base = g.mix(base, (0.05, 0.06, 0.035), g.math("MULTIPLY", g.remap(algae, 0.55, 0.7, 0.0, 0.6), low))
    recess = g.remap(g.ao(distance=0.03, samples=12), 0.4, 0.95, 1.0, 0.0)
    base = g.mix(base, (0.55, 0.5, 0.44), g.math("MULTIPLY", recess, 0.7), blend="MULTIPLY")
    g.set("Base Color", base)
    g.set("Roughness", g.remap(sand, 0.3, 0.7, 0.86, 0.96))
    height = g.math("ADD", g.math("MULTIPLY", lumps, 1.0), g.math("MULTIPLY", sand, 0.25))
    height = g.math("SUBTRACT", height, g.math("MULTIPLY", crack_mask, 0.4))
    height = g.math("ADD", height, g.math("MULTIPLY", grit_mask, 0.3))
    # Pointed by trowel: broad lumpy ridges and hollows under the sand grain.
    trowel = g.noise(g.vmath("MULTIPLY", ps, (1.0, 1.0, 1.6)), scale=9.0, detail=3.0, roughness=0.55).outputs["Fac"]
    broad = g.bump(trowel, strength=0.9, distance=0.012)
    g.set("Normal", g.bump(height, strength=0.7, distance=0.003, normal=broad))
    return g.mat


def slate(name, dark=(0.045, 0.05, 0.055), light=(0.12, 0.125, 0.13), lichen=0.35, moss=0.15,
          seed=0.0, offset_attr=None):
    """Split roofing stone (grey-blue slate / flaggy sandstone tilestone). pcoord is the
    slate-local frame in meters: x/y across the plate, z through its thickness, so the
    cleavage laminae show as fine bands on the chipped edges. Weathered upper faces carry
    pale crustose lichen and a grey bloom; the tails keep moss and dark water stains.
    ``offset_attr`` shifts the patterns per slate."""
    g = Graph(name)
    p = g.coord()
    ps = g.vmath("ADD", p, (seed * 2.3, seed * 4.1, seed * 1.3))
    if offset_attr:
        shift = g.node("ShaderNodeAttribute", attribute_name=offset_attr, attribute_type="GEOMETRY")
        ps = g.vmath("ADD", ps, shift.outputs["Vector"])
    x, y, z = g.separate(ps)
    normal = g.node("ShaderNodeTexCoord").outputs["Normal"]
    _, _, nz = g.separate(normal)
    face = g.remap(nz, 0.35, 0.8)
    laminae = g.noise(g.combine(g.math("MULTIPLY", x, 3.0), g.math("MULTIPLY", y, 3.0),
                                g.math("MULTIPLY", z, 900.0)), scale=1.0, detail=3.0).outputs["Fac"]
    mottle = g.noise(ps, scale=7.0, detail=5.0, roughness=0.6).outputs["Fac"]
    color = g.mix(dark, light, g.remap(mottle, 0.35, 0.72))
    color = g.mix(color, (0.8, 0.8, 0.82), g.math("MULTIPLY", g.remap(laminae, 0.4, 0.7),
                                                 g.math("SUBTRACT", 1.0, face)), blend="MULTIPLY")
    rust = g.noise(ps, scale=4.0, detail=4.0).outputs["Fac"]
    color = g.mix(color, (1.35, 1.05, 0.8), g.remap(rust, 0.64, 0.76, 0.0, 0.6), blend="MULTIPLY")
    bloom = g.noise(ps, scale=2.5, detail=3.0).outputs["Fac"]
    color = g.mix(color, (0.2, 0.2, 0.19), g.math("MULTIPLY", g.remap(bloom, 0.45, 0.7, 0.0, 0.45), face))
    grain = g.noise(ps, scale=1400.0, detail=2.0).outputs["Fac"]
    lichen_mask = None
    if lichen:
        crust = g.noise(ps, scale=11.0, detail=7.0, roughness=0.62).outputs["Fac"]
        colony = g.noise(ps, scale=1.6, detail=2.0).outputs["Fac"]
        threshold = g.math("SUBTRACT", 0.72, g.math("MULTIPLY", g.remap(colony, 0.3, 0.7), 0.35 * lichen))
        lichen_mask = g.math("MULTIPLY", g.remap(g.math("SUBTRACT", crust, threshold), 0.0, 0.015), face)
        species = g.channel(g.noise(ps, scale=3.0, detail=1.0).outputs["Color"], 0)
        crust_color = g.ramp(species, [(0.0, (0.30, 0.31, 0.28)), (0.5, (0.36, 0.33, 0.16)),
                                       (0.62, (0.22, 0.24, 0.20))], interpolation="CONSTANT")
        color = g.mix(color, crust_color, g.math("MULTIPLY", lichen_mask, 0.9))
    moss_mask = None
    if moss:
        tail = g.node("ShaderNodeAttribute", attribute_name="slate_tail", attribute_type="GEOMETRY").outputs["Fac"]
        clump = g.noise(ps, scale=30.0, detail=5.0).outputs["Fac"]
        moss_mask = g.math("MULTIPLY", g.remap(g.math("ADD", g.math("MULTIPLY", tail, moss * 3.0),
                                                      g.math("SUBTRACT", clump, 0.5)), 0.5, 0.62), face)
        tuft = g.noise(ps, scale=400.0, detail=3.0).outputs["Fac"]
        color = g.mix(color, g.mix((0.03, 0.045, 0.015), (0.07, 0.085, 0.03), tuft), moss_mask)
    cavity = g.remap(g.ao(distance=0.02, samples=12), 0.4, 0.95, 1.0, 0.0)
    color = g.mix(color, (0.5, 0.5, 0.5), g.math("MULTIPLY", cavity, 0.6), blend="MULTIPLY")
    g.set("Base Color", color)
    rough = g.remap(mottle, 0.3, 0.7, 0.58, 0.74)
    for mask, value in ((lichen_mask, 0.9), (moss_mask, 0.95)):
        if mask is not None:
            rough = g.node("ShaderNodeMix", data_type="FLOAT", Factor=mask, A=rough, B=value).outputs[0]
    g.set("Roughness", rough)
    ripple = g.noise(g.vmath("MULTIPLY", ps, (1.0, 5.0, 1.0)), scale=18.0, detail=4.0).outputs["Fac"]
    height = g.math("ADD", g.math("MULTIPLY", ripple, 0.6), g.math("MULTIPLY", grain, 0.15))
    height = g.math("ADD", height, g.math("MULTIPLY", laminae, g.math("SUBTRACT", 1.0, face)))
    if lichen_mask is not None:
        height = g.math("ADD", height, g.math("MULTIPLY", lichen_mask, 0.3))
    g.set("Normal", g.bump(height, strength=0.45, distance=0.002))
    return g.mat


def pitch_rag(name, spent=False, cloth=(0.33, 0.27, 0.19), seed=0.0):
    """Torch head: strips of coarse linen wound round the stake and soaked in pine pitch.
    pcoord is the tube frame (x, y, arclength). Fresh: a brown-black soak through the
    whole wrap with the plain weave still reading through it, glossy black clots of tar
    in the hollows and runs, and dull brown cloth where the strip edges stand proud and
    soaked thin; fibres fuzz at the edges. ``spent``: the pitch has burnt off, leaving
    brittle charred cloth, grey ash and white ash ghosts of the weave, dry and matte."""
    g = Graph(name)
    p = g.coord()
    ps = g.vmath("ADD", p, (seed * 1.1, seed * 2.9, seed * 0.7))
    x, y, s = g.separate(p)
    angle = g.math("ARCTAN2", y, x)
    # Unrolled cloth coordinates: around the head (meters of circumference) and along it.
    around = g.math("MULTIPLY", angle, 0.045)
    cloth_uv = g.combine(around, s, seed)
    warp_threads = g.wave(cloth_uv, scale=380.0, kind="BANDS", direction="X", distortion=0.6,
                          detail=1.0).outputs["Fac"]
    weft_threads = g.wave(cloth_uv, scale=340.0, kind="BANDS", direction="Y", distortion=0.6,
                          detail=1.0).outputs["Fac"]
    weave = g.math("MULTIPLY", g.remap(warp_threads, 0.2, 0.9), g.remap(weft_threads, 0.2, 0.9))
    fuzz = g.noise(ps, scale=900.0, detail=3.0, roughness=0.7).outputs["Fac"]
    soak = g.noise(ps, scale=28.0, detail=5.0, roughness=0.6).outputs["Fac"]
    curvature = g.node("ShaderNodeNewGeometry").outputs["Pointiness"]
    hollow = g.remap(curvature, 0.5, 0.44)
    proud = g.remap(curvature, 0.52, 0.6)
    if not spent:
        clots = g.math("ADD", g.math("MULTIPLY", hollow, 0.55), g.remap(soak, 0.5, 0.68))
        drips = g.noise(g.combine(g.math("MULTIPLY", angle, 1.3), 0.0, g.math("MULTIPLY", s, 9.0)),
                        scale=6.0, detail=3.0).outputs["Fac"]
        clots = g.math("MINIMUM", g.math("MAXIMUM", clots, g.remap(drips, 0.6, 0.66)), 1.0)
        bare = g.math("MULTIPLY", g.math("ADD", g.math("MULTIPLY", proud, 0.7),
                                         g.remap(soak, 0.36, 0.26, 0.0, 0.6)),
                      g.math("SUBTRACT", 1.0, clots))
        cloth_color = g.mix(tuple(c * 0.55 for c in cloth), cloth, g.remap(fuzz, 0.35, 0.65))
        cloth_color = g.mix(cloth_color, (0.55, 0.5, 0.45), g.math("SUBTRACT", 1.0, weave), blend="MULTIPLY")
        soaked = g.mix((0.028, 0.018, 0.010), (0.075, 0.048, 0.026), g.math("MULTIPLY", weave, 0.8))
        soaked = g.mix(soaked, (0.05, 0.035, 0.02), g.remap(soak, 0.3, 0.7, 0.0, 0.5))
        tar = g.mix((0.010, 0.007, 0.004), (0.022, 0.013, 0.006), g.remap(soak, 0.5, 0.8))
        color = g.mix(soaked, cloth_color, g.math("MINIMUM", bare, 1.0))
        color = g.mix(color, tar, clots)
        rough = g.remap(fuzz, 0.3, 0.7, 0.52, 0.66)
        rough = g.node("ShaderNodeMix", data_type="FLOAT", Factor=g.math("MINIMUM", bare, 1.0), A=rough,
                       B=0.9).outputs[0]
        rough = g.node("ShaderNodeMix", data_type="FLOAT", Factor=clots, A=rough,
                       B=g.remap(fuzz, 0.3, 0.7, 0.16, 0.3)).outputs[0]
        # Tar fills the weave: its relief fades under the clots.
        height = g.math("ADD", g.math("MULTIPLY", weave, g.math("SUBTRACT", 1.0, g.math("MULTIPLY", clots, 0.85))),
                        g.math("MULTIPLY", clots, g.math("MULTIPLY", soak, 1.4)))
        height = g.math("ADD", height, g.math("MULTIPLY", g.math("MULTIPLY", fuzz, bare), 0.4))
        strength = 0.45
    else:
        ash = g.remap(g.math("ADD", g.math("MULTIPLY", weave, 0.55), g.math("MULTIPLY", fuzz, 0.45)), 0.45, 0.75)
        ash = g.math("MULTIPLY", ash, g.math("ADD", 0.35, g.math("MULTIPLY", proud, 1.0)))
        char = g.mix((0.012, 0.011, 0.011), (0.04, 0.038, 0.036), g.remap(soak, 0.35, 0.75))
        cracks = g.voronoi(ps, scale=160.0, feature="DISTANCE_TO_EDGE").outputs["Distance"]
        char = g.mix(char, (0.004, 0.004, 0.004), g.remap(cracks, 0.0, 0.08, 1.0, 0.0))
        grey = g.mix((0.16, 0.155, 0.15), (0.42, 0.41, 0.39), g.remap(fuzz, 0.45, 0.8))
        edge = g.remap(soak, 0.6, 0.75, 0.0, 0.7)
        color = g.mix(char, grey, g.math("MINIMUM", g.math("MAXIMUM", g.math("MULTIPLY", ash, 0.6), edge), 1.0))
        rough = g.remap(ash, 0.0, 1.0, 0.82, 0.97)
        height = g.math("SUBTRACT", weave, g.remap(cracks, 0.0, 0.08, 0.8, 0.0))
        strength = 0.7
    g.set("Base Color", color)
    g.set("Roughness", rough)
    g.set("Normal", g.bump(height, strength=strength, distance=0.0012))
    return g.mat

def wrought_iron(name, rust=0.4, wear=0.35, seed=0.0):
    """Blacksmith-forged wrought iron (brackets, straps, nails): black-brown forge scale
    with hammer dimples and the fibrous slag streaks of wrought iron along part-local Z,
    rust blooming in pits and on upper surfaces, grey steel showing on worn edges.
    Metallic ~0.75 on scale, 1 on bare iron, 0 on rust."""
    g = Graph(name)
    p = g.coord()
    ps = g.vmath("ADD", p, (seed * 0.7, seed * 1.9, seed * 1.3))
    _, _, z = g.separate(ps)
    scale_noise = g.noise(ps, scale=60.0, detail=5.0, roughness=0.65).outputs["Fac"]
    fibre = g.noise(g.vmath("MULTIPLY", ps, (40.0, 40.0, 3.0)), scale=4.0, detail=4.0).outputs["Fac"]
    blooms = g.noise(ps, scale=14.0, detail=5.0, roughness=0.7).outputs["Fac"]
    pits = g.noise(ps, scale=380.0, detail=2.0).outputs["Fac"]
    rust_mask = g.math("MINIMUM", g.math("ADD", g.remap(blooms, 0.62 - 0.12 * rust, 0.72 - 0.12 * rust),
                                        g.math("MULTIPLY", g.remap(pits, 0.66, 0.74), rust)), 1.0)
    curvature = g.node("ShaderNodeNewGeometry").outputs["Pointiness"]
    worn = g.math("MULTIPLY", g.remap(curvature, 0.52, 0.6), wear)
    forge = g.mix((0.028, 0.026, 0.025), (0.07, 0.062, 0.055), g.remap(scale_noise, 0.35, 0.72))
    forge = g.mix(forge, (0.7, 0.7, 0.72), g.remap(fibre, 0.45, 0.62, 0.0, 0.35), blend="MULTIPLY")
    rust_color = g.mix((0.07, 0.03, 0.014), (0.19, 0.075, 0.03), g.remap(pits, 0.5, 0.8))
    color = g.mix(forge, (0.34, 0.33, 0.32), worn)
    color = g.mix(color, rust_color, rust_mask)
    g.set("Base Color", color)
    metal = g.math("ADD", 0.72, g.math("MULTIPLY", worn, 0.28))
    g.set("Metallic", g.math("MULTIPLY", metal, g.math("SUBTRACT", 1.0, rust_mask)))
    rough = g.remap(scale_noise, 0.3, 0.7, 0.55, 0.7)
    rough = g.node("ShaderNodeMix", data_type="FLOAT", Factor=worn, A=rough, B=0.38).outputs[0]
    rough = g.node("ShaderNodeMix", data_type="FLOAT", Factor=rust_mask, A=rough, B=0.9).outputs[0]
    g.set("Roughness", rough)
    dimples = g.voronoi(ps, scale=90.0, feature="SMOOTH_F1").outputs["Distance"]
    height = g.math("ADD", g.math("MULTIPLY", g.math("MULTIPLY", dimples, dimples), 1.0),
                    g.math("MULTIPLY", fibre, 0.25))
    height = g.math("ADD", height, g.math("MULTIPLY", rust_mask, g.math("MULTIPLY", pits, 0.5)))
    g.set("Normal", g.bump(height, strength=0.6, distance=0.0015))
    return g.mat
