"""Hafted flint axe: knapped flint head socketed through a seasoned wooden haft,
bound with twisted rawhide above and below the head and crossed over both faces.

Everything is generated here: no scanned or downloaded geometry or textures.
Units are meters; the haft stands along +Z with the grip end at the origin and
the bit pointing +X.
"""
import math

from mathutils import Vector, noise

NAME = "FlintAxe"
DESCRIPTION = "Primitive hand tool: hafted knapped-flint axe with rawhide lashing (original)."
COLLISION = "convex"
TRIANGLE_BUDGET = 120000
BAKE = {"size": 2048, "samples": 96}
# Review lying on the ground, broad face up, angled across the frame; close-up on the socket.
BEAUTY = {"pose": (90, 0, 28), "focus": (0.02, 0.0, 0.50)}

HAFT_LENGTH = 0.60
HEAD_Z = 0.50            # head centerline height on the haft
HEAD_LENGTH = 0.185      # butt to bit
HEAD_BUTT = -0.055       # butt end x (haft axis at x=0)
CORD = 0.0024            # rawhide radius


def haft_radius(t):
    """Radius along the haft (t = 0 grip end .. 1 top)."""
    z = t * HAFT_LENGTH
    grip = 0.0165 + 0.0025 * math.exp(-((z - 0.015) / 0.02) ** 2)       # small pommel flare
    swell = 0.0085 * math.exp(-((z - HEAD_Z) / 0.065) ** 2)             # socket swell
    top = 1.0 if z < HAFT_LENGTH - 0.02 else math.sqrt(max(0.0, 1 - ((z - (HAFT_LENGTH - 0.02)) / 0.02) ** 2)) * 0.6 + 0.4
    return (grip + swell - 0.0015 * t) * top


def haft_axis(z):
    """Gentle natural bow of the stick."""
    t = z / HAFT_LENGTH
    return Vector((0.010 * math.sin(math.pi * t) - 0.004 * t, 0.003 * math.sin(2.1 * math.pi * t), z))


def build_haft(kit, mat):
    points = [haft_axis(HAFT_LENGTH * i / 120) for i in range(121)]
    haft = kit.tube("Haft", points, radius=haft_radius, sides=48, material=mat)
    # Knots and slight irregularity of a hand-shaped stick.
    knots = [(0.18, 1.2, 0.0035), (0.33, 4.1, 0.003), (0.41, 2.4, 0.0025)]
    def relief(co, pco):
        z = pco.z
        angle = math.atan2(pco.y, pco.x)
        bump = noise.noise(Vector((pco.x * 60, pco.y * 60, z * 9))) * 0.0006
        # Long shallow knife facets from shaping the stick (fade out into the socket swell).
        facet_phase = 8 * angle + 1.7 * noise.noise(Vector((0.0, 0.0, z * 4.0)))
        bump -= 0.00045 * abs(math.cos(facet_phase / 2)) * (1 - math.exp(-((z - HEAD_Z) / 0.05) ** 2))
        for kz, ka, height in knots:
            d = ((z - kz) / 0.012) ** 2 + ((math.atan2(math.sin(angle - ka), math.cos(angle - ka))) / 0.35) ** 2
            bump += height * math.exp(-d)
        return bump
    kit.displace(haft, relief)
    return haft


def build_head(kit, mat):
    rings, sides = 90, 64
    width_bit, thick_max = 0.072, 0.032
    points = [(HEAD_BUTT + HEAD_LENGTH * i / (rings - 1), 0.0, HEAD_Z) for i in range(rings)]
    head = kit.tube("Head", points, radius=1.0, sides=sides, material=mat, cap=True)

    def shape(co):
        # Loft from unit circles: s = 0 butt .. 1 bit.
        s = (co.x - HEAD_BUTT) / HEAD_LENGTH
        s = min(max(s, 0.0), 1.0)
        cy, cz = co.y, co.z - HEAD_Z
        width = width_bit * (0.52 + 0.48 * s ** 0.75)
        thick = max(thick_max * (0.55 + 0.45 * math.sin(math.pi * min(1.0, s * 1.35 + 0.1))) * (1 - s ** 6), 0.0012)
        butt_round = math.sqrt(max(0.0, 1 - ((0.08 - s) / 0.08) ** 2)) if s < 0.08 else 1.0
        # Superellipse cross-section: broad flat faces, rounded sides.
        ey = math.copysign(abs(cy) ** 0.8, cy) * thick / 2 * butt_round
        ez = math.copysign(abs(cz) ** 0.6, cz) * width / 2 * butt_round
        # Arc of the cutting edge: the middle of the bit reaches furthest.
        x = co.x + 0.012 * (1 - (abs(cz) ** 2)) * s ** 5
        return (x, ey, HEAD_Z + ez)

    kit.warp(head, shape)
    kit.tag_coords(head.data)
    kit.subdivide(head, levels=1)

    def flake_scars(co, pco):
        s = (pco.x - HEAD_BUTT) / HEAD_LENGTH

        def scar(side):
            point = Vector((pco.x * 55, pco.z * 55, side * 7.3))
            distances, _ = noise.voronoi(point, distance_metric="DISTANCE", exponent=2.5)
            # Each scar is a shallow conchoidal dish; neighbouring scars meet in
            # crisp ridges (F2 - F1 -> 0 on the cell border).
            edge = min((distances[1] - distances[0]) / 0.45, 1.0)
            return -(edge ** 0.55)

        # Separate scar patterns per face, blended smoothly across the rim so the
        # displacement stays continuous (a hard switch tears spikes into thin edges).
        blend = min(max((co.y + 0.004) / 0.008, 0.0), 1.0)
        blend = blend * blend * (3 - 2 * blend)
        value = scar(1.0) * blend + scar(-1.0) * (1 - blend)
        rim = min(abs(co.y) / 0.005, 1.0)
        # The cortex-covered butt was never flaked.
        body = min(max((s - 0.10) / 0.08, 0.0), 1.0)
        # Ground (polished) bevel toward the bit; scars stay on the body.
        depth = 0.0032 * (1 - max(0.0, (s - 0.7) / 0.3)) ** 2 * (0.25 + 0.75 * rim) * body
        # Never cut deeper than a fraction of the local half-thickness, or the two
        # faces cross through each other and tear spikes out of the thin rims.
        depth = min(depth, 0.3 * abs(co.y))
        # Cortex rind is lumpy and pitted rather than a smooth dome.
        lump = noise.noise(Vector((pco.x * 140, pco.y * 140, pco.z * 140))) * 0.0009 * (1 - body)
        return value * depth + lump
    kit.displace(head, flake_scars)
    return head


def wrap_band(kit, name, mat, z0, z1, turns, phase=0.0, seed=0):
    points, radii = [], []
    steps = int(turns * 36)
    for i in range(steps + 1):
        t = i / steps
        z = z0 + (z1 - z0) * t
        angle = phase + 2 * math.pi * turns * t
        # Hand-wrapped: turns sit a little unevenly and the strip varies in thickness.
        wobble = noise.noise(Vector((t * 9.0, seed, 0.0)))
        radius = haft_radius(z / HAFT_LENGTH) + CORD * (0.8 + 0.25 * wobble)
        center = haft_axis(z) + Vector((0, 0, 0.0012 * noise.noise(Vector((t * 23.0, seed, 3.0)))))
        points.append(center + Vector((math.cos(angle) * radius, math.sin(angle) * radius, 0)))
        radii.append(CORD * (0.9 + 0.18 * noise.noise(Vector((t * 31.0, seed, 7.0)))))
    return kit.tube(name, points, radii=radii, sides=10, material=mat)


def cross_cord(kit, name, mat, side, rising, offset):
    """Diagonal binding over one broad face of the socket (side=+1/-1 on Y)."""
    points = []
    for i in range(41):
        t = i / 40
        angle = side * math.pi / 2 + (t - 0.5) * math.radians(130) * (1 if rising else -1)
        z = HEAD_Z - 0.036 + 0.072 * t + offset
        radius = haft_radius(z / HAFT_LENGTH) + CORD * 0.9
        center = haft_axis(z)
        points.append(center + Vector((math.cos(angle) * radius, math.sin(angle) * radius, 0)))
    return kit.tube(name, points, radius=CORD * 0.95, sides=10, material=mat)


def build(kit):
    mats = kit.mats
    wood = mats.wood("M_AxeHaft", light=(0.25, 0.17, 0.10), dark=(0.11, 0.065, 0.035),
                     grain=0.9, roughness=0.6, weathering=0.35, grime=0.45)
    stone = mats.flint("M_AxeHead", cortex=(0.24, 0.2, 0.145), cortex_amount=0.0,
                       cortex_below_x=HEAD_BUTT + 0.022)
    hide = mats.rawhide("M_AxeLashing", color=(0.19, 0.115, 0.055), strands=2, twist=38.0)

    parts = [build_haft(kit, wood), build_head(kit, stone)]
    parts.append(wrap_band(kit, "WrapLow", hide, HEAD_Z - 0.062, HEAD_Z - 0.030, 6.5, seed=1))
    parts.append(wrap_band(kit, "WrapHigh", hide, HEAD_Z + 0.030, HEAD_Z + 0.060, 6.0, phase=1.3, seed=2))
    for side in (1, -1):
        for rising, offset in ((True, 0.0), (False, 0.003)):
            parts.append(cross_cord(kit, f"Cross{side}{rising}", hide, side, rising, offset))
    return kit.join(parts, "SM_FlintAxe", pivot="base", unwrap=False, reshade=True, smooth_angle=70)
