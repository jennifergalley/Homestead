"""Held source-shader family review; never a production import set.

Usage: New-Prop.ps1 caught_fish_family_source -Live -OutDirectory <E: scratch>
Open that output blend explicitly before render_beauty.py --samples 192.
"""
import hashlib
import importlib.util
from pathlib import Path

SOURCE = Path(__file__).with_name("caught_fish.py")
spec = importlib.util.spec_from_file_location("family_review_source", SOURCE)
fish = importlib.util.module_from_spec(spec)
spec.loader.exec_module(fish)

NAME = "CaughtFishFamilySource"
DESCRIPTION = "Draft-only six-species head, oral chamber and fin-fan review."
PROVENANCE = fish.PROVENANCE
COLLISION = fish.COLLISION
TRIANGLE_BUDGET = fish.TRIANGLE_BUDGET
BAKE = None
BEAUTY = fish.BEAUTY
REPORT = dict(fish.REPORT, draft_only=True, draft_mode="source",
              authored_source=SOURCE.name,
              authored_source_sha256=hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
              material_source_sha256=hashlib.sha256(
                  SOURCE.parents[1].joinpath("homestead_materials.py").read_bytes()).hexdigest())


def build(kit) -> list:
    return fish.build(kit)
