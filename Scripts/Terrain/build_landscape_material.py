"""Build M_EstateLandscape: a seven-layer weight-blended landscape material. Run in the editor.

Each layer samples BaseColor at two tilings (near detail fading into a larger-scale sample with
distance, which hides repeats), Normal and Roughness. A ground-finish pass then adds broad colour
variation, dry grass, trodden soil, leaf litter and moss under the trees, stony soil on steep banks
and the sward's colour where the 3D meadow grows (T_EstateGround and T_EstateCanopy from
bake_ground.py; run build_ground.py first). Textures come from /Game/SurvivalGame/Estate/Landscape/Textures (CC0 Poly
Haven, see docs/asset-credits.md) except where TEXTURES overrides a layer.
"""
import unreal

MEL = unreal.MaterialEditingLibrary
PATH = '/Game/SurvivalGame/Estate/Landscape/M_EstateLandscape'
LAYERS = ['Pasture', 'WoodlandFloor', 'Moorland', 'DuneSand', 'Beach', 'CliffRock', 'DirtRoad']
TILING = {'Pasture': 3.0, 'WoodlandFloor': 4.0, 'Moorland': 5.0, 'DuneSand': 6.0, 'Beach': 5.0, 'CliffRock': 8.0, 'DirtRoad': 3.0}
# Linear multipliers on each layer's base colour. The CC0 grass scans are dry straw; spring Cornish
# pasture is lush green, and the moor is heather and olive rather than hay.
TINT = {'Pasture': (0.46, 0.74, 0.38), 'WoodlandFloor': (0.55, 0.74, 0.42), 'Moorland': (0.56, 0.58, 0.42),
        'DuneSand': (1.0, 1.0, 1.0), 'Beach': (1.0, 1.0, 1.0), 'CliffRock': (0.8, 0.8, 0.8), 'DirtRoad': (0.9, 0.85, 0.8)}
TEXTURES = {
    # The admitted woodland grass-ground set reads greener than any Poly Haven meadow.
    'Pasture': ('/Game/Trials/GrassGround_20260921_01/Textures/T_GrassGround_Diff',
                '/Game/Trials/GrassGround_20260921_01/Textures/T_GrassGround_NormalDX',
                '/Game/Trials/GrassGround_20260921_01/Textures/T_GrassGround_Roughness'),
}

def texture(layer, slot):
    if layer in TEXTURES:
        return unreal.load_asset(TEXTURES[layer]['DNR'.index(slot)])
    return unreal.load_asset(f'/Game/SurvivalGame/Estate/Landscape/Textures/T_Estate_{layer}_{slot}')

mat = unreal.load_asset(PATH)
if mat is None:
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_EstateLandscape', '/Game/SurvivalGame/Estate/Landscape',
                                                                 unreal.Material, unreal.MaterialFactoryNew())
# delete_all_material_expressions leaves some nodes behind on a rebuild (a second
# LandscapeGrassOutput then fails the compile), so remove every node explicitly.
MEL.delete_all_material_expressions(mat)
for e in list(MEL.get_material_expressions(mat)):
    MEL.delete_material_expression(mat, e)
assert MEL.get_num_material_expressions(mat) == 0, 'material still has nodes after clearing'

def blend(x, y):
    b = MEL.create_material_expression(mat, unreal.MaterialExpressionLandscapeLayerBlend, x, y)
    arr = []
    for i, n in enumerate(LAYERS):
        li = unreal.LayerBlendInput()
        li.set_editor_property('layer_name', n)
        li.set_editor_property('blend_type', unreal.LandscapeLayerBlendType.LB_WEIGHT_BLEND)
        li.set_editor_property('preview_weight', 1.0 if i == 0 else 0.0)
        arr.append(li)
    b.set_editor_property('layers', arr)
    return b

bc, nm, rg = blend(-300, -400), blend(-300, 0), blend(-300, 400)

# Ground finish, after the layers blend: the meadow ground from bake_ground.py (T_EstateGround: grass
# density, height, dryness and wear; T_EstateCanopy: tree cover), broad colour variation so no two
# fields match, trodden soil where she and the carts walk, leaf litter and moss under the trees, stony
# soil on steep banks, and the sward colour at range where the 3D grass has faded out.
GROUND = '/Game/SurvivalGame/Estate/Ground'
FINISH_HLSL = """
float4 gd = GroundTex;                     // R density, G height, B dryness, A wear (sampled in the graph)
float canopy = Canopy;
float dist = D * 0.01;
float3 n = normalize(NIn);
float3 col = BC;
float rough = R;

// Broad variation: 25 m and 110 m blotches of richer and paler green, and warmer, drier swales.
float v1 = Macro1 - 0.5, v2 = Macro2 - 0.5;
float grassy = saturate(gd.r * 1.4);
col *= 1.0 + v1 * 0.34 + v2 * 0.26;
col = lerp(col, col * float3(1.12, 1.02, 0.78), saturate(v2 * 1.6 + 0.2) * grassy * 0.5);

// Dry grass (south slopes, the moor edges): towards straw.
col = lerp(col, col * float3(1.35, 1.12, 0.62), gd.b * grassy * 0.55);

// Where 3D blades grow, the ground between them is the sward's shaded base; past their fade it takes
// on the blades' own colour so the meadow doesn't change colour where the grass mesh ends.
float3 sward = lerp(SwardNear.rgb, SwardFar.rgb, smoothstep(10.0, 42.0, dist)) * (1.0 + v1 * 0.45 + v2 * 0.3);
sward = lerp(sward, sward * float3(1.7, 1.25, 0.55), gd.b * 0.7);
col = lerp(col, sward, gd.r * SwardMix);
rough = lerp(rough, 0.85, gd.r * 0.6);

// Trodden soil with tufts (grass_path_2): the manor's approach, road shoulders, working sites.
float wear = saturate(gd.a * 1.25) * (0.8 + 0.4 * Macro1);
col = lerp(col, WearD * Tints[0].rgb, wear);
n = normalize(lerp(n, WearN, wear));
rough = lerp(rough, WearR, wear);

// Leaf litter and moss under the canopy: darker, browner, damp, with moss where it's thickest.
float moss = saturate((Macro1 - 0.45) * 3.0) * canopy;
float3 litter = lerp(LitterD * Tints[1].rgb, LitterD * Tints[2].rgb, moss);
col = lerp(col, litter, canopy * 0.85);
rough = lerp(rough, 0.72, canopy * 0.6);
n = normalize(lerp(n, LitterN, canopy * 0.7));

// Stony soil on steep banks that aren't painted cliff (baked from the slope): rocky_trail.
float steep = Stony;
col = lerp(col, RockD * Tints[3].rgb, steep * 0.8);
n = normalize(lerp(n, RockN, steep * 0.8));
rough = lerp(rough, RockR, steep * 0.8);

// The MVP woodland zone (T_EstateCanopy.B): the survival prototype's forest floor exactly as its
// M_GrassGroundBlend drew it: lerp(T_Ground* (brown mud and leaves), T_GrassGround*, w) at 3 m world
// tiling, where away from the creek w = 0.07 + 0.2 * patch (CreekGroundBlendWeight's far-bank value),
// so mostly leaf litter with patches of grass. Past 60 m it eases into a coarser copy to hide tiling.
float zone = saturate(Zone);
float patch = 0.5 + 0.5 * sin(WP.x * 0.0021) * cos(WP.y * 0.0017);
float mw = 0.07 + 0.20 * patch;
float3 mvp = lerp(MudD, MvpD, mw);
mvp = lerp(mvp, lerp(MudFarD, MvpFarD, mw), smoothstep(60.0, 160.0, dist) * 0.5);
col = lerp(col, mvp, zone);
n = normalize(lerp(n, normalize(lerp(MudN, MvpN, mw)), zone));
rough = lerp(rough, lerp(MudR, MvpR, mw), zone);

// Rain (MPC_EstateGround.Wetness): soil, litter and stone darken most, turf less; everything turns
// glossy, and trodden ground and wheel ruts hold a sheen of standing water.
float w = saturate(Wet);
float porous = saturate(1.0 - grassy * 0.6);
col *= lerp(1.0, lerp(0.8, 0.52, porous), w);
rough = lerp(rough, lerp(0.5, 0.3, porous), w);
float pools = saturate(wear * 1.4 + (0.5 - Macro1) * 0.6) * porous;
rough = lerp(rough, 0.08, w * smoothstep(0.55, 0.9, pools));
n = normalize(lerp(n, float3(0, 0, 1), w * smoothstep(0.6, 0.9, pools) * 0.8));

NormalOut = n;
RoughOut = rough;
#if GROUND_DEBUG
return float3(steep, wear, canopy);
#endif
return col;
"""
import os as _os
if _os.environ.get("GROUND_DEBUG"):
    FINISH_HLSL = "#define GROUND_DEBUG 1\n" + FINISH_HLSL


def ground_finish(bc, nm, rg, y0):
    def tex(path, x, y, kind, uvs, clamp=False):
        t = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureSample, x, y)
        t.set_editor_property('texture', unreal.load_asset(path))
        t.set_editor_property('sampler_type', kind)
        t.set_editor_property('sampler_source', unreal.SamplerSourceMode.SSM_CLAMP_WORLD_GROUP_SETTINGS if clamp
                              else unreal.SamplerSourceMode.SSM_WRAP_WORLD_GROUP_SETTINGS)
        MEL.connect_material_expressions(uvs, '', t, 'UVs')
        return t

    ST = unreal.MaterialSamplerType
    wp = MEL.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -1600, y0)
    xy = MEL.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -1500, y0)
    xy.set_editor_property('r', True); xy.set_editor_property('g', True)
    MEL.connect_material_expressions(wp, '', xy, '')

    def scaled(metres, x, y, offset=0.0):
        d = MEL.create_material_expression(mat, unreal.MaterialExpressionDivide, x, y)
        d.set_editor_property('const_b', metres * 100.0)
        MEL.connect_material_expressions(xy, '', d, 'A')
        if not offset:
            return d
        a = MEL.create_material_expression(mat, unreal.MaterialExpressionAdd, x + 80, y)
        a.set_editor_property('const_b', offset)
        MEL.connect_material_expressions(d, '', a, 'A')
        return a

    map_uv = MEL.create_material_expression(mat, unreal.MaterialExpressionAdd, -1400, y0 + 60)
    map_uv.set_editor_property('const_b', 201600.0)
    MEL.connect_material_expressions(xy, '', map_uv, 'A')
    map_uv2 = MEL.create_material_expression(mat, unreal.MaterialExpressionDivide, -1320, y0 + 60)
    map_uv2.set_editor_property('const_b', 403200.0)
    MEL.connect_material_expressions(map_uv, '', map_uv2, 'A')

    y = y0 + 150
    ground = tex(f'{GROUND}/T_EstateGround', -1100, y, ST.SAMPLERTYPE_LINEAR_COLOR, map_uv2, True)
    canopy = tex(f'{GROUND}/T_EstateCanopy', -1100, y + 60, ST.SAMPLERTYPE_LINEAR_COLOR, map_uv2, True)
    macro1 = tex(f'{GROUND}/T_GrassWind', -1100, y + 120, ST.SAMPLERTYPE_LINEAR_GRAYSCALE, scaled(25.0, -1300, y + 120))
    macro2 = tex(f'{GROUND}/T_GrassWind', -1100, y + 180, ST.SAMPLERTYPE_LINEAR_GRAYSCALE, scaled(110.0, -1300, y + 180, 0.37))
    wuv = scaled(3.2, -1300, y + 260)
    wear_d = tex(f'{GROUND}/T_Ground_Trodden_D', -1100, y + 240, ST.SAMPLERTYPE_COLOR, wuv)
    wear_n = tex(f'{GROUND}/T_Ground_Trodden_N', -1100, y + 300, ST.SAMPLERTYPE_NORMAL, wuv)
    wear_r = tex(f'{GROUND}/T_Ground_Trodden_R', -1100, y + 360, ST.SAMPLERTYPE_MASKS, wuv)
    luv = scaled(4.0, -1300, y + 420)
    litter_d = tex(texture('WoodlandFloor', 'D').get_path_name(), -1100, y + 420, ST.SAMPLERTYPE_COLOR, luv)
    litter_n = tex(texture('WoodlandFloor', 'N').get_path_name(), -1100, y + 480, ST.SAMPLERTYPE_NORMAL, luv)
    MVP = '/Game/Trials/GrassGround_20260921_01/Textures/T_GrassGround'
    muv = scaled(3.0, -1300, y + 700)
    mvp_d = tex(f'{MVP}_Diff', -1100, y + 700, ST.SAMPLERTYPE_COLOR, muv)
    mvp_n = tex(f'{MVP}_NormalDX', -1100, y + 760, ST.SAMPLERTYPE_NORMAL, muv)
    mvp_r = tex(f'{MVP}_Roughness', -1100, y + 820, ST.SAMPLERTYPE_MASKS, muv)
    mvp_far = tex(f'{MVP}_Diff', -1100, y + 880, ST.SAMPLERTYPE_COLOR, scaled(12.9, -1300, y + 880, 0.21))
    MUD = '/Game/SurvivalGame/Textures/T_Ground'
    mud_d = tex(f'{MUD}Color', -1100, y + 940, ST.SAMPLERTYPE_COLOR, muv)
    mud_n = tex(f'{MUD}Normal', -1100, y + 1000, ST.SAMPLERTYPE_NORMAL, muv)
    mud_r = tex(f'{MUD}Roughness', -1100, y + 1060, ST.SAMPLERTYPE_LINEAR_COLOR, muv)
    mud_far = tex(f'{MUD}Color', -1100, y + 1120, ST.SAMPLERTYPE_COLOR, scaled(12.9, -1300, y + 1120, 0.21))
    ruv = scaled(4.5, -1300, y + 560)
    rock_d = tex(f'{GROUND}/T_Ground_Stony_D', -1100, y + 540, ST.SAMPLERTYPE_COLOR, ruv)
    rock_n = tex(f'{GROUND}/T_Ground_Stony_N', -1100, y + 600, ST.SAMPLERTYPE_NORMAL, ruv)
    rock_r = tex(f'{GROUND}/T_Ground_Stony_R', -1100, y + 660, ST.SAMPLERTYPE_MASKS, ruv)
    depth = MEL.create_material_expression(mat, unreal.MaterialExpressionPixelDepth, -1100, y + 780)

    def vec(name, value, yy):
        v = MEL.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -800, yy)
        v.set_editor_property('parameter_name', name)
        v.set_editor_property('default_value', unreal.LinearColor(*value, 1.0))
        v.set_editor_property('group', 'Ground')
        return v

    def scal(name, value, yy):
        v = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -800, yy)
        v.set_editor_property('parameter_name', name)
        v.set_editor_property('default_value', value)
        v.set_editor_property('group', 'Ground')
        return v

    wet_p = MEL.create_material_expression(mat, unreal.MaterialExpressionCollectionParameter, -800, y + 1240)
    wet_p.set_editor_property('collection', unreal.load_asset(f'{GROUND}/MPC_EstateGround'))
    wet_p.set_editor_property('parameter_name', 'Wetness')
    sward_near = vec('SwardNear', (0.03, 0.055, 0.018), y + 820)
    sward_far = vec('SwardFar', (0.036, 0.066, 0.02), y + 880)
    tints = [vec('TintTrodden', (0.85, 0.8, 0.72), y + 940), vec('TintLitter', (0.4, 0.36, 0.3), y + 1000),
             vec('TintMoss', (0.42, 0.62, 0.3), y + 1060), vec('TintStony', (0.78, 0.74, 0.68), y + 1120)]

    c = MEL.create_material_expression(mat, unreal.MaterialExpressionCustom, -300, y0 + 300)
    code = FINISH_HLSL.replace('Tints[0]', 'T0').replace('Tints[1]', 'T1').replace('Tints[2]', 'T2').replace('Tints[3]', 'T3')
    c.set_editor_property('code', code)
    c.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    c.set_editor_property('description', 'GroundFinish')
    inputs = [('BC', bc, ''), ('NIn', nm, ''), ('R', rg, ''), ('WP', wp, ''), ('D', depth, ''),
              ('GroundTex', ground, 'RGBA'), ('Canopy', canopy, 'R'), ('Stony', canopy, 'G'), ('Zone', canopy, 'B'),
              ('MvpD', mvp_d, 'RGB'), ('MvpN', mvp_n, 'RGB'), ('MvpR', mvp_r, 'R'), ('MvpFarD', mvp_far, 'RGB'),
              ('MudD', mud_d, 'RGB'), ('MudN', mud_n, 'RGB'), ('MudR', mud_r, 'R'), ('MudFarD', mud_far, 'RGB'), ('Macro1', macro1, 'R'), ('Macro2', macro2, 'R'),
              ('WearD', wear_d, 'RGB'), ('WearN', wear_n, 'RGB'), ('WearR', wear_r, 'R'),
              ('LitterD', litter_d, 'RGB'), ('LitterN', litter_n, 'RGB'),
              ('RockD', rock_d, 'RGB'), ('RockN', rock_n, 'RGB'), ('RockR', rock_r, 'R'),
              ('SwardNear', sward_near, 'RGB'), ('SwardFar', sward_far, 'RGB'),
              ('SwardMix', scal('SwardMix', 0.8, y + 1180), ''),
              ('Wet', wet_p, ''),
              ('T0', tints[0], 'RGB'), ('T1', tints[1], 'RGB'), ('T2', tints[2], 'RGB'), ('T3', tints[3], 'RGB')]
    ins = []
    for name, *_ in inputs:
        ci = unreal.CustomInput(); ci.set_editor_property('input_name', name); ins.append(ci)
    c.set_editor_property('inputs', ins)
    outs = []
    for name, kind in (('NormalOut', unreal.CustomMaterialOutputType.CMOT_FLOAT3), ('RoughOut', unreal.CustomMaterialOutputType.CMOT_FLOAT1)):
        o = unreal.CustomOutput(); o.set_editor_property('output_name', name); o.set_editor_property('output_type', kind); outs.append(o)
    c.set_editor_property('additional_outputs', outs)
    for name, src, out in inputs:
        if not MEL.connect_material_expressions(src, out, c, name):
            raise RuntimeError(f'GroundFinish: could not wire {name}')
    return c

coords = {}
def uv(scale):
    if scale not in coords:
        c = MEL.create_material_expression(mat, unreal.MaterialExpressionLandscapeLayerCoords, -1600, -800 + 120 * len(coords))
        c.set_editor_property('mapping_scale', scale)
        coords[scale] = c
    return coords[scale]

def sample(layer, slot, scale, x, y, kind):
    t = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureSample, x, y)
    t.set_editor_property('texture', texture(layer, slot))
    t.set_editor_property('sampler_type', kind)
    t.set_editor_property('sampler_source', unreal.SamplerSourceMode.SSM_WRAP_WORLD_GROUP_SETTINGS)
    MEL.connect_material_expressions(uv(scale), '', t, 'UVs')
    return t

# Near detail fades into the 4.3x macro sample with distance, so the 3-8 m tiling is gone by 30-60 m.
depth = MEL.create_material_expression(mat, unreal.MaterialExpressionPixelDepth, -1600, -1300)
far_mix = MEL.create_material_expression(mat, unreal.MaterialExpressionCustom, -1400, -1300)
far_mix.set_editor_property('code', 'return lerp(0.22, 0.85, smoothstep(1200.0, 7000.0, D));')
far_mix.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT1)
_ci = unreal.CustomInput(); _ci.set_editor_property('input_name', 'D'); far_mix.set_editor_property('inputs', [_ci])
MEL.connect_material_expressions(depth, '', far_mix, 'D')

y = -900
colour, rough = {}, {}
for n in LAYERS:
    s = TILING[n]
    near = sample(n, 'D', s, -1200, y, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    far = sample(n, 'D', s * 4.3, -1200, y + 60, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    lerp = MEL.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -800, y)
    MEL.connect_material_expressions(near, 'RGB', lerp, 'A')
    MEL.connect_material_expressions(far, 'RGB', lerp, 'B')
    MEL.connect_material_expressions(far_mix, '', lerp, 'Alpha')
    tint = MEL.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -650, y)
    tint.set_editor_property('parameter_name', 'Tint_' + n)
    tint.set_editor_property('default_value', unreal.LinearColor(*TINT[n], 1.0))
    mul = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -500, y)
    MEL.connect_material_expressions(lerp, '', mul, 'A')
    MEL.connect_material_expressions(tint, 'RGB', mul, 'B')
    colour[n] = (mul, '')
    MEL.connect_material_expressions(sample(n, 'N', s, -1000, y + 120, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL), 'RGB', nm, 'Layer ' + n)
    rough[n] = (sample(n, 'R', s, -1000, y + 180, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS), 'R')
    y += 260

# Cart ruts on the estate road: two dark wheel tracks, a grass crown between them and grass creeping
# over the shoulders, drawn from the road's signed-distance texture (Scripts/Terrain/road_ruts.py).
ROAD_SDF = '/Game/SurvivalGame/Estate/Landscape/Textures/T_EstateRoadSDF'
RUTS_HLSL = '''
float d = (Sdf - 0.5) * 8.0;
float ad = abs(d);
float crown = 1.0 - smoothstep(0.30, 0.50, ad);
float rut = exp(-pow((ad - 0.78) / 0.17, 2.0));
float shoulder = smoothstep(0.98, 1.45, ad);
float3 track = Dirt * lerp(1.0, 0.55, rut);
float grass = max(crown * 0.85, shoulder * 0.9) * (1.0 - rut);
RoughOut = lerp(Rough, Rough * 0.72, rut);
return lerp(track, Grass, grass);
'''
sdf_tex = unreal.load_asset(ROAD_SDF)
if sdf_tex:
    wp = MEL.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -1600, y)
    uvx = MEL.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -1450, y)
    uvx.set_editor_property('r', True); uvx.set_editor_property('g', True)
    MEL.connect_material_expressions(wp, '', uvx, '')
    add = MEL.create_material_expression(mat, unreal.MaterialExpressionAdd, -1350, y)
    add.set_editor_property('const_b', 201600.0)
    MEL.connect_material_expressions(uvx, '', add, 'A')
    div = MEL.create_material_expression(mat, unreal.MaterialExpressionDivide, -1250, y)
    div.set_editor_property('const_b', 403200.0)
    MEL.connect_material_expressions(add, '', div, 'A')
    sdf = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -1100, y)
    sdf.set_editor_property('texture', sdf_tex)
    sdf.set_editor_property('sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE)
    sdf.set_editor_property('sampler_source', unreal.SamplerSourceMode.SSM_CLAMP_WORLD_GROUP_SETTINGS)
    MEL.connect_material_expressions(div, '', sdf, 'UVs')
    ruts = MEL.create_material_expression(mat, unreal.MaterialExpressionCustom, -350, y)
    ruts.set_editor_property('code', RUTS_HLSL)
    ruts.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    ins = []
    for name in ('Dirt', 'Grass', 'Rough', 'Sdf'):
        ci = unreal.CustomInput(); ci.set_editor_property('input_name', name); ins.append(ci)
    ruts.set_editor_property('inputs', ins)
    extra = unreal.CustomOutput(); extra.set_editor_property('output_name', 'RoughOut')
    extra.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    ruts.set_editor_property('additional_outputs', [extra])
    MEL.connect_material_expressions(colour['DirtRoad'][0], '', ruts, 'Dirt')
    MEL.connect_material_expressions(colour['Pasture'][0], '', ruts, 'Grass')
    MEL.connect_material_expressions(rough['DirtRoad'][0], 'R', ruts, 'Rough')
    MEL.connect_material_expressions(sdf, 'R', ruts, 'Sdf')
    colour['DirtRoad'] = (ruts, '')
    rough['DirtRoad'] = (ruts, 'RoughOut')
else:
    print('No', ROAD_SDF, '- import Scripts/Terrain/T_EstateRoadSDF.png first; road drawn without ruts')
for n in LAYERS:
    MEL.connect_material_expressions(colour[n][0], colour[n][1], bc, 'Layer ' + n)
    MEL.connect_material_expressions(rough[n][0], rough[n][1], rg, 'Layer ' + n)
finish = ground_finish(bc, nm, rg, y)
MEL.connect_material_property(finish, '', unreal.MaterialProperty.MP_BASE_COLOR)
MEL.connect_material_property(finish, 'NormalOut', unreal.MaterialProperty.MP_NORMAL)
MEL.connect_material_property(finish, 'RoughOut', unreal.MaterialProperty.MP_ROUGHNESS)
# Runtime grass: the landscape scatters these around the camera wherever the layer is painted.
GRASS = {
    'Pasture': [('/Game/Trials/GrassGround_20260921_01/Meshes/SM_GrassMedium01_mid_b', 45.0, 0.8, 1.25),
                ('/Game/Trials/GrassGround_20260921_01/Meshes/SM_GrassMedium01_small_b', 60.0, 0.8, 1.3),
                ('/Game/Trials/GrassGround_20260921_01/Meshes/SM_GrassMedium01_tall_a', 10.0, 0.8, 1.2),
                ('/Game/SurvivalGame/Environment/Props/GrassYarrowTuft/SM_GrassYarrowTuft', 1.2, 0.8, 1.2)],
    'Moorland': [('/Game/SurvivalGame/Environment/Props/BrackenFern/SM_BrackenFern', 0.35, 0.7, 1.1),
                 ('/Game/Trials/GrassGround_20260921_01/Meshes/SM_GrassMedium01_small_b', 12.0, 0.7, 1.1)],
    'WoodlandFloor': [('/Game/Trials/Fern02_20260920_01/Meshes/SM_Fern02_a', 0.6, 0.8, 1.2),
                      ('/Game/Trials/GrassGround_20260921_01/Meshes/SM_GrassMedium01_small_b', 6.0, 0.8, 1.2)],
}
# Off for now: in UE 5.8 PIE the proxies create GrassInstancedStaticMeshComponents but they stay
# empty, with or without grass.GrassMap.UseRuntimeGeneration. Near-camera grass is still to do.
LANDSCAPE_GRASS = False
if LANDSCAPE_GRASS:
    grass_out = MEL.create_material_expression(mat, unreal.MaterialExpressionLandscapeGrassOutput, 200, 800)
    inputs = []
    for i, (layer, varieties) in enumerate(GRASS.items()):
        path = f'/Game/SurvivalGame/Estate/Landscape/LGT_{layer}'
        gt = unreal.load_asset(path) or unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            f'LGT_{layer}', '/Game/SurvivalGame/Estate/Landscape', unreal.LandscapeGrassType, None)
        vs = []
        for mesh, density, smin, smax in varieties:
            v = unreal.GrassVariety()
            v.set_editor_property('grass_mesh', unreal.load_asset(mesh))
            d = v.get_editor_property('grass_density'); d.set_editor_property('default', density); v.set_editor_property('grass_density', d)
            sc = v.get_editor_property('scale_x'); sc.set_editor_property('min', smin); sc.set_editor_property('max', smax); v.set_editor_property('scale_x', sc)
            v.set_editor_property('scaling', unreal.GrassScaling.UNIFORM)
            v.set_editor_property('random_rotation', True)
            v.set_editor_property('align_to_surface', True)
            v.set_editor_property('use_grid', False)
            sd = v.get_editor_property('start_cull_distance'); sd.set_editor_property('default', 2500); v.set_editor_property('start_cull_distance', sd)
            ed = v.get_editor_property('end_cull_distance'); ed.set_editor_property('default', 4500 if density > 3 else 9000); v.set_editor_property('end_cull_distance', ed)
            v.set_editor_property('cast_dynamic_shadow', density < 3)
            vs.append(v)
        gt.set_editor_property('grass_varieties', vs)
        unreal.EditorAssetLibrary.save_loaded_asset(gt)
        gi = unreal.GrassInput(); gi.set_editor_property('name', layer); gi.set_editor_property('grass_type', gt)
        inputs.append(gi)
    grass_out.set_editor_property('grass_types', inputs)
    for i, (layer, _) in enumerate(GRASS.items()):
        s = MEL.create_material_expression(mat, unreal.MaterialExpressionLandscapeLayerSample, -100, 800 + i * 120)
        s.set_editor_property('parameter_name', layer)
        MEL.connect_material_expressions(s, '', grass_out, layer)
MEL.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat)
print('M_EstateLandscape rebuilt with', MEL.get_num_material_expressions(mat), 'expressions')
