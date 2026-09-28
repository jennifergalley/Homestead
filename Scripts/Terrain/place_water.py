"""Place the Estate map's water: the Single Layer Water sea at datum and the estate river.

Run inside the editor (Estate level loaded, PIE stopped). Idempotent: updates the labelled actors in place.
The sea is SM_EstateOcean with M_EstateOcean, authored by bake_ocean.py + build_ocean.py; without
them it falls back to a scaled engine plane.
"""
import json
import os
import unreal

LAYOUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "estate_layout.json")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
existing = {a.get_actor_label(): a for a in eas.get_all_level_actors() if a.get_actor_label() in ("EstateSea", "EstateRiver")}

L = json.load(open(LAYOUT))
OCEAN_MESH = "/Game/SurvivalGame/Estate/Water/SM_EstateOcean"
MATERIAL_SEA = "/Game/SurvivalGame/Estate/Water/MI_EstateOcean"
if not unreal.EditorAssetLibrary.does_asset_exist(MATERIAL_SEA):
    MATERIAL_SEA = "/Game/SurvivalGame/Materials/M_CreekWater"

# Sea: the ocean mesh at datum (world centimetres, so no scale); land above it hides it.
# Existing actors are updated in place so their World Partition actor files keep their names.
sea = existing.get("EstateSea") or eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, 0))
sea.set_actor_label("EstateSea")
sea.set_folder_path("Water")
sea.set_actor_location(unreal.Vector(0, 0, 0), False, False)
sea.set_actor_rotation(unreal.Rotator(0, 0, 0), False)
smc = sea.static_mesh_component
if unreal.EditorAssetLibrary.does_asset_exist(OCEAN_MESH):
    smc.set_static_mesh(unreal.load_asset(OCEAN_MESH))
    smc.set_world_scale3d(unreal.Vector(1, 1, 1))
else:
    smc.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Plane"))
    smc.set_world_scale3d(unreal.Vector(4200, 4200, 1))
smc.set_material(0, unreal.load_asset(MATERIAL_SEA))
smc.set_editor_property("cast_shadow", False)
smc.set_editor_property("affect_distance_field_lighting", False)
smc.set_collision_profile_name("NoCollision")
sea.set_editor_property("is_spatially_loaded", False)
sea.tags = ["HomesteadSea"]  # never HomesteadWater: the sea must not refill the pail

# River: surface 0.3 m below the carved banks, i.e. 0.6 m above the channel bed. The channel isn't
# carved across the cove beach, so the stream soaks away into the sand there: the ribbon ends where
# its surface would meet the beach, dipping under the sand a few metres on. That also keeps the ribbon
# (and its HomesteadWater tag) off salt water.
from array import array

HEIGHTFIELD = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "Content", "SurvivalGame", "Estate", "Runtime", "EstateHeightfield.r16")
hf = array("H")
with open(HEIGHTFIELD, "rb") as fh:
    hf.frombytes(fh.read())
HN = int(round(len(hf) ** 0.5))


def ground(x, y):
    """Authored terrain height in metres at game-frame metres (x north, y east), bilinear."""
    fx, fy = x + (HN - 1) / 2, y + (HN - 1) / 2
    i, j = int(fx), int(fy)
    tx, ty = fx - i, fy - j
    v = lambda a, b: (hf[b * HN + a] - 32768) / 128.0
    return (v(i, j) * (1 - tx) * (1 - ty) + v(i + 1, j) * tx * (1 - ty)
            + v(i, j + 1) * (1 - tx) * ty + v(i + 1, j + 1) * tx * ty)


pts, beds = L["river"], L["riverBed"]
course, widths = [], []
last = None
for i in range(0, len(pts), 3):
    x, y = pts[i]
    g = ground(x, y)
    z = beds[i] + 0.6
    if (z < g + 0.05 or g < 0.0) and last:
        # Surface meets the ground between the last point and this one: end a few metres past it.
        px, py, pz, pg = last
        t = min(max((pz - pg - 0.05) / max((pz - pg) - (z - g), 1e-3), 0.0), 1.0)
        dx, dy = x - px, y - py
        n = max((dx * dx + dy * dy) ** 0.5, 1e-3)
        cx, cy = px + dx * t + dx / n * 3.0, py + dy * t + dy / n * 3.0
        course.append(unreal.Vector(cx * 100, cy * 100, (ground(cx, cy) - 0.3) * 100))
        widths.append(330.0)
        break
    course.append(unreal.Vector(x * 100, y * 100, max(z, 0.05) * 100))
    widths.append(330.0)
    last = (x, y, z, g)
river = existing.get("EstateRiver") or eas.spawn_actor_from_class(unreal.HomesteadWaterRibbon, course[0])
river.set_actor_label("EstateRiver")
river.set_folder_path("Water")
river.modify()  # set_course alone doesn't dirty the actor package, and PIE streams the saved one
river.set_course(course, widths)
print("sea + river placed:", len(course), "river points, mouth at", course[-1])