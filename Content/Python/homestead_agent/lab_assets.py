"""Assets for the character lab (see Source/SurvivalGame/HomesteadLab.cpp).

    from homestead_agent import lab_assets
    lab_assets.build_grid_material()

M_LabGrid: world-aligned floor grid (10 cm fine lines, 1 m lines, 10 m heavy lines), so stride
length, foot sliding and ground contact can be read against fixed marks at any floor size.
"""
import unreal

PATH = '/Game/Lab/Materials'
GRID = 'M_LabGrid'


def build_grid_material():
    mel = unreal.MaterialEditingLibrary
    if not unreal.EditorAssetLibrary.does_directory_exist(PATH):
        unreal.EditorAssetLibrary.make_directory(PATH)
    full = f'{PATH}/{GRID}'
    # Rebuild in place: deleting and recreating a loaded material raises a modal overwrite prompt.
    if unreal.EditorAssetLibrary.does_asset_exist(full):
        material = unreal.load_asset(full)
        mel.delete_all_material_expressions(material)
    else:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            GRID, PATH, unreal.Material, unreal.MaterialFactoryNew())

    def node(cls, x, y, **props):
        expression = mel.create_material_expression(material, cls, x, y)
        for key, value in props.items():
            expression.set_editor_property(key, value)
        return expression

    def link(a, b, pin=''):
        mel.connect_material_expressions(a, '', b, pin)

    world = node(unreal.MaterialExpressionWorldPosition, -1500, 0)
    xy = node(unreal.MaterialExpressionComponentMask, -1350, 0, r=True, g=True, b=False, a=False)
    link(world, xy)

    def lines(spacing, half_width, y):
        """1 on grid lines: distance to the nearest line (in cells) below half_width on X or Y."""
        cells = node(unreal.MaterialExpressionDivide, -1200, y, const_b=spacing)
        link(xy, cells, 'A')
        frac = node(unreal.MaterialExpressionFrac, -1050, y)
        link(cells, frac)
        centred = node(unreal.MaterialExpressionSubtract, -900, y, const_b=0.5)
        link(frac, centred, 'A')
        from_centre = node(unreal.MaterialExpressionAbs, -750, y)   # 0.5 at a line, 0 mid-cell
        link(centred, from_centre)
        on_line = node(unreal.MaterialExpressionStep, -600, y, const_y=0.5 - half_width)
        link(from_centre, on_line, 'X')
        x = node(unreal.MaterialExpressionComponentMask, -450, y, r=True, g=False, b=False, a=False)
        link(on_line, x)
        yy = node(unreal.MaterialExpressionComponentMask, -450, y + 60, r=False, g=True, b=False, a=False)
        link(on_line, yy)
        either = node(unreal.MaterialExpressionMax, -300, y)
        link(x, either, 'A')
        link(yy, either, 'B')
        return either

    fine = lines(10.0, 0.035, 0)
    metre = lines(100.0, 0.012, 300)
    ten = lines(1000.0, 0.003, 600)
    base = node(unreal.MaterialExpressionConstant3Vector, 0, -300, constant=unreal.LinearColor(0.32, 0.31, 0.29, 1))
    ink = node(unreal.MaterialExpressionConstant3Vector, 0, -150, constant=unreal.LinearColor(0.10, 0.10, 0.10, 1))

    def blend(under, alpha, amount, x, y):
        scaled = node(unreal.MaterialExpressionMultiply, x - 150, y, const_b=amount)
        link(alpha, scaled, 'A')
        mix = node(unreal.MaterialExpressionLinearInterpolate, x, y)
        link(under, mix, 'A')
        link(ink, mix, 'B')
        link(scaled, mix, 'Alpha')
        return mix

    colour = blend(blend(blend(base, fine, 0.3, 300, 0), metre, 0.85, 500, 300), ten, 1.0, 700, 600)
    mel.connect_material_property(colour, '', unreal.MaterialProperty.MP_BASE_COLOR)
    rough = node(unreal.MaterialExpressionConstant, 700, 800, r=0.9)
    mel.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, False)
    return material
