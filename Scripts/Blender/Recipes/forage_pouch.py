"""Forager's waist pouch: a gathered circle pouch of soft smoke-tanned buckskin,
cinched at the neck with a two-ply plant-fibre drawstring threaded through slits and
tied off in front, hanging from a short cord loop that ties onto a cord belt.

Real-object research (written before modeling):
- Primitive "possibles"/forage pouches are commonly cut from one circle of brain- or
  smoke-tanned hide (no side seam), punched with a ring of slits ~4-5 cm in from the
  edge; a thong or cord is threaded through the slits and drawn up, so everything above
  it gathers into a ruffled crown and everything below into radial pleats.
- Brain-tanned, smoked buckskin: soft, suede-like on both faces, golden-tan to smoky
  brown, mottled from the smoke, darker and slicker where handled, dirty at the bottom.
- Drawstring: 3-4 mm two-ply reverse-wrapped plant fibre (nettle, dogbane, basswood) or
  a leather thong; ends finished with overhand stopper knots.
- Size here: ~18 cm tall (bottom to the gathered rim) x ~14 cm wide x ~8 cm deep when
  holding roots and berries; the loop adds ~6 cm above the neck.

Rigid static mesh for a pelvis/thigh socket. Units are meters, Z up.
PIVOT: the origin is the top of the hanging loop (the cord centreline at its apex),
i.e. the attachment point; the pouch hangs straight down (-Z) from it. The broad
front face (with the knot and drawstring tails) faces -Y; the flatter back that lies
against the hip faces +Y; width runs along X.
Everything is generated here: no scanned or downloaded geometry or textures.
"""
import math

from mathutils import Vector, noise
from mathutils.bvhtree import BVHTree

NAME = "ForagePouch"
DESCRIPTION = ("Forager's drawstring waist pouch in smoke-tanned buckskin with plant-fibre cord "
               "(original). Pivot = top of the hanging loop; hangs along -Z, front faces -Y.")
COLLISION = "none"
TRIANGLE_BUDGET = 6000
BAKE = {"size": 2048, "samples": 96}
# Review standing on its bottom as it would sit on the ground; close-up on the neck and knot.
BEAUTY = {"pose": (0, 0, 18), "focus": (0.0, -0.03, -0.075)}

SIDES = 52
PLEATS = 8
Z_NECK = -0.064
CROWN_H = 0.028
Z_BOT = Z_NECK + CROWN_H - 0.180
NECK_R = 0.0135
HALF_W = 0.070
HALF_D = 0.041
CORD_R = 0.0017
LOOP_R = 0.0021


def smoothstep(a, b, x):
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3 - 2 * t)


def pleat(theta, amount, s, soft=False):
    """Radial gathers: broad rounded outer folds and tight inner valleys (how gathered
    hide actually folds), irregular in spacing and depth."""
    jitter = 1.1 * noise.noise(Vector((math.cos(theta) * 1.3, math.sin(theta) * 1.3, 4.0)))
    wave = math.cos(PLEATS * theta + jitter + 0.35 * s)
    shaped = wave ** 0.8 if wave >= 0 else -(abs(wave) ** 1.8)
    if soft:   # the loose crown: plain rounded ruffles, no tight creases
        shaped = wave
    depth = 0.62 + 0.45 * noise.noise(Vector((math.cos(theta) * 2.1, math.sin(theta) * 2.1, 9.0)))
    return 1 + amount * depth * shaped


def body_ring(s):
    """Body row at s (0 bottom .. 1 neck)."""
    z = Z_BOT + (Z_NECK - Z_BOT) * s
    belly = 0.40
    if s <= belly:
        grow, taper = math.sin(0.5 * math.pi * s / belly) ** 0.42, 1.0
    else:
        grow, taper = 1.0, math.cos(0.5 * math.pi * (s - belly) / (1 - belly)) ** 1.25
    ax = (NECK_R + (HALF_W - NECK_R) * taper) * grow
    ay = (NECK_R + (HALF_D - NECK_R) * taper) * grow
    amount = 0.25 * smoothstep(0.45, 0.97, s)
    ring = []
    for j in range(SIDES):
        theta = 2 * math.pi * j / SIDES
        k = pleat(theta, amount, s)
        c, sn = math.cos(theta), math.sin(theta)
        # Fuller at the front (-Y), flatter at the back that lies against the hip.
        depth = ay * (0.925 - 0.175 * sn)
        ring.append((ax * c * k, depth * sn * k, z))
    return ring


def crown_radius(c):
    return NECK_R + (0.026 - NECK_R) * math.sin(0.5 * math.pi * c) ** 0.8


def crown_ring(c, inset=1.0, drop=0.0):
    z = Z_NECK + CROWN_H * c
    rc = crown_radius(c) * inset
    amount = 0.14 + 0.10 * c
    ring = []
    for j in range(SIDES):
        theta = 2 * math.pi * j / SIDES
        k = pleat(theta, amount, 1.0 + 0.3 * c, soft=True)
        wave = 0.006 * c * noise.noise(Vector((math.cos(theta) * 2.5, math.sin(theta) * 2.5, 1.7)))
        # Soft leather: the wider lobes flop outwards and down under their own weight.
        flop = 0.007 * c * c * (k - 1.0 + amount) / (2.0 * amount)
        ring.append((rc * math.cos(theta) * k, rc * math.sin(theta) * k * 0.92, z + wave - drop - flop + 0.003 * c * (k - 1.0) / amount))
    return ring


def build_body(kit, hide):
    ss = [0.012, 0.03, 0.055, 0.085, 0.12, 0.16, 0.2, 0.25, 0.3, 0.35, 0.4]
    ss += [0.4 + 0.6 * i / 12 for i in range(1, 13)]
    rows = [body_ring(s) for s in ss]
    rows += [crown_ring(i / 8) for i in range(1, 9)]
    # The gathered rim rolls over (leather thickness) and funnels into the closed mouth.
    rows.append(crown_ring(1.0, inset=0.9, drop=0.003))
    rows.append(crown_ring(1.0, inset=0.45, drop=0.012))
    body = kit.loft("Body", rows, material=hide, cap_start=(0, 0, Z_BOT - 0.0015),
                    cap_end=(0, 0, Z_NECK + CROWN_H - 0.022))

    def lumps(co, pco):
        s = (co.z - Z_BOT) / (Z_NECK - Z_BOT)
        weight = 1 - smoothstep(0.5, 0.82, s)
        weight *= 0.65 - 0.35 * max(-1.0, min(1.0, co.y / 0.035))
        big = noise.noise(pco * 26.0 + Vector((3.0, 1.0, 0.0)))
        small = noise.noise(pco * 70.0 + Vector((0.0, 7.0, 2.0)))
        return weight * (0.0038 * big + 0.0011 * small)
    kit.displace(body, lumps)
    return body


def surface_probe(body):
    mesh = body.data
    tree = BVHTree.FromPolygons([v.co.copy() for v in mesh.vertices], [p.vertices[:] for p in mesh.polygons])

    def probe(theta, z, lift):
        direction = Vector((-math.cos(theta), -math.sin(theta), 0.0))
        origin = Vector((0.0, 0.0, z)) - direction * 0.3
        hit, normal, _, _ = tree.ray_cast(origin, direction, 0.6)
        if hit is None:
            return Vector((0.0, 0.0, z)) - direction * NECK_R
        if normal.dot(direction) > 0:
            normal = -normal
        return hit + normal * lift
    return probe


def build_drawstring(kit, cord):
    points, radii = [], []
    count = 42
    for i in range(count + 1):
        theta = -0.5 * math.pi + 2 * math.pi * i / count
        # Over-and-under through eight slits: dips below the leather between them.
        m = smoothstep(-0.45, -0.1, math.cos(8 * (theta + 0.5 * math.pi) + math.pi / 8))
        r = NECK_R * 1.12 + CORD_R * (0.8 * m - 1.4 * (1 - m))
        points.append((r * math.cos(theta), r * math.sin(theta) * 0.95, Z_NECK + 0.0006 * math.sin(3 * theta)))
        radii.append(CORD_R)
    return kit.tube("Drawstring", points, radii=radii, sides=7, material=cord)


KNOT = Vector((0.0, -(NECK_R * 1.12 + 0.0052), Z_NECK - 0.002))


def build_knot(kit, cord):
    s = 0.0023
    points = []
    for i in range(45):
        t = 2 * math.pi * i / 44
        x = s * (math.sin(t) + 2 * math.sin(2 * t))
        z = s * (math.cos(t) - 2 * math.cos(2 * t)) * 0.8
        y = s * 0.9 * -math.sin(3 * t)
        points.append(KNOT + Vector((x, y, z)))
    return kit.tube("Knot", points, radius=CORD_R * 0.95, sides=7, material=cord)


def build_tail(kit, name, cord, probe, theta_end, length, seed):
    points, radii = [], []
    steps = 20
    start = KNOT + Vector((0.004 * math.cos(theta_end + 0.5 * math.pi), -0.001, -0.004))
    for i in range(steps + 1):
        t = i / steps
        z = start.z - length * t
        theta = -0.5 * math.pi + (theta_end + 0.5 * math.pi) * smoothstep(0.0, 1.0, t)
        theta += 0.06 * noise.noise(Vector((t * 3.0, seed, 0.0)))
        on_surface = probe(theta, z, CORD_R * 1.05)
        blend = smoothstep(0.0, 0.25, t)
        points.append(start.lerp(on_surface, blend) if i else start)
        radius = CORD_R
        radius *= 1 + 0.75 * math.exp(-((t - 0.86) / 0.045) ** 2)       # overhand stopper knot
        radius *= 0.55 + 0.45 * (1 - smoothstep(0.9, 1.0, t))             # frayed end
        radii.append(radius)
    return kit.tube(name, points, radii=radii, sides=7, material=cord)


def build_loop(kit, cord, probe):
    apex_r = 0.0065
    z_top = -LOOP_R * 0.0
    leg = []
    zs = [Z_NECK + (z_top - apex_r - Z_NECK) * i / 16 for i in range(17)]
    base_x = NECK_R * 1.12 + LOOP_R * 0.6
    for z in zs:
        taut = base_x + (apex_r - base_x) * (z - Z_NECK) / (z_top - apex_r - Z_NECK)
        clear = probe(0.0, z, LOOP_R * 1.2).x if z < Z_NECK + CROWN_H + 0.004 else 0.0
        leg.append([max(taut, clear), z])
    for _ in range(3):   # relax the kink where the taut line meets the crown
        leg = [leg[0]] + [[(a[0] + 2 * b[0] + c[0]) / 4, b[1]] for a, b, c in zip(leg, leg[1:], leg[2:])] + [leg[-1]]
    leg[0][0] = NECK_R * 0.9          # tucked in under the drawstring
    right = [(x + 0.0012 * noise.noise(Vector((z * 60.0, 1.0, 0.0))), 0.001 + 0.0015 * noise.noise(Vector((z * 40.0, 2.0, 0.0))), z)
             for x, z in leg]
    top = []
    for i in range(1, 12):
        a = math.pi * i / 12
        top.append((apex_r * math.cos(a), 0.001, z_top - apex_r + apex_r * math.sin(a)))
    left = [(-x - 0.0012 * noise.noise(Vector((z * 55.0, 3.0, 0.0))), 0.001 + 0.0015 * noise.noise(Vector((z * 45.0, 4.0, 0.0))), z)
            for x, z in reversed(leg)]
    return kit.tube("HangingLoop", right + top + left, radius=LOOP_R, sides=7, material=cord)


def build(kit):
    mats = kit.mats
    hide = mats.leather("M_PouchHide", color=(0.275, 0.195, 0.11), dark=(0.10, 0.072, 0.044),
                        roughness=0.76, soil_below=Z_BOT + 0.05, soil=0.5,
                        handled=(Z_NECK, 0.03), stains=0.35, seed=3.0)
    cord = mats.rawhide("M_PouchCord", color=(0.21, 0.165, 0.10), strands=2, twist=110.0)
    body = build_body(kit, hide)
    probe = surface_probe(body)
    parts = [body, build_drawstring(kit, cord), build_knot(kit, cord), build_loop(kit, cord, probe)]
    parts.append(build_tail(kit, "TailA", cord, probe, -0.5 * math.pi - 0.32, 0.068, 1.0))
    parts.append(build_tail(kit, "TailB", cord, probe, -0.5 * math.pi + 0.22, 0.084, 2.0))
    return kit.join(parts, "SM_ForagePouch", pivot=None, unwrap=False, reshade=True, smooth_angle=180)
