---
name: blender-assets
description: Create or edit original, high-fidelity 3D assets for Homestead (SurvivalGame) with Blender, live in a visible Blender window or headless, and export them for Unreal. Use when asked to model, build, sculpt, texture, tweak or export a game asset or prop with Blender, recreate an object from a reference image, open Blender, or turn something hand-modeled in Blender into a game asset.
---

# Blender assets for Homestead

Full reference: `docs\blender-assets.md`. Tooling lives in `Scripts\Blender`.

**Jenny only wants high-fidelity assets that look right rendered at 4K on her RTX 5080.**
Flat-colored primitives (`kit.box`, `kit.sphere`, ...) are blockout only (`chopping_block.py`);
never present them as finished. The primary skill is **authoring brand-new assets from
scratch**: procedural geometry, procedural PBR materials baked to textures, and Cycles review
renders you critique like an art director. CC0 photoscans (Poly Haven) are a frequent,
acceptable fallback or ingredient.

Reference recipes:

| Recipe | Technique |
| --- | --- |
| `flint_axe.py` | From scratch: lofted/warped tubes, Voronoi flake scars, swept cord lashing, `kit.mats` wood/flint/rawhide, 2K bake |
| `wattle_panel.py` | From scratch: woven rod sweeps around stakes, bark + daub materials, 4K bake |
| `wild_garlic.py` | From scratch foliage: real-geometry leaves/flowers on a shared UV atlas (`BAKE repack: False`) |
| `bush.py` | Scan composition: instanced, varied Poly Haven shoots with LODs |
| `granite_*.py`, `hand_stones.py` | From scratch rocks: `homestead_rocks` implicit fields, exfoliation plates, `kit.mats.granite`, shared LOD UVs; big ones layer the tiling `granite_detail.py` maps |

## Choose the mode Jenny asks for

Both modes run the same recipes and produce identical outputs, so use whichever she asks
for at the time. If she doesn't say, work headless when she's away and ask when she's around.

- **Headless**: `.\Scripts\Blender\New-Prop.ps1 <recipe>` (no window).
- **Live (visible window)**: check `.\Scripts\Blender\Invoke-BlenderLive.ps1 -Ping`, and start it
  if needed with `.\Scripts\Blender\Start-BlenderLive.ps1`. Then run `New-Prop.ps1 <recipe> -Live`, or
  build headless and `-Show` (or `Show-Prop.ps1 <Name>`) to load the result into her window
  with EEVEE rendered shading. Close the window with `Stop-Process -Id <pid>` only when she says so.

## Authoring loop (from scratch)

1. **Research the real object.** Real dimensions, materials and construction (how the head is
   hafted, how leaves attach, what the weathering looks like). Write them at the top of the recipe.
2. **Model from parts** in `build(kit)`:
   - `kit.tube(name, points, radius|radii, sides)` sweeps a profile along a path, for hafts, stems, rods and cords.
   - `kit.warp(obj, fn)` reshapes it (loft a tube into an axe head or blade).
   - `kit.subdivide` adds density.
   - `kit.displace(obj, fn(co, pcoord))` adds knots, flake scars and bark relief along the normals.
   - `kit.mesh(...)` builds custom grids such as leaf blades.
   - Finish with `kit.join([...], "SM_<Name>", unwrap=False, reshade=True)`.
3. **Materials.** Use `kit.mats` (`homestead_materials.py`): `wood`, `bark`, `flint`, `rawhide`,
   `leaf`, `stem` and `daub`. They read the per-part `pcoord` attribute, so grain follows each part.
   Extend the library rather than inlining one-off node graphs, and keep albedo in realistic
   linear ranges (wood 0.1-0.3, chalk/cortex ≤ 0.35; nothing near white except flowers).
4. **Bake and review settings.**
   - `BAKE = {"size": 2048|4096, "samples": 96}` converts materials to basecolor, roughness,
     normal (OpenGL) and AO PNGs on fresh UVs.
   - For foliage with a hand-laid shared UV atlas, use `"repack": False` and drop AO.
     `kit.assign_tube_uvs` puts stems and scapes into their own atlas rects.
   - `BEAUTY = {"pose": (rx, ry, rz), "focus": (x, y, z)}` sets the review pose. For example, lay a tool flat
     on the ground, then aim the close-up camera at the interesting joint.
5. **Critique the 4K renders.** Look at `beauty_SM_<Name>_hero.png` and `_detail.png` yourself.
   Name what looks fake (plastic sheen, over-saturation, uniformity, facets, floating parts,
   wrong scale), fix it, and rebuild. Expect 3-6 passes. Never report on a render you haven't viewed.
6. Record the triangle count, size and remaining weaknesses honestly in your report.

## From a reference image

Save Jenny's image to `Assets\Source\Blender\references\<name>.<ext>`.
Read proportions and materials off it and write a recipe as above. In live mode, show it
behind the model with `kit.reference_image(path, view="FRONT"|"SIDE", height=<m>)` to match the
silhouette. When comparing, view the image and the hero render together and list the differences
before each pass.

## Lessons learned (avoid repeating)

- **Displacement.** Never displace deeper than about 30% of the local half-thickness; the faces cross and
  thin rims tear into spikes. Blend per-face patterns smoothly across rims, never with a hard `if y >= 0`.
- **Silhouette density.** Silhouettes show facets. Use ≥12 sides for rods and stems, ≥24 for hafts, and enough path points.
- **Material masks.** Keep them region-confined (e.g. `flint(cortex_below_x=...)`), otherwise rind or grime blotches
  cover everything.
- **Colour.** Glossy, saturated green reads as plastic. Foliage needs roughness around 0.5, colour variation,
  translucency and imperfections.
- **Review renders.** `render_beauty` uses AgX with -0.35 exposure and an f/22 detail camera. Judge colour there,
  not in the Workbench previews.
- **Rocks (see `docs\blender-assets.md`, "Rocks").** Cut big exfoliation steps *before* the voxel remesh
  (it heals folds); only small flakes and relief after it. Displacing 0.3 m+ on the final mesh folds into
  spikes in concave creases. Cluster stones remesh separately (`union=False`) or they fuse into puddles.
  Measure baked albedo instead of trusting a sunlit render: granite at 0.3 linear already looks white in sun;
  add metre-scale variation (lichen film, streaks), not darkness.
- **Diagnose with the baked maps.** When something reads wrong (a pale glow, a colour cast), open the
  `T_<Name>_*` PNGs and ray-cast the render pixel to its UV before tweaking colours. The StoneHoe's "maroon,
  glowing" blade was the bake giving the whole mesh flint's Subsurface Weight with a 5 cm radius. Zero
  subsurface on opaque sets.
- **UV packing of long parts.** A 1.2 m haft packs as a diagonal sliver and starves every other part of texels.
  Lay tube UVs yourself in segments and use `BAKE["repack"] = False` (`stone_hoe.py`), and inspect the basecolor
  atlas once per new asset.
- **Knapped stone.** Uniform Voronoi scars read as crumpled paper or turtle shell. Scars are struck from the
  edges, so elongate the cells across the blade and keep them few and broad. Soil-worn chert is waxy
  (roughness ~0.45); only the use-polished bit is glossy.

## Rules

- A live recipe build **clears the current scene**. If the window contains
  unsaved hand work, ask before rebuilding (or save a copy first).
- Use original geometry or verified-license sources only. Scans come from CC0 Poly Haven via
  `Get-PolyHavenAsset.ps1`, which verifies the MD5 and writes a receipt. Record credit in the recipe's
  `PROVENANCE` and in `docs\asset-credits.md`. For scans, always `join(..., unwrap=False, reshade=False)`.
- Use meters, Z up and -Y forward, with the pivot at bottom-center (the kit enforces this). Name meshes `SM_*` and materials `M_*`.
- Use seeded randomness so rebuilds are reproducible.
- Hand-modeled work: name objects `SM_*` and run `New-Prop.ps1 -FromLive <Name>`. This exports a snapshot
  and never modifies her open file.
- **Unreal import.**
  - Run `.\Scripts\Blender\Import-Props.ps1 -Name <Name>`.
  - It needs exclusive use of the project's editor; **don't run it when another session is using Unreal**.
  - Textured and baked props need a textured/foliage parent material in the importer first. Flip the normal-map green channel for Unreal.
  - A Blender render is not in-game evidence.
- The character pipeline (`Scripts\Characters`) uses its own pinned Blender 4.5.14 and
  MPFB; don't move it to 5.2 as a side effect.
