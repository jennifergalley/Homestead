# Add an oil lamp

## Why

Jenny: "We're going to need an initial source of light. Please start me with an oil lamp and some
oil flasks for it (purchasable at the general store), and add an animation for me carrying it
around. Enable me to set it on the ground so it can cast light in an area, then pick it up again.
If I select it on my hotbar, it should be actively shedding light and I should be holding it up
like Mr. Filch searching the library."

Nights on the estate are dark and she has no light of her own. A lamp is the first light she can
carry, and oil is a small, steady reason to visit the general store.

## What Changes

- Two items: the **Oil lamp** (a hotbar tool) and the **Oil flask** (a supply that refills it).
- A new game starts with the lamp, full, pinned to the hotbar, and three flasks. Saves from before
  the lamp get the same kit once, when they're next loaded.
- The general store sells oil flasks.
- The lamp holds a few hours of oil. It burns only while lit: in her hand while it's the selected
  hotbar tool, or on the ground where she set it. A flask fills it. Gentle toasts say when it's
  burning low and when it's out.
- Holding it: she raises it up and out ahead of her at head height, peering past it, over
  walking and idling. A warm, flickering, shadow-casting light lights the ground around her at
  night.
- Setting it down: with the lamp in hand she kneels and sets it on the ground ahead of her, where
  it keeps burning and lighting the area. Walking up to it offers "Pick up".
- A set-down lamp is saved (it's a world drop) and keeps burning oil while it sits there.
- Blender props for the lamp and the flask, inventory icons, and lab commands for the new poses.

## Impact

- Simulation: `Item::OilLamp`, `Item::OilFlask`; lamp oil and the one-time kit flag in `State`,
  saved as an optional tagged trailing section (`lamp`). No `SimulationSaveVersion` change.
  Appending the Items waits on the save hardening (count-prefixed stocks) landing on main.
- Game: held prop and light, placed-lamp actors, hotbar oil meter, set-down action and animation.
- Store: flasks on the general store's shelf.
