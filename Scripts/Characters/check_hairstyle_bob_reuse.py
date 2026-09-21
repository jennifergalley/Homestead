"""Read-only validation of the isolated stock-bob drop and wave freeze."""
import hashlib
import json
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT/"Assets"/"Characters"/"HairstyleRefinement"/"BobReuse"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    report = json.loads((OUT/"manifest.json").read_text())
    assert len(report["joined"])==6
    assert len(report["modular"]) in [0,3]
    assert {(e["body"],e["outfit"]) for e in report["joined"]} == {
        (b,o) for b in ["Preferred","Willow","Hazel"] for o in ["Tunic","Apron"]}
    for kind in ["joined","modular"]:
        for entry in report[kind]:
            path = OUT/entry["fbx"]
            assert sha(path)==entry["sha256"]
            assert sha(ROOT/entry["source"])==entry["source_sha256"]
            assert path.stem+"."+path.stem==entry["object_path"].rsplit("/",1)[1]
            checks = entry["checks"]
            assert checks["nonhair_roundtrip_error"]<1e-5
            assert checks["nonhair_normal_error"]<1e-4
            assert checks["nonhair_material_values_links_texture_hashes_exact"]
            assert checks["bind_error"]<2e-5
            assert checks["roundtrip"]["bones"]==53
            assert entry["hair_material"]=="M_Heroine_Hair_bob01_Neutral"
            assert entry["hair_material_slot"]==(7 if kind=="joined" else 3)
            if kind=="joined":
                assert checks["nonhair_preexport_error"]==0
                assert checks["upper_vertices_exact"]>0
                assert 1.40<checks["new_min_z_m"]<1.42
            else:
                assert checks["joined_hair_surface_error"]<1e-5
                assert checks["joined_hair_normal_error"]<.001
                assert all(n>1000 for n in checks["permanent_coverage_faces_unchanged"].values())
    for path,digest in {**report["frozen_wave_and_appearance_hashes"],**report["protected_source_hashes"]}.items():
        assert sha(ROOT/path)==digest,path
    material = json.loads((OUT/"materials.json").read_text())["M_Heroine_Hair_bob01_Neutral"]
    texture = OUT/material["texture"]
    assert sha(texture)==material["sha256"]
    r,g,b,a = Image.open(texture).convert("RGBA").split()
    assert r.tobytes()==g.tobytes()==b.tobytes()
    low,high = r.getextrema()
    assert high-low>100
    original = Image.open(ROOT/material["derived_from"]).convert("RGBA")
    assert a.tobytes()==original.getchannel("A").tobytes()
    print(f"BOB_REUSE_VERIFIED: 6 joined + {len(report['modular'])} modular; cap retained, stock alpha retained, nonhair/rig preserved")
    print(f"WAVE_FREEZE_VERIFIED: {len(report['frozen_wave_and_appearance_hashes'])} wave/Appearance files unchanged")


if __name__=="__main__":
    main()
