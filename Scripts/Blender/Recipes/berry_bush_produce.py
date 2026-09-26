"""Ripe wild blackberries to dress a berry shrub: eight small fruiting clusters (3-5
berries each on short prickly pedicels, a twig stub leading back into the foliage),
laid out as separate mesh elements in one static mesh around a 60-80 cm shrub.

Real-object research: see ``berry_cluster.py`` (same Rubus berries and materials).
On a real bramble the fruit hangs in loose terminal clusters on the outer, sunlit
shell of the canopy, mostly ripe black at peak season with the odd purple-red or
unripe red berry among them. This mesh replaces placeholder spheres on the bushes.

Rigid static mesh; units are meters, Z up.
PIVOT: the origin is the shrub base (ground level, at the shrub's centre). Clusters sit
15-45 cm above it within a ~35 cm radius; each twig stub points back towards the shrub
centre, and the berries hang down and outward. Place it at the bush actor's origin.
Everything is generated here: no scanned or downloaded geometry or textures.
"""
import importlib.util
import math
import random
from pathlib import Path

from mathutils import Vector

NAME = "BerryBushProduce"
DESCRIPTION = ("Eight ripe blackberry clusters laid out to sit on a 60-80 cm shrub (original). "
               "Pivot = shrub base; clusters at 15-45 cm height within 35 cm radius.")
COLLISION = "none"
TRIANGLE_BUDGET = 8000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 64}

_spec = importlib.util.spec_from_file_location("homestead_berry_cluster", Path(__file__).with_name("berry_cluster.py"))
berries = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(berries)

SEED = 9133
CLUSTERS = 8


def layout():
    rng = random.Random(SEED)
    placements = []
    for k in range(CLUSTERS):
        azimuth = math.radians(k * 137.508 + rng.uniform(-18, 18))
        height = 0.15 + 0.30 * ((k * 0.618 + rng.uniform(0, 0.3)) % 1.0)
        shell = 0.35 * math.sqrt(max(0.15, 1.0 - 0.75 * ((height - 0.15) / 0.36) ** 2))
        radius = shell * rng.uniform(0.78, 0.97)
        out = Vector((math.cos(azimuth), math.sin(azimuth), 0.0))
        node = out * radius + Vector((0, 0, height))
        placements.append((node, out, rng.randint(3, 5), rng.uniform(0.85, 1.05), rng.random()))
    return placements


PLACEMENTS = layout()
_first = PLACEMENTS[0][0]
BEAUTY = {"pose": (0, 0, 0), "focus": (_first.x, _first.y, _first.z - 0.02)}


def cluster(kit, name, mats, node, out, count, size, seed):
    rng = random.Random(seed * 1000 + 17)
    parts = []
    inward = node - out * 0.035 + Vector((0, 0, 0.012))
    mid = node - out * 0.016 + Vector((0, 0, 0.007))
    parts.append(kit.tube(name + "Twig", [inward, mid, node], radius=lambda t: 0.0016 - 0.0005 * t, sides=6,
                          material=mats["stem"]))
    side = Vector((0, 0, 1)).cross(out).normalized()
    for b in range(count):
        roll = rng.random()
        material = mats["berry"] if roll < 0.82 else mats["berry_purple"] if roll < 0.94 else mats["berry_red"]
        if b == 0:
            direction, reach, drop = out * 0.5 + side * rng.uniform(-0.2, 0.2), 0.002, 0.005
        else:
            spread = math.radians(-100 + 200 * (b - 1) / (count - 2)) if count > 2 else math.radians(-50 + 100 * (b - 1))
            a = spread + rng.uniform(-0.25, 0.25)
            direction = out * math.cos(a) + side * math.sin(a)
            reach, drop = rng.uniform(0.008, 0.011), rng.uniform(0.003, 0.005)
        z = -0.002 * b
        local = dict(mats, berry=material)
        parts += berries.berry_on_pedicel(kit, f"{name}B{b}", local, node + Vector((0, 0, z)) + out * (0.0015 * b),
                                          direction, reach * size, drop * size,
                                          rng.uniform(0.0150, 0.0180) * size, rng.uniform(0.0120, 0.0140) * size,
                                          seed * 10 + b, lowres=True)
    return parts


def build(kit):
    mats = berries.materials(kit)
    mats["berry_red"] = kit.mats.blackberry("M_BlackberryRed", color=(0.14, 0.010, 0.018),
                                            glint=(0.26, 0.022, 0.032), crevice=(0.05, 0.004, 0.008), seed=2.0)
    parts = []
    for k, (node, out, count, size, _) in enumerate(PLACEMENTS):
        parts += cluster(kit, f"Cluster{k}", mats, node, out, count, size, k + 1)
    return kit.join(parts, "SM_BerryBushProduce", pivot=None, unwrap=False, reshade=True, smooth_angle=100)
