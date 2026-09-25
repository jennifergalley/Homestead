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
         weathering=0.0, grime=0.0, seed=0.0):
    """Stripped/seasoned wood: fine long grain along Z, growth-ring banding,
    long tonal streaks, pores, optional grey weathering and dark handling grime."""
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
    g.set("Base Color", color)
    g.set("Roughness", g.remap(fibres, 0.3, 0.7, roughness - 0.08, roughness + 0.1))
    height = g.math("ADD", g.math("MULTIPLY", fibres, 1.0), g.math("MULTIPLY", pores, 0.4))
    g.set("Normal", g.bump(height, strength=0.35, distance=0.0015))
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
