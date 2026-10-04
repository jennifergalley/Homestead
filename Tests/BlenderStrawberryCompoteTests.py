"""Check original cooked strawberry source, never artistic/import/grip acceptance.

Usage: Invoke-BlenderLive.ps1 -File Tests\\BlenderStrawberryCompoteTests.py
Open the exported StrawberryCompoteSource blend first.
"""
import importlib
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import BlenderPreparedFoodChecks as checks

checks = importlib.reload(checks)
MATERIAL_SCALES = {
    "M_StrawberryReductionGlazedDish": {170, 2100},
    "M_StrawberryReducedJuice": {450, 2200},
    "M_StrawberryReductionHalf": {450, 2200},
    "M_StrawberryReductionSpoonHalf": {450, 2200},
    "M_StrawberryCompoteAchene": {1700},
    "M_StrawberryReductionMapleSpoon": {720, 2300},
}


def main() -> None:
    serving = checks.check_source_mesh("SM_StrawberryCompote", "StrawberryCompote",
                                      ((.131, .133), (.131, .133), (.036, .051)),
                                      170, (35000, 60000), MATERIAL_SCALES)
    assert serving["food_role"] == "serving"
    assert serving["food_fruit_halves"] == 8 and serving["food_achenes"] == 160
    checks.sampled_container_clearance(serving, "M_StrawberryReductionGlazedDish")
    portion = checks.check_source_mesh("SM_StrawberryCompotePortion", "StrawberryCompote",
                                      ((.023, .027), (.151, .153), (.009, .025)),
                                      22, (9000, 15000), MATERIAL_SCALES)
    assert portion["food_role"] == "handheld_portion" and portion["food_grip_not_verified"]
    assert portion["food_eating_utensil"] == "original generated 15.2cm carved maple spoon"
    assert not any(slot.material.name.startswith("M_StrawberryReductionGlazedDish")
                   for slot in portion.material_slots)
    checks.sampled_container_clearance(portion, "M_StrawberryReductionMapleSpoon")
    depth = checks.sampled_spoon_hollow(portion, "M_StrawberryReductionMapleSpoon", .152)
    print("ORIGINAL_STRAWBERRY_COMPOTE_SPOON_HOLLOW_PASS", depth, "m; grip still unverified")


if __name__ == "__main__":
    main()
