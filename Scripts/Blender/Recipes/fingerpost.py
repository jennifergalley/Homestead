"""Fingerposts at the cove (add-cove-route-kit): "To the cove" at the manor's front door (yaw 159) and at the head of
the cliff steps (yaw 153), and the "To the mine" arm that hangs under the cove arm on the same post.

The look (post, painted board, lettering, sizes) lives in homestead_signage.py, shared by every fingerpost: edit
that to restyle them all, and see its docstring to add a new one.

Real-object research: estate and parish fingerposts were oak, a square post about 5 in (12.5 cm) and 7 ft (2.1 m)
high with a capped finial, and a flat arm about 1 in thick morticed through the post near the top, its outer end
cut to a point, lettered in black on both faces. Weathering: silver-grey oak post with dark checks, the board's paint
chalky and worn.

Pivot at the post's foot on the ground; the arm points along +X (Water sets the yaw), 1.70 m up. The second mesh,
SM_Fingerpost_ToTheMineArm, is the "To the mine" arm alone, 1.43 m up on the same pivot; HomesteadWorldMine hangs it
on the cove post, yawed toward the mine.
Lettering: Abril Fatface (OFL). Everything else is original procedural geometry and materials.
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from homestead_signage import Arm, fingerpost  # noqa: E402

NAME = "Fingerpost"
DESCRIPTION = ("Cove fingerpost 'To the cove' and the 'To the mine' arm alone (original; Victorian lettering in "
               "Abril Fatface, OFL). Pivot = post foot; the arm points +X.")
COLLISION = "box"
TRIANGLE_BUDGET = 30000
PROVENANCE = ("Original project-authored procedural geometry and materials. Lettering: Abril Fatface, "
              "TypeTogether, SIL Open Font License 1.1 (Assets/Fonts/AbrilFatface/OFL.txt).")
BAKE = {"size": 2048, "samples": 96, "maps": ("basecolor", "roughness", "normal", "ao")}
BEAUTY = {"pose": (0, 0, -20), "focus": (0.35, 0.0, 1.70)}
REPORT = {"pivot": "foot of the post on the ground; the arm points along +X", "arm_height_m": 1.70,
          "text": "To the cove", "mine_arm_height_m": 1.43,
          "yaws_deg": {"front door": 159, "head of the cliff steps": 153}}

COVE_Z = 1.70
MINE_Z = 1.43   # one board below the cove's; matches SignArmHeightCm in HomesteadWorldMine.cpp


def build(kit):
    return [
        fingerpost(kit, "SM_Fingerpost_ToTheCove", [Arm("To the cove", COVE_Z)]),
        fingerpost(kit, "SM_Fingerpost_ToTheMineArm", [Arm("To the mine", MINE_Z)], with_post=False),
    ]
