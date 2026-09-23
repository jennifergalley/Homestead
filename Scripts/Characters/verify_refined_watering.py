"""Verify the fresh plot-directed watering clip."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from verify_tilling import verify


if __name__ == "__main__":
    verify("RefinedWatering/AN_Heroine_WaterRefined.fbx", 2.0, 0.3,
           "refined-watering-export-validation.json", "Refined watering")
