"""Crop produce material: the ripening fruit, roots, heads and pods drawn over each plot.

    from homestead_agent import crop_produce
    crop_produce.build()   # /Game/SurvivalGame/Materials/M_CropProduce, then re-parents MI_Crop*Produce

AHomesteadWorld::BuildPlot draws a crop's produce (SM_Crop<Name>_Produce) as instances on one
instanced mesh per plot, scaled with growth, and writes each instance's ripeness (0 unripe .. 1 ripe)
into per-instance custom data 0. This master lerps the baked (ripe-coloured) albedo from a pale
unripe tint by that value, so one mesh and one material show every stage. It keeps the foliage
parent's texture parameters (BaseColorTexture, NormalTexture, PackedTexture: R roughness, B AO), so
the import's MI_Crop<Name>Produce instances only need re-parenting. Produce doesn't sway.
"""
import unreal

MASTER = '/Game/SurvivalGame/Materials/M_CropProduce'
PROPS = '/Game/SurvivalGame/Environment/Props'
WHITE = '/Engine/EngineResources/WhiteSquareTexture'
FLAT_NORMAL = '/Engine/EngineMaterials/DefaultNormal'
# Unripe tint per crop (linear): what the produce looks like before it colours up.
UNRIPE = {
    'CropTurnip': (0.46, 0.55, 0.30),
    'CropCarrot': (0.34, 0.46, 0.16),
    'CropPotato': (0.42, 0.44, 0.26),
    'CropCabbage': (0.30, 0.48, 0.16),
    'CropBroadBean': (0.36, 0.52, 0.20),
    'CropStrawberry': (0.58, 0.64, 0.36),
}

LIB = unreal.MaterialEditingLibrary
ASSETS = unreal.EditorAssetLibrary


def _master():
    if ASSETS.does_asset_exist(MASTER):
        mat = unreal.load_asset(MASTER)
        LIB.delete_all_material_expressions(mat)
    else:
        folder, name = MASTER.rsplit('/', 1)
        mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, folder, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_MASKED)
    mat.set_editor_property('two_sided', True)
    mat.set_editor_property('opacity_mask_clip_value', 0.5)
    mat.set_editor_property('used_with_instanced_static_meshes', True)

    def node(cls, x, y):
        return LIB.create_material_expression(mat, cls, x, y)

    def link(a, out, b, pin):
        if not LIB.connect_material_expressions(a, out, b, pin):
            raise RuntimeError(f'Could not connect {a.get_name()}.{out} -> {b.get_name()}.{pin}')

    def sampler(param, default, kind, y):
        s = node(unreal.MaterialExpressionTextureSampleParameter2D, -1100, y)
        s.set_editor_property('parameter_name', param)
        s.set_editor_property('texture', default if not isinstance(default, str) else unreal.load_asset(default))
        s.set_editor_property('sampler_type', kind)
        return s

    base = sampler('BaseColorTexture', WHITE, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, -400)
    normal = sampler('NormalTexture', FLAT_NORMAL, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, 0)
    # A linear (non-sRGB) default: a Masks sampler rejects the engine's sRGB white.
    import import_props  # Scripts/Blender on sys.path (the crop import puts it there)
    packed = sampler('PackedTexture', import_props.mask_default(True), unreal.MaterialSamplerType.SAMPLERTYPE_MASKS, 250)

    # Unripe: the pale tint, shaded by the baked albedo's brightness so detail survives.
    lum_weights = node(unreal.MaterialExpressionConstant3Vector, -900, -250)
    lum_weights.set_editor_property('constant', unreal.LinearColor(0.3, 0.59, 0.11, 1.0))
    lum = node(unreal.MaterialExpressionDotProduct, -760, -300)
    link(base, 'RGB', lum, 'A')
    link(lum_weights, '', lum, 'B')
    lifted = node(unreal.MaterialExpressionMultiply, -640, -300)
    lifted.set_editor_property('const_b', 2.0)
    link(lum, '', lifted, 'A')
    bright = node(unreal.MaterialExpressionAdd, -540, -300)
    bright.set_editor_property('const_b', 0.35)
    link(lifted, '', bright, 'A')
    bright_sat = node(unreal.MaterialExpressionSaturate, -440, -300)
    link(bright, '', bright_sat, '')
    tint = node(unreal.MaterialExpressionVectorParameter, -640, -180)
    tint.set_editor_property('parameter_name', 'UnripeTint')
    tint.set_editor_property('default_value', unreal.LinearColor(0.4, 0.52, 0.22, 1.0))
    unripe = node(unreal.MaterialExpressionMultiply, -320, -260)
    link(tint, '', unripe, 'A')
    link(bright_sat, '', unripe, 'B')

    ripeness = node(unreal.MaterialExpressionPerInstanceCustomData, -640, -60)
    ripeness.set_editor_property('data_index', 0)
    ripeness.set_editor_property('const_default_value', 1.0)
    colour = node(unreal.MaterialExpressionLinearInterpolate, -160, -300)
    link(unripe, '', colour, 'A')
    link(base, 'RGB', colour, 'B')
    link(ripeness, '', colour, 'Alpha')
    LIB.connect_material_property(colour, '', unreal.MaterialProperty.MP_BASE_COLOR)
    LIB.connect_material_property(normal, 'RGB', unreal.MaterialProperty.MP_NORMAL)
    LIB.connect_material_property(packed, 'R', unreal.MaterialProperty.MP_ROUGHNESS)
    LIB.connect_material_property(packed, 'B', unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    LIB.connect_material_property(base, 'A', unreal.MaterialProperty.MP_OPACITY_MASK)
    LIB.recompile_material(mat)
    ASSETS.save_asset(MASTER)
    return mat


def build():
    """(Re)builds the master and re-parents every imported MI_Crop<Name>Produce to it."""
    master = _master()
    done = []
    for visual, tint in UNRIPE.items():
        name = visual + 'Produce'
        path = f'{PROPS}/{visual}/MI_{name}'
        if not ASSETS.does_asset_exist(path):
            continue
        mi = unreal.load_asset(path)
        LIB.set_material_instance_parent(mi, master)
        LIB.set_material_instance_vector_parameter_value(mi, 'UnripeTint', unreal.LinearColor(*tint, 1.0))
        LIB.update_material_instance(mi)
        ASSETS.save_asset(path)
        done.append(visual)
    return done
