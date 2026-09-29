"""Holly (Ilex aquifolium): the evergreen understorey of Cornish oak and beech woods.

The real plant (what the geometry and textures follow):
- Under the oak canopy it grows as a dense, dark, broadly conical bush or small tree 2-4 m, often
  a main stem and a sucker or two, clothed in leaves almost to the ground; branches spread level
  and turn up at the tips.
- Bark thin, smooth and silver-grey with tiny warty lenticels, greenish with algae on the damp side.
- Leaves evergreen, leathery, 5-10 cm, elliptic with a wavy margin drawn into 3-6 sharp spines a
  side (upper and older leaves often nearly spineless); very glossy near-black green above, paler
  dull yellow-green beneath, spirally set on short green shoots.

Game notes: walk-through (no collision; the wood's paths go round it). Four LODs (see
homestead_tree.py) with two material slots (0 bark, 1 leaves); wind in the Wind vertex colour
(R height, G branch phase, B flutter).
"""
import math
import random

import numpy as np

import homestead_foliage as F
import homestead_tree as T

NAME = "Holly"
DESCRIPTION = ("Woodland holly, a dense glossy evergreen cone about 2.8 m tall and 2.6 m across on two smooth "
               "grey stems; painted spiny leaf-cluster canopy, four LODs, walk-through.")
COLLISION = "none"
TRIANGLE_BUDGET = 70000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
SEED = 8123
BEAUTY = {"pose": (0, 0, 0), "focus": (0.0, 0.0, 1.5), "views": ("hero", "detail", "eye"),
          "eye_distance": 6.0, "detail_distance": 2.2, "detail_fstop": 8.0}

CLUSTERS = ("c0", "c1", "c2")
MASS = ("m0",)
LEAF_KEYS = ("mid", "mid", "mid", "dark", "dark", "dark", "gloss", "gloss", "under", "old")

SPEC = dict(
    height=2.75, rx=1.25, ry=1.15, seed=SEED, crown_center=0.45, crown_base=0.05,
    crowns=[(0.0, 0.0), (0.16, -0.08)], crown_jitter=0.02,
    profile=[(0.0, 0.45), (0.08, 0.85), (0.25, 1.0), (0.45, 0.9), (0.65, 0.68), (0.85, 0.38),
             (1.0, 0.08)],
    lobes=0.14, stem_radius=0.075, step=0.07, min_radius=0.0025, bark="bark",
    flare=dict(amount=0.25, height=0.18, lobe_height=0.3, rough=0.02,
               lobes=[(4, 0.08, 0.5), (7, 0.04, 1.3)]),
    skirt=0.15, bark_size=0.4, capsule_height=1.0,
    levels=[
        dict(count=2, elev=(80, 88), length=(0.8, 0.92), tropism=1.1, outward=0.0, wander=0.25,
             stop=1.0, tip_radius=0.12, step=0.07),
        dict(per_m=6.0, start=0.04, end=0.95, angle=(55, 80), abs_length=(0.5, 1.2), radius=(0.4, 0.6),
             up=0.12, out=0.45, tropism=0.3, outward=0.3, wander=0.6, stop=1.0, max=18, step=0.06,
             tip_radius=0.2),
        dict(per_m=7.0, start=0.12, end=0.97, angle=(40, 70), abs_length=(0.2, 0.5), radius=(0.45, 0.6),
             up=0.2, out=0.35, tropism=0.2, outward=0.3, wander=0.9, stop=1.03, max=10, step=0.05,
             tip_radius=0.3),
        dict(per_m=9.0, start=0.15, end=0.98, angle=(40, 70), abs_length=(0.08, 0.2), radius=(0.5, 0.65),
             up=0.2, out=0.3, tropism=0.1, outward=0.3, wander=1.0, stop=1.06, max=6, step=0.04,
             inner=0.4, inner_skip=0.4, tip_radius=0.45),
    ],
    shell=dict(count=420, depth=(0.72, 1.0), max_len=0.4, z_min=0.04, radius=0.004, max_level=2),
    foliage=dict(level=3, zone=0.35, spacing=(0.06, 0.1), size=0.36, inner=0.45, inner_density=0.7,
                 on_parents=0.7, along=0.45, face_out=0.7, face_up=0.45, droop=0.04, min_z=0.03),
    normal_blend=0.55,
    lods=[
        dict(levels=9, sides=[12, 8, 5, 3], trunk_sides=12, step=0.06, grid=(2, 2), fold=0.14,
             droop=0.06, back=0.08),
        dict(levels=2, sides=[8, 5, 3], trunk_sides=8, step=0.2, cell=0.42, card_scale=1.55,
             mass=True, grid=(1, 1), back=0.5, droop=0.0),
        dict(levels=2, sides=[5, 3, 3], trunk_sides=5, step=0.4, min_radius=0.015, cell=0.8,
             card_scale=1.55, mass=True, grid=(1, 1), back=0.5, droop=0.0),
        dict(levels=1, sides=[4, 3], trunk_sides=4, step=0.8, min_radius=0.03, cell=1.3,
             card_scale=1.5, mass=True, grid=(1, 1), back=0.5, droop=0.0, min_count=2),
    ],
)


def _palettes():
    base = dict(yellow=(0.16, 0.15, 0.04), brown=(0.06, 0.035, 0.015), stalk=(0.04, 0.06, 0.02),
                dry=(0.13, 0.09, 0.04))
    return {
        "mid": dict(base, base=(0.020, 0.044, 0.012), tip=(0.022, 0.046, 0.012), vein=(0.04, 0.065, 0.025),
                    margin=(0.045, 0.06, 0.02)),
        "dark": dict(base, base=(0.013, 0.031, 0.009), tip=(0.015, 0.034, 0.010), vein=(0.03, 0.05, 0.018),
                     margin=(0.035, 0.048, 0.016)),
        "gloss": dict(base, base=(0.016, 0.038, 0.011), tip=(0.018, 0.040, 0.011), vein=(0.035, 0.06, 0.022),
                      margin=(0.04, 0.055, 0.018)),
        "under": dict(base, base=(0.070, 0.090, 0.032), tip=(0.074, 0.094, 0.033), vein=(0.10, 0.11, 0.05),
                      margin=(0.08, 0.09, 0.035)),
        "old": dict(base, base=(0.03, 0.05, 0.014), tip=(0.06, 0.07, 0.02), vein=(0.07, 0.085, 0.035),
                    margin=(0.06, 0.06, 0.022), yellow=(0.12, 0.11, 0.03)),
    }


PARAMS = {
    "mid": dict(gloss=1.3, trans=0.25),
    "dark": dict(gloss=1.5, trans=0.2),
    "gloss": dict(gloss=1.8, trans=0.2),
    "under": dict(gloss=-0.1, trans=0.3),
    "old": dict(gloss=0.9, yellow=0.3, damage=0.35, edge_burn=0.15, trans=0.25),
}


def holly_shape(rng, spines=True):
    """Elliptic leathery blade whose wavy margin is drawn out into sharp forward spines."""
    width = rng.uniform(0.28, 0.32) if spines else rng.uniform(0.24, 0.27)
    body = F.ovate(width=width, base=0.04, tip=0.96, widest=0.48, tip_sharp=0.85, base_round=0.7)
    n = rng.choice((3, 4, 4, 5)) if spines else 0
    depth = rng.uniform(0.38, 0.5)
    phase = rng.uniform(0, 1)
    offset = rng.uniform(0.3, 0.6)

    def shape(X, Y):
        inside, s, hw = body(X, Y)
        if n:
            ph = s * n + phase + np.where(X > 0, offset, 0.0)
            saw = ph - np.floor(ph)
            # Needle points on concave scallops.
            spike = np.power(np.clip(1.0 - np.abs(saw - 0.62) / 0.62, 0, 1), 5.0)
            scallop = np.sin(saw * math.pi) * 0.35
            fade = F.smoothstep(0.1, 0.22, s) * (1 - F.smoothstep(0.88, 0.97, s))
            hw = hw * (1 + depth * (spike * 1.6 - scallop) * fade)
            inside = np.minimum(hw - np.abs(X), np.minimum(Y - 0.04, 0.96 - Y) * 3.0)
        return inside, s, hw

    veins = F.pinnate_veins(count=rng.choice((5, 6)), angle=0.9, curve=0.4, reach=0.8, base=0.04, tip=0.96,
                            width=width, widest=0.48, start=0.12, stop=0.8,
                            rng=np.random.default_rng(rng.randrange(1 << 30)))
    # A pale midrib; the side veins are sunk in the leathery blade and barely show.
    veins = [veins[0]] + [(pts, w * 0.45) for pts, w in veins[1:]]
    return shape, veins


def make_blade(nrng, rng):
    pals = _palettes()
    shapes = [holly_shape(random.Random(SEED + 7 * i), spines=i < 7) for i in range(8)]

    def blade(X, Y, px, key):
        key = key or "mid"
        shape, veins = shapes[rng.randrange(len(shapes))]
        # A pale thickened margin and midrib, faint side veins: a leathery blade.
        return F.paint_blade(X, Y, nrng, shape, veins, pals[key], px, vein_width=0.007,
                             vein_depth=0.0002, puff=0.0006, tertiary=0.05, stalk=0.02, **PARAMS[key])
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
        # Stiff green shoots crowded with spirally set leaves.
        layout = T.cluster_layout(lrng, pattern="spiral", side_twigs=3, side_len=(0.35, 0.55),
                                  side_angle=(30, 55), leaf_len=0.22, per_twig=(8, 11), leaf_angle=(35, 70),
                                  squash=(0.7, 1.0), twig_width=0.012, curve=0.08, keys=LEAF_KEYS)
        T.paint_cluster(atlas, key, nrng, lrng, blade, layout, twig_color=(0.05, 0.07, 0.03), meters=0.36)
    T.paint_mass(atlas, "m0", nrng, random.Random(SEED + 5), blade, count=900, leaf_len=0.06,
                 keys=LEAF_KEYS, core=(0.006, 0.012, 0.004), holes=0.35, meters=1.4)
    atlas.save()
    return atlas


def paint_bark():
    atlas = F.Atlas(NAME + "Bark", size=1024, seed=SEED + 1, folder=F.props_folder(NAME) / "Textures")
    atlas.column("bark", 1024)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED + 1)
    T.paint_bark(atlas, "bark", nrng, size_m=SPEC["bark_size"], style="smooth",
                 base=(0.11, 0.11, 0.10), depth=0.003, lenticels=0.8, algae=0.5,
                 lichen=0.35, lichen_color=(0.21, 0.22, 0.18), moss=0.0, rough=0.66)
    atlas.save()
    return atlas


def build(kit):
    bark = paint_bark()
    leaves = paint_leaves()
    objs, desc, cap = T.build(kit, SPEC, bark, leaves, CLUSTERS, MASS, "SM_" + NAME)
    return objs


REPORT = {
    "blocking": False,
    "unreal_folder": "Trees",
    "nanite": True,
    "tree": {"height_m": SPEC["height"], "crown_m": [SPEC["rx"] * 2, SPEC["ry"] * 2],
             "ground_line_cm": SPEC["skirt"] * 100,
             "note": "Pivot is the bottom of a 15 cm skirt under the stems; the ground line is 15 cm up. "
                     "No collision."},
    "wind": {"attribute": "Wind (vertex colour)", "R": "height above ground / plant height",
             "G": "per-branch random phase", "B": "leaf flutter 0 at card base -> 1 at tip, 0 on wood", "A": "1",
             "strength": 4.0, "flutter": 0.6},
    "material_notes": ("Slot 0 M_HollyBark: tiling smooth bark T_HollyBark_* (0.4 m square). Slot 1 "
                       "M_HollyLeaves: spiny leaf-cluster atlas T_HollyLeaves_* (basecolor alpha = mask, clip 0.5; "
                       "roughness R rough, G translucency, B AO; normal OpenGL). Two-sided foliage, masked."),
}
