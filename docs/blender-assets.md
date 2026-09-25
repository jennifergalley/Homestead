# Blender prop pipeline

**Quality bar: high fidelity only.** Assets must hold up in a 4K render on an RTX 5080,
meaning photoscanned or equivalently detailed geometry and PBR maps. Flat-tinted
primitive props (the `kit.box`/`kit.cylinder` path) are for blockout and tooling
tests, never for final assets. For nature and organic props, start from verified
CC0 scans (Poly Haven) and use Blender to compose, vary, clean up and LOD them;
see the Bush recipe.

Props are authored with Blender **5.2.1 LTS** (`C:\Program Files\Blender Foundation\Blender 5.2`)
from Python *recipes*, in a visible Blender window or headless. The character
pipeline stays on its pinned portable 4.5.14 toolchain (`docs\character-pipeline.md`).

Everything lives in `Scripts\Blender`:

| File | Role |
|---|---|
| `Start-BlenderLive.ps1` | Opens a visible Blender with the live bridge (localhost, per-session token) |
| `Invoke-BlenderLive.ps1` | Runs Python code or a file inside that window and returns its output/traceback |
| `New-Prop.ps1` | Builds an asset set: FBX + `.blend` + preview sheet + `report.json` |
| `homestead_kit.py` | Recipe helpers: materials, primitives, `roughen`/`taper`, `join`, export, preview |
| `build_prop.py` | Blender-side builder used by `New-Prop.ps1` (both live and headless) |
| `Recipes\*.py` | One recipe per asset set (`bush.py` = scanned composition, `chopping_block.py` = primitive blockout) |
| `Get-PolyHavenAsset.ps1` | Fetches a CC0 Poly Haven `.blend` + maps (or HDRI) into `Assets\Source\Blender\polyhaven`, MD5-verified, with `receipt.json` |
| `render_beauty.py` | Headless Cycles review renders (GPU/OptiX, HDRI sky, 4K hero + close detail) |
| `Import-Props.ps1` / `import_props.py` | Unreal import of built props |

## Working live (watch it happen)

```powershell
.\Scripts\Blender\Start-BlenderLive.ps1                 # opens Blender; wait for "ready"
.\Scripts\Blender\New-Prop.ps1 chopping_block -Live      # rebuilds the recipe in that window
.\Scripts\Blender\Invoke-BlenderLive.ps1 "import bpy; print(len(bpy.data.objects))"
```

A live recipe build clears the **current scene** first, then leaves the finished
mesh selected and framed with material colors. Save anything you care about
before rebuilding. The bridge accepts only 127.0.0.1 connections carrying the
token in `Saved\BlenderLive\session.json` (regenerated per launch); close Blender
to stop it.

To check the window without asking you for a screenshot, an agent can run
`bpy.ops.screen.screenshot(filepath=...)` through `Invoke-BlenderLive.ps1`.

### Hand modeling

Model in the live window, name each exportable object `SM_<Something>`, give every
part a material, then export whatever is on screen:

```powershell
.\Scripts\Blender\New-Prop.ps1 -FromLive Bench      # -> Assets\Props\Bench\
```

This snapshots the scene to `Saved\BlenderLive\Bench.blend` and finalizes the copy,
so your working file is untouched. `-Blend path.blend` does the same from a saved
file. Add `-KeepPivot` if you placed the origin deliberately.

## Headless

`New-Prop.ps1 chopping_block` (no `-Live`) runs the same builder with
`--background --factory-startup --offline-mode`; use it for batch rebuilds or when
nobody needs to watch.

## Writing a recipe

```python
NAME = "ChoppingBlock"          # output folder Assets\Props\<NAME>
DESCRIPTION = "..."
COLLISION = "box"               # box | convex | none (applied on Unreal import)
TRIANGLE_BUDGET = 3000          # exceeded -> warning in the report

def build(kit):
    bark = kit.material("M_ChoppingBlockBark", (0.20, 0.13, 0.08), roughness=0.95)
    stump = kit.cylinder("Stump", 0.30, 0.46, (0, 0, 0.23), material=bark, sides=18)
    kit.roughen(stump, strength=0.022, scale=7.0, seed=3, subdivide=2)
    return kit.join([stump, ...], "SM_ChoppingBlock")   # or a list of SM_ meshes
```

Primitives: `box`, `cylinder` (also cones/frustums via `radius_top`), `sphere`,
`mesh` (raw polygons). Positions are meters, rotations XYZ degrees, parts are
centered on `location`. Colors are **linear** RGB; each material becomes one Unreal
slot (`MI_<Name>` instance of `M_Field` with the same Tint/Roughness). Keep
recipes deterministic: seeded `random.Random` / `roughen(seed=...)`.

`join`/`finalize` apply modifiers and transforms, weld duplicate vertices, put the
pivot at bottom-center on the origin, smart-UV-project and set sharp edges from
a 35° angle.

### Composing scanned sources

```python
def build(kit):
    templates = kit.append(kit.polyhaven("shrub_02"), ["shrub_02_a_LOD0", ...])
    parts = [kit.instance(templates["shrub_02_a_LOD0"], "Shoot0", matrix=placement), ...]
    return kit.join(parts, "SM_Bush", pivot=None, unwrap=False, reshade=False)
```

- Fetch first: `.\Scripts\Blender\Get-PolyHavenAsset.ps1 shrub_02` (default 4k). The
  cache is git-ignored; the recipe's `PROVENANCE` and `docs\asset-credits.md` carry the
  credit. Check the license on the asset page, not just "free".
- `append` brings objects in as hidden templates with their materials and 4K images;
  `instance` copies one, keeping the template's own rotation/scale but not its location.
- **Always** `unwrap=False, reshade=False` for scans; the default re-unwrap would
  destroy the atlas UVs.
- Build every publisher LOD from the same placements (`SM_X`, `SM_X_LOD1`, ...), with
  `pivot=None` so all LODs share one origin.
- Textures used by the result are copied to `Assets\Props\<Name>\Textures` and the
  saved `.blend` is repointed to them.

## Outputs and review

`Assets\Props\<Name>\` receives `SM_*.fbx`, `<Name>.blend`, `report.json`
(triangles, size in cm, materials, hashes, source recipe hash) and
`preview_SM_*.png`: a Workbench contact sheet with a gray 1.63 m heroine-height
marker in a 3/4 view, plus orthographic front (-Y), right (+X) and top views.
Textured sets also get `beauty_SM_*_hero.png` and `_detail.png`: 3840x2160 Cycles
renders under the Kloofendal CC0 sky on the GPU (OptiX on the RTX 5080). They run
headless after the build so the live window stays responsive (`-BeautySamples`,
`-NoBeauty`). **Judge fidelity from the beauty renders** and the silhouette from
the contact sheet before calling an asset done. They have already caught a floating
hatchet head and a bush that splayed like a starburst. They are Blender evidence,
not in-game evidence.

Conventions match the other exports: meters, Z up, -Y forward, FBX
`FBX_SCALE_UNITS`, no leaf bones.

## Unreal import

```powershell
.\Scripts\Blender\Import-Props.ps1 -Name ChoppingBlock
```

Requires the built editor module and `M_Field` (from `Scripts\Build-Game.ps1`), and
**no other editor using the project**. It verifies the FBX hashes against the
report, imports to `/Game/SurvivalGame/Environment/Props/<Name>`, assigns the
tinted instances, adds the requested simple collision and checks the imported
height against Blender. Placing props in the Homestead map is separate, per-feature
work. The importer has not yet been exercised end to end, because the editor was
busy in another session when it was written.

Textured props are **not importable yet**: `import_props.py` stops with a clear error
until a masked, two-sided foliage parent material (base color, alpha mask, normal,
roughness, translucency) and LOD-chain assembly are added. The Fern 02 and Grass
Medium 01 import scripts already wire equivalent materials, so reuse their approach.
Wind (pivot/vertex data) is also not authored yet.
Flat-tinted blockout props import as `M_Field` instances.
