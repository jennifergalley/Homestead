"""Common hawthorn (Crataegus monogyna), wind-pruned: the tree of Cornish hedges and wood edges.

The real tree (what the geometry and textures follow):
- Every Cornish hedge-bank and cliff-top has them: a small tree 4-6 m, on exposed ground bent
  hard away from the prevailing south-westerly. The short, often fluted bole leans downwind; the
  windward side is clipped close by salt gales, so the dense, flat-topped crown streams out to
  leeward like a flag, lower and wider than it is tall.
- Wood dense and crooked, zig-zag twigs, short leafy spur shoots with thorns.
- Bark grey-brown to orange-brown, broken into small scaly plates by fine fissures; near the
  coast thickly crusted with pale grey-green lichens.
- Leaves small, 2-4 cm, wedge-based and deeply cut into 3-5 (7) lobes toothed toward their
  tips; dark glossy green above, paler below, in tufts on the spurs.

Game notes: trunk-only capsule collision, tilted along the leaning bole; four LODs (see
homestead_tree.py) with two material slots (0 bark, 1 leaves); wind in the Wind vertex colour
(R height, G branch phase, B flutter). The prevailing wind comes from -X (the crown streams
toward +X), so rotate instances to match the local wind.
"""
import math
import random

import numpy as np

import homestead_foliage as F
import homestead_tree as T

NAME = "Hawthorn"
DESCRIPTION = ("Wind-pruned hawthorn, about 5 m tall with a dense flat-topped crown streaming 7 m "
               "downwind from a leaning lichen-crusted bole; painted leaf-cluster canopy, four LODs, "
               "tilted trunk capsule.")
COLLISION = "capsule"
TRIANGLE_BUDGET = 70000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
SEED = 7411
BEAUTY = {"pose": (0, 0, 0), "focus": (1.5, 0.0, 2.6), "views": ("hero", "detail", "eye"),
          "eye_distance": 9.0, "detail_distance": 3.5, "detail_fstop": 8.0}

CLUSTERS = ("c0", "c1", "c2")
MASS = ("m0",)
LEAF_KEYS = ("mid", "mid", "dark", "dark", "light", "under", "yellow", "holes")

SPEC = dict(
    height=5.2, rx=3.5, ry=2.9, center=(2.0, 0.0), seed=SEED, crown_center=0.6, crown_base=0.2,
    profile=[(0.0, 0.3), (0.14, 0.5), (0.24, 0.8), (0.4, 1.0), (0.6, 0.98), (0.78, 0.8),
             (0.92, 0.5), (1.0, 0.12)],
    lobes=0.2, stem_radius=0.21, step=0.12, min_radius=0.0035, bark="bark",
    # The prevailing wind pushes every shoot downwind (+X) and holds it low.
    bias=(0.32, 0.0, -0.04),
    flare=dict(amount=0.35, height=0.35, lobe_height=0.6, rough=0.03,
               lobes=[(4, 0.16, 0.3), (3, 0.08, 1.9), (7, 0.05, 1.1)]),
    skirt=0.2, bark_size=0.5, capsule_height=1.5,
    levels=[
        dict(count=1, elev=(52, 58), azimuth=0.0, azimuth_jitter=8.0, length=(0.62, 0.7), tropism=0.55,
             outward=0.0, wander=0.25, stop=8.0, tip_radius=0.18, step=0.12),
        dict(per_m=3.0, start=0.25, end=0.95, angle=(35, 70), abs_length=(1.8, 3.6), radius=(0.55, 0.75),
             up=0.15, out=0.4, tropism=0.25, outward=0.25, wander=0.7, stop=1.0, gravity=0.02,
             max=11, step=0.12, tip_radius=0.12),
        dict(per_m=4.5, start=0.08, end=0.97, angle=(35, 70), abs_length=(0.6, 1.5), radius=(0.45, 0.6),
             up=0.2, out=0.35, tropism=0.2, outward=0.3, wander=1.1, stop=1.02, max=14, step=0.08,
             tip_radius=0.25),
        dict(per_m=6.5, start=0.1, end=0.98, angle=(40, 75), abs_length=(0.2, 0.5), radius=(0.5, 0.65),
             up=0.2, out=0.3, tropism=0.1, outward=0.3, wander=1.3, stop=1.05, max=10, step=0.06,
             inner=0.4, inner_skip=0.3, tip_radius=0.4),
    ],
    shell=dict(count=700, depth=(0.7, 1.0), max_len=0.8, z_min=0.2, radius=0.007, max_level=2),
    foliage=dict(level=3, zone=0.6, spacing=(0.09, 0.15), size=0.6, inner=0.5, inner_density=0.6,
                 on_parents=0.6, along=0.5, face_out=0.7, face_up=0.55, droop=0.08, min_z=0.16),
    normal_blend=0.6,
    lods=[
        dict(levels=9, sides=[16, 10, 5, 3], trunk_sides=18, step=0.1, grid=(2, 2), fold=0.18,
             droop=0.12, back=0.08),
        dict(levels=2, sides=[10, 6, 4], trunk_sides=10, step=0.3, cell=0.7, card_scale=1.5,
             mass=True, grid=(1, 1), back=0.5, droop=0.0),
        dict(levels=2, sides=[6, 4, 3], trunk_sides=6, step=0.6, min_radius=0.025, cell=1.3,
             card_scale=1.5, mass=True, grid=(1, 1), back=0.5, droop=0.0),
        dict(levels=1, sides=[5, 3], trunk_sides=5, step=1.2, min_radius=0.06, cell=2.2,
             card_scale=1.45, mass=True, grid=(1, 1), back=0.5, droop=0.0, min_count=2),
    ],
)


def _palettes():
    base = dict(yellow=(0.20, 0.16, 0.05), brown=(0.07, 0.035, 0.014), stalk=(0.08, 0.06, 0.03),
                dry=(0.15, 0.09, 0.04))
    return {
        "mid": dict(base, base=(0.032, 0.060, 0.016), tip=(0.036, 0.064, 0.017), vein=(0.07, 0.095, 0.033),
                    margin=(0.03, 0.052, 0.014)),
        "dark": dict(base, base=(0.022, 0.044, 0.012), tip=(0.025, 0.048, 0.013), vein=(0.054, 0.076, 0.028),
                     margin=(0.02, 0.038, 0.01)),
        "light": dict(base, base=(0.062, 0.098, 0.026), tip=(0.07, 0.104, 0.028), vein=(0.095, 0.125, 0.043),
                      margin=(0.055, 0.086, 0.023)),
        "under": dict(base, base=(0.056, 0.076, 0.040), tip=(0.06, 0.08, 0.042), vein=(0.082, 0.098, 0.058),
                      margin=(0.05, 0.068, 0.036)),
        "yellow": dict(base, base=(0.065, 0.085, 0.022), tip=(0.12, 0.12, 0.032), vein=(0.12, 0.12, 0.045),
                       margin=(0.09, 0.08, 0.028)),
        "holes": dict(base, base=(0.036, 0.062, 0.017), tip=(0.042, 0.066, 0.018), vein=(0.072, 0.096, 0.034),
                      margin=(0.045, 0.052, 0.018)),
    }


PARAMS = {
    "mid": dict(gloss=0.4, damage=0.06),
    "dark": dict(gloss=0.5),
    "light": dict(gloss=0.2, trans=0.7),
    "under": dict(gloss=-0.3, hair=0.1, trans=0.5),
    "yellow": dict(yellow=0.4, damage=0.5, edge_burn=0.3),
    "holes": dict(holes=1.0, damage=0.3, edge_burn=0.12),
}

ATTACH = 0.05


def hawthorn_shape(rng):
    """Wedge-based blade cut deeply into 3-5 lobes, toothed toward their tips."""
    n_pairs = rng.choice((1, 2, 2))
    tooth = dict(teeth=rng.choice((3, 4)), tooth_depth=0.12, double=0.2)
    terminal = F.ovate(width=0.5, base=0.0, tip=1.0, widest=0.55, tip_sharp=0.9, base_round=0.6,
                       phase=rng.uniform(0, 1), **tooth)
    origin = (0.0, ATTACH)
    parts = [T.placed(terminal, origin, rng.uniform(-0.08, 0.08), 0.95),
             # The narrow wedge of blade between the stalk and the lobes.
             T.placed(F.ovate(width=0.5, base=0.0, tip=1.0, widest=0.8, tip_sharp=0.6, base_round=0.3),
                      origin, 0.0, 0.42)]
    veins = [([(0.0, ATTACH + 0.9 * t) for t in np.linspace(0, 1, 8)], 1.0)]
    for k in range(n_pairs):
        s = ATTACH + (0.2 + 0.2 * k) * rng.uniform(0.92, 1.08)
        for side in (-1, 1):
            ang = side * math.radians((58 - 16 * k) + rng.uniform(-8, 8))
            ln = (0.5 - 0.12 * k) * rng.uniform(0.88, 1.1)
            lobe = F.ovate(width=0.52, base=0.0, tip=1.0, widest=0.6, tip_sharp=0.9, base_round=0.7,
                           teeth=rng.choice((2, 3)), tooth_depth=0.12, phase=rng.uniform(0, 1))
            parts.append(T.placed(lobe, (0.0, s), ang, ln))
            veins.append(([(0.0, s), (math.sin(ang) * ln * 0.85, s + math.cos(ang) * ln * 0.85)], 0.55))
    return T.union(*parts), veins


def make_blade(nrng, rng):
    pals = _palettes()
    shapes = [hawthorn_shape(random.Random(SEED + 7 * i)) for i in range(8)]

    def blade(X, Y, px, key):
        key = key or "mid"
        shape, veins = shapes[rng.randrange(len(shapes))]
        return F.paint_blade(X, Y, nrng, shape, veins, pals[key], px, vein_width=0.01,
                             vein_depth=0.0003, puff=0.0003, tertiary=0.3, stalk=0.03, **PARAMS[key])
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
        # Zig-zag twig with many short spurs, each a tuft of small leaves.
        layout = T.cluster_layout(lrng, pattern="tips", side_twigs=9, side_len=(0.14, 0.3),
                                  side_angle=(40, 75), leaf_len=0.13, tip_fan=(4, 7), twig_width=0.009,
                                  curve=0.3, keys=LEAF_KEYS)
        T.paint_cluster(atlas, key, nrng, lrng, blade, layout, twig_color=(0.07, 0.055, 0.045), meters=0.5)
    T.paint_mass(atlas, "m0", nrng, random.Random(SEED + 5), blade, count=1400, leaf_len=0.035,
                 keys=LEAF_KEYS, core=(0.010, 0.018, 0.006), holes=0.45, meters=2.5)
    atlas.save()
    return atlas


def paint_bark():
    atlas = F.Atlas(NAME + "Bark", size=1024, seed=SEED + 1, folder=F.props_folder(NAME) / "Textures")
    atlas.column("bark", 1024)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED + 1)
    T.paint_bark(atlas, "bark", nrng, size_m=SPEC["bark_size"], style="fissured",
                 base=(0.095, 0.080, 0.064), ridge=(0.15, 0.13, 0.105), fissure=(0.028, 0.02, 0.014),
                 depth=0.012, ridges=11.0, breaks=0.95, lichen=0.55, lichen_color=(0.27, 0.29, 0.22),
                 moss=0.2, moss_color=(0.045, 0.07, 0.018), algae=0.25, rough=0.84)
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
             "prevailing_wind": "from -X (crown streams toward +X)",
             "note": "Pivot is the bottom of a 20 cm skirt under the trunk flare; the ground line is 20 cm up."},
    "wind": {"attribute": "Wind (vertex colour)", "R": "height above ground / tree height",
             "G": "per-branch random phase", "B": "leaf flutter 0 at card base -> 1 at tip, 0 on wood", "A": "1",
             "strength": 8.0, "flutter": 1.0},
    "material_notes": ("Slot 0 M_HawthornBark: tiling bark T_HawthornBark_* (0.5 m square). Slot 1 "
                       "M_HawthornLeaves: leaf-cluster atlas T_HawthornLeaves_* (basecolor alpha = mask, clip 0.5; "
                       "roughness R rough, G translucency, B AO; normal OpenGL). Two-sided foliage, masked."),
}
