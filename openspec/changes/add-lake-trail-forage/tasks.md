# Tasks

## 1. Source

- [x] 1.1 `Scripts/Terrain/lake_path_plants.py`: forage rows (582300+) and wildflower records (kinds 42-48)
- [x] 1.2 `ProvisionalEstatePlacements` lake section after the rack; EstateSceneryKinds 42-48
- [x] 1.3 Native: lake-trail forage checks (HomesteadPublicRoadTests); ordering and save-hash tests allow the section
- [x] 1.4 Props (jennifergalley-lake-flowers 74b57c97): SM_WoodAnemoneClump, SM_RedCampionClump, SM_Foxglove, SM_CowParsley (recipes with wind like the
      existing flower clumps)
- [x] 1.5 Registry: placement ids 582300-582399 and scenery kinds 42-48 (round page, via Docs)

## 2. Verification (slot)

- [x] 2.1 PIE walk (2026-10-01): bramble 582301 +5 berries, roots 582302 +4, both not ready after. All 7 flower meshes load; at 1x
      they were specks in the tall grass, so clumps are drawn 1.15-2x by kind and the drifts are denser (866 clumps)
- [x] 2.2 Frame time on the trail with the flowers shown vs hidden: 16.4-17.0 ms either way (no measurable cost);
      captures in E:\CopilotScratch\89914e30-...\trail
