"""Fingerposts for the public road (replace-road-sign-teleport): "To the village", "To the manor" and the two-armed
"Village / Manor" at the gateway. They only point the way: walking up to one no longer teleports her anywhere.

The look lives in homestead_signage.py, shared by every fingerpost (edit that to restyle them all). This recipe is
the pattern for a new fingerpost: one `fingerpost(kit, mesh name, [Arm(words, height, turn)])` per mesh.

Pivot at the post's foot on the ground; the (upper) arm points along +X (HomesteadRoadSign sets the yaw); a second
arm with turn=-1 points along -X, and the upper arm is the nearer destination.
Lettering: Abril Fatface (OFL). Everything else is original procedural geometry and materials.
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from homestead_signage import Arm, fingerpost  # noqa: E402

NAME = "FingerpostRoad"
DESCRIPTION = ("Road fingerposts: 'To the village', 'To the manor' and a two-armed 'Village / Manor' (original; "
               "Victorian lettering in Abril Fatface, OFL). Pivot = post foot; the (upper) arm points +X, the "
               "second arm -X.")
COLLISION = "box"
TRIANGLE_BUDGET = 40000
PROVENANCE = ("Original project-authored procedural geometry and materials. Lettering: Abril Fatface, "
              "TypeTogether, SIL Open Font License 1.1 (Assets/Fonts/AbrilFatface/OFL.txt).")
BAKE = {"size": 2048, "samples": 96, "maps": ("basecolor", "roughness", "normal", "ao")}
BEAUTY = {"pose": (0, 0, -20), "focus": (0.35, 0.0, 1.70)}
REPORT = {"pivot": "foot of the post on the ground; the upper arm points along +X", "arm_height_m": 1.70,
          "texts": ["To the village", "To the manor", "Village", "Manor"]}

UPPER_Z = 1.70
LOWER_Z = 1.40


def build(kit):
    return [
        fingerpost(kit, "SM_Fingerpost_ToTheVillage", [Arm("To the village", UPPER_Z)]),
        fingerpost(kit, "SM_Fingerpost_ToTheManor", [Arm("To the manor", UPPER_Z)]),
        fingerpost(kit, "SM_Fingerpost_VillageManor", [Arm("Village", UPPER_Z), Arm("Manor", LOWER_Z, turn=-1.0)]),
    ]
