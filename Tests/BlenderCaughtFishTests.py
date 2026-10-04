"""Check the six original catch meshes without clearing the visible scene.

Usage: Scripts\\Blender\\Invoke-BlenderLive.ps1 -File Tests\\BlenderCaughtFishTests.py
"""
import hashlib
import importlib.util
import math
import struct
import sys
from pathlib import Path

import bpy
from mathutils import Vector

EXPECTED = {
    "RiverTrout": ("Salmo trutta", .34),
    "RiverSalmon": ("Salmo salar", .62),
    "LakePerch": ("Perca fluviatilis", .30),
    "LakeCarp": ("Cyprinus carpio", .42),
    "SeaMackerel": ("Scomber scombrus", .36),
    "SeaBass": ("Dicentrarchus labrax", .46),
}


def check_fin_ray_attachment() -> None:
    source = Path(__file__).resolve().parents[1] / "Scripts" / "Blender"
    sys.path.insert(0, str(source))
    import homestead_kit as kit

    spec = importlib.util.spec_from_file_location("caught_fish_regression", source / "Recipes" / "caught_fish.py")
    recipe = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(recipe)
    material = bpy.data.materials.new("CaughtFishRegressionFin")
    parts = []
    try:
        base = lambda s: Vector((0, .04 * s, 0))
        edge = lambda s: base(s) + Vector((0, .015, .05))
        parts = recipe.membrane(kit, "CaughtFishRegressionFin", base, edge, 8,
                                material, .34, material)
        for ray, obj in enumerate(parts[1:], 1):
            s = ray / 8
            for ring in range(8):
                v = ring / 7
                center = sum((vertex.co for vertex in obj.data.vertices[ring*12:(ring+1)*12]), Vector()) / 12
                expected = base(s).lerp(edge(s), v) + Vector((
                    .34 * .0025 * math.sin(math.pi * v) * math.sin(math.pi * s), 0, 0))
                assert (center - expected).length < .000002, "Fin ray leaves its bowed membrane"
        print("ORIGINAL_FISH_FIN_ATTACHMENT_PASS 7 rays, 8 stations")
    finally:
        for obj in parts:
            data = obj.data
            bpy.data.objects.remove(obj, do_unlink=True)
            if data.users == 0:
                bpy.data.meshes.remove(data)
        if material.users == 0:
            bpy.data.materials.remove(material)


def main() -> None:
    check_fin_ray_attachment()
    signatures = set()
    for key, (species, length) in EXPECTED.items():
        obj = bpy.data.objects.get("SM_" + key)
        assert obj is not None, "Missing catch mesh: " + key
        assert obj.get("fish_species") == species, "Species/catalogue mismatch: " + key
        assert obj.get("fish_original") is True, "Unattributed catch: " + key
        data = obj.data
        assert all(math.isfinite(c) for v in data.vertices for c in v.co), key + " has invalid vertices"
        low = min(v.co.y for v in data.vertices)
        high = max(v.co.y for v in data.vertices)
        assert abs((high - low) - length) < .002, key + " has wrong authored scale"
        assert abs(min(v.co.z for v in data.vertices)) < .00001, key + " has wrong ground pivot"
        triangles = sum(len(p.vertices) - 2 for p in data.polygons)
        assert 25000 <= triangles <= 65000, key + " is empty or exceeds the poly budget"
        assert data.uv_layers.get("UVMap") is not None, key + " has no bake UV"
        assert all(math.isfinite(c) for uv in data.uv_layers["UVMap"].data for c in uv.uv), key + " has invalid UVs"
        normalized = b"".join(struct.pack("<3f", *(c / length for c in vertex.co)) for vertex in data.vertices)
        signature = hashlib.sha256(normalized).hexdigest()
        assert signature not in signatures, key + " reuses another catch's normalized geometry"
        signatures.add(signature)
        assert len(obj.material_slots) in (1, 6, 7), key + " has missing anatomy materials"
        print(f"ORIGINAL_FISH_PASS {key}: {species}, {triangles} tris, length/pivot/UV/unique geometry")
    print("ORIGINAL_FISH_FAMILY_PASS 6")


if __name__ == "__main__":
    main()
