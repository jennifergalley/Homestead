"""Draft-only five-map review of all six original catches, never an admission.

Usage: New-Prop.ps1 caught_fish_family_review -Live -OutDirectory <E: scratch>
"""
import importlib.util
from pathlib import Path

SOURCE = Path(__file__).with_name("caught_fish_trout_proof.py")
spec = importlib.util.spec_from_file_location("fish_family_review", SOURCE)
proof = importlib.util.module_from_spec(spec)
spec.loader.exec_module(proof)

NAME = "CaughtFishFamilyReview"
DESCRIPTION = "Held original six-species anatomy/material review."
PROVENANCE = proof.PROVENANCE
COLLISION = proof.COLLISION
TRIANGLE_BUDGET = proof.TRIANGLE_BUDGET
BAKE = proof.fish.BAKE
BEAUTY = proof.BEAUTY
REPORT = dict(proof.fish.REPORT, **proof.REPORT)
REPORT["draft_mode"] = "family_baked"
build = proof.fish.build
after_bake = proof.fish.after_bake
