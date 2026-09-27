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
| `Recipes\primitive_outfit.py` + `Recipes\outfit\` | Standalone skinned-garment recipe for the MetaHuman heroine: fit, procedural cloth textures, weights, test poses, FBX. Run it directly with `blender --background --python`, not through `New-Prop.ps1`. See `Assets\Characters\PrimitiveOutfit\README.md` |
| `Recipes\clothing_wardrobe.py` | Her wardrobe (tee, long shirt, wool trousers, sheepskin coat) on the same `outfit\` modules: layered fitting over the base outfit, sleeves, drape, fleece, test poses, per-garment coverage masks. See `Assets\Characters\Clothing\README.md` |
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
- `loft(name, rows, cap_start, cap_end, cyclic)` skins a stack of closed cross-section rings into
  quads. Use it for shapes a round tube can't make: the lenticular, bevelled machete blade, or a
  pleated pouch body. Caps fan to the centroid or to a given point.
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

For metals, add `"metallic"` to `BAKE["maps"]` (as in `machete.py`). Basecolor and metallic are
then baked through an emission pass, so metal albedo isn't lost to the diffuse bake. This writes
`T_<Name>_metallic` and wires it into the image material.

The beauty renders use the baked maps, so they show what ships. Subsurface weight carries over
for leaves.

**Materials.** `kit.mats` includes the following, besides wood, flint and rawhide:

- `steel`: forge scale, patina, pitted rust, and a bright honed bevel. Bare steel has albedo
  ~0.56 and metallic 1; metallic is lower where the steel is scaled or rusted.
- `brass`: polished high points and tarnished recesses.
- `leather`: grain, creases, grime and stains.

`wood` also takes `polish` / `polish_center` / `polish_length` (a worn band where the hand grips;
`polish_center` may be a list for two-handed tools) and `relief`. Recipes built from scratch with these include
`flint_axe.py`, `machete.py`, `forage_pouch.py` (pivot at the top of the hanging loop),
`cord_belt.py`, and the forage props `berry_cluster.py`, `wild_root.py` and `berry_bush_produce.py`.
The forage props use `blackberry` (packed glossy drupelets), `leaf_pcoord` (the leaf material
driven by pcoord, so it survives the bake repack), `root` and `soil`. `berry_bush_produce.py`
reuses the berry builders by loading `berry_cluster.py` beside it. `seeds.py` builds the planting
props: `SM_Seeds`, a pinch of four tepary beans (pivot at the cluster centre), and
`SM_SoilMound` (+ `_LOD1`), a covered-seed loose-loam mound with a fingertip press (pivot at the
bottom centre, rim sunk 3 mm).
`tilled_bed.py` builds `SM_TilledBed` (+ `_LOD1`), one 1 m garden square of hoe-turned loam: a
heightfield sheet with ridges, clods and a feathered edge that sinks into the grass, pivot at
the bottom centre, planar UVs. The game swaps in `MI_TilledBed_Wet` (basecolor darkened to
0.6/0.57/0.54 with ffmpeg `colorchannelmixer`) once the square is watered.

**Open sheets need explicit face winding.** `kit.recalc_normals` guesses the outward side
from the volume, and an open sheet has none, so it can come out facing down and be back-face
culled in Unreal (the bed was invisible from above). `tilled_bed.py` flips any face whose
normal points down after building; check the average normal z in the report if a
ground-hugging prop disappears.

**LODs are hidden while each object bakes.** `build_prop.py` hides every other mesh from
render while it bakes one object's AO and normals. Before that fix, a `_LOD1` coincident with
LOD0 self-shadowed the AO bake black and blotchy (`seeds.py` and `tilled_bed.py` hit this). If an
older prop with an LOD shows unexplained dark blotches, rebuild it.

**The baked material inherits the largest Subsurface Weight** of the recipe's source materials,
with Principled's default ~5 cm red-biased radius, across the whole mesh. `flint` (0.08) and
`rawhide` (0.15) set one. For opaque sets, zero it on every material before baking, as
`stone_hoe.py` does; otherwise thin edges glow and dark stone turns maroon.

**Hand-held hotbar tools.** `flint_knife.py`, `digging_stick.py`, `water_pail.py` and
`flint_hatchet.py` are authored in their attach frame, not bottom-centre: the origin is the
main hand's grip centre, the handle runs along Z, and the working edge faces -Y. They join
with `pivot=None` so the authored origin survives export. Each docstring states its pivot,
axes and where the working end points; `report.json` records `size_cm`. The pail hangs -Z
below its bail grip, with its spout toward +X.

- `flint_hatchet.py` is a game-ready rework of `flint_axe.py` (under 10k triangles instead of
  70k). It loads the axe's shape functions with `importlib`, lowers the tessellation, and moves
  the flake-scar ridges into the bake with `add_scar_bump`. Its `REPORT["attach"]` gives cm
  offsets from the pivot to the upper-hand choke, the edge centre, the butt and the haft top.
- `stone_hoe.py` is the two-handed tilling tool (replaces the digging stick): a knapped chert
  blade rawhide-lashed at 75 degrees into the natural crook of an elbow haft (a sapling that
  grew from a thicker limb, the limb cut into a foot and heel). Its pivot is the right-hand grip
  a third of the way down the 125 cm haft; the blade is at the -Z end pointing -Y.
  `REPORT["attach"]` gives cm offsets to the left-hand grip (upper choke), the edge centre, the
  bit corners, the haft top, the crook and the blade end (the head's lowest point). It lays its
  own UVs (`BAKE["repack"] = False`): the haft is cut into four cylindrical islands so the long
  tube doesn't pack as one diagonal sliver, and the blade and lashing get 1.6x texel density.
- `flint_knife.py` reuses `add_scar_bump` by loading `flint_hatchet.py`.
- `digging_stick.py` layers fire-hardening onto `wood` by arclength (`char_overlay`).
- `water_pail.py` adds wet-wood (`wet_overlay`) and adze-facet (`adze_marks`) overlays.
  Its water disc is baked into the single texture set as a dark, low-roughness region.
  There is no separate water material slot.

**Review pose.** `BEAUTY["pose"]` rotates the asset for review only; tools lie on the ground.
`BEAUTY["focus"]` aims the 85 mm detail camera at an authoring-space point. For tiny props
(`seeds.py`), `BEAUTY["detail_distance"]` (m) moves the detail camera in closer than its 0.35 m
floor, and `BEAUTY["detail_fstop"]` stops the aperture down so a macro close-up keeps its depth
in focus. The near clip scales with the camera distance.

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
and house-sized rocks bake macro maps without crystals (4K) plus a cover mask
(`T_<Name>_mask`, 1 = bare rock, lower under lichen, moss and soil) and layer the shared,
seamless `Assets\Props\GraniteDetail` maps (1 m tiles, generated on a flat 4D torus so
they tile without seams) in object/world space: `k = DetailStrength * Mask`,
`BaseColor = Macro * MacroBrightness * lerp(1, 2 * Detail, k)`, detail normal blended over
the macro normal by `k`, roughness `lerp(Macro, Detail, 0.5 * k)`. In Unreal these live in
`M_PropGranite` (`import_props.granite_parent`); DetailStrength 0.5 and MacroBrightness
0.74 keep sunlit domes from reading as white plaster under the forest exposure.
`kit.layer_detail` wires the same thing into the Blender material for the review renders.
Each recipe's `NOTES` (copied to `report.json`) gives the sink depth, collision advice and
material notes.

Rocks are authored with the ground line at z = 0 and pivot there, so placing one at
terrain height sinks it by its recorded `sink_depth_m`; `BEAUTY["ground"] = "origin"`
reviews them half-buried the same way. `HandStones` are the exception: bounds-centre
pivots for hand attachment.

### Procedural foliage (bushes, brambles, ferns, groundcover)

The woodland underbrush set (`blackberry_bramble`, `toyon_hedge`, `hazel`, `deer_brush`,
`thimbleberry`, `bracken_fern`, `wild_strawberry`, `grass_yarrow_tuft`) is built from
scratch with two shared modules in `Scripts\Blender`:

- `homestead_foliage.py` (`F`) paints a texture atlas in numpy and builds the geometry.
  `F.Atlas(NAME, size, seed)` allocates `column` strips (tiling bark and stems, v along
  the stem) and `tile` boxes (leaves, flowers, sprays). `F.paint_blade` draws one leaf
  from a silhouette (`F.ovate` or a custom shape) and a vein list. The result is saved as
  three PNGs under `Assets\Props\<Name>\Textures`:
  - `_basecolor`: sRGB, alpha is the opacity mask, clip at 0.5.
  - `_normal`: OpenGL; flip green in Unreal.
  - `_roughness`: packed as R roughness, G translucency mask, B AO.

  `atlas.material()` makes the two-sided, masked Principled material.

  `F.Batch` collects cards, flats, tubes and spheres. `F.finish_lods` writes `SM_<Name>`,
  `_LOD1` and `_LOD2`, which share one origin.
- `homestead_shrub.py` (`S`) grows a multi-stem shrub from a `SPEC` dict: an envelope,
  branch levels, crown clumps, spurs, shell fill and leaf placement. `S.emit` then turns
  it into one LOD. Outer twigs get real leaf cards and inner ones get painted spray
  cards. `lod_levels` drops branch levels per LOD. `lod2_leaves` keeps a share of the
  real leaves at LOD2, for plants with few big leaves.
- **Wind vertex colour** `Wind` on every vertex:
  - R: height above ground / plant height.
  - G: per-branch (or per-frond / per-stem) random phase.
  - B: leaf flutter, 0 at the leaf base to 1 at the tip, 0 on wood.
  - A: 1.
- Each recipe also sets `COLLISION` (`"convex"` for blocking masses such as brambles
  and hedges, `"none"` for walk-through plants) and a `REPORT` dict (blocking, wind,
  material notes) that is merged into `report.json`. `BEAUTY` may hold per-mesh
  poses/focus under `"meshes"`.
- `$env:HOMESTEAD_REUSE_TEXTURES = '1'` reuses the existing PNGs. That gives quick,
  geometry-only iterations (seconds instead of minutes for a 4K atlas). Set it back to
  `'0'` after any painting change.

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

Requires the built editor module and `M_Field` (from `Scripts\Build-Game.ps1`). It
verifies the FBX hashes against the report, imports to
`/Game/SurvivalGame/Environment/Props/<Name>`, merges `_LODn` meshes into one LOD chain,
assigns material instances, adds the requested simple collision and checks the imported
height against Blender. The headless commandlet lacks `StaticMeshEditorSubsystem`, so LOD
and collision work needs a running editor: `sys.path.insert(0, r"<repo>\Scripts\Blender")`,
`import import_props`, then `import_props.import_prop(name, unreal.load_asset(M_Field))`
through the MCP `run_python` tool.

Material parents by report type:

- Flat-tinted blockout props: `M_Field` instances.
- Textured props: `M_PropTextured` (base colour, normal, packed roughness).
- Foliage atlases: `M_PropFoliage` (masked, two-sided, translucency).
- Rocks whose report lists `notes.detail_textures` and a cover mask: `M_PropGranite`.

`COLLISION_OVERRIDES` forces complex-as-simple collision where a convex hull would fill a
gap (the SplitBoulder's crack), and `NANITE_PROPS` enables Nanite on the house-sized rocks.

In-game placement lives in `AHomesteadWorld::BuildDecorations`:

- `GenerateUnderbrush` scatters the foliage set.
- `GenerateRocks` scatters the granite: erratic outcrops in Perlin bands, lone erratics, and
  scattered clusters. House-sized rocks go only on `Homestead::Generation::GraniteKnob`
  sites (about one chunk in seven). World generation keeps trees and forage off those
  sites, because the 4 m tree grid leaves no gap large enough for them otherwise. Save
  loading drops resource edits for candidates that no longer generate.
