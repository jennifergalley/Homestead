# Proposal

## Why

Jenny: "Change dye" does nothing, and she wants to choose the colour of her garment.

The linen tunic and apron are the dyeable garments, with four dyes (Moss, Wine, Slate, Flax), and the book only stepped to the next dye. On the MetaHuman heroine the linen tunic is presented as the homespun tank top and shorts, which were never tinted, so the saved change had no visible effect.

## What Changes

- "Change dye..." opens a chooser popup over the live book: a swatch and name for each of the four dyes, "Apply <dye>" and "Cancel".
- Moving over a dye shows her wearing it at once. The preview goes through the same wardrobe preparation a real recolour uses, applied to a copy of the simulation, so nothing is saved or spent. Choosing a dye moves the focus to Apply. Apply commits it (`RecolorWearable`); Cancel, B or closing the chooser puts her own colour back.
- On the MetaHuman, the equipped tunic's dye tints the homespun tank top and shorts (`HomesteadLook::HomespunDyeTint` over M_PropTextured's `Tint`). Dye 0 leaves the cloth as woven, so a new game looks unchanged.
- Dyeing is free, as before, and a dye is still stored as 0–3, so saves are unchanged.

## Impact

- `UI/SHomesteadMenu.h/.cpp` (the chooser, swatch chips on popup options, the preview in Tick), `HomesteadControllerDye.cpp` (new: the preview and its restore), `UI/HomesteadMenuInventory.cpp` (the chosen dye is passed through), `HomesteadCharacter.h/.cpp` (the tunic dye on the MetaHuman outfit), `HomesteadAppearance.h/.cpp` (the tint table), `UI/HomesteadNativeMenuTest.cpp` (open, preview, choose, apply).
- The apron has no MetaHuman garment yet, so its dye still shows only on the legacy body.
