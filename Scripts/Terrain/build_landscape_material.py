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
MEL.delete_all_material_expressions(mat)

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
    MEL.connect_material_expressions(lerp, '', bc, 'Layer ' + n)
    MEL.connect_material_expressions(sample(n, 'N', s, -1000, y + 120, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL), 'RGB', nm, 'Layer ' + n)
    MEL.connect_material_expressions(sample(n, 'R', s, -1000, y + 180, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS), 'R', rg, 'Layer ' + n)
    y += 260
MEL.connect_material_property(bc, '', unreal.MaterialProperty.MP_BASE_COLOR)
MEL.connect_material_property(nm, '', unreal.MaterialProperty.MP_NORMAL)
MEL.connect_material_property(rg, '', unreal.MaterialProperty.MP_ROUGHNESS)
MEL.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat)
print('M_EstateLandscape rebuilt with', MEL.get_num_material_expressions(mat), 'expressions')
