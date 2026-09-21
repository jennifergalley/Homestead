# Asset credits

This project uses original prototype geometry and the following licensed sources.
`Assets\asset-manifest.json` records the exact download URLs and license references.
`Assets\download-receipt.json` records file sizes and SHA-256 hashes after fetching.

## Music

**Evening Fall (Harp)** by **Kevin MacLeod** (incompetech.com).
Licensed under [Creative Commons Attribution 4.0](https://creativecommons.org/licenses/by/4.0/).
[Official track page](https://www.incompetech.com/music/royalty-free/index.html?isrc=USUAN1100236).
Converted for game playback; playback fades applied. The in-game field book
includes this credit. Include this document in any distributed build.

## Public-domain assets

| Asset | Creator | Source | License |
| --- | --- | --- | --- |
| Brown Mud Leaves 01 | Rob Tuytel | https://polyhaven.com/a/brown_mud_leaves_01 | CC0 |
| Rock Moss Set 02 | Kless Gyzen | https://polyhaven.com/a/rock_moss_set_02 | CC0 |
| Fern 02 | Rob Tuytel (scanning), Rico Cilliers (modeling) | https://polyhaven.com/a/fern_02 | CC0 |
| Tree Small 02 (qualified provisional grove) | Rico Cilliers | https://polyhaven.com/a/tree_small_02 | CC0 |
| Grass Medium 01 (four selected clumps) | Rob Tuytel (photography), Rico Cilliers (modeling) | https://polyhaven.com/a/grass_medium_01 | CC0 |
| Grass Ground (ground blend) | Charlotte Baglioni | https://polyhaven.com/a/grass_ground | CC0 |
| Shrub 04 (two selected shoots) | Rico Cilliers | https://polyhaven.com/a/shrub_04 | CC0 |
| Dry Branches Medium 01 (three pieces) | Rico Cilliers | https://polyhaven.com/a/dry_branches_medium_01 | CC0 |
| Fir Sapling (two small conifers) | Rob Tuytel (photography), Rico Cilliers (modeling) | https://polyhaven.com/a/fir_sapling | CC0 |
| Flower Empodium (two clumps) | Jenelle van Heerden (photography), Rico Cilliers (modeling) | https://polyhaven.com/a/flower_empodium | CC0 |
| Forest Ambience | TinyWorlds | https://opengameart.org/content/forest-ambience | CC0 |
| Impact Sounds | Kenney | https://kenney.nl/assets/impact-sounds | CC0 |
| Interface Sounds | Kenney | https://kenney.nl/assets/interface-sounds | CC0 |

The Fern 02 clearing candidate uses four separately imported meshes and the
publisher's 1K diffuse, DirectX normal, roughness, ambient-occlusion and alpha
maps. Source-axis/unit conversion was baked once; the project-authored masked,
two-sided material connects those maps. Exact source files, licenses and hashes
are recorded in `Assets\Environment\woodland-preparation-01`; import and preview
evidence is in `docs\research\environment-assets`. This does not imply that the
other acquired woodland assets are included in the playable candidate.

The qualified Tree Small 02 increment uses the project-reduced LOD2 mesh,
thirteen unchanged 2K maps and three project-wired materials. Provenance is in
`Assets\Environment\TreeSmall02Prepared\v3\provenance.json`. Import adapts the
branch UV ordering and repairs only invalid normals while preserving valid
custom normals. Twelve branch tangent/binormal corners remain a disclosed
diagnostic qualification; original import03 remains FAILED. The first two
ordinary contact-observer scenarios failed and the corrected third scenario
passed. Coordinator review accepted this limited playtest increment, not clean
mesh, full woodland, performance or Jenny's art approval.

The grass/ground increment retains four Grass Medium 01 objects (`mid_b`,
`small_b`, `tall_a`, `tiny_a`): 2,279 source/render triangles in total, not the
publisher's aggregate. The reference-free derivative preserves their geometry,
normals, UVs and transforms; provenance is in
`Assets\Environment\GrassMedium01Prepared\v1\provenance.json`. Five unchanged1K
maps feed a project-wired masked two-sided material. Three Grass Ground2K maps
blend with the existing ground maps through terrain vertex-red weights without
changing terrain positions, topology, normals, UVs or collision. Actual import
and separate reload evidence is in
`docs\research\environment-assets\grass-assets-01`. There is no authored wind
or claim that this restrained clearing patch is a complete woodland.

The woodland resource candidate selects nine meshes from the four additional
CC0 sets above, using23 unchanged1K maps and five project-wired materials.
Reference-free preparation preserves source geometry, normals, UVs, transforms
and ordered material bindings; native import bakes scene/unit conversion once.
Fir distance LODs and the separately derived Tree Small02 distance LODs are
project-generated, not publisher-authored chains. Provenance is in
`Assets\Environment\WoodlandResources\candidate01`; actual38-package import and
independent reload are sealed in
`docs\research\environment-assets\woodland-assets-01`. Shrub04 contains no fruit:
separate prototype berries remain gameplay produce. Fir is a small conifer,
not a mature canopy tree. Visual woodland acceptance remains separate.

## Character prototype

The clothed heroine prototype uses MakeHuman Community / MPFB CC0 graphical
assets and an original procedural tunic and idle/walk clips. The relaxed idle
and grounded walk revision is also project-authored on that same rig; no
motion-capture service or replacement character asset was used. Full provenance,
tool-license distinctions, source URLs, and modifications are recorded in
`Assets\Characters\provenance.json` and `docs\character-pipeline.md`. The GPL
authoring tools are separate from the exported CC0 graphical assets.

The character is a provisional interpretation of the brief, not final appearance
approval or the complete character creator. Older technical packages may still
contain the labeled geometric stand-in; see their build status before testing.
The supplied portrait is local reference only, not a licensed game texture or a
file to publish. No Skyrim, Hades, Coral Island, or Dreamlight Valley assets are used.
