# Design

## Context
See proposal.md. Store goods and pack-row already support ordinary carried tools. Open water splines represent rivers, closed shores lakes, and the existing terrain/sea probe finds the coast. Crop recipes share native ingredients, assessment, cooking and Energy metadata.

## Goals / Non-Goals
Deliver original active fishing and Jenny's explicitly required all-new item art without duplicating inventory, economy, food or water infrastructure. No bait, new animations, grip/carry tuning, terrain placements, fishmonger or travel changes.

## Decisions
- Native simulation owns readiness, casting cost, timing, catch identity and inventory award. Unreal supplies water classification; it cannot supply a catch. A pole costs exactly 1500 regardless of the general markup.
- Cast with tool button or E/A. Wait 2-4 seconds; react within 0.9 seconds of the bite, then press within the highlighted 55-85% band on two 1.8-second landing passes. Wrong/late presses release the fish; walking away, changing item, opening a menu or cancelling ends the cast. This compact timing game is original, not copied art/code.
- River trout/salmon, lake perch/carp, sea mackerel/bass are separate pools. Hash saved world/hour/location for reproducible casts without a new RNG/save field. In-flight fishing is ephemeral and cancels on successful load/new game; consumed casting Energy remains saved.
- Reuse recipe ingredient/output data. Raw mackerel slices need no fire/fuel; seven cooked fish dishes use one kindling and existing hearth/fire rules. Energy follows the crop-meal sale-opportunity formula.
- A themed Slate timing card above the hotbar reads constant-size session state. Original Blender pole, six species-specific catches and eight prepared dishes replace reused meshes and generic food/fish presentation. Shared new material/UV families are acceptable; each item has distinctly authored geometry and viewed 4K evidence. No new animation lane.

## Risks / Trade-offs
Jenny's 20:00 permission clears the playtime hold; Integration owns current-source compile/import/verification. Its successful combined compile is not original-art acceptance. Freshwater wins near river mouths when both fresh and sea probes overlap. Original assets may roll beyond tonight; do not promote placeholder art to meet the deadline.

## Migration Plan
Append Item/Recipe ids only, widening existing counted stocks. No save sections/version/bake changes; preserve Travel Rest's independent optional travel section. Integration handles editor compile and release admission, Jenny manual acceptance.
