"""Mine materials not in the shared library (procedural Cycles node trees, baked by the recipes).

Real-object research:
- Coal: bituminous coal is banded, bright glassy vitrain laminae (a few mm to 2 cm) between dull,
  harder durain, broken into near-cubic blocks by two sets of cleat at right angles; black with a
  faint brown cast, conchoidal fracture on the bright bands.
- Copper lodes in granite country: a quartz-and-chlorite vein, milky white to grey quartz with
  brassy chalcopyrite blebs; where air and water reach it the copper weathers to green malachite
  crusts (sometimes blue azurite) and the iron to rusty gossan along the walls of the vein.
- Gem pocket (vug): an open cavity in a quartz vein lined with hexagonal quartz prisms ending in
  six-sided pyramids, water-clear to milky at the base; amethyst is quartz coloured violet by iron,
  deepest at the terminations and often pale or colourless toward the base.
Albedos stay in realistic linear ranges (quartz <= 0.6, coal ~0.02-0.04).
"""
import bpy


def _tree(name):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    tree = mat.node_tree
    bsdf = tree.nodes["Principled BSDF"]
    return mat, tree, bsdf


def _coord(tree):
    """Object coordinates (pcoord when the mesh has it, so patterns follow each part)."""
    attr = tree.nodes.new("ShaderNodeAttribute")
    attr.attribute_name = "pcoord"
    attr.attribute_type = "GEOMETRY"
    return attr.outputs["Vector"]


def _noise(tree, vector, scale, detail=4.0, roughness=0.55):
    n = tree.nodes.new("ShaderNodeTexNoise")
    n.inputs["Scale"].default_value = scale
    n.inputs["Detail"].default_value = detail
    n.inputs["Roughness"].default_value = roughness
    tree.links.new(vector, n.inputs["Vector"])
    return n.outputs["Fac"]


def _ramp(tree, fac, stops):
    r = tree.nodes.new("ShaderNodeValToRGB")
    els = r.color_ramp.elements
    while len(els) > len(stops):
        els.remove(els[-1])
    while len(els) < len(stops):
        els.new(0.5)
    for el, (pos, col) in zip(els, stops):
        el.position = pos
        el.color = (*col, 1.0) if len(col) == 3 else col
    tree.links.new(fac, r.inputs["Fac"])
    return r.outputs["Color"]


def _mix(tree, a, b, fac):
    m = tree.nodes.new("ShaderNodeMix")
    m.data_type = "RGBA"
    for sock, val in ((m.inputs[6], a), (m.inputs[7], b), (m.inputs[0], fac)):
        if isinstance(val, (tuple, list)):
            sock.default_value = (*val, 1.0) if len(val) == 3 else val
        elif isinstance(val, (int, float)):
            sock.default_value = val
        else:
            tree.links.new(val, sock)
    return m.outputs[2]


def _math(tree, op, a, b=None):
    m = tree.nodes.new("ShaderNodeMath")
    m.operation = op
    for sock, val in zip(m.inputs, (a, b)):
        if val is None:
            continue
        if isinstance(val, (int, float)):
            sock.default_value = val
        else:
            tree.links.new(val, sock)
    return m.outputs[0]


def _map(tree, fac, lo, hi, a=0.0, b=1.0):
    m = tree.nodes.new("ShaderNodeMapRange")
    m.inputs["From Min"].default_value = lo
    m.inputs["From Max"].default_value = hi
    m.inputs["To Min"].default_value = a
    m.inputs["To Max"].default_value = b
    m.clamp = True
    tree.links.new(fac, m.inputs["Value"])
    return m.outputs["Result"]


def _bump(tree, height, strength, distance=0.004):
    b = tree.nodes.new("ShaderNodeBump")
    b.inputs["Strength"].default_value = strength
    b.inputs["Distance"].default_value = distance
    tree.links.new(height, b.inputs["Height"])
    return b.outputs["Normal"]


def coal(name="M_MineCoal"):
    mat, tree, bsdf = _tree(name)
    p = _coord(tree)
    sep = tree.nodes.new("ShaderNodeSeparateXYZ")
    tree.links.new(p, sep.inputs[0])
    # Laminae: horizontal bands wobbling slightly; bright (vitrain) vs dull (durain).
    wobble = _noise(tree, p, 3.0, 2.0)
    z = _math(tree, "ADD", sep.outputs["Z"], _math(tree, "MULTIPLY", wobble, 0.012))
    wave = tree.nodes.new("ShaderNodeTexWave")
    wave.wave_type = "BANDS"
    wave.bands_direction = "Z"
    wave.inputs["Scale"].default_value = 9.0
    wave.inputs["Distortion"].default_value = 3.0
    wave.inputs["Detail"].default_value = 3.0
    comb = tree.nodes.new("ShaderNodeCombineXYZ")
    tree.links.new(z, comb.inputs["Z"])
    tree.links.new(comb.outputs[0], wave.inputs["Vector"])
    bright = _map(tree, wave.outputs["Fac"], 0.55, 0.75)
    grit = _noise(tree, p, 140.0, 2.0)
    color = _mix(tree, (0.024, 0.022, 0.021), (0.012, 0.012, 0.013), bright)
    color = _mix(tree, color, (0.045, 0.040, 0.034), _map(tree, grit, 0.6, 0.75, 0.0, 0.5))
    tree.links.new(color, bsdf.inputs["Base Color"])
    rough = _math(tree, "ADD", _math(tree, "MULTIPLY", bright, -0.38), 0.6)
    tree.links.new(rough, bsdf.inputs["Roughness"])
    bsdf.inputs["Specular IOR Level"].default_value = 0.45
    height = _math(tree, "ADD", _math(tree, "MULTIPLY", wave.outputs["Fac"], 0.5), _math(tree, "MULTIPLY", grit, 0.3))
    tree.links.new(_bump(tree, height, 0.35, 0.003), bsdf.inputs["Normal"])
    return mat


def copper_vein(name="M_MineCopperVein", malachite=1.0):
    """Milky quartz with malachite crusts, chalcopyrite blebs and gossan staining."""
    mat, tree, bsdf = _tree(name)
    p = _coord(tree)
    milky = _noise(tree, p, 9.0, 6.0, 0.65)
    quartz = _mix(tree, (0.20, 0.20, 0.19), (0.36, 0.355, 0.33), _map(tree, milky, 0.35, 0.7))
    mal_n = _noise(tree, p, 7.5, 6.0, 0.62)
    mal = _math(tree, "MULTIPLY", _map(tree, mal_n, 0.56, 0.61), malachite)
    mal_col = _mix(tree, (0.025, 0.075, 0.045), (0.06, 0.17, 0.10), _map(tree, _noise(tree, p, 40.0, 3.0), 0.3, 0.7))
    blue = _math(tree, "MULTIPLY", _map(tree, _noise(tree, p, 11.0, 4.0), 0.68, 0.73), malachite)
    mal_col = _mix(tree, mal_col, (0.04, 0.09, 0.17), blue)
    color = _mix(tree, quartz, mal_col, mal)
    gossan = _map(tree, _noise(tree, p, 3.0, 5.0), 0.5, 0.66, 0.0, 0.85 * malachite + 0.25)
    color = _mix(tree, color, (0.20, 0.09, 0.03), _math(tree, "MULTIPLY", gossan, _math(tree, "SUBTRACT", 1.0, mal)))
    vor = tree.nodes.new("ShaderNodeTexVoronoi")
    vor.inputs["Scale"].default_value = 26.0
    tree.links.new(p, vor.inputs["Vector"])
    bleb = _map(tree, vor.outputs["Distance"], 0.16, 0.10)
    bleb = _math(tree, "MULTIPLY", _math(tree, "MULTIPLY", bleb, _map(tree, _noise(tree, p, 5.0, 2.0), 0.45, 0.6)), malachite)
    color = _mix(tree, color, (0.42, 0.31, 0.10), bleb)
    tree.links.new(color, bsdf.inputs["Base Color"])
    rough = _mix(tree, (0.32, 0.32, 0.32), (0.7, 0.7, 0.7), mal)
    rough = _mix(tree, rough, (0.22, 0.22, 0.22), bleb)
    sep = tree.nodes.new("ShaderNodeSeparateColor")
    tree.links.new(rough, sep.inputs[0])
    tree.links.new(sep.outputs[0], bsdf.inputs["Roughness"])
    height = _math(tree, "ADD", mal, _math(tree, "MULTIPLY", _noise(tree, p, 60.0, 3.0), 0.4))
    tree.links.new(_bump(tree, height, 0.4, 0.003), bsdf.inputs["Normal"])
    return mat


def crystal(name, tint_base, tint_tip, roughness=0.07):
    """Quartz-family crystal faces; the point attribute ``tip`` (0 at the base, 1 at the point)
    grades the colour (amethyst deepens toward the termination, quartz turns water-clear)."""
    mat, tree, bsdf = _tree(name)
    attr = tree.nodes.new("ShaderNodeAttribute")
    attr.attribute_name = "tip"
    attr.attribute_type = "GEOMETRY"
    p = _coord(tree)
    cloud = _noise(tree, p, 30.0, 3.0)
    fac = _math(tree, "ADD", attr.outputs["Fac"], _math(tree, "MULTIPLY", _math(tree, "SUBTRACT", cloud, 0.5), 0.3))
    color = _mix(tree, tint_base, tint_tip, _map(tree, fac, 0.1, 0.9))
    tree.links.new(color, bsdf.inputs["Base Color"])
    tree.links.new(_map(tree, attr.outputs["Fac"], 0.0, 0.6, 0.3, roughness), bsdf.inputs["Roughness"])
    bsdf.inputs["Specular IOR Level"].default_value = 0.7
    # Growth striations across the prism faces.
    wave = tree.nodes.new("ShaderNodeTexWave")
    wave.wave_type = "BANDS"
    wave.bands_direction = "Z"
    wave.inputs["Scale"].default_value = 60.0
    tree.links.new(p, wave.inputs["Vector"])
    tree.links.new(_bump(tree, wave.outputs["Fac"], 0.08, 0.001), bsdf.inputs["Normal"])
    return mat


def quartz(name="M_MineQuartz"):
    return crystal(name, (0.46, 0.46, 0.45), (0.20, 0.21, 0.22))


def amethyst(name="M_MineAmethyst"):
    return crystal(name, (0.34, 0.30, 0.36), (0.13, 0.03, 0.22))
