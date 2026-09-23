# Design

## Context

See `proposal.md` and both capability specs. Inventory currently renders a labeled horizontal bar of three wide equipment buttons (Torso, Apron, Feet). Simulation has Torso, Legs, Apron, and Feet slots; the tunic occupies Torso+Legs. The menu preview is a 240-wide bordered Pine panel with status copy plus turn-left, turn-right, and zoom buttons. Its 768x1536 render target clears to opaque dark green, follows the gameplay mesh pose, and captures only when explicitly refreshed, so the displayed heroine is effectively frozen.

Runtime already uses `AN_Heroine_RelaxedIdle`, a three-second 60 FPS loop with planted feet and tiny procedural spine/arm breathing. The motion is valid but around half a degree in the spine and not perceptible enough at gameplay distance. The current skeleton, wardrobe leader-pose system, icon renderer, scene-capture actor, analytical authoring script, and animation proxy are reusable.

## Goals / Non-Goals

**Goals:**

- Replace oversized labeled equipment controls with a compact paper-doll composition.
- Make the fixed transparent heroine preview feel alive and useful for clothing review.
- Strengthen the shared idle enough to read naturally without becoming stylized or restless.
- Preserve authoritative ownership, current wearable behavior, menu navigation, and action/locomotion blending.

**Non-Goals:**

- New clothing content, drag/drop, portrait manipulation, facial animation, blinking, multiple random idles, cloth/hair physics, or a wholesale inventory redesign.

## Decisions

### 1. Append semantic slots without renumbering current ones

Keep current Torso=0, Legs=1, Apron=2, Feet=3 IDs and append Head and Hands. Increase the validated equipment array and current save version; fresh/disposable test saves may reset with explicit disclosure. Current wearable masks remain unchanged, while Head/Hands validate as empty until later content exists.

UI order is Head, Hands, Torso, Legs, Feet, with Apron below or offset as an accessory layer. This gives Jenny's requested body map without pretending apron is a body part. Multi-slot tunic shows its icon in Torso and Legs but remains one wearable instance.

**Alternative considered:** draw decorative Head/Hands icons without state. Rejected because focus/details would imply equipment support that authority does not recognize.

### 2. Use a compact icon paper doll beside the preview

Remove the heading and build six approximately 44-52 virtual-pixel square slots in a compact vertical/two-column cluster around the fixed character. Empty slots use new original line/silhouette kinds for head, hands, torso, legs, feet, and apron. Occupied slots use the wearable's current icon/tint. Focus uses the existing Gold border plus shape/contrast; hover/focus details name the slot and item.

The character image is not focusable. Directional geometry links actual slots to inventory/details, reducing one dead navigation region and supporting the quieter-menu policy.

### 3. Make the capture transparent and fixed

Remove portrait border, Pine backplate, status copy, turn buttons, zoom button, orbit/zoom APIs, and right-stick/pointer portrait routing. Set a transparent render target/capture source that preserves alpha, keep show-only character components and hidden neutral lights, and composite the image directly over the parent menu surface.

Use one fixed full-body framing at a slight three-quarter yaw chosen to show outfit silhouette and face without user manipulation. Recompute bounds after clothing changes but retain angle/framing policy. A 512x1024 target captured around 24 FPS only while Inventory/Appearance is visible bounds cost while making subtle idle readable.

**Alternative considered:** keep a neutral portrait rectangle. Rejected because Jenny explicitly wants no visible background and a Coral Island-like integrated character composition.

### 4. Give the portrait its own admitted idle playback

The capture body plays the same relaxed idle sequence locally; garment components leader-pose to it. Do not leader-pose the portrait body to the paused gameplay mesh. Continuous capture advances only the transient portrait animation and never Simulation/world time. Clothing refresh preserves normalized idle phase where feasible instead of restarting at frame zero.

### 5. Reauthor one calmer, more readable idle

Extend the loop to approximately five seconds and author one natural breath with slight asymmetry: subtle chest/spine expansion, small shoulder rise, less than about 1.5 cm weight shift, restrained head tilt/yaw, relaxed arm/hand/finger response, locked foot contacts, and coincident endpoints. No random runtime bone noise is added; authored deterministic motion is easier to verify and remains identical in gameplay/portrait.

The locomotion proxy continues using the same idle/walk blend and work-action overlay. Offline checks add amplitude floors (motion must be perceptible) and ceilings (no sway/fidget), in addition to skeleton, bind, scale, roots, feet, seam, and notifies.

### 6. Parallelize UI and animation behind stable contracts

The UI lane owns equipment enum/save migration, icons, paper-doll layout, portrait capture, and menu tests. The animation lane owns only idle source/FBX/verification/import. Integration agrees on final idle asset path and equipment slot order, then runs shared Editor/cook/package serially.

## Risks / Trade-offs

- **[Transparent capture has alpha/fringing artifacts]** -> verify capture source, premultiplication and hair/eyelash edges at 720p/4K before removing the old backplate.
- **[Continuous portrait capture costs GPU time]** -> reduce target to display-appropriate resolution, cap at ~24 FPS, tick only while visible, and compare menu cadence.
- **[Head/Hands save expansion rejects old tests]** -> bump current disposable save version with clear reset disclosure; never silently reinterpret old arrays.
- **[Idle becomes exaggerated or repetitive]** -> use one slow asymmetric loop with measured amplitude bounds and ordinary long-view review.
- **[Icon-only slots become ambiguous]** -> distinctive silhouettes plus focus/hover details, without restoring permanent text labels.

## Migration Plan

1. Record current slot/portrait/idle behavior and Coral Island reference notes.
2. Add validated Head/Hands state plus compact empty/occupied body/accessory slots.
3. Replace the portrait panel with fixed transparent continuous idle capture.
4. Author/import the living idle and integrate it with gameplay/preview.
5. Run inventory/wardrobe/save/navigation, appearance/action/locomotion, 720p/4K visual and cadence acceptance.
6. Build one immutable Shipping candidate and retain the selected work-animation build as rollback until explicit promotion.

