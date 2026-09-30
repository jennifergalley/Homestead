# Tasks

## 1. Simulation
- [x] 1.1 `Item::OilLamp` (tool) and `Item::OilFlask` (supply) with catalogue rows and icons
- [x] 1.2 Lamp oil reservoir, burn while lit (held and selected, or set down), go out at empty
- [x] 1.3 `RefillLamp`, `SetDownLamp`; pick-up through `PickUpDrop`
- [x] 1.4 New-game kit and the one-time kit for older saves; `lamp` save section
- [x] 1.5 Oil flasks on the general store's shelf
- [x] 1.6 Native tests: items, store listing, burn, refill, set down / pick up, save and load

## 2. Game
- [x] 2.1 `SM_OilLamp` and `SM_OilFlask` Blender props, glass and flame materials, pack icons
- [x] 2.2 Held lamp: prop on the bail, flickering light, burns only while selected
- [x] 2.3 Set-down lamps render with the lamp mesh and light; "Pick up" focus
- [x] 2.4 Hotbar oil meter; toasts when low and when out
- [x] 2.5 Refill from the flask's pack menu and the in-hand secondary action
- [x] 2.6 Hotbar layout version bump: the lamp on older hotbars
- [x] 2.7 Raised-lamp upper-body pose, set-down/pick-up clip, lab commands

## 3. Verification
- [x] 3.1 PIE at night: held and walking; set down and the area lit; picked up; refilled; flask bought
- [x] 3.2 Native tests pass; editor module compiles

## 4. Reach (Jenny's playtest, 2026-09-29: about four times the visible reach)
- [x] 4.1 `Simulation/HomesteadLampLight`: the engine's two point-light attenuation models and the tuned profile. It's 4 unitless (about 4 lux at the flame), 20 m radius, falloff exponent 5, not inverse-square: moonlit-ground level (0.2 lux) at 13.4 m against the first lamp's 3.3 m, and 3-5 times as far across 0.1-0.3 lux. Round her hand it's 4 lux against the old 9 at 0.5 m and 2.2 at 1 m, so auto exposure has no blaze to darken the night for. Inverse-square at 16 times the intensity would reach as far but put about 140 lux on her dress. The placed lamp casts shadows only within 25 m of the camera. Native: `HomesteadLampTests` "the lamp reaches about four times as far".
- [x] 4.2 `HomesteadLampLook` applies the profile to the held, placed and lab lamps. Console variables `homestead.LampIntensity`, `LampRadius` (m), `LampFalloff`, `LampLegacy` (1: the first lamp, for A/B) and `PlacedLampShadowDistance` (m) tune it at runtime. (Unreal code uncompiled.)
- [ ] 4.3 Editor slot, ray tracing on, at 22:00: the held lamp walking through the woods and the meadow, and a lamp set down, each A/B against `homestead.LampLegacy 1` (captures). Check how far the lit ground reads under auto exposure and the frame cost (`stat gpu`: lights and shadows). Tune the console variables and bake the values into `OilLampLight`. _Partly done 2026-09-30 in the Water slot (agent editor, ray tracing off): the held light carries the profile (I 3.79 with flicker, R 2000 cm, exponent 5, not inverse-square). At 22:00 in the meadow the lit ground reaches well out into the grass: lower-half luma 57 against the old lamp's 22 (`homestead.LampLegacy 1`), with no blow-out on her. The placed lamp casts shadows near the camera, not from 40 m, and again on return. Still to do: the ray-traced check and `stat gpu`, then tune if needed._
