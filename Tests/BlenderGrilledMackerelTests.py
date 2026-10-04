"""Verify original grilled mackerel meal, not art/import/bake/eating acceptance.

Usage: Invoke-BlenderLive.ps1 -File Tests\\BlenderGrilledMackerelTests.py
Open the exported GrilledMackerelSource blend first.
"""
import importlib
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import BlenderPreparedFoodChecks as checks

checks = importlib.reload(checks)


def main() -> None:
    scales = {"M_GrilledMackerelPlatter": {55, 1600},
              "M_CookedMackerelFilletFlesh": {190, 50, 2200},
              "M_CookedMackerelFilletSkin": {110, 60, 1800},
              "M_CookedMackerelEdiblePieceFlesh": {190, 50, 2200},
              "M_CookedMackerelEdiblePieceSkin": {110, 60, 1800}}
    for index in range(2):
        scales["M_CookedMackerelBrokenPiece" + str(index) + "Flesh"] = {190, 50, 2200}
        scales["M_CookedMackerelBrokenPiece" + str(index) + "Skin"] = {110, 60, 1800}
    serving = checks.check_source_mesh("SM_GrilledMackerel", "GrilledMackerel",
                                      ((.207, .209), (.151, .153), (.014, .035)),
                                      4, (24000, 65000), scales)
    assert serving["food_role"] == "serving" and serving["food_fillets"] == 1
    assert serving["food_broken_pieces"] == 2
    checks.sampled_container_clearance(serving, "M_GrilledMackerelPlatter")
    portion = checks.check_source_mesh("SM_GrilledMackerelPortion", "GrilledMackerel",
                                      ((.018, .026), (.030, .036), (.006, .012)),
                                      1, (4500, 15000), scales)
    assert portion["food_role"] == "handheld_portion" and portion["food_grip_not_verified"]
    assert not any(slot.material.name.startswith("M_GrilledMackerelPlatter") for slot in portion.material_slots)


if __name__ == "__main__":
    main()
