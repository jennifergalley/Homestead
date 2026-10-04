"""Draft-only source-shader carp muzzle proof; never a production import family.

Usage: New-Prop.ps1 caught_fish_carp_proof -Live -OutDirectory <E: scratch>
Render with render_beauty.py at 4K/192 samples before another family bake.
"""
import hashlib
import importlib.util
from pathlib import Path

SOURCE = Path(__file__).with_name("caught_fish.py")
spec = importlib.util.spec_from_file_location("carp_proof_source", SOURCE)
fish = importlib.util.module_from_spec(spec)
spec.loader.exec_module(fish)

NAME = "CaughtFishCarpProof"
DESCRIPTION = "Held original carp blunt muzzle, soft terminal mouth and cheek proof."
PROVENANCE = fish.PROVENANCE
COLLISION = fish.COLLISION
TRIANGLE_BUDGET = fish.TRIANGLE_BUDGET
BAKE = None
BEAUTY = fish.BEAUTY
REPORT = {"draft_only": True, "draft_mode": "source",
          "authored_source": SOURCE.name,
          "authored_source_sha256": hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
          "material_source_sha256": hashlib.sha256(
              SOURCE.parents[1].joinpath("homestead_materials.py").read_bytes()).hexdigest(),
          "wet_fish": fish.REPORT["wet_fish"]}


def build(kit) -> list:
    return [fish.build_species(kit, next(species for species in fish.FISH
                                        if species["key"] == "LakeCarp"))]
