"""Check original bramble reduction source, not artistic/import/eating acceptance.

Usage: Invoke-BlenderLive.ps1 -File Tests\\BlenderBerryCompoteTests.py
Open the exported BerryCompoteSource blend first.
"""
import importlib
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import BlenderPreparedFoodChecks as checks

checks = importlib.reload(checks)
MATERIAL_SCALES = {
    "M_BerryReductionGlazedDish": {170, 2100},
    "M_BrambleReducedJuice": {480, 3200},
    "M_BrambleReductionFruit": {480, 3200},
    "M_BrambleReductionSpoonFruit": {480, 3200},
    "M_BerryReductionMapleSpoon": {720, 2300},
}


def main() -> None:
    serving = checks.check_source_mesh("SM_BerryCompote", "BerryCompote",
                                      ((.117, .119), (.117, .119), (.032, .045)),
                                      14, (35000, 60000), MATERIAL_SCALES)
    assert serving["food_role"] == "serving" and serving["food_fruit_pieces"] == 12
    checks.sampled_container_clearance(serving, "M_BerryReductionGlazedDish")
    portion = checks.check_source_mesh("SM_BerryCompotePortion", "BerryCompote",
                                      ((.022, .026), (.145, .147), (.009, .025)),
                                      2, (7000, 12000), MATERIAL_SCALES)
    assert portion["food_role"] == "handheld_portion" and portion["food_grip_not_verified"]
    assert portion["food_eating_utensil"] == "original generated 14.6cm carved maple spoon"
    assert not any(slot.material.name.startswith("M_BerryReductionGlazedDish")
                   for slot in portion.material_slots)
    checks.sampled_container_clearance(portion, "M_BerryReductionMapleSpoon")
    depth = checks.sampled_spoon_hollow(portion, "M_BerryReductionMapleSpoon", .146)
    print("ORIGINAL_BERRY_COMPOTE_SPOON_HOLLOW_PASS", depth, "m; grip still unverified")


if __name__ == "__main__":
    main()
