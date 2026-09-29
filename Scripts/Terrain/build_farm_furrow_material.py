"""Builds M_FarmFurrowsGrass: the derelict farm's old crop ridges, grassed over.

The ridges (SM_FarmFurrows) must melt into the pasture around them, so this material samples the
Estate landscape's Pasture textures in world space with the same tiling, near/far blend and tint as
M_EstateLandscape (Scripts/Terrain/build_landscape_material.py), then lets a little bare soil from
the ridge bake show in the deepest furrow bottoms. AHomesteadDerelictFarm puts it on the furrow
instances. Run inside the editor (MCP run_python): exec(open(<this file>).read()).
"""
import unreal

MEL = unreal.MaterialEditingLibrary
PATH = '/Game/SurvivalGame/Environment/Props/FarmField'
NAME = 'M_FarmFurrowsGrass'
PASTURE = '/Game/Trials/GrassGround_20260921_01/Textures/T_GrassGround_'
FURROW = PATH + '/Textures/T_FarmFurrows_'
# Must match build_landscape_material.py: Pasture tiling (landscape quads of 100 cm), far = x4.3,
# near/far blend 0.35, tint; the landscape's origin, so the world-space pattern lines up with it.
TILE_CM = 3.0 * 100.0
FAR_CM = TILE_CM * 4.3
TINT = (0.46, 0.74, 0.38)
ORIGIN = 201600.0

mat = unreal.load_asset(f'{PATH}/{NAME}')
if mat is None:
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, PATH, unreal.Material, unreal.MaterialFactoryNew())
for e in list(MEL.get_material_expressions(mat)):
    MEL.delete_material_expression(mat, e)
mat.set_editor_property('used_with_nanite', True)
mat.set_editor_property('used_with_instanced_static_meshes', True)


def node(cls, x, y, **props):
    n = MEL.create_material_expression(mat, cls, x, y)
    for k, v in props.items():
        n.set_editor_property(k, v)
    return n


def link(a, a_out, b, b_in):
    MEL.connect_material_expressions(a, a_out, b, b_in)


world = node(unreal.MaterialExpressionWorldPosition, -1600, 0)
xy = node(unreal.MaterialExpressionComponentMask, -1450, 0, r=True, g=True, b=False, a=False)
link(world, '', xy, '')
shift = node(unreal.MaterialExpressionAdd, -1300, 0, const_b=ORIGIN)
link(xy, '', shift, 'A')


def uv(scale, y):
    d = node(unreal.MaterialExpressionDivide, -1150, y, const_b=scale)
    link(shift, '', d, 'A')
    return d


def sample(tex, coords, x, y, kind):
    t = node(unreal.MaterialExpressionTextureSample, x, y, texture=unreal.load_asset(tex), sampler_type=kind)
    link(coords, '', t, 'UVs')
    return t


near_uv, far_uv = uv(TILE_CM, -200), uv(FAR_CM, 0)
colour = unreal.MaterialSamplerType.SAMPLERTYPE_COLOR
near = sample(PASTURE + 'Diff', near_uv, -950, -300, colour)
far = sample(PASTURE + 'Diff', far_uv, -950, -100, colour)
mix = node(unreal.MaterialExpressionLinearInterpolate, -700, -200, const_alpha=0.35)
link(near, 'RGB', mix, 'A')
link(far, 'RGB', mix, 'B')
tint = node(unreal.MaterialExpressionConstant3Vector, -700, -50, constant=unreal.LinearColor(*TINT, 1.0))
grass = node(unreal.MaterialExpressionMultiply, -550, -150)
link(mix, '', grass, 'A')
link(tint, '', grass, 'B')

# The ridge bake: AO darkens the furrow bottoms, and there a little old soil shows through the sward.
ao = node(unreal.MaterialExpressionTextureSample, -950, 250, texture=unreal.load_asset(FURROW + 'ao'),
          sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
soil = node(unreal.MaterialExpressionTextureSample, -950, 450, texture=unreal.load_asset(FURROW + 'basecolor'),
            sampler_type=colour)
shade = node(unreal.MaterialExpressionLinearInterpolate, -700, 250, const_a=0.72, const_b=1.0)
link(ao, 'R', shade, 'Alpha')
shaded = node(unreal.MaterialExpressionMultiply, -400, -100)
link(grass, '', shaded, 'A')
link(shade, '', shaded, 'B')
# Soil only where AO is well below its ridge-top value: saturate(0.55 - ao) * 1.6.
bare = node(unreal.MaterialExpressionSubtract, -700, 400, const_a=0.55)
link(ao, 'R', bare, 'B')
bare_k = node(unreal.MaterialExpressionMultiply, -550, 400, const_b=1.6)
link(bare, '', bare_k, 'A')
bare_s = node(unreal.MaterialExpressionSaturate, -420, 400)
link(bare_k, '', bare_s, '')
base = node(unreal.MaterialExpressionLinearInterpolate, -250, 0)
link(shaded, '', base, 'A')
link(soil, 'RGB', base, 'B')
link(bare_s, '', base, 'Alpha')
MEL.connect_material_property(base, '', unreal.MaterialProperty.MP_BASE_COLOR)

normal = sample(PASTURE + 'NormalDX', near_uv, -950, 650, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
MEL.connect_material_property(normal, 'RGB', unreal.MaterialProperty.MP_NORMAL)
rough = sample(PASTURE + 'Roughness', near_uv, -950, 850, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
MEL.connect_material_property(rough, 'R', unreal.MaterialProperty.MP_ROUGHNESS)

MEL.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat)
print(NAME, 'built with', MEL.get_num_material_expressions(mat), 'expressions')
