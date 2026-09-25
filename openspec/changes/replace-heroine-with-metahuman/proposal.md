# Proposal

## Why

Jenny wants the heroine to look like she belongs in a 2026 game on her RTX 5080: photorealistic
at MetaHuman quality, in the spirit of Skyrim and Tomb Raider characters. The current heroine is a
CC0 MakeHuman/MPFB prototype (53-bone custom skeleton, joined card-like hair, Blender-keyed
animation). Many trials failed to make it attractive (face sculpts, wave/bob hair, Vitruvian face,
re-authored meshes), and Jenny still describes the movement as robotic. On 2026-09-25 she chose
MetaHuman and agreed to create/sign in to an Epic account for its cloud steps.

## What Changes

- Replace the playable heroine with a MetaHuman authored in the in-editor MetaHuman Creator:
  softly sculpted face with slightly sharp cheekbones, a slim nose, moderately full lips, defined
  brows, light eyes, and long dark-brown hair, on a petite/curvy young-adult body of about 1.60 m.
  Assembled with the **UE Optimized / High** pipeline and a full face rig, with strand hair at
  close range.
- Establish a modern rendering baseline the MetaHuman needs, which also benefits the whole world:
  D3D12 **SM6** (Virtual Shadow Maps actually work), **hardware ray-traced Lumen**, the MetaHuman
  skinning settings (16-bit bone indices, unlimited bone influences, skin cache), and a measured
  60 FPS 4K target with upscaling on the target PC.
- Replace robotic locomotion with **motion-matched** walking, starts, stops, turns and sprint from
  Epic's Game Animation Sample (GASP), plus foot placement on slopes.
- Move the existing work actions (gather, chop, water, till, knife) onto the new skeleton, then
  improve them with hand/tool contact and target alignment.
- Add facial life: blinks, eye saccades, breathing and look-at.
- Map the existing Appearance options (hairstyle, hair colour, eyes, skin tone, outfit/dye) onto
  MetaHuman grooms, materials and garments. Hair, eye and skin colour stay player-configurable in
  game; the authored look is only the default. Rebuild the tunic, apron and footwear as
  MetaHuman-fitted clothing.
- **BREAKING (test saves):** the character's skeleton and assets change. Saved appearance IDs are
  preserved where they map; incompatible test saves may reset, which is acceptable for current
  playtesting.

Reuse research: MetaHuman Creator core data (727 MB) and a Python API
(`MetaHumanCharacterEditorSubsystem`) ship with UE 5.8. The engine is missing only its optional
content pack. MetaHuman is free to ship in an Unreal game until a product's lifetime gross revenue
reaches $1M (standard UE EULA). GASP is free on Fab, updated for 5.8, and shippable inside Unreal
projects. It already includes a MetaHuman player template and retargets, and runs on Pose Search
with Chooser (the Mover plugin isn't needed). Grooms and MetaHuman-compatible clothing are
available through MetaHuman Creator's wardrobe and Fab's MetaHuman channel. The editor MCP
toolset (`.github/skills/unreal-editor-mcp`) lets agents script MetaHuman Creator, play the game and
capture results. Custom work is needed only for the gaps: game-specific clothing,
homestead work animations (not in GASP), integration with `AHomesteadCharacter`, and appearance
mapping.

Manual steps only Jenny can do: install the MetaHuman optional content from the Epic Launcher;
sign in to Epic and accept the MetaHuman consent when the editor prompts; add GASP (and any chosen
Fab MetaHuman garments) to her Fab library.

Smallest useful result and first playable demonstration: the new MetaHuman heroine, in a starter
outfit, walking and gathering in the real Homestead woodland with the existing animations retargeted,
captured in ordinary play. Full-round acceptance adds motion-matched locomotion, improved work
actions, facial life, full appearance/wardrobe mapping, and Jenny's review of a packaged Shipping
build at 720p/4K. Additional body presets are deferred until one heroine is approved.

## Capabilities

### New Capabilities

- `metahuman-heroine`: the playable heroine is a photorealistic MetaHuman with mapped appearance
  options, fitted clothing, offline runtime, and recorded provenance.
- `modern-rendering-baseline`: the renderer features and performance target that photoreal
  characters and the world rely on.

### Modified Capabilities

- `heroine-motion-quality` (in-flight in `prioritize-heroine-quality-and-tool-clarity`; the main spec
  inventory is empty): adds motion-matched locomotion, foot placement, tool contact and facial life.
  This change supersedes that change's remaining appearance and walk tasks (1.1, 2.1, 4.1-4.3),
  which should be reconciled when this change lands.

## Impact

- `SurvivalGame.uproject` and `Config/DefaultEngine.ini`: MetaHuman, Pose Search, Chooser, Motion
  Warping, Hair Strands and RigLogic plugins; SM6, hardware ray tracing, skinning settings. This
  triggers one full shader recompile.
- `Source/SurvivalGame/HomesteadCharacter.*`, `HomesteadAnimInstance.*`,
  `HomesteadAppearance.*` and the menu's Appearance/portrait code.
- New content under `/Game/Characters/Heroine_MH/` and `/Game/Animation/GASP/` (Git LFS; expect a
  large increase in repository and package size, up to a few GB).
- `Assets/` manifests and `docs/asset-credits.md` for MetaHuman, GASP and any Fab garment
  licenses.
- Rollback: the current prototype heroine and `inventory-drop-v18` stay available until Jenny
  approves the new heroine.
