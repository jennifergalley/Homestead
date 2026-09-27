# Tasks

## 1. Source assets (Blender)

- [x] 1.1 Build a parametric last from her body mesh, with verified skin gaps and the fur boot
  shaft ≥ 18 mm off the skin.
- [x] 1.2 Model FurBoots, WovenSandals and TurnShoes, left foot then mirrored, with original
  construction details.
- [x] 1.3 Synthesize 2048² basecolor, normal (OpenGL), roughness and AO maps per pair.
- [x] 1.4 Transfer weights from the body, cap them at 8 influences and normalize them. Export
  the FBXs to the Unreal conventions.
- [x] 1.5 Write the body coverage masks, and measure poke-through in bind, walk, kneel, squat
  and tiptoe.
- [x] 1.6 Render the 4K reviews: each pair on her feet, close-ups, the test poses, and the boots
  over a trouser stand-in.
- [x] 1.7 Write the READMEs, `report.json` with the input and output hashes, and the credits
  entry. Push the branch and hand off to the coordinator.

## 2. Unreal integration (coordinator)

- [x] 2.1 Import the three FBXs onto `metahuman_base_skel`, with materials and a flipped green
  channel on the normals.
- [x] 2.2 Lift the character by the sole thickness: FurBoots 1.2 cm, WovenSandals 1.05 cm,
  TurnShoes 0.5 cm (mesh Z and foot-IK ground height).
- [ ] 2.2a Apply `BodyCoverageMask_<Name>` while each pair is worn. Deferred: the body still
  renders under footwear, and the fitted clearance hides it at bind pose.
- [x] 2.3 Hook the warmth ratings (sandals 0, turnshoes 1, fur boots 4) into the survival
  temperature model (`Insulation()` offsets seasonal cold loss).
- [ ] 2.4 Verify the pairs in kneeling and squatting gameplay views (standing views are checked
  in PIE).
