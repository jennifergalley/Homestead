"""Engine-side materials for the oil lamp (add-oil-lamp): the chimney glass and the flame.

    from homestead_agent import lamp_materials as lm
    lm.build()   # M_OilLampGlass, M_OilLampFlame in /Game/SurvivalGame/Environment/Props/OilLamp

The Blender recipe (Scripts/Blender/Recipes/oil_lamp.py) exports the glass and flame as their own
meshes with no bake; HomesteadLampLook::AddParts puts these materials on them.
"""
import unreal

FOLDER = '/Game/SurvivalGame/Environment/Props/OilLamp'
MEL = unreal.MaterialEditingLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
LIB = unreal.EditorAssetLibrary


def _fresh(name):
    path = f'{FOLDER}/{name}'
    if LIB.does_asset_exist(path):
        LIB.delete_asset(path)
    return TOOLS.create_asset(name, FOLDER, unreal.Material, unreal.MaterialFactoryNew())


def _constant3(material, rgb, x, y):
    node = MEL.create_material_expression(material, unreal.MaterialExpressionConstant3Vector, x, y)
    node.set_editor_property('constant', unreal.LinearColor(*rgb, 1.0))
    return node


def _scalar(material, name, value, x, y):
    node = MEL.create_material_expression(material, unreal.MaterialExpressionScalarParameter, x, y)
    node.set_editor_property('parameter_name', name)
    node.set_editor_property('default_value', value)
    return node


def build_glass():
    """Thin blown glass, faintly green and a little smoky near the top: mostly clear, with
    specular highlights from the scene and the flame."""
    m = _fresh('M_OilLampGlass')
    m.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property('two_sided', True)
    m.set_editor_property('translucency_lighting_mode', unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
    color = _constant3(m, (0.78, 0.84, 0.80), -500, -200)
    MEL.connect_material_property(color, '', unreal.MaterialProperty.MP_BASE_COLOR)
    # Soot gathers toward the chimney's top: opacity rises with height in the mesh's own frame.
    uv = MEL.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -900, 100)
    mask = MEL.create_material_expression(m, unreal.MaterialExpressionComponentMask, -700, 100)
    mask.set_editor_property('g', True)
    mask.set_editor_property('r', False)
    MEL.connect_material_expressions(uv, '', mask, '')
    clear = _scalar(m, 'Opacity', 0.12, -700, 250)
    sooty = _scalar(m, 'SootOpacity', 0.28, -700, 350)
    lerp = MEL.create_material_expression(m, unreal.MaterialExpressionLinearInterpolate, -400, 200)
    MEL.connect_material_expressions(clear, '', lerp, 'A')
    MEL.connect_material_expressions(sooty, '', lerp, 'B')
    one_minus = MEL.create_material_expression(m, unreal.MaterialExpressionOneMinus, -550, 100)
    MEL.connect_material_expressions(mask, '', one_minus, '')
    power = MEL.create_material_expression(m, unreal.MaterialExpressionPower, -480, 100)
    MEL.connect_material_expressions(one_minus, '', power, 'Base')
    exponent = _scalar(m, 'SootFalloff', 4.0, -600, 20)
    MEL.connect_material_expressions(exponent, '', power, 'Exp')
    MEL.connect_material_expressions(power, '', lerp, 'Alpha')
    MEL.connect_material_property(lerp, '', unreal.MaterialProperty.MP_OPACITY)
    # The glass sits on lighting channel 2 only, so the flame's point light can't blow it out to
    # white; its warm glow comes from here instead. HomesteadLampLook::SetLit drives Glow (0 = out).
    # Translucent emissive is scaled by opacity, hence the strong GlowStrength.
    warm = _constant3(m, (1.0, 0.62, 0.30), -700, 520)
    glow = _scalar(m, 'Glow', 0.0, -700, 620)
    strength = _scalar(m, 'GlowStrength', 3.0, -700, 700)
    lit = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -500, 560)
    MEL.connect_material_expressions(warm, '', lit, 'A')
    MEL.connect_material_expressions(glow, '', lit, 'B')
    emissive = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -350, 600)
    MEL.connect_material_expressions(lit, '', emissive, 'A')
    MEL.connect_material_expressions(strength, '', emissive, 'B')
    MEL.connect_material_property(emissive, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.connect_material_property(_scalar(m, 'Roughness', 0.06, -400, 400), '', unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(_scalar(m, 'Specular', 0.6, -400, 480), '', unreal.MaterialProperty.MP_SPECULAR)
    MEL.recompile_material(m)
    LIB.save_loaded_asset(m, False)
    return m


def build_flame():
    """An unlit, additive flame: a white-yellow core fading to orange at the edges and tip."""
    m = _fresh('M_OilLampFlame')
    m.set_editor_property('blend_mode', unreal.BlendMode.BLEND_ADDITIVE)
    m.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property('two_sided', True)
    core = _constant3(m, (1.0, 0.86, 0.55), -700, -200)
    edge = _constant3(m, (1.0, 0.42, 0.10), -700, -50)
    fresnel = MEL.create_material_expression(m, unreal.MaterialExpressionFresnel, -700, 100)
    lerp = MEL.create_material_expression(m, unreal.MaterialExpressionLinearInterpolate, -450, -100)
    MEL.connect_material_expressions(core, '', lerp, 'A')
    MEL.connect_material_expressions(edge, '', lerp, 'B')
    MEL.connect_material_expressions(fresnel, '', lerp, 'Alpha')
    glow = _scalar(m, 'Glow', 14.0, -450, 100)
    multiply = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -250, 0)
    MEL.connect_material_expressions(lerp, '', multiply, 'A')
    MEL.connect_material_expressions(glow, '', multiply, 'B')
    MEL.connect_material_property(multiply, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(m)
    LIB.save_loaded_asset(m, False)
    return m


def build():
    glass = build_glass()
    flame = build_flame()
    for mesh_name, material in (('SM_OilLampGlass', glass), ('SM_OilLampFlame', flame)):
        mesh = LIB.load_asset(f'{FOLDER}/{mesh_name}')
        if mesh:
            mesh.set_material(0, material)
            LIB.save_loaded_asset(mesh, False)
    stats = [MEL.get_statistics(glass).num_pixel_shader_instructions,
             MEL.get_statistics(flame).num_pixel_shader_instructions]
    return stats
