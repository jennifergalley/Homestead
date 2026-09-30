# Tasks

## 1. Recipes (Props; Blender slot)

- [ ] 1.1 Author `cove_steps.py`: treads A/B/C, the landing and the side cheek, to the design's pivots and sizes. Critique the 4K hero and detail renders
- [ ] 1.2 Author `cove_kerb.py`: straight, curve and end pieces
- [ ] 1.3 Author `cove_handrail.py`: level bay, raked bays at 26/28/30 degrees, end and corner posts
- [ ] 1.4 Author `fingerpost.py` with "To the Cove", after checking and recording the font's licence

## 2. Import (Props; Unreal slot)

- [ ] 2.1 Import all pieces with their collision (tread boxes, kerb boxes, fingerpost box, no rail collision) and Nanite on the stone. Check the pivots in the editor against the design

## 3. Placement (Water)

- [ ] 3.1 Commit the route data (generated `.inc`/JSON, like the road bridge in `estate_layout.json`)
- [ ] 3.2 Cut the heightfield to the step and ramp profile and build the flights, kerbs, rails, pawn-only blockers and fingerposts at runtime from the route data
- [ ] 3.3 PIE: walk the route down and back up with the gamepad and keyboard, walk into the rails and kerbs, and check the cove and gate views
