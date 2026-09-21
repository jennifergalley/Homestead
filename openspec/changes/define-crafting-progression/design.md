# Design

## Context

See proposal.md for motivation. Baseline `c933270` has fourteen Item values,
seven ResourceKind values, five Recipe values and seven Piece values, plus four
wearables. Knife is the sole starter inventory item. Three garments consume
Fiber directly; there is no current Cloth, Rope, Clay, ore, durability,
crafting skill, spoilage or research currency system.

Simulation transactions are instantaneous and have no direct energy cost.
Controller placement separately advances 0.1 game hour and its recipe
activation advances 0.05 game hour. Record these separately rather than treating
zero-time authority calls as the entire native interaction. At default speed one game
hour is 150 real seconds; awake hunger/energy drain 2/2.4 points per game hour.
Menus and planning pause; sleep advances the same shared clock.

## Goals / Non-Goals

**Goals:** machine-readable handoff and small deterministic arithmetic checks;
exact source snapshot; enough production-specific detail to implement the next
small content increment without an invented replacement framework.

**Non-Goals:** runtime loading of this JSON, engine builds, save changes,
geographic seed/version ownership, blanket economics rebalancing, a general
crafting language, botanical realism claims, or a large simulation test harness.

## Decisions

### Separate current snapshot from design additions

Use `docs/crafting-progression/catalog.json` with schema version, baseline,
entity records, actions/recipes, source records and budget cases. Namespaced
design keys distinguish items, structures, garments and capabilities; current
enum names/IDs are recorded separately. Planned additions use null runtime IDs.
Every action has input/output quantities and retained prerequisites. Use
explicit named field units rather than inferring seconds, hours or kilograms.
The README carries tables, dependency graph, interpretation and roadmap.

Alternative rejected: edit Simulation constants from design estimates. That
would trespass on the authority lane and silently turn a proposal into gameplay.
An engine DataTable or plugin adds no value until runtime integration is agreed.

### Bound the opening around existing transactions

Opening: knife -> selected Branches/Stones/Reeds -> Hatchet -> chosen-site
clearance -> foundations/walls/doorway/roofs, bed/chest and fueled outdoor fire
-> cooked roots -> digging stick/watering can -> root and berry plots.
Nothing requires a purchased tool or rare drop. Available materials reveal
meaningful next recipes in a future UI step; current runtime has no discovery
gates. Cordage processing is a later explicit addition, not retroactively
inserted into current Hatchet costs.

Use two adjacent roofed foundation cells with five perimeter walls and one
doorway. Bed and chest occupy different cells; fire is outside. Current shelter
requires enclosure, not just one roof. Cumulative construction plus all three
tools and three fuel branches: 60 Branch, 12 Stone, 22 Fiber. This is 12 hours
of fire, not continuous daily heating. Seven current ready Sapling clears at
8B/2F, two 5B branch sites, three 4S stone sites and two 5F reed sites yield
66B/12S/24F. This is a source-baseline surrogate, not a claim that mature-tree
clearance is already implemented or balanced. Three root,
two berry and one herb gathering supply food and planting stock. This is a
representative assumption, not a minimum hardcoded into every chunk.

### Meaningful tiers rather than a long item list

Current: crude tools, simple woven/branch shelter, pot-free cooking, storage,
garden, cosmetic fiber garments.
Next: distinct construction Timber, explicit Cordage and split fuel from
finite tree processing; reuse existing placement instead of a new tech tree.
Follow with a workbench, better joinery, a covered drying rack and preservation
recipes, then actual insulating cloth/padded clothing. New IDs and costs remain
proposed. Later: clay vessels/hearth, linen processing and loom, cellar,
scavenged hide/bone before optional hunting, then locally self-sufficient
metalworking and decorative furniture. No mandatory village bottleneck.
Document concrete recipes/benefits for these tiers, but only bound and validate
the first deliverable's full resource budget; do not imply future systems exist.

### Preserve geographic meaning while tuning supply

Generator supplies stable source identity/habitat, count, traversable placement
and reachability. Authority supplies yield and cooldown/depletion transaction.
Keep scarce loose Branch sites visually small. Mature tree outputs are finite;
8 Branch + 2 Fiber is an interim bridge, not a timber conversion ratio.
Later source roles distinguish small deadfall, usable stems/timber, plant
fiber, construction stone/tool stone and forage; no need to fill every role
with a distinct inventory item in the first slice.

### Validate only what this delivery can establish

Use a standalone PowerShell script following existing Scripts tooling, JSON
parsing and built-in assertions, with no package installation. Check unique
references, valid status/ID mappings, positive amounts, reachable tools/stations,
fixed-point bootstrap (renewable farming loops can be seeded), cumulative
inputs/outputs and sequential pack/stock bounds. Keep malformed/cycle/capacity/
undersupply negative examples small. Reference a pinned source baseline;
source-drift checks must explain mismatches rather than bless changed runtime.

## Risks / Trade-offs

- Prototype Branch represents too many uses -> retain current meaning, propose
  Timber separately, and defer actual replacement until owner-approved change.
- Supply passes but routes feel tedious -> explicitly require ordinary walking/
  gathering playtests; arithmetic is not visual or experiential acceptance.
- Existing stone/branch cooldowns imply implausible renewal -> report them as
  current, propose finite extraction/storm replenishment separately.
- Food does not spoil and seasons do not yet govern plant availability ->
  provisional reserve arithmetic must not pretend preservation is implemented.
- Standing foliage must be removable on a selected site -> generation and
  authority lanes own actual clearance, chunk persistence and reliable capacity
  rejection. This lane only specifies the budget contract.

## Migration Plan

Add isolated documents/data/checker only. Main can cherry-pick the lane after
review; rollback removes only these new files. No saves, numeric IDs, installed
tools, normal preview, build caches or runtime behavior are changed.

## Open Questions

- After ordinary play, choose processing interaction length and whether heavy
  timber stays in on-site stacks before introducing any weight system.
- Decide whether the first insulation material is woven plant padding alone
  or includes optional scavenged hides; neither gates the opening.
- Balance seasonal yields/preservation after the first-winter weather rules,
  not by silently altering current hunger or forcing daily social tasks.
