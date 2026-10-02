# Spec Delta

## Purpose

Let the heroine wear period-plausible footwear that fits her body, animates with her rig, hides
her bare feet where it should, and tells the game how much warmth it gives.

## ADDED Requirements

### Requirement: Footwear is period-plausible and original

Each pair SHALL use only pre-industrial materials and construction:

- tanned or smoked leather, fur or sheepskin, plant-fibre cordage;
- hand stitching and leather thongs or laces;
- no rubber, plastic, machine eyelets or synthetic soles.

All geometry and textures SHALL be project-authored.

#### Scenario: Close review

- **WHEN** a pair is viewed in a 4K close-up
- **THEN** its construction (seams, thongs, weave, cuff) reads as hand-made, with no modern
  hardware

### Requirement: Footwear fits and follows the heroine

Each pair SHALL be one skeletal mesh with both feet on `metahuman_base_skel`. It SHALL use:

- centimetres, Z up and −Y forward;
- a `root` armature;
- deforming bones only;
- no leaf bones and no animation.

It SHALL show no vertices inside the body in the bind pose, except for sub-millimetre
vertices that are hidden between the toes. For the walk and tiptoe poses, per-pose poke-through
SHALL be measured and reported. For the kneel and squat poses, it SHALL be measured and reported
together with its cause.

#### Scenario: Import and walk

- **WHEN** the coordinator imports a pair onto the existing skeleton and the heroine walks
- **THEN** the footwear follows her feet without visible skin poking through

### Requirement: Footwear declares its sole offset, coverage and warmth

Each pair SHALL provide the following:

- It SHALL report its sole thickness and the matching character offset.
- It SHALL provide a body coverage mask in the body's UV convention:
  - boots and shoes hide the whole foot;
  - sandals leave the toes and top of the foot visible.
- It SHALL give a suggested warmth rating from 0 to 10.

#### Scenario: Equip boots over trousers

- **WHEN** the fur boots are worn over snug wool trousers
- **THEN** the boot shaft clears the trousers by at least 18 mm from the ankle to the top, and
  the character is lifted by the reported sole thickness so the soles rest on the ground
