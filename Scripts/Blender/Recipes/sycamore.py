"""Sycamore (Acer pseudoplatanus): the big, wind-hardy maple of Cornish farms, lanes and woods.

The real tree (what the geometry and textures follow):
- Naturalised everywhere in Cornwall; it shrugs off salt gales that stunt oak and beech, so it
  shelters farmsteads and fills valley woods. Mature trees stand about 15 m (up to 25) with a
  broad, dense, heavy dome about as wide as tall on a stout bole that forks at 3-5 m into
  several steeply rising limbs; stout, opposite shoots.
- Bark smooth and grey on young wood, on old trunks breaking into irregular flat plates that
  flake away to show pinkish-orange fresh bark beneath; green algae on the damp side.
- Leaves large, 10-16 cm, palmate with five pointed, coarsely toothed lobes (the two basal ones
  small), five main veins from the stalk; dark, dull green above, paler below; on long reddish
  stalks in opposite pairs. Late summer leaves carry black "tar spot" blotches and nibbled edges.

Game notes: trunk-only capsule collision; four LODs (see homestead_tree.py) with two material
slots (0 bark, 1 leaves); wind in the Wind vertex colour (R height, G branch phase, B flutter).
"""
import math
import random

import numpy as np

import homestead_foliage as F
import homestead_tree as T

NAME = "Sycamore"
DESCRIPTION = ("Mature sycamore, about 15 m tall with a broad dense 15 m dome on a stout plated bole; "
               "painted palmate leaf-cluster canopy, four LODs, trunk capsule.")
COLLISION = "capsule"
TRIANGLE_BUDGET = 90000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
SEED = 6307
BEAUTY = {"pose": (0, 0, 0), "focus": (0.0, 0.0, 5.0), "views": ("hero", "detail", "eye"),
          "eye_distance": 16.0, "detail_distance": 6.0, "detail_fstop": 11.0}

CLUSTERS = ("c0", "c1", "c2")
MASS = ("m0",)
LEAF_KEYS = ("mid", "mid", "dark", "dark", "light", "under", "spot", "yellow")

SPEC = dict(
    height=15.0, rx=7.6, ry=7.2, seed=SEED, crown_center=0.6, crown_base=0.22,
    profile=[(0.0, 0.07), (0.2, 0.12), (0.27, 0.6), (0.38, 0.9), (0.54, 1.0), (0.72, 0.93),
             (0.87, 0.66), (1.0, 0.2)],
    lobes=0.1, stem_radius=0.44, step=0.3, min_radius=0.004, bark="bark",
    flare=dict(amount=0.45, height=0.6, lobe_height=0.9, rough=0.025,
               lobes=[(5, 0.12, 0.9), (3, 0.06, 2.3), (8, 0.04, 0.4)]),
    skirt=0.3, bark_size=0.9, capsule_height=3.4,
    levels=[
        dict(count=1, elev=(88, 90), length=(0.62, 0.68), tropism=1.4, outward=0.0, wander=0.1,
             stop=0.97, tip_radius=0.12, step=0.3),
        dict(per_m=1.7, start=0.3, end=0.9, angle=(30, 52), abs_length=(6.0, 8.5), radius=(0.62, 0.78),
             up=0.3, out=0.45, tropism=0.35, outward=0.18, wander=0.4, stop=1.0, gravity=0.012,
             max=10, step=0.25, tip_radius=0.1),
        dict(per_m=1.6, start=0.12, end=0.97, angle=(32, 60), abs_length=(1.6, 3.6), radius=(0.44, 0.6),
             up=0.25, out=0.4, tropism=0.22, outward=0.3, wander=0.7, stop=1.02, max=12, step=0.15,
             tip_radius=0.22),
        dict(per_m=2.6, start=0.15, end=0.98, angle=(35, 65), abs_length=(0.5, 1.0), radius=(0.5, 0.65),
             up=0.2, out=0.3, tropism=0.1, outward=0.3, wander=0.8, stop=1.05, max=10, step=0.1,
             inner=0.5, inner_skip=0.6, tip_radius=0.4),
    ],
    shell=dict(count=560, depth=(0.72, 1.0), max_len=1.5, z_min=0.24, radius=0.013, max_level=2),
    foliage=dict(level=3, zone=1.0, spacing=(0.2, 0.3), size=0.95, inner=0.6, inner_density=0.35,
                 on_parents=0.45, along=0.55, face_out=0.65, face_up=0.5, droop=0.1, min_z=0.22),
    normal_blend=0.62,
    lods=[
        dict(levels=9, sides=[22, 12, 5, 3], trunk_sides=26, step=0.2, grid=(2, 2), fold=0.16,
             droop=0.12, back=0.08),
        dict(levels=2, sides=[14, 8, 4], trunk_sides=14, step=0.5, cell=1.25, card_scale=1.5,
             mass=True, grid=(1, 1), back=0.5, droop=0.0),
        dict(levels=2, sides=[8, 4, 3], trunk_sides=8, step=1.0, min_radius=0.04, cell=2.4,
             card_scale=1.5, mass=True, grid=(1, 1), back=0.5, droop=0.0),
        dict(levels=1, sides=[6, 3], trunk_sides=6, step=2.0, min_radius=0.11, cell=4.0,
             card_scale=1.45, mass=True, grid=(1, 1), back=0.5, droop=0.0, min_count=2),
    ],
)


def _palettes():
    base = dict(yellow=(0.20, 0.17, 0.05), brown=(0.012, 0.010, 0.008), stalk=(0.13, 0.045, 0.03),
                dry=(0.15, 0.09, 0.04))
    return {
        "mid": dict(base, base=(0.036, 0.064, 0.016), tip=(0.040, 0.068, 0.017), vein=(0.080, 0.100, 0.040),
                    margin=(0.033, 0.056, 0.014)),
        "dark": dict(base, base=(0.026, 0.048, 0.013), tip=(0.029, 0.052, 0.014), vein=(0.062, 0.080, 0.032),
                     margin=(0.024, 0.042, 0.011)),
        "light": dict(base, base=(0.064, 0.100, 0.026), tip=(0.072, 0.108, 0.028), vein=(0.105, 0.13, 0.05),
                      margin=(0.058, 0.09, 0.024)),
        "under": dict(base, base=(0.060, 0.080, 0.046), tip=(0.064, 0.084, 0.048), vein=(0.10, 0.10, 0.07),
                      margin=(0.054, 0.072, 0.042)),
        # Tar spot (Rhytisma acerinum): the painter's dark "brown" damage blotches, near-black here.
        "spot": dict(base, base=(0.036, 0.062, 0.016), tip=(0.042, 0.066, 0.017), vein=(0.078, 0.098, 0.04),
                     margin=(0.034, 0.054, 0.014)),
        "yellow": dict(base, base=(0.07, 0.09, 0.024), tip=(0.12, 0.12, 0.03), vein=(0.12, 0.13, 0.05),
                       margin=(0.10, 0.09, 0.03), brown=(0.08, 0.045, 0.02)),
    }


PARAMS = {
    "mid": dict(gloss=0.1, damage=0.06),
    "dark": dict(gloss=0.15),
    "light": dict(gloss=0.05, trans=0.7),
    "under": dict(gloss=-0.3, hair=0.2, trans=0.5),
    "spot": dict(damage=0.7, gloss=0.1),
    "yellow": dict(yellow=0.4, damage=0.5, edge_burn=0.3),
}

ATTACH = 0.24


def sycamore_shape(rng):
    """Five coarsely toothed palmate lobes from the stalk's attachment point."""
    lobe = F.ovate(width=0.36, base=0.0, tip=1.0, widest=0.42, tip_sharp=1.25, base_round=0.8,
                   teeth=rng.choice((5, 6)), tooth_depth=0.1, double=0.35, phase=rng.uniform(0, 1))
    small = F.ovate(width=0.4, base=0.0, tip=1.0, widest=0.45, tip_sharp=1.2, base_round=0.8,
                    teeth=3, tooth_depth=0.08, phase=rng.uniform(0, 1))
    origin = (0.0, ATTACH)
    spread = rng.uniform(-4, 4)
    arms = [(0.0, 0.68 * rng.uniform(0.96, 1.02), lobe, 1.0),
            (math.radians(47 + spread), 0.6 * rng.uniform(0.94, 1.04), lobe, 0.8),
            (-math.radians(47 - spread), 0.6 * rng.uniform(0.94, 1.04), lobe, 0.8),
            (math.radians(102 + rng.uniform(-6, 6)), 0.34 * rng.uniform(0.9, 1.1), small, 0.55),
            (-math.radians(102 + rng.uniform(-6, 6)), 0.34 * rng.uniform(0.9, 1.1), small, 0.55)]
    parts = [T.ellipse(0.17, 0.15, center=(0.0, ATTACH + 0.08))]
    veins = []
    for theta, length, shape, w in arms:
        parts.append(T.placed(shape, origin, theta, length))
        tip = (math.sin(theta) * length * 0.92, ATTACH + math.cos(theta) * length * 0.92)
        veins.append(([(origin[0] + (tip[0] - origin[0]) * t, origin[1] + (tip[1] - origin[1]) * t)
                       for t in np.linspace(0, 1, 8)], w))
        # A few side veins off each main vein, toward the lobe teeth.
        for k in range(3):
            t = 0.3 + 0.2 * k
            p = (origin[0] + (tip[0] - origin[0]) * t, origin[1] + (tip[1] - origin[1]) * t)
            for side in (-1, 1):
                a = theta + side * math.radians(45)
                ln = length * 0.18 * (1 - 0.2 * k)
                veins.append(([p, (p[0] + math.sin(a) * ln, p[1] + math.cos(a) * ln)], w * 0.35))
    return T.union(*parts), veins


def make_blade(nrng, rng):
    pals = _palettes()
    shapes = [sycamore_shape(random.Random(SEED + 7 * i)) for i in range(8)]

    def blade(X, Y, px, key):
        key = key or "mid"
        shape, veins = shapes[rng.randrange(len(shapes))]
        return F.paint_blade(X, Y, nrng, shape, veins, pals[key], px, vein_width=0.008,
                             vein_depth=0.0004, puff=0.0003, tertiary=0.5, stalk=0.02, **PARAMS[key])
    return blade


def paint_leaves():
    atlas = F.Atlas(NAME + "Leaves", size=2048, seed=SEED, folder=F.props_folder(NAME) / "Textures")
    for key in CLUSTERS + MASS:
        atlas.tile(key, 1024, 1024)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED)
    rng = random.Random(SEED)
    blade = make_blade(nrng, rng)
    for i, key in enumerate(CLUSTERS):
        lrng = random.Random(SEED + 31 * i)
        # Stout shoots with a few big leaves in opposite pairs on long stalks.
        layout = T.cluster_layout(lrng, pattern="opposite", side_twigs=3, side_len=(0.3, 0.45),
                                  side_angle=(35, 55), leaf_len=0.3, per_twig=(2, 3), leaf_angle=(35, 65),
                                  squash=(0.7, 1.0), twig_width=0.012, curve=0.1, keys=LEAF_KEYS)
        T.paint_cluster(atlas, key, nrng, lrng, blade, layout, twig_color=(0.08, 0.06, 0.045), meters=0.9)
    T.paint_mass(atlas, "m0", nrng, random.Random(SEED + 5), blade, count=620, leaf_len=0.06,
                 keys=LEAF_KEYS, core=(0.011, 0.02, 0.007), holes=0.45, meters=4.0)
    atlas.save()
    return atlas


def paint_bark():
    atlas = F.Atlas(NAME + "Bark", size=2048, seed=SEED + 1, folder=F.props_folder(NAME) / "Textures")
    atlas.column("bark", 2048)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED + 1)
    T.paint_bark(atlas, "bark", nrng, size_m=SPEC["bark_size"],     style="plated", plates=55,
                     base=(0.082, 0.078, 0.068), plate_color=(0.15, 0.10, 0.065), fissure=(0.058, 0.052, 0.044),
                     depth=0.0015, crack=(0.002, 0.008), fresh=0.18, broken=0.85, algae=0.5, lichen=0.2,
                 lichen_color=(0.19, 0.20, 0.16), moss=0.0, rough=0.78)
    atlas.save()
    return atlas


def build(kit):
    bark = paint_bark()
    leaves = paint_leaves()
    objs, desc, cap = T.build(kit, SPEC, bark, leaves, CLUSTERS, MASS, "SM_" + NAME)
    REPORT["capsule"] = T.capsule_report(cap)
    return objs


REPORT = {
    "blocking": True,
    "unreal_folder": "Trees",
    "nanite": True,
    "tree": {"height_m": SPEC["height"], "crown_m": [SPEC["rx"] * 2, SPEC["ry"] * 2],
             "ground_line_cm": SPEC["skirt"] * 100,
             "note": "Pivot is the bottom of a 30 cm skirt under the trunk flare; the ground line is 30 cm up."},
    "wind": {"attribute": "Wind (vertex colour)", "R": "height above ground / tree height",
             "G": "per-branch random phase", "B": "leaf flutter 0 at card base -> 1 at tip, 0 on wood", "A": "1",
             "strength": 13.0, "flutter": 1.0},
    "material_notes": ("Slot 0 M_SycamoreBark: tiling plated bark T_SycamoreBark_* (0.9 m square). Slot 1 "
                       "M_SycamoreLeaves: palmate leaf-cluster atlas T_SycamoreLeaves_* (basecolor alpha = mask, "
                       "clip 0.5; roughness R rough, G translucency, B AO; normal OpenGL). Two-sided foliage, masked."),
}
