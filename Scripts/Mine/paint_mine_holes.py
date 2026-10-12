"""Hole the landscape under the mine house's trapdoor (run in the editor with the Estate open, PIE stopped).

    . .\\Scripts\\McpHelpers.ps1 -Port <p>; pyfile Scripts\\Mine\\paint_mine_holes.py

The yard's landscape runs under the house's floorboards, 18 cm down, so without a hole the shaft under the
trapdoor shows grass. The trapdoor opening is centred on a landscape vertex (``layout.TRAPDOOR``), so the
4 x 4 landscape quads round it are painted as holes on the Visibility layer (the sea cave's hole material,
``build_landscape_holes.py``, renders them); the shaft lining fills those quads, and the floorboards hide the
part-transparent ring the painted vertices leave on the quads beyond. ``HOLE_PAINT_RECT`` is cleared first, so
a moved trapdoor repaints cleanly.
"""
import sys
from pathlib import Path

import unreal

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "Scripts" / "Blender" / "Recipes"))
import importlib  # noqa: E402

from mine import layout as L  # noqa: E402

importlib.reload(L)

HALF = 2016                 # landscape vertex index = world metres + HALF (EstateHeightfield.r16)
QUADS = 2                   # quads holed on each side of the opening's centre vertex (4 x 4 m under the boards)
RING = 6                    # the paint rectangle reaches this far round it (cleared, then painted)


def main(dry_run=False):
    x, y, _ = L.house_to_world(*L.TRAPDOOR["centre"])
    cx, cy = round(x), round(y)
    if abs(cx - x) > 0.01 or abs(cy - y) > 0.01:
        raise RuntimeError(f"The trapdoor ({x}, {y}) must sit on a landscape vertex")
    vertices = [unreal.IntPoint(cx + HALF + dx, cy + HALF + dy)
                for dx in range(-QUADS, QUADS + 1) for dy in range(-QUADS, QUADS + 1)]
    rect = (cx + HALF - RING, cy + HALF - RING, cx + HALF + RING, cy + HALF + RING)
    result = unreal.HomesteadEstateAuthoringLibrary.paint_estate_landscape_holes(vertices, *rect, dry_run)
    print("mine landscape holes:", result)
    if result.startswith("error"):
        raise RuntimeError(result)
    if not dry_run:
        # Save only the landscape proxies over the hole (a blanket save would also save unrelated dirty actors).
        x, y = cx * 100.0, cy * 100.0
        proxies = []
        for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
            if isinstance(actor, unreal.LandscapeStreamingProxy):
                origin, extent = actor.get_actor_bounds(False)
                if abs(origin.x - x) < extent.x + 600 and abs(origin.y - y) < extent.y + 600:
                    proxies.append(actor.get_outermost())
        unreal.EditorLoadingAndSavingUtils.save_packages(proxies, False)
        print("mine landscape holes: saved", [p.get_name() for p in proxies])


main()
