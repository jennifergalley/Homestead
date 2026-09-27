"""Place the Estate map's water: one Single Layer Water sea plane at datum and the estate river.

Run inside the editor (Estate level loaded, PIE stopped). Idempotent: removes earlier copies by label.
"""
import json
import unreal

LAYOUT = r"E:\Repos\copilot-worktrees\SurvivalGame\jennifergalley-cautious-pancake\Scripts\Terrain\estate_layout.json"
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in eas.get_all_level_actors():
    if a.get_actor_label() in ("EstateSea", "EstateRiver"):
        eas.destroy_actor(a)

L = json.load(open(LAYOUT))
MATERIAL_SEA = "/Game/SurvivalGame/Estate/Water/MI_EstateSea"
if not unreal.EditorAssetLibrary.does_asset_exist(MATERIAL_SEA):
    MATERIAL_SEA = "/Game/SurvivalGame/Materials/M_CreekWater"

# Sea: one plane over the whole map at datum; land above it hides it.
sea = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, 0))
sea.set_actor_label("EstateSea")
sea.set_folder_path("Water")
smc = sea.static_mesh_component
smc.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Plane"))
smc.set_world_scale3d(unreal.Vector(4200, 4200, 1))
smc.set_material(0, unreal.load_asset(MATERIAL_SEA))
smc.set_editor_property("cast_shadow", False)
smc.set_collision_profile_name("NoCollision")
sea.tags = ["HomesteadSea"]

# River: surface 0.3 m below the carved banks, i.e. 0.6 m above the channel bed.
pts, beds = L["river"], L["riverBed"]
course, widths = [], []
for i in range(0, len(pts), 3):
    x, y = pts[i]
    z = max(beds[i] + 0.6, 0.05)
    course.append(unreal.Vector(x * 100, y * 100, z * 100))
    widths.append(330.0)
river = eas.spawn_actor_from_class(unreal.HomesteadWaterRibbon, course[0])
river.set_actor_label("EstateRiver")
river.set_folder_path("Water")
river.set_course(course, widths)
print("sea + river placed:", len(course), "river points")
