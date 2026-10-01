# Audio checks

Like the UI gallery, but for sound: one row per effect cue, giving what triggers it, what Jenny should hear, and how
an agent checks it without ears. The check is a log line: the cue's name and the clip phase when it fired, against
the clip's contact time (the "ear proxy"). Add a row whenever a cue is added or changed.

Run: start PIE, do the action, and filter the output log for the tag.

| Cue | Trigger | What she should hear | Check (log) |
| --- | --- | --- | --- |
| `CraftStrikeA`/`B`/`C`, the pickaxe ping | Every landed pickaxe strike on rubble or a rock (`PlayStrikeCue`). Earlier strikes rotate A, B, C at gain 0.12; the breaking strike plays A at 0.15. | A bright stone-on-steel ping on every strike, varied a little in pitch (+/-4%), with no wood chop. The last strike is a touch louder. | `STRIKE_CUE tool=Pickaxe swing=N final=0/1 cue=CraftStrike* phase=… contact=…`, with `phase` at or just past `contact` (`FellStrikeSeconds(0)`). |
| `CaneCutA`/`B`/`C`, the billhook cane cut (`Scripts/generate_billhook_sound.py`) | Every landed billhook swing on brambles or a sapling, rotating A, B, C at gain 0.75 (the chops' gain; mastered to their loudness). | A quick hooked slash, a woody fibrous snap, then leaves rustling as the cane falls. | `STRIKE_CUE tool=Billhook … cue=CaneCut*`, with `phase` at or just past `contact` (`MacheteClearSeconds`, 1.25 s). |
| `ScytheSwish` | Each scythe sweep that mows something. | An airy swish through grass, with no thud. | (no log yet) |
