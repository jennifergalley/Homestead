# Asset credits

This project uses original prototype geometry and the following licensed sources.
`Assets\asset-manifest.json` records the exact download URLs and license references.
`Assets\download-receipt.json` records file sizes and SHA-256 hashes after fetching.

The heroine chopping, tilling and refined watering clips, procedural hatchet,
digging stick and watering can are project-authored original prototype content.
They reuse the admitted heroine skeleton and existing project materials; they
introduce no external asset or license dependency.

The creek-bank reed clump and remaining stubble are project-authored Blender
meshes (`Scripts\Environment\build_reed_clumps.py`); their shafts, blades and
seed heads reuse `M_Field` color instances. The small held Knife is likewise
project-authored procedural geometry. Neither adds a third-party asset.

The MetaHuman heroine's primitive outfit (`Assets\Characters\PrimitiveOutfit`: homespun tank top
and low-rise shorts) is project-authored. `Scripts\Blender\Recipes\primitive_outfit.py` models it
around her MetaHuman body and skins it to `metahuman_base_skel`. The same seeded recipe synthesizes
every garment texture (weave, slubs, stitching, stains) in numpy, so it uses no scanned or
downloaded texture. The body and face meshes in `PrimitiveOutfit.blend` are the heroine's
MetaHuman assets (Unreal Engine EULA) and are included only for fitting and review. Only the
review renders use Poly Haven's CC0 `kloofendal_48d_partly_cloudy_puresky` HDRI (Greg Zaal, sky edits by
Jarod Guest, https://polyhaven.com/a/kloofendal_48d_partly_cloudy_puresky). It isn't part of the game asset.

The heroine's footwear (`Assets\Characters\Footwear`: sheepskin FurBoots, plant-fibre
WovenSandals and leather TurnShoes) is project-authored. `Scripts\Blender\Recipes\footwear.py`
and its `shoemaking\` package build a last from her MetaHuman body, model every part on it, and
skin each pair to `metahuman_base_skel`. Every leather, wool and fibre texture is synthesized in
numpy, with no scanned or downloaded images. The `.blend` files carry the MetaHuman body and face
only for fitting and review. The review renders use the same CC0 Kloofendal HDRI, which ships
with no asset.

Her wardrobe in `Assets\Characters\Clothing` (linen tee, laced long shirt, wool trousers and the
sheepskin coat with horn toggles) is project-authored in the same way:
`Scripts\Blender\Recipes\clothing_wardrobe.py` fits, skins and textures every garment from numpy
procedural maps, with no scanned or downloaded texture or mesh. `Clothing.blend` includes the
MetaHuman body and face for fitting and review only. The review renders use the same CC0 HDRI.

The granite rocks and boulders in `Assets\Props` (GraniteCobbles, GraniteSpalls,
GraniteRubble, GraniteBoulderLoaf, GraniteBlockTalus, GraniteBoulderLow,
GraniteErratic, GraniteBoulderJointed, GraniteDome, GraniteSplitBoulder, HandStones)
and the shared tiling GraniteDetail maps are project-authored: procedural geometry
(`Scripts\Blender\homestead_rocks.py`) and procedural materials
(`homestead_materials.granite`) baked to textures, with no scanned or downloaded
geometry or textures. Reference photographs were viewed only for comparison and are
not stored in the repository. The review renders use the CC0 Kloofendal 48d
Partly Cloudy (Pure Sky) HDRI by Greg Zaal (https://polyhaven.com/a/kloofendal_48d_partly_cloudy_puresky),
which ships with no asset.

The woodland underbrush set (`Assets\Props\` BlackberryBramble, ToyonHedge, Hazel, DeerBrush,
Thimbleberry, BrackenFern, WildStrawberry and GrassYarrowTuft, built with
`Scripts\Blender\homestead_foliage.py` and `homestead_shrub.py`) is project-authored: every mesh is generated from
code and every leaf, flower, berry and bark texture is painted procedurally with numpy by that
library. No scan, photo or third-party texture is used.

The StoneHoe (`Scripts\Blender\Recipes\stone_hoe.py`), the TilledBed garden square (`Scripts\Blender\Recipes\tilled_bed.py`) and the Seeds set (tepary beans and the
covered-seed SoilMound, `Scripts\Blender\Recipes\seeds.py`) are project-authored procedural geometry
with procedural materials baked to textures; they use no scanned or downloaded geometry or
textures. Their review renders use the same CC0 Kloofendal HDRI, which ships with no asset.

The DeerRemains set (`SM_DeerRemains` and `SM_DeerBones`, `Scripts\Blender\Recipes\deer_remains.py`
and `Recipes\deer\`) is project-authored:
- The bones are signed-distance geometry meshed by Blender's bundled OpenVDB.
- The hide is a solved membrane; the ribs, antlers and leaves are swept or lofted.
- The materials are procedural and baked to textures.

It uses no scanned or downloaded geometry or textures.

## Music

All music is by **Kevin MacLeod** (incompetech.com), licensed under
[Creative Commons Attribution 4.0](https://creativecommons.org/licenses/by/4.0/):

| Track | Official track page |
| --- | --- |
| Evening Fall (Harp) | https://www.incompetech.com/music/royalty-free/index.html?isrc=USUAN1100236 |
| Ascending the Vale | https://www.incompetech.com/music/royalty-free/index.html?isrc=USUAN1600064 |
| Teller of the Tales | https://www.incompetech.com/music/royalty-free/index.html?isrc=USUAN1400020 |
| Meditation Impromptu 02 | https://www.incompetech.com/music/royalty-free/index.html?isrc=USUAN1100162 |
| At Rest | https://www.incompetech.com/music/royalty-free/index.html?isrc=USUAN1100748 |

Converted for game playback; playback fades and level matching applied. The tracks play shuffled
with gaps between them. The in-game field book includes this credit. Include this document in any
distributed build.

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
| Shrub 02 (22-shoot Blender bush composition, `Assets\Props\Bush`; not yet in a build) | Rico Cilliers | https://polyhaven.com/a/shrub_02 | CC0 |
| Dry Branches Medium 01 (three pieces) | Rico Cilliers | https://polyhaven.com/a/dry_branches_medium_01 | CC0 |
| Fir Sapling (two small conifers) | Rob Tuytel (photography), Rico Cilliers (modeling) | https://polyhaven.com/a/fir_sapling | CC0 |
| Jacaranda Tree (one spreading canopy) | Rob Tuytel (guidance), Rico Cilliers (modeling) | https://polyhaven.com/a/jacaranda_tree | CC0 |
| Fir Sapling Medium (one pole conifer) | Rob Tuytel (photography), Rico Cilliers (modeling) | https://polyhaven.com/a/fir_sapling_medium | CC0 |
| Flower Empodium (two clumps) | Jenelle van Heerden (photography), Rico Cilliers (modeling) | https://polyhaven.com/a/flower_empodium | CC0 |
| Forest Ambience | TinyWorlds | https://opengameart.org/content/forest-ambience | CC0 |
| Impact Sounds | Kenney | https://kenney.nl/assets/impact-sounds | CC0 |
| Interface Sounds | Kenney | https://kenney.nl/assets/interface-sounds | CC0 |
| Tree chop fall thud (hatchet chops ChopA/ChopB and the TreeFall landing) | kheetor | https://opengameart.org/content/tree-chop-fall-thud | CC0 |
| 100 CC0 metal and wood SFX (`wood_hammer_02`, chop ChopC) | rubberduck | https://opengameart.org/content/100-cc0-metal-and-wood-sfx | CC0 |
| Small brook, stream, water sound (the creek's burbling loop, CreekLoop) | SamsterBirdies | https://freesound.org/s/584269/ | CC0 |

The felling sounds are cut, band-limited and normalised from those two recordings by
`Scripts\generate_chop_sounds.py`. The generated WAVs are tracked in `Assets\Audio\Effects`; to
regenerate, download the two Ogg sources into `Assets\Source\oga-wood-chop` (gitignored).

The creek loop (`Assets\Audio\Ambience\CreekLoop.wav`) is cut, band-limited and seam-crossfaded from
the SamsterBirdies brook recording by `Scripts\generate_creek_assets.py`; to regenerate, download
its HQ preview to `Assets\Source\freesound-brook\SmallBrook.mp3` (gitignored). The same script
synthesizes the creek's ripple normal map and foam flecks (`Assets\Environment\Creek`) from filtered
noise; they are original to this project.

The MetaHuman heroine's bare-foot footsteps (`Assets\Audio\Footsteps`) are original to this
project. `Scripts\generate_bare_footsteps.py` synthesizes them from noise and decaying tones, with
no third-party audio. The legacy heroine keeps the Kenney grass steps.

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

The generated version-three palette additionally uses one selected Jacaranda
LOD0 and one selected Fir Sapling Medium `c` form. Project-derived distance
LODs preserve all three material roles; the publisher catalog's `lods:true`
flag was not treated as proof of authored FBX LODs. Exact CC0 provenance,
source hashes, preparation records, model-specific collision qualifications,
and the deferred Island Tree 02 source are recorded in
`docs\research\environment-assets\tree-palette-20260921.md` and
`tree-palette-assets-01`. Fir Tree 01 was not acquired because its FBX exceeds
the established per-file cap.

## Character prototype

The default heroine is now a **MetaHuman** made with MetaHuman Creator in
Unreal Engine 5.8. MetaHumans are covered by the Unreal Engine EULA (from UE 5.6
there is no separate MetaHuman licence), free below the Unreal royalty threshold.
The EULA forbids using MetaHuman data to train or feed generative AI.

She uses:

- Epic's cloud rig and 4K texture sources.
- Epic's stock grooms (long straight hair, brows, lashes).
- The stock placeholder garment.
- The project's own clips, retargeted onto the MetaHuman skeleton.

`Assets\Characters\MetaHumanHeroine\provenance.json` records the look scripts,
retarget setup, content inventory and cooked folders. The prototype heroine below
remains the rollback (`-HomesteadLegacyHeroine`).

Her walk and sprint (run) loops come from Epic Games' **Game Animation Sample**
project (free on Fab). It is Epic-published, UE-only content under the Unreal Engine
EULA, so it may be used and modified in Unreal Engine products only. Five UEFN
mannequin loops, the mannequin mesh/skeleton and `IK_UEFN_Mannequin` were migrated to
their original `/Game/Characters/UEFN_Mannequin` paths. They are then retargeted onto
the heroine by `Content\Python\homestead_agent\gasp_locomotion.py`. The sample's foley
notifies and audio were not kept.

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

The retired heroine face trial (its launcher and build were removed on 2026-09-25) fit face/eye geometry from
[CharMorph Vitruvian](https://github.com/Upliner/CharMorph-Vitruvian) to the
existing CC0 MPFB heroine. Its `config.yaml` declares the model data CC0, and
its README records the original model author's permission to relicense.
The fitted neck reuses the admitted MPFB skin texture and the original
modular rig and garments. Exact pinned source revision, file hashes and
script-stripping procedure are in
`Assets\Characters\HeroineTrials\Vitruvian01\source-license.json`; the
separate CharMorph add-on code was not included or executed. Jenny
subsequently rejected this face and its neck seam; it is retained only
in the repository's trial source assets.

The retired experimental motion trial also used
Carnegie Mellon University Graphics Lab Motion Capture Database subject
07 walk recordings 01 and 04. The original dataset's
[FAQ](https://mocap.cs.cmu.edu/faqs.php) permits copying, modification
and redistribution without permission. Only the publisher's original
text ASF/AMC files were downloaded and retargeted by first-party
scripts; exact official URLs, data digests and motion descriptions are
in `Assets\Characters\HeroineTrials\CMUWalk01\source-license.json`.
The separate `CMUWalk02` upright-head retarget uses those same
digest-pinned source files, the existing heroine skeleton and original
face; it does not reuse the rejected Vitruvian geometry. This is not
visual approval of the resulting walk or Sprint.
