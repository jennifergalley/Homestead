# Stone building kit and torches: status (paused 2026-09-27)

Jenny paused this work when Homestead pivoted to the 1850s Cornish estate plan. The stone pieces
may be reused for the ruined manor. Nothing has been imported into Unreal.

All extents are in Unreal local cm. Blender (x, y, z) imports as Unreal (x, -y, z). Every
`report.json` records the pivots, extents and placement notes.

## Finished (full-quality 4K bake, 192-sample 4K beauty renders reviewed)

| Asset | Tris LOD0/1/2 | Pivot and extent | Material slot |
|---|---|---|---|
| `Assets\Props\StoneWall` | 18,395 / 8,276 / 2,942 | Cell centre on the floor. X -150..160, Y 128..160 (faces at 130 and 158), Z -10..260 | `M_StoneWall` |
| `Assets\Props\StoneDoorway` | 8,554 / 3,849 / 1,368 | Same envelope as the wall, with a 130 x 220 opening and a lintel | `M_StoneDoorway` |
| `Assets\Props\StoneFoundation` | 11,743 / 5,284 / 1,877 | Cell centre, top at Z 0. 300 x 300, down to Z -40 | `M_StoneFoundation` |
| `Assets\Props\StoneRoof` | 16,416 / 7,386 / 2,626 | Cell centre Z 0. Slab Z ~258..289 | `M_StoneRoof` |

- **Walls and doorway:**
  - Place at yaw 0/90/180/270 about the cell centre.
  - The +X end has a proud quoin pier that fills the outer corner, and the -X end tucks inside
    the neighbour's pier.
- **Roof:**
  - Place at yaw 0 only.
  - The slate courses tile across cells in Y, and the undercloak strips slide under the
    neighbours in X.
- **Collision:**
  - Box collision for all four pieces.
  - The doorway is the exception: `import_props.py` gives it complex-as-simple collision on LOD1.
- **Roof re-render:** the last change squared the slate corners, nibbled their edges, and darkened
  the timber and slate. The rebuild and re-render finished, but I haven't viewed the new renders yet.

## Half-done

**Torches (`Assets\Props\TorchGround` and `Assets\Props\TorchWall`)** are only built as drafts
(1K bakes, 48-sample renders).

- **Geometry, done:** the stake, the lapped head, the lashing, the drips, the spent head with its
  burnt stake end, and the forged sconce (nailed strap, scroll, twisted arm, two rings).
- **Materials:** `pitch_rag` was rewritten and not yet reviewed.
- **Still to do:** view the draft renders, run a full build (`$env:HOMESTEAD_DRAFT='0'`), then
  critique and fix.
- **Flame sockets** (Unreal cm, from `report.json`):

  | Mesh | Socket (x, y, z) |
  |---|---|
  | `SM_TorchGround` | (0, 0, 112.4) |
  | `SM_TorchGround_Spent` | (0, 0, 105.0) |
  | `SM_TorchWall` | (0, -31.3, 60.0) |
  | `SM_TorchWall_Spent` | (0, -28.9, 53.3) |

- **Material slots:** each torch mesh bakes to one `M_<MeshName>` slot. The wall torch also bakes
  a metallic map.

## Not started

- **2x2 tiling layout render:** check the corners, the roof laps and z-fighting. The plan was a
  `stone_building\layout.py` scene. To match the game in Blender, place each instance at
  rotation -yaw and location (cx, -cy).
- **Per-asset READMEs.**
- **Handing off to the coordinator session for the Unreal import:** run `import_props.main([...])`
  in the editor.

## Known weaknesses

- Wall LOD0 is ~18k tris, above the ~15k target.
- In long straight runs the quoin pier repeats every 3 m.
- The roof is sensitive to rotation (yaw 0 only).
- The roof slates show a straight vertical joint every 300 cm.

## Rebuild commands

```powershell
.\Scripts\Blender\New-Prop.ps1 <recipe> [-NoBeauty] [-BeautySamples N]
```

- `<recipe>` is one of: stone_wall, stone_doorway, stone_foundation, stone_roof, torch_ground,
  torch_wall.
- Set `$env:HOMESTEAD_DRAFT='1'` for quick 1K builds, or `'0'` for full quality.
- Beauty renders need the Kloofendal HDRI cache (`Get-PolyHavenAsset.ps1`).
