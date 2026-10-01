# Proposal

## Why

Jenny's latest playtest finds the heroine's walk, overall appearance, and tool use well below the game's intended quality, while Craft repeats oversized prose and the hotbar advertises tools she has not made. The next playable round should make the heroine feel more alive and improve tool readability before expanding unrelated world systems.

## What Changes

- Replace Craft's stacked ingredient text cards with compact original item icons, short names, numeric owned/required counts, and distinct met/missing cues. Reveal sourcing and full accessible descriptions on hover or focus, without hiding the pre-hatchet Fiber route. Retained tools, cookfire, and pack capacity remain truthful.
- Keep the requested ten numbered hotbar positions but display no uncrafted or non-carried tool icons. A tool appears when actually carried and disappears on storage/drop; its stable assignment may return when recovered. Do not grant a tool or change inventory by drawing the hotbar.
- Show a small original Knife in the heroine's right hand when its hotbar slot is hovered or selected, with a distinct cutting/clearing motion for successful Knife work. The existing Knife already permits low-growth clearing, gathering and early crafting; make its actual work visible instead of playing the hatchet chop. Hover is presentation only, not use or selection.
- Prioritize a visibly more human walk/start/stop/turn and a distinct toggle sprint before the broad distant-world/time-card polish. Sprint has zero sprint-specific Energy cost, is refused/turned off below about 25% Energy, and remains capsule/Simulation authoritative; below about 10% walking slows and tools refuse without an Estate failure state. Evaluate Epic's Game Animation Sample as a read-only retargeting reference; adopt only a licensed, compatible, measured subset if it outperforms the existing original animation pipeline.
- Replace the current disliked heroine if a genuinely free, distributable, higher-quality base passes licensing, access, rig/retarget, outfit fit, performance and normal-game review. This is the preferred art direction, not an optional polish of the current face. Keep the CC0 MPFB base as rollback; if external candidates fail, substantially re-author its face/body, skin, eyes, hair edges and silhouette rather than shipping a cosmetic lighting tweak. Preserve deep body/hair/color/clothing customization. MetaHuman is an aspiration, not a dependency or a promised photorealistic result.

**First playable demonstration:** A better-looking heroine candidate or substantially re-authored fallback is shown in the actual woodland, not just a studio render. At the normal gameplay camera she walks, starts and stops without the current stiff loop, toggles Shift or controller L3 for a distinct sprint, selects or hovers the carried Knife and sees it in hand, and performs one real low-growth clearing action with a Knife-specific gesture. Opening Craft shows compact icon/count requirements; uncrafted hotbar tools are visually absent.

**Full acceptance:** Review normal 720p/4K and neutral-day/night gameplay plus close outfit/face views across representative bodies, hair and clothing; verify exact crafting assessment, earned tool visibility, authoritative cutting, zero sprint-specific Energy cost/low-Energy cutoff/cancellation, stable locomotion and prior content/preview rollback. A visible subjective improvement is required; an imported asset or passing geometry test alone is not character approval.

**Reuse finding:** Existing recipe assessment, original icons, 53-bone rig, modular garments, CC0 MPFB source and work-action authoring are available now. Epic's [modular-character guidance](https://dev.epicgames.com/documentation/en-us/unreal-engine/working-with-modular-characters-in-unreal-engine) identifies City Sample Crowds as a possible trial; the [Game Animation Sample](https://dev.epicgames.com/documentation/en-us/unreal-engine/game-animation-sample-project-in-unreal-engine) supports character imports and motion comparison. Neither Fab item's current license, access or fit is verified here, so no outside asset is approved for download or shipping. The concrete gaps are an appealing base character, natural walk/sprint, a Knife-specific prop/gesture and quieter requirement presentation.

Deferred work: combat, new hunting violence, new tools/recipes, generalized hand IK, a separate stamina stat, motion-matching adoption without a successful trial, paid asset purchases, blind asset replacement before game-camera review, and the unrelated 9x9 world/view-distance/time-HUD expansion.

## Capabilities

### New Capabilities

- `icon-first-crafting-requirements`: Compact accessible icon/count requirements with conditional source, tool, station and capacity detail.
- `earned-tool-and-knife-presentation`: Truthful hotbar icons, transient held Knife, and Knife-specific clearing presentation without inventory authority changes.
- `heroine-motion-quality`: Integrated walk/sprint/action presentation quality gates that prioritize the existing pending sprint and locomotion contracts without duplicating their mechanics.
- `heroine-visual-quality`: Preferred heroine-base replacement with reversible comparison and game-camera acceptance across customization; substantive re-authoring only if free candidates fail.

### Modified Capabilities

None. The main OpenSpec capability inventory is empty; related in-flight change specs are coordinated below rather than silently rewritten or marked complete.

## Impact

- Reuse `Simulation::AssessRecipe`, the current ingredient/source metadata and 37 original Slate icons; Craft remains authority-backed and hold-to-craft stays 1.2 seconds.
- Reuse the ten-slot saved hotbar, carried tool count, current Knife `Harvest`/`Clear` and apparel ownership. Add only presentation states and the essential original Knife prop/action clip to the retained 53-bone character rig.
- The unstarted `improve-contextual-feedback-and-sprint` and `polish-locomotion-view-distance-and-time-hud` changes already specify sprint and walk mechanics; `refine-equipment-preview-and-idle` and `refine-playable-heroine-hairstyles` own portrait/idle and rejected wave ends. Before implementing overlapping work, reconcile ownership with those artifacts so one accepted route closes each task; do not falsely mark them implemented or make world streaming a prerequisite for character improvement.
- Preserve `drop-02-shipping / inventory-drop-v18` until a character-first immutable candidate passes. Current test saves remain disposable, but current-version saving and explicit reset behavior remain mandatory.
