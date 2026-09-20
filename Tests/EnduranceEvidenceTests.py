"""Reject tampered renderer evidence using copies of an actual successful short fixture."""
import importlib.util
import json
from pathlib import Path
import shutil
import sys
import tempfile


def main():
    source = Path(sys.argv[1]).resolve()
    scratch = Path(sys.argv[2]).resolve()
    spec = importlib.util.spec_from_file_location(
        "endurance_analysis", Path(__file__).resolve().parents[1] / "Scripts" / "Analyze-Endurance.py"
    )
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    cases = ("valid", "mode", "lighting", "complexity", "flags", "f9", "frame", "unsolicited")
    with tempfile.TemporaryDirectory(prefix="endurance-evidence-", dir=scratch) as temporary:
        for case in cases:
            root = Path(temporary) / case
            root.mkdir()
            for name in ("progress.json", "endurance-events.txt", "endurance-samples.csv", "engine.log"):
                shutil.copy2(source / name, root / name)
            for name in ("SmokeSave", "Frames"):
                shutil.copytree(source / name, root / name)
            (root / "EngineUser").mkdir()
            result = json.loads((root / "progress.json").read_text(encoding="utf-8-sig"))
            if case == "mode":
                result["presentation"]["viewMode"] = 8
            elif case == "lighting":
                result["presentation"]["lighting"] = False
            elif case == "complexity":
                result["presentation"]["shaderComplexity"] = True
            elif case == "flags":
                result["presentation"]["showFlags"] = "Lighting=0,ShaderComplexity=0"
            elif case == "f9":
                result["f9NoScreenshotChecks"] = 1
            elif case == "frame":
                path = next((root / "Frames").glob("*.json"))
                frame = json.loads(path.read_text(encoding="utf-8-sig"))
                frame["viewMode"] = 8
                path.write_text(json.dumps(frame), encoding="utf-8")
            elif case == "unsolicited":
                (root / "EngineUser" / "unexpected.png").write_bytes(b"synthetic unsolicited file")
            (root / "progress.json").write_text(json.dumps(result), encoding="utf-8")
            rejected = False
            try:
                module.analyze(root)
            except AssertionError:
                rejected = True
            assert rejected == (case != "valid"), f"Unexpected validation result: {case}"
            assert (root / "analysis.json").exists() == (case == "valid")
    print(f"Endurance evidence: {len(cases)} positive/negative checks passed; original proof unchanged.")


if __name__ == "__main__":
    main()
