"""Scripted MetaHuman Creator look edits for the heroine.

Run inside the editor (MCP ``run_python``), for example::

    from homestead_agent import metahuman_look as ml
    ml.set_hair(ml.HEROINE, ml.HEROINE_LOOK["hair"])   # one run_python call
    ml.apply_colours(ml.HEROINE, ml.HEROINE_LOOK)       # a later run_python call

Close the character's Creator window before applying; scripted edits don't show in an
already-open Creator window.
"""

import os
import re
import tempfile

import unreal

HEROINE = "/Game/Characters/Heroine_MH/MHC_Heroine"
HAIR_DIR = "/MetaHumanCharacter/Optional/Grooms/Bindings/Hair"
EYE_PRESETS = "/MetaHumanCharacter/Tools/EyePresets/EyePresets"

# Jenny's default dark brown (option C, 2026-09-25). In game, hair, eye and skin colour stay
# player-configurable; these are only the authored defaults.
CHESTNUT = {"Melanin": 0.72, "Redness": 0.35, "Lightness": 0.5, "Whiteness": 0.0}
CHESTNUT_BROWS = {"Melanin": 0.8, "Redness": 0.35}

HEROINE_LOOK = {
    "hair": "WI_Hair_L_Straight",
    "hair_params": CHESTNUT,
    "brow_params": CHESTNUT_BROWS,
    "eye_preset": 8,
}


def _subsystem():
    return unreal.get_editor_subsystem(unreal.MetaHumanCharacterEditorSubsystem)


def close_editors(character):
    unreal.get_editor_subsystem(unreal.AssetEditorSubsystem).close_all_editors_for_asset(character)


def eye_preset_settings(index):
    """Return MetaHumanCharacterEyesSettings for Creator eye preset ``index`` (1-based)."""
    task = unreal.AssetExportTask()
    task.object = unreal.load_asset(EYE_PRESETS)
    task.filename = os.path.join(tempfile.gettempdir(), "homestead_eyepresets.t3d")
    task.automated = True
    task.prompt = False
    task.replace_identical = True
    if not unreal.Exporter.run_asset_export_task(task):
        raise RuntimeError("Could not export eye presets")
    with open(task.filename, encoding="utf-8", errors="ignore") as handle:
        text = handle.read()
    match = re.search(r'PresetName="Preset %03d",EyesSettings=' % index, text)
    if not match:
        raise ValueError("No eye preset %d" % index)
    start = match.end()
    depth = 0
    for end in range(start, len(text)):
        depth += {"(": 1, ")": -1}.get(text[end], 0)
        if depth == 0:
            break
    settings = unreal.MetaHumanCharacterEyesSettings()
    settings.import_text(text[start:end + 1])
    return settings


def _selected_items(collection):
    return {
        str(entry.selection.slot_name): entry.selection.selected_item
        for entry in collection.default_instance.get_slot_selection_data()
    }


def _set_params(instance, item_key, values):
    params = {str(p.name): p for p in instance.get_instance_parameters(
        item_path=unreal.MetaHumanPaletteItemPath(item_key=item_key))}
    for name, value in values.items():
        param = params.get(name)
        if param is None:
            raise KeyError("Missing instance parameter %s" % name)
        if isinstance(value, unreal.LinearColor):
            param.set_color(value=value)
        elif isinstance(value, bool):
            param.set_bool(value=value)
        else:
            param.set_float(value=float(value))


def _edit(character):
    sub = _subsystem()
    if not sub.try_add_object_to_edit(character):
        raise RuntimeError("%s is already being edited" % character.get_path_name())
    return sub


def _end_edit(character):
    sub = _subsystem()
    if sub.is_object_added_for_editing(character):
        sub.remove_object_to_edit(character)


def set_hair(character_path, hair):
    """Select a hair wardrobe item, reusing its palette entry if it was added before.

    Groom instance parameters for a new selection only appear after the editor ticks, so
    call ``apply_colours`` in a separate ``run_python`` call.
    """
    character = unreal.load_asset(character_path)
    close_editors(character)
    sub = _edit(character)
    try:
        collection = sub.get_preview_collection(character)
        item = unreal.load_asset("%s/%s" % (HAIR_DIR, hair))
        keys = list(collection.get_item_keys_for_wardrobe_item(item))
        key = keys[0] if keys else collection.try_add_item_from_wardrobe_item(slot_name="Hair", wardrobe_item=item)
        collection.default_instance.set_single_slot_selection("Hair", key)
        sub.on_edit_preview_collection(character)
    finally:
        _end_edit(character)
    unreal.EditorAssetLibrary.save_loaded_asset(character, False)
    return character


def apply_colours(character_path, look, save=True):
    """Apply groom colours and the eye preset from ``look`` to the current selections."""
    character = unreal.load_asset(character_path)
    close_editors(character)
    sub = _edit(character)
    try:
        collection = sub.get_preview_collection(character)
        sub.assemble_for_preview(character=character)
        selected = _selected_items(collection)
        if look.get("hair_params"):
            _set_params(collection.default_instance, selected["Hair"], look["hair_params"])
        if look.get("brow_params"):
            for slot in ("Eyebrows", "Eyelashes"):
                if slot in selected:
                    _set_params(collection.default_instance, selected[slot], look["brow_params"])
        sub.on_edit_preview_collection(character)
        if look.get("eye_preset"):
            sub.commit_eyes_settings(character=character, eyes_settings=eye_preset_settings(look["eye_preset"]))
    finally:
        _end_edit(character)
    if save:
        unreal.EditorAssetLibrary.save_loaded_asset(character, False)
    return character


def open_for_review(character_path, frame="FACE"):
    """Open the Creator window framed for a clean review screenshot."""
    character = unreal.load_asset(character_path)
    close_editors(character)
    settings = character.viewport_settings
    settings.show_viewport_overlays = False
    settings.camera_frame = getattr(unreal.MetaHumanCharacterCameraFrame, frame)
    character.viewport_settings = settings
    unreal.get_editor_subsystem(unreal.AssetEditorSubsystem).open_editor_for_assets([character])
    return character


# Face landmark indices from get_face_landmarks (81 points; x = lateral, y = forward, z = up, cm).
# Pairs are (left, right); the right side mirrors x. Mapped on MHC_Heroine 2026-09-25.
LANDMARKS = {
    "nose_tip": [64], "nose_wing": [(67, 45)], "nose_groove": [(70, 74)], "nose_bridge": [(57, 40)],
    "upper_lip_top": [5], "upper_lip_peak": [(47, 31)], "lower_lip_bottom": [4],
    "lower_lip_side": [(9, 16)], "mouth_corner": [(48, 32)],
    "cheekbone": [(58, 41)], "cheek_arch": [(69, 73)], "cheek_hollow": [(55, 21)],
    "cheek_front": [(66, 23)], "cheek_mouth": [(2, 29)],
}

# Jenny 2026-09-25: thinner lips, thinner nose, slightly sharper cheekbones.
# (dx, dy, dz) per region; dx is outward (positive widens), applied mirrored to pairs.
HEROINE_SCULPT = {
    "nose_wing": (-0.25, 0.0, 0.0), "nose_groove": (-0.15, 0.0, 0.0), "nose_bridge": (-0.1, 0.0, 0.0),
    "upper_lip_top": (0.0, -0.1, -0.22), "upper_lip_peak": (0.0, -0.08, -0.18),
    "lower_lip_bottom": (0.0, -0.1, 0.22), "lower_lip_side": (0.0, -0.08, 0.16),
    "cheekbone": (0.3, 0.15, 0.1), "cheek_arch": (0.2, 0.0, 0.05),
    "cheek_hollow": (-0.3, -0.2, 0.0), "cheek_mouth": (0.0, -0.15, 0.0),
}


def sculpt_face(character_path, edits, scale=1.0, save=True):
    """Translate landmark regions by ``edits`` (see HEROINE_SCULPT) and commit the face."""
    character = unreal.load_asset(character_path)
    close_editors(character)
    indices, deltas = [], []
    for region, (dx, dy, dz) in edits.items():
        for entry in LANDMARKS[region]:
            sides = [(entry[0], -1.0), (entry[1], 1.0)] if isinstance(entry, tuple) else [(entry, 0.0)]
            for index, sign in sides:
                indices.append(index)
                deltas.append(unreal.Vector(dx * sign * scale, dy * scale, dz * scale))
    sub = _edit(character)
    try:
        sub.translate_face_landmarks(character=character, landmark_indices=indices, deltas=deltas)
        sub.commit_face_state(character=character)
    finally:
        _end_edit(character)
    if save:
        unreal.EditorAssetLibrary.save_loaded_asset(character, False)
    return character
