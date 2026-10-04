"""Verify original cooked-trout sources, not art/import/eating acceptance.

Usage: Invoke-BlenderLive.ps1 -File Tests\\BlenderGrilledTroutTests.py
Open the exported GrilledTroutSource blend first.
"""
import importlib
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import BlenderPreparedFoodChecks as checks

checks = importlib.reload(checks)


def main() -> None:
    scales = {"M_GrilledTroutPlatter": {55, 1600},
              "M_CookedTroutFilletFlesh": {160, 45, 2100},
              "M_CookedTroutFilletSkin": {75, 350, 1300},
              "M_CookedTroutEdiblePieceFlesh": {160, 45, 2100},
              "M_CookedTroutEdiblePieceSkin": {75, 350, 1300}}
    for index in range(3):
        scales["M_CookedTroutFlake" + str(index) + "Flesh"] = {160, 45, 2100}
    serving = checks.check_source_mesh("SM_GrilledTrout", "GrilledTrout",
                                      ((.219, .221), (.155, .157), (.015, .050)),
                                      5, (20000, 65000), scales)
    assert serving["food_role"] == "serving" and serving["food_fillets"] == 1
    assert serving["food_flake_count"] == 3
    checks.sampled_container_clearance(serving, "M_GrilledTroutPlatter")
    portion = checks.check_source_mesh("SM_GrilledTroutPortion", "GrilledTrout",
                                      ((.018, .028), (.033, .038), (.006, .012)),
                                      1, (4500, 15000), scales)
    assert portion["food_role"] == "handheld_portion" and portion["food_grip_not_verified"]
    assert not any(slot.material.name.startswith("M_GrilledTroutPlatter") for slot in portion.material_slots)


if __name__ == "__main__":
    main()
