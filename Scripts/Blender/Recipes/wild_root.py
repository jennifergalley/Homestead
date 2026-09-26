"""Freshly pulled wild taproot (a yampah / wild-carrot-like root) with damp soil
clinging to it, fine rootlets, a small side fork and a tuft of cut green leaf stalks.

Real-object research (written before modeling):
- Sierra Nevada foragers dug yampah (Perideridia spp., "wild carrot"), whose roots are
  finger-like, 3-15 cm, thin-skinned, tan to cream with white flesh. Naturalised wild
  carrot (Daucus carota) has a pale cream, tapering taproot 1-2 cm across at the shoulder
  and ~10-20 cm long, marked with fine transverse growth rings and lenticel scars.
- Pulled by hand, a root keeps a crown of short, grooved leaf-stalk bases (a purple-green
  flush low down, pale where cut), a rounded shoulder, a tapering body that often forks
  or kinks where it met a stone, and hair-fine lateral rootlets that are mostly torn off.
- Soil clings in the creases and in a few damp clods: dark and damp where thick, paler
  and grey-brown where it has started to dry at the edges.
- Size here: 12 cm crown-to-tip, 1.9 cm across the shoulder; the cut stalks add 3.5 cm.

Rigid static mesh; units are meters, Z up.
PIVOT: the origin is the centre of the crown, on the root's axis where the stalks leave
it (the grip point under the stems). The root hangs straight down (-Z, tip at about
z -0.12); the cut stalks point up (+Z, to about z +0.035).
Everything is generated here: no scanned or downloaded geometry or textures.
"""
import math
import random

from mathutils import Matrix, Vector, noise

NAME = "WildRoot"
DESCRIPTION = ("Freshly pulled wild taproot with clinging soil, fine rootlets and cut green stalks "
               "(original). Pivot = crown centre; root hangs -Z, stalks point +Z.")
COLLISION = "none"
TRIANGLE_BUDGET = 3000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 96}
# Lay it down for review; close-up on the crown and shoulder.
BEAUTY = {"pose": (90, 0, 30), "focus": (0.0, 0.0, -0.022)}

SEED = 4242
LENGTH = 0.120
R_MAX = 0.0095
SIDES = 14


def axis_point(t):
    """Centreline of the taproot, t 0 (crown) .. 1 (tip): a gentle wandering S."""
    return Vector((0.0055 * math.sin(2.4 * t + 0.2) - 0.0011 + 0.004 * t ** 3,
                   0.0038 * math.sin(3.6 * t) - 0.001 * t,
                   -LENGTH * t))


def body_radius(t):
    """Rounded shoulder, fullest ~1.5 cm down, long taper to a thread-fine tip."""
    shoulder = math.sin(0.5 * math.pi * min(t / 0.075, 1.0)) ** 0.45
    taper = (1.0 - t) ** 1.25
    knuckle = 1.0 + 0.06 * math.sin(31.0 * t + 1.1) * (1 - t) + 0.08 * math.exp(-((t - 0.36) / 0.035) ** 2)
    return max(R_MAX * (0.42 + 0.58 * shoulder) * taper * knuckle, 0.00035)


def tangent(t):
    return (axis_point(min(t + 0.01, 1.0)) - axis_point(max(t - 0.01, 0.0))).normalized()


def surface(t, angle, inset=1.0):
    """A point on the (undisplaced) root surface and its outward direction."""
    tan = tangent(t)
    helper = Vector((1, 0, 0)) if abs(tan.x) < 0.8 else Vector((0, 1, 0))
    e1 = tan.cross(helper).normalized()
    e2 = tan.cross(e1).normalized()
    out = math.cos(angle) * e1 + math.sin(angle) * e2
    return axis_point(t) + out * body_radius(t) * inset, out


def build_taproot(kit, material):
    ts = [0.0, 0.006, 0.018, 0.035] + [0.05 + 0.95 * (i / 24) ** 1.08 for i in range(25)]
    points = [axis_point(t) for t in ts]
    radii = [body_radius(t) for t in ts]
    points[0] = points[0] + Vector((0, 0, -0.0002))
    radii[0] = 0.0042
    obj = kit.tube("Taproot", points, radii=radii, sides=SIDES, material=material)

    def lumps(co, pco):
        r = math.hypot(pco.x, pco.y)
        big = noise.noise(Vector((pco.x * 160, pco.y * 160, pco.z * 55)) + Vector((3.1, 0, 0)))
        small = noise.noise(Vector((pco.x * 520, pco.y * 520, pco.z * 190)))
        return r * (0.10 * big + 0.035 * small)

    return kit.displace(obj, lumps)


def build_fork(kit, material):
    t0 = 0.56
    base, out = surface(t0, 2.3, inset=0.2)
    down = tangent(t0)
    direction = (down * 0.85 + out * 0.5).normalized()
    pts = []
    for i in range(10):
        s = i / 9
        wander = Vector((0.0012 * math.sin(5 * s), 0.001 * math.cos(4 * s), 0))
        pts.append(base + direction * (0.036 * s) + down * (0.008 * s * s) + wander * s)
    obj = kit.tube("Fork", pts, radius=lambda s: 0.0024 * (1 - s) ** 1.3 + 0.0003, sides=8, material=material)
    return kit.displace(obj, lambda co, pco: math.hypot(pco.x, pco.y) * 0.1 * noise.noise(pco * 400))


def build_rootlets(kit, material, count=18):
    rng = random.Random(SEED)
    parts = []
    for k in range(count):
        t = 0.12 + 0.8 * (k + rng.random()) / count
        base, out = surface(t, rng.uniform(0, 2 * math.pi), inset=0.75)
        down = tangent(t)
        length = rng.uniform(0.008, 0.016) + 0.02 * t * rng.random()
        spread = rng.uniform(0.35, 0.9)
        pts = []
        for i in range(6):
            s = i / 5
            kink = Vector((noise.noise(Vector((k, s * 3, 0.5))), noise.noise(Vector((k, s * 3, 5.5))),
                           noise.noise(Vector((k, s * 3, 9.5))))) * (0.0038 * s)
            pts.append(base + out * (length * spread * s) + down * (length * (1 - spread * 0.5) * s * s) + kink)
        r0 = rng.uniform(0.00032, 0.00048) * (1.1 - 0.4 * t)
        parts.append(kit.tube(f"Rootlet{k}", pts, radius=lambda s, r0=r0: r0 * (1 - 0.8 * s), sides=4,
                              material=material))
    return parts


def build_stalks(kit, stalk, cut, count=5):
    rng = random.Random(SEED + 1)
    parts = []
    for k in range(count):
        a = 2 * math.pi * (k + rng.uniform(-0.2, 0.2)) / count
        radial = Vector((math.cos(a), math.sin(a), 0))
        base = radial * rng.uniform(0.0008, 0.0024) + Vector((0, 0, -0.0035))
        length = rng.uniform(0.024, 0.036) if k else 0.038
        lean = rng.uniform(0.12, 0.35)
        pts = [base + radial * (lean * length * s * s) + Vector((0, 0, length * s)) for s in (0, 0.2, 0.45, 0.7, 0.88, 1.0)]
        r0 = rng.uniform(0.00115, 0.0015)
        sides = 8
        obj = kit.tube(f"Stalk{k}", pts, radius=lambda s, r0=r0: r0 * (1.0 - 0.15 * s + 0.45 * (1 - s) ** 6),
                       sides=sides, roll=a)
        obj.data.materials.append(stalk)
        obj.data.materials.append(cut)
        body = (len(pts) - 1) * sides
        for index, poly in enumerate(obj.data.polygons):
            poly.material_index = 1 if index >= body + sides else 0    # end cap = cut face
        # Grooved stalks: shallow flutes along their length.
        kit.displace(obj, lambda co, pco: 0.00022 * math.cos(math.atan2(pco.y, pco.x) * 8))
        parts.append(obj)
    return parts


def build_clods(kit, material, count=7):
    rng = random.Random(SEED + 2)
    parts = []
    for k in range(count):
        t = rng.uniform(0.1, 0.62) if k else 0.03
        centre, out = surface(t, rng.uniform(0, 2 * math.pi), inset=0.82)
        size = rng.uniform(0.0022, 0.0045) * (1.0 - 0.6 * t)
        tan = tangent(t)
        side = out.cross(tan).normalized()
        basis = Matrix((tan * 1.35, side * 1.05, out * 0.55)).transposed()
        obj = kit.sphere(f"Clod{k}", 1.0, segments=8, rings=5)
        offset = Vector((k * 3.3, k * 1.7, 0.4))
        kit.warp(obj, lambda co, b=basis, c=centre, s=size, o=offset:
                 c + b @ (co * s * (1.0 + 0.3 * noise.noise(co * 2.2 + o))))
        kit.tag_coords(obj.data)
        obj.data.materials.append(material)
        parts.append(obj)
    return parts


def build(kit):
    m = kit.mats
    skin = m.root("M_RootSkin", dirt=0.6, seed=1.0)
    thread = m.root("M_RootThread", skin=(0.30, 0.21, 0.12), dark=(0.20, 0.14, 0.08), dirt=0.5, seed=2.0)
    earth = m.soil("M_RootSoil", seed=3.0)
    stalk = m.stem("M_RootStalk", color=(0.085, 0.14, 0.035), dark=(0.11, 0.055, 0.06), roughness=0.55)
    cut = m.stem("M_RootStalkCut", color=(0.36, 0.40, 0.20), dark=(0.24, 0.28, 0.12), roughness=0.6)
    parts = [build_taproot(kit, skin), build_fork(kit, skin)]
    parts += build_rootlets(kit, thread)
    parts += build_stalks(kit, stalk, cut)
    parts += build_clods(kit, earth)
    return kit.join(parts, "SM_WildRoot", pivot=None, unwrap=False, reshade=True, smooth_angle=75)
