"""Five-map bake of the one-trout structural proof, never a production family.

Usage: New-Prop.ps1 caught_fish_trout_bake -Live -OutDirectory <E: scratch> -BeautySamples 192
"""
import importlib.util
from pathlib import Path

SOURCE = Path(__file__).with_name("caught_fish_trout_proof.py")
spec = importlib.util.spec_from_file_location("trout_bake_proof", SOURCE)
proof = importlib.util.module_from_spec(spec)
spec.loader.exec_module(proof)

NAME = "CaughtFishTroutBake"
DESCRIPTION = proof.DESCRIPTION
PROVENANCE = proof.PROVENANCE
COLLISION = proof.COLLISION
TRIANGLE_BUDGET = proof.TRIANGLE_BUDGET
BAKE = proof.fish.BAKE
BEAUTY = proof.BEAUTY
REPORT = dict(proof.REPORT, draft_mode="baked")
build = proof.build
after_bake = proof.fish.after_bake
