# Tasks

- [x] 1.1 Packets stack by crop in the simulation; native tests cover buy, plant, split/merge, chest, drop and save round trips, and that different crops never merge.
- [ ] 1.2 Jenny buys two packets of one seed: they share a slot and show a count; a packet of another crop takes its own slot.
- [x] 1.3 Loading an older save folds legacy single-packet seed groups into stacks without losing units or leaving dangling hotbar references; deliberately split stacks survive a load (native tests: SeedPacketTests, SplitStackSurvivesLoad).
