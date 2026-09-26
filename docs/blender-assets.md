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
| `homestead_kit.py` | Recipe helpers: primitives, `tube`/`warp`/`displace`/`subdivide`, `join`, `bake`, export, previews, beauty renders, Poly Haven append/instance, `reference_image` |
| `homestead_materials.py` | Procedural PBR material library (`kit.mats`): wood, bark, flint, rawhide, leaf, stem, daub |
| `Show-Prop.ps1` | Loads a built asset set into the live window with EEVEE rendered shading and the review sky |
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
nobody needs to watch. Live and headless builds produce identical outputs, so pick
whichever Jenny wants at the time. To show a headless result in the window afterwards, run
`New-Prop.ps1 <recipe> -Show` or `Show-Prop.ps1 <Name>` (EEVEE rendered shading under the
review sky).

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

### Authoring from scratch

```python
NAME = "FlintAxe"
COLLISION = "convex"
TRIANGLE_BUDGET = 120000
BAKE = {"size": 2048, "samples": 96}                           # procedural -> texture maps
BEAUTY = {"pose": (90, 0, 28), "focus": (0.02, 0.0, 0.50)}     # lay it down; close-up target

def build(kit):
    wood = kit.mats.wood("M_AxeHaft", light=(0.25, 0.17, 0.10), dark=(0.11, 0.065, 0.035))
    haft = kit.tube("Haft", points, radius=lambda t: ..., sides=48, material=wood)
    kit.displace(haft, lambda co, pco: knots_and_facets(pco))
    head = kit.tube("Head", points, radius=1.0, sides=64, material=kit.mats.flint("M_AxeHead"))
    kit.warp(head, loft_to_axe_head)     # reshape the tube's unit circles
    kit.tag_coords(head.data)            # re-anchor material coordinates after reshaping
    kit.subdivide(head, levels=1)
    kit.displace(head, flake_scars)
    return kit.join([haft, head, ...], "SM_FlintAxe", unwrap=False, reshade=True, smooth_angle=70)
```

**Modeling tools**

- `tube(name, points, radius|radii, sides, cap, roll)` sweeps a circle along a path with
  parallel-transport frames. A callable `radius(t)` tapers it.
- `warp(obj, fn(co) -> co)` reshapes any mesh.
- `displace(obj, fn(co, pcoord) -> meters)` offsets vertices along their normals.
- `subdivide(obj, levels, smooth)` and `apply_modifiers` add density.
- `mesh(name, verts, faces, ...)` builds custom grids (leaf blades, petals).
- `pack_uvs` creates fresh bake UVs.
- `assign_tube_uvs(obj, (u0, u1, v0, v1), sides, rings)` maps an un-joined tube into a fixed atlas
  rect, which is what you want for foliage and cord atlases baked with `"repack": False`.

**Material coordinates.** Every primitive writes a `pcoord` point attribute with part-local rest
coordinates; tubes use `(x, y, arclength)`. The `kit.mats` materials read it, so wood grain runs
along each haft, cord strands wind around each lashing, and joins or displacement don't swim the
texture. Call `kit.tag_coords(mesh)` again after a large `warp` if the material should follow the
new shape.

**Baking.** With `BAKE` set, `build_prop.py` repacks UVs, unless `"repack": False`, which is for
hand-laid shared atlases such as foliage. It then runs `kit.bake`, which writes the following
PNGs to `Assets\Props\<Name>\Textures` and replaces the procedural materials with one image
material `M_<Name>`:

- `T_<Name>_basecolor`
- `T_<Name>_roughness`
- `T_<Name>_normal` (tangent space, OpenGL +Y)
- `T_<Name>_ao`
- extra roles (e.g. `height`) bake whatever node each material labels `HOMESTEAD_<ROLE>`

Recipes may define `after_bake(kit, obj)` (e.g. to layer shared detail maps) and `NOTES`
(a dict copied into `report.json`). Textures that live in another asset set under
`Assets\Props` stay shared and are reported by relative path instead of copied.

The beauty renders use the baked maps, so they show what ships. Subsurface weight carries over
for leaves.

**Review pose.** `BEAUTY["pose"]` rotates the asset for review only; tools lie on the ground.
`BEAUTY["focus"]` aims the 85 mm detail camera at an authoring-space point.

**Art-director loop.** After every build, view `beauty_*_hero.png` and `_detail.png` and list
what reads fake, then fix it and rebuild. The axe took seven passes:

- The cortex was blotched all over the head, then chalk-white.
- The flake-scar displacement tore spikes through thin rims.
- The rawhide was too orange.
- The haft was too pale and too perfect.

Common tells:

- plastic gloss;
- over-saturation;
- identical repeated parts;
- visible facets on silhouettes;
- floating or intersecting parts;
- colour that is too clean (no grime where hands and soil touch).

### From a reference image

1. Save the image under `Assets\Source\Blender\references\`.
2. Note proportions, materials and construction.
3. Write a recipe. In live mode, pin the image behind the model with
   `kit.reference_image(path, view="FRONT"|"SIDE", height=<meters>, opacity=0.5)`
   (viewport only, never rendered).
4. Compare hero renders with the image side by side each pass.

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

## Rocks (Sierra Nevada granite)

`Scripts\Blender\homestead_rocks.py` builds rocks from implicit fields (numpy, seeded):

- `boulder(radii, power, lumps, joints, rotate, bend)` is a superellipsoid corestone cut by
  rounded joint planes; `cut` splits one along a (rough) fracture plane; `scatter` makes one
  solid per stone for clusters.
- `solid(name, field, center, post)` meshes the surface by bisection along icosphere rays
  (star-shaped pieces), with an optional `post(P, N)` pass: `plates` (exfoliation shells
  spalled in polygonal Voronoi plates), `pits` (weathering pans), `crack` (joint traces).
- `finish(kit, pieces, name, lod0_tris, lod_tris, ground_z, detail, union)` voxel-remeshes
  (union or per piece), unwraps a smoothed copy into few large islands (buried islands at
  30 % texel density), subdivides, applies `detail(P, N) -> offset | (offset, attributes)`,
  puts the ground line at the origin and decimates LODs that keep LOD0's UVs. The builder
  bakes LOD0 once and gives `_LODn` meshes the same material.
- `detail` can write point attributes the material reads: `fresh` (newly spalled or
  broken rock: paler, less rind and lichen) and `joint` (old joint faces: rusty film).

`kit.mats.granite` is weathered granodiorite: interlocking crystals (coarse plagioclase
and K-feldspar, interstitial quartz, biotite and hornblende), weathering rind, mafic
enclaves, optional K-feldspar megacrysts, iron stains, black water streaks on steep faces,
a metre-scale lichen/cyanobacteria film, crustose lichens (grey-green, chartreuse
areolate map lichen with black prothallus, black spots), crevice and north-side moss,
and a soil line. Linear albedo of bare weathered rock is ~0.28-0.35, roughness 0.8-0.95
(quartz and mica glossier).

Texel density: crystals are 2-5 mm, so rocks up to ~1.5 m bake them in (2K). The large
and house-sized rocks bake macro maps without crystals (4K) and layer the shared,
seamless `Assets\Props\GraniteDetail` maps (1 m tiles, generated on a flat 4D torus so
they tile without seams) in object/world space: `BaseColor = Macro * lerp(1, 2 * Detail,
0.8)`, detail normal blended over the macro normal, roughness `lerp(Macro, Detail, 0.4)`.
`kit.layer_detail` wires the same thing into the Blender material for the review renders.
Each recipe's `NOTES` (copied to `report.json`) gives the sink depth, collision advice and
material notes.

Rocks are authored with the ground line at z = 0 and pivot there, so placing one at
terrain height sinks it by its recorded `sink_depth_m`; `BEAUTY["ground"] = "origin"`
reviews them half-buried the same way. `HandStones` are the exception: bounds-centre
pivots for hand attachment.

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
