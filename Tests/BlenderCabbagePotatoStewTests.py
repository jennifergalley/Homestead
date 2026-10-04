"""Check original stew and spoon source, not art/grip/import acceptance.

Usage: Invoke-BlenderLive.ps1 -File Tests\\BlenderCabbagePotatoStewTests.py
Open the exported CabbagePotatoStewSource blend first.
"""
import importlib
import sys
from pathlib import Path

import bpy
from mathutils import Vector
from mathutils.bvhtree import BVHTree

sys.path.insert(0, str(Path(__file__).resolve().parent))
import BlenderPreparedFoodChecks as checks

checks = importlib.reload(checks)
MATERIAL_SCALES = {
    "M_CabbageStewEarthenware": {75, 1800},
    "M_CabbageStewBroth": {120},
    "M_CabbageStewMeadowHerbs": {650, 2200},
    "M_CabbageStewPotato": {310, 1900},
    "M_CabbageStewSpoonPotato": {310, 1900},
    "M_CabbageStewLeaf": {240, 1700},
    "M_CabbageStewSpoonLeaf": {240, 1700},
    "M_CabbageStewMapleSpoon": {720, 2300},
}


def sampled_container_clearance(obj: bpy.types.Object, prefix: str) -> None:
    container_slots = {index for index, slot in enumerate(obj.material_slots)
                       if slot.material.name.startswith(prefix)}
    assert len(container_slots) == 1
    faces = [tuple(face.vertices) for face in obj.data.polygons if face.material_index in container_slots]
    surface = BVHTree.FromPolygons([vertex.co for vertex in obj.data.vertices], faces)
    samples = {index for face in obj.data.polygons if face.material_index not in container_slots
               for index in face.vertices}
    for index in samples:
        point = obj.data.vertices[index].co
        hit, _, _, _ = surface.ray_cast(Vector((point.x, point.y, 1)), Vector((0, 0, -1)), 1)
        assert hit is not None and point.z + .00002 >= hit.z, (obj.name, "sampled container penetration", index)


def main() -> None:
    serving = checks.check_source_mesh("SM_CabbagePotatoStew", "CabbagePotatoStew",
                                      ((.175, .177), (.175, .177), (.060, .074)),
                                      38, (25000, 50000), MATERIAL_SCALES)
    assert serving["food_role"] == "serving"
    assert serving["food_potato_pieces"] == 6 and serving["food_cabbage_pieces"] == 12
    sampled_container_clearance(serving, "M_CabbageStewEarthenware")
    portion = checks.check_source_mesh("SM_CabbagePotatoStewPortion", "CabbagePotatoStew",
                                      ((.027, .032), (.181, .183), (.014, .030)),
                                      3, (6000, 15000), MATERIAL_SCALES)
    assert portion["food_role"] == "handheld_portion" and portion["food_grip_not_verified"]
    assert portion["food_eating_utensil"] == "original carved maple spoon"
    assert not any(slot.material.name.startswith("M_CabbageStewEarthenware")
                   for slot in portion.material_slots)
    sampled_container_clearance(portion, "M_CabbageStewMapleSpoon")
    wood_slots = {index for index, slot in enumerate(portion.material_slots)
                  if slot.material.name.startswith("M_CabbageStewMapleSpoon")}
    wood_vertices = {index for face in portion.data.polygons if face.material_index in wood_slots
                     for index in face.vertices}
    centre_y = min(portion.data.vertices[index].co.y for index in wood_vertices) + .0275
    spoon_surface = BVHTree.FromPolygons(
        [vertex.co for vertex in portion.data.vertices],
        [tuple(face.vertices) for face in portion.data.polygons if face.material_index in wood_slots])
    heights = []
    for x in (0, -.011, .011):
        hit, _, _, _ = spoon_surface.ray_cast(Vector((x, centre_y, 1)), Vector((0, 0, -1)), 1)
        assert hit is not None
        heights.append(hit.z)
    assert min(heights[1:]) - heights[0] > .0025, "Original spoon head must remain hollow"
    print("ORIGINAL_CABBAGE_STEW_SPOON_HOLLOW_PASS", min(heights[1:]) - heights[0], "m; grip still unverified")


if __name__ == "__main__":
    main()
