"""Shared seamless granite crystal detail (tiling textures), not a placeable prop.

Big boulders cannot carry 2-5 mm crystals in a per-asset bake: a 4K map over a
house-sized rock is ~4 mm per texel. They bake macro maps (weathering, lichen,
streaks, moss, soil) without crystals and layer these tiling maps on top, projected
in world/object space at 1 tile per meter:

- ``T_GraniteDetail_basecolor``: crystal colours normalised so the mean is linear 0.5;
  multiply by 2 over the macro base colour (strength 0.6-0.9).
- ``T_GraniteDetail_normal``: OpenGL tangent normal of the grain relief (flip green
  for Unreal), blend over the macro normal (e.g. BlendAngleCorrectedNormals).
- ``T_GraniteDetail_roughness``: per-mineral roughness (quartz and biotite glossier).
- ``T_GraniteDetail_height``: grain relief height (Blender uses it as bump).

The swatch mesh is a 1 x 1 m plane with UVs 0..1; the texture is periodic because
the plane is mapped onto a flat torus in 4D noise space (see
``homestead_materials.granite_detail``). Everything is generated here.
"""
NAME = "GraniteDetail"
DESCRIPTION = "Seamless 1 m tiling granite crystal detail maps (shared by the large boulders); swatch plane."
COLLISION = "none"
TRIANGLE_BUDGET = 200
BAKE = {"size": 2048, "samples": 32, "repack": False,
        "maps": ["basecolor", "roughness", "normal", "height"]}
BEAUTY = {"pose": (0, 0, 0), "focus": (0.0, 0.0, 0.0), "views": ["hero", "detail"]}
NOTES = {"tile_m": 1.0, "grain_m": 0.0032,
         "usage": "World/object-aligned projection at 1 tile per meter; basecolor x2 multiply over macro."}


def build(kit):
    mat = kit.mats.granite_detail("M_GraniteDetailSource", tile=1.0, grain=0.0032)
    # 8 x 8 grid with a sub-millimetre bow (the builder rejects perfectly flat meshes).
    steps = 8
    verts, uvs = [], []
    for j in range(steps + 1):
        for i in range(steps + 1):
            u, v = i / steps, j / steps
            verts.append((u - 0.5, v - 0.5, 0.0008 * (1 - (2 * u - 1) ** 2)))
            uvs.append((u, v))
    faces = [(j * (steps + 1) + i, j * (steps + 1) + i + 1, (j + 1) * (steps + 1) + i + 1,
              (j + 1) * (steps + 1) + i) for j in range(steps) for i in range(steps)]
    plane = kit.mesh("SM_GraniteDetail", verts, faces, material=mat)
    layer = plane.data.uv_layers["UVMap"].data
    for loop in plane.data.loops:
        layer[loop.index].uv = uvs[loop.vertex_index]
    return kit.finalize(plane, pivot=None, unwrap=False, reshade=False)
