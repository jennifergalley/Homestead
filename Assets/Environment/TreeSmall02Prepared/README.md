# Tree Small 02 prepared source

**Handoff: v3. Source work only; not imported or tested in Unreal.**
Poly Haven Tree Small 02 by Rico Cilliers, CC0-1.0:
https://polyhaven.com/a/tree_small_02
License: https://polyhaven.com/license

The 14 admitted originals were checked against the coordinator's acquisition
receipt before and after processing. They remain ignored and unchanged. This
namespace contains four derived FBXs, 13 unchanged source texture copies, exact
material mapping, measured geometry/round-trip records and internal comparison
images. All FBXs and images use the repository's existing Git LFS attributes.

## First integration

Start with **`v3\TreeSmall02_LOD2.fbx`** for one real-world tree, or LOD1 if the
closer camera needs more detail. Do not use the 2.06-million-triangle source
reference as the default mass-instanced asset. A measured gameplay view should
decide the first mesh and transition distances; no target frame rate is inferred.

| File | Triangles | Branches / leaves / trunk | Intended source role |
| --- | ---: | --- | --- |
| `TreeSmall02_SourceLOD0.fbx` | 2,062,487 | 94,814 / 1,939,380 / 28,293 | Source-resolution geometry reference |
| `TreeSmall02_LOD1.fbx` | 378,926 | 47,407 / 310,300 / 21,219 | Closest reduced detail |
| `TreeSmall02_LOD2.fbx` | 231,785 | 23,702 / 193,938 / 14,145 | First integration candidate |
| `TreeSmall02_LOD3.fbx` | 170,289 | 9,481 / 155,150 / 5,658 | Farther reduced candidate |

These are separate FBX files, not an FBX LODGroup or an Unreal LOD asset.
Main owns UE import and the eventual LOD chain. A single reduced mesh is a valid
first playable increment, not evidence that runtime LOD selection is complete.
No Nanite, wind, collision, sockets, actor logic or gameplay resource identity
is authored into them.

## Units, root and UV contract

- Blender 4.5.14 imports the original as a roughly **4.5567 m** tall tree with
  source origin at **(0, 0, 0)**. No extra 100x scale, recentering, ground lift
  or handcrafted geometry was applied.
- All exports declare **FBX UnitScaleFactor=100**, OriginalUnitScaleFactor=100,
  UpAxis=2/+1, FrontAxis=1/+1, CoordAxis=0/-1. Export uses `-Y` forward, `Z` up,
  `FBX_SCALE_UNITS`, apply-unit-scale enabled, global scale 1.
- Use the native importer's normal unit/axis conversion once. Expect about
  **456 cm height**, not 4.56 cm or 45,600 cm. Confirm actual UE imported bounds;
  do not compensate with a guessed scale.
- Source Blender-world bounds in metres are
  X `[-1.308993, 1.607707]`, Y `[-2.909850, 1.382627]`,
  Z `[-0.024104, 4.532637]`. Small reduced bounds differences are measured in
  `provenance.json`; LOD2 spans Z `[-0.024194, 4.539844]`.
- Keep the source root on the intended ground point. Its small negative Z
  extends about **2.4 cm below the root**, not a centered rock-style pivot.
  Blender-world axes above are source coordinates, not a substitute for checking
  Unreal's handedness conversion.
- Both UV layers survive every export/reimport: channel 0 **`UVMap`** is active
  for rendering; channel 1 **`UV_map_01`** is retained. Material maps use channel 0.
  The second layer is not certified as an Unreal lightmap unwrap. Reduction
  interpolates UVs with geometry; it does not retain every original UV vertex.

## Material and texture contract

`v3\materials.json` is authoritative for all 13 relative texture paths.
All three material slots remain in this exact order, including every reduced
level:

| Slot | Name | Shading inputs |
| --- | --- | --- |
| 0 | `tree_small_02_branches` | `branch` diffuse, DX normal, roughness, AO; opaque |
| 1 | `tree_small_02_leaves` | `leaves` diffuse, DX normal, roughness, AO, separate alpha; masked and two-sided |
| 2 | `tree_small_02_trunk` | un-suffixed diffuse JPG, DX normal, roughness, AO; opaque |

Base color is sRGB; normal, roughness, AO and opacity-mask maps are linear data.
Use the DirectX normal convention and normal-map compression in Unreal; do not
load the old source FBX's missing OpenGL-normal references. Leaf material
response/clip threshold, normal/tangent policy and two-sided foliage shading
remain main's in-engine decisions. A flat prototype Tint override would discard
this material separation.

The prepared FBXs intentionally contain **no image references or embedded
textures**. Blender's bundled FBX parser/writer removes source Texture/Video
objects and their connections from a temporary geometry-only copy before
import; original absolute paths never reach Blender's image loader. Supported
import options disable image search, animation and custom-property import.
Textures are then explicitly mapped from the 14-file receipt whitelist.
No code shipped with the asset is executed.

## Method and source checks

`Scripts\Environment\TreePreparation.py` uses Blender's standard material
separation, Decimate COLLAPSE modifiers and FBX export. The three selected
branch/leaf/trunk ratios are `[0.50,0.16,0.75]`, `[0.25,0.10,0.50]`,
and `[0.10,0.08,0.20]`; this is not a custom decimator.

Blender reimport checks pass for all four files: exact triangle count per
material, all three ordered slots, both finite UV layers/ranges, origin and
world bounds within 0.00001 m. The 13 copied texture SHA-256 values match their
admitted sources. `inspection.json`, `provenance.json`, `roundtrip.json` and
`fbx-contract.json` contain measurements; provenance binds the source files,
tool version/hash, URLs/license and every exported FBX hash.

Two rejected trials are retained only in the owning session's local files,
not in this published namespace:

- Leaf collapse at 3%/1% reached 73k/55k total triangles but visibly depleted
  the canopy. Those exports are not integration candidates.
- Standard planar dissolve preserving boundaries/UV seams left more than 1.4M
  triangles even at 12 degrees. It did not solve source complexity adequately.

## Internal visual comparison, not a requested game screenshot

`v3\Comparison\all-front.png` and `all-side.png` use columns
**SourceLOD0, LOD1, LOD2, LOD3**. They are CPU Cycles, orthographic 512x512 per
view, eight samples, unlit source albedo/alpha, no GPU capture or Unreal lighting.
Both views were inspected. Tree structure and the canopy outline remain
recognizable; the farther levels deliberately lose fine foliage detail.

| Level | Front / side covered-pixel ratio | Front / side silhouette IoU |
| --- | --- | --- |
| LOD1 | 99.14% / 98.77% | 95.82% / 93.87% |
| LOD2 | 95.57% / 93.27% | 91.94% / 87.98% |
| LOD3 | 91.89% / 87.82% | 88.43% / 83.05% |

These source-relative metrics use alpha >= 0.5 and are sensitive to sampling and
view. In particular LOD3's side view is about 12% thinner. They do not establish
temporal stability, masked overdraw, physical PBR appearance or acceptable
in-game transitions. No 4K, GPU, wind/shadow, collision or 60 FPS claim is made.

## Reproduction and freeze

Run only in the approved official Blender, with a new output version and a
lane-owned profile/temp folder. Example after setting those process-local paths:

```powershell
& 'E:\Tools\blender-4.5.14-windows-x64\blender.exe' `
  --background --factory-startup --disable-autoexec --threads 4 `
  --python-exit-code 1 --python '.\Scripts\Environment\TreePreparation.py' -- `
  --source-dir '<copied-admitted-tree-directory>' `
  --receipt '<coordinator-download-receipt.json>' `
  --output '.\Assets\Environment\TreeSmall02Prepared\v4' `
  --mode prepare --leaf-method collapse
```

`--mode preview` produces the internal comparisons; `--mode verify` checks
round-trip geometry and texture hashes. Neither is an Unreal import.
`FROZEN.json` blocks regeneration/preview writes to a handed-off version; verify
is read-only when frozen. Experiments after handoff must name a new version.
