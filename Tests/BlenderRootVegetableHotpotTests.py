"""Verify original hotpot source, not artistic/import/grip acceptance.

Usage: Invoke-BlenderLive.ps1 -File Tests\\BlenderRootVegetableHotpotTests.py
Open the exported RootVegetableHotpotSource blend first.
"""
import importlib
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import BlenderPreparedFoodChecks as checks

checks = importlib.reload(checks)
MATERIAL_SCALES = {
    "M_RootHotpotCrock": {75, 1800},
    "M_RootHotpotBroth": {120},
    "M_RootHotpotWildRoot": {180, 1250},
    "M_RootHotpotTurnip": {180, 1250},
    "M_RootHotpotSpoonWildRoot": {180, 1250},
    "M_RootHotpotSpoonTurnip": {180, 1250},
    "M_RootHotpotMeadowHerbs": {650, 2200},
    "M_RootHotpotMapleSpoon": {720, 2300},
}


def main() -> None:
    serving = checks.check_source_mesh("SM_RootVegetableHotpot", "RootVegetableHotpot",
                                      ((.191, .198), (.165, .167), (.057, .082)),
                                      44, (35000, 80000), MATERIAL_SCALES)
    assert serving["food_role"] == "serving"
    assert serving["food_root_pieces"] == 10 and serving["food_turnip_pieces"] == 6
    assert serving["food_herb_fragments"] == 24
    checks.sampled_container_clearance(serving, "M_RootHotpotCrock")
    portion = checks.check_source_mesh("SM_RootVegetableHotpotPortion", "RootVegetableHotpot",
                                      ((.025, .029), (.169, .171), (.012, .029)),
                                      5, (9000, 20000), MATERIAL_SCALES)
    assert portion["food_role"] == "handheld_portion" and portion["food_grip_not_verified"]
    assert portion["food_eating_utensil"] == "original generated 17cm carved maple spoon"
    assert not any(slot.material.name.startswith("M_RootHotpotCrock") for slot in portion.material_slots)
    checks.sampled_container_clearance(portion, "M_RootHotpotMapleSpoon")
    depth = checks.sampled_spoon_hollow(portion, "M_RootHotpotMapleSpoon", .170)
    print("ORIGINAL_ROOT_HOTPOT_SPOON_HOLLOW_PASS", depth, "m; grip still unverified")


if __name__ == "__main__":
    main()
