# Proposal

## Why

Jenny wants to eat berries from the hotbar with the gamepad's A or X, not only RT/LMB, and to see what a meal actually did: a clear "+N Energy" beside the energy bar as it fills.

## What Changes

- With food selected on the hotbar and nothing in front of her to interact with, A / E (Interact) and X / F (Secondary) eat one. A chest, drop, plot, weed, fire, bed, door or shopkeeper keeps its own action. The cue says `[A] Eat berries` (the device's glyph). One press eats one: presses while she's still eating are ignored and the bindings fire on press only. With the stack empty, the button says none are left.
- After a meal from the hotbar, the food and energy bars fill from where they stood to the new value, and "+N Food" / "+N Energy" rises and fades above each bar's right end. N is the actual change after caps, so a full bar shows nothing. The popups are drawn over the vitals backing, so nothing moves.
- The keyboard hint bar now says what the wheel does: `Wheel: tool   Ctrl+wheel: zoom`, and sizes itself to its text.
- RT / LMB eating, the seed pouch, the lamp and every focused action are unchanged.

## Capabilities

### New Capabilities

- `hotbar-eat-and-meal-gains`: Eating hotbar food with the interact buttons, and honest meal-gain feedback on the vitals.

### Modified Capabilities

None.

## Impact

- `HomesteadControllerHotbarEat.cpp` (new), `HomesteadController.h/.cpp` (Interact / Secondary fallbacks, the cue, the meal-gain record), `UI/SHomesteadVitals.h/.cpp` (fill and popups), `HomesteadHUD.cpp` (hint text).
- No simulation, save or input-mapping change.
