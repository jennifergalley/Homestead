# Tasks

## Current state and next visible goal

Planned 2026-09-25 after live editor exploration. MetaHuman Creator opens through the MCP tooling
but is limited until the optional content is installed. Next visible goal: the new MetaHuman
heroine walking and gathering in the real Homestead woodland (first playable, task 4.3).
Verify each task with the editor MCP tooling (`.github/skills/unreal-editor-mcp/SKILL.md`) and
ordinary-play captures. Stills from MetaHuman Creator alone never count as acceptance.

## 1. Jenny's setup steps (manual)

- [x] 1.1 Jenny installs the MetaHuman Creator optional content for UE 5.8 from the Epic Launcher (engine Options); verify the editor no longer logs "MetaHuman Optional Content folder not found" and Creator shows its preset and wardrobe libraries.
- [ ] 1.2 Jenny signs in to Epic in the editor and accepts the MetaHuman consent at the first Download Texture Sources / Create Full Rig prompt; verify both cloud steps complete for the trial asset.
- [x] 1.3 Jenny adds the Game Animation Sample to her Fab library and creates the sample project from the Launcher; verify its project path is recorded for migration. (`E:\Unreal Projects\GameAnimationSample\GameAnimationSample.uproject`)

## 2. Rendering baseline

- [x] 2.1 Record a before baseline: packaged 720p and 4K woodland walk captures, p95/p99 frame times, and the Lumen/VSM/SM6 warnings from the log; verify evidence files exist. (Development `-game` route, not packaged; see `docs/research/rendering-baseline/`.)
- [ ] 2.2 Enable SM6, hardware ray tracing, Lumen hardware RT, mesh distance fields and the MetaHuman skinning settings in `DefaultEngine.ini`, then run the one-time shader rebuild; verify the editor and packaged logs show no missing-settings, VSM or Lumen ray-tracing warnings.
- [ ] 2.3 Compare after captures and frame times against 2.1 in the same routes; verify at least 60 FPS at 4K output with the chosen upscaling, and record lighting/exposure changes needed for dawn, midday, dusk and firelight.

## 3. Author the heroine

- [ ] 3.1 Script MetaHuman Creator to create `MHC_Heroine` from the closest preset, set body constraints (about 160 cm, feminine, petite/curvy), light eyes and skin, and capture front/three-quarter/profile portraits; verify Jenny picks a starting face from the captures.
- [ ] 3.2 Refine face and hair toward the reference (softly sculpted face, defined brows, full lips, long chestnut waves to mid-back) with capture rounds; verify Jenny approves the face and default hair in Creator captures under Homestead-like lighting.
- [ ] 3.3 Download 4K texture sources, create the full rig, assemble with UE Optimized / High into `/Game/Characters/Heroine_MH/`, and record the asset list, sizes and provenance in `Assets/` and `docs/asset-credits.md`; verify the assembled Blueprint renders in an editor level with strand hair.

## 4. First playable: MetaHuman in Homestead

- [ ] 4.1 Add a heroine data asset and move `AHomesteadCharacter` to a leader-pose body/face/groom/garment component stack loaded from it, keeping capsule, camera, input and saves; verify PIE starts with the MetaHuman visible and no missing-asset fallback.
- [ ] 4.2 Build an IK Rig for the 53-bone skeleton and an IK Retargeter to the MetaHuman skeleton; retarget idle, walk, slow walk, sprint, gather, water, chop, till and knife clips; verify each plays on the MetaHuman in PIE without broken limbs or floating feet at the gameplay camera.
- [ ] 4.3 Put the heroine in a starter outfit (a fitted tunic and shoes, or a Fab MetaHuman garment that meets the rustic brief) and run an ordinary PIE route (walk, sprint, gather berries, fell a tree, eat) through the MCP play toolset; verify captures show the MetaHuman working in the woodland and deliver them to Jenny.

## 5. Motion-matched locomotion

- [ ] 5.1 Migrate GASP locomotion content (Pose Search databases, Choosers, idles, starts/stops/turns/pivots, IK rigs and retargeters) into `/Game/Animation/GASP/`, trimming traversal and combat; record its licence; verify the content loads without missing references.
- [ ] 5.2 Create the body Animation Blueprint on the `UHomesteadAnimInstance` base with motion matching, add a trajectory component, and drive it from the existing movement; verify recorded PIE footage of start, stop, reverse, orbit-strafe and sprint shows planted feet and no sliding.
- [ ] 5.3 Add foot placement on slopes and small steps with Control Rig/IK; verify standing and walking captures on the creek bank show both feet grounded.

## 6. Work actions and facial life

- [ ] 6.1 Convert work actions to montages triggered after successful transactions, add tool attachment sockets and target alignment (motion warping or IK) for chop, water, gather, till and knife; verify captures of each show hand/tool contact with the target and unchanged transaction behavior.
- [ ] 6.2 Evaluate replacement mocap for work actions (Rokoko free packs, CMU, GASP subsets) against the retargeted clips on the new body, and adopt the better ones with licences recorded; verify side-by-side captures and Jenny's pick.
- [ ] 6.3 Add procedural blinks, eye saccades, breathing and look-at (focused resource in play, camera in the Appearance preview) on the face; verify a 10-second idle capture shows natural blinks and gaze shifts.

## 7. Appearance and wardrobe mapping

- [ ] 7.1 Map hairstyles (long, bob, ponytail) to stock grooms and hair colours to groom Melanin/Redness, with brows and lashes following; default to dark brown (Melanin 0.72, Redness 0.35); verify each style/colour in the Appearance preview and the world survives save and reload.
- [ ] 7.2 Map eye colours to iris parameters seeded from Creator's eye presets (default preset 8, green); verify each choice in the preview and world survives save and reload.
- [ ] 7.2a Author a small set of skin-tone texture variants of the heroine's face in Creator and swap them on the face and body materials at runtime; verify each tone shows no face/body seam in the preview and world and survives save and reload.
- [ ] 7.2b Hide body-preset options without MetaHuman equivalents and make legacy saves fall back to defaults; verify loading a save that used a hidden option doesn't error.
- [ ] 7.3 Author MetaHuman-fitted tunic, apron and footwear with dye parameters, with cloth on the apron hem if needed; verify all dye/layer combinations through walking, sprinting and chopping show no body poke-through at the gameplay camera.
- [ ] 7.4 Update the Appearance portrait to frame the MetaHuman with suitable preview lighting; verify 720p and 4K captures.

## 8. Integrated acceptance

- [ ] 8.1 Build a packaged Shipping candidate, run the ordinary new-world route plus save/quit/reload offline, and record 720p/4K captures, p95/p99 frame times and package size; verify no network or Epic service access occurs.
- [ ] 8.2 Jenny reviews the candidate against the prototype heroine rollback in daylight, dusk and firelight; record her verdict here and promote only if she approves, keeping the prototype as rollback until then.
- [ ] 8.3 Reconcile `prioritize-heroine-quality-and-tool-clarity` tasks 1.1, 2.1 and 4.1-4.3 and the hairstyle/idle changes against this delivery; verify their tasks record what this change superseded.
