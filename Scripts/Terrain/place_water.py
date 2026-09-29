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

# River: the channel river_channel.py graded (its water surface and waterline half-width per layout
# point, a spring pool at the head, a ford at the drive, a shallow run over the cove beach into the
# shore wash). The ribbon's surface runs under the banks and rounds off at both ends; the controller
# reads its spline scale Y as the waterline. Not spatially loaded, so it's drawn (and the pail probe
# finds it) from anywhere on the estate. It stays tagged HomesteadWater; the sea never is.
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


import math

surface, halfw, end = L["riverSurface"], L["riverHalfWidth"], L["riverEnd"]
pts = L["river"][:end]
course = [unreal.Vector(x * 100, y * 100, z * 100) for (x, y), z in zip(pts, surface)]
widths = [w * 100 for w in halfw]
# The source is the pool centre: start the spline one pool radius (plus the overlap under the bank)
# upstream so the rounded cap closes round the far side of the pool, inside the head wall.
(x0, y0), (x1, y1) = pts[0], pts[1]
n = math.hypot(x0 - x1, y0 - y1)
ux, uy = (x0 - x1) / n, (y0 - y1) / n
cap = halfw[0] + 0.45
course.insert(0, unreal.Vector((x0 + ux * cap) * 100, (y0 + uy * cap) * 100, surface[0] * 100))
widths.insert(0, widths[0])

# The river's water: M_CreekWater's graph (Scripts/bootstrap_unreal.py) authored as its own material,
# whose vertex colour B turns riffles and the spring to white water. Created once; to re-author it,
# delete the asset with no level open that uses it, then run this again (clearing a material the
# renderer is using can crash the editor).
RIVER_MATERIAL = "/Game/SurvivalGame/Materials/M_EstateRiver"
if not unreal.EditorAssetLibrary.does_asset_exist(RIVER_MATERIAL):
    import importlib.util
    spec = importlib.util.spec_from_file_location(
        "hs_bootstrap", os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "bootstrap_unreal.py"))
    bootstrap = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(bootstrap)
    bootstrap.creek_water_material(name="M_EstateRiver")

river = existing.get("EstateRiver") or eas.spawn_actor_from_class(unreal.HomesteadWaterRibbon, course[0])
river.set_actor_label("EstateRiver")
river.set_folder_path("Water")
river.set_editor_property("is_spatially_loaded", False)
river.modify()  # set_course alone doesn't dirty the actor package, and PIE streams the saved one
river.set_editor_property("bank_overlap", 45.0)
river.set_editor_property("start_cap", cap * 100.0)
river.set_editor_property("end_cap", 600.0)
river.set_editor_property("material", unreal.load_asset(RIVER_MATERIAL))
river.set_course(course, widths)

# The spring: granite stones set into the head wall round the pool, a couple half-sunk in the rill.
SPRING_STONES = [
    # (mesh, angle from upstream in degrees, distance from the pool centre m, scale, footprint radius m, yaw)
    ("GraniteBoulderJointed/SM_GraniteBoulderJointed", 0.0, 3.0, 0.5, 0.8, 15.0),
    ("GraniteBoulderLow/SM_GraniteBoulderLow", 58.0, 2.5, 0.85, 0.6, 75.0),
    ("GraniteBoulderLoaf/SM_GraniteBoulderLoaf", -55.0, 2.6, 0.8, 0.5, 140.0),
    ("GraniteBoulderLow/SM_GraniteBoulderLow", -100.0, 2.3, 0.55, 0.4, 210.0),
    ("GraniteBoulderLoaf/SM_GraniteBoulderLoaf", 105.0, 2.2, 0.5, 0.35, 300.0),
    ("GraniteCobbles/SM_GraniteCobbles", 180.0, 2.4, 1.0, 0.45, 30.0),
    ("GraniteCobbles/SM_GraniteCobbles", 178.0, 6.5, 0.9, 0.4, 160.0),
    ("GraniteCobbles/SM_GraniteCobbles", 25.0, 1.7, 0.8, 0.35, 250.0),
]
# The stones are spatially loaded: load the spring's cells first, or a re-run spawns a second set.
try:
    wp = unreal.WorldPartitionBlueprintLibrary
    box = unreal.Box(unreal.Vector((x0 - 12) * 100, (y0 - 12) * 100, -1e5), unreal.Vector((x0 + 12) * 100, (y0 + 12) * 100, 1e5))
    found = wp.get_intersecting_actor_descs(box)
    descs = found[1] if isinstance(found, tuple) else found
    wp.load_actors([desc.guid for desc in descs])
except Exception as error:  # noqa: BLE001 - report and carry on; check the outliner for duplicates
    print("warning: couldn't load the spring's cells:", error)
stones = {a.get_actor_label(): a for a in eas.get_all_level_actors() if a.get_actor_label().startswith("EstateSpringStone")}
base = math.degrees(math.atan2(uy, ux))
for index, (mesh, angle, dist, scale, radius, yaw) in enumerate(SPRING_STONES):
    a = math.radians(base + angle)
    sx, sy = x0 + math.cos(a) * dist, y0 + math.sin(a) * dist
    # Seat it on the lowest ground under its footprint, a little sunk, so no edge floats on the head wall.
    under = [ground(sx + math.cos(t) * radius, sy + math.sin(t) * radius) for t in [k * math.pi / 4 for k in range(8)]]
    sz = min(under + [ground(sx, sy)]) - 0.06
    label = f"EstateSpringStone{index + 1}"
    stone = stones.get(label) or eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(sx * 100, sy * 100, sz * 100))
    stone.set_actor_label(label)
    stone.set_folder_path("Water/Spring")
    stone.modify()
    stone.static_mesh_component.set_static_mesh(unreal.load_asset(f"/Game/SurvivalGame/Environment/Props/{mesh}"))
    stone.set_actor_location(unreal.Vector(sx * 100, sy * 100, sz * 100), False, False)
    stone.set_actor_rotation(unreal.Rotator(0, 0, base + yaw), False)
    stone.set_actor_scale3d(unreal.Vector(scale, scale, scale))
print("sea + river placed:", len(course), "river points, mouth at", course[-1], "; spring at", course[1])