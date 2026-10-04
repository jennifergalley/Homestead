# Seed packets stack by crop

## Why

Jenny (2026-10-04): packets of the same seed or plant type should stack; only packets of different crops must not. This reverses the afternoon rule in individual-crop-seed-packets that every packet is its own group.

## What changes

- Every seed packet item stacks like any other item. Each crop is its own item, so a carrot packet never merges with a turnip packet.
- Buying, chests, drops and planting treat packets as ordinary stacks; planting takes one packet from the stack.
- Saves written by the afternoon build (single-packet groups) load unchanged and can be merged by hand. No save version change.
- Crop-named packet icons and the always-Harvest cue from individual-crop-seed-packets stay.
