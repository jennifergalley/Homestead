"""Fire-hardened digging stick: a slightly crooked hardwood sapling, bark stripped and
knife-shaved, with a charred, sharpened and flattened chisel point, a hand-polished
upper grip and a short greased-leather grip wrap tied off with a thong.

Real-object research (written before modeling):
- Digging sticks are the oldest root-gathering tool; in California and the Great Basin
  they were ~0.9-1.2 m long, 2.5-3.5 cm thick, cut from mountain mahogany, oak,
  manzanita or buckbrush, and used two-handed: stabbed in, then levered back.
- The working end is shaved to a chisel or spatula point 2-3 cm wide, then hardened by
  holding it in hot coals and scraping off the loose char, repeated several times. That
  leaves the last 10-15 cm black with fine alligator (checkered) char cracks, a scorched
  brown fringe above it, and the very tip abraded dull brown-black by soil.
- Side branches are trimmed flush, leaving knot stubs; the stripped sapling keeps its
  natural crooks; long shallow facets from shaving run along it; the top is blunt and a
  little battered. Hands polish and darken the upper grip; some users add a wrap.

Rigid static mesh; units are meters, Z up.
PIVOT: the origin is the centre of the upper hand's grip, 25 cm below the blunt top end.
The top is toward +Z (z +0.25); the chisel point is toward -Z (tip edge at z -0.75).
The chisel point is flattened front-to-back: its broad faces face +-Y (the front face
-Y), and the tip edge runs along X. The grip wrap is centred on the pivot.
Everything is generated here: no scanned or downloaded geometry or textures.
"""
import math

from mathutils import Vector, noise

NAME = "DiggingStick"
DESCRIPTION = ("Fire-hardened hardwood digging stick with charred chisel point and leather grip wrap "
               "(original). Pivot = upper-hand grip 25 cm below the top; point -Z, blunt top +Z.")
COLLISION = "none"
TRIANGLE_BUDGET = 10000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 96}
# Review lying on the ground; close-up on the charred chisel point.
BEAUTY = {"pose": (0, 90, 30), "focus": (0.0, 0.0, -0.69)}

Z_TOP = 0.25
Z_TIP = -0.75
LENGTH = Z_TOP - Z_TIP
R = 0.0138
CHISEL = 0.135           # length of the shaved, flattened point
CHAR_FROM = 0.80         # arclength from the top where scorching starts
CHAR_FULL = 0.875        # fully charred below this


def smoothstep(a, b, x):
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3 - 2 * t)


def _raw_axis(z):
    s = (Z_TOP - z) / LENGTH
    return Vector((0.011 * math.sin(2.6 * s + 0.4) + 0.006 * math.exp(-((s - 0.62) / 0.08) ** 2),
                   0.006 * math.sin(4.1 * s) - 0.004 * s, z))


_OFFSET = _raw_axis(0.0)


def axis(z):
    """Crooked centreline passing exactly through the pivot at the origin."""
    p = _raw_axis(z) - Vector((_OFFSET.x, _OFFSET.y, 0.0))
    return p


def radius(s):
    """s = arclength fraction from the blunt top (0) to the tip (1)."""
    r = R * (1.02 - 0.08 * s)
    top = min(s / 0.012, 1.0)
    r *= 0.72 + 0.28 * math.sqrt(1 - (1 - top) ** 2)
    return r


def stick_zs():
    zs = [Z_TOP - d for d in (0.0, 0.0015, 0.004, 0.008, 0.013)]
    body_end = Z_TIP + CHISEL
    n = 70
    zs += [Z_TOP - 0.02 - (Z_TOP - 0.02 - body_end) * i / n for i in range(n + 1)]
    m = 26
    zs += [body_end - (body_end - Z_TIP) * (1 - (1 - i / m) ** 1.6) for i in range(1, m + 1)]
    return zs


def build_stick(kit, wood):
    zs = stick_zs()
    stick = kit.tube("Stick", [axis(z) for z in zs], radius=radius, sides=20, material=wood)
    knots = [(0.08, 1.9, 0.0028), (0.31, 4.6, 0.0035), (0.47, 0.6, 0.0025), (0.66, 3.3, 0.003)]

    def relief(co, pco):
        s = pco.z / LENGTH
        angle = math.atan2(pco.y, pco.x)
        bump = noise.noise(Vector((pco.x * 70, pco.y * 70, pco.z * 7))) * 0.0005
        facet = 6 * angle + 1.8 * noise.noise(Vector((0.0, 0.0, pco.z * 3.0)))
        bump -= 0.0004 * abs(math.cos(facet / 2))
        for ks, ka, h in knots:
            d = ((s - ks) * LENGTH / 0.011) ** 2 + (math.atan2(math.sin(angle - ka), math.cos(angle - ka)) / 0.42) ** 2
            bump += h * math.exp(-d)
        # Top end battered: a few flattened chips.
        bump -= 0.0008 * smoothstep(0.012, 0.0, s) * (0.5 + 0.5 * noise.noise(Vector((pco.x * 400, pco.y * 400, 1))))
        return bump * (1 - smoothstep(1 - CHISEL / LENGTH - 0.02, 1 - CHISEL / LENGTH + 0.03, s))

    kit.displace(stick, relief)

    def chisel(co):
        t = smoothstep(Z_TIP + CHISEL, Z_TIP, co.z) if co.z < Z_TIP + CHISEL else 0.0
        if t <= 0.0:
            return co
        c = axis(co.z)
        d = co - Vector((c.x, c.y, co.z))
        # Flatten front-to-back (Y) into a spatula, widen very slightly along X, and
        # round the corners of the tip edge.
        u = (Z_TIP + CHISEL - co.z) / CHISEL
        thick = 1 - 0.87 * u ** 1.1
        wide = (1 + 0.14 * math.sin(math.pi * min(u / 0.8, 1.0) * 0.5)) * (1 - 0.35 * max(0.0, u - 0.9) / 0.1)
        # Slight one-sided bevel: the back (+Y) face is shaved more, so the edge sits forward.
        shift = -0.0022 * u ** 1.5
        return Vector((c.x + d.x * wide, c.y + d.y * thick + shift, co.z))

    kit.warp(stick, chisel)
    return stick


def build_wrap(kit, leather, z0=-0.038, z1=0.038, turns=4.6, width=0.0175, thick=0.0016):
    """Helical greased-leather strip, each turn overlapping the one below."""
    rows, coords = [], []
    steps = int(turns * 26)
    section = [(-0.5, 0.0), (-0.47, 0.6), (-0.3, 1.0), (0.3, 1.0), (0.47, 0.6), (0.5, 0.0), (0.2, -0.05), (-0.2, -0.05)]
    for i in range(steps + 1):
        t = i / steps
        z = z0 + (z1 - z0) * t
        angle = 2 * math.pi * turns * t
        c = axis(z)
        s = (Z_TOP - z) / LENGTH
        base_r = radius(s) - 0.0003
        radial = Vector((math.cos(angle), math.sin(angle), 0.0))
        # Strip tilts with the helix pitch; overlap lifts each turn slightly.
        pitch = (z1 - z0) / turns
        tangent_z = Vector((0, 0, 1.0))
        ring, pco = [], []
        taper = smoothstep(0.0, 0.04, t) * smoothstep(1.0, 0.95, t)
        w = width * (0.55 + 0.45 * taper)
        for a, b in section:
            along = a * w
            out = base_r + thick * (b + 0.35) * (0.4 + 0.6 * taper) + 0.0006 * (a + 0.5)
            p = c + radial * out + tangent_z * (along - a * pitch * 0.05)
            ring.append(tuple(p))
            pco.append((a * 0.02, b * 0.002, t * turns * 2 * math.pi * base_r))
        rows.append(ring)
        coords.append(pco)
    return kit.loft("Wrap", rows, material=leather, coords=coords, cap_start=True, cap_end=True)


def build_thong(kit, cord, z=0.041, turns=2.2):
    points = []
    steps = int(turns * 16)
    for i in range(steps + 1):
        t = i / steps
        angle = 0.8 + 2 * math.pi * turns * t
        zz = z + 0.004 * t
        c = axis(zz)
        r = radius((Z_TOP - zz) / LENGTH) + 0.0017
        points.append(c + Vector((math.cos(angle) * r, math.sin(angle) * r, 0.0)))
    # Knot, then the loose end hangs down over the wrap.
    last = points[-1]
    radial = (last - axis(last.z)).normalized()
    for k in range(1, 12):
        f = k / 11
        points.append(last + radial * (0.004 * math.sin(math.pi * f) + 0.0015 * f) +
                      Vector((0.0015 * math.sin(f * 5), 0.001 * f, -0.042 * f)))
    radii = [0.0011] * (steps + 1) + [0.0011 * (1 - 0.3 * k / 11) for k in range(1, 12)]
    return kit.tube("Thong", points, radii=radii, sides=6, material=cord)


def _link_value(tree, socket):
    return socket.links[0].from_socket if socket.links else None


def char_overlay(mat):
    """Blend fire-hardening into a wood material by pcoord arclength (z): scorched
    brown fringe, then black alligator-checked char, the very tip abraded by soil."""
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = nodes["Principled BSDF"]

    def node(kind, **inputs):
        n = nodes.new(kind)
        for key, value in inputs.items():
            if key in n.inputs:
                if hasattr(value, "is_linked") or hasattr(value, "node"):
                    links.new(value, n.inputs[key])
                else:
                    n.inputs[key].default_value = value
            else:
                setattr(n, key, value)
        return n

    def maprange(value, a, b, c=0.0, d=1.0):
        return node("ShaderNodeMapRange", Value=value, **{"From Min": a, "From Max": b, "To Min": c, "To Max": d}).outputs[0]

    def mix(a, b, fac):
        n = node("ShaderNodeMix", data_type="RGBA")
        sockets = [s for s in n.inputs if s.type == "RGBA"]
        links.new(fac, n.inputs["Factor"])
        for sock, v in zip(sockets, (a, b)):
            if isinstance(v, tuple):
                sock.default_value = (*v, 1.0)
            else:
                links.new(v, sock)
        return next(s for s in n.outputs if s.type == "RGBA")

    def fmix(a, b, fac):
        n = node("ShaderNodeMix", data_type="FLOAT")
        links.new(fac, n.inputs["Factor"])
        for idx, v in ((2, a), (3, b)):
            if isinstance(v, float):
                n.inputs[idx].default_value = v
            else:
                links.new(v, n.inputs[idx])
        return n.outputs[0]

    attr = node("ShaderNodeAttribute", attribute_name="pcoord", attribute_type="GEOMETRY")
    sep = node("ShaderNodeSeparateXYZ", Vector=attr.outputs["Vector"])
    s = node("ShaderNodeMath", operation="DIVIDE", **{}).outputs[0]
    div = s.node
    links.new(sep.outputs["Z"], div.inputs[0])
    div.inputs[1].default_value = LENGTH
    ragged = node("ShaderNodeTexNoise", Vector=attr.outputs["Vector"], Scale=90.0, Detail=4.0).outputs["Fac"]
    edge = node("ShaderNodeMath", operation="ADD").outputs[0]
    links.new(s, edge.node.inputs[0])
    off = node("ShaderNodeMath", operation="MULTIPLY").outputs[0]
    links.new(ragged, off.node.inputs[0])
    off.node.inputs[1].default_value = 0.03
    links.new(off, edge.node.inputs[1])
    scorch = maprange(edge, CHAR_FROM, CHAR_FULL - 0.01)
    char = maprange(edge, CHAR_FULL - 0.012, CHAR_FULL + 0.015)
    abrade = maprange(edge, 0.975, 1.0)

    stretched = node("ShaderNodeMapping", Vector=attr.outputs["Vector"])
    stretched.inputs["Scale"].default_value = (1.0, 1.0, 0.45)
    cracks = node("ShaderNodeTexVoronoi", Vector=stretched.outputs["Vector"], Scale=380.0,
                  feature="DISTANCE_TO_EDGE").outputs["Distance"]
    # Alligator checking only in patches where the char was deepest (not scraped off).
    patches = node("ShaderNodeTexNoise", Vector=attr.outputs["Vector"], Scale=28.0, Detail=3.0).outputs["Fac"]
    crack = node("ShaderNodeMath", operation="MULTIPLY").outputs[0]
    links.new(maprange(cracks, 0.0, 0.06, 1.0, 0.0), crack.node.inputs[0])
    links.new(maprange(patches, 0.47, 0.6), crack.node.inputs[1])
    sheen = node("ShaderNodeTexNoise", Vector=attr.outputs["Vector"], Scale=300.0, Detail=3.0).outputs["Fac"]

    base_in = _link_value(mat.node_tree, bsdf.inputs["Base Color"])
    # Scraped fire-hardened wood is a very dark brown, blacker where char remains.
    char_col = mix((0.014, 0.0095, 0.007), (0.042, 0.027, 0.016), maprange(sheen, 0.35, 0.7))
    char_col = mix(char_col, (0.005, 0.004, 0.0035), crack)
    char_col = mix(char_col, (0.055, 0.042, 0.03), abrade)
    col = mix(base_in, (0.06, 0.033, 0.016), scorch)
    col = mix(col, char_col, char)
    links.new(col, bsdf.inputs["Base Color"])

    rough_in = _link_value(mat.node_tree, bsdf.inputs["Roughness"])
    rough_char = fmix(fmix(0.58, 0.8, maprange(sheen, 0.35, 0.7)), 0.95, crack)
    rough = fmix(rough_in if rough_in is not None else bsdf.inputs["Roughness"].default_value, rough_char, char)
    links.new(rough, bsdf.inputs["Roughness"])

    normal_in = _link_value(mat.node_tree, bsdf.inputs["Normal"])
    height = node("ShaderNodeMath", operation="MULTIPLY").outputs[0]
    links.new(maprange(crack, 0.0, 1.0, 0.0, -1.0), height.node.inputs[0])
    links.new(char, height.node.inputs[1])
    bump = node("ShaderNodeBump", Strength=0.7, Distance=0.0008, Height=height)
    if normal_in is not None:
        links.new(normal_in, bump.inputs["Normal"])
    links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])
    return mat


def build(kit):
    m = kit.mats
    wood = m.wood("M_DiggingStickWood", light=(0.215, 0.138, 0.078), dark=(0.10, 0.06, 0.031), grain=0.8,
                  roughness=0.64, weathering=0.1, grime=0.5, seed=11.0,
                  polish=0.9, polish_center=Z_TOP, polish_length=0.09, relief=0.9)
    char_overlay(wood)
    leather = m.leather("M_DiggingStickWrap", color=(0.17, 0.105, 0.055), dark=(0.06, 0.038, 0.02),
                        roughness=0.6, creases=0.8, handled=(0.2, 0.3), seed=5.0)
    cord = m.rawhide("M_DiggingStickThong", color=(0.16, 0.10, 0.055), strands=2, twist=120.0)
    parts = [build_stick(kit, wood), build_wrap(kit, leather), build_thong(kit, cord)]
    return kit.join(parts, "SM_DiggingStick", pivot=None, unwrap=False, reshade=True, smooth_angle=45)
