"""Check original cooked broad bean source, never art/import/grip acceptance.

Usage: Invoke-BlenderLive.ps1 -File Tests\\BlenderHerbedBroadBeanTests.py
Open the exported HerbedBroadBeansSource blend first.
"""
import importlib
import sys
from pathlib import Path

from mathutils import Vector
from mathutils.bvhtree import BVHTree

sys.path.insert(0, str(Path(__file__).resolve().parent))
import BlenderPreparedFoodChecks as checks

checks = importlib.reload(checks)

ITEM = "HerbedBroadBeans"
MATERIAL_SCALES = {
    "M_OriginalBroadBeanEarthenware": {75, 1800},
    "M_OriginalBroadBeanMeadowHerbs": {650, 2200},
    "M_OriginalCookedBroadBean": {180, 1100, 2600},
    "M_EdibleHerbedBroadBean": {180, 1100, 2600},
}


def main() -> None:
    serving = checks.check_source_mesh("SM_HerbedBroadBeans", ITEM,
                                ((.133, .135), (.133, .135), (.033, .062)),
                                61, (50000, 65000), MATERIAL_SCALES)
    assert serving["food_role"] == "serving"
    assert serving["food_bean_count"] == 24 and serving["food_herb_flakes"] == 36
    bowl_slots = {index for index, slot in enumerate(serving.material_slots)
                  if slot.material.name.startswith("M_OriginalBroadBeanEarthenware")}
    bowl_faces = [tuple(face.vertices) for face in serving.data.polygons
                  if face.material_index in bowl_slots]
    bowl_surface = BVHTree.FromPolygons([vertex.co for vertex in serving.data.vertices], bowl_faces)
    food_vertices = {index for face in serving.data.polygons if face.material_index not in bowl_slots
                     for index in face.vertices}
    for index in food_vertices:
        point = serving.data.vertices[index].co
        hit, _, _, _ = bowl_surface.ray_cast(Vector((point.x, point.y, 1)), Vector((0, 0, -1)), 1.0)
        assert hit is not None and point.z + .00002 >= hit.z, "Sampled food/bowl penetration"
    portion = checks.check_source_mesh("SM_HerbedBroadBeansPortion", ITEM,
                                ((.013, .016), (.021, .026), (.007, .009)),
                                1, (2400, 2500), MATERIAL_SCALES)
    assert portion["food_role"] == "handheld_portion"
    assert len(portion.material_slots) == 1


if __name__ == "__main__":
    main()
