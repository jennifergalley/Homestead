"""Round shrub: overlapping lumpy leaf clumps on a few woody stems."""
import math
import random

NAME = "Bush"
DESCRIPTION = "Homestead woodland/yard shrub, about knee-to-waist height."
COLLISION = "convex"
TRIANGLE_BUDGET = 2500


def build(kit):
    rng = random.Random(8812)
    shade = kit.material("M_BushLeafShade", (0.11, 0.20, 0.07), roughness=0.9)
    sun = kit.material("M_BushLeafSun", (0.20, 0.33, 0.10), roughness=0.9)
    bark = kit.material("M_BushStem", (0.17, 0.11, 0.07), roughness=0.95)

    parts = []
    # Woody stems fanning out from the root, mostly hidden inside the foliage.
    for index in range(4):
        yaw = index * 90 + rng.uniform(-25, 25)
        lean = rng.uniform(28, 40)
        length = rng.uniform(0.30, 0.40)
        direction = (math.sin(math.radians(lean)) * math.cos(math.radians(yaw)),
                     math.sin(math.radians(lean)) * math.sin(math.radians(yaw)),
                     math.cos(math.radians(lean)))
        center = tuple(d * length / 2 for d in direction)
        stem = kit.cylinder(f"Stem{index}", 0.022, length, center, material=bark, sides=6,
                            radius_top=0.010, rotation=(0, lean, yaw))
        parts.append(stem)

    # Clumps spread over a dome (uniform in height, golden-angle around) so the
    # silhouette is broken but foliage still reaches close to the ground. Color
    # shifts from shaded underside to a sunlit crown.
    mid = kit.material("M_BushLeafMid", (0.15, 0.26, 0.08), roughness=0.9)
    clumps = [(0.0, 0.0, 0.42, 0.36, mid)]
    count = 14
    for index in range(count):
        t = (index + 0.5) / count
        elevation = math.asin(t)
        azimuth = math.radians(index * 137.5 + rng.uniform(-10, 10))
        reach = 0.42 * math.cos(elevation) + rng.uniform(-0.03, 0.03)
        leaf = shade if t < 0.4 else mid if t < 0.75 else sun
        clumps.append((reach * math.cos(azimuth), reach * math.sin(azimuth),
                       0.26 + 0.46 * math.sin(elevation),
                       rng.uniform(0.21, 0.28) * (1 - 0.25 * t), leaf))

    for index, (x, y, z, radius, leaf) in enumerate(clumps):
        clump = kit.sphere(f"Clump{index}", radius, (x, y, z), material=leaf, segments=10, rings=7,
                           rotation=(0, 0, rng.uniform(0, 360)), scale=(1, 1, 0.82))
        kit.roughen(clump, strength=radius * 0.22, scale=3.2 / radius, seed=index + 1)
        parts.append(clump)
    return kit.join(parts, "SM_Bush", smooth_angle=80)
