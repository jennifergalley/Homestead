"""Draft-only trout/perch iteration; never import this set as accepted game art.

Usage: New-Prop.ps1 caught_fish_head_review -Live -OutDirectory <E: scratch> -BeautySamples 192
"""
import hashlib
import importlib.util
from pathlib import Path

SOURCE = Path(__file__).with_name("caught_fish.py")
spec = importlib.util.spec_from_file_location("fish_head_review_source", SOURCE)
fish = importlib.util.module_from_spec(spec)
spec.loader.exec_module(fish)

NAME = "CaughtFishHeadReview"
DESCRIPTION = "Draft-only original trout/perch head, jaw and skin refinement."
PROVENANCE = fish.PROVENANCE
COLLISION = fish.COLLISION
TRIANGLE_BUDGET = fish.TRIANGLE_BUDGET
BAKE = None
BEAUTY = fish.BEAUTY
REPORT = {"draft_only": True, "authored_source": SOURCE.name,
          "authored_source_sha256": hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
          "material_source_sha256": hashlib.sha256(
              SOURCE.parents[1].joinpath("homestead_materials.py").read_bytes()).hexdigest(),
          "wet_fish": fish.REPORT["wet_fish"]}


def build(kit) -> list:
    return [fish.build_species(kit, species) for species in fish.FISH
            if species["key"] in {"RiverTrout", "LakePerch"}]
