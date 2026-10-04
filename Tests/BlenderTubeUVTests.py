"""Tube atlas regression checks without clearing the user's live scene.

Usage: Scripts\\Blender\\Invoke-BlenderLive.ps1 -File Tests\\BlenderTubeUVTests.py
"""
import importlib
import sys
from pathlib import Path

import bpy

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "Scripts" / "Blender"))
import homestead_kit as kit

kit = importlib.reload(kit)
EPSILON = 0.000001


def check_tube(capped: bool) -> None:
    sides, rings = 12, 5
    rect = (0.1, 0.3, 0.15, 0.85)
    obj = kit.tube("AgentTubeUVRegression", [(0, 0, i * 0.05) for i in range(rings)],
                   radius=0.01, sides=sides, cap=capped)
    mesh = obj.data
    try:
        kit.assign_tube_uvs(obj, rect, sides, rings)
        body_faces = (rings - 1) * sides
        coords = mesh.uv_layers["UVMap"].data
        body_v = [coords[i].uv.y for p in mesh.polygons[:body_faces] for i in p.loop_indices]
        for uv in (coords[i].uv for p in mesh.polygons for i in p.loop_indices):
            assert rect[0] - EPSILON <= uv.x <= rect[1] + EPSILON
            assert rect[2] - EPSILON <= uv.y <= rect[3] + EPSILON
        if capped:
            for index, poly in enumerate(mesh.polygons[body_faces:]):
                cap_v = [coords[i].uv.y for i in poly.loop_indices]
                if index < sides:
                    assert max(cap_v) < min(body_v) - EPSILON
                else:
                    assert min(cap_v) > max(body_v) + EPSILON
                uv = [coords[i].uv for i in poly.loop_indices]
                area = abs((uv[1].x - uv[0].x) * (uv[2].y - uv[0].y)
                           - (uv[2].x - uv[0].x) * (uv[1].y - uv[0].y))
                assert area > EPSILON
        else:
            assert abs(min(body_v) - rect[2]) <= EPSILON
            assert abs(max(body_v) - rect[3]) <= EPSILON
        print(f"TUBE_UV_PASS capped={capped}: atlas bounds, cap separation and nonzero fan area")
    finally:
        bpy.data.objects.remove(obj)
        bpy.data.meshes.remove(mesh)


check_tube(True)
check_tube(False)
