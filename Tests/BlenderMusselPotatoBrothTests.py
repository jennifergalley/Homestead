"""Verify the original mussel, potato and laver broth source, not art/import/bake/eating acceptance.

Usage: blender -b Assets\\Props\\PreparedFood\\MusselPotatoBrothSourceProof\\MusselPotatoBrothSource.blend
       --python Tests\\BlenderMusselPotatoBrothTests.py   (or Invoke-BlenderLive.ps1 -File ... with it open)
"""
import importlib
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import BlenderPreparedFoodChecks as checks

checks = importlib.reload(checks)
BEACH = "homestead_beach_food.py"
SHADERS = (BEACH, "homestead_food_materials.py")
# Node sockets store float32; 1.6 does not round-trip as a Python float.
SHELL_SCALES = {150, struct.unpack("f", struct.pack("f", 1.6))[0], 160, 95, 1900, 1.0}
NACRE_SCALES = {55, 1.0, 320, 120, 1500}
MEAT_SCALES = {150, 48, 600, 520}
BROTH_SCALES = {85, 60}
POTATO_SCALES = {280, 70, 1700}
LAVER_SCALES = {210, 1400}


def main() -> None:
    scales = {"M_MusselBrothCreamware": {40, 30, 160, 900}, "M_MusselBrothSpoonful": BROTH_SCALES,
              "M_MusselBrothLiquid": BROTH_SCALES, "M_MusselBrothSpoonPotato": POTATO_SCALES,
              "M_MusselBrothPotato": POTATO_SCALES, "M_MusselBrothSpoonLaver": LAVER_SCALES,
              "M_MusselBrothLaver": LAVER_SCALES, "M_MusselBrothShell": SHELL_SCALES,
              "M_MusselBrothNacre": NACRE_SCALES, "M_MusselBrothSpoonMeat": MEAT_SCALES,
              "M_MusselBrothMeat": MEAT_SCALES, "M_MusselBrothMapleSpoon": {720, 2300}}
    # Bowl, broth, six laver ribbons, six potato chunks, three loose meats, three open mussels (3 parts each).
    serving = checks.check_source_mesh("SM_MusselPotatoBroth", "MusselPotatoBroth",
                                      ((.157, .161), (.157, .161), (.070, .080)),
                                      2 + 6 + 6 + 3 + 3 * 3, (52000, 64000), scales, SHADERS, BEACH)
    assert serving["food_role"] == "serving"
    assert serving["food_potato_pieces"] == 6 and serving["food_mussels_in_shell"] == 3
    assert serving["food_loose_meats"] == 3 and serving["food_laver_ribbons"] == 6
    checks.sampled_container_clearance(serving, "M_MusselBrothCreamware")
    portion = checks.check_source_mesh("SM_MusselPotatoBrothPortion", "MusselPotatoBroth",
                                      ((.024, .029), (.168, .172), (.013, .017)),
                                      5, (10500, 12800), scales, SHADERS, BEACH)
    assert portion["food_role"] == "handheld_portion" and portion["food_grip_not_verified"]
    checks.sampled_spoon_hollow(portion, "M_MusselBrothMapleSpoon", .170)
    spoon = {i for i, slot in enumerate(portion.material_slots) if slot.material.name == "M_MusselBrothMapleSpoon"}
    assert len(spoon) == 1 and any(face.material_index not in spoon for face in portion.data.polygons)
    print("MUSSEL_POTATO_BROTH_SOURCE_PASS")


if __name__ == "__main__":
    main()
