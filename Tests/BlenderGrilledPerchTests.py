"""Verify original perch meal source, not art/import/bake/eating acceptance.

Usage: Invoke-BlenderLive.ps1 -File Tests\\BlenderGrilledPerchTests.py
Open the exported GrilledPerchSource blend first.
"""
import importlib
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import BlenderPreparedFoodChecks as checks

checks = importlib.reload(checks)
sys.path.insert(0, str(checks.PIPELINE))
import homestead_food_geometry as shapes

shapes = importlib.reload(shapes)


def check_invalid_fillet_inputs() -> None:
    valid = {"length": .1, "width": .03, "height": .01, "rows": 32,
             "sides": 80, "spacing_m": .0095, "depth_m": .00035}
    for field, value in (("length", 0), ("width", float("nan")), ("height", -.01),
                         ("rows", 3), ("sides", 11), ("spacing_m", 0),
                         ("depth_m", -.001), ("depth_m", float("inf"))):
        parameters = dict(valid)
        parameters[field] = value
        try:
            shapes.cooked_fillet(None, "InvalidCookedFish", 0, flesh_material=None,
                                 skin_material=None, seed=1, whole_fillet=True, **parameters)
        except ValueError as error:
            assert "finite closed loft" in str(error)
        else:
            raise AssertionError("Invalid cooked-fillet input accepted: " + field)


def main() -> None:
    check_invalid_fillet_inputs()
    scales = {"M_GrilledPerchPlatter": {55, 1600},
              "M_CookedPerchEdiblePieceFlesh": {160, 45, 2100},
              "M_CookedPerchEdiblePieceSkin": {85, 410, 1700}}
    for index in range(2):
        scales["M_CookedPerchFillet" + str(index) + "Flesh"] = {160, 45, 2100}
        scales["M_CookedPerchFillet" + str(index) + "Skin"] = {85, 410, 1700}
    serving = checks.check_source_mesh("SM_GrilledPerch", "GrilledPerch",
                                      ((.197, .199), (.143, .145), (.014, .035)),
                                      3, (24000, 65000), scales)
    assert serving["food_role"] == "serving" and serving["food_fillets"] == 2
    checks.sampled_container_clearance(serving, "M_GrilledPerchPlatter")
    portion = checks.check_source_mesh("SM_GrilledPerchPortion", "GrilledPerch",
                                      ((.018, .026), (.030, .036), (.006, .011)),
                                      1, (4500, 15000), scales)
    assert portion["food_role"] == "handheld_portion" and portion["food_grip_not_verified"]
    assert not any(slot.material.name.startswith("M_GrilledPerchPlatter") for slot in portion.material_slots)


if __name__ == "__main__":
    main()
