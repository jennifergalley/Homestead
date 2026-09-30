# Tasks

- [x] 1.1 Trace: brambles, toyon hedge, deer brush and thimbleberry (`EstateSceneryKinds` 23-27, and pickable bramble visuals) sway via `M_PropFoliage` WPO and cast shadows (VSM with Rigid invalidation; ray-traced when RT is on); the camera-safe dither comes from `MPC_CameraSafeFoliage`.
- [x] 1.2 `homestead.ShrubWind` (default 0.4), `homestead.ShrubShadows`, `homestead.FoliageDither`; shared MIDs; applied on change (source, uncompiled).
- [ ] 2.1 Editor and Game build.
- [ ] 2.2 Integration A/B, packaged ray-traced 4K, fixed camera beside a bramble hedge at noon, 10 s each. Runs: `ShrubWind 1` (before), `ShrubWind 0.4` (default), `ShrubWind 0`, `ShrubShadows 0`, `FoliageDither 0`. Compare per-pixel temporal flicker in the shadow band and frame cost. Keep natural sway and tree shadows, then set the defaults from the result.
