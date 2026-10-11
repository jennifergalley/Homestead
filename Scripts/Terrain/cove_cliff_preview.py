"""Live preview of HomesteadCoveCliff.inc in PIE: replaces the sea cave actor's cliff instances with the table's
current contents (the same transforms AHomesteadSeaCave::BuildCliff uses), so a regenerated table can be judged
without rebuilding the C++. Review aid only.

    pyfile Scripts/Terrain/cove_cliff_preview.py      (McpHelpers, in PIE)
"""
import math
import os
import re

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
INC = os.path.join(HERE, "..", "..", "Source", "SurvivalGame", "Simulation", "HomesteadCoveCliff.inc")
LINE = re.compile(r'COVE_CLIFF_PROP\("([^"]+)",\s*([^)]+)\)')


def quat_between_up(u):
    """The rotation taking +Z onto unit vector u (as FQuat::FindBetweenNormals)."""
    ux, uy, uz = u
    w = 1.0 + uz
    if w < 1e-6:
        return (1.0, 0.0, 0.0, 0.0)
    q = (-uy, ux, 0.0, w)            # cross((0,0,1), u) = (-uy, ux, 0)
    n = math.sqrt(sum(c * c for c in q))
    return tuple(c / n for c in q)


def quat_mul(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return (aw * bx + ax * bw + ay * bz - az * by, aw * by - ax * bz + ay * bw + az * bx,
            aw * bz + ax * by - ay * bx + az * bw, aw * bw - ax * bx - ay * by - az * bz)


world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
cave = [a for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor)
        if a.get_class().get_name() == "HomesteadSeaCave"][0]
batches = {c.static_mesh.get_path_name(): c for c in cave.get_components_by_class(unreal.InstancedStaticMeshComponent)}
for c in batches.values():
    c.clear_instances()
count = 0
for m in LINE.finditer(open(INC).read()):
    path = m.group(1)
    x, y, z, yaw, ux, uy, uz, s, sz = (float(v) for v in m.group(2).split(","))
    comp = batches.get(path)
    if not comp:
        continue
    half = math.radians(yaw) / 2
    q = quat_mul(quat_between_up((ux, uy, uz)), (0.0, 0.0, math.sin(half), math.cos(half)))
    rot = unreal.Quat(*q).rotator()
    comp.add_instance(unreal.Transform(unreal.Vector(x, y, z), rot, unreal.Vector(s, s, sz)), True)
    count += 1
print("cove cliff preview:", count, "instances")
