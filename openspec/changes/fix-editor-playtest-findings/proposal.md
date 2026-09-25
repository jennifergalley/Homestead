# Proposal

## Why

On 2026-09-25 an agent played a fresh woodland in Play-In-Editor through the new editor MCP play
toolset (`.github/skills/unreal-editor-mcp`). Five defects surfaced that Jenny wants recorded and
fixed later. Some break existing requirements (Settings focus, feedback not covering or moving
content); the others make ordinary play look wrong (trees vanish, flat creek, lighting warning). Jenny later
reported a sixth from her own play: shadows jumping as the sun moves.

## What Changes

- **Settings row skipping.** Each D-pad/arrow Up/Down moves two Settings rows, so Load latest
  save, Camera sensitivity, Music volume and Effects volume can't be reached by directional input.
  Settings shall move exactly one visible row per step.
- **Felled trees vanish.** A successful RT fell removed the mature tree within 0.8 s, with no
  visible fall, stump or debris. Felling shall end with a readable removal tied to the chop impact.
- **Flat creek water.** The creek rendered as a flat, uniformly saturated blue ribbon in ordinary
  views. It shall read as natural shallow water in the packaged game.
- **Competing directional lights.** PIE shows the engine warning "Multiple directional lights are
  competing to be the single one used for forward shading, translucent, water or volumetric fog."
  Sun and moonlight shall have one deterministic forward-shading owner.
- **Shadows jump as the sun moves** (Jenny, 2026-09-25). The sun stepped by 0.5 degrees to keep
  Virtual Shadow Map re-renders rare, which visibly jumps long low-sun shadows every few seconds
  and stalls a frame each step. Shadows shall move smoothly without frame-time spikes.
- **Menu feedback reflows content.** The field-book feedback banner (for example "Ate Berries.")
  pushed the Inventory page down about 60 px and shrank the portrait. Feedback shall not move rows,
  focus targets or the portrait.

Reuse: every fix works inside existing systems. The native menu's own grid navigation, the
existing chop animation and tree-clearing transaction, the current creek terrain/material
pipeline, the existing sun/moon components in `HomesteadWorld`, and the existing book feedback
placement. No new assets or libraries are expected. The editor MCP play toolset is the
reproduction harness.

Smallest useful result: one Settings list that steps row by row with the D-pad (first playable
demonstration). The other four fixes are independent increments. Each is verified by an ordinary
PIE or packaged playtest capture of the same action.

## Capabilities

### New Capabilities

None in intent. The main spec inventory is currently empty, so each delta below adds requirements
under a capability path already defined by an in-flight change, and carries a `## Purpose` in case
this change archives first.

### Modified Capabilities

In-flight capabilities extended with new, distinctly named requirements:
- `settings-screen-navigation`: directional steps move exactly one visible row.
- `homestead-work-animations`: felled mature trees leave the world visibly after impact.
- `woodland-creek-presentation`: creek water surface reads as natural water.
- `woodland-environment-presentation`: one directional light owns forward shading at a time; sun shadows move smoothly.
- `homestead-menu-overlay`: menu feedback never reflows page content.

## Impact

- `Source/SurvivalGame/UI/SHomesteadMenu.cpp` (Settings column count vs layout; feedback banner
  placement).
- `Source/SurvivalGame/HomesteadWorld.cpp` (sun/moon directional light settings; tree clearing
  presentation) and related animation/notify code.
- Creek water material/terrain presentation used by the generated woodland.
- First delivery is a per-bug increment verified in PIE; full acceptance also needs the packaged
  Shipping build at 720p and 4K. No save format, simulation rule or input binding changes.
