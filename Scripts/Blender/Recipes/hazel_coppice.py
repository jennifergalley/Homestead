"""Hazel coppice stool (Corylus avellana): the understorey of every worked Cornish oakwood.

The real plant (what the geometry and textures follow):
- Cut to the ground every 7-15 years for rods, hurdles and thatching spars, a hazel regrows as a
  stool: a knuckled base 0.5-1 m across sending up a dozen or more straight, slender poles that
  rise steeply and arch outward into an open vase, 2.5-4 m after a few years' growth.
- Bark smooth, glossy grey-brown to coppery, peeling in thin papery strips on old poles, dotted with
  pale lenticels; young twigs glandular-hairy.
- Leaves alternate in flat two-ranked sprays, 6-12 cm, almost round with a heart-shaped base and an
  abrupt short point, doubly toothed, soft-hairy, mid green and matte, paler beneath, with 6-8
  pairs of straight veins.

Game notes: walk-through (no collision). Four LODs (see homestead_tree.py) with two material slots
(0 bark, 1 leaves); wind in the Wind vertex colour (R height, G branch phase, B flutter). Not the
older ``Hazel`` prop (a California hazelnut for the MVP forest).
"""
import random

import numpy as np

import homestead_foliage as F
import homestead_tree as T

NAME = "HazelCoppice"
DESCRIPTION = ("Hazel coppice stool, a vase of about fifteen straight poles 2.8 m tall arching to 3 m across "
               "from a knuckled 0.8 m stool; painted leaf-spray canopy, four LODs, walk-through.")
COLLISION = "none"
TRIANGLE_BUDGET = 70000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
SEED = 9241
BEAUTY = {"pose": (0, 0, 0), "focus": (0.0, 0.0, 1.4), "views": ("hero", "detail", "eye"),
          "eye_distance": 6.0, "detail_distance": 2.2, "detail_fstop": 8.0}

CLUSTERS = ("c0", "c1", "c2")
MASS = ("m0",)
LEAF_KEYS = ("mid", "mid", "mid", "dark", "light", "light", "under", "yellow")

SPEC = dict(
    height=2.8, rx=1.55, ry=1.45, seed=SEED, crown_center=0.58, crown_base=0.1,
    crowns=[(0.0, 0.0), (0.22, 0.08), (-0.2, 0.12), (0.1, -0.22), (-0.14, -0.18), (0.28, -0.1),
            (-0.3, -0.02), (0.02, 0.26)],
    crown_jitter=0.05, clumps=True,
    profile=[(0.0, 0.3), (0.15, 0.55), (0.4, 0.82), (0.68, 1.0), (0.86, 0.86), (1.0, 0.35)],
    lobes=0.16, stem_radius=0.028, step=0.06, min_radius=0.002, bark="bark",
    flare=dict(amount=0.35, height=0.08, lobe_height=0.12, rough=0.03,
               lobes=[(3, 0.12, 0.4), (5, 0.06, 1.2)]),
    skirt=0.15, bark_size=0.3, capsule_height=1.0,
    levels=[
        dict(count=15, elev=(62, 86), length=(0.82, 1.0), tropism=0.25, outward=0.25, wander=0.18,
             stop=1.15, tip_radius=0.2, step=0.06, gravity=0.004, low_frac=0.2, low_elev=(45, 60),
             low_length=(0.5, 0.7)),
        dict(per_m=3.4, start=0.14, end=0.96, angle=(28, 55), abs_length=(0.35, 0.9), radius=(0.4, 0.55),
             up=0.12, out=0.5, tropism=0.12, outward=0.3, wander=0.5, stop=1.02, max=9, step=0.05,
             gravity=0.01, tip_radius=0.25),
        dict(per_m=6.0, start=0.12, end=0.98, angle=(35, 65), abs_length=(0.12, 0.3), radius=(0.5, 0.65),
             up=0.1, out=0.35, tropism=0.05, outward=0.3, wander=0.8, stop=1.06, max=6, step=0.04,
             tip_radius=0.45),
    ],
    shell=dict(count=260, depth=(0.72, 1.0), max_len=0.4, z_min=0.1, radius=0.004, max_level=1),
    foliage=dict(level=2, zone=0.3, spacing=(0.08, 0.13), size=0.5, inner=0.5, inner_density=0.5,
                 on_parents=0.6, along=0.5, face_out=0.55, face_up=0.75, droop=0.05, min_z=0.08),
    normal_blend=0.5,
    lods=[
        dict(levels=9, sides=[10, 6, 3], trunk_sides=10, step=0.06, grid=(2, 2), fold=0.1,
             droop=0.08, back=0.08),
        dict(levels=2, sides=[6, 4], trunk_sides=6, step=0.2, cell=0.45, card_scale=1.55,
             mass=True, grid=(1, 1), back=0.5, droop=0.0),
        dict(levels=1, sides=[4, 3], trunk_sides=4, step=0.4, min_radius=0.012, cell=0.85,
             card_scale=1.55, mass=True, grid=(1, 1), back=0.5, droop=0.0),
        dict(levels=1, sides=[3, 3], trunk_sides=3, step=0.9, min_radius=0.02, cell=1.4,
             card_scale=1.5, mass=True, grid=(1, 1), back=0.5, droop=0.0, min_count=2),
    ],
)


def _palettes():
    base = dict(yellow=(0.20, 0.17, 0.05), brown=(0.09, 0.05, 0.02), stalk=(0.09, 0.07, 0.035),
                dry=(0.16, 0.10, 0.04))
    return {
        "mid": dict(base, base=(0.050, 0.086, 0.020), tip=(0.056, 0.092, 0.022), vein=(0.075, 0.11, 0.032),
                    margin=(0.044, 0.074, 0.018)),
        "dark": dict(base, base=(0.036, 0.064, 0.015), tip=(0.040, 0.068, 0.016), vein=(0.056, 0.085, 0.026),
                     margin=(0.032, 0.056, 0.013)),
        "light": dict(base, base=(0.080, 0.125, 0.030), tip=(0.090, 0.135, 0.032), vein=(0.105, 0.15, 0.045),
                      margin=(0.072, 0.11, 0.027)),
        "under": dict(base, base=(0.070, 0.095, 0.045), tip=(0.074, 0.10, 0.047), vein=(0.095, 0.12, 0.06),
                      margin=(0.064, 0.086, 0.04)),
        "yellow": dict(base, base=(0.08, 0.105, 0.026), tip=(0.13, 0.135, 0.032), vein=(0.13, 0.14, 0.05),
                       margin=(0.11, 0.105, 0.03)),
    }


PARAMS = {
    "mid": dict(gloss=-0.1, damage=0.08, hair=0.35),
    "dark": dict(gloss=0.0, hair=0.3, damage=0.04),
    "light": dict(gloss=-0.1, trans=0.8, hair=0.35),
    "under": dict(gloss=-0.4, hair=0.5, trans=0.55),
    "yellow": dict(yellow=0.35, damage=0.45, edge_burn=0.2, holes=0.4, hair=0.3),
}


def hazel_shape(rng):
    """Round, heart-based blade with an abrupt short point, doubly toothed; 6-8 straight vein pairs."""
    width = rng.uniform(0.40, 0.45)
    shape = F.ovate(width=width, base=0.06, tip=0.98, widest=0.46, tip_sharp=2.4, base_round=0.45,
                    teeth=rng.choice((9, 10, 11)), tooth_depth=0.05, double=0.6, cordate=0.35,
                    phase=rng.uniform(0, 1))
    veins = F.pinnate_veins(count=rng.choice((6, 7, 7, 8)), angle=0.75, curve=0.2, reach=0.92, base=0.06,
                            tip=0.98, width=width, widest=0.46, start=0.08, stop=0.8,
                            rng=np.random.default_rng(rng.randrange(1 << 30)))
    return shape, veins


def make_blade(nrng, rng):
    pals = _palettes()
    shapes = [hazel_shape(random.Random(SEED + 7 * i)) for i in range(8)]

    def blade(X, Y, px, key):
        key = key or "mid"
        shape, veins = shapes[rng.randrange(len(shapes))]
        return F.paint_blade(X, Y, nrng, shape, veins, pals[key], px, vein_width=0.009,
                             vein_depth=0.0006, puff=0.0005, tertiary=0.6, stalk=0.03, **PARAMS[key])
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
        # Flat two-ranked sprays of big round leaves laid out to catch the shade light.
        layout = T.cluster_layout(lrng, pattern="alternate", side_twigs=4, side_len=(0.3, 0.5),
                                  side_angle=(38, 62), leaf_len=0.24, per_twig=(3, 5), leaf_angle=(55, 85),
                                  squash=(0.8, 1.0), twig_width=0.009, curve=0.14, keys=LEAF_KEYS)
        T.paint_cluster(atlas, key, nrng, lrng, blade, layout, twig_color=(0.10, 0.075, 0.05), meters=0.5)
    T.paint_mass(atlas, "m0", nrng, random.Random(SEED + 5), blade, count=700, leaf_len=0.07,
                 keys=LEAF_KEYS, core=(0.014, 0.026, 0.008), holes=0.5, meters=1.5)
    atlas.save()
    return atlas


def paint_bark():
    atlas = F.Atlas(NAME + "Bark", size=1024, seed=SEED + 1, folder=F.props_folder(NAME) / "Textures")
    atlas.column("bark", 1024)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED + 1)
    T.paint_bark(atlas, "bark", nrng, size_m=SPEC["bark_size"], style="smooth",
                 base=(0.105, 0.078, 0.058), depth=0.002, lenticels=0.9, algae=0.35,
                 lichen=0.25, lichen_color=(0.2, 0.21, 0.17), moss=0.0, rough=0.55, stripes=0.2)
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
             "note": "Pivot is the bottom of a 15 cm skirt under the stool; the ground line is 15 cm up. "
                     "No collision."},
    "wind": {"attribute": "Wind (vertex colour)", "R": "height above ground / plant height",
             "G": "per-branch random phase", "B": "leaf flutter 0 at card base -> 1 at tip, 0 on wood", "A": "1",
             "strength": 7.0, "flutter": 1.0},
    "material_notes": ("Slot 0 M_HazelCoppiceBark: tiling smooth bark T_HazelCoppiceBark_* (0.3 m square). Slot 1 "
                       "M_HazelCoppiceLeaves: leaf-spray atlas T_HazelCoppiceLeaves_* (basecolor alpha = mask, "
                       "clip 0.5; roughness R rough, G translucency, B AO; normal OpenGL). Two-sided foliage, masked."),
}
