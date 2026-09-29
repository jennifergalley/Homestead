# Tasks

## 1. Simulation
- [ ] 1.1 `Item::OilLamp` (tool) and `Item::OilFlask` (supply) with catalogue rows and icons
- [ ] 1.2 Lamp oil reservoir, burn while lit (held and selected, or set down), go out at empty
- [ ] 1.3 `RefillLamp`, `SetDownLamp`; pick-up through `PickUpDrop`
- [ ] 1.4 New-game kit and the one-time kit for older saves; `lamp` save section
- [ ] 1.5 Oil flasks on the general store's shelf
- [ ] 1.6 Native tests: items, store listing, burn, refill, set down / pick up, save and load

## 2. Game
- [ ] 2.1 `SM_OilLamp` and `SM_OilFlask` Blender props, glass and flame materials, pack icons
- [ ] 2.2 Held lamp: prop on the bail, flickering light, burns only while selected
- [ ] 2.3 Set-down lamps render with the lamp mesh and light; "Pick up" focus
- [ ] 2.4 Hotbar oil meter; toasts when low and when out
- [ ] 2.5 Refill from the flask's pack menu and the in-hand secondary action
- [ ] 2.6 Hotbar layout version bump: the lamp on older hotbars
- [ ] 2.7 Raised-lamp upper-body pose, set-down/pick-up clip, lab commands

## 3. Verification
- [ ] 3.1 PIE at night: held and walking; set down and the area lit; picked up; refilled; flask bought
- [ ] 3.2 Native tests pass; editor module compiles
