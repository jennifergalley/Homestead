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
        # Rebuild in place: deleting a loaded asset (in PIE) raises a modal overwrite prompt.
        mat = unreal.load_asset(path)
        lib.delete_all_material_expressions(mat)
    else:
        mat = tools.create_asset(NAME, FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_ADDITIVE)

    def node(cls, x, y):
        return lib.create_material_expression(mat, cls, x, y)

    def link(a, b, pin=''):
        if not lib.connect_material_expressions(a, '', b, pin):
            raise RuntimeError(f'Could not connect {a.get_name()} -> {b.get_name()}.{pin}')

    def mask(src, channel, x, y):
        m = node(unreal.MaterialExpressionComponentMask, x, y)
        for c in 'rgba':
            m.set_editor_property(c, c == channel)
        link(src, m)
        return m

    # Radial glow from the sprite's centre: saturate(1 - 2 * distance(uv, 0.5))^3.
    uv = node(unreal.MaterialExpressionTextureCoordinate, -1400, 0)
    half = node(unreal.MaterialExpressionConstant2Vector, -1400, 150)
    half.set_editor_property('r', 0.5)
    half.set_editor_property('g', 0.5)
    dist = node(unreal.MaterialExpressionDistance, -1200, 0)
    link(uv, dist, 'A')
    link(half, dist, 'B')
    scaled = node(unreal.MaterialExpressionMultiply, -1050, 0)
    scaled.set_editor_property('const_b', 2.0)
    link(dist, scaled, 'A')
    inv = node(unreal.MaterialExpressionOneMinus, -900, 0)
    link(scaled, inv)
    sat = node(unreal.MaterialExpressionSaturate, -780, 0)
    link(inv, sat)
    glow = node(unreal.MaterialExpressionPower, -660, 0)
    glow.set_editor_property('const_exponent', 3.0)
    link(sat, glow, 'Base')

    # Faint four-point star: a thin cross, brightest at the centre.
    u = mask(uv, 'r', -1200, 250)
    v = mask(uv, 'g', -1200, 350)
    du = node(unreal.MaterialExpressionSubtract, -1050, 250)
    du.set_editor_property('const_b', 0.5)
    link(u, du, 'A')
    dv = node(unreal.MaterialExpressionSubtract, -1050, 350)
    dv.set_editor_property('const_b', 0.5)
    link(v, dv, 'A')
    au = node(unreal.MaterialExpressionAbs, -920, 250)
    link(du, au)
    av = node(unreal.MaterialExpressionAbs, -920, 350)
    link(dv, av)
    thin = node(unreal.MaterialExpressionMin, -800, 300)
    link(au, thin, 'A')
    link(av, thin, 'B')
    sharp = node(unreal.MaterialExpressionMultiply, -680, 300)
    sharp.set_editor_property('const_b', 30.0)
    link(thin, sharp, 'A')
    ray = node(unreal.MaterialExpressionOneMinus, -560, 300)
    link(sharp, ray)
    ray_sat = node(unreal.MaterialExpressionSaturate, -440, 300)
    link(ray, ray_sat)
    star = node(unreal.MaterialExpressionMultiply, -320, 250)
    link(ray_sat, star, 'A')
    link(sat, star, 'B')
    star_half = node(unreal.MaterialExpressionMultiply, -200, 250)
    star_half.set_editor_property('const_b', 0.6)
    link(star, star_half, 'A')
    shape = node(unreal.MaterialExpressionAdd, -120, 50)
    link(glow, shape, 'A')
    link(star_half, shape, 'B')

    # Pulse: 0.625 + 0.375 * sin(2pi (t / PERIOD + phase)), phase from the plot's world position.
    time = node(unreal.MaterialExpressionTime, -1400, 500)
    per = node(unreal.MaterialExpressionMultiply, -1250, 500)
    per.set_editor_property('const_b', 1.0 / PERIOD)
    link(time, per, 'A')
    pos = node(unreal.MaterialExpressionObjectPositionWS, -1400, 620)
    px = mask(pos, 'r', -1250, 620)
    py_ = mask(pos, 'g', -1250, 720)
    wx = node(unreal.MaterialExpressionMultiply, -1100, 620)
    wx.set_editor_property('const_b', 0.0137)
    link(px, wx, 'A')
    wy = node(unreal.MaterialExpressionMultiply, -1100, 720)
    wy.set_editor_property('const_b', 0.0091)
    link(py_, wy, 'A')
    offset = node(unreal.MaterialExpressionAdd, -980, 660)
    link(wx, offset, 'A')
    link(wy, offset, 'B')
    phase = node(unreal.MaterialExpressionAdd, -860, 550)
    link(per, phase, 'A')
    link(offset, phase, 'B')
    sine = node(unreal.MaterialExpressionSine, -740, 550)
    sine.set_editor_property('period', 1.0)
    link(phase, sine)
    pulse = node(unreal.MaterialExpressionMultiply, -620, 550)
    pulse.set_editor_property('const_b', 0.375)
    link(sine, pulse, 'A')
    pulse_up = node(unreal.MaterialExpressionAdd, -500, 550)
    pulse_up.set_editor_property('const_b', 0.625)
    link(pulse, pulse_up, 'A')

    color = node(unreal.MaterialExpressionConstant3Vector, -120, 400)
    color.set_editor_property('constant', unreal.LinearColor(*COLOR, 1.0))
    lit = node(unreal.MaterialExpressionMultiply, 40, 150)
    link(shape, lit, 'A')
    link(pulse_up, lit, 'B')
    out = node(unreal.MaterialExpressionMultiply, 180, 200)
    link(lit, out, 'A')
    link(color, out, 'B')
    lib.connect_material_property(out, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    lib.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(path)
    return mat
