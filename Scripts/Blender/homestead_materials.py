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
            relief=1.0, north=(0.0, 1.0, 0.0), seed=0.0, fresh="fresh", spots=1.0, film=0.5):
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
    """
    g = Graph(name)
    p = g.coord()
    ps = g.vmath("ADD", p, (seed * 7.13, seed * 3.37, seed * 5.71))
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
