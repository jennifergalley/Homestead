"""Place the town square's blockout massing in the Estate level (run with run_python, Estate open, not in PIE).

Each building is an AHomesteadTownBuilding level actor in outliner folder `Town`, labelled
`Town_Blockout_<name>`, snapped to the estate ground by the actor itself. The buildings come from
"town" in Scripts/Terrain/estate_layout.json (Scripts/Terrain/town_layout.py lays the square out: a
60 x 45 m open square, terraces and cottages with 3-6 m side lanes, the town street). Rerunning updates
the existing actors by label and deletes blockouts that are no longer in the layout, so it's safe to
re-run after town_layout.py. The general store stands in the middle of the square's east side at the
GeneralStoreDoor anchor and is spawned by the game, not here. The EastHouse slot, north of the store,
is where the seedsman's shop (Tregear's, parked branch) goes. Only the saved external-actor packages of
these buildings are written.
"""
import json
import os

import unreal

EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ROOF = {"RidgeAlongStreet": unreal.TownRoof.RIDGE_ALONG_STREET, "GableToStreet": unreal.TownRoof.GABLE_TO_STREET}
WALL = {"Rubble": unreal.TownWall.RUBBLE, "Ashlar": unreal.TownWall.ASHLAR, "Render": unreal.TownWall.RENDER}
LAYOUT = os.path.join(unreal.Paths.project_dir(), "Scripts", "Terrain", "estate_layout.json")


def c(rgb):
    return unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0)


def buildings():
    town = json.load(open(LAYOUT)).get("town")
    if not town:
        raise RuntimeError("estate_layout.json has no \"town\": run Scripts/Terrain/town_layout.py first.")
    return town["buildings"]


def load_town():
    """World Partition keeps unloaded actors out of get_all_level_actors, so load every town building
    first; otherwise a rerun would spawn duplicates beside the saved ones (from the parked Seedsman fix)."""
    library = unreal.WorldPartitionBlueprintLibrary
    guids = [d.get_editor_property("guid") for d in library.get_actor_descs()
             if str(d.get_editor_property("label")).startswith("Town_Blockout_")]
    if guids:
        library.load_actors(guids)
    return len(guids)


def existing():
    found = {}
    for actor in EAS.get_all_level_actors():
        if isinstance(actor, unreal.HomesteadTownBuilding):
            found[actor.get_actor_label()] = actor
    return found


def main():
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    if not world or world.get_name() != "Estate":
        raise RuntimeError("Open the Estate level (not PIE) before placing the town massing.")
    load_town()
    have = existing()
    packages = []
    wanted = set()
    for row in buildings():
        name, x, y, yaw = row["name"], row["x"] * 100.0, row["y"] * 100.0, row["yaw"]
        label = f"Town_Blockout_{name}"
        wanted.add(label)
        location = unreal.Vector(x, y, 9150.0)
        rotation = unreal.Rotator(0.0, 0.0, yaw)
        actor = have.get(label)
        if actor is None:
            actor = EAS.spawn_actor_from_class(unreal.HomesteadTownBuilding, location, rotation)
            actor.set_actor_label(label)
        else:
            actor.set_actor_location_and_rotation(location, rotation, False, True)
        actor.set_folder_path("Town")
        for key, value in (
            ("width", float(row["width"]) * 100.0), ("depth", float(row["depth"]) * 100.0), ("storeys", row["storeys"]),
            ("storey_height", float(row["storeyHeight"])), ("roof_pitch", float(row["pitch"])), ("roof", ROOF[row["roof"]]),
            ("wall", WALL[row["wall"]]), ("wall_tint", c(row["tint"])), ("chimneys", row["chimneys"]),
            ("door_at", float(row["doorAt"])), ("shopfront", row["shopfront"]), ("sign_color", c(row["sign"])),
            ("side_windows", row["sideWindows"]), ("snap_to_ground", True),
        ):
            actor.set_editor_property(key, value)
        packages.append(actor.get_outermost())
    # Blockouts the layout no longer has (the old packed square's) go; their external-actor files go with them.
    for label, actor in have.items():
        if label.startswith("Town_Blockout_") and label not in wanted:
            packages.append(actor.get_outermost())
            EAS.destroy_actor(actor)
    # The `Town` outliner folder is its own external object the first time it's used.
    packages += [p for p in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
                 if "__ExternalObjects__" in p.get_name()]
    unreal.EditorLoadingAndSavingUtils.save_packages(packages, False)
    unreal.log(f"TOWN_MASSING_PLACED {len(packages)}")


main()
