"""Verify the original steamed-mussel source, not art/import/bake/eating acceptance.

Usage: blender -b Assets\\Props\\PreparedFood\\SteamedMusselsSourceProof\\SteamedMusselsSource.blend
       --python Tests\\BlenderSteamedMusselsTests.py   (or Invoke-BlenderLive.ps1 -File ... with it open)
"""
import importlib
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import BlenderPreparedFoodChecks as checks

checks = importlib.reload(checks)
BEACH = "homestead_beach_food.py"
# Node sockets store float32; 1.6 does not round-trip as a Python float.
SHELL_SCALES = {150, struct.unpack("f", struct.pack("f", 1.6))[0], 160, 95, 1900, 1.0}
NACRE_SCALES = {55, 1.0, 320, 120, 1500}
MEAT_SCALES = {150, 48, 600, 520}


def main() -> None:
    scales = {"M_SteamedMusselsSlipware": {48, 140, 90, 1700}, "M_SteamedMusselsLiquor": {140},
              "M_SteamedMusselShell": SHELL_SCALES, "M_SteamedMusselNacre": NACRE_SCALES,
              "M_SteamedMusselMeat": MEAT_SCALES,
              "M_SteamedMusselPortionShell": SHELL_SCALES, "M_SteamedMusselPortionNacre": NACRE_SCALES,
              "M_SteamedMusselPortionMeat": MEAT_SCALES}
    # Nine mussels of two valves and one meat each, plus the bowl and its liquor.
    serving = checks.check_source_mesh("SM_SteamedMussels", "SteamedMussels",
                                      ((.195, .199), (.195, .199), (.078, .088)),
                                      2 + 9 * 3, (70000, 80000), scales, BEACH, BEACH)
    assert serving["food_role"] == "serving" and serving["food_mussels"] == 9
    checks.sampled_container_clearance(serving, "M_SteamedMusselsSlipware")
    portion = checks.check_source_mesh("SM_SteamedMusselsPortion", "SteamedMussels",
                                      ((.058, .066), (.063, .071), (.016, .021)),
                                      3, (6000, 8000), scales, BEACH, BEACH)
    assert portion["food_role"] == "handheld_portion" and portion["food_grip_not_verified"]
    assert not any(slot.material.name.startswith("M_SteamedMusselsSlipware") for slot in portion.material_slots)
    print("STEAMED_MUSSELS_SOURCE_PASS")


if __name__ == "__main__":
    main()
