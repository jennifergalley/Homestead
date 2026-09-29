"""Build the Estate meadow's assets inside the running editor: the grass patch meshes, the ground
and wind textures, and M_EstateGrass / MI_EstateGrass. See bake_ground.py for the data.

Run `python Scripts/Terrain/bake_ground.py` first, then, with PIE stopped:
  pyfile Scripts/Terrain/build_ground.py            (McpHelpers)
Re-running re-imports everything and re-authors the material graph in place.

The runtime side is UHomesteadGrassField (Source/SurvivalGame/HomesteadGrassField.*): it instances
the patches round the camera and writes each patch's clear circles into per-instance custom data.
"""
import os

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
FOLDER = "/Game/SurvivalGame/Estate/Ground"
LIB = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
SMS = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
ROAD_SDF = "/Game/SurvivalGame/Estate/Landscape/Textures/T_EstateRoadSDF"
MPC = "/Game/SurvivalGame/Environment/CameraSafeFoliage/MPC_CameraSafeFoliage"


def import_file(source, name):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", source)
    task.set_editor_property("destination_path", FOLDER)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", False)
    TOOLS.import_asset_tasks([task])
    asset = unreal.load_asset(f"{FOLDER}/{name}")
    if not asset:
        raise RuntimeError(f"Could not import {source}")
    return asset


# CC0 ground sets from Poly Haven for M_EstateLandscape's finish pass (see docs/asset-credits.md).
# The 2k JPGs are downloaded to a scratch folder, imported, and not kept in the repo.
POLYHAVEN = {"Trodden": "grass_path_2", "Stony": "rocky_trail"}
POLYHAVEN_MAPS = {"D": ("Diffuse", "color"), "N": ("nor_dx", "normal"), "R": ("Rough", "mask")}


def import_polyhaven(scratch):
    import json
    import urllib.request
    os.makedirs(scratch, exist_ok=True)
    def fetch(url):
        # Poly Haven's API refuses requests without a browser-like User-Agent.
        return urllib.request.urlopen(urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0 Homestead"}))

    for name, pid in POLYHAVEN.items():
        files = None
        for slot, (key, kind) in POLYHAVEN_MAPS.items():
            local = os.path.join(scratch, f"{pid}_{slot}.jpg")
            if not os.path.exists(local):
                files = files or json.load(fetch(f"https://api.polyhaven.com/files/{pid}"))
                with open(local, "wb") as out:
                    out.write(fetch(files[key]["2k"]["jpg"]["url"]).read())
            import_texture(local, f"T_Ground_{name}_{slot}", kind)


def import_texture(source, name, kind):
    tex = import_file(source, name)
    if kind == "color":
        tex.set_editor_property("srgb", True)
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT)
        tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD)
        LIB.save_loaded_asset(tex, False)
        return tex
    if kind == "normal":
        tex.set_editor_property("srgb", False)
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD_NORMAL_MAP)
        LIB.save_loaded_asset(tex, False)
        return tex
    if kind == "mask":
        tex.set_editor_property("srgb", False)
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
        tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD)
        LIB.save_loaded_asset(tex, False)
        return tex
    tex.set_editor_property("srgb", False)
    tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD)
    if kind == "data":   # uncompressed RGBA8, clamped to the map
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_VECTOR_DISPLACEMENTMAP)
        tex.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP)
        tex.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
    else:
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_GRAYSCALE)
    LIB.save_loaded_asset(tex, False)
    return tex


def import_patch(lod):
    name = f"SM_GrassPatch_LOD{lod}"
    mesh = import_file(os.path.join(REPO, "Saved", "Ground", f"{name}.obj"), name)
    settings = SMS.get_lod_build_settings(mesh, 0)
    settings.set_editor_property("generate_lightmap_u_vs", False)
    settings.set_editor_property("recompute_normals", False)
    settings.set_editor_property("recompute_tangents", True)
    # UV0's whole part is each blade's rank bucket (0-255): half floats can't hold it with the root.
    settings.set_editor_property("use_full_precision_u_vs", True)
    settings.set_editor_property("distance_field_resolution_scale", 0.0)
    SMS.set_lod_build_settings(mesh, 0, settings)
    nanite = mesh.get_editor_property("nanite_settings")
    nanite.set_editor_property("enabled", False)
    mesh.set_editor_property("nanite_settings", nanite)
    SMS.remove_collisions(mesh)
    LIB.save_loaded_asset(mesh, False)
    return mesh


def uv_flipped(mesh):
    """Interchange may import OBJ texture V as 1 - v. Compare the blades' vertices with their roots."""
    desc = mesh.get_static_mesh_description(0)
    err = [0.0, 0.0]
    for i in range(0, min(400, desc.get_vertex_instance_count()), 4):
        vi = unreal.VertexInstanceID(i)
        uv = desc.get_vertex_instance_uv(vi, 0)
        pos = desc.get_vertex_position(desc.get_vertex_instance_vertex(vi))
        root_x = (uv.x - int(uv.x)) / 0.999 * 200.0 - 100.0
        if abs(root_x - pos.x) > 25.0:
            continue
        for flip in (0, 1):
            fy = (1.0 - uv.y) if flip else uv.y
            err[flip] += abs(fy / 0.999 * 200.0 - 100.0 - pos.y)
    print("grass patch UV check (straight, flipped):", [round(e) for e in err])
    return err[1] < err[0]


GROUND_MPC = f"{FOLDER}/MPC_EstateGround"


def build_mpc():
    """Wetness (0 dry .. 1 soaked) and Daylight (0 night .. 1 day), set every refresh by
    AHomesteadWorld::UpdateLighting from the game clock and the rain schedule."""
    mpc = unreal.load_asset(GROUND_MPC) if LIB.does_asset_exist(GROUND_MPC) else None
    if mpc is None:
        mpc = TOOLS.create_asset("MPC_EstateGround", FOLDER, unreal.MaterialParameterCollection,
                                 unreal.MaterialParameterCollectionFactoryNew())
    params = []
    for name, value in (("Wetness", 0.0), ("Daylight", 1.0)):
        s = unreal.CollectionScalarParameter()
        s.set_editor_property("parameter_name", name)
        s.set_editor_property("default_value", value)
        params.append(s)
    if [str(p.get_editor_property("parameter_name")) for p in mpc.get_editor_property("scalar_parameters")] != ["Wetness", "Daylight"]:
        mpc.set_editor_property("scalar_parameters", params)
    LIB.save_loaded_asset(mpc, False)
    return mpc


ROOT_LOCAL = """
float fy = VFlip > 0.5 ? 1.0 - UV.y : UV.y;
return float3(frac(UV.x) / 0.999 * 200.0 - 100.0, fy / 0.999 * 200.0 - 100.0, 0.0);
"""

VERTEX_CODE = """
float rank = (floor(UV.x) + 0.5) / 256.0;
float2 g = (RootW.xy + 201600.0) / 403200.0;
float4 gd = Texture2DSampleLevel(GroundTex, GroundTexSampler, g, 0);
float rd = abs(Texture2DSampleLevel(RoadTex, RoadTexSampler, g, 0).r - 0.5) * 8.0;   // m from the road's centre
float onRoad = 1.0 - smoothstep(1.05, 1.45, rd);
float crown = 1.0 - smoothstep(0.2, 0.36, rd);
float density = gd.r * lerp(1.0, 0.5 * crown, onRoad);

// Thin by rank with distance (UHomesteadGrassField picks patch LODs against the same curve).
float dist = length(RootW - Cam) * 0.01;
float keep = min(1.0, pow(Fade.x / max(dist, 0.1), Fade.y)) * (1.0 - smoothstep(Fade.z, Fade.w, dist));
float s = saturate((density * keep - rank) * 12.0);
float2 cc[3] = {float2(C0, C1), float2(C3, C4), float2(C6, C7)};
float cr[3] = {C2, C5, C8};
// Round interactables the sward is grazed short rather than bare, so nothing is hidden and there's
// no bald patch: blades keep a fifth of their height inside the circle.
float graze = 1.0;
[unroll] for (int i = 0; i < 3; i++)
    if (cr[i] > 0.0)
    {
        // A ragged, trampled edge rather than a mown circle: the radius wanders by a quarter.
        float2 o = RootW.xy - cc[i];
        float ang = atan2(o.y, o.x);
        float wob = 0.78 + 0.14 * sin(ang * 3.0 + cc[i].x * 0.013) + 0.1 * sin(ang * 7.0 + cc[i].y * 0.021) + 0.12 * frac(rank * 17.3);
        graze = min(graze, smoothstep(cr[i] * wob - 25.0, cr[i] * wob + 55.0, length(o)));
    }

// Height from the ground (lush by the river, short on the moor and the trodden edges); far blades
// widen so they stay a pixel or more across instead of shimmering.
// Camera-safe: blades near the game camera and along its line to the heroine are grazed short, so a
// low camera never looks through a wall of grass (MPC_CameraSafeFoliage, as the woodland foliage).
float2 seg = Hero.xy - CamPos.xy;
float tSeg = saturate(dot(RootW.xy - CamPos.xy, seg) / max(dot(seg, seg), 1.0));
float dSeg = length(RootW.xy - (CamPos.xy + seg * tSeg));
float camLow = 1.0 - smoothstep(60.0, 160.0, CamPos.z - RootW.z);      // only when the camera is down in it
float safe = lerp(1.0, smoothstep(Push.w, Push.w + 45.0, dSeg) * smoothstep(Push.z, Push.z + 45.0, length(RootW.xy - CamPos.xy)), camLow * (1.0 - tSeg * 0.6));
graze = min(graze, max(safe, 0.0));
float hgt = lerp(Shape.x, Shape.y, gd.g) * lerp(1.0, 0.4, onRoad) * (0.85 + 0.3 * frac(rank * 53.7)) * lerp(0.22, 1.0, graze);
float3 v = VtxW - RootW;
float widen = clamp(pow(max(dist / Shape.z, 1.0), 0.8), 1.0, Shape.w);
float3 nv = float3(v.xy * widen, v.z * hgt) * s;
float hn = max(nv.z, 0.0) / 50.0;

// Wind: broad gusts rolling across the field plus a small per-blade flutter.
float wa = radians(Wind.x);
float2 wd = float2(cos(wa), sin(wa));
float2 wq = RootW.xy * 0.01;
float gust = Texture2DSampleLevel(WindTex, WindTexSampler, (wq - wd * Time * Wind.w) / Wind.z, 0).r;
float gust2 = Texture2DSampleLevel(WindTex, WindTexSampler, (wq - wd * Time * Wind.w * 1.6) / (Wind.z * 0.29) + 0.37, 0).r;
float gs = smoothstep(0.35, 0.8, gust) * 0.8 + gust2 * 0.35;
float bend = Wind.y * (0.25 + 1.4 * gs) * 14.0 * hn * hn;
float flut = Wind.y * (0.4 + gs) * 1.6 * sin(Time * (4.0 + 5.0 * rank) + rank * 91.0 + dot(wq, float2(1.7, 1.3))) * hn * hn;
float2 disp = wd * bend + float2(-wd.y, wd.x) * flut;

// The heroine parts the grass as she walks through it.
float2 away = RootW.xy - Hero.xy;
float hd = length(away);
float nearHero = (1.0 - saturate(hd / Push.x)) * step(abs(RootW.z - Hero.z + 65.0), 160.0);
disp += away / max(hd, 1.0) * Push.y * nearHero * nearHero * saturate(hn * 1.3);

// Bent blades get shorter rather than longer.
nv.xy += disp;
nv.z = max(nv.z - dot(disp, disp) / max(2.0 * nv.z, 8.0), nv.z * 0.35);
Interp = float4(saturate(gd.b + (rank - 0.5) * 0.25), saturate(v.z / 55.0), rank, gs);
return RootW + nv - VtxW;
"""

PIXEL_CODE = """
float dry = I.x, t = I.y, rank = I.z, gs = I.w;
float wet = saturate(Wet), day = saturate(Day);
float hue = frac(rank * 37.31);
float3 tip = lerp(Tip.rgb, Dry.rgb, dry) * (0.78 + 0.44 * hue);
tip = lerp(tip, tip * float3(1.25, 1.05, 0.7), saturate(frac(rank * 11.7) * 3.0 - 2.2));   // the odd yellowing blade
float3 col = lerp(Root.rgb, tip, smoothstep(0.0, 0.85, t));
// The gust sheen is sunlight catching the blades: none at night.
col *= lerp(1.0, 1.12, gs * t * day);
// Rain: water film darkens the blades and makes them glossy.
col *= lerp(1.0, 0.72, wet);
RoughOut = lerp(Rough, 0.38, wet);
SpecOut = lerp(Spec, 0.5, wet);
// Light through the blades is a daylight effect; at night the moon and sky would make it glow.
Sub = tip * SubTint.rgb * lerp(NightTx, 1.0, smoothstep(0.05, 0.6, day)) * lerp(1.0, 0.7, wet);
return col;
"""


def build_material(v_flip):
    path = f"{FOLDER}/M_EstateGrass"
    material = unreal.load_asset(path) if LIB.does_asset_exist(path) else None
    if material is None:
        material = TOOLS.create_asset("M_EstateGrass", FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    for _ in range(8):
        if MEL.get_num_material_expressions(material) == 0:
            break
        MEL.delete_all_material_expressions(material)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
    material.set_editor_property("two_sided", True)
    material.set_editor_property("used_with_instanced_static_meshes", True)

    def node(cls, x, y, **props):
        e = MEL.create_material_expression(material, cls, x, y)
        for k, v in props.items():
            e.set_editor_property(k, v)
        return e

    def link(a, out, b, pin=""):
        if not MEL.connect_material_expressions(a, out, b, pin):
            raise RuntimeError(f"Could not wire {a.get_name()}.{out} -> {b.get_name()}.{pin}")

    def vector(name, value, x, y, group="Grass"):
        return node(unreal.MaterialExpressionVectorParameter, x, y, parameter_name=name,
                    default_value=unreal.LinearColor(*value), group=group)

    def scalar(name, value, x, y, group="Grass"):
        return node(unreal.MaterialExpressionScalarParameter, x, y, parameter_name=name, default_value=value, group=group)

    def texobj(name, tex, x, y, sampler):
        return node(unreal.MaterialExpressionTextureObjectParameter, x, y, parameter_name=name, texture=tex, sampler_type=sampler)

    F3, F4 = unreal.CustomMaterialOutputType.CMOT_FLOAT3, unreal.CustomMaterialOutputType.CMOT_FLOAT4

    def custom(code, inputs, x, y, extra=()):
        c = node(unreal.MaterialExpressionCustom, x, y, code=code, output_type=F3, description="EstateGrass")
        ins = []
        for n, _ in inputs:
            ci = unreal.CustomInput()
            ci.set_editor_property("input_name", n)
            ins.append(ci)
        c.set_editor_property("inputs", ins)
        outs = []
        for name, kind in extra:
            o = unreal.CustomOutput()
            o.set_editor_property("output_name", name)
            o.set_editor_property("output_type", kind)
            outs.append(o)
        c.set_editor_property("additional_outputs", outs)
        for n, (src, out) in inputs:
            link(src, out, c, n)
        return c

    ST = unreal.MaterialSamplerType
    ground = texobj("GroundData", unreal.load_asset(f"{FOLDER}/T_EstateGround"), -1600, -600, ST.SAMPLERTYPE_LINEAR_COLOR)
    road = texobj("RoadSDF", unreal.load_asset(ROAD_SDF), -1600, -450, ST.SAMPLERTYPE_LINEAR_GRAYSCALE)
    wind_tex = texobj("WindNoise", unreal.load_asset(f"{FOLDER}/T_GrassWind"), -1600, -300, ST.SAMPLERTYPE_LINEAR_GRAYSCALE)

    uv = node(unreal.MaterialExpressionTextureCoordinate, -1600, -900, coordinate_index=0)
    flip = scalar("RootVFlip", 1.0 if v_flip else 0.0, -1600, -800, "Data")
    root_local = custom(ROOT_LOCAL, [("UV", (uv, "")), ("VFlip", (flip, ""))], -1350, -850)
    root_world = node(unreal.MaterialExpressionTransformPosition, -1150, -850)
    root_world.set_editor_property("transform_source_type", unreal.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_LOCAL)
    root_world.set_editor_property("transform_type", unreal.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_WORLD)
    link(root_local, "", root_world, "")
    vtx = node(unreal.MaterialExpressionWorldPosition, -1150, -1000)
    vtx.set_editor_property("world_position_shader_offset", unreal.WorldPositionIncludedOffsets.WPT_EXCLUDE_ALL_SHADER_OFFSETS)
    cam = node(unreal.MaterialExpressionCameraPositionWS, -1150, -1100)
    time = node(unreal.MaterialExpressionTime, -1150, -1200)
    hero = node(unreal.MaterialExpressionCollectionParameter, -1150, -1300,
                collection=unreal.load_asset(MPC), parameter_name="HeroTargetPosition")
    cam_safe = node(unreal.MaterialExpressionCollectionParameter, -1150, -1400,
                    collection=unreal.load_asset(MPC), parameter_name="CameraPosition")
    data = [node(unreal.MaterialExpressionPerInstanceCustomData, -1400, -150 + 60 * i, data_index=i) for i in range(9)]

    wind = vector("GrassWind", (30.0, 1.0, 28.0, 2.2), -1600, 0)          # heading deg, strength, gust size m, gust speed m/s
    fade = vector("GrassFade", (12.0, 1.7, 42.0, 50.0), -1600, 100)        # full to m, falloff power, edge fade m..m
    shape = vector("GrassShape", (0.45, 1.0, 7.0, 4.0), -1600, 200)     # height at G=0, at G=1, widen from m, widen max
    push = vector("GrassPush", (75.0, 32.0, 95.0, 55.0), -1600, 300)      # radius cm, lean cm, camera clear cm, camera-line clear cm

    inputs = [("UV", (uv, "")), ("RootW", (root_world, "")), ("VtxW", (vtx, "")), ("Cam", (cam, "")), ("Time", (time, "")),
              ("Hero", (hero, "")), ("CamPos", (cam_safe, "")), ("GroundTex", (ground, "")), ("RoadTex", (road, "")), ("WindTex", (wind_tex, "")),
              ("Wind", (wind, "RGBA")), ("Fade", (fade, "RGBA")), ("Shape", (shape, "RGBA")), ("Push", (push, "RGBA"))]
    inputs += [(f"C{i}", (d, "")) for i, d in enumerate(data)]
    vs = custom(VERTEX_CODE, inputs, -800, -400, extra=(("Interp", F4),))
    interp = node(unreal.MaterialExpressionVertexInterpolator, -500, 100)
    link(vs, "Interp", interp, "VS")

    root_c = vector("RootColour", (0.022, 0.04, 0.014, 0), -500, 250, "Colour")
    tip_c = vector("TipColour", (0.085, 0.16, 0.04, 0), -500, 330, "Colour")
    dry_c = vector("DryColour", (0.3, 0.25, 0.11, 0), -500, 410, "Colour")
    sub_c = vector("SubsurfaceTint", (0.8, 1.0, 0.45, 0), -500, 490, "Colour")
    mpc = build_mpc()
    wet = node(unreal.MaterialExpressionCollectionParameter, -500, 570, collection=mpc, parameter_name="Wetness")
    day = node(unreal.MaterialExpressionCollectionParameter, -500, 640, collection=mpc, parameter_name="Daylight")
    F1 = unreal.CustomMaterialOutputType.CMOT_FLOAT1
    ps = custom(PIXEL_CODE, [("I", (interp, "PS")), ("Root", (root_c, "RGB")), ("Tip", (tip_c, "RGB")),
                             ("Dry", (dry_c, "RGB")), ("SubTint", (sub_c, "RGB")), ("Wet", (wet, "")), ("Day", (day, "")),
                             ("Rough", (scalar("Roughness", 0.72, -500, 710), "")), ("Spec", (scalar("Specular", 0.32, -500, 780), "")),
                             ("NightTx", (scalar("NightTransmission", 0.3, -500, 850), ""))],
                -200, 250, extra=(("Sub", F3), ("RoughOut", F1), ("SpecOut", F1)))
    P = unreal.MaterialProperty
    for src, out, prop in ((ps, "", P.MP_BASE_COLOR), (ps, "Sub", P.MP_SUBSURFACE_COLOR),
                           (ps, "RoughOut", P.MP_ROUGHNESS), (ps, "SpecOut", P.MP_SPECULAR),
                           (vs, "", P.MP_WORLD_POSITION_OFFSET)):
        if not MEL.connect_material_property(src, out, prop):
            raise RuntimeError(f"Could not connect {prop}")
    MEL.recompile_material(material)
    LIB.save_loaded_asset(material, False)
    stats = MEL.get_statistics(material)
    print("M_EstateGrass instructions: vs", stats.num_vertex_shader_instructions, "ps", stats.num_pixel_shader_instructions)
    return material


def build_instance(material):
    path = f"{FOLDER}/MI_EstateGrass"
    mi = unreal.load_asset(path) if LIB.does_asset_exist(path) else None
    if mi is None:
        mi = TOOLS.create_asset("MI_EstateGrass", FOLDER, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, material)
    LIB.save_loaded_asset(mi, False)
    return mi


def main():
    import_texture(os.path.join(REPO, "Saved", "Ground", "T_EstateGround.png"), "T_EstateGround", "data")
    import_texture(os.path.join(REPO, "Assets", "Environment", "Ground", "T_GrassWind.png"), "T_GrassWind", "gray")
    import_texture(os.path.join(REPO, "Saved", "Ground", "T_EstateCanopy.png"), "T_EstateCanopy", "data")
    import_polyhaven(os.environ.get("HOMESTEAD_GROUND_DOWNLOADS", os.path.join(REPO, "Saved", "Ground", "Downloads")))
    meshes = [import_patch(lod) for lod in range(3)]
    material = build_material(uv_flipped(meshes[0]))
    mi = build_instance(material)
    for m in meshes:
        m.set_material(0, mi)
        LIB.save_loaded_asset(m, False)
    print("estate grass built:", [SMS.get_number_verts(m, 0) for m in meshes], "verts per patch LOD")


if __name__ == "__main__":
    main()
