"""The ripe-crop glint: a small, soft, slowly pulsing sparkle over a ripe plot.

    from homestead_agent import crop_glint
    crop_glint.build()   # /Game/SurvivalGame/Environment/Props/CropGlint/M_CropRipeGlint

An unlit, additive billboard material: a warm radial glow with a faint four-point star, pulsing
between about 25% and 100% every ~2.4 s. Each plot's glint takes its phase from its world position,
so a row of ripe plants doesn't flash in step. AHomesteadWorld::BuildPlot shows it on a
UMaterialBillboardComponent above ripe plants.
"""
import unreal

FOLDER = '/Game/SurvivalGame/Environment/Props/CropGlint'
NAME = 'M_CropRipeGlint'
# Warm, pale gold (HDR, linear): bright enough to catch the eye at 15 m in daylight, not a lamp.
COLOR = (2.2, 1.7, 0.8)
PERIOD = 2.4


def build():
    lib = unreal.MaterialEditingLibrary
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    path = f'{FOLDER}/{NAME}'
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)
    mat = tools.create_asset(NAME, FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_ADDITIVE)

    def node(cls, x, y):
        return lib.create_material_expression(mat, cls, x, y)

    # Radial falloff from the sprite's centre: (1 - 2|uv - 0.5|)^3.
    uv = node(unreal.MaterialExpressionTextureCoordinate, -1400, 0)
    half = node(unreal.MaterialExpressionConstant2Vector, -1400, 150)
    half.set_editor_property('r', 0.5)
    half.set_editor_property('g', 0.5)
    offset = node(unreal.MaterialExpressionSubtract, -1200, 0)
    lib.connect_material_expressions(uv, '', offset, 'A')
    lib.connect_material_expressions(half, '', offset, 'B')
    dist = node(unreal.MaterialExpressionLength, -1050, 0)
    lib.connect_material_expressions(offset, '', dist, '')
    scaled = node(unreal.MaterialExpressionMultiply, -900, 0)
    scaled.set_editor_property('const_b', 2.0)
    lib.connect_material_expressions(dist, '', scaled, 'A')
    inv = node(unreal.MaterialExpressionOneMinus, -750, 0)
    lib.connect_material_expressions(scaled, '', inv, '')
    sat = node(unreal.MaterialExpressionSaturate, -620, 0)
    lib.connect_material_expressions(inv, '', sat, '')
    glow = node(unreal.MaterialExpressionPower, -480, 0)
    glow.set_editor_property('const_exponent', 3.0)
    lib.connect_material_expressions(sat, '', glow, 'Base')

    # Four-point star: thin bright cross, (1 - k|x|)(1 - k|y|) style via min of axis distances.
    comp_x = node(unreal.MaterialExpressionComponentMask, -1050, 250)
    comp_x.set_editor_property('r', True)
    comp_y = node(unreal.MaterialExpressionComponentMask, -1050, 350)
    comp_y.set_editor_property('g', True)
    lib.connect_material_expressions(offset, '', comp_x, '')
    lib.connect_material_expressions(offset, '', comp_y, '')
    abs_x = node(unreal.MaterialExpressionAbs, -900, 250)
    abs_y = node(unreal.MaterialExpressionAbs, -900, 350)
    lib.connect_material_expressions(comp_x, '', abs_x, '')
    lib.connect_material_expressions(comp_y, '', abs_y, '')
    thin = node(unreal.MaterialExpressionMin, -750, 300)
    lib.connect_material_expressions(abs_x, '', thin, 'A')
    lib.connect_material_expressions(abs_y, '', thin, 'B')
    sharp = node(unreal.MaterialExpressionMultiply, -620, 300)
    sharp.set_editor_property('const_b', 30.0)
    lib.connect_material_expressions(thin, '', sharp, 'A')
    ray = node(unreal.MaterialExpressionOneMinus, -500, 300)
    lib.connect_material_expressions(sharp, '', ray, '')
    ray_sat = node(unreal.MaterialExpressionSaturate, -380, 300)
    lib.connect_material_expressions(ray, '', ray_sat, '')
    star = node(unreal.MaterialExpressionMultiply, -260, 250)
    lib.connect_material_expressions(ray_sat, '', star, 'A')
    lib.connect_material_expressions(sat, '', star, 'B')
    star_half = node(unreal.MaterialExpressionMultiply, -140, 250)
    star_half.set_editor_property('const_b', 0.6)
    lib.connect_material_expressions(star, '', star_half, 'A')
    shape = node(unreal.MaterialExpressionAdd, -120, 50)
    lib.connect_material_expressions(glow, '', shape, 'A')
    lib.connect_material_expressions(star_half, '', shape, 'B')

    # Pulse: 0.25 + 0.75 * (0.5 + 0.5 sin(2pi (t / PERIOD + phase))), phase from the object position.
    time = node(unreal.MaterialExpressionTime, -1400, 500)
    per = node(unreal.MaterialExpressionMultiply, -1250, 500)
    per.set_editor_property('const_b', 1.0 / PERIOD)
    lib.connect_material_expressions(time, '', per, 'A')
    pos = node(unreal.MaterialExpressionObjectPositionWS, -1400, 620)
    pos_x = node(unreal.MaterialExpressionComponentMask, -1250, 620)
    pos_x.set_editor_property('r', True)
    pos_x.set_editor_property('g', True)
    lib.connect_material_expressions(pos, '', pos_x, '')
    dot = node(unreal.MaterialExpressionDotProduct, -1100, 620)
    weights = node(unreal.MaterialExpressionConstant2Vector, -1250, 720)
    weights.set_editor_property('r', 0.0137)
    weights.set_editor_property('g', 0.0091)
    lib.connect_material_expressions(pos_x, '', dot, 'A')
    lib.connect_material_expressions(weights, '', dot, 'B')
    phase = node(unreal.MaterialExpressionAdd, -950, 550)
    lib.connect_material_expressions(per, '', phase, 'A')
    lib.connect_material_expressions(dot, '', phase, 'B')
    sine = node(unreal.MaterialExpressionSine, -820, 550)
    sine.set_editor_property('period', 1.0)
    lib.connect_material_expressions(phase, '', sine, '')
    pulse = node(unreal.MaterialExpressionMultiply, -690, 550)
    pulse.set_editor_property('const_b', 0.375)
    lib.connect_material_expressions(sine, '', pulse, 'A')
    pulse_up = node(unreal.MaterialExpressionAdd, -560, 550)
    pulse_up.set_editor_property('const_b', 0.625)
    lib.connect_material_expressions(pulse, '', pulse_up, 'A')

    color = node(unreal.MaterialExpressionConstant3Vector, -120, 400)
    color.set_editor_property('constant', unreal.LinearColor(*COLOR, 1.0))
    lit = node(unreal.MaterialExpressionMultiply, 40, 150)
    lib.connect_material_expressions(shape, '', lit, 'A')
    lib.connect_material_expressions(pulse_up, '', lit, 'B')
    out = node(unreal.MaterialExpressionMultiply, 180, 200)
    lib.connect_material_expressions(lit, '', out, 'A')
    lib.connect_material_expressions(color, '', out, 'B')
    lib.connect_material_property(out, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    lib.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(path)
    return mat
