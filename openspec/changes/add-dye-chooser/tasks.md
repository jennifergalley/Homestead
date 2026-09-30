# Tasks

- [x] 1.1 The chooser popup (swatches, Apply, Cancel), with its preview on a copy of the simulation and the restore
- [x] 1.2 The MetaHuman outfit tinted by the equipped tunic's dye
- [x] 1.3 The native menu test walks open, preview, choose, apply
- [x] 1.4 Editor build
- [x] 1.5 PIE (1080p editor viewport): each swatch tints her tank top and shorts in the portrait and in-world front and back; Cancel restores the exact tint at no cost; Apply persists through closing the book, F5 and a fresh load, and through unequip/re-equip. Needed a dyeable parent material: M_PropTextured had no Tint, so the preview did nothing until M_HomespunDyeable (import_primitive_outfit.py) went in. Placeholder tints kept.
- [ ] 1.6 4K standalone check (not run: machine slot limits)
- [x] 1.7 Ported onto the split controller/menu/character files (branch jennifergalley-menu-appearance-0930)
- [ ] 1.8 Editor build, NativeMenu 720/4K and a PIE swatch check on the port (awaiting an Unreal slot)
