"""Build the Estate ocean inside the running editor: textures, SM_EstateOcean, M_EstateOcean and the sea actor.

Run `python Scripts/Terrain/bake_ocean.py` first (it writes Saved/Ocean and Assets/Environment/Ocean),
then, with the Estate level loaded and PIE stopped:
  pyfile Scripts/Terrain/build_ocean.py            (McpHelpers)
Re-running re-authors the material graph in place and re-imports the textures and mesh.

The ocean is Single Layer Water. A few long analytic swells displace the mesh (world position offset)
and every pixel re-evaluates them. On top of that it samples a wind sea from a baked FFT ocean patch
that loops in time. There are three layers at unrelated scales, so the sea evolves with deep-water
dispersion and never slides. A capillary-ripple map and shore waves are added last; the shore-wave
crests follow the baked shore-distance field and break into foam over the shallows. Detail too fine
for a pixel to resolve is averaged away and its slope variance is moved into roughness, so the sea
far away turns into a soft sun glitter instead of shimmering.
"""
import json
import math
import os

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
FOLDER = "/Game/SurvivalGame/Estate/Water"
LIB = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
FRAME = json.load(open(os.path.join(REPO, "Saved", "Ocean", "ocean_bake.json")))

# Long swell components shared by the vertex (displacement) and pixel (normal) evaluation:
# (wavelength factor, direction offset in degrees, amplitude factor, phase).
SWELLS = [(1.0, 0.0, 0.55, 0.0), (0.73, 13.0, 0.36, 1.7), (0.56, -9.0, 0.24, 4.1), (0.43, 24.0, 0.16, 2.6)]


def import_texture(source, name, kind):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", source)
    task.set_editor_property("destination_path", FOLDER)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", False)
    TOOLS.import_asset_tasks([task])
    tex = unreal.load_asset(f"{FOLDER}/{name}")
    if not tex:
        raise RuntimeError(f"Could not import {source}")
    tex.set_editor_property("srgb", False)
    if kind == "normal":
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
    elif kind == "gray":
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_GRAYSCALE)
    else:  # data: uncompressed RGBA8, clamped to the baked frame
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_VECTOR_DISPLACEMENTMAP)
        tex.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP)
        tex.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
    tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD)
    LIB.save_loaded_asset(tex, False)
    return tex


def import_mesh():
    path = f"{FOLDER}/SM_EstateOcean"
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", os.path.join(REPO, "Saved", "Ocean", "SM_EstateOcean.obj"))
    task.set_editor_property("destination_path", FOLDER)
    task.set_editor_property("destination_name", "SM_EstateOcean")
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", False)
    TOOLS.import_asset_tasks([task])
    mesh = unreal.load_asset(path)
    if not isinstance(mesh, unreal.StaticMesh):
        raise RuntimeError(f"OBJ import did not produce a static mesh: {task.get_editor_property('imported_object_paths')}")
    sms = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    settings = sms.get_lod_build_settings(mesh, 0)
    settings.set_editor_property("generate_lightmap_u_vs", False)
    settings.set_editor_property("recompute_normals", False)
    settings.set_editor_property("recompute_tangents", True)
    settings.set_editor_property("distance_field_resolution_scale", 0.0)
    sms.set_lod_build_settings(mesh, 0, settings)
    nanite = mesh.get_editor_property("nanite_settings")
    nanite.set_editor_property("enabled", False)
    mesh.set_editor_property("nanite_settings", nanite)
    sms.remove_collisions(mesh)
    for i in range(len(mesh.get_editor_property("static_materials"))):
        mesh.set_material(i, unreal.load_asset(f"{FOLDER}/M_EstateOcean") or unreal.load_asset("/Engine/EngineMaterials/DefaultMaterial"))
    LIB.save_loaded_asset(mesh, False)
    return mesh
def import_wave_volume():
    """T_OceanWaves.png (8x8 atlas of 128^2 frames) -> VT_OceanWaves, a 128x128x64 volume texture."""
    atlas = import_texture(os.path.join(REPO, "Assets", "Environment", "Ocean", "T_OceanWaves.png"), "T_OceanWavesAtlas", "data")
    atlas.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    atlas.set_editor_property("never_stream", True)
    LIB.save_loaded_asset(atlas, False)
    path = f"{FOLDER}/VT_OceanWaves"
    vol = unreal.load_asset(path) if LIB.does_asset_exist(path) else None
    if vol is None:
        vol = TOOLS.create_asset("VT_OceanWaves", FOLDER, unreal.VolumeTexture, unreal.VolumeTextureFactory())
    vol.set_editor_property("srgb", False)
    vol.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_VECTOR_DISPLACEMENTMAP)
    vol.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD)
    # Setting the source picks a default tile size (102 for a 1024^2 atlas), so set the tile size after it.
    vol.set_editor_property("source2d_texture", atlas)
    vol.set_editor_property("source2d_tile_size_x", 128)
    vol.set_editor_property("source2d_tile_size_y", 128)
    LIB.save_loaded_asset(vol, False)
    print("VT_OceanWaves from", vol.get_editor_property("source2d_texture").get_name(), "tile", vol.get_editor_property("source2d_tile_size_x"))
    return vol


# ---------------------------------------------------------------------------------------------
# HLSL. Units inside are metres and seconds; P arrives in world centimetres.

PRELUDE = """
float2 pm = P.xy * 0.01;
float2 uv = float2((pm.y - Frame.x) / Frame.y, (Frame.z - pm.x) / Frame.w);
float2 edge = max(abs(pm) - 2016.0, 0.0);
// Open sea beyond the map and beyond the baked frame (everything inside the map but outside the frame is land).
float2 edgeUV = max(max(-uv, uv - 1.0), 0.0) * float2(Frame.y, Frame.w);
float inMap = saturate(1.0 - max(length(edge), length(edgeUV)) / 300.0);
const float TAU = 6.2831853;
"""


def swell_code(evaluate_slope):
    lines = ["float swellH = 0; float2 swellG = 0;"]
    for i, (lf, dang, af, ph) in enumerate(SWELLS):
        lines.append(f"""
{{
    float lam = Swell.y * {lf};
    float k = TAU / lam;
    float ang = radians(Swell.z + {dang});
    float2 d = float2(cos(ang), sin(ang));
    float a = Swell.x * {af};
    float th = k * dot(d, pm) - sqrt(9.81 * k) * Time + {ph};
    swellH += a * sin(th);
    {"swellG += a * k * d * cos(th) * saturate(lam / (fw * 6.0) - 0.5);" if evaluate_slope else ""}
}}""")
    return "\n".join(lines)


SHORE_WAVE = """
float shoreMask = saturate(1.0 - sd / ShoreW.w) * expo;
float setEnv = 0.6 + 0.4 * sin(Time * TAU / (ShoreW.y * 6.7) + pm.y * 0.0043 - pm.x * 0.0021);
float phi = sd / ShoreW.z + Time / ShoreW.y + 0.35 * sin(pm.x * 0.011 + pm.y * 0.017) + 0.25 * sin(pm.y * 0.029 - pm.x * 0.007) + 0.2 * sin(pm.x * 0.047 + pm.y * 0.071);
float f = frac(phi);
float front = pow(f, 14.0);
float back = exp(-6.0 * f);
float prof = max(front, back) - 0.2;
float dprof = front > back ? 14.0 * pow(f, 13.0) : -6.0 * back;
float shoreAmp = ShoreW.x * shoreMask * setEnv * smoothstep(8.0, 2.0, depth) * (0.35 + 0.65 * smoothstep(0.05, 1.2, depth));
"""

VERTEX_CODE = PRELUDE + """
float4 sb = Texture2DSampleLevel(ShoreTex, ShoreTexSampler, uv, 4.0);
float4 sf = Texture2DSampleLevel(ShoreTex, ShoreTexSampler, uv, 0.0);
// Past the baked frame the sea is open and deep, so the swell carries on to the horizon unchanged.
float depthB = lerp(32.0, sb.r * sb.r * 32.0, inMap);
float depth = lerp(32.0, sf.r * sf.r * 32.0, inMap);
float sd = lerp(512.0, sf.g * sf.g * 512.0, inMap);
float expo = sb.b;
float fw = 0.0;
""" + swell_code(False) + SHORE_WAVE + """
float h = swellH * saturate((depthB - 1.0) / 5.0) + shoreAmp * prof;
return float3(0.0, 0.0, h * 100.0);
"""

PIXEL_CODE = PRELUDE + """
float fw = max(max(length(ddx(pm)), length(ddy(pm))), 1e-4);
float4 sf = Texture2DSample(ShoreTex, ShoreTexSampler, uv);
float4 sb = Texture2DSampleLevel(ShoreTex, ShoreTexSampler, uv, 4.0);
float depth = lerp(32.0, sf.r * sf.r * 32.0, inMap);
float depthB = lerp(32.0, sb.r * sb.r * 32.0, inMap);
float sd = lerp(512.0, sf.g * sf.g * 512.0, inMap);
float expo = sb.b;
WaterDepth = depth;
""" + swell_code(True) + """
float2 g = swellG * saturate((depthB - 1.0) / 5.0);
""" + SHORE_WAVE + """
// Shore-wave slope: d(height)/d(phase) times the gradient of the shore distance.
float2 texel = float2(1.0 / 2048.0, 1.0 / 1024.0);
float gu1 = Texture2DSampleLevel(ShoreTex, ShoreTexSampler, uv + float2(texel.x, 0), 0).g;
float gu0 = Texture2DSampleLevel(ShoreTex, ShoreTexSampler, uv - float2(texel.x, 0), 0).g;
float gv1 = Texture2DSampleLevel(ShoreTex, ShoreTexSampler, uv + float2(0, texel.y), 0).g;
float gv0 = Texture2DSampleLevel(ShoreTex, ShoreTexSampler, uv - float2(0, texel.y), 0).g;
float2 gradSd = float2((gv0 * gv0 - gv1 * gv1) * 512.0 / (2.0 * texel.y * Frame.w),
                       (gu1 * gu1 - gu0 * gu0) * 512.0 / (2.0 * texel.x * Frame.y));
g += shoreAmp * dprof * gradSd / ShoreW.z * inMap;

// Wind sea: deep-water dispersion, spread around the wind direction, faded below pixel size.
// Gusts: slow drifting patches of rougher and glassier water (cat's paws), hundreds of metres across.
float gustN = Texture2DSampleLevel(FoamTex, FoamTexSampler, pm / Gust.y + Time * Gust.z * float2(cos(radians(Wind.w)), sin(radians(Wind.w))) / Gust.y, 0).r;
float gustM = Texture2DSampleLevel(FoamTex, FoamTexSampler, pm / (Gust.y * 0.37) + 0.41 - Time * Gust.z * 0.6 / Gust.y, 0).r;
float gust = lerp(1.0, saturate(0.25 + 1.5 * (gustN * 0.7 + gustM * 0.3)), Gust.x);
float chop = lerp(0.35, 1.0, saturate(depth / 1.5)) * lerp(0.45, 1.0, expo) * gust;
float lost = 0.0;
float caps = 0.0;
// Three layers of the looping FFT patch at unrelated scales and headings, so neither the 48 m tile
// nor the 16 s loop shows. Scaling a patch by s keeps its slopes; its clock runs 1/sqrt(s) as fast,
// which keeps deep-water dispersion right. Stored slopes decode to an RMS of 0.25.
[unroll] for (int i = 0; i < 3; i++)
{
    float s = i == 0 ? 1.0 : (i == 1 ? 0.37 : 2.7);
    float off = i == 0 ? 0.0 : (i == 1 ? 41.0 : -23.0);
    float amp = i == 0 ? 0.62 : (i == 1 ? 0.5 : 0.45);
    float a = radians(Wind.w + off);
    float2 d = float2(cos(a), sin(a));
    float2 e = float2(-d.y, d.x);
    float tileM = Wind.y * s;
    float3 uvw = float3(dot(pm, d) / tileM + 0.31 * i, dot(pm, e) / tileM + 0.17 * i,
                        frac(Time / (Wind.z * sqrt(s)) + 0.29 * i));
    float4 w = Texture3DSample(WaveVol, WaveVolSampler, uvw);
    float2 sl = (w.rg * 2.0 - 1.0) * 4.0 * Wind.x * amp * chop;
    g += sl.x * d + sl.y * e;
    // Variance the mip chain has averaged away (about 4.7 equal octaves between the patch's peak and
    // its finest wave) goes to roughness instead.
    float lostFrac = saturate(log2(fw * 2.0 / (0.75 * s)) / 4.7);
    float v = Wind.x * amp * chop;
    lost += v * v * lostFrac;
    caps += w.b * amp * (1.0 - lostFrac);
}
// Capillary ripples from the tiling map: two layers at unrelated angles and scales so their tiles
// never line up into a grid, drifting downwind. Kept faint; the FFT patch carries the real waves.
float2 rs = 0;
[unroll] for (int j = 0; j < 2; j++)
{
    float ra = radians(Wind.w + (j == 0 ? 17.0 : -61.0));
    float2 rd = float2(cos(ra), sin(ra));
    float2 re = float2(-rd.y, rd.x);
    float rt = Micro.y * (j == 0 ? 1.0 : 0.43);
    float2 ruv = float2(dot(pm, rd), dot(pm, re)) / rt + float2(Time * Micro.z * (j == 0 ? 1.0 : 1.6) / Micro.y * 2.4, 0.37 * j);
    float3 r = Texture2DSample(RippleTex, RippleTexSampler, ruv).xyz * 2.0 - 1.0;
    float2 rsl = r.xy / max(r.z, 0.2);
    rs += (rsl.x * rd + rsl.y * re) * 0.5;
}
g += rs * Micro.x * chop;
LostSlope = sqrt(lost);

// Foam lace drifts shoreward with the wash: a two-phase flow map keeps the offsets bounded.
float2 flow = -gradSd * 0.45;
float fp0 = frac(Time * 0.22);
float fp1 = frac(Time * 0.22 + 0.5);
float fw1 = abs(fp0 - 0.5) * 2.0;
float2 o0 = flow * (fp0 - 0.5) * 4.5;
float2 o1 = flow * (fp1 - 0.5) * 4.5;
float t1 = lerp(Texture2DSample(FoamTex, FoamTexSampler, (pm - o0) / 6.1).r,
                Texture2DSample(FoamTex, FoamTexSampler, (pm - o1) / 6.1 + 0.5).r, fw1);
float t2 = lerp(Texture2DSample(FoamTex, FoamTexSampler, (pm - o0) / 2.3 + 0.37).r,
                Texture2DSample(FoamTex, FoamTexSampler, (pm - o1) / 2.3 + 0.87).r, fw1);
float lace = saturate(t1 * 0.65 + t2 * 0.55);
// Swash: each wave that reaches the beach (f wraps to 0 there) throws a sheet of white water up the
// sand that thins into lace as it drains; a thin line of bubbles always lingers at the waterline.
float fresh = exp(-3.0 * f);
float swash = smoothstep(FoamP.x * (0.35 + 0.65 * fresh), 0.0, depth) * (0.25 + 0.75 * fresh) * lerp(0.6, 1.0, expo);
float linger = 0.4 * smoothstep(0.18, 0.0, depth);
float breakZone = smoothstep(FoamP.y, 0.8, depth) * saturate(1.0 - sd / ShoreW.w) * expo * setEnv;
float trail = exp(-5.0 * f) + 0.6 * front;
// Whitecaps: the most folded crests of the wind sea, more of them in the gusts.
float whitecap = saturate((caps * gust - Caps.y) * Caps.x);
float cover = saturate(saturate(swash + linger + breakZone * trail * FoamP.z) * inMap + whitecap);
FoamAmt = saturate((lace + cover * 1.25 - 1.0) * FoamP.w) * saturate(cover * 2.5);
// Keep reflections above the horizon: a facet tilted so far that the mirrored view ray points into
// the sea would pick up black from the reflection trace, so flatten it just enough.
float3 n = normalize(float3(-g, 1.0));
[unroll] for (int j = 0; j < 4; j++)
{
    float3 rv = reflect(-ViewDir, n);
    if (rv.z < 0.03) n = normalize(float3(n.xy * 0.6, n.z));
}
return n;
"""


def build_material():
    path = f"{FOLDER}/M_EstateOcean"
    material = unreal.load_asset(path) if LIB.does_asset_exist(path) else None
    if material is None:
        material = TOOLS.create_asset("M_EstateOcean", FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    # delete_all_material_expressions can leave some behind (it removed about half of a 71-node graph), so repeat.
    for _ in range(8):
        if MEL.get_num_material_expressions(material) == 0:
            break
        MEL.delete_all_material_expressions(material)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_SINGLE_LAYER_WATER)
    material.set_editor_property("refraction_method", unreal.RefractionMode.RM_PIXEL_NORMAL_OFFSET)
    material.set_editor_property("tangent_space_normal", False)
    material.set_editor_property("two_sided", False)

    def node(cls, x, y, **props):
        e = MEL.create_material_expression(material, cls, x, y)
        for k, v in props.items():
            e.set_editor_property(k, v)
        return e

    def link(a, out, b, pin=""):
        if not MEL.connect_material_expressions(a, out, b, pin):
            raise RuntimeError(f"Could not wire {a.get_name()}.{out} -> {b.get_name()}.{pin}")

    def scalar(name, value, x, y, group="Ocean"):
        return node(unreal.MaterialExpressionScalarParameter, x, y, parameter_name=name, default_value=value, group=group)

    def vector(name, value, x, y, group="Ocean"):
        return node(unreal.MaterialExpressionVectorParameter, x, y, parameter_name=name,
                    default_value=unreal.LinearColor(*value), group=group)

    def texobj(name, tex, x, y, sampler):
        return node(unreal.MaterialExpressionTextureObjectParameter, x, y, parameter_name=name, texture=tex, sampler_type=sampler)

    shore_tex = unreal.load_asset(f"{FOLDER}/T_EstateOceanShore")
    ripples_tex = unreal.load_asset(f"{FOLDER}/T_OceanRipples_N")
    foam_tex = unreal.load_asset(f"{FOLDER}/T_OceanFoam")
    ST = unreal.MaterialSamplerType
    shore_v = texobj("ShoreData", shore_tex, -1400, -400, ST.SAMPLERTYPE_LINEAR_COLOR)
    shore_p = texobj("ShoreDataPixel", shore_tex, -1400, 200, ST.SAMPLERTYPE_LINEAR_COLOR)
    ripples = texobj("RippleNormals", ripples_tex, -1400, 400, ST.SAMPLERTYPE_NORMAL)
    foam = texobj("FoamPattern", foam_tex, -1400, 600, ST.SAMPLERTYPE_LINEAR_GRAYSCALE)
    waves = texobj("WindSeaVolume", unreal.load_asset(f"{FOLDER}/VT_OceanWaves"), -1400, 700, ST.SAMPLERTYPE_LINEAR_COLOR)

    frame = vector("ShoreFrame", (FRAME["ShoreY0"], FRAME["ShoreSizeY"], FRAME["ShoreX1"], FRAME["ShoreSizeX"]), -1400, -250, "Data")
    swell = vector("Swell", (0.32, 78.0, 12.0, 0.0), -1400, -100, "Waves")          # height m, wavelength m, heading deg
    wind = vector("WindSea", (0.12, 48.0, 16.0, 30.0), -1400, 0, "Waves")         # slope RMS, patch m, loop s, heading deg
    shore_w = vector("ShoreWaves", (0.24, 9.0, 21.0, 140.0), -1400, 100, "Waves")  # height m, period s, crest spacing m, reach m
    micro = vector("Ripples", (0.035, 2.4, 0.03, 0.0), -1400, 300, "Waves")          # slope, tile m, drift per s
    caps = vector("Whitecaps", (1.6, 0.35, 0.0, 0.0), -1400, 250, "Foam")        # strength, threshold
    gust = vector("Gusts", (0.7, 260.0, 1.5, 0.0), -1400, 200, "Waves")            # strength, patch size m, drift m/s
    foam_p = vector("FoamShape", (0.7, 2.8, 0.85, 2.6), -1400, 800, "Foam")        # swash depth, surf depth, surf strength, sharpness

    time = node(unreal.MaterialExpressionTime, -1400, -550)
    wp_v = node(unreal.MaterialExpressionWorldPosition, -1400, -650)
    wp_p = node(unreal.MaterialExpressionWorldPosition, -1400, -750)
    wp_p.set_editor_property("world_position_shader_offset", unreal.WorldPositionIncludedOffsets.WPT_EXCLUDE_ALL_SHADER_OFFSETS)
    cam_v = node(unreal.MaterialExpressionCameraVectorWS, -1400, -850)

    F1, F3 = unreal.CustomMaterialOutputType.CMOT_FLOAT1, unreal.CustomMaterialOutputType.CMOT_FLOAT3

    def custom(code, inputs, x, y, extra=()):
        c = node(unreal.MaterialExpressionCustom, x, y, code=code, output_type=F3, description="EstateOcean")
        ins = []
        for n, _ in inputs:
            ci = unreal.CustomInput()
            ci.set_editor_property("input_name", n)
            ins.append(ci)
        c.set_editor_property("inputs", ins)
        outs = []
        for name in extra:
            o = unreal.CustomOutput()
            o.set_editor_property("output_name", name)
            o.set_editor_property("output_type", F1)
            outs.append(o)
        c.set_editor_property("additional_outputs", outs)
        for n, (src, out) in inputs:
            link(src, out, c, n)
        return c

    vs = custom(VERTEX_CODE, [("P", (wp_v, "")), ("Time", (time, "")), ("ShoreTex", (shore_v, "")),
                              ("Frame", (frame, "RGBA")), ("Swell", (swell, "RGBA")), ("ShoreW", (shore_w, "RGBA"))], -900, -500)
    ps = custom(PIXEL_CODE, [("P", (wp_p, "")), ("ViewDir", (cam_v, "")), ("Time", (time, "")), ("ShoreTex", (shore_p, "")),
                             ("RippleTex", (ripples, "")), ("FoamTex", (foam, "")), ("Frame", (frame, "RGBA")),
                             ("Swell", (swell, "RGBA")), ("Wind", (wind, "RGBA")), ("ShoreW", (shore_w, "RGBA")),
                             ("Micro", (micro, "RGBA")), ("FoamP", (foam_p, "RGBA")), ("Gust", (gust, "RGBA")), ("WaveVol", (waves, "")), ("Caps", (caps, "RGBA"))], -900, 100,
                extra=("FoamAmt", "WaterDepth", "LostSlope"))

    # Surface response.
    foam_color = vector("FoamColor", (0.78, 0.8, 0.78, 0), -500, 300, "Foam")
    base = node(unreal.MaterialExpressionMultiply, -300, 300)
    link(foam_color, "", base, "A")
    link(ps, "FoamAmt", base, "B")

    rough_base = scalar("Roughness", 0.035, -500, 450)
    rough_slope = scalar("RoughnessFromLostSlope", 0.9, -500, 520)
    lost = node(unreal.MaterialExpressionMultiply, -350, 500)
    link(ps, "LostSlope", lost, "A")
    link(rough_slope, "", lost, "B")
    rough_sum = node(unreal.MaterialExpressionAdd, -200, 480)
    link(rough_base, "", rough_sum, "A")
    link(lost, "", rough_sum, "B")
    rough_clamp = node(unreal.MaterialExpressionMin, -100, 480)
    link(rough_sum, "", rough_clamp, "A")
    link(scalar("RoughnessMax", 0.35, -300, 600), "", rough_clamp, "B")
    rough = node(unreal.MaterialExpressionLinearInterpolate, 50, 480)
    link(rough_clamp, "", rough, "A")
    link(scalar("FoamRoughness", 0.6, -300, 660), "", rough, "B")
    link(ps, "FoamAmt", rough, "Alpha")

    # Refraction and specular soften over the last few centimetres of water.
    edge = node(unreal.MaterialExpressionSaturate, -300, 760)
    edge_scale = node(unreal.MaterialExpressionMultiply, -450, 760)
    link(ps, "WaterDepth", edge_scale, "A")
    link(scalar("EdgeFadeInvDepth", 4.0, -600, 800), "", edge_scale, "B")
    link(edge_scale, "", edge, "")
    refr = node(unreal.MaterialExpressionLinearInterpolate, 50, 760)
    link(scalar("EdgeRefraction", 1.0, -150, 820), "", refr, "A")
    link(scalar("Refraction", 1.15, -150, 880), "", refr, "B")
    link(edge, "", refr, "Alpha")
    spec = node(unreal.MaterialExpressionLinearInterpolate, 50, 640)
    link(scalar("ShoreSpecular", 0.2, -150, 940), "", spec, "A")
    link(scalar("Specular", 0.5, -150, 1000), "", spec, "B")
    link(edge, "", spec, "Alpha")

    water = node(unreal.MaterialExpressionSingleLayerWaterMaterialOutput, 400, 900)
    # Per-centimetre coefficients. Absorption takes red first, so sand shallows read turquoise; scattering is kept low so the open Atlantic reads deep blue-green rather than milky teal.
    link(vector("Scattering", (0.00006, 0.00025, 0.0004, 0), 100, 1100, "Colour"), "", water, "ScatteringCoefficients")
    link(vector("Absorption", (0.0040, 0.0009, 0.0007, 0), 100, 1200, "Colour"), "", water, "AbsorptionCoefficients")
    link(scalar("PhaseG", 0.25, 100, 1300, "Colour"), "", water, "PhaseG")
    link(scalar("ColorScaleBehindWater", 0.9, 100, 1360, "Colour"), "", water, "ColorScaleBehindWater")

    P = unreal.MaterialProperty
    for src, out, prop in ((base, "", P.MP_BASE_COLOR), (ps, "", P.MP_NORMAL), (ps, "FoamAmt", P.MP_OPACITY),
                           (spec, "", P.MP_SPECULAR), (rough, "", P.MP_ROUGHNESS), (refr, "", P.MP_REFRACTION),
                           (vs, "", P.MP_WORLD_POSITION_OFFSET)):
        if not MEL.connect_material_property(src, out, prop):
            raise RuntimeError(f"Could not connect {prop}")
    MEL.recompile_material(material)
    LIB.save_loaded_asset(material, False)
    stats = MEL.get_statistics(material)
    print("M_EstateOcean instructions: vs", stats.num_vertex_shader_instructions, "ps", stats.num_pixel_shader_instructions)
    return material


def build_instance(material):
    path = f"{FOLDER}/MI_EstateOcean"
    mi = unreal.load_asset(path) if LIB.does_asset_exist(path) else None
    if mi is None:
        mi = TOOLS.create_asset("MI_EstateOcean", FOLDER, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, material)
    LIB.save_loaded_asset(mi, False)
    return mi


def place_sea(mesh, mi):
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    sea = next((a for a in eas.get_all_level_actors() if a.get_actor_label() == "EstateSea"), None)
    if sea is None:
        sea = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, 0))
        sea.set_actor_label("EstateSea")
        sea.set_folder_path("Water")
    sea.tags = ["HomesteadSea"]
    sea.set_actor_location(unreal.Vector(0, 0, 0), False, False)
    sea.set_actor_rotation(unreal.Rotator(0, 0, 0), False)
    sea.set_actor_scale3d(unreal.Vector(1, 1, 1))
    try:
        sea.set_editor_property("is_spatially_loaded", False)
    except Exception as e:  # noqa: BLE001
        print("is_spatially_loaded:", e)
    smc = sea.static_mesh_component
    smc.set_static_mesh(mesh)
    smc.set_material(0, mi)
    smc.set_editor_property("cast_shadow", False)
    smc.set_editor_property("affect_distance_field_lighting", False)
    smc.set_collision_profile_name("NoCollision")
    return sea


def main():
    ocean = os.path.join(REPO, "Assets", "Environment", "Ocean")
    import_texture(os.path.join(REPO, "Saved", "Ocean", "T_EstateOceanShore.png"), "T_EstateOceanShore", "data")
    import_texture(os.path.join(ocean, "T_OceanRipples_N.png"), "T_OceanRipples_N", "normal")
    import_texture(os.path.join(ocean, "T_OceanFoam.png"), "T_OceanFoam", "gray")
    import_wave_volume()
    material = build_material()
    mi = build_instance(material)
    mesh = import_mesh()
    mesh.set_material(0, mi)
    LIB.save_loaded_asset(mesh, False)
    b = mesh.get_bounds()
    print("SM_EstateOcean bounds origin", b.origin, "extent", b.box_extent, "verts", unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem).get_number_verts(mesh, 0))
    place_sea(mesh, mi)
    print("ocean built")


if __name__ == "__main__":
    main()
