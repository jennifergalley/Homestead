"""Hafted flint knife: a bifacially knapped, backed honey-brown flint blade set into
a split-wood handle with birch-tar pitch and a sinew wrap.

Real-object research (written before modeling):
- Hafted stone knives (e.g. the Ötzi dagger: 13 cm total, 6.4 cm flint blade in an ash
  handle; Great Basin and Californian hafted bifaces: 8-15 cm blades in split willow,
  ash or elderberry-wood handles) are made by splitting the end of a round stick,
  seating the blade's tang in the split with pine/birch-tar pitch, and binding the
  split shut with wet sinew that shrinks tight as it dries.
- Blades are pressure-flaked bifaces 2-3 cm wide and 6-10 mm thick: broad shallow
  flake scars on both faces, a finer row of retouch scars along the edges that leaves a
  slightly scalloped, sawtoothed working edge. A backed knife blunts one edge (the back)
  with steep retouch so a finger can rest on it; the other edge is the cutting edge.
- Honey-brown to grey flint/chert, translucent at thin edges; pitch is glossy black-brown
  and lumpy where it has been smeared warm; sinew is pale, translucent amber-cream.
- Size here: blade 11 cm exposed, handle 10 cm, 21 cm overall.

Rigid static mesh; units are meters, Z up.
PIVOT: the origin is the middle of the handle (grip centre). The handle runs along
+-Z (butt at z -0.05); the blade points +Z (tip at z +0.16). The cutting edge faces -Y,
the blunt backed edge +Y, and the flat faces +-X.
Everything is generated here: no scanned or downloaded geometry or textures.
"""
import importlib.util
import math
from pathlib import Path

from mathutils import Vector, noise

_spec = importlib.util.spec_from_file_location("homestead_flint_hatchet", Path(__file__).with_name("flint_hatchet.py"))
hatchet = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(hatchet)

NAME = "FlintKnife"
DESCRIPTION = ("Hafted knapped-flint knife: backed biface in a split-wood handle with pitch and "
               "sinew wrap (original). Pivot = mid-handle; blade +Z, edge -Y.")
COLLISION = "none"
TRIANGLE_BUDGET = 10000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 96}
# Review lying flat on the ground; close-up on the pitch collar and blade base.
BEAUTY = {"pose": (0, 90, 20), "focus": (0.0, -0.004, 0.07)}

Z_BUTT = -0.050
Z_TOP = 0.050            # top of the handle (split end)
Z_TANG = 0.028           # blade base, hidden inside the split
Z_TIP = 0.160
HANDLE_R = 0.0108
CORD = 0.00085


def smoothstep(a, b, x):
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3 - 2 * t)


# ---------------------------------------------------------------------- blade

def blade_u(z):
    return min(max((z - Z_TANG) / (Z_TIP - Z_TANG), 0.0), 1.0)


def retouch(z, side):
    """Small pressure-flake scallops along an edge (meters)."""
    phase = z / 0.0042 + 0.9 * noise.noise(Vector((z * 70, side, 0.0)))
    depth = 0.0009 * (0.7 + 0.5 * noise.noise(Vector((z * 240, side, 1.0))))
    return depth * (1 - abs(math.cos(math.pi * phase))) ** 1.5


def back_y(z):
    u = blade_u(z)
    return 0.0092 - 0.0105 * u ** 2.4 - retouch(z, 1.0) * 0.5 * smoothstep(0.05, 0.12, u)


def blade_width(z):
    u = blade_u(z)
    body = 0.0205 + 0.0055 * math.sin(math.pi * min(u / 0.62, 1.0) * 0.5) ** 1.2
    tip = max(0.0, 1 - (max(0.0, u - 0.45) / 0.55) ** 2.2) ** 0.75
    tang = 0.78 + 0.22 * smoothstep(0.04, 0.2, u)          # narrower where it sits in the split
    return max(body * tip * tang - retouch(z, -1.0) * smoothstep(0.12, 0.2, u), 0.0)


def thickness(z):
    u = blade_u(z)
    return max(0.0082 * (0.8 + 0.2 * math.sin(math.pi * min(u / 0.5, 1.0) * 0.5)) *
               (1 - u ** 3.0) ** 0.8, 0.0006)


FACE_S = (0.0, 0.05, 0.14, 0.26, 0.38, 0.5, 0.61, 0.71, 0.8, 0.87, 0.93, 0.97)


def face_profile(s):
    """Half-thickness fraction across the blade, s 0 (blunt back) .. 1 (edge):
    steep-retouched back, thickest a third across, long convex taper to the edge."""
    if s < 0.3:
        return 0.62 + 0.38 * math.sin(0.5 * math.pi * s / 0.3)
    return ((1 - s) / 0.7) ** 0.75


def blade_rows():
    zs = [Z_TANG + (0.056 - Z_TANG) * i / 5 for i in range(5)]
    zs += [0.056 + (0.140 - 0.056) * i / 38 for i in range(39)]
    zs += [0.140 + (Z_TIP - 0.0006 - 0.140) * math.sin(0.5 * math.pi * i / 10) for i in range(1, 11)]
    return zs


def section(z):
    """Blade cross-section at height z: +X face from the back to the edge, then -X face."""
    yb, w, t = back_y(z), blade_width(z), thickness(z)
    faces = [(0.5 * t * face_profile(s), yb - w * s) for s in FACE_S]
    return faces + [(0.0, yb - w)] + [(-x, y) for x, y in reversed(faces)]


def build_blade(kit, stone):
    rows, coords = [], []
    for z in blade_rows():
        ring, pco = [], []
        for x, y in section(z):
            ring.append((x, y, z))
            pco.append((x * 10.0, y, z))
        rows.append(ring)
        coords.append(pco)
    blade = kit.loft("Blade", rows, material=stone, coords=coords, cap_start=True,
                     cap_end=(0.0, back_y(Z_TIP), Z_TIP))

    def scars(co, pco):
        def pattern(side, scale, seed):
            p = Vector((co.y * scale, co.z * scale * 0.8, side * 7.3 + seed))
            d, _ = noise.voronoi(p, distance_metric="DISTANCE", exponent=2.5)
            return -(min((d[1] - d[0]) / 0.45, 1.0) ** 0.55)

        blend = smoothstep(-0.0015, 0.0015, co.x)
        big = pattern(1, 150, 0) * blend + pattern(-1, 150, 0) * (1 - blend)
        fine = pattern(1, 420, 3) * blend + pattern(-1, 420, 3) * (1 - blend)
        yb, w = back_y(co.z), max(blade_width(co.z), 1e-5)
        s = min(max((yb - co.y) / w, 0.0), 1.0)
        near_edge = smoothstep(0.62, 0.9, s) + smoothstep(0.2, 0.05, s)
        exposed = smoothstep(0.050, 0.058, co.z)
        depth = 0.0014 * big * (1 - 0.6 * near_edge) + 0.0007 * fine * near_edge
        return max(depth * exposed, -0.3 * abs(co.x))

    return kit.displace(blade, scars)


# --------------------------------------------------------------------- handle

def handle_axis(z):
    t = (z - Z_BUTT) / (Z_TOP - Z_BUTT)
    return Vector((0.0009 * math.sin(math.pi * t), -0.0015 + 0.0012 * math.sin(2.2 * t), z))


def handle_radius(t):
    """t 0 butt .. 1 split top: rounded butt knob, slight waist, swelling to the split."""
    z = Z_BUTT + (Z_TOP - Z_BUTT) * t
    r = HANDLE_R * (1.0 + 0.07 * math.exp(-((t - 0.07) / 0.07) ** 2) - 0.06 * math.exp(-((t - 0.35) / 0.2) ** 2))
    if t < 0.04:
        r *= math.sqrt(max(0.0, 1 - ((0.04 - t) / 0.04) ** 2)) * 0.7 + 0.3
    if z > Z_TOP - 0.004:
        r *= 1 - 0.18 * ((z - (Z_TOP - 0.004)) / 0.004) ** 2
    return r


def build_handle(kit, wood):
    zs = [Z_BUTT + (Z_TOP - Z_BUTT) * t for t in
          [0.0, 0.006, 0.016, 0.03, 0.05] + [0.05 + 0.95 * i / 30 for i in range(1, 31)]]
    handle = kit.tube("Handle", [handle_axis(z) for z in zs], radius=handle_radius, sides=22, material=wood)

    def shape(co, pco):
        z = co.z
        angle = math.atan2(pco.y, pco.x)
        # Oval section: slightly flattened across the blade faces (X).
        oval = -0.0011 * math.cos(angle) ** 2
        # The split: a dark slot running down both edges (+-Y) from the top.
        near = min(abs(math.cos(angle)), 1.0)
        split = -0.0011 * math.exp(-(near / 0.09) ** 2) * smoothstep(0.004, 0.011, z) * smoothstep(0.018, 0.0145, z)
        bark_scar = 0.00025 * smoothstep(-0.047, -0.04, z) * noise.noise(Vector((pco.x * 300, pco.y * 300, z * 60)))
        facets = -0.0003 * abs(math.cos(4.5 * angle + 2.0 * noise.noise(Vector((0, 0, z * 20)))))
        return oval + split + bark_scar + facets

    return kit.displace(handle, shape)


def build_pitch(kit, pitch):
    """Birch-tar collar smeared over the split where the blade leaves the handle: a round
    sleeve over the handle top that shrinks onto the blade's own cross-section."""
    rows, coords = [], []
    zs = [Z_TOP - 0.009, Z_TOP - 0.006, Z_TOP - 0.003, Z_TOP, Z_TOP + 0.002, Z_TOP + 0.0038, Z_TOP + 0.0052,
          Z_TOP + 0.0062]
    for i, z in enumerate(zs):
        k = i / (len(zs) - 1)
        c = handle_axis(min(z, Z_TOP))
        sec = section(z)
        cy = back_y(z) - 0.5 * blade_width(z)
        r0 = handle_radius(min((z - Z_BUTT) / (Z_TOP - Z_BUTT), 1.0)) + 0.0009
        f = smoothstep(0.25, 0.95, k)
        pad = 0.0011 * (1 - f) + 0.00025
        ring, pco = [], []
        for j, (bx, by) in enumerate(sec):
            a = math.atan2(bx, by - cy)
            lump = 1 + 0.1 * noise.noise(Vector((math.sin(a) * 2, math.cos(a) * 2, z * 400))) * (1 - 0.6 * f)
            round_pt = Vector((c.x + math.sin(a) * r0 * lump, c.y + math.cos(a) * r0 * lump))
            d = Vector((bx, by - cy))
            hug = Vector((bx, by)) + (d.normalized() if d.length > 1e-6 else Vector((0.0, 0.0))) * pad * lump
            q = round_pt * (1 - f) + hug * f
            ring.append((q.x, q.y, z))
            pco.append((q.x, q.y, z))
        rows.append(ring)
        coords.append(pco)
    return kit.loft("Pitch", rows, material=pitch, coords=coords, cap_start=True, cap_end=True)


def build_wrap(kit, sinew, z0=0.017, z1=0.041, turns=10.5):
    points, radii = [], []
    steps = int(turns * 14)
    for i in range(steps + 1):
        t = i / steps
        z = z0 + (z1 - z0) * t
        angle = 2 * math.pi * turns * t
        r = (handle_radius((z - Z_BUTT) / (Z_TOP - Z_BUTT)) - 0.0009 - 0.0011 * math.cos(angle) ** 2
             - 0.0003 * abs(math.cos(4.5 * angle))              + CORD * (0.85 + 0.2 * noise.noise(Vector((t * 11, 1, 0)))))
        c = handle_axis(z)
        points.append(c + Vector((math.cos(angle) * r, math.sin(angle) * r, 0.0006 * noise.noise(Vector((t * 19, 2, 0))))))
        radii.append(CORD * (0.85 + 0.25 * noise.noise(Vector((t * 27, 3, 0)))))
    return kit.tube("Wrap", points, radii=radii, sides=6, material=sinew)


def pitch_material(kit, name):
    g = kit.mats.Graph(name)
    p = g.coord()
    lumps = g.noise(p, scale=260.0, detail=4.0, roughness=0.6).outputs["Fac"]
    smear = g.noise(g.vmath("MULTIPLY", p, (1.0, 1.0, 3.0)), scale=90.0, detail=3.0).outputs["Fac"]
    grit = g.voronoi(p, scale=1400.0, feature="F1").outputs["Distance"]
    base = g.mix((0.018, 0.011, 0.006), (0.055, 0.032, 0.014), g.remap(smear, 0.4, 0.65))
    base = g.mix(base, (0.07, 0.065, 0.06), g.remap(grit, 0.08, 0.03, 0.0, 0.5))
    g.set("Base Color", base)
    g.set("Roughness", g.remap(lumps, 0.35, 0.65, 0.22, 0.48))
    g.set("Normal", g.bump(g.math("ADD", lumps, g.math("MULTIPLY", smear, 0.4)), strength=0.5, distance=0.0008))
    return g.mat


def build(kit):
    m = kit.mats
    stone = m.flint("M_KnifeBlade", body=(0.042, 0.029, 0.018), light=(0.125, 0.088, 0.052), cortex_amount=0.0)
    hatchet.add_scar_bump(stone, scale=38.0, strength=0.32, distance=0.0015, along="y")
    wood = m.wood("M_KnifeHandle", light=(0.24, 0.165, 0.095), dark=(0.11, 0.07, 0.038), grain=0.6,
                  roughness=0.6, weathering=0.05, grime=0.5, seed=7.0,
                  polish=0.8, polish_center=0.045, polish_length=0.035, relief=0.8)
    sinew = m.rawhide("M_KnifeSinew", color=(0.14, 0.095, 0.05), strands=2, twist=160.0)
    pitch = pitch_material(kit, "M_KnifePitch")
    parts = [build_blade(kit, stone), build_handle(kit, wood), build_pitch(kit, pitch), build_wrap(kit, sinew)]
    return kit.join(parts, "SM_FlintKnife", pivot=None, unwrap=False, reshade=True, smooth_angle=50)
