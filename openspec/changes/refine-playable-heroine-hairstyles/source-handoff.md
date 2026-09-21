# Published source handoff

The selected artifact set is `Assets\Characters\HairstyleRefinement\final-bundle.json`,
not every reference directory: 12 current-selector joined FBXs and 6 matching
modular bases, latest qualified waves plus stock-based bob01 adaptation.
BobReuse is an authoring donor/reference; PublishedWaveParity is an older
alternative. Neither directory overrides the canonical manifest.

| Commit | Purpose |
| --- | --- |
| `617d554` | Separate hairstyle OpenSpec round |
| `013c70b` | Five-color palette and minimal incumbent neutral-material dispatch |
| `a3a6dee` | Historical early wave source baseline |
| `79c3e79` | Qualified updated waves, manifest `cb6e61cd...0cac8` |
| `a37e24d` | Complete selected stock-bob/modular source bundle and hash receipts |
| `3a75556` | Preserve archived reference JSON bytes under Windows autocrlf |
| `9f25ac6` | Modular neutral-material admission and BeginPlay guard; requires wardrobe API `11ba95c` |

The clothing source set remains `1425aff`: three complete adult fits with
separate permanent bra/briefs, nine body/hair bases and twelve independent
tunic/apron/shoes-with-socks/footwrap exports. Its original asset hashes still
pass. The later hairstyle exports map to six of those base object paths without
rewriting the original clothing source files.

Verification: final 18-export hair checker, exact published-wave check,
21-export original modular checker and strict OpenSpec validation pass.
Archived JSON reference files were additionally checked after Windows
autocrlf-enabled index checkout. The native wardrobe selection test's earlier
unguarded compiler incident is documented in `modular-clothing-lane.md`; no
clean compiler-route or UE compile claim is inferred from source checks.

**Open:** wave ends still read chunky with a conspicuous cut edge. Task 1.1 is
not accepted. Stock-based bob is also not human-approved. Main owns actual
Unreal import/build/cook and normal-camera play review (tasks 1.3/2.3), and must
retain the old alternative if the candidate looks worse. No automatic merge,
PR or playable promotion was performed by this source lane.

## Committed-checkout receipt correction

Main reported that the final checker required ignored `.blend1` authoring
backups absent from its committed checkout. The follow-up is limited to receipt
enumeration and dependent metadata: exclude Blender numbered backups and `.bak`
files, retain strict checks for every real source/canonical asset, and verify a
fresh committed-tree checkout without untracked backup files. No geometry,
texture, rig, authoring export, compiler or engine work is authorized by this fix.

Historical local backup observations are retained here, not as import or
validation prerequisites. These ignored files were recorded during authoring;
their prior observations are **not rerun or newly proven** in the committed-tree
check. Paths below are relative to `Assets\Characters\ModularClothing`.

| Historical backup | Recorded SHA-256 |
| --- | --- |
| `Hazel\Hazel_Modular.blend1` | `9bae214cf07817033eea987d3054d30a0ab486894a8a229c6e5c6cce348a2122` |
| `Preferred\Preferred_Modular.blend1` | `a2fd44708341a4f41be3ed113d27be161ef0c90cc50e93ffe04a672a7ca444d9` |
| `Willow\Willow_Modular.blend1` | `cd2014de9e5503821fe8751363648ca2252da1ea2c7c0fd402c9d2dcc7f12fc2` |
