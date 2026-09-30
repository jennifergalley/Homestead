# Tasks

## 1. Source port (branch `jennifergalley-pickup-feedback-0930-a7c`)

- [x] 1.1 Port `HomesteadHoldings.h`, `HomesteadControllerPickups.cpp` and `UI/SHomesteadPickups` from
  `origin/jennifergalley-menu-pickups-accept` onto the split controller.
- [x] 1.2 Quiet successful harvest, craft and pack-to-chest transfers. Refusals still toast.
- [x] 1.3 Resync silently when the revision goes backwards (rollbacks, in-place loads).
- [x] 1.4 Native economy tests: gains count only new things, and stack arrangement, eating, garment and lamp
  drops show nothing.

## 2. Unreal checks (not run yet: the garden-outline lane owned UE/UBT/UAT)

- [ ] 2.1 Editor/UBT compile, including a Shipping unity build (includes are explicit in every new file).
- [ ] 2.2 PIE, with keyboard/mouse and with a controller: gather berries, harvest a crop, craft in the book, buy at
  the store. Each shows one `+N` line beside her, and the HUD doesn't move.
- [ ] 2.3 Take the pail from the manor chest, drop and pick up a stack, and load a save. None of them shows a line.
- [ ] 2.4 Check the line at 1080p and 4K, both beside the vitals and above the hotbar.
- [ ] 2.5 Run the native menu, hotbar and feedback automation tests (their toast expectations are unchanged).
