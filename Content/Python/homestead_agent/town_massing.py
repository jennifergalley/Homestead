"""Place the town square's blockout massing in the Estate level (run with run_python, Estate open, not in PIE).

Each building is an AHomesteadTownBuilding level actor in outliner folder `Town`, labelled
`Town_Blockout_<name>`, snapped to the estate ground by the actor itself. Rerunning updates the
existing actors by label, so it's safe to tweak a row and run it again. The general store stands on
the north side at the GeneralStoreDoor anchor (-54000, 117000), and Tregear's (the seedsman) on the east
side at the SeedsmanDoor anchor (-52050, 116000); both are spawned by the game, not here. Rows listed in
REMOVED are deleted from the level (the east house made way for Tregear's).
Only the saved external-actor packages of these buildings are written.
"""
import unreal

EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
RIDGE = unreal.TownRoof.RIDGE_ALONG_STREET
GABLE = unreal.TownRoof.GABLE_TO_STREET
RUBBLE, ASHLAR, RENDER = unreal.TownWall.RUBBLE, unreal.TownWall.ASHLAR, unreal.TownWall.RENDER


def c(r, g, b):
    return unreal.LinearColor(r, g, b, 1.0)


# Frontages: north y=116950 (yaw 90, in line with the store front), south y=113500 (yaw -90),
# west x=-56000 (yaw 180), east x=-52000 (yaw 0). The south-east corner stays open for the road.
BUILDINGS = [
    # name, x, y, yaw, width, depth, storeys, storey height, pitch, roof, wall, wall tint, chimneys,
    # door at, shopfront, sign colour, side windows
    ("Draper", -55010, 116950, 90, 1000, 800, 2, 290, 40, RIDGE, RUBBLE, c(1, 1, 1), 2, 0.6, True, c(0.16, 0.05, 0.08), False),
    ("CornerHouse", -55995, 116950, 90, 900, 700, 2, 300, 42, GABLE, RENDER, c(1.0, 0.97, 0.9), -1, -0.5, False, c(0.05, 0.05, 0.05), True),
    ("Baker", -53040, 116950, 90, 900, 750, 2, 280, 45, GABLE, RENDER, c(0.98, 0.97, 0.93), -1, 0.55, True, c(0.05, 0.08, 0.16), False),
    ("NorthCottage", -52180, 116950, 90, 780, 650, 1, 290, 50, RIDGE, RUBBLE, c(0.95, 0.93, 0.9), 1, -0.4, False, c(0.1, 0.07, 0.03), True),
    ("Ironmonger", -56000, 116150, 180, 1100, 800, 2, 300, 38, RIDGE, RENDER, c(1.0, 0.92, 0.85), 2, -0.5, True, c(0.20, 0.04, 0.04), False),
    ("Inn", -56000, 114900, 180, 1200, 850, 3, 290, 32, RIDGE, ASHLAR, c(1, 1, 1), 2, 0.0, False, c(0.03, 0.05, 0.12), False),
    ("WestCottage", -56000, 113700, 180, 900, 650, 1, 290, 45, GABLE, RUBBLE, c(1.05, 1.02, 0.95), -1, 0.5, False, c(0.1, 0.07, 0.03), True),
    ("Butcher", -55350, 113500, -90, 900, 750, 2, 290, 45, GABLE, RUBBLE, c(0.92, 0.92, 0.9), -1, -0.55, True, c(0.05, 0.12, 0.06), True),
    ("SouthHouse", -54430, 113500, -90, 900, 700, 2, 300, 38, RIDGE, RENDER, c(0.92, 0.95, 1.0), 2, 0.0, False, c(0.05, 0.05, 0.05), False),
    ("Chemist", -53520, 113500, -90, 880, 750, 2, 310, 35, RIDGE, ASHLAR, c(1.05, 1.0, 0.92), 1, 0.55, True, c(0.10, 0.06, 0.02), True),
    ("EastCottage", -52000, 114900, 0, 800, 650, 1, 285, 48, GABLE, RUBBLE, c(1, 1, 1), 1, -0.5, False, c(0.1, 0.07, 0.03), True),
]

# Buildings whose plots were taken over by a game-spawned shop.
REMOVED = ["EastHouse"]


def load_town():
    """World Partition keeps unloaded actors out of get_all_level_actors, so load every town building
    first; otherwise a rerun would spawn duplicates beside the saved ones."""
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
    for name in REMOVED:
        gone = have.pop(f"Town_Blockout_{name}", None)
        if gone is not None:
            # Saving its external-actor package deletes the file.
            packages.append(gone.get_outermost())
            EAS.destroy_actor(gone)
            unreal.log(f"TOWN_MASSING_REMOVED {name}")
    for (name, x, y, yaw, width, depth, storeys, storey_h, pitch, roof, wall, tint, chimneys, door,
         shop, sign, side) in BUILDINGS:
        label = f"Town_Blockout_{name}"
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
            ("width", float(width)), ("depth", float(depth)), ("storeys", storeys),
            ("storey_height", float(storey_h)), ("roof_pitch", float(pitch)), ("roof", roof), ("wall", wall),
            ("wall_tint", tint), ("chimneys", chimneys), ("door_at", float(door)), ("shopfront", shop),
            ("sign_color", sign), ("side_windows", side), ("snap_to_ground", True),
        ):
            actor.set_editor_property(key, value)
        packages.append(actor.get_outermost())
    # The `Town` outliner folder is its own external object the first time it's used.
    packages += [p for p in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
                 if "__ExternalObjects__" in p.get_name()]
    unreal.EditorLoadingAndSavingUtils.save_packages(packages, False)
    unreal.log(f"TOWN_MASSING_PLACED {len(packages)}")


main()
