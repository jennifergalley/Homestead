"""Verify the original fried-laver source, not art/import/bake/eating acceptance.

Usage: blender -b Assets\\Props\\PreparedFood\\FriedLaverSourceProof\\FriedLaverSource.blend
       --python Tests\\BlenderFriedLaverTests.py   (or Invoke-BlenderLive.ps1 -File ... with it open)
"""
import importlib
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import BlenderPreparedFoodChecks as checks

checks = importlib.reload(checks)
BEACH = "homestead_beach_food.py"
LAVER_SCALES = {170, 45, 24, 70, 62, 18, 95, 30, 60, 700, 1100}


def main() -> None:
    scales = {"M_FriedLaverPearlware": {35, 1.0, 320}, "M_FriedLaverPiece": LAVER_SCALES,
              "M_FriedLaverSheets": LAVER_SCALES}
    # The plate, ten crumpled sheets and six crumbled flakes.
    serving = checks.check_source_mesh("SM_FriedLaver", "FriedLaver",
                                      ((.178, .182), (.178, .182), (.029, .035)),
                                      1 + 10 + 6, (65000, 78000), scales, BEACH, BEACH)
    assert serving["food_role"] == "serving"
    assert serving["food_laver_sheets"] == 10 and serving["food_laver_flakes"] == 6
    checks.sampled_container_clearance(serving, "M_FriedLaverPearlware")
    portion = checks.check_source_mesh("SM_FriedLaverPortion", "FriedLaver",
                                      ((.046, .054), (.032, .039), (.006, .009)),
                                      1, (4200, 5000), scales, BEACH, BEACH)
    assert portion["food_role"] == "handheld_portion" and portion["food_grip_not_verified"]
    print("FRIED_LAVER_SOURCE_PASS")


if __name__ == "__main__":
    main()
