# Tasks

## 1. Derelict farm

- [x] 1.1 Pick a flat-ish site near the manor off the road; add `Anchor::DerelictFarm` and `Anchor::DerelictFarmGate`
- [x] 1.2 Author the Blender props: FarmFence (post, snapped post, rail, broken rail, gateway) and FarmField (ridges, dead stalks, bean poles), FarmPlough
- [x] 1.3 `AHomesteadDerelictFarm`: the fence in runs of sound/leaning/snapped/missing posts and rails, the gate, ridges, stalks, poles and plough on the heightfield
- [x] 1.4 Clearable overgrowth in the field (550000+), with native tests

## 2. Estate disrepair

- [x] 2.1 Author EstateDebris (broken barrel, smashed crate, rubbish heap, collapsed lean-to) and FarmCart
- [x] 2.2 Place the debris round the ruin, the toppled drive fence, more ivy and slate (`Scripts/Terrain/estate_disrepair.py`). Barrels, crates and rubbish heaps near the manor became clearable nodes owned by the clearing lane (570000+), reusing the EstateDebris meshes; the lean-to, cart, timbers and slate stay static
- [x] 2.3 More clearable overgrowth round the grounds and the drive's verges, clear of routes
- [x] 2.4 Re-walk the routes in PIE: chest, salvage piles, front door, rear gap to the farm gate, the drive

## 3. Acceptance

- [ ] 3.1 Eye-level PIE captures of the farm, the grounds and the drive (farm gate, ridges, corner, lean-to and grounds captured; the drive was walked but its toppled fence has no dedicated capture yet)
- [ ] 3.2 Jenny playtests
