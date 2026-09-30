# Tasks

## 1. Recipes (Props; Blender slot)

- [ ] 1.1 Author `cove_steps.py`: treads A/B/C, the 1.2 m landing, the 0.6 m landing slab and the corner-landing wedge, to the design's pivots and sizes (recipe written; build and review pending a Blender slot). Critique the 4K hero and detail renders
- [ ] 1.2 Author `cove_kerb.py`: straight, curve and end pieces (recipe written; build pending)
- [ ] 1.3 Author `cove_handrail.py`: level bay, raked bays at 26/28/30 degrees, end and corner posts, each symmetric across its rail line so it can be mirrored (scale Y = -1) (recipe written; build pending)
- [ ] 1.4 Author `fingerpost.py` with "To the Cove", after checking and recording the font's licence (recipe written with Blender's bundled DejaVu Sans, Bitstream Vera licence; build pending)

## 2. Import (Props; Unreal slot)

- [ ] 2.1 Import all pieces with their collision (tread boxes, kerb boxes, fingerpost box, no rail collision) and Nanite on the stone. Check the pivots in the editor against the design

## 3. Placement (Water)

- [x] 3.1 Commit the route data (`jennifergalley-cove-route` @010c7003: `HomesteadEstateCoveRoute.inc` and `add-cove-route`)
- [ ] 3.2 Cut the heightfield to the step and ramp profile and build the flights, kerbs, rails, pawn-only blockers and fingerposts at runtime from the route data
- [ ] 3.3 PIE: walk the route down and back up with the gamepad and keyboard, walk into the rails and kerbs, and check the cove and gate views
