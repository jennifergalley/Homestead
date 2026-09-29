"""Wheat crop plot: a 1 m drilled stand of red Lammas-type wheat in five growth stages.

Research notes and the build live in homestead_grain.py (shared with crop_barley.py). Wheat: tall
(95 cm on the plot), narrow dark-green leaves, awnless russet-gold ears 9 cm long that stay erect
with a slight lean. Harvest: Cut (the cabbage kneel-and-cut), yielding tied wheat sheaves.
"""
import homestead_grain as G

NAME = "CropWheat"
DESCRIPTION = ("One 1 m tilled-bed plot of drilled wheat, 27 tillered plants on three ridges, from a "
               "green braird to a straw-gold stand; the ears are separate instanced produce.")
COLLISION = "none"
TRIANGLE_BUDGET = 7000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
SEED = 1851_231
BEAUTY = {"pose": (0, 0, 0), "views": ["hero", "detail", "eye"], "eye_distance": 4.5,
          "meshes": {
              "SM_CropWheat_Growing": {"focus": (0.0, 0.0, 0.30), "eye_distance": 4.0},
              "SM_CropWheat_Ripe": {"focus": (0.0, 0.0, 0.50), "eye_distance": 4.6},
              "SM_CropWheat_Produce": {"focus": (0.01, 0.0, 0.05), "eye_distance": 0.6,
                                       "detail_distance": 0.25, "detail_fstop": 32.0},
              "SM_CropWheat_Harvest": {"focus": (0.0, 0.0, -0.20), "eye_distance": 1.6},
          }}
REPORT = {
    "blocking": False,
    "wind": {"attribute": "Wind (vertex colour)", "R": "height above ground / stage height",
             "G": "per-plant phase", "B": "leaf flutter 0.1 at the sheath -> 1 at the tip", "A": "1"},
    "crop_layout": "27 plants (4 tillers each) along the ridges y=-0.3,0,+0.3; base z=0.045 m.",
    "material_notes": "One M_CropWheat atlas: alpha in basecolor; roughness R, translucency G, AO B. "
                      "Ears use M_CropWheatProduce (re-parented to M_CropProduce in Unreal).",
    "harvest_mesh": "SM_CropWheat_Harvest: tied sheaf, grip at the band (origin), ears hang down -Z.",
}

CONFIG = G.Grain(
    name=NAME, seed=SEED, report=REPORT, plants_per_ridge=9, tillers=4,
    culm_radius=0.0023, blade_halfwidth=0.030, leaf_len=0.24,
    leaf=(0.058, 0.118, 0.034), leaf_pale=(0.082, 0.148, 0.048), leaf_vein=(0.095, 0.155, 0.060),
    leaf_young=(0.078, 0.168, 0.040),
    stem=(0.080, 0.140, 0.050), stem_hi=(0.125, 0.185, 0.075),
    straw=(0.50, 0.35, 0.13), straw_hi=(0.68, 0.51, 0.23), straw_dark=(0.26, 0.16, 0.06),
    ear_light=(0.70, 0.48, 0.19), ear_mid=(0.56, 0.36, 0.13), ear_dark=(0.38, 0.22, 0.08),
    ear_len=0.090, ear_radius=0.0064, ear_flatten=0.66, ear_bow=0.004, ear_lean=12.0,
    spikelets=18, awns=False, awn_len=0.0, sheaf_len=0.40,
    stages={
        "Sprout": dict(blades=5, blade_len=0.100, blade_elev=(62, 84), blade_droop=(0.05, 0.25),
                       blade_key="blade_young"),
        "Young": dict(tufts=3, tuft_len=0.22, tuft_key="tuft_young", blades=2, blade_len=0.20,
                      blade_elev=(40, 70), blade_droop=(0.5, 1.0), blade_key="blade"),
        "Growing": dict(tufts=2, tuft_len=0.18, tuft_key="tuft", culm=0.56, lean=(1, 5), leaves=2,
                        leaf_keys=("blade", "blade_pale"), leaf_elev=(28, 52), leaf_droop=(0.6, 1.1)),
        "Mature": dict(tufts=1, tuft_len=0.16, tuft_key="tuft", culm=0.90, lean=(2, 7), leaves=2,
                       leaf_keys=("blade_turning", "blade"), leaf_elev=(25, 50), leaf_droop=(0.7, 1.2)),
        "Ripe": dict(tufts=1, tuft_len=0.15, tuft_key="tuft_dry", culm=0.95, lean=(4, 10), leaves=2,
                     leaf_keys=("blade_dry", "blade_turning"), leaf_elev=(15, 45), leaf_droop=(0.9, 1.5)),
    },
)


def build(kit):
    return G.build(kit, CONFIG)
