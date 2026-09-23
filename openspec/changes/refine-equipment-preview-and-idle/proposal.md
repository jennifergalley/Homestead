# Proposal

## Why

The inventory devotes too much space to three labeled equipment buttons and frames the heroine as a separate rotatable portrait tool rather than part of the wardrobe composition. The existing three-second relaxed idle technically contains breathing, but its motion is too subtle to read in gameplay and the menu portrait captures only a static frame.

## What Changes

- Replace the large `Equipped slots` bar and heading with a compact icon-only paper-doll cluster beside the character.
- Expose five intuitive body slots in visual order: Head, Hands, Torso, Legs, and Feet. Empty slots show original body-part silhouette icons; occupied slots show the equipped clothing icon. Preserve the apron as a smaller separate accessory/layer slot rather than mislabeling it as a body part.
- Extend equipment-state validation for currently empty Head and Hands slots without inventing headwear/gloves in this round or changing current tunic/apron/footwear ownership.
- Remove persistent slot text; hover/focus details may identify the slot and equipped item contextually.
- Replace the portrait's bordered Pine panel, status copy, rotation buttons, zoom button, and right-stick/mouse orbit with one fixed full-body, slightly three-quarter character presentation composited directly into the inventory/Appearance layout over the existing menu surface.
- Give the portrait a transparent capture background and no separate frame/backplate. Keep neutral presentation lighting but no visible studio scene or invented scenery.
- Play the actual relaxed idle continuously in the inventory/Appearance preview rather than capturing a frozen pose.
- Reauthor the shared gameplay idle as a natural readable living loop with breathing, minute weight shift, subtle head/shoulder/hand movement, stable feet, and no exaggerated swaying or fidgeting.
- Preserve the admitted heroine skeleton, body/hair/garment system, save/wardrobe authority, focus navigation, pointer/controller/keyboard selection, collision, locomotion blend, and work-action overlays.

Coral Island inventory/outfit screenshots and descriptions contribute only the compact paper-doll hierarchy, small body-part empty-state symbols, and static full-character outfit review ([inventory reference](https://coralisland.fandom.com/wiki/Inventory), [Steam screenshots](https://store.steampowered.com/app/1158160/Coral_Island/)). Homestead follows Jenny's stronger direction—no visible portrait background and no rotation—even where a reference uses a neutral frame. No proprietary art, icons, exact spacing, frame shape, or character pose will be copied.

The smallest useful in-game result removes the equipment heading/labels and portrait controls, shows five small body icons plus an apron accessory slot around a fixed transparent full-body heroine, and lets every current wearable still equip/unequip correctly. The first playable demonstration watches her breathe/move naturally while standing in the world and in the open inventory, then changes tunic/apron/feet and sees the portrait update without freezing or rotating. Full acceptance covers every empty/occupied slot, all body/hair/garment combinations, navigation/focus, save/reload, 720p/4K layout, menu capture lifecycle, locomotion/action blending, and immutable Shipping replay.

Deferred scope includes actual hats, gloves, glasses, jewelry, backpacks, new garments, drag-and-drop equipment, portrait poses/emotes, manual camera controls, multiple idle variants, facial animation, blinking, hair/cloth physics, and a full character-screen redesign beyond the inventory/Appearance composition.

## Capabilities

### New Capabilities

- `equipment-paperdoll-ui`: Compact icon-only body/accessory equipment slots and a fixed transparent animated character preview.
- `living-idle-presentation`: Readable natural resting motion shared by gameplay and character preview without affecting movement or authority.

### Modified Capabilities

None.

## Impact

- Simulation equipment enum/array validation and current-version save serialization: append empty Head/Hands slots while preserving current wearable semantics.
- `SHomesteadMenu`, `SHomesteadIcon`, wardrobe row/focus tests: compact body/accessory slot cluster, empty/occupied icons, contextual detail, and removal of portrait controls/status/background.
- `HomesteadMenuPortrait`: transparent fixed framing, own looping idle playback, continuous bounded capture, garment synchronization, and no orbit/zoom API.
- `build_locomotion.py`, verification/import assets, `AN_Heroine_RelaxedIdle`, animation proxy and character pipeline docs: more visible natural idle shared with gameplay.
- Existing project-authored content and Unreal facilities are sufficient; no external asset, package, account, purchase, download, or license is required.

