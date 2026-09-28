"""Build M_EstateLandscape: a seven-layer weight-blended landscape material. Run in the editor.

Each layer samples BaseColor at two tilings (near detail plus a larger-scale tint that hides repeats),
Normal and Roughness. Textures come from /Game/SurvivalGame/Estate/Landscape/Textures (CC0 Poly
Haven, see docs/asset-credits.md) except where TEXTURES overrides a layer.
"""
import unreal

MEL = unreal.MaterialEditingLibrary
PATH = '/Game/SurvivalGame/Estate/Landscape/M_EstateLandscape'
LAYERS = ['Pasture', 'WoodlandFloor', 'Moorland', 'DuneSand', 'Beach', 'CliffRock', 'DirtRoad']
TILING = {'Pasture': 3.0, 'WoodlandFloor': 4.0, 'Moorland': 5.0, 'DuneSand': 6.0, 'Beach': 5.0, 'CliffRock': 8.0, 'DirtRoad': 3.0}
# Linear multipliers on each layer's base colour. The CC0 grass scans are dry straw; spring Cornish
# pasture is lush green, and the moor is heather and olive rather than hay.
TINT = {'Pasture': (0.46, 0.74, 0.38), 'WoodlandFloor': (0.85, 0.95, 0.7), 'Moorland': (0.78, 0.72, 0.62),
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

y = -900
for n in LAYERS:
    s = TILING[n]
    near = sample(n, 'D', s, -1200, y, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    far = sample(n, 'D', s * 4.3, -1200, y + 60, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    lerp = MEL.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -800, y)
    lerp.set_editor_property('const_alpha', 0.35)
    MEL.connect_material_expressions(near, 'RGB', lerp, 'A')
    MEL.connect_material_expressions(far, 'RGB', lerp, 'B')
    tint = MEL.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -650, y)
    tint.set_editor_property('parameter_name', 'Tint_' + n)
    tint.set_editor_property('default_value', unreal.LinearColor(*TINT[n], 1.0))
    mul = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -500, y)
    MEL.connect_material_expressions(lerp, '', mul, 'A')
    MEL.connect_material_expressions(tint, 'RGB', mul, 'B')
    MEL.connect_material_expressions(mul, '', bc, 'Layer ' + n)
    MEL.connect_material_expressions(sample(n, 'N', s, -1000, y + 120, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL), 'RGB', nm, 'Layer ' + n)
    MEL.connect_material_expressions(sample(n, 'R', s, -1000, y + 180, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS), 'R', rg, 'Layer ' + n)
    y += 260
MEL.connect_material_property(bc, '', unreal.MaterialProperty.MP_BASE_COLOR)
MEL.connect_material_property(nm, '', unreal.MaterialProperty.MP_NORMAL)
MEL.connect_material_property(rg, '', unreal.MaterialProperty.MP_ROUGHNESS)
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
