"""Builds the beach campfire's flame and ember materials (add-beach-gathering).

    Run inside the editor (MCP run_python): exec(open(r'<repo>\\Scripts\\Props\\build_campfire_materials.py').read())

Jenny (2026-10-10): the campfire sheds light "just like the lamp, but more orange and flickering", and she wants it to
read lit in daylight too. The lamp's additive flame (M_OilLampFlame) is tuned for a wick behind glass and vanishes
against sunlit sand, so the campfire gets its own:
  M_BeachCampfireFlame  unlit translucent tongues for SM_CampfireFlames (UV V 0 at a tongue's base, 1 at its tip):
                        white-gold at the base through orange to deep red at the tip, fading out towards the tip and
                        at grazing edges, bright enough to read in sun. Scalar "Glow" (AHomesteadWorld flickers it).
  M_BeachCampfireEmbers unlit opaque glowing coals for SM_CampfireEmbers, orange-red, scalar "Glow".
AHomesteadWorld::RefreshBeachCampfires loads them by these names; the tuning constants are below.
"""
import unreal

MEL = unreal.MaterialEditingLibrary
PATH = '/Game/SurvivalGame/Environment/Props/BeachCampfire'
FLAME_BASE = (1.0, 0.82, 0.42)       # white-gold heart at the logs
FLAME_MID = (1.0, 0.42, 0.06)        # orange body
FLAME_TIP = (0.75, 0.10, 0.02)       # deep red tips
FLAME_EMISSIVE = 3.0                 # x Glow, on screen: divided by the eye adaptation so it reads in sun and at night
FLAME_OPACITY = 0.85
EDGE_POWER = 1.6                     # fresnel softening of the tongue edges
EMBER_COLOUR = (1.0, 0.28, 0.04)
EMBER_EMISSIVE = 2.0


def material(name, blend, two_sided=True):
    mat = unreal.load_asset(f'{PATH}/{name}')
    if mat is None:
        mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, PATH, unreal.Material, unreal.MaterialFactoryNew())
    MEL.delete_all_material_expressions(mat)
    mat.set_editor_property('blend_mode', blend)
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('two_sided', two_sided)
    return mat


def node(mat, cls, x, y, **props):
    n = MEL.create_material_expression(mat, cls, x, y)
    for key, value in props.items():
        n.set_editor_property(key, value)
    return n


def colour(mat, rgb, x, y):
    return node(mat, unreal.MaterialExpressionConstant3Vector, x, y, constant=unreal.LinearColor(*rgb, 1.0))


def link(a, b, pin=''):
    MEL.connect_material_expressions(a, '', b, pin)


def exposure_relative(mat, glow, scale, x, y):
    """Glow x scale / the eye adaptation: a constant on-screen brightness whatever the day's exposure."""
    strength = node(mat, unreal.MaterialExpressionMultiply, x, y, const_b=scale)
    link(glow, strength, 'A')
    adaptation = node(mat, unreal.MaterialExpressionEyeAdaptation, x, y + 120)
    relative = node(mat, unreal.MaterialExpressionDivide, x + 150, y)
    link(strength, relative, 'A')
    link(adaptation, relative, 'B')
    return relative


def build_flame():
    mat = material('M_BeachCampfireFlame', unreal.BlendMode.BLEND_TRANSLUCENT)
    uv = node(mat, unreal.MaterialExpressionTextureCoordinate, -1500, 0)
    mask = node(mat, unreal.MaterialExpressionComponentMask, -1350, 0, r=False, g=True, b=False, a=False)
    link(uv, mask)
    # Blender authored V 0 at the base, 1 at the tip; Unreal flips V on FBX import, so turn it back.
    v = node(mat, unreal.MaterialExpressionOneMinus, -1200, 0)
    link(mask, v)
    # Base -> mid over the lower half of each tongue, mid -> tip over the upper half.
    double_v = node(mat, unreal.MaterialExpressionMultiply, -1000, -100, const_b=2.0)
    link(v, double_v, 'A')
    lower_t = node(mat, unreal.MaterialExpressionSaturate, -850, -100)
    link(double_v, lower_t)
    lower = node(mat, unreal.MaterialExpressionLinearInterpolate, -700, -250)
    link(colour(mat, FLAME_BASE, -1000, -350), lower, 'A')
    link(colour(mat, FLAME_MID, -1000, -250), lower, 'B')
    link(lower_t, lower, 'Alpha')
    minus = node(mat, unreal.MaterialExpressionSubtract, -850, 50, const_b=1.0)
    link(double_v, minus, 'A')
    upper_t = node(mat, unreal.MaterialExpressionSaturate, -700, 50)
    link(minus, upper_t)
    upper = node(mat, unreal.MaterialExpressionLinearInterpolate, -450, -150)
    link(lower, upper, 'A')
    link(colour(mat, FLAME_TIP, -700, -50), upper, 'B')
    link(upper_t, upper, 'Alpha')
    glow = node(mat, unreal.MaterialExpressionScalarParameter, -600, 100, parameter_name='Glow', default_value=1.0)
    strength = exposure_relative(mat, glow, FLAME_EMISSIVE, -450, 100)
    emissive = node(mat, unreal.MaterialExpressionMultiply, -150, -100)
    link(upper, emissive, 'A')
    link(strength, emissive, 'B')
    MEL.connect_material_property(emissive, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    # Opacity: full at the base, gone at the tip, and soft where the tongue turns edge-on.
    one_minus_v = node(mat, unreal.MaterialExpressionOneMinus, -1000, 250)
    link(v, one_minus_v)
    tip_fade = node(mat, unreal.MaterialExpressionPower, -850, 250, const_exponent=0.6)
    link(one_minus_v, tip_fade, 'Base')
    fresnel = node(mat, unreal.MaterialExpressionFresnel, -1000, 400, exponent=EDGE_POWER, base_reflect_fraction=0.0)
    facing = node(mat, unreal.MaterialExpressionOneMinus, -850, 400)
    link(fresnel, facing)
    opacity = node(mat, unreal.MaterialExpressionMultiply, -650, 300)
    link(tip_fade, opacity, 'A')
    link(facing, opacity, 'B')
    scaled = node(mat, unreal.MaterialExpressionMultiply, -450, 300, const_b=FLAME_OPACITY)
    link(opacity, scaled, 'A')
    MEL.connect_material_property(scaled, '', unreal.MaterialProperty.MP_OPACITY)
    MEL.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)


def build_embers():
    mat = material('M_BeachCampfireEmbers', unreal.BlendMode.BLEND_OPAQUE, two_sided=False)
    glow = node(mat, unreal.MaterialExpressionScalarParameter, -700, 100, parameter_name='Glow', default_value=1.0)
    strength = exposure_relative(mat, glow, EMBER_EMISSIVE, -550, 100)
    emissive = node(mat, unreal.MaterialExpressionMultiply, -200, 0)
    link(colour(mat, EMBER_COLOUR, -400, -50), emissive, 'A')
    link(strength, emissive, 'B')
    MEL.connect_material_property(emissive, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)


build_flame()
build_embers()
for asset in ('M_BeachCampfireFlame', 'M_BeachCampfireEmbers'):
    if not unreal.EditorAssetLibrary.save_asset(f'{PATH}/{asset}', only_if_is_dirty=False):
        raise RuntimeError(f'could not save {asset}')
print('CAMPFIRE_MATERIALS_BUILT')
