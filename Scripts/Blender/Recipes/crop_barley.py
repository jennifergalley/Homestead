"""Barley crop plot: a 1 m drilled stand of two-rowed Chevalier-type malting barley, five stages.

Research notes and the build live in homestead_grain.py (shared with crop_wheat.py). Barley:
shorter than wheat (75 cm on the plot), broader paler leaves, and a thin two-rowed ear with a long
stiff beard of awns that hooks over as it ripens to pale straw-gold. Harvest: Cut, yielding tied
barley sheaves.
"""
import homestead_grain as G

NAME = "CropBarley"
DESCRIPTION = ("One 1 m tilled-bed plot of drilled bearded barley, 30 tillered plants on three ridges; "
               "the awned ears are separate instanced produce that nod as they ripen.")
COLLISION = "none"
TRIANGLE_BUDGET = 7000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
SEED = 1851_577
BEAUTY = {"pose": (0, 0, 0), "views": ["hero", "detail", "eye"], "eye_distance": 4.2,
          "meshes": {
              "SM_CropBarley_Growing": {"focus": (0.0, 0.0, 0.26), "eye_distance": 3.8},
              "SM_CropBarley_Ripe": {"focus": (0.0, 0.0, 0.42), "eye_distance": 4.2},
              "SM_CropBarley_Produce": {"focus": (0.03, 0.0, 0.07), "eye_distance": 0.7,
                                        "detail_distance": 0.30, "detail_fstop": 32.0},
              "SM_CropBarley_Harvest": {"focus": (0.0, 0.0, -0.18), "eye_distance": 1.6},
          }}
REPORT = {
    "blocking": False,
    "wind": {"attribute": "Wind (vertex colour)", "R": "height above ground / stage height",
             "G": "per-plant phase", "B": "leaf flutter 0.1 at the sheath -> 1 at the tip", "A": "1"},
    "crop_layout": "30 plants (4 tillers each) along the ridges y=-0.3,0,+0.3; base z=0.045 m.",
    "material_notes": "One M_CropBarley atlas: alpha in basecolor; roughness R, translucency G, AO B. "
                      "Ears and awns use M_CropBarleyProduce (re-parented to M_CropProduce in Unreal).",
    "harvest_mesh": "SM_CropBarley_Harvest: tied sheaf, grip at the band (origin), ears hang down -Z.",
}

CONFIG = G.Grain(
    name=NAME, seed=SEED, report=REPORT, plants_per_ridge=10, tillers=4,
    culm_radius=0.0020, blade_halfwidth=0.036, leaf_len=0.20,
    leaf=(0.070, 0.135, 0.042), leaf_pale=(0.098, 0.165, 0.058), leaf_vein=(0.110, 0.170, 0.068),
    leaf_young=(0.085, 0.175, 0.045),
    stem=(0.090, 0.150, 0.056), stem_hi=(0.135, 0.195, 0.082),
    straw=(0.56, 0.45, 0.20), straw_hi=(0.74, 0.62, 0.32), straw_dark=(0.30, 0.20, 0.08),
    ear_light=(0.76, 0.62, 0.30), ear_mid=(0.60, 0.46, 0.20), ear_dark=(0.34, 0.23, 0.08),
    ear_len=0.075, ear_radius=0.0056, ear_flatten=0.55, ear_bow=0.010, ear_lean=38.0,
    spikelets=14, awns=True, awn_len=0.13, sheaf_len=0.34,
    stages={
        "Sprout": dict(blades=5, blade_len=0.095, blade_elev=(60, 84), blade_droop=(0.05, 0.25),
                       blade_key="blade_young"),
        "Young": dict(tufts=3, tuft_len=0.19, tuft_key="tuft_young", blades=2, blade_len=0.17,
                      blade_elev=(40, 70), blade_droop=(0.5, 1.0), blade_key="blade"),
        "Growing": dict(tufts=2, tuft_len=0.16, tuft_key="tuft", culm=0.44, lean=(1, 5), leaves=2,
                        leaf_keys=("blade", "blade_pale"), leaf_elev=(28, 52), leaf_droop=(0.6, 1.1)),
        "Mature": dict(tufts=1, tuft_len=0.15, tuft_key="tuft", culm=0.70, lean=(2, 8), leaves=2,
                       leaf_keys=("blade_turning", "blade_pale"), leaf_elev=(25, 50), leaf_droop=(0.7, 1.2)),
        "Ripe": dict(tufts=1, tuft_len=0.14, tuft_key="tuft_dry", culm=0.74, lean=(5, 12), leaves=2,
                     leaf_keys=("blade_dry", "blade_turning"), leaf_elev=(15, 45), leaf_droop=(0.9, 1.5)),
    },
)


def build(kit):
    return G.build(kit, CONFIG)
