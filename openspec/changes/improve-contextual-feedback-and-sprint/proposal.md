# Proposal

## Why

The gameplay HUD currently displays interaction and menu guidance even when nothing nearby can be used, while successful gathering reports verbose renewal prose instead of the inventory change the player cares about. Traversal also has only one walking pace, so crossing the generated woodland feels slower and less expressive than intended.

## What Changes

- Hide the bottom-right context panel when no resource, plot, work structure, or water interaction is actually focused.
- Remove the persistent top-right field-book/camera keyboard or controller strip entirely; nearby context shows only controls relevant to the focused interaction.
- Keep rejection/error feedback visible, but replace successful resource-gather messages with a short pickup toast listing the exact item quantities added to the carried inventory.
- Shorten ordinary world-action rejection toasts such as missing-tool guidance from eight to six seconds while retaining longer critical save, recovery, and settings failures.
- Replace the wide text-labeled Food/Energy/Warmth row with three compact icon-led meters stacked vertically in the lower-left corner.
- Remove renewal commentary from gather success messages. Renewable forage still renews silently.
- Make fallen-branch patches one-time pickups: successful gathering removes the patch permanently, persists the edit, and never schedules regrowth.
- Add hold-to-sprint on keyboard Shift and controller left-stick click, with faster grounded movement, a distinct authored sprint animation, and modest drain from the existing Energy resource.
- Stop sprinting automatically when movement, ground contact, Energy, menu, planning, failure, or action-presentation conditions no longer permit it.
- Reuse the current focus detector, Canvas HUD, transient toast lifecycle, Simulation inventory authority, Energy/save state, Enhanced Input mapping, movement component, retained heroine skeleton, analytical Blender authoring helpers, and animation proxy. No external asset or dependency is required.

The smallest useful in-game result removes the always-on `Woodland`/till/menu panels, stacks icon-led survival meters in the lower left, and replaces one successful gather with an exact `Added to pack` toast. The first playable demonstration additionally confirms a missing-hatchet rejection clears after six seconds, gathers a fallen-branch patch twice across save/reload to prove it remains gone, then holds Shift to enter a visibly distinct faster gait that consumes Energy and safely returns to walking. Full-round acceptance covers keyboard/controller parity, all representative forage yields, rejection-message classes, HUD layout at 720p/4K, sprint cancellation, persistence, animation quality, and immutable Shipping replay.

Deferred scope includes remappable bindings, a separate regenerating short-term stamina pool, crouch/dodge/climb locomotion, sprint particles or new audio, and a broader redesign of inventory, quest, survival-meter, or menu copy.

## Capabilities

### New Capabilities

- `contextual-hud-feedback`: Target-gated interaction prompts, shorter ordinary-action rejection feedback, compact icon-led survival meters, and concise exact inventory pickup feedback.
- `sprint-locomotion`: Hold-to-sprint input, Energy cost, faster movement, dedicated animation, and safe cancellation.
- `fallen-branch-persistence`: One-time fallen-branch gathering with durable non-renewal across save and revisit.

### Modified Capabilities

None.

## Impact

- `Source/SurvivalGame/HomesteadHUD.cpp/.h` and feedback tests: conditional context/control rendering and pickup-toast presentation.
- `Source/SurvivalGame/HomesteadController.cpp/.h`: focus visibility, gather inventory-delta formatting, sprint request/authority coordination, and error/success feedback policy.
- `Source/SurvivalGame/HomesteadCharacter.cpp/.h`, `HomesteadAnimInstance.cpp/.h`, input and visual tests: Shift/L3 bindings, movement speed, sprint state, animation blending, and cancellation.
- `Source/SurvivalGame/Simulation/HomesteadSimulation.cpp/.h` and portable tests: bounded Energy exertion plus persistent one-time branch gathering.
- `Assets/Characters/Heroine`, `Scripts/Characters`, and a fresh trial animation package: one original sprint clip using the existing skeleton and project-authored pipeline.
- Current save format remains compatible: branch removal uses existing sparse resource edits and sprint uses existing Energy. No new persistent field or legacy-save migration is required.
