"""The estate's bramble overgrowth in spring: thin bramble, a dense thicket and an old bramble bank,
cleared with the billhook at worn, iron and steel tier. Same plant as ``blackberry_bramble.py``
(Rubus fruticosus agg. in Cornwall; the canes, leaves, prickles and atlas are that recipe's), with
its fruiting switched off: in spring the first-year canes are leafing out and the floricanes have
not flowered yet (flowers from late May, fruit late summer into early autumn), so no flowers or
berries are emitted. The fruiting look arrives with round 2's seasons, from the fruiting meshes.

- Thin bramble: a young clump of a few arching canes, about 0.75 m tall and 1.2 m across; the kind
  that chokes a doorway after a few untended years.
- Thicket: the medium mound (1.2 m) with more canes.
- Bank: an old, wide bank, about 2 m tall and 3.5 m across, with many dead canes inside.
"""
import importlib.util
from pathlib import Path

_spec = importlib.util.spec_from_file_location("homestead_blackberry", Path(__file__).with_name("blackberry_bramble.py"))
bramble = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(bramble)
F = bramble.F

NAME = "BrambleOvergrowth"
DESCRIPTION = ("Spring bramble overgrowth for estate clearing: thin clump, thicket and old bank of "
               "blackberry canes without flowers or fruit (original, from the BlackberryBramble recipe).")
# Walk-through for now: the world presents resources without collision, and a blocking ring at the`r`n# doorway could trap her before she has a billhook.`r`nCOLLISION = "none"
TRIANGLE_BUDGET = 60000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
BEAUTY = {"pose": (0, 0, 0), "focus": (0.05, -0.4, 0.45),
          "meshes": {"SM_BrambleThicket": {"focus": (0.10, -0.55, 0.80)},
                     "SM_BrambleBank": {"focus": (0.2, -1.2, 1.30)}}}
REPORT = dict(bramble.REPORT)

THIN = dict(bramble.MEDIUM, name="SM_BrambleThin", height=0.78, reach=0.62, canes=14, dead=1, laterals=3,
            crowns=[(0.0, 0.0), (0.09, 0.06)], radius=0.0068, full=0.3, leaf_scale=1.05, cane_length=1.05,
            suckers=3, cane_step=0.06, fruit=False, seed=bramble.SEED + 211)
THICKET = dict(bramble.MEDIUM, name="SM_BrambleThicket", canes=42, dead=8, laterals=10, fruit=False,
               seed=bramble.SEED + 223)
BANK = dict(bramble.LARGE, name="SM_BrambleBank", reach=1.75, canes=64, dead=16, laterals=16,
            crowns=[(0.0, 0.0), (0.55, 0.25), (-0.5, 0.3), (0.25, -0.5), (-0.4, -0.35), (0.8, -0.2), (-0.85, 0.05)],
            suckers=24, fruit=False, seed=bramble.SEED + 239)


def build(kit):
    atlas = bramble.paint_atlas()
    material = atlas.material()
    meshes = []
    for spec in (THIN, THICKET, BANK):
        desc = bramble.describe(spec)
        leaves = sum(len(c["leaves"]) for c in desc["canes"])
        print(f"HOMESTEAD_PLANT {spec['name']} canes={len(desc['canes'])} leaves={leaves}")
        batches = [bramble.emit(desc, atlas, lod) for lod in range(3)]
        objs = F.finish_lods(kit, batches, spec["name"], material)
        print("HOMESTEAD_LODS", F.lod_report(objs))
        meshes.extend(objs)
    return meshes
