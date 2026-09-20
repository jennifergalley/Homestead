# Offline heroine pipeline

## Scope and status

This is a **concrete, reversible character prototype**, not completion of the
appearance MVP. One adult female face/body recipe, two complete hairstyle
variants, one original practical tunic outfit, and original procedural idle/walk
clips are authored with **Blender 4.5.14 LTS + MPFB 2.0.17**. No account, paid asset,
online generator, Mixamo download, MetaHuman service or copyrighted game costume
was used.

The selected body is 1.60 m tall before hair. The recipe aims for petite, slim but
curvy proportions, an oval face, fuller lips, defined brows, light blue eyes and
chestnut hair. It is an interpretation of the local reference, **not a likeness
claim**. The moss linen scoop-neck tunic, shoulder straps, belt and buckle are
original procedural designs, not a reconstruction of Melinoe's costume. Fitted
brown lace-up shoes and socks use the official CC0 `shoes01` asset; a crude
procedural boot experiment was rejected during visual review.
The age slider is explicitly the MPFB **young-adult anchor, 0.5** (not a fraction
of years); the recipe asserts there is no child/baby target contribution.

### Review first

Open `Build\CharacterPreview\export-long-threequarter.png`, `export-long-face.png`,
`export-bob-threequarter.png`, and `heroine-walk-contact-sheet.png` when present.
These show **FBX files reimported into Blender**, not just the pre-export scene.
`export-long-back.png`, `export-gameplay-distance.png`, and `export-firelight.png`
provide additional checks. Individual walk frames are numbered in temporal order;
the contact sheet reads left-to-right, top-to-bottom.

`Assets\Characters\Heroine\Heroine.blend` retains the rigged authoring scene,
packed textures, preview lights, both hairstyles and animation actions.
`Build\CharacterPreview\ExportReview.blend` retains the reimported review scene.

See `Build\CharacterPreview\export-validation.json` for current measured topology,
bone count, influence limits, file hashes and FBX round-trip dimensions.
An Unreal import is **not** implied by Blender validation: only a successful
`Build\CharacterPreview\unreal-import-report.json` establishes an actual Unreal
import, and even that is not gameplay integration or visual approval.
Unreal 5.8.2 import has now been verified: both meshes share the imported skeleton,
with twelve material slots and both animation assets. The report contains the
actual engine asset paths.

### Verified technical baseline

- Both clothed FBX presets reimport with 53 bones, normalized skin weights
  limited to four influences, and a live armature binding.
- Measured rest heights including hair/footwear are approximately 160.6 cm (long)
  and 161.8 cm (bob). Current meshes are about 125k and 143k triangles respectively,
  with twelve material slots; see the generated report for exact current values.
- Both animation files reimport with the intended duration, non-static bone
  motion and coincident loop endpoints. The relaxed idle arm angle is checked,
  and the preview checks animation/mesh rest-matrix compatibility and actual
  Blender action-slot binding.
- Front, three-quarter, portrait, back, distant-camera, firelight and eight walk
  frames are rendered from the exported assets. This is Blender visual evidence,
  not an Unreal frame or gameplay performance result.
- Python syntax, PowerShell syntax and the idempotent installed-tool setup path
  have been checked. The official retained asset catalog marks all entries CC0.

## Locomotion slice (2026-09-20)

The runtime now uses animation-only `AN_Heroine_RelaxedIdle` and
`AN_Heroine_GroundedWalk` clips with the existing 18 wardrobe meshes and shared
skeleton. `build_locomotion.py` starts from the retained `Heroine.blend`, solves
the leg chain to a narrower stance and a linear planted-foot trajectory, and
authors relaxed arms, modest counter-swing, breathing and finger flexion.
It does not rebuild or import wardrobe geometry.

The one-second walk has a 60 cm stance traverse per half-cycle: 120 cm/s at
unit play rate. The native `UHomesteadAnimInstance` scales cadence by actual
horizontal velocity, preserves clip phase, and blends idle/walk over 0.20 s
instead of calling `PlayAnimation` whenever speed crosses a threshold.
Maximum walking speed remains 180 cm/s; acceleration is 700 cm/s2, braking
900 cm/s2, and movement-facing rotation 300 degrees/s.

Run the two Blender scripts with the existing isolated authoring profile:

```powershell
$env:BLENDER_USER_RESOURCES = 'E:\Tools\BlenderCharacterUser'
& 'E:\Tools\blender-4.5.14-windows-x64\blender.exe' -b --python '.\Scripts\Characters\build_locomotion.py'
& 'E:\Tools\blender-4.5.14-windows-x64\blender.exe' -b --python '.\Scripts\Characters\verify_locomotion.py'
.\Scripts\Import-Locomotion.ps1 -EngineRoot 'E:\Program Files\UE_5.8'
```

The importer targets only these two clip packages, retains original idle/walk
assets, checks source hashes, and verifies persisted skeleton/duration in a
fresh editor process. `Build-Game.ps1` includes it. FBX round-trip checks compare
every bind matrix against the original exported mesh, loop endpoints, stance
width, foot height and stance velocity. Evidence goes under
`Build\CharacterPreview\locomotion-*`; the small source contract is retained
with the new FBXs in `Assets\Characters\Heroine\Locomotion`.

This remains an authored in-place gait, not motion capture or a terrain-aware
foot-IK system. Stops crossfade rather than choosing a planted stopping step;
turns can still slide. Face, hair, fabric, gathering motion and subjective
character approval are separate work. Actual-game evidence is recorded in
`docs\visual-playtesting.md`.

## Rejected face-material hypothesis (2026-09-20)

The source skin shader includes a Blender subsurface weight of 0.07, which the
original portable manifest/importer did not transfer. Unreal therefore used
uniform Default Lit skin. The eye atlas also had one uniform roughness response,
without a separate wet reflection layer. These are the two targets of this
material-only experiment; no facial geometry or expression was changed.

The separate `presentation-01` candidate tried a Subsurface Profile with a 0.35
scatter mask and the existing albedo/tint, plus Clear Coat eyes with 0.06 wet-layer
roughness over 0.32 underlying roughness. **The visual gate rejected it.**
Matching noon portraits showed red, noisy orbital/nose/neck shadows; firelight
lost nose/lip definition. More distinct eye highlights did not justify keeping
either material experiment. Technical import/full-loop success was not treated
as evidence of better facial presentation.

Both production graphs and all generated asset resaves were restored exactly
to movement checkpoint `132898f`, with SHA-256 evidence under
`Build\CharacterPreview\presentation-recovery-hashes.json`. The experimental
profile, configuration and production-import wiring were removed. The separate
rejected package and comparison captures remain local and labeled as rejected;
they are not promoted. No third cosmetic iteration was authorized.

Source inspection confirms smooth-shaded body/eye polygons, but no eye, eyelid
or jaw bones and no retained facial shape keys. Eyes, brows, lashes and teeth
are head-weighted. Credible blinking or facial repose changes would require
additional facial authoring; none was faked through eye scaling or added here.
There are no retained new textures, acquired assets, hair edits, wardrobe
geometry reimports, or changes to world lights/exposure. The fixed-angle fixture
and stronger baseline shading/color/slot/save checks are retained as diagnostics,
not a claim of facial-quality improvement.

The deterministic daytime/firelight comparison fixture is separate from
ordinary play; see `docs\visual-playtesting.md`. Its close camera, frozen pose
and copied visual world state are test-only, not runtime presentation hacks.

## Rejected mid-back length experiment (2026-09-20)

`hair-length-01` shortened only the six existing long-wave exports, preserving
the authoring scenes, crown/framing, width, UVs, weights, bind and non-hair
surfaces. Its trial target was **111.05cm**, midway between the shoulder joint
(`upperarm_l`, 126.03cm) and waist landmark (`spine_02`, 96.07cm), compared with
original evaluated tips at 89.15-89.29cm. Actual reloaded Unreal geometry reached
the target, and packaged appearance/save/movement checks passed.

**It nevertheless failed visual review.** Compressing lower lengths into this
span bunched the waves into obvious horizontal accordion-like ridges and a
blunt/frayed shelf. The coordinator independently agreed. Baseline source FBXs,
Unreal meshes, manifests, provenance and export/build wiring were restored;
the experimental authoring/import scripts and active contract were removed.
The requested shorter hairstyle remains unmet. There was no third cosmetic pass.

Only the isolated `Test-Game.ps1 -HairLength` back/three-quarter fixture and
`Review-HairLength.py` comparison tool remain, alongside honest rejection
documentation. Local rejected source/measurements are preserved under
`Build\CharacterPreview\HairLength`; the discarded package is explicitly marked
rejected. No production hair, face, material, rig or animation change survives.
See `docs\visual-playtesting.md` for exact evidence and recovery verification.

Useful import lessons: native Unreal skeletal-FBX export needs
`-AllowCommandletRendering -RenderOffscreen`; null-RHI export asserts without a
rendering mesh object. UE also re-triangulates deformed quads and splits tangent
vertices. The trial's Willow meshes gained four render vertices with unchanged
16,432 hair triangles, original source quads and UV coverage. Comparing oriented
UV boundaries and non-hair face surfaces distinguishes this from lost geometry;
raw render vertex-count equality alone is not a valid topology gate.

## Acquisition, rights and privacy

The existing PATH and usual Blender installation locations were checked before
acquiring portable tools. Tools live under `E:\Tools`; no admin installer or
elevation was used. Their archives were verified against official SHA-256 values
**before execution**:

| Tool | Official verification |
|---|---|
| Blender 4.5.14 LTS | [Official release checksum](https://download.blender.org/release/Blender4.5/blender-4.5.14.sha256), retained under `Assets\Characters\Source` |
| MPFB 2.0.17 | [Official Blender extension API](https://extensions.blender.org/api/v1/extensions/), `archive_hash`, retained in `Source\mpfb-official-release.json` |

The official [MakeHuman system asset pack](https://static.makehumancommunity.org/assets/assetpacks/makehuman_system_assets.html)
is CC0. Its per-asset catalog and original headers are retained in
`Source\SystemAssets`. The pack has a locally recorded integrity hash; **no
separate official publisher checksum was advertised for this data-only archive**.
Do not mislabel that local hash as publisher authentication.

`Assets\Characters\provenance.json` records authors, copyright holders, licenses,
URLs, digests and the particular assets actually used. The full official catalog
records every retained pack entry. Unused bundled clothes/hair/skins are not part
of the exported character.

MPFB software is GPL-3.0-or-later; its bundled graphical assets/output are CC0.
MakeHuman software is AGPL, but it is **not installed or required** here.
MakeHuman's [license, sections C–D](https://github.com/makehumancommunity/makehuman/blob/master/LICENSE.md)
expressly separates CC0 graphical output from program code. These distinctions
do not impose the authoring tool's software license on the game's exported
character. Blender's own bundled license notices remain with the portable tool.

The supplied reference was preserved at
`Assets\Characters\Reference\private-direction.png`. Its nested `.gitignore`
excludes it from future Git additions. **Keep this private reference out of
uploads, source distributions and packaged games.** It is not embedded in the
Blender scene, textures or exports. No local reference was sent to a third party.

## Rebuild offline

From `E:\Repos\SurvivalGame` in PowerShell:

```powershell
.\Scripts\Characters\setup.ps1   # First acquisition only; reuses installed tools/assets.
.\Scripts\Characters\build.ps1  # Author, export, validate and render.

# Faster geometry/export iteration without renders:
.\Scripts\Characters\build.ps1 -SkipRender

# Re-render existing exports without rebuilding:
.\Scripts\Characters\build.ps1 -PreviewOnly
```

`setup.ps1` acquires exactly the pinned portable toolchain and the one official
system asset pack. Once these exist, authoring/export/preview make no network
requests. `BLENDER_USER_RESOURCES=E:\Tools\BlenderCharacterUser` isolates the
profile from a personal Blender installation. Blender online access remains
disabled. If opening the authoring project interactively with the MPFB panel:

```powershell
$env:BLENDER_USER_RESOURCES = 'E:\Tools\BlenderCharacterUser'
& 'E:\Tools\blender-4.5.14-windows-x64\blender.exe' `
  'E:\Repos\SurvivalGame\Assets\Characters\Heroine\Heroine.blend'
```

The exact MPFB API calls were checked against the official tagged implementation
and `script_samples\10_complete_character_export_fbx.py`, not invented:
`HumanService.create_human`, `TargetService.load_target`,
`HumanService.add_builtin_rig`, and `HumanService.add_mhclo_asset`.
The source tag is `v2.0.17`, commit
`80919fa4682335c41847f761a4d79dcad4124732`.

### Editing

- `build_heroine.py`: explicit phenotype/detail targets, height, original outfit,
  hair construction, colors and procedural animation. This is the authoritative
  deterministic recipe. The `.blend` contains baked geometry with an editable
  rig/materials; it does not preserve live body morph controls after export
  preparation. Change the recipe and rebuild for proportion changes.
- `Heroine\recipe.json`: selected macro/detail values.
- `Heroine\mpfb-authoring-preset.json`: additional upstream reconstruction record;
  it is **not** a standalone replacement for the custom outfit/hair recipe.
- `export_heroine.py`: hides under-outfit skin by removing it from gameplay
  exports, joins each full preset, normalizes at most four bone influences,
  exports FBXs, and reimports them to test scale, rig binding and topology.
- `preview_exports.py`: reimports the exported mesh/clip files and rebuilds the
  portable PBR material specification for actual output review.

## Proposed Unreal integration contract

Source directory: `Assets\Characters\Heroine`.
Destination: `/Game/SurvivalGame/Characters/Heroine`.

| File | Contract |
|---|---|
| `SK_Heroine_LongWave.fbx` | Joined clothed full preset, long-wave default |
| `SK_Heroine_Bob.fbx` | Same body/outfit and rest skeleton, short bob alternative |
| `AN_Heroine_Idle.fbx` | Original in-place 3 s / 91 sampled frames at 30 FPS |
| `AN_Heroine_Walk.fbx` | Original in-place 1 s / 31 sampled frames at 30 FPS |
| `materials.json` + `Textures\*` | Explicit base color, alpha mask, roughness, metallic and texture paths |

The animation FBXs also contain the clothed long-hair reference mesh to preserve
an identical FBX bind pose; import them as **animations only**, not additional
characters. The verifier tests matching rest matrices and relaxed idle arms,
as well as motion and loop endpoints. The MPFB high-poly eye atlas **requires its
alpha channel**: importing it opaque produces black eyes. Preserve the eye,
eyelash, eyebrow and hair alpha masks specified by `materials.json`.

The rig is MPFB **game_engine**, with one `Root`, pelvis, spine, limbs, hands and
fingers (53 authoring bones). Unreal's actual imported reference skeleton has
**54 bones**, adding a `Heroine_Rig` wrapper above `Root`. It is not a promised
UE Manny/Quinn retarget-compatible skeleton.
Do not assign unrelated Epic animations directly. Both presets must import onto
the same skeleton. An initial full-mesh swap selects the hairstyle; no runtime
deep-slider system or face/body mix-and-match is provided.

Blender authoring units are meters, Z up, facing -Y. FBX uses explicit unit
conversion (`FBX_SCALE_UNITS`) and -Y forward / Z up, no leaf bones. Import with
scene and scene-unit conversion, uniform scale 1. Expected height is about
160–163 cm including hair, not 1.6 cm or 16,000 cm. **Check facing in the actual
Unreal preview** before wiring movement/camera offsets; correct any mesh-component
yaw there rather than rotating the rest skeleton differently between presets.

`import_unreal.py` is a self-contained editor-side importer. It creates only the
character destination assets, shares the first imported skeleton, assigns
explicit opaque/masked two-sided materials and validates mesh scale. It does not
change the engine installation, project plugin settings or game C++.

**Do not run while Unreal is still installing.** After the parent confirms a
complete engine and enables Python Editor Script Plugin in the project:

```powershell
. .\Scripts\Set-EngineEnvironment.ps1
& 'E:\Program Files\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' `
  'E:\Repos\SurvivalGame\SurvivalGame.uproject' `
  -run=pythonscript `
  '-script=E:\Repos\SurvivalGame\Scripts\Characters\import_unreal.py' `
  -unattended -nosplash
```

Without an actual successful import report, treat that importer as **prepared,
not engine-validated**. Python API/import-factory behavior can differ by engine
release. The parent owns runtime mesh attachment, movement animation selection,
camera tuning and packaged build verification.

## Honest limitations and next review gates

- One face/body preset, two hairstyles and one outfit is **not** the requested
  full appearance MVP (several presets, color choices, three curated hairstyles
  and outfit selection).
- The face, skin and hair are a naturalistic starting point, not production
  beauty approval or an exact match to the supplied image. The selected long
  style is the CC0 `long01` mesh, lightly waved, fitted clear of the tunic and
  recolored chestnut. It does **not** yet achieve the reference's generous loose
  curls/volume. A procedural ribbon-hair experiment was rejected during visual
  review rather than presented as finished hair. These are hair cards, not a
  strand groom; silhouettes and alpha sorting require scrutiny at close range.
- Hair follows the head rigidly. No hair/cloth simulation, strand physics,
  facial rig, blinking, lip sync, eye tracking or expression controls are added.
- The dress uses a simple smooth pelvis/thigh skinning approximation, not a
  production garment solver. Kneeling, sprinting, steep slopes and extreme
  poses are untested and can intersect. Shoulder-strap skinning still needs
  review in more extreme poses. The stock lace-up shoes/socks are a provisional
  practical choice, not a finalized preindustrial footwear design.
- Procedural idle/walk are useful motion/rig tests, **not** polished locomotion:
  no foot IK, turn/stop transitions, root-motion gait, blend space or slope
  adaptation. Foot sliding can remain.
- The source is intentionally review-quality rather than optimized. No LODs,
  material atlas, collision/physics asset, platform memory budget or gameplay
  performance measurement has been delivered.
- Under-outfit body surfaces are absent in playable meshes. A future outfit
  swap must rebuild the correct occlusion mask; removing the tunic at runtime
  would reveal missing geometry, not a usable unclothed body.
- Actual Unreal/daylight/gameplay-camera visual approval and Jenny's aesthetic
  review remain separate gates from technically valid FBX exports.

## Runtime appearance material contract

`Assets\Characters\Heroine\appearance-contract.json` is generated **from the
imported Unreal meshes**, not from an assumed FBX material ordering. It includes
each preset's actual zero-based slot indices, slot names, material paths, roles,
appearance groups, supported parameters and actual imported bone names.

Create per-character dynamic material instances for the relevant slots; do not
change shared base-material defaults at runtime.

| Group / slots | Parameter | Neutral default |
|---|---|---|
| Skin: `M_Heroine_Skin` | `ColorTint` vector | `(1,1,1,1)` |
| Hair: `M_Heroine_Hair_long01`, `M_Heroine_Hair_bob01` | `ColorTint` vector | `(1,1,1,1)` |
| Brows: `M_Heroine_Eyebrows` (hair appearance group) | `ColorTint` vector | `(1,1,1,1)` |
| Tunic: `M_Heroine_MossLinen`, `M_Heroine_LinenTrim` | `ColorTint` vector | `(1,1,1,1)` |
| Eyes: `M_Heroine_LightEyes` | `IrisColor` vector and `IrisMix` scalar | color `(0.15,0.40,0.55,1)`, **mix `0`** |

`ColorTint` multiplies the existing texture RGB or constant authored albedo.
White preserves the authored look exactly. These are linear RGB multipliers,
**not absolute replacement colors**. Nonnegative, modest multipliers are
recommended. Dark pixels remain dark; this is not a replacement skin library.
Leather, footwear, brass, eyelashes, teeth and tongue are deliberately fixed.

Eyes do **not** use whole-eye tinting. The original RGB is interpolated toward
`IrisColor` multiplied by texture brightness, weighted by clamped `IrisMix` and:

```text
chroma = max(R,G,B) - min(R,G,B)
irisMask = saturate(8*(chroma-.08))
         * saturate(8*(max(R,G,B)-.08))
         * saturate(8*(.90-max(R,G,B)))
```

Thus achromatic sclera whites and very dark pupil pixels have zero recolor
weight. `IrisMix=0` preserves all original eye RGB exactly. This is a heuristic
for the supplied blue-eye atlas, not semantic segmentation or a guarantee for
other textures; dark/desaturated iris regions may retain the original color.
For skin, hair, brows, eyes and all other materials, the existing texture alpha
connection and masked/opaque material mode remain unchanged.

The importer checks every material connection and explicit asset-save result.
It verifies the compiled parameter names and neutral defaults before reporting
success. Failure removes the prior success report/contract instead of leaving
stale success evidence.
Every generated heroine base material explicitly sets and verifies
`used_with_skeletal_mesh=True` before saving. This is a persisted packaging
requirement, not reliance on editor auto-usage detection; dynamic material
instances inherit the base material's skeletal shader support.
The importer also explicitly saves the generated shared skeleton before its
referencing meshes/animations. FBX task auto-save did not reliably persist this
dependent package: a same-process success report alone was insufficient.
UE 5.8 may route FBX imports through Interchange even with an explicit
`FbxFactory`; this importer now disables that FBX route **for its process only**
so its `FbxImportUI` settings are authoritative. No project configuration or
engine installation setting is changed.
`Scripts\Characters\verify_unreal_materials.py` performs a fresh-process,
read-only check of saved skeletal usage flags, neutral defaults, alpha modes,
slot assignments, mesh scale and shared skeleton references on both meshes
and both clips. Its success evidence is
`Build\CharacterPreview\skeletal-material-usage-verification.json`.
The final import and separate verification both exited successfully with **zero
errors and zero warnings**. All **13** shared/alternative base materials had the
saved skeletal usage flag, checked before loading a mesh could auto-enable it.
The shared skeleton `.uasset` now exists on disk and both meshes/clips reload
with that same skeleton. Four prior generated packages were backed up under
`Build\CharacterPreview\PreSkeletonRecovery` before a clean FBX regeneration;
these are recovery history, not assets to cook or use.
Existing correctly scaled mesh and animation assets are reused for appearance
updates. This avoids an observed UE atomic-FBX-reimport unit regression.
When geometry must be imported/repaired, both cached asset import data and task
replacement settings explicitly enable scene/unit conversion at uniform scale 1.
The initial appearance commandlet restored/verified 160.6 cm and 161.8 cm heights.
Its package-reference warning prompted the stronger skeleton-persistence and
fresh-process verification above. Final packaging-usage logs are
`Build\CharacterPreview\skeletal-material-usage-commandlet.log` and
`skeletal-material-reload-commandlet.log`; the earlier appearance import logs
remain diagnostic history, not the final persistence evidence.

The actual indices are identical across the two imported presets: skin **1**,
eyes **2**, brows **3**, hair **7**, tunic **9**, trim **10**. Hair slot 7 uses
`M_Heroine_Hair_long01` for LongWave and `M_Heroine_Hair_bob01` for Bob. Prefer
the generated contract or slot names over assuming these indices stay fixed
after a future re-export.

### Deterministic facing inference bones

The imported rig uses `Root` (capital R), `head`, `foot_l`, `foot_r`, `ball_l`,
and `ball_r`. `ball_l`/`ball_r` are the toe/forefoot bones; there are no separate
`toe_l`/`toe_r` names. Use **component-space reference-pose** bone positions:
average `(ball_l - foot_l)` and `(ball_r - foot_r)`, project onto XY and normalize.
This avoids guessing the imported forward axis or using an animated frame.
Compose local reference transforms through the parent chain if reading
`FReferenceSkeleton` poses directly.

This has now been measured from the actual UE reference transforms for **both**
presets: component-space forward is **`(0,+1,0)`**, within floating-point
tolerance. A mesh relative yaw of **−90 degrees** points the asset toward
Unreal's canonical +X. Apply that correction once, not in addition to another
template mesh correction. The contract records the exact bone positions,
forward vector and correction angle. The source FBX-axis expectation was not
sufficient to establish this; use the measured engine result.
