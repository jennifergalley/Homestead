# Tasks

## Playtest queue
- [ ] 1. Walk under woodland canopy and confirm foliage shadows do not distract or hitch.

## Fix
- [x] 2. Hold tree, foliage and resource shadows at the rest pose (virtual shadow maps cache them Rigid; ray-traced shadows skip their WPO) while the leaves keep swaying. `homestead.FoliageShadowSway 1` restores the old swaying shadows for A/B. Compiled and native tests pass; Jenny confirms task 1 in the playtest.
