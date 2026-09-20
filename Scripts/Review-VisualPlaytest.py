"""Build a local timestamped review player and conservative motion diagnostics."""
import argparse
import csv
import html
import json
import math
from pathlib import Path
import statistics


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=Path)
    args = parser.parse_args()
    directory = args.directory.resolve()
    with (directory / "telemetry.csv").open(encoding="utf-8-sig") as source:
        rows = list(csv.DictReader(source))
    if not rows:
        raise ValueError("No recorded gameplay frames.")
    frames = []
    for row in rows:
        frames.append({
            "t": float(row["seconds"]), "pass": row["pass"],
            "src": f"Frames/frame-{int(row['frame']):05d}.png",
        })
    diagnostics = {
        "capture": "Real engine controls; no teleport travel. Sampled frames are not a smooth-frame-rate measurement.",
        "frames": len(rows),
        "duration_seconds": frames[-1]["t"],
        "motion": {},
    }
    idle = [r for r in rows if r["pass"] == "idle"]
    diagnostics["idle_toe_separation_cm"] = statistics.median(
        math.hypot(float(r["left_toe_x"]) - float(r["right_toe_x"]),
                   float(r["left_toe_y"]) - float(r["right_toe_y"])) for r in idle)
    if "walk_weight" in rows[0]:
        diagnostics["blend_samples"] = [
            {"t": float(r["seconds"]), "pass": r["pass"], "speed": float(r["speed"]),
             "weight": float(r["walk_weight"]), "rate": float(r["gait_rate"])}
            for r in rows if 0.001 < float(r["walk_weight"]) < 0.999
        ]
    for label in ("slow-walk", "full-walk", "turn-while-moving"):
        group = [row for row in rows if row["pass"] == label]
        result = {"median_actor_speed_cm_s": statistics.median(float(row["speed"]) for row in group)}
        for side in ("left", "right"):
            relative = [float(row[f"{side}_toe_z"]) - float(row["z"]) for row in group]
            cutoff = min(relative) + 2.0
            velocities = []
            for previous, current in zip(group, group[1:]):
                dt = float(current["seconds"]) - float(previous["seconds"])
                if dt <= 0:
                    continue
                near_ground = all(float(row[f"{side}_toe_z"]) - float(row["z"]) <= cutoff for row in (previous, current))
                if near_ground:
                    dx = float(current[f"{side}_toe_x"]) - float(previous[f"{side}_toe_x"])
                    dy = float(current[f"{side}_toe_y"]) - float(previous[f"{side}_toe_y"])
                    velocities.append(math.hypot(dx, dy) / dt)
            result[f"{side}_low_toe_samples"] = len(velocities)
            result[f"{side}_low_toe_median_world_speed_cm_s"] = statistics.median(velocities) if velocities else None
            if "walk_phase" in rows[0]:
                planted = []
                offset = 0 if side == "left" else 0.5
                for previous, current in zip(group, group[1:]):
                    a = (float(previous["walk_phase"]) + offset) % 1
                    b = (float(current["walk_phase"]) + offset) % 1
                    dt = float(current["seconds"]) - float(previous["seconds"])
                    if (dt <= 0 or not 0.04 <= a < b <= 0.46
                            or min(float(r["walk_weight"]) for r in (previous, current)) < 0.999):
                        continue
                    planted.append(math.hypot(
                        float(current[f"{side}_toe_x"]) - float(previous[f"{side}_toe_x"]),
                        float(current[f"{side}_toe_y"]) - float(previous[f"{side}_toe_y"])) / dt)
                result[f"{side}_authored_stance_samples"] = len(planted)
                result[f"{side}_authored_stance_median_world_speed_cm_s"] = statistics.median(planted) if planted else None
        diagnostics["motion"][label] = result
    diagnostics["interpretation_limit"] = (
        "Low-toe filtering is a stance approximation, not a collision/foot-contact detector. "
        "Use it with the recorded frames; do not call every toe movement a defect."
    )
    (directory / "motion-review.json").write_text(json.dumps(diagnostics, indent=2), encoding="utf-8")
    data = json.dumps(frames)
    report = html.escape(json.dumps(diagnostics, indent=2))
    page = """<!doctype html><html lang="en"><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Homestead visual playtest</title>
<style>
body{margin:0;background:#101b16;color:#eeeedd;font:16px system-ui}
main{max-width:1280px;margin:auto;padding:24px}h1{margin:0 0 8px}
p{max-width:80ch;color:#becbbe;line-height:1.5}
img{display:block;width:100%;height:auto;background:#080d0a}
.controls{display:flex;gap:14px;align-items:center;padding:18px 0;flex-wrap:wrap}
button,select{background:#284534;color:#fff;border:1px solid #83937e;padding:10px 16px;font:inherit}
input{flex:1;min-width:240px}pre{white-space:pre-wrap;color:#c8d4c6}
</style><main><h1>Homestead visual playtest</h1>
<p>Recorded from the running game using normal movement and interaction controls.
No teleport travel. This player respects recorded timestamps, but the capture is sampled at approximately 8 Hz;
capture choppiness is not evidence of gameplay frame-rate problems.</p>
<div class="controls"><button id="play">Play</button><button id="previous">Previous frame</button>
<button id="next">Next frame</button><select id="passes" aria-label="Jump to action"></select></div>
<img id="frame" alt="Actual recorded gameplay frame">
<div class="controls"><input id="time" aria-label="Recorded time" type="range" min="0" step="0.01">
<span id="status"></span></div><details><summary>Motion diagnostics and limits</summary><pre>REPORT</pre></details></main>
<script>const frames=DATA;let index=0,playing=false,start=0,offset=0;
const image=document.querySelector('#frame'),slider=document.querySelector('#time'),status=document.querySelector('#status'),button=document.querySelector('#play');
slider.max=frames.at(-1).t;
function show(i){index=Math.max(0,Math.min(frames.length-1,i));image.src=frames[index].src;slider.value=frames[index].t;status.textContent=`${frames[index].t.toFixed(2)}s | ${frames[index].pass} | frame ${index+1}/${frames.length}`;}
function pause(){playing=false;button.textContent='Play';}
function at(t){let i=0;while(i+1<frames.length&&frames[i+1].t<=t)i++;show(i);}
button.onclick=()=>{if(playing){pause();return;}if(index===frames.length-1)show(0);playing=true;offset=frames[index].t;start=performance.now();button.textContent='Pause';requestAnimationFrame(tick);};
function tick(now){if(!playing)return;const t=offset+(now-start)/1000;at(t);if(t>=frames.at(-1).t){pause();return;}requestAnimationFrame(tick);}
slider.oninput=()=>{pause();at(Number(slider.value));};
document.querySelector('#previous').onclick=()=>{pause();show(index-1);};
document.querySelector('#next').onclick=()=>{pause();show(index+1);};
const passes=document.querySelector('#passes');const seen=new Set();
frames.forEach((f,i)=>{if(!seen.has(f.pass)){seen.add(f.pass);const o=document.createElement('option');o.value=i;o.textContent=f.pass;passes.append(o);}});
passes.onchange=()=>{pause();show(Number(passes.value));};show(0);
</script></html>"""
    (directory / "review.html").write_text(page.replace("REPORT", report).replace("DATA", data), encoding="utf-8")
    print(json.dumps(diagnostics, indent=2))
    print(directory / "review.html")


if __name__ == "__main__":
    main()
