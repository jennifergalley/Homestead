# Proposal

## Why

Jenny wants sprint as a toggle instead of hold-to-sprint: L3 on the gamepad, Shift on the keyboard.

## What Changes

- L3 flips sprint on press. On the keyboard a tap of Shift flips it on release, unless Shift was used as a modifier meanwhile (Shift+Q in the seed pouch, Shift+click); movement keys don't count, so Shift+W still toggles.
- Stopping, working (any action clip), a menu, the shop, planning or falling suspends sprint speed but leaves the toggle on; she runs again when she moves on.
- Menus, the shop, Appearance, planning and a failed run don't flip it. A load, a new game, a retry or a teleport turns it off.
- With too little Energy (the simulation's reserve of 10), a request is refused, or a running sprint turns itself off, with one gentle notice ("Too tired to run. Sprint is off until you've eaten or rested."), and she walks on. Nothing forces a collapse. Walking, the run clip, its blend and the Energy drain are unchanged.
- Hints: the HUD bar says `[Shift] Sprint: off/on` (`[L3]` on a gamepad), the Character Lab says `Sprint toggle Shift / L3` and shows the toggle and whether she's running, and the README controls table lists the toggle.
- The hotbar, creek and visual sprint playtests now tap Shift / L3 to turn sprint on and off.

## Impact

- `HomesteadCharacter.h/.cpp` (the toggle, suspend vs reset, the Energy floor), `HomesteadControllerSprint.cpp` (new: Shift taps and the notice), `HomesteadController.h/.cpp` (reset on load, new game, retry, teleport), `HomesteadHUD.cpp`, `HomesteadLab.cpp`, `README.md`, and the three tests.
- No save or simulation change.
