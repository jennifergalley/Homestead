"""Photoreal surfaces for the mine house: tiling CC0 photoscans on its stone, mortar, limewash, slate and timber.

Usage (repo root):
    pwsh Scripts\\Blender\\Get-PolyHavenAsset.ps1 <id> -Resolution 2k     # for each SURFACES source (git-ignored cache)
    python Scripts\\Mine\\mine_surfaces.py                                 # step 1: maps -> Assets/Props/MineSurfaces
    pyfile Scripts\\Mine\\mine_surfaces.py                                 # step 2, in the editor after import_props:
                                                                           # material, instances, slots, full-precision UVs

Jenny (review 2, 2026-10-11): "Textures inside and out still need significant work to be photorealistic (granite
rubble, dressed quoins, slate, timber, plaster/floor inside)". Scripts/Blender/Recipes/mine_house.py (PHOTO) gives
the masonry, roof and floor meshes UVs in metres, one layout per stone/slate/board with its own random patch of
the photograph, and the piece's tint in the vertex colour. M_MineSurface samples a photoscan at TilesPerMetre,
multiplies the per-piece tint, breaks up the repeat with a low-frequency sample of itself, and darkens a band of
splash and soil at the wall foot. The scans are CC0 from Poly Haven (credits: Assets/Props/MineSurfaces/CREDITS.json).
"""
import json
import os
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CACHE = ROOT / "Assets" / "Source" / "Blender" / "polyhaven"
OUT = ROOT / "Assets" / "Props" / "MineSurfaces" / "Textures"
DEST = "/Game/SurvivalGame/Environment/Props/MineSurfaces"
PARENT = f"{DEST}/M_MineSurface"
SIZE = 2048
YARD_Z_CM = 5600.0          # the yard level (HomesteadEstate.cpp Anchor::MineEntrance z) for the foot grime

# Photoscan per surface (Poly Haven id).
SOURCES = {
    "Granite": "lichen_rock",            # weathered grey rock with lichen: Cornish granite rubble and quoins
    "Mortar": "grey_plaster_02",         # lime mortar
    "Limewash": "white_rough_plaster",   # limewash over the inner rubble, flaking
    "Slate": "seaside_rock",             # fine dark grey stone: Delabole slate
    "Boards": "fine_grained_wood",       # floorboards, worn
    "Timber": "rough_wood",              # joists, beams, roof boarding
    "Brick": "rock_surface",             # fired clay faces of the stack's brick
}
# Material instances: slot (Blender material name) -> surface and parameters.
#   tiles: texture repeats a metre; tint: linear multiplier; vc: how much the per-piece tint applies;
#   normal: normal-map strength; rough: (scale, bias); macro: low-frequency break-up; grime: foot band strength.
# Tints lean a little away from green: the house stands in tall meadow grass, whose bounce light tinted the pale
# limewash and granite yellow-green in review (Jenny, Balance: "too green", "limewash reads yellow-green").
INSTANCES = {
    "M_MineHouseStone": ("Granite", dict(tiles=0.9, tint=(1.85, 1.55, 1.58), sat=0.12, vc=1.0, normal=1.0,
                                         rough=(1.0, 0.0), macro=0.35, grime=0.45)),
    "M_MineHouseShaftStone": ("Granite", dict(tiles=0.9, tint=(0.55, 0.53, 0.50), sat=0.1, vc=1.0, normal=1.0,
                                              rough=(0.9, 0.0), macro=0.3, grime=0.0)),
    "M_MineHouseMortar": ("Mortar", dict(tiles=1.2, tint=(0.62, 0.53, 0.56), sat=0.0, vc=0.0, normal=1.0,
                                         rough=(1.0, 0.05), macro=0.4, grime=0.6)),
    "M_MineHouseLimewash": ("Limewash", dict(tiles=1.1, tint=(0.82, 0.76, 0.96), sat=0.0, vc=0.0, normal=1.0,
                                             rough=(1.0, 0.0), macro=0.35, grime=0.5)),
    "M_MineHouseBrick": ("Brick", dict(tiles=2.5, tint=(1.15, 0.62, 0.45), sat=1.0, vc=1.0, normal=0.8,
                                       rough=(1.0, 0.0), macro=0.2, grime=0.0)),
    "M_MineHouseSlate": ("Slate", dict(tiles=1.6, tint=(0.62, 0.66, 0.72), sat=1.0, vc=1.0, normal=0.9,
                                       rough=(0.85, 0.0), macro=0.3, grime=0.0)),
    "M_MineHouseRidge": ("Brick", dict(tiles=2.0, tint=(1.05, 0.58, 0.42), sat=1.0, vc=1.0, normal=0.8,
                                       rough=(1.0, 0.0), macro=0.2, grime=0.0)),
    "M_MineHouseDeck": ("Timber", dict(tiles=0.7, tint=(0.72, 0.62, 0.52), sat=0.8, vc=1.0, normal=1.0,
                                       rough=(1.0, 0.0), macro=0.3, grime=0.0)),
    "M_MineHouseBoards": ("Boards", dict(tiles=0.6, tint=(0.95, 0.85, 0.75), sat=0.9, vc=1.0, normal=1.0,
                                         rough=(1.0, 0.0), macro=0.35, grime=0.0)),
    "M_MineHouseBeams": ("Timber", dict(tiles=0.6, tint=(0.62, 0.54, 0.46), sat=0.8, vc=1.0, normal=1.0,
                                        rough=(1.0, 0.0), macro=0.3, grime=0.0)),
    # Joinery (kit primitives, no metric UVs): world-aligned triplanar.
    # Jenny: frames "bright orange-red like rusted metal; should be weathered or painted wood".
    "M_MineHousePaint": ("Timber", dict(tiles=1.0, tint=(0.62, 0.50, 0.40), sat=0.45, vc=0.0, normal=1.0,
                                         rough=(1.0, 0.0), macro=0.3, grime=0.3, tri=1.0)),
    "M_MineHouseWeatherboard": ("Timber", dict(tiles=0.8, tint=(0.80, 0.76, 0.70), sat=0.45, vc=0.0, normal=1.0,
                                               rough=(1.0, 0.0), macro=0.3, grime=0.3, tri=1.0)),
    "M_MineHouseIron": ("Slate", dict(tiles=3.0, tint=(0.26, 0.21, 0.18), sat=1.0, vc=0.0, normal=0.6,
                                      rough=(0.7, 0.15), macro=0.2, grime=0.0, tri=1.0)),
    # The furnishing (whole meshes, see WHOLE_MESHES).
    "M_MineFurnitureWood": ("Boards", dict(tiles=1.3, tint=(0.80, 0.66, 0.52), sat=0.9, vc=0.0, normal=0.8,
                                           rough=(0.9, 0.0), macro=0.25, grime=0.0, tri=1.0)),
    "M_MinePileStone": ("Granite", dict(tiles=1.4, tint=(1.85, 1.55, 1.58), sat=0.12, vc=0.0, normal=1.0,
                                        rough=(1.0, 0.0), macro=0.3, grime=0.0, tri=1.0)),
    "M_MineLidBoards": ("Boards", dict(tiles=0.6, tint=(0.95, 0.85, 0.75), sat=0.9, vc=0.0, normal=1.0,
                                       rough=(1.0, 0.0), macro=0.35, grime=0.0, tri=1.0)),
}
# Meshes whose every slot takes one instance (baked props that read better in the photoscans).
WHOLE_MESHES = {"/Game/SurvivalGame/Environment/Props/MineFurnishing/SM_MineDesk": "M_MineFurnitureWood",
                "/Game/SurvivalGame/Environment/Props/MineFurnishing/SM_MineChair": "M_MineFurnitureWood",
                "/Game/SurvivalGame/Environment/Props/MineFurnishing/SM_MineTrapdoorLid": "M_MineLidBoards",
                "/Game/SurvivalGame/Environment/Props/MineFurnishing/SM_MineRockPile": "M_MinePileStone"}
MESHES = ["SM_MineHouseWallE", "SM_MineHouseWallW", "SM_MineHouseWallN", "SM_MineHouseWallS", "SM_MineHouseStack",
          "SM_MineHouseShaft", "SM_MineHouseRoof", "SM_MineHouseFloors", "SM_MineHouseJoinery"]
MESH_DEST = "/Game/SurvivalGame/Environment/Props/MineHouse"


# --------------------------------------------------------------------------- step 1: maps (system Python)

def prepare():
    os.environ["OPENCV_IO_ENABLE_OPENEXR"] = "1"
    import cv2
    import numpy as np

    def read(path):
        image = cv2.imread(str(path), cv2.IMREAD_UNCHANGED)
        if image is None:
            raise RuntimeError(f"could not read {path}")
        if image.dtype != np.float32:
            image = image.astype(np.float32) / (65535.0 if image.dtype == np.uint16 else 255.0)
        return cv2.resize(image, (SIZE, SIZE), interpolation=cv2.INTER_AREA)

    def write(path, image):
        cv2.imwrite(str(path), np.clip(np.rint(image * 255.0), 0, 255).astype(np.uint8),
                    [cv2.IMWRITE_JPEG_QUALITY, 94] if path.suffix == ".jpg" else [])

    OUT.mkdir(parents=True, exist_ok=True)
    credits = {}
    for surface, ident in SOURCES.items():
        folder = CACHE / f"{ident}_2k"
        receipt = json.loads((folder / "receipt.json").read_text(encoding="utf-8-sig"))
        textures = folder / "textures"
        # 8/16-bit PNG and JPG first: OpenCV cannot read every EXR compression Poly Haven uses.
        found = {p.name: p for p in sorted(textures.iterdir(), key=lambda p: p.suffix == ".exr")}

        def pick(*keys):
            for key in keys:
                for name, path in found.items():
                    if f"_{key}_" in name:
                        return path
            raise RuntimeError(f"{ident}: no {keys} map in {textures}")

        colour = read(pick("diff", "col"))
        write(OUT / f"T_MineSurf_{surface}_basecolor.jpg", colour)
        mean = float((colour[..., :3] * np.array([0.11, 0.59, 0.3], np.float32)).sum(axis=-1).mean())   # BGR
        normal = read(pick("nor_gl"))
        write(OUT / f"T_MineSurf_{surface}_normal.png", normal[..., :3])
        rough = read(pick("rough"))
        if rough.ndim == 3:
            rough = rough[..., 0]
        write(OUT / f"T_MineSurf_{surface}_roughness.jpg", rough)
        credits[surface] = {"id": ident, "name": receipt["name"], "authors": receipt["authors"],
                            "license": receipt["license"], "source": receipt["source"], "mean_luminance": round(mean, 4)}
        print(f"{surface}: {ident} -> T_MineSurf_{surface}_*")
    (OUT.parent / "CREDITS.json").write_text(json.dumps(credits, indent=2) + "\n", encoding="utf-8")


# --------------------------------------------------------------------------- step 2: Unreal

MACRO_CODE = """
float l = dot(Texture2DSample(Tex, TexSampler, UV * 0.071 + float2(0.37, 0.61)).rgb, float3(0.3, 0.59, 0.11));
float m = dot(Texture2DSample(Tex, TexSampler, UV * 0.023 + float2(0.13, 0.29)).rgb, float3(0.3, 0.59, 0.11));
return lerp(1.0, (l + m) / max(Mean * 2.0, 1e-3), Amount);
"""
GRIME_CODE = """
float above = (WorldPos.z - Ground) / 100.0;
float band = saturate(1.0 - above / 0.6) * saturate((above + 1.5) / 1.0);
return band * Amount;
"""
SAMPLE_CODE = """
if (Tri < 0.5)
    return Texture2DSample(Tex, TexSampler, UV).rgb;
float3 w = pow(abs(normalize(N)), 4.0);
w /= (w.x + w.y + w.z);
float3 p = WorldPos * (Tile / 100.0);
return Texture2DSample(Tex, TexSampler, p.yz).rgb * w.x + Texture2DSample(Tex, TexSampler, p.xz).rgb * w.y
     + Texture2DSample(Tex, TexSampler, p.xy).rgb * w.z;
"""
SATURATION_CODE = """
float l = dot(C, float3(0.3, 0.59, 0.11));
return lerp(float3(l, l, l), C, S);
"""
NORMAL_CODE = """
float2 xy = (Raw.xy * 2.0 - 1.0) * K;
return normalize(float3(xy, sqrt(saturate(1.0 - dot(xy, xy)))));
"""


def build_parent(unreal):
    LIB, EDIT = unreal.EditorAssetLibrary, unreal.MaterialEditingLibrary
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    if LIB.does_asset_exist(PARENT):
        material = LIB.load_asset(PARENT)
        EDIT.delete_all_material_expressions(material)
        for expression in list(EDIT.get_material_expressions(material) or []):
            EDIT.delete_material_expression(material, expression)
    else:
        folder, name = PARENT.rsplit("/", 1)
        material = tools.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    material.set_editor_property("used_with_nanite", True)
    material.set_editor_property("used_with_instanced_static_meshes", True)
    ST = unreal.MaterialSamplerType

    def node(kind, x, y):
        created = EDIT.create_material_expression(material, kind, x, y)
        if not created:
            raise RuntimeError(f"could not create {kind}")
        return created

    def link(a, out, b, inp):
        if not EDIT.connect_material_expressions(a, out, b, inp):
            raise RuntimeError(f"could not connect {a.get_name()}.{out} -> {b.get_name()}.{inp}")

    def scalar(name, value, y):
        s = node(unreal.MaterialExpressionScalarParameter, -1800, y)
        s.set_editor_property("parameter_name", name)
        s.set_editor_property("default_value", value)
        return s

    def custom(desc, code, inputs, out_type, x, y):
        c = node(unreal.MaterialExpressionCustom, x, y)
        c.set_editor_property("description", desc)
        c.set_editor_property("output_type", out_type)
        items = []
        for name in inputs:
            item = unreal.CustomInput()
            item.set_editor_property("input_name", name)
            items.append(item)
        c.set_editor_property("inputs", items)
        c.set_editor_property("code", code)
        return c

    world = node(unreal.MaterialExpressionWorldPosition, -1800, 700)
    pixel_normal = node(unreal.MaterialExpressionVertexNormalWS, -1800, 600)
    triplanar = scalar("Triplanar", 0.0, -700)

    def sampler(name, default, kind, uv, y):
        """The photo at UV (metric per-piece UVs), or world-aligned triplanar for meshes without them."""
        texture = node(unreal.MaterialExpressionTextureObjectParameter, -1400, y)
        texture.set_editor_property("parameter_name", name)
        texture.set_editor_property("texture", unreal.load_object(None, default))
        texture.set_editor_property("sampler_type", kind)
        sample = custom("Sample " + name, SAMPLE_CODE, ("Tex", "UV", "Tri", "WorldPos", "N", "Tile"),
                        unreal.CustomMaterialOutputType.CMOT_FLOAT3, -1000, y)
        link(texture, "", sample, "Tex")
        link(uv, "", sample, "UV")
        link(triplanar, "", sample, "Tri")
        link(world, "", sample, "WorldPos")
        link(pixel_normal, "", sample, "N")
        link(tiles, "", sample, "Tile")
        return sample

    masks_default = "/Game/SurvivalGame/Materials/Textures/T_PropDefaultWhite.T_PropDefaultWhite"
    coords = node(unreal.MaterialExpressionTextureCoordinate, -1800, -900)
    tiles = scalar("TilesPerMetre", 1.0, -800)
    uv = node(unreal.MaterialExpressionMultiply, -1500, -850)
    link(coords, "", uv, "A")
    link(tiles, "", uv, "B")
    base = sampler("BaseColorTexture", "/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture",
                   ST.SAMPLERTYPE_COLOR, uv, -900)
    normal = sampler("NormalTexture", masks_default, ST.SAMPLERTYPE_MASKS, uv, -500)  # raw GL normal, decoded below
    rough = sampler("RoughnessTexture", masks_default, ST.SAMPLERTYPE_MASKS, uv, -100)

    base_obj = node(unreal.MaterialExpressionTextureObjectParameter, -1500, 300)
    base_obj.set_editor_property("parameter_name", "MacroTexture")
    base_obj.set_editor_property("texture", unreal.load_object(
        None, "/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"))
    macro = custom("Macro break-up", MACRO_CODE, ("Tex", "UV", "Amount", "Mean"),
                   unreal.CustomMaterialOutputType.CMOT_FLOAT1, -1000, 300)
    link(base_obj, "", macro, "Tex")
    link(uv, "", macro, "UV")
    link(scalar("MacroAmount", 0.3, 400), "", macro, "Amount")
    link(scalar("MacroMean", 0.25, 500), "", macro, "Mean")

    tint = node(unreal.MaterialExpressionVectorParameter, -1800, -1100)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    vertex = node(unreal.MaterialExpressionVertexColor, -1800, -1300)
    vc_scaled = node(unreal.MaterialExpressionMultiply, -1500, -1300)
    link(vertex, "", vc_scaled, "A")
    vc_scaled.set_editor_property("const_b", 1.5)
    vc = node(unreal.MaterialExpressionLinearInterpolate, -1300, -1250)
    vc.set_editor_property("const_a", 1.0)
    link(vc_scaled, "", vc, "B")
    link(scalar("VertexTint", 1.0, -1200), "", vc, "Alpha")

    grime = custom("Foot grime", GRIME_CODE, ("WorldPos", "Ground", "Amount"),
                   unreal.CustomMaterialOutputType.CMOT_FLOAT1, -1000, 700)
    link(world, "", grime, "WorldPos")
    link(scalar("GroundZ", YARD_Z_CM, 800), "", grime, "Ground")
    link(scalar("GrimeAmount", 0.0, 900), "", grime, "Amount")
    grime_colour = node(unreal.MaterialExpressionVectorParameter, -1000, 1000)
    grime_colour.set_editor_property("parameter_name", "GrimeColor")
    grime_colour.set_editor_property("default_value", unreal.LinearColor(0.32, 0.27, 0.20, 1))

    # Desaturate the photo first, then tint, so the tint sets the hue (a tint applied before a strong
    # desaturation only changed the brightness).
    saturated = custom("Saturation", SATURATION_CODE, ("C", "S"), unreal.CustomMaterialOutputType.CMOT_FLOAT3,
                       -760, -1000)
    link(base, "", saturated, "C")
    link(scalar("Saturation", 1.0, -1300), "", saturated, "S")
    c1 = node(unreal.MaterialExpressionMultiply, -650, -900)
    link(saturated, "", c1, "A")
    link(tint, "", c1, "B")
    c2 = node(unreal.MaterialExpressionMultiply, -550, -900)
    link(c1, "", c2, "A")
    link(vc, "", c2, "B")
    c3 = node(unreal.MaterialExpressionMultiply, -400, -900)
    link(c2, "", c3, "A")
    link(macro, "", c3, "B")
    grimed = node(unreal.MaterialExpressionMultiply, -400, -700)
    link(c3, "", grimed, "A")
    link(grime_colour, "", grimed, "B")
    colour = node(unreal.MaterialExpressionLinearInterpolate, -250, -850)
    link(c3, "", colour, "A")
    link(grimed, "", colour, "B")
    link(grime, "", colour, "Alpha")
    EDIT.connect_material_property(colour, "", unreal.MaterialProperty.MP_BASE_COLOR)

    decoded = custom("Normal strength", NORMAL_CODE, ("Raw", "K"), unreal.CustomMaterialOutputType.CMOT_FLOAT3,
                     -600, -500)
    link(normal, "", decoded, "Raw")
    link(scalar("NormalStrength", 1.0, -400), "", decoded, "K")
    EDIT.connect_material_property(decoded, "", unreal.MaterialProperty.MP_NORMAL)

    r1 = node(unreal.MaterialExpressionMultiply, -600, -100)
    link(rough, "", r1, "A")
    link(scalar("RoughnessScale", 1.0, -100), "", r1, "B")
    r2 = node(unreal.MaterialExpressionAdd, -450, -100)
    link(r1, "", r2, "A")
    link(scalar("RoughnessBias", 0.0, 0), "", r2, "B")
    r3 = node(unreal.MaterialExpressionLinearInterpolate, -300, -50)
    link(r2, "", r3, "A")
    r3.set_editor_property("const_b", 1.0)
    link(grime, "", r3, "Alpha")
    EDIT.connect_material_property(r3, "", unreal.MaterialProperty.MP_ROUGHNESS)

    EDIT.recompile_material(material)
    stats = EDIT.get_statistics(material)
    print("M_MineSurface ps instructions", stats.num_pixel_shader_instructions)
    if stats.num_pixel_shader_instructions <= 0:
        raise RuntimeError("M_MineSurface failed to compile (see the log)")
    LIB.save_loaded_asset(material, False)
    return material


def import_maps(unreal, surface):
    LIB = unreal.EditorAssetLibrary
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    out = {}
    for role in ("basecolor", "normal", "roughness"):
        source = OUT / f"T_MineSurf_{surface}_{role}.{'png' if role == 'normal' else 'jpg'}"
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", str(source))
        task.set_editor_property("destination_path", f"{DEST}/Textures")
        task.set_editor_property("destination_name", source.stem)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", False)
        tools.import_asset_tasks([task])
        texture = LIB.load_asset(f"{DEST}/Textures/{source.stem}")
        if role == "basecolor":
            texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT)
            texture.set_editor_property("srgb", True)
        else:
            # Normals stay raw (GL, decoded in the material with a strength); both are linear masks.
            texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
            texture.set_editor_property("srgb", False)
        LIB.save_loaded_asset(texture, False)
        out[role] = texture
    return out


def apply(unreal):
    LIB, EDIT = unreal.EditorAssetLibrary, unreal.MaterialEditingLibrary
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    parent = build_parent(unreal)
    maps = {surface: import_maps(unreal, surface) for surface in SOURCES}
    # Mean luminance of each basecolor, so the macro break-up keeps the average brightness.
    credits = json.loads((OUT.parent / "CREDITS.json").read_text(encoding="utf-8"))
    means = {s: credits[s]["mean_luminance"] for s in SOURCES}
    instances = {}
    for slot, (surface, p) in INSTANCES.items():
        name = "MI_MineSurf_" + (slot[len("M_MineHouse"):] if slot.startswith("M_MineHouse") else slot[len("M_Mine"):])
        path = f"{DEST}/{name}"
        mi = LIB.load_asset(path) if LIB.does_asset_exist(path) else tools.create_asset(
            name, DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        EDIT.set_material_instance_parent(mi, parent)
        for role, parameter in (("basecolor", "BaseColorTexture"), ("basecolor", "MacroTexture"), ("normal", "NormalTexture"),
                                ("roughness", "RoughnessTexture")):
            EDIT.set_material_instance_texture_parameter_value(mi, parameter, maps[surface][role])
        for parameter, value in (("TilesPerMetre", p["tiles"]), ("VertexTint", p["vc"]),
                                 ("NormalStrength", p["normal"]), ("RoughnessScale", p["rough"][0]),
                                 ("RoughnessBias", p["rough"][1]), ("MacroAmount", p["macro"]),
                                 ("MacroMean", means[surface] ** 2.2), ("GrimeAmount", p["grime"]),
                                 ("Saturation", p.get("sat", 1.0)), ("Triplanar", p.get("tri", 0.0))):
            EDIT.set_material_instance_scalar_parameter_value(mi, parameter, float(value))
        EDIT.set_material_instance_vector_parameter_value(mi, "Tint", unreal.LinearColor(*p["tint"], 1.0))
        EDIT.update_material_instance(mi)
        LIB.save_loaded_asset(mi, False)
        instances[slot] = mi
    sub = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    for mesh_name in MESHES:
        mesh = LIB.load_asset(f"{MESH_DEST}/{mesh_name}")
        if not mesh:
            print("missing", mesh_name)
            continue
        changed = False
        for lod in range(sub.get_lod_count(mesh)):
            settings = sub.get_lod_build_settings(mesh, lod)
            if not settings.get_editor_property("use_full_precision_u_vs"):
                settings.set_editor_property("use_full_precision_u_vs", True)
                sub.set_lod_build_settings(mesh, lod, settings)
                changed = True
        slots = [str(m.get_editor_property("material_slot_name")) for m in mesh.get_editor_property("static_materials")]
        for index, slot in enumerate(slots):
            if slot in instances:
                mesh.set_material(index, instances[slot])
        LIB.save_loaded_asset(mesh, False)
        print(mesh_name, "slots", slots, "full-precision UVs set" if changed else "")
    for path, slot in WHOLE_MESHES.items():
        mesh = LIB.load_asset(path)
        if not mesh:
            print("missing", path)
            continue
        for index in range(len(mesh.get_editor_property("static_materials"))):
            mesh.set_material(index, instances[slot])
        LIB.save_loaded_asset(mesh, False)
        print(path.rsplit("/", 1)[1], "->", instances[slot].get_name())


if __name__ == "__main__":
    try:
        import unreal
    except ImportError:
        prepare()
    else:
        apply(unreal)
