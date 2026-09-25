---
name: blender-assets
description: Create or edit original 3D props for Homestead (SurvivalGame) with Blender - live in a visible Blender window or headless - and export them for Unreal. Use when asked to model, build, sculpt, tweak or export a game asset or prop with Blender, open Blender, or turn something hand-modeled in Blender into a game asset.
---

# Blender assets for Homestead

Full reference: `docs\blender-assets.md`. Tooling lives in `Scripts\Blender`.

**Jenny only wants high-fidelity assets that look right rendered at 4K on her RTX 5080.**
Flat-colored primitives (`kit.box`, `kit.sphere`, ...) are blockout only; never present
them as a finished asset. For organic and nature assets, compose, vary and LOD verified CC0
photoscans (Poly Haven via `Get-PolyHavenAsset.ps1`); `Recipes\bush.py` is the reference.
Judge results from the Cycles `beauty_*` renders, and critique silhouette, density and scale
honestly before reporting.

## Default workflow (Jenny wants to watch: use the visible window)

1. Check whether a live bridge is already up: `.\Scripts\Blender\Invoke-BlenderLive.ps1 -Ping`.
   If that fails, run `.\Scripts\Blender\Start-BlenderLive.ps1` (opens Blender 5.2.1 on her screen).
2. Write or edit a recipe at `Scripts\Blender\Recipes\<snake_name>.py` (see
   `chopping_block.py`): define `NAME`, `DESCRIPTION`, `COLLISION`, `TRIANGLE_BUDGET`,
   and `build(kit)` returning `kit.join([...], "SM_<Name>")`.
3. Build it in the window: `.\Scripts\Blender\New-Prop.ps1 <snake_name> -Live`.
4. **Look at the result before you report it:** view the 4K
   `Assets\Props\<Name>\beauty_SM_<Name>_hero.png` / `_detail.png` (textured sets) and
   `preview_SM_<Name>.png` (silhouette + 1.63 m heroine marker), and if useful capture the window with
   `Invoke-BlenderLive.ps1 "import bpy; bpy.ops.screen.screenshot(filepath=r'<abs path>')"`.
   In Blender 5.2 the live viewport's photoreal mode is `shading.type = 'RENDERED'` (EEVEE);
   there is no `MATERIAL` mode. Fix disconnected parts, wrong scale and bad silhouettes, then rebuild.
5. For ad-hoc inspection or tweaks, send Python with `Invoke-BlenderLive.ps1 "<code>"`
   or `-File script.py`; output and tracebacks come back to you.

Headless (`New-Prop.ps1 <recipe>` without `-Live`) is for batch rebuilds.
If Jenny hand-models something, name objects `SM_*` and run `New-Prop.ps1 -FromLive <Name>`,
which exports a snapshot and never modifies her open file.

## Rules

- A live recipe build **clears the current scene**. If the window contains
  unsaved hand work, ask before rebuilding (or save a copy first).
- Original geometry or verified-license sources only. Scans come from CC0 Poly Haven via
  `Get-PolyHavenAsset.ps1` (MD5-verified, receipt written); record credit in the recipe's
  `PROVENANCE` and `docs\asset-credits.md`. For scans, always `join(..., unwrap=False, reshade=False)`.
- Meters, Z up, -Y forward, pivot at bottom-center (the kit enforces this).
  Materials named `M_<Asset><Part>` with linear RGB tints; they map to `M_Field` instances.
- Use seeded randomness so rebuilds are reproducible.
- Unreal import: `.\Scripts\Blender\Import-Props.ps1 -Name <Name>`. It needs exclusive use
  of the project's editor; **don't run it when another session is using Unreal**.
  Textured props still need a foliage parent material in the importer before they can go in.
  Import and placement in the map are game work; a Blender render is not in-game evidence.
- The character pipeline (`Scripts\Characters`) uses its own pinned Blender 4.5.14 and
  MPFB; don't move it to 5.2 as a side effect.
