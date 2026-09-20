# Cozy Survival Game

## Status

The initial implementation plan has been approved. This is the project-owned
design and roadmap, with subsequent interview additions incorporated.
Confirmed choices and proposed implementation details are distinguished below.
The first homestead MVP is packaged and ready for Jenny's playtest. Unreal 5.8.2
is installed at E:\Program Files\UE_5.8. The standalone game passed the complete
automated route at native 4K: three character presets, three hairstyles, two
cosmetic outfits, saved colors, both crops, foraging/crafting/building/cooking,
storage, sleep, and checkpoint recovery. Launch with Play.cmd or
Build\Windows\SurvivalGame.exe.
Jenny liked the first running-scene screenshot; its warm direction is preserved.
Art/animation remain prototype-quality, with known highlight/garment limitations;
physical-controller comfort and listening review await Jenny. This is not the
complete roadmap or the final high-fidelity character creator.
See README.md and docs\setup.md for exact evidence and limits.

Jenny's first playtest found a lot working, but the heroine's appearance and
robotic movement do not meet the brief. Character presentation and locomotion
are the next priority, ahead of adding more gameplay systems.

Additional playtest notes are tracked in docs\playtest-feedback.md. Jenny
explicitly deferred the rendering/prompt and inventory/field-book feedback;
recording it is not authorization to interrupt the current visual-playtest work.

The project folder is E:\Repos\SurvivalGame. It now contains the Unreal project,
source code, licensed source assets, scripts, and documentation; there is no git
repository yet. The technical project targets stable Unreal 5.8. Character-pipeline
and packaged-build feasibility remain open gates; geometric stand-ins are not
approval of the finished art direction.
The performance target is 60 FPS with quality adjustments/upscaling allowed.
The target machine is Jenny's current Windows PC;
inspected specifications are recorded below.

## Documentation ownership

Jenny wants detailed specifications and durable plans in the project repository,
not Notion. This consolidated document, docs\game-plan.md, is the source of truth
for the design, MVP, roadmap, and proposed technical approach. The approved session
draft has been moved here rather than maintained as a competing specification.

As implementation grows, sections may be extracted into focused game-design, MVP,
roadmap, and technical-design documents under docs. Link to extracted sections
rather than keeping divergent copies. Notion holds brief journal entries and
project context only.

## Vision

A beautiful, relatively cozy survival and homesteading game inspired by Minecraft,
Coral Island, Disney Dreamlight Valley, and the pleasure of wandering Skyrim
collecting flowers. Dreamlight Valley is specifically a reference for enjoyable
daily maintenance and tending; Coral Island also informs the foraging loop.
These are experience references, not requests to copy their settings, characters,
magic, or quest structures.
Begin as a young adult woman in spring with almost nothing.
Clear land, gather resources, grow and cook food, craft tools and clothing, build
shelter, and turn it into a home worth spending winter in.

Live independently within reach of a village. Conversation, trade, optional
requests, and eventually romance enrich the world without making villagers'
approval or errands a condition of survival.

Believability should create satisfying preparation, not constant emergency.
Clothing, warmth, food, shelter, seasons, and daylight matter. Hunting supports
subsistence but avoids gore, distressing vocalizations, and graphic processing.
Occasional predators in deeper wilderness add external stakes and a reason to
carry a spear; the homestead and village approaches remain safe.
Animal companions provide friendship at home and a small follower on outings.
Care is gentle and rewarding, never a threat to a beloved pet's safety.
An appealing player character is a core part of the experience, not an optional
polish task.

## Confirmed requirements

- Controller-first play, with mouse and keyboard also supported.
- Free third-person camera, reasonably close, with zoom out for farming/building.
- Beautiful, cozy environments with realistic graphics where practical.
- A beautiful, peaceful, varied landscape: forest, occasional meadows,
  wildflowers, rocks, rivers/streams, lakes, and waterfalls over the roadmap.
- Gathering plant resources is an explicit, recurring gameplay loop, including
  flower picking and casual foraging walks. It is not only a starting-material
  step or a way to obtain the first crop seeds.
- Deep character appearance customization in the larger game.
- Clearing land, farming, gathering, crafting, cooking, building, and decoration.
- Clothing eventually craftable or purchasable, and relevant to surviving cold.
- Day/night cycles and a seasonal progression from spring toward winter.
- Hunting eventually includes crafted bows, rabbits, squirrels, deer, and
  tracking, with restrained presentation and little emphasis on combat.
  Additional small prey can follow where appropriate to the setting.
- Scavenge found carcasses, including abandoned predator kills, for usable bones,
  raw hide, and freshness-dependent meat without having to kill the animal.
  Carcasses should be a substantial, viable source of bone for crafting.
- Scavenging belongs early after the homestead MVP and before active hunting.
  Jenny explicitly chose to keep it outside the first homestead build.
- Predators are occasional territorial encounters in deeper wilderness, not a
  constant threat. The homestead and village approaches stay safe, encounters
  have warnings and a reliable escape route, and a spear has a defensive purpose.
- Befriend animal companions such as squirrels, rabbits, or foxes. Keep several
  companions at home and choose one active follower for outings.
- Companions use gentle care: feeding, petting, and bonding, with no death, injury,
  or permanent loss from neglect. Their role is companionship, not a survival
  obligation or compulsory combat system.
- Optional village involvement, low-stakes requests, later romance.
- Frequent playable versions for Jenny's direct feedback.
- Grounded fictional countryside with preindustrial hand tools, bows, hearths,
  village craftspeople, and no overt magic.
- Modular, snapping structural pieces so Jenny can design and expand the rooms
  herself, rather than whole-building placement or voxel construction.
- MVP character choices: several complete face-and-body presets, skin/eye/hair
  colors, a few hairstyles, and a simple outfit choice. Complete presets are
  explicitly acceptable; independent face/body mixing and detailed sliders follow.
  One preset must closely follow Jenny's preferred visual direction.
- Free assets first. Present specific paid alternatives only when they materially
  improve the result; no purchase is authorized.
- Xbox controller / Xbox-style layout is the primary controller target.
- Survival failure reloads a recent checkpoint; no permanent failure penalty.
- Initial needs are hunger, warmth, and tiredness. Thirst and detailed health
  systems are deferred.
- Default clock: a full 24-hour game day takes 60 real minutes before sleep
  advances time. Make the pace configurable.
- Default full-year calendar: 14 in-game days per season, adjustable. Full seasons
  are beyond the spring/cold-night MVP.
- Pause simulation in menus and while planning/positioning buildings. Normal
  world actions advance game time.
- Start with worn clothing and a knife, not a metal hatchet, seeds, food, house,
  or money. Craft crude tools through the opening progression.
- Obtain the first plantable seeds or roots from wild edible plants, then
  cultivate them; no village dependency in the MVP.
- Farming is hands-on but forgiving: clear, till, plant, water, weed, harvest.
  Jenny explicitly wants weeding alongside watering as an enjoyable daily
  maintenance activity in the homestead MVP.
- Dreamlight Valley's satisfying daily-maintenance rhythm is an explicit design
  reference. Tending should be enjoyable, not just an upkeep cost.
- Rain helps; missed watering slows growth rather than immediately killing crops.
- Jenny accepts a native Unreal-based setup and occasional guided account/editor
  steps if the character prototype works. No installation during planning.
- Offline single-player; multiplayer is not a goal.
- Target 60 FPS on the inspected PC, allowing upscaling and sensible quality
  adjustments. Native 4K rendering is not required.
- Simple MVP weather: cold nights, shelter/fire/clothing warmth, and rain-fed
  crops. Wet clothing, drying, wind chill, and detailed indoor heating follow
  in the first-winter expansion.
- Built-in atmospheric wilderness music, with a Skyrim-like sense of quiet wonder,
  using original or appropriately licensed recordings rather than Skyrim assets.
- Music plays in passages separated by stretches of natural ambience, not a
  continuous score. Jenny explicitly selected this cadence.
- Jenny has a Suno subscription and may generate and supply original tracks for
  the game. This is an optional music source, not a prerequisite for playing.

### Character visual brief

The lead is an adult young woman; a precise age is not selected.
Jenny's preferred initial heroine is thin, petite but curvy, with long slightly
wavy brown hair. The first outfit should be slightly revealing, with Melinoe's
dress in Hades 2 as a reference. Use an original design informed by the desired
silhouette rather than copying that costume.

Long, wavy brown hair is the default and the starting point for the first
character preview. Jenny clarified its desired length as approximately mid-back:
the current wavy hairstyle is too long. Retain the brunette waves and shorten
them only in a separately authorized slice across all three bodies and both
outfits. The 2026-09-20 `hair-length-01` attempt reached its measured length but
bunched the waves into accordion-like ridges; it was rejected and restored.
**The requested mid-back length is still unmet.** No further hair/face attempt
is authorized by that rejected slice, and the blonde-bob redesign remains deferred.
Jenny clarified the alternative as a short, straight blonde
bob, with Melinoe's haircut in Hades II as the visual reference, rather than the
current prototype's "karen hairstyle." This is deferred feedback, not approval
of the current bob or authorization to copy game assets. Inspect the reference
when that work is scheduled before specifying technical cut details; create an
original interpretation. It does not replace the long brown-haired default.
The homestead MVP should offer a small curated hairstyle selection;
target three distinct styles, with the third selected from suitable free assets
and reviewed in the character preview. Brown is the preferred default color, not
the only available color. Keep preview-stage scope distinct from the MVP:
starting with one hairstyle in the preview does not remove selection from the MVP.

Jenny supplied a face/hair reference image, visible and reviewed in the interview:
`C:\Users\Jenny\.copilot\workspaces\70592af5-9ceb-4df5-b0cf-975cd6f7e1da\attachments\cd279e85-7c44-4cbb-b3af-376415dd6818-beed8c40-0104-40d5-bc16-c58522da67f9-clipboard.png`

Visual direction: softly sculpted face, defined brows, full lips, light eyes, and
long chestnut-brown hair with substantial volume and loose waves. Naturalistic
materials/proportions with flattering art direction, not gritty deprivation.
Use this as a direction for the preferred preset, not a promise of an exact
likeness or a restriction that all presets must share the same appearance.
The face/hair reference does not replace the separate body and outfit brief.

Keep the supplied image local; it is not authorization to upload it to any
third-party character-generation service. Before character asset work begins,
preserve a local reference and link it from this document. Do not publish the
supplied image as part of a repository upload.
Judge the resulting character in both a close preview and the actual gameplay
camera, with movement, ordinary daylight, and night/fire lighting.

Proposed clothing progression: preserve the preferred silhouette where possible,
adding practical warm layers rather than making a bulky replacement outfit the
only viable winter look. This is a proposal, not yet a confirmed layering system.

### First playable choice

Jenny selected an attractive, tightly scoped homestead slice:
an appealing female character, controller movement, clearing/plant foraging, basic
shelter, planting, watering/weeding, cooking, and a cold night. Animal companions,
the village, and hunting follow.

This is the first MVP, not a promise to include the entire vision immediately.
Deep customization and a complete first winter are subsequent milestones.

## Proposed MVP: A place to spend the night

The following content budget and mechanics translate the confirmed scope into a
proposed implementation. Specific assets must pass the feasibility gate.

One small, authored area connecting a woodland clearing, a modest wildflower
meadow, a short streamside walk, and rock outcrops, with room for the homestead.
These are pockets within one coherent landscape, not several full biomes.
Give the player a peaceful, repeatable foraging route near home. Larger rivers,
lakes, waterfalls, and broader landscape variety expand the world later.

The playable loop:

1. Arrive in spring; explore comfortably using the controller.
2. Follow a peaceful route, pick flowers and useful wild plants, gather accessible
   materials, and clear a small patch of land.
3. Use the starting knife and gathered materials to craft a crude first tool
   through a clearly taught bootstrap path.
4. Place a shelter, fire, bed, and storage through a controller-friendly workflow.
5. Gather viable seeds/roots from wild edible plants, cultivate a small patch,
   tend it with watering and weeding, eat raw-edible forage, and prepare simple
   meals over a small cookfire.
6. Feel the difference between a cold night outdoors and warmth at the homestead.
7. Sleep, return to the growing homestead, revisit renewing forage patches,
   and continue from a saved game.

### Minimum supporting systems

- A player character with attractive, coherent proportions, movement, and outfit.
  Include the confirmed face/body presets, color controls, hairstyles, and outfit
  choices; asset compatibility must be demonstrated rather than assumed.
- Camera collision, comfortable movement, readable interaction targeting, and
  input prompts appropriate to the active input device.
- Small inventory and recipe set; controller navigation without mouse-only steps.
- Resource collection, persistent land clearing, limited modular construction,
  storage, simple crop growth with watering/weeding, and cooking.
- Explicit flower/plant harvesting with readable readiness, inventory yields,
  meaningful resource uses, and plant-specific renewal on the game clock.
- Day/night lighting, hunger, warmth, and tiredness. No thirst in the MVP.
- Pause during menus/build planning, settings, save/load, and checkpoint recovery.
- A small built-in atmospheric score with ambient-only intervals, natural
  environmental sound, and separate music/ambience/effects volume controls.
- A local playable build that Jenny can launch without working in the editor.

### Proposed small content budget

- One woodland/meadow/streamside map with wildflowers and rocks, not multiple
  large biomes or a complete river/lake/waterfall network.
- Three complete character presets, including the preferred heroine.
- Three distinct hairstyles, including long waves and the straight bob.
- Two starter outfit choices with simple warmth values; no full layering engine.
- One compact modular cabin kit plus bed, storage, and cooking fire.
- A small set of renewable forage/resources, two plantable foods, and a few
  recipes sufficient to sustain the opening loop. The forage set includes
  pickable flowers/flowering herbs, food plants, fiber, and plantable seeds/roots;
  exact species are selected to suit the setting and available assets.

Content counts constrain production, not the data model. If free compatible assets
cannot meet the appearance brief, stop for Jenny's choice rather than silently
reducing customization, changing the aesthetic, or buying a solution.

### Explicit MVP exclusions

- A populated village, romance, and social simulation.
- Befriending, companion-following AI, and pet care.
- Carcass scavenging, hunting, animal-material processing, hostile combat, and
  predator AI.
- Full-year simulation and a complete winter survival economy.
- Full-depth face/body creator, a large wardrobe, or cloth simulation.
- Extensive decorating catalogs, livestock, fishing, mining, or automation.
- Procedural open worlds, multiplayer, or console distribution.
- The full landscape catalog, swimming mechanics, and simulated waterways.
  Attractive water scenery does not require these systems in the MVP.
- Thirst, detailed injuries/disease, nutritional micromanagement, and tool/cloth
  physics simulation.

These exclusions define the recommended initial boundary, not rejected future
features, except multiplayer: offline single-player is the confirmed direction.
Companions, carcass scavenging, small-game hunting, and occasional predators expand post-MVP
milestones; they do not enlarge the initial homestead MVP.

## Feedback-driven roadmap

Advance only when Jenny enjoys the current loop. Reorder later milestones based
on playtesting rather than building every system to completion in isolation.

| Milestone | Playable outcome | Gate before expanding |
| --- | --- | --- |
| Foundation preview | Walk around the intended visual environment as the intended kind of character using a controller | Character appeal, camera comfort, interaction feel, and performance are acceptable |
| Homestead MVP | Complete peaceful flower/plant foraging, shelter, planting, watering/weeding, cooking, cold-night, and save/reload loops | Foraging walks and daily tending are enjoyable; progress persists |
| Character and movement quality | An appealing heroine in actual gameplay lighting, with convincing idle/walk/start/stop/turn behavior and grounded feet | Jenny likes the character standing and moving; functional checks alone cannot pass this gate |
| First animal friend | Befriend one animal type, feed/pet it, choose follow or stay home, and preserve the bond across saves | The companion is delightful, safe, and reliable rather than a navigation or upkeep burden |
| Woodland scavenging | Find abandoned carcasses, assess usable remains, recover substantial bone/raw hide/freshness-dependent meat, and craft simple bone/hide items | Animal resources are obtainable without hunting; freshness is readable, harvesting restrained, and yields persistent |
| First winter | Spring-to-winter progression, seasonal forage, storage/preservation, clothing warmth, weather, and improved shelter | Preparation matters without creating unwanted anxiety or unavoidable failure |
| Landscape expansion | More meadows, richer forest, larger rivers, lakes, waterfalls, and habitat-specific plants | New walks are beautiful, readable, and rewarding without bloating travel or reducing performance |
| Village edge | Small cast, conversation, trade, optional requests, and useful alternate acquisition paths | Ignoring villagers remains a viable and emotionally neutral choice |
| Quiet hunting | Bow crafting, rabbits and squirrels as small prey, deer as larger quarry, aiming assistance, tracking, restrained harvesting | Small targets work comfortably with a controller; hunting is readable and not distressing |
| Wilderness predators | Occasional territorial encounters deeper in the wilds, warning cues, reliable retreat, and a defensive crafted spear | Home/village routes remain safe; danger adds stakes without constant harassment or mandatory combat |
| Personal expression | Deeper appearance editing, expanded clothing, dyes, furnishings, building variety, and more companion species/home comforts | Character edits, clothing, and companion switching work across existing saves |
| Relationships and breadth | Friendship, romance, additional activities and locations chosen from feedback | New features enrich the homestead rather than turning play into obligations |

Some appearance work may move earlier because it is central to the fantasy.
Asset topology, rigging, clothing compatibility, and legal reuse need evaluation
before the foundation preview, even if advanced creator controls arrive later.

## Next iteration: character and movement quality

Jenny described the initial heroine as "vaguely horrifying" and her movement as
robotic. The current MakeHuman-derived assets and procedural animation clips are
a technical foundation, not an acceptable visual-quality baseline to expand
indefinitely. Preserve the working game loop and the warm landscape direction.

Recommended sequence, not authorization for purchases or a new pipeline commitment:

1. Observe and record normal play in the existing build before diagnosing the
   character or selecting another pipeline. Functional smoke tests that teleport
   between tasks do not establish movement or interaction quality. Use real
   walking, turns, stops, camera orbiting, close inspection and gathering, with
   captured frames and motion telemetry; keep these checks in the ongoing workflow.
2. Diagnose and improve the current character presentation first. MetaHuman is
   on hold as an unproven alternative, not the next presumed requirement.
   Determine whether the problems are materials/lighting, pose, rigging, animation
   or the underlying mesh before replacing assets. Prove one attractive adult heroine
   close to the supplied face/hair direction, her petite/curvy proportions, and
   compatible clothing. Inspect skin, eyes, hair, silhouette, and material/lighting
   response in this game's daylight and firelight, not only in a creator preview.
   Favor one successful heroine before expanding the preset catalog.
3. Correct the observed locomotion defects. Improve or replace the procedural
   clips based on the visual playtest, using appropriately licensed artist-authored
   or motion-captured animation only where justified. Prove the rig/retargeting and
   proportions first; then blend idle, walking, starts, stops and turns, align
   movement speed to stride, and add foot placement/slope handling where needed.
   Small breathing/blinking/attention behaviors should reduce the mannequin
   effect without turning idle movement into distracting fidgeting.
4. Give daily interactions appropriate motion: reach/pick, chop, water, weed,
   and use the cookfire, with responsive transitions and synchronized feedback.
   Do not increase animation complexity before ordinary walking looks credible.

Bounded technical progress on 2026-09-20: the separate movement, wild gathering
and `watering-01` candidates now cover relaxed locomotion, picking and a short
watering-can gesture. Watering was observed after an actual fresh-start
gather/craft/refill/till/plant route. These are not Jenny's aesthetic/comfort
approval or completion of every interaction: chopping, weeding, cooking and
refill gestures remain outside these slices. See `docs\visual-playtesting.md`.

The next playable review should be a character-quality slice in the existing
clearing: standing, close inspection, walking slowly, turning, stopping, walking
over a slope, and gathering a plant. Keep the existing build available for
comparison. Preserve simulation, controller mappings and saved-world behavior;
character asset replacement is not a reason to rewrite those systems.

Free-first remains the acquisition policy. If a paid character/clothing/animation
asset materially improves the result, present the specific option and cost before
purchase. MetaHuman's editor authoring tools do not themselves supply a shipped
runtime creator; retain that distinction when choosing a replacement pipeline.
The larger village, companion, hunting and winter milestones wait behind this
quality pass. The user review is about liking the heroine, not just counting
presets or demonstrating that a skeleton animates.

## Landscape and recurring plant foraging

The environment should be worth walking through even without an urgent task.
Jenny specifically enjoyed wandering Skyrim picking flowers and Coral Island's
foraging loop. Plant gathering and daily tending are distinct pleasures:
foraging is discovering and revisiting the wild landscape; gardening is caring
for the patch she has cultivated.

### Environmental direction

- Compose peaceful, varied routes with wooded shade, sunny meadow openings,
  wildflowers, running water, and natural rock formations rather than a uniformly
  dense forest or an empty resource field.
- Build the MVP around one small coherent woodland/meadow/stream area. Add larger
  rivers, lakes, waterfalls, and expanded habitats incrementally; do not make the
  full scenery wishlist a dependency of the first playable build.
- Use coherent lighting, plant groupings, ambient wildlife/water sounds, and
  readable landmarks to support exploration. Beauty and atmosphere are part of
  the playtest, not substitutes for accessible paths or stable frame times.
- Keep ordinary near-home foraging routes within the peaceful part of the map.
  Basic food, fiber, and planting stock must not require entering predator
  territory. Deeper excursions are optional additions, not a daily survival toll.

### Proposed harvesting loop

- Walk, notice a plant, approach, pick it with a simple contextual action, receive
  a useful resource, and return later as the plant renews. Keep the controller
  interaction fluid rather than opening a separate collection menu for each stem.
- Make flowers explicitly pickable, not merely background decoration. Select
  recognizable harvestable plants; the entire foliage layer need not be separate
  interactive objects, but readiness and interaction targets must be readable.
- Give plants appropriate habitats so Jenny can learn where to look: meadow
  flowers, shaded woodland forage, and waterside materials. Begin with a small
  catalog and expand species with seasons and landscape growth.
- Connect the initial catalog to existing systems: raw-edible food for immediate
  eating, food/flowering herbs for simple recipes, fiber for crude crafting, and
  viable seeds/roots for cultivation.
  Later uses can include dyes, floral decoration, or trade. Do not assume every
  flower is edible or add alchemy/medicine systems merely to justify gathering.
- Show a modest harvested-state change and clear collection feedback. Picking
  blooms/leaves should not erase the entire bush or surrounding scenery.
  Distinguish picking renewable parts from intentionally uprooting a plant.
- Use plant-specific regeneration on the shared game clock, not instant respawn
  or a blanket reset of every plant at dawn. Tune yields and availability so
  routine walks remain rewarding without requiring that every patch be visited.
- Store resource identity, harvested state, and renewal timing. Sleep, time
  advancement, and reload must not duplicate yields or reset harvested patches.
  Closing the game does not advance the forage clock.
- Keep renewable wild forage separate from permanent land clearance. A node
  removed to make room for the house cannot regrow through a foundation, while
  intact wild patches can replenish their harvestable parts.
- Gathering is optional pleasure beyond meeting basic needs: no missed-day
  penalty, flower-picking quota, or requirement to strip the entire landscape.

## Music and sound

The game supplies its own soundtrack; Jenny does not need to run external
background music. She may contribute tracks generated with her Suno subscription.
Custom music is welcome but must not delay the first playable if suitable free
licensed tracks can meet the initial direction.

### Score direction and cadence

- Aim for spacious, peaceful wilderness music with quiet wonder: gentle strings,
  woodwinds, restrained melodic movement, and occasional distant wordless choral
  texture. Skyrim is an atmosphere reference, not a request to copy its melodies,
  recordings, or distinctive themes.
- Play a piece, then leave stretches of natural ambience: birds, wind in foliage,
  water, footsteps, and the homestead's fire. Avoid an uninterrupted music bed,
  immediate track repeats, or abrupt restarts during ordinary interactions.
- Begin with a small curated playlist and configurable ambient-only gaps.
  Location, day/night, weather, and seasonal musical palettes can grow later;
  a complex adaptive score is not an MVP dependency.
- Provide separate music, ambience, and effects controls. Music can be muted
  without losing environmental cues or interaction feedback.

### Optional Suno contribution workflow

- Jenny can supply downloaded audio files; no Suno login, API access, live music
  generation, or streaming service is required inside the game.
- Preserve each source recording locally and make game-ready derivatives for
  trimming, volume balancing, encoding, and any loop preparation. Prefer
  lossless source files where available; do not assume every export has that
  format or generate missing originals by transcoding a lossy recording.
- Complete instrumental pieces can play once and fade into an ambience-only
  interval. Seamless looping is useful for some cues, not mandatory for all.
- For pieces intended to repeat, inspect suitable loop regions or use tested
  crossfades. Raw generated exports are not presumed loop-ready; listen for
  clicks, mismatched phrasing, abrupt changes, clipped reverb, and volume jumps.
- Record source, creator/account provenance, generation/download date when
  available, applicable subscription tier/terms, and permitted usage alongside
  other asset-license records. Verify rights for the intended use, especially
  before distribution; a current subscription alone does not establish the
  permissions for every earlier recording.
- Do not upload supplied audio to outside processing services without permission.
  Import approved recordings as packaged local audio assets so play stays offline.

For the first playable, select supplied tracks or suitable free licensed music,
prepare the playback assets, and implement the simple score/ambience schedule.
The initial licensed candidates have been downloaded: Evening Fall (Harp) by
Kevin MacLeod (CC BY 4.0) and Forest Ambience by TinyWorlds (CC0). Import,
in-engine listening, and transition validation still require Unreal.
No Suno tracks have been supplied or generated for this project.

## Carcass scavenging and animal materials

Release this early after the homestead MVP, before active hunting. The goal is
a useful alternative to killing animals, not merely an occasional decorative
loot source. A deer killed by wolves is one example; finding its remains should
not require simulating the entire encounter.

### Proposed first scavenging loop

- Discover an abandoned carcass, inspect its condition and available resources,
  then use a suitable tool to recover selected usable materials.
- Begin with authored or controlled-spawn carcass sites, including implied
  abandoned predator kills. Active wolves, live deer AI, and a simulated food
  chain are not prerequisites. Keep replenishment bounded and persistent so
  reloading is not a source of unlimited new carcasses.
- Separate material eligibility: recent remains can provide usable meat and
  raw hide; older remains may lose those resources while retaining useful bone.
  Use clear game-state labels such as fresh, weathered, and skeletal rather
  than asking the player to infer food usability from a graphic depiction.
- Raw hide is not finished leather. Include a modest hide-processing recipe
  chain when introducing animal-material crafting; detailed tanning simulation
  is not required.
- A deer should yield a generous stack of usable bone supporting several small
  crafting recipes, not a single token. Represent this as stackable material
  units rather than separately collecting every anatomical bone. Exact yields
  depend on species, remaining material, and usable condition.
- Introduce straightforward uses such as bone needles, awls, and other small
  tools or components. Balance these against renewable carcass availability so
  scavenging is a practical primary bone source, not a rare lucky exception.
- Keep extraction discreet: brief interaction/animation, restrained carcass art,
  and an inventory result, without graphic skinning, dismemberment, or distress.
  This does not add an injury, disease, or elaborate butchery system.

### Freshness, persistence, and wildlife integration

- Use the shared in-game clock for carcass age and per-material condition.
  Store stable identity, original age/death time, and remaining material amounts;
  partial harvesting must not refresh the carcass or regenerate taken resources.
- Recovered meat retains relevant age/condition instead of becoming newly fresh
  because it entered the inventory. Spoiled meat is clearly ineligible for food
  recipes; simply cooking it must not silently reset spoiled material to fresh.
  Exact freshness thresholds and later preservation methods are tuning decisions.
- Recheck available quantities, condition, tool requirements, and inventory
  capacity when the harvest commits. Give clear feedback if conditions changed;
  never silently lose yields or duplicate them on cancellation/retry.
- Some abandoned remains should be accessible without an active predator
  encounter. Once predator AI exists, deeper-wilderness kills can carry readable
  territorial risk, but must obey the warning/retreat and safe-home rules.
  Opening a loot interaction must not trigger an unavoidable ambush.
- Later player kills and simulated predator kills feed the same carcass/material
  system rather than creating incompatible loot paths.
- Companions are never eligible prey or carcasses. They cannot be harvested,
  even when their species also exists as huntable wildlife.

## Animal companions

Confirmed: Jenny wants animal friends that follow her, the ability to befriend
several, one active follower, and the others safe at home. Care consists of
feeding, petting, and bonding without death, injury, or permanent loss from
neglect. Companions fit the grounded setting rather than requiring magical powers.

### Proposed first companion release

- Place the first animal-friend release early after the homestead MVP, before
  romance and independent of hunting/combat development.
- Start with one suitable species, chosen with Jenny from a squirrel, rabbit, or
  fox after checking free assets and animation quality. Additional species follow;
  the first release does not need all three.
- Befriend the animal through calm, repeatable interactions and suitable food.
  No capture violence, rare spawn schedule, or real-world daily-login condition.
  Exact bonding progression is tuned during the companion release.
- Offer simple controller-friendly commands: follow, stay at home, and choose
  the active companion. Switching followers returns the previous one home and
  never silently releases or deletes it.
- Feeding and petting provide reactions and positive bonding progress. Missing
  them does not cause hunger damage, injury, abandonment, or a dwindling
  relationship that demands daily maintenance.
- Let the follower keep a comfortable distance, idle nearby while the player
  gardens, and rejoin after ordinary movement. At home, companions have safe
  wandering/resting behavior without requiring a built shelter to stay alive.
- Make companions clearly recognizable, with distinct friendly targeting and
  interaction prompts. Do not rely solely on coat color to distinguish a friend
  from huntable wildlife of the same species.
- Protect companions from all player/environment/predator damage and exclude
  them from hunting lock-on, harvest actions, predator prey selection, and loot.
  Protection is a gameplay rule, not merely an interface convention.
- Companions do not initiate combat, attract predators, or scare off quarry.
  A pet fox must not attack the player's rabbit or squirrel companions.
  Companions cannot be used to tank enemy attacks or block projectiles.
- Use non-blocking collision with the player and critical interactions so pets
  do not trap the character in a doorway or intercept farming/building inputs.
- Recover from blocked paths or separation reliably, with unobtrusive catch-up
  outside the camera view when normal navigation fails. Do not make the player
  search the map for a stuck or lost follower.
- Persist each befriended animal's stable identity, species, appearance, bond,
  home assignment, and follow/home selection. Leaving the game, sleeping, or
  retrying a checkpoint must not create duplicates or convert a saved companion
  back into ordinary huntable wildlife.

### Later expansion and boundaries

Expand to multiple befriended animals, additional species, and optional home
comforts as assets and feedback support them. Keep one active follower.
The roster limit and cosmetic options are later design decisions; do not promise
an unlimited fully simulated animal population.

Breeding, livestock production, animal illness, combat pets, magical familiar
abilities, and automated resource collection are not implied by companionship
and are not currently planned systems.

## Wildlife expansion: hunting and occasional danger

This is roadmap design, not additional MVP implementation.

### Small game and deer

- Include rabbits and squirrels, not only deer. Introduce small-game hunting
  before or alongside larger quarry; add other small species only when they fit
  the environment and justify their art/behavior requirements.
- Give species distinct habitats and readable behavior. Wildlife should feel
  present in the landscape rather than appearing solely as hostile targets.
- Carry the crafted-bow progression through aiming, a shot, retrieval/tracking
  where needed, and a restrained harvest interaction.
- Use optional controller aim assistance and accessible target readability.
  Tiny, erratically moving animals must not turn the hunt into an accuracy test
  that is enjoyable only with a mouse.
- Small prey need not use the same extended tracking loop as deer. Keep any
  wounded-animal sequence brief; no prolonged suffering, graphic processing,
  gore emphasis, or distressing cries.
- Tracks, disturbed vegetation, and other non-graphic cues can support tracking
  without requiring a blood trail.
- Balance meat, usable bone, small hides/fur, and larger hides appropriately
  rather than giving every animal interchangeable yields. Use the shared
  scavenging/carcass system for remains. Hunting complements scavenging, farming,
  foraging, and trade rather than becoming a required daily chore.

### Predator encounter contract

Confirmed: occasional encounters deeper in the wilderness, a safe homestead and
village approaches, warnings, reliable escape, and meaningful defensive equipment.
Proposed implementation:

- Start with one territorial predator species appropriate to the chosen region;
  wolves and bears are possibilities, not a commitment to ship both together.
  Species choice and exact rarity remain later wildlife-design decisions.
- Locate predator territory away from the designated homestead and safe village
  routes. No home raids, camping the front door, or predators spawned into safe
  zones. Placing an arbitrary camp object in deep wilderness does not redefine
  the map's protected homestead boundary.
- Signal risk through environmental signs and an animal's warning behavior before
  escalation. Avoid surprise attacks with no reasonable opportunity to react.
- Use a readable behavior sequence: ordinary activity, awareness/warning,
  defensive escalation if the player presses in, then disengagement.
- Provide a navigable retreat route. Leaving the encounter area and creating
  distance must reliably de-escalate the animal; it does not pursue the player
  across the map or into protected areas.
- A crafted spear is defensive insurance: a readable brace/ward-off or defensive
  strike can create space to retreat. Do not require killing the animal or mastery
  of a complex melee-combo system to continue the game.
- Heeding early warnings should allow unarmed retreat too; the spear adds margin
  for an encounter that has escalated, rather than becoming a toll to explore.
- Use a tunable encounter budget and cooldown so many expeditions are uneventful
  and an animal does not repeatedly re-engage a player who has already withdrawn.
  Do not spawn encounters just to fill a quota.
- Failure uses the same checkpoint retry rules as other survival failures.
  Threat can be meaningful without permanent gear loss or a different penalty.
- Keep the existing restrained presentation: no graphic mauling or gore, and no
  prolonged animal-suffering sequence if defensive contact occurs.

## Technical approach and guardrails

### Target machine and initial tooling inspection

- Windows PC: AMD Ryzen 7 3700X, 8 cores / 16 logical processors.
- NVIDIA GeForce RTX 5080, approximately 16 GB VRAM; 32 GB system RAM.
- Current display resolution: 3840 x 2160. Target 60 FPS; internal rendering
  resolution, upscaling, and quality settings can be adjusted after profiling.
  Display resolution is not a promise of native-4K rendering performance.
- E: has approximately 1,408 GB free; C: approximately 81 GB free at inspection.
  Prefer E: for engine, content, and large build/shader caches where configurable.
- Initial inspection found no game engine and an incomplete old VS Preview.
  Visual Studio Build Tools 2022 17.14.41 and Windows SDK 10.0.26100.0 are now
  installed; the new toolchain is on E:\Tools\VSBuildTools.
- Epic Launcher is installed and signed in. Unreal 5.8.2 installation at
  E:\Program Files\UE_5.8 is complete, verified by Epic metadata and successful
  editor compilation, bootstrap, gameplay launch, and packaging.

### Engine and implementation decisions

Use the preferred Unreal route for a feasibility gate, not an unconditional
promise that its stock character tools satisfy the complete game.

- Select and pin a compatible, non-preview Unreal release and supported compiler/
  Windows SDK after verifying the release-specific requirements.
- Prove a representative rigged character, environment, controller, and packaged
  build before committing to substantial content production.
- Prototype runtime complete-character preset switching, color controls, hairstyles, and
  outfit swaps with animation and save/load. A creator that only works inside the
  development editor does not meet the in-game customization requirement.
- Treat MetaHuman as a candidate for authored presets, not a ready-made shipped
  face/body-slider system. Epic's documented workflow assembles rigged characters
  from editor-authored assets. Deep runtime shape customization needs a separate
  feasibility investigation before committing to a long-term character pipeline.
- If the candidate pipeline cannot support the intended progression to detailed
  runtime customization and compatible clothes, stop and present alternatives
  before producing a large wardrobe. Code abstraction alone does not make
  incompatible character art interchangeable.
- Guided setup may require Jenny to sign into Epic, acquire permitted assets, and
  perform creator/editor steps. Document which steps are manual; do not assume a
  headless tool can accept licenses or complete every asset-authoring operation.
- Use original or appropriately licensed assets; track origin and permitted use.
  Purchases and major downloads require Jenny's approval.
- Keep item, recipe, crop, clothing, and seasonal values data-driven.
- Use stable object identifiers for cleared resources, placed structures, crops,
  inventory, character appearance, and saved progression.
- Keep simulation state separate from presentation and input.
- When companion work begins, separate companion identity/state from wild-animal
  spawning and prey/damage eligibility. Share suitable animation/navigation
  components without making a befriended animal vulnerable to wildlife cleanup,
  seasonal respawns, harvesting, or hostile targeting.
- Route controller and keyboard/mouse through shared gameplay actions.
- Save with a schema version and recoverable writes. Proposed defaults: manual
  save slots, three rotating autosaves, and a separately retained recovery
  checkpoint. Preserve prior saves on write failure and show a clear error.
  Resume/failure screens expose earlier checkpoints rather than trapping the
  player in the latest failing state. Do not introduce cloud-save dependencies.
- Profile frame times in a packaged build on the target PC, aiming for 60 FPS
  (approximately 16.7 ms per frame). Record average and low-percentile performance
  in the character/forest scene and again after the homestead systems are active.
  Do not claim success from editor FPS or an empty map.
- Introduce repository/build conventions after engine choice; do not scaffold
  several competing implementations.
- No gameplay accounts, services, online economy, or AI-generated NPC runtime.
  Epic/asset accounts may be needed during development; the packaged game must
  work offline as a single-player game.

### Proposed project organization

When game implementation begins, create one Unreal project in E:\Repos\SurvivalGame.
Prefer testable C++ gameplay/state code and limited Blueprint asset/presentation
wiring; avoid an opaque all-Blueprint implementation that is difficult to review
and maintain with text-based tools. Do not build a general-purpose game framework.

- SurvivalGame.uproject and Config: pinned engine/plugins, inputs, rendering,
  defaults, and packaging.
- Source\SurvivalGame: player/camera, interactions, inventory/crafting, building,
  plant foraging/renewal, farming, time/vitals, appearance selections, and persistence.
- Content\SurvivalGame: maps, character/animation setup, item/recipe data,
  construction pieces, environments, audio, and controller UI.
- Content\SurvivalGame\Audio\Music: approved imported score assets and playlist/
  playback configuration; keep source-audio provenance with the asset manifest.
- Content\ThirdParty: licensed content kept identifiable where package structure
  permits; do not break references merely to enforce a folder convention.
- Scripts: repeatable editor setup/build helpers where the engine supports them.
- docs: design, MVP, roadmap, technical setup, playtest feedback, and an asset
  source/license manifest. Reference art remains local, not in a public upload.

Use Unreal's input actions for both input methods, a shared simulation clock, and
versioned save records. Prefer engine-native collision, animation, UI, audio, and
serialization rather than introducing extra frameworks. Keep tuning in data
assets so feedback changes do not require redesigning system code.

### Documented engine constraints

Official Epic sources inspected during planning:

- https://dev.epicgames.com/documentation/en-us/metahuman/getting-started-with-metahuman-creator-in-unreal-engine
  MetaHuman Creator requires additional Core Data and a plugin; auto-rigging and
  texture synthesis use cloud services.
- https://dev.epicgames.com/documentation/en-us/metahuman/creating-a-metahuman-character-in-unreal-engine
  Character creation/assembly is an editor workflow. Once rigged, head/body and
  material editing in that workflow requires removing the rig; hair/clothing
  remain editable. This does not establish a ready-made runtime creator.
- https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-visual-studio-development-environment-for-cplusplus-projects-in-unreal-engine
  Compiler compatibility varies by engine release. Current documentation lists
  newer supported toolchains than the registered local VS 17.9 Preview for UE 5.8.
  Verify the chosen release rather than mixing requirements from different versions.

Asset-specific license checks, a free character/clothing shortlist, engine release
selection, and exact disk requirements remain outstanding. Search summaries alone
are not sufficient evidence for license or toolchain decisions.

## Proposed mechanics needed to close the MVP loop

These implementation details still need a scope review, not separate permanent
systems for every future mechanic:

- One small, non-voxel authored map; clearing affects identified resource
  objects rather than general terrain excavation.
- Knife -> gathered stone/wood/fiber -> crude cutting and digging tools.
  Balance the progression around shrubs/saplings and salvageable wood; do not
  imply that a knife or fragile stone tool instantly fells a mature tree.
- A small snapping kit: foundation/floor, wall, opening, door, and roof, plus bed,
  storage, and a cooking fire. Exact content counts follow asset feasibility.
- A believable initial ignition recipe using available materials, with clear
  feedback rather than a tedious fire-starting minigame.
- A small crop/forage set that supports a sustainable initial food loop without
  requiring a trader, a hunting system, or unavailable seeds.
- Make eating foraged food explicit in the MVP: clearly labeled raw-edible
  items, such as an in-game edible berry, can be consumed directly from inventory
  to restore hunger. The player must be able to eat before building a cookfire
  or growing a crop. Not all forage items are food or suitable to eat raw.
- Start cooking with a small fueled cookfire and one or two simple forage-based
  recipes, such as roasted roots. Initial recipes require no purchased ingredients,
  metal cookware, kitchen building, or hunting; include any simple stick/tool
  preparation in the existing knife-and-gathered-material progression.
- Cooking gives a useful food benefit over the relevant raw ingredient without
  making cooked meals the only viable opening food. Consume ingredients/fuel and
  award food exactly once, with clear requirements and controller-friendly use.
  Advanced kitchens, recipe trees, and nutritional micromanagement stay deferred.
- Flower and plant picking is a repeatable action with useful outputs and
  persistent harvested/renewal state, not just a generic one-time pickup.
- Resource renewal and wild food provide a recovery route while crops grow.
  The opening must not require surviving on plants that have not matured yet.
- Add a craftable watering vessel and a usable nearby water source to complete
  the farming loop; this is not a player-thirst system.
- Add gradual, visible weed growth to cultivated plots. Distinguish weeds clearly
  from planted crops and use a simple contextual controller/keyboard action to
  remove them, with a short readable animation and satisfying visual/audio
  feedback. No precision timing or mandatory minigame.
- Proposed forgiving weed effect: unattended weeds slow crop growth rather than
  kill or erase the crop. Weeding restores the normal growth rate; it does not
  instantly award accumulated missed growth or destroy harvest-ready produce.
- Keep weed growth sparse and tunable rather than filling every plot each dawn.
  Clearing weeds should leave a visibly tidy plot, without immediate regrowth.
  The routine must leave room for building, exploring, or ignoring the garden.
- Keep one-time land clearance separate from recurring crop tending. Do not
  aggressively repopulate cleared building sites with obstacles as a maintenance
  mechanic. Optional additional tidying activities can follow playtest feedback;
  they are not extra MVP systems.
- Store crop moisture, weed state, and relevant growth/regrowth timing with each
  plot. Growth, rain, watering, and weeds use the shared simulation clock and
  behave consistently through sleep and reload.
- Daily maintenance refers to in-game days. Nothing grows or deteriorates while
  the game is closed; no real-world daily-login requirement or penalty.
- Cooking and planting are world actions, not free consequences of pausing a menu.
  Placement previews pause; any construction time is charged explicitly on commit
  if construction duration is included.
- Warmth responds clearly to ambient conditions, clothing, fire proximity, and
  enclosed shelter. Simple rain waters crops; wet-clothing simulation, wind chill,
  and detailed structural heat modeling are explicitly deferred.
- Shelter protection must depend on a valid enclosed/roofed structure in the
  supported snapping kit, not merely proximity to one wall or a placed foundation.
  Implement a bounded enclosure check rather than full thermal simulation.
- Persistent hunger/tiredness/warmth use one simulation clock. Sleep advances crops,
  moisture, weeds, needs, fuel, and weather consistently; decide safe wake-up
  behavior before adding sleep skips that can jump past danger warnings.
- Checkpoints retain earlier recoverable states. Never replace the only good save
  with an unrecoverable starvation/cold state. Offer manual save/load as well as
  explicit failure retry; define rotating autosave slots during implementation.

## Validation and acceptance

The MVP is not complete merely because its project opens.

- Launch a packaged build on Jenny's target machine.
- Complete the full first-playable loop with controller alone, including menus,
  placement, inventory, cooking, saving, and resuming.
- Exercise the same core actions with mouse and keyboard.
- Reload and verify actual inventory, cleared resources, placed objects, crop
  state, and game clock against the state saved.
- Verify resource consumption, recipe output, stacking/storage boundaries,
  valid placement, crop progression, and warmth effects with focused tests.
- Complete the near-home flower/plant foraging route using both input methods.
  Confirm readable harvest targets, appropriate yields, and a use in an existing
  eating/recipe/crafting/planting system for each MVP forage resource.
- From the knife-only start, gather and eat raw-edible forage before making a
  fire; verify inventory consumption and hunger restoration. Then craft/light
  the small cookfire and prepare the initial recipes using only available
  materials, without hunting, trading, grown crops, or unavailable cookware.
- Verify cooking ingredient/fuel consumption and food output across cancellation,
  insufficient resources, and save/load; do not duplicate food or consume items
  when the recipe cannot complete.
- Verify per-plant renewal across sleep and reload, no duplicate yields, no
  real-world/offline regeneration, and no regrowth inside permanently cleared
  building sites. Harvesting must not leave the landscape visually stripped bare.
- Verify watering and weeding with both input methods, distinguish crop/weed
  targets, and ensure a weeding action cannot accidentally remove the crop.
- Verify weeds slow growth under the proposed rule, removal restores the normal
  rate, skipped tending does not kill crops, and sleeping/reloading preserves
  weed/moisture state without immediate repeated regrowth or growth exploits.
- Confirm cleared homestead construction areas remain cleared and closing the
  game does not advance maintenance needs.
- Confirm that insufficient materials and invalid placement explain the problem
  without consuming resources or trapping focus.
- Check camera occlusion, sloping ground, night readability, and visual comfort.
- Benchmark the representative scene against the agreed frame-time target.
- Have Jenny evaluate character appeal and daily-loop enjoyment explicitly;
  technical success cannot substitute for either.
- Have Jenny evaluate the feel and frequency of watering/weeding directly,
  using her enjoyment of Dreamlight Valley's daily tending as a reference.
- Have Jenny evaluate the scenery and flower-picking/foraging loop directly,
  using Skyrim's casual flower collecting and Coral Island's foraging as
  experience references, rather than judging only resource acquisition speed.
- Check that pause really freezes simulation, both input methods can resume, and
  sleep/time skips do not desynchronize needs, fuel, crops, and the day clock.
- Verify failure/retry restores a recoverable checkpoint without permanent
  inventory loss or duplicating resources.
- Verify 60-real-minute day scaling and 14-day season configuration mathematically;
  use debug clock advancement to test boundaries rather than waiting through them.
- Verify long hair and bob selection survive save/load, do not expose missing
  scalp sections, and behave acceptably with movement, camera zoom, and outfits.
- Launch the packaged game with networking unavailable and complete core play.
- Verify built-in music plays offline, deliberately leaves ambient-only gaps,
  respects separate volume controls, and does not restart whenever a menu opens.
- Listen through track endings, transitions, and every prepared loop boundary.
  Check for clicks, clipped tails, sharp loudness changes, and accidental
  overlapping playback after scene reloads or checkpoint retries.
- Record and review the source/usage rights of every shipped recording. Custom
  Suno files are optional; a supplied soundtrack is not a player setup requirement.

### Companion milestone acceptance (later, not MVP)

- Befriend, feed, pet, send home, and select a follower using the controller
  alone; exercise the same actions with mouse/keyboard.
- Switch followers with more than one befriended animal and verify exactly one
  follows while the others remain safe at home.
- Neglect feeding/petting over multiple in-game days and verify no injury, death,
  abandonment, or permanent loss. Closing the game does not create care debt.
- Attempt attacks, harvest targeting, and predator selection against companions:
  friends remain unharmed, unharvestable, and excluded from hostile prey logic.
  Ordinary wild animals remain huntable when the hunting milestone is present.
- Verify a fox companion cannot attack other companions or autonomously start
  combat with wildlife.
- Test doors, narrow paths, terrain, player building, gardening, and camera zoom;
  the companion must not obstruct movement, interactions, or aiming.
- Verify safe navigation recovery and recall/home switching after separation.
- Save/load, sleep, and retry checkpoints with companions in each supported
  state; check identities, bonds, active-follower uniqueness, and home assignments.
- Have Jenny judge whether following and affection feel enjoyable before adding
  extra care systems or species.

### Scavenging milestone acceptance (later, not MVP)

- Complete a found-carcass harvest and craft a useful bone item without killing
  an animal or requiring live predator AI, with controller and mouse/keyboard.
- Test fresh, weathered, and skeletal remains against explicit material-yield
  rules. A suitable deer carcass must support multiple small bone recipes.
- Verify raw hide requires the intended processing step before a leather recipe
  accepts it, and unusable meat cannot become food through normal cooking.
- Verify carcass age, partial yields, meat condition, and replenishment state
  through sleep, save/load, streamed-area reload, and checkpoint retry.
- Exercise a full inventory, interrupted harvesting, and freshness boundaries;
  materials must not be duplicated, silently discarded, or reset to fresh.
- Confirm safe abandoned remains are available without obligatory hunting or
  predator encounters, and companion entities cannot enter the carcass system.
- Review the presentation with Jenny: useful and matter-of-fact, not gruesome.

### Wildlife milestone acceptance (later, not MVP)

- Hunt rabbits, squirrels, and deer using the controller; test assistance enabled
  and disabled, and readable non-blood tracking cues.
- Check save/load of harvested animals, resources, and relevant wildlife state
  so reloading cannot duplicate harvests.
- Verify predators cannot spawn, initiate attacks, or pursue targets into the
  designated homestead and village-approach safe zones.
- Verify warning cues occur before an avoidable escalation and can be understood
  without relying on sound alone.
- Exercise navigable escape routes, territory boundaries, and disengagement with
  and without a spear. Leaving territory must not create endless reacquisition.
- Verify encounter cooldowns and safe-zone rules after sleep, time advancement,
  and save/load; profile the actual wildlife population, not just one test animal.
- Have Jenny evaluate whether occasional danger adds interest without making
  routine resource gathering feel unsafe. Adjust frequency before adding species.

## Remaining decisions and approval boundaries

Core design decisions are sufficient to begin a bounded foundation preview after
approval. Remaining technical facts are explicit first-step investigations:

1. Select a stable supported engine/toolchain version and verify actual setup
   requirements. Guide account/installer actions; no unattended purchases.
2. Identify free licensed character, hair, outfit, animation, environment, and
   structure sources. Record permitted usage and account requirements.
3. Demonstrate appearance changes and establish a credible future deep-creator
   route before producing a wardrobe. Escalate asset or pipeline incompatibility.
4. Preserve a foundation/character preview for Jenny to judge. During the
   authorized unattended implementation period, make reversible character/art
   choices and continue technical work rather than blocking on each preview.
   Jenny will review afterward; a technical stand-in is still not approval of
   the finished heroine or art direction.

Later content interviews, not blockers for a foundation preview:
village proximity and personalities, trade balance, romance preferences,
deep face/body customization controls, clothing layering, hunting presentation
and tracking, predator species/rarity and defensive-spear feel, carcass freshness/
material yields and hide-processing recipes, and winter-specific balance.
Companion species, acquisition details, and eventual roster size are
also later decisions. Gentle pet care, one follower, predator placement, and the
safe-home contract are already decided.

Plan approval authorizes the proposed build sequence; it does not declare all
future systems designed or all candidate assets suitable. Keep delivering small
playable increments. Do not implement the full roadmap in one pass.

Jenny has authorized several hours of unattended implementation, including free
tools/assets and reversible visual decisions. Do not make purchases, bypass
account/license prompts, weaken security settings, or present provisional art as
finished. Continue until a meaningful playable increment is verified or a genuine
human-only blocker is reached; do not spend the unattended period repeatedly
checking an unchanged blocker.

## Implementation todos

The initial interview, implementation, and automated packaged-MVP verification
are complete. Jenny's initial feedback prioritizes replacing/improving the heroine
and her movement. Plan the character-quality slice before automatic expansion
into the later roadmap, and keep cosmetic/art limitations explicit.
Dependencies are tracked in the session SQL database.

1. Capture confirmed design decisions and the proposed MVP.
2. Maintain this durable design/specification document in project docs.
3. Select engine/toolchain and compatible free character/environment assets;
   perform setup and the bounded character-pipeline feasibility investigation.
4. Create the project, input/camera foundation, representative scene, and packaged
   foundation preview.
5. Establish persistent world state, save/load, inventory, and item definitions.
6. Add recurring flower/plant foraging, resource renewal, permanent clearing,
   and the starting tool/recipe progression.
7. Add basic shelter placement, storage, fire, and sleep.
8. Add crop growth, watering, weeding, food preparation, and the day/night warmth
   loop; tune daily tending for satisfaction rather than obligation.
9. Integrate controller-complete UI and agreed appearance options; tune feedback,
   accessibility, and visuals throughout rather than postponing them wholesale.
   Integrate the initial built-in score, ambient sound, and audio settings using
   approved supplied recordings or suitable free licensed tracks.
10. Validate the complete MVP, package it, and capture Jenny's playtest feedback.
