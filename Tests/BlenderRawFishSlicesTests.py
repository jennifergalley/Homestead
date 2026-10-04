"""Verify original raw-fish meal source, not art/import/grip or catch acceptance.

Usage: Invoke-BlenderLive.ps1 -File Tests\\BlenderRawFishSlicesTests.py
Open the exported RawFishSlicesSource blend first.
"""
import importlib
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import BlenderPreparedFoodChecks as checks

checks = importlib.reload(checks)
MATERIAL_SCALES = {
    "M_RawMackerelOvalPlate": {60, 1400},
}


def main() -> None:
    scales = dict(MATERIAL_SCALES)
    for index in range(6):
        scales["M_RawMackerelSlice" + str(index) + "Flesh"] = {220, 2200}
        scales["M_RawMackerelSlice" + str(index) + "Skin"] = {190, 3100}
    scales["M_RawMackerelEdibleSliceFlesh"] = {220, 2200}
    scales["M_RawMackerelEdibleSliceSkin"] = {190, 3100}
    serving = checks.check_source_mesh("SM_RawFishSlices", "RawFishSlices",
                                      ((.219, .221), (.163, .165), (.014, .029)),
                                      7, (30000, 65000), scales)
    assert serving["food_role"] == "serving" and serving["food_slice_count"] == 6
    checks.sampled_container_clearance(serving, "M_RawMackerelOvalPlate")
    portion = checks.check_source_mesh("SM_RawFishSlicesPortion", "RawFishSlices",
                                      ((.028, .036), (.013, .018), (.005, .009)),
                                      1, (4500, 12000), scales)
    assert portion["food_role"] == "handheld_portion" and portion["food_grip_not_verified"]
    assert not any(slot.material.name.startswith("M_RawMackerelOvalPlate") for slot in portion.material_slots)


if __name__ == "__main__":
    main()
