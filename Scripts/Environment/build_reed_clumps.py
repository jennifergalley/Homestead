"""Author two original, noncolliding creek-bank reed meshes with no downloaded assets."""
import hashlib
import json
import math
import random
from pathlib import Path

import bpy

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "Assets" / "Environment" / "Reeds"
MATERIALS = ("M_ReedsStem", "M_ReedsLeaf", "M_ReedsSeed")


def make_mesh(name, ready):
    rng = random.Random(6926)
    vertices, faces, roles = [], [], []

    def face(points, role):
        first = len(vertices)
        vertices.extend(points)
        faces.append(tuple(range(first, first + len(points))))
        roles.append(role)

    def ring(cx, cy, z, radius, sides=5):
        return [(cx + radius * math.cos(2 * math.pi * i / sides),
                 cy + radius * math.sin(2 * math.pi * i / sides), z)
                for i in range(sides)]

    for index in range(22):
        angle = index * 2.39996 + rng.uniform(-0.24, 0.24)
        distance = math.sqrt((index + 0.35) / 22) * 0.22
        x, y = distance * math.cos(angle), distance * math.sin(angle)
        full_height = rng.uniform(0.72, 1.10)
        height = full_height if ready else rng.uniform(0.10, 0.23)
        lean_angle = angle + rng.uniform(-0.4, 0.4)
        lean = rng.uniform(0.015, 0.075) if ready else 0.008
        levels = 5 if ready else 2
        for segment in range(levels):
            t0, t1 = segment / levels, (segment + 1) / levels
            def at(t):
                return ring(x + math.cos(lean_angle) * lean * t * t,
                            y + math.sin(lean_angle) * lean * t * t,
                            height * t, (0.0052 if ready else 0.007) * (1 - 0.55 * t))
            lower, upper = at(t0), at(t1)
            for side in range(5):
                next_side = (side + 1) % 5
                face((lower[side], lower[next_side], upper[next_side], upper[side]), 0)
        if not ready:
            continue

        for leaf_index in (0, 1):
            fraction = 0.21 + leaf_index * 0.21
            direction = angle + leaf_index * 2.4
            u = (math.cos(direction), math.sin(direction))
            v = (-u[1], u[0])
            root = (x + u[0] * 0.01, y + u[1] * 0.01, height * fraction)
            middle = (root[0] + u[0] * 0.09, root[1] + u[1] * 0.09,
                      root[2] + rng.uniform(0.13, 0.18))
            tip = (middle[0] + u[0] * 0.11, middle[1] + u[1] * 0.11,
                   middle[2] + rng.uniform(0.10, 0.16))
            width = 0.009
            left = (middle[0] - v[0] * width, middle[1] - v[1] * width, middle[2])
            right = (middle[0] + v[0] * width, middle[1] + v[1] * width, middle[2])
            face((root, left, tip), 1)
            face((tip, right, root), 1)
            face((tip, left, root), 1)
            face((root, right, tip), 1)

        if index % 3:
            top = ring(x + math.cos(lean_angle) * lean,
                       y + math.sin(lean_angle) * lean, height - 0.19, 0.014, 6)
            waist = ring(x + math.cos(lean_angle) * (lean + 0.006),
                         y + math.sin(lean_angle) * (lean + 0.006), height - 0.06, 0.027, 6)
            crown = ring(x + math.cos(lean_angle) * (lean + 0.008),
                         y + math.sin(lean_angle) * (lean + 0.008), height, 0.006, 6)
            for side in range(6):
                next_side = (side + 1) % 6
                face((top[side], top[next_side], waist[next_side], waist[side]), 2)
                face((waist[side], waist[next_side], crown[next_side], crown[side]), 2)

    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    for label, color in zip(MATERIALS, ((0.21, 0.31, 0.15), (0.29, 0.39, 0.19), (0.35, 0.20, 0.09))):
        material = bpy.data.materials.get(label) or bpy.data.materials.new(label)
        material.diffuse_color = (*color, 1)
        mesh.materials.append(material)
    for polygon, role in zip(mesh.polygons, roles):
        polygon.material_index = role
        polygon.use_smooth = role == 0
    uv = mesh.uv_layers.new(name="UVMap")
    for polygon in mesh.polygons:
        corners = ((0, 0), (1, 0), (1, 1), (0, 1)) if len(polygon.loop_indices) == 4 \
            else ((0, 0), (1, 0), (0.5, 1))
        for index, value in zip(polygon.loop_indices, corners):
            uv.data[index].uv = value
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.fbx(filepath=str(OUT / (name + ".fbx")), use_selection=True,
                             object_types={"MESH"}, global_scale=1, apply_unit_scale=True,
                             apply_scale_options="FBX_SCALE_UNITS", axis_forward="-Y", axis_up="Z",
                             bake_anim=False, path_mode="STRIP", use_mesh_modifiers=True,
                             mesh_smooth_type="FACE", use_tspace=True)
    return {"vertices": len(mesh.vertices), "triangles": sum(len(p.vertices) - 2 for p in mesh.polygons),
            "height_m": max(p[2] for p in vertices), "radius_m": max(math.hypot(p[0], p[1]) for p in vertices),
            "sha256": hashlib.sha256((OUT / (name + ".fbx")).read_bytes()).hexdigest()}


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.unit_settings.system = "METRIC"
    bpy.context.scene.unit_settings.scale_length = 1
    report = {name: make_mesh(name, ready) for name, ready in
              (("SM_ReedClump", True), ("SM_ReedStubble", False))}
    report["provenance"] = "Original project-authored geometry; no third-party asset or reference texture"
    (OUT / "report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print("REED_CLUMPS_AUTHORED " + str(OUT / "report.json"))


if __name__ == "__main__":
    main()
