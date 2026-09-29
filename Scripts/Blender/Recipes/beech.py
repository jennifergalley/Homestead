"""Common beech (Fagus sylvatica): a mature woodland beech for the Cornish estate.

The real tree (what the geometry and textures follow):
- Planted and self-sown in Cornish valley woods, estates and hedgebanks. A woodland beech stands
  18-22 m: a tall, clean, smooth bole (often 6-8 m to the first limbs, 0.7-1.0 m across) with a
  spreading "elephant's foot" of surface roots, then steep, sweeping limbs that fork into long
  fans of fine zig-zag twigs. The crown is tall and domed, dense and layered: leaves sit in flat
  sprays, so the canopy casts a deep shade and reads as smooth tiers of green.
- Bark thin, smooth and silver-grey, faintly wrinkled round branch junctions, with horizontal
  dashes, green algal bloom on the damp west side and pale lichen crusts.
- Leaves 5-10 cm, oval with a short point, wavy (not toothed) margin fringed with fine hairs,
  7-9 pairs of straight, parallel veins; bright mid-green and glossy above, paler below;
  alternate in two ranks along the twigs.

Game notes: trunk-only capsule collision; four LODs (see homestead_tree.py) with two material
slots (0 bark, 1 leaves); wind in the Wind vertex colour (R height, G branch phase, B flutter).
"""
import random

import numpy as np

import homestead_foliage as F
import homestead_tree as T

NAME = "Beech"
DESCRIPTION = ("Mature common beech, about 20 m tall with a tall domed 14 m crown on a smooth grey "
               "bole; layered painted leaf-spray canopy, four LODs, trunk capsule.")
COLLISION = "capsule"
TRIANGLE_BUDGET = 90000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
SEED = 5203
BEAUTY = {"pose": (0, 0, 0), "focus": (0.0, 0.0, 6.0), "views": ("hero", "detail", "eye"),
          "eye_distance": 18.0, "detail_distance": 6.0, "detail_fstop": 11.0}

CLUSTERS = ("c0", "c1", "c2")
MASS = ("m0",)
LEAF_KEYS = ("mid", "mid", "mid", "dark", "light", "light", "under", "yellow")

SPEC = dict(
    height=20.0, rx=7.4, ry=7.0, seed=SEED, crown_center=0.64, crown_base=0.3,
    profile=[(0.0, 0.06), (0.26, 0.10), (0.33, 0.55), (0.45, 0.88), (0.60, 1.0), (0.76, 0.92),
             (0.90, 0.64), (1.0, 0.2)],
    lobes=0.12, stem_radius=0.46, step=0.3, min_radius=0.004, bark="bark",
    flare=dict(amount=0.55, height=0.8, lobe_height=1.1, rough=0.02,
               lobes=[(6, 0.16, 0.2), (4, 0.06, 1.7), (9, 0.04, 2.6)]),
    skirt=0.3, bark_size=1.0, capsule_height=5.5,
    levels=[
        dict(count=1, elev=(87, 89), length=(0.72, 0.78), tropism=1.0, outward=0.0, wander=0.18,
             stop=0.97, tip_radius=0.1, step=0.3),
        dict(per_m=1.1, start=0.4, end=0.92, angle=(24, 44), abs_length=(6.5, 9.5), radius=(0.6, 0.78),
             up=0.3, out=0.35, tropism=0.35, outward=0.14, wander=0.35, stop=1.0, gravity=0.01,
             max=9, step=0.25, tip_radius=0.1),
        dict(per_m=1.6, start=0.15, end=0.97, angle=(45, 72), abs_length=(1.8, 4.0), radius=(0.4, 0.56),
             up=0.05, out=0.5, tropism=0.08, outward=0.35, wander=0.6, stop=1.02, gravity=0.02, max=13,
             step=0.15, tip_radius=0.2),
        dict(per_m=3.0, start=0.12, end=0.98, angle=(40, 70), abs_length=(0.5, 1.1), radius=(0.45, 0.6),
             up=0.08, out=0.3, tropism=0.05, outward=0.25, wander=0.9, stop=1.05, max=11, step=0.1,
             inner=0.5, inner_skip=0.6, tip_radius=0.35),
    ],
    shell=dict(count=620, depth=(0.72, 1.0), max_len=1.6, z_min=0.3, radius=0.011, max_level=2),
    foliage=dict(level=3, zone=1.0, spacing=(0.2, 0.3), size=0.9, inner=0.6, inner_density=0.35,
                 on_parents=0.45, along=0.55, face_out=0.55, face_up=0.7, droop=0.06, min_z=0.28),
    normal_blend=0.6,
    lods=[
        dict(levels=9, sides=[22, 12, 5, 3], trunk_sides=26, step=0.2, grid=(2, 2), fold=0.12,
             droop=0.08, back=0.08),
        dict(levels=2, sides=[14, 8, 4], trunk_sides=14, step=0.5, cell=1.25, card_scale=1.5,
             mass=True, grid=(1, 1), back=0.5, droop=0.0),
        dict(levels=2, sides=[8, 4, 3], trunk_sides=8, step=1.0, min_radius=0.04, cell=2.4,
             card_scale=1.5, mass=True, grid=(1, 1), back=0.5, droop=0.0),
        dict(levels=1, sides=[6, 3], trunk_sides=6, step=2.0, min_radius=0.11, cell=4.0,
             card_scale=1.45, mass=True, grid=(1, 1), back=0.5, droop=0.0, min_count=2),
    ],
)


def _palettes():
    base = dict(yellow=(0.20, 0.17, 0.05), brown=(0.09, 0.05, 0.02), stalk=(0.08, 0.06, 0.03),
                dry=(0.16, 0.10, 0.04))
    return {
        "mid": dict(base, base=(0.046, 0.082, 0.018), tip=(0.052, 0.088, 0.020), vein=(0.085, 0.12, 0.035),
                    margin=(0.040, 0.070, 0.016)),
        "dark": dict(base, base=(0.032, 0.060, 0.014), tip=(0.036, 0.064, 0.015), vein=(0.064, 0.094, 0.030),
                     margin=(0.030, 0.052, 0.012)),
        "light": dict(base, base=(0.075, 0.12, 0.026), tip=(0.085, 0.13, 0.028), vein=(0.11, 0.15, 0.045),
                      margin=(0.068, 0.105, 0.024)),
        "under": dict(base, base=(0.064, 0.090, 0.040), tip=(0.068, 0.094, 0.042), vein=(0.09, 0.115, 0.06),
                      margin=(0.058, 0.08, 0.036)),
        "yellow": dict(base, base=(0.075, 0.10, 0.024), tip=(0.12, 0.13, 0.03), vein=(0.13, 0.14, 0.05),
                       margin=(0.10, 0.10, 0.03)),
    }


PARAMS = {
    "mid": dict(gloss=0.45, damage=0.05, hair=0.12),
    "dark": dict(gloss=0.5, hair=0.1),
    "light": dict(gloss=0.3, trans=0.75, hair=0.15),
    "under": dict(gloss=-0.2, hair=0.3, trans=0.5),
    "yellow": dict(yellow=0.35, damage=0.4, edge_burn=0.25),
}


def beech_shape(rng):
    """Oval blade with a short point and a softly wavy margin; 7-9 straight parallel vein pairs."""
    width = rng.uniform(0.30, 0.34)
    shape = F.ovate(width=width, base=0.05, tip=0.98, widest=0.44, tip_sharp=1.25, base_round=0.75,
                    teeth=rng.choice((7, 8, 9)), tooth_depth=0.035, phase=rng.uniform(0, 1))
    veins = F.pinnate_veins(count=rng.choice((7, 8, 8, 9)), angle=0.7, curve=0.12, reach=0.93, base=0.05,
                            tip=0.98, width=width, widest=0.44, start=0.1, stop=0.84,
                            rng=np.random.default_rng(rng.randrange(1 << 30)))
    return shape, veins


def make_blade(nrng, rng):
    pals = _palettes()
    shapes = [beech_shape(random.Random(SEED + 7 * i)) for i in range(8)]

    def blade(X, Y, px, key):
        key = key or "mid"
        shape, veins = shapes[rng.randrange(len(shapes))]
        return F.paint_blade(X, Y, nrng, shape, veins, pals[key], px, vein_width=0.008,
                             vein_depth=0.0004, puff=0.0003, tertiary=0.35, stalk=0.05, **PARAMS[key])
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
        # Flat two-ranked sprays: a zig-zag main twig with side shoots, leaves alternating.
        layout = T.cluster_layout(lrng, pattern="alternate", side_twigs=6, side_len=(0.3, 0.5),
                                  side_angle=(38, 62), leaf_len=0.17, per_twig=(4, 6), leaf_angle=(50, 80),
                                  squash=(0.75, 1.0), twig_width=0.008, curve=0.1, keys=LEAF_KEYS)
        T.paint_cluster(atlas, key, nrng, lrng, blade, layout, twig_color=(0.075, 0.055, 0.04), meters=0.75)
    T.paint_mass(atlas, "m0", nrng, random.Random(SEED + 5), blade, count=1000, leaf_len=0.042,
                 keys=LEAF_KEYS, core=(0.013, 0.024, 0.007), holes=0.45, meters=4.0)
    atlas.save()
    return atlas


def paint_bark():
    atlas = F.Atlas(NAME + "Bark", size=2048, seed=SEED + 1, folder=F.props_folder(NAME) / "Textures")
    atlas.column("bark", 2048)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED + 1)
    T.paint_bark(atlas, "bark", nrng, size_m=SPEC["bark_size"], style="smooth",
                 base=(0.088, 0.086, 0.078), depth=0.004, lenticels=0.5, algae=0.65,
                 lichen=0.3, lichen_color=(0.19, 0.20, 0.16), moss=0.0, rough=0.7, stripes=0.35)
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
             "strength": 16.0, "flutter": 0.9},
    "material_notes": ("Slot 0 M_BeechBark: tiling bark T_BeechBark_* (1 m square). Slot 1 M_BeechLeaves: leaf-spray "
                       "atlas T_BeechLeaves_* (basecolor alpha = mask, clip 0.5; roughness R rough, G translucency, "
                       "B AO; normal OpenGL). Two-sided foliage, masked."),
}
