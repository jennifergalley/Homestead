# Spec Delta

## Purpose

Makes recipe readiness and shortages understandable before activation while preserving authoritative crafting validation and current recipe balance.

## ADDED Requirements

### Requirement: Recipe tiles are icon-only and communicate current availability
Each Craft grid tile SHALL show only the recipe icon. Recipe name, output, description, and requirements SHALL appear in the details pane. An unavailable recipe icon SHALL use a clearly greyed treatment but MUST remain selectable for inspection.

#### Scenario: Open Craft without enough ingredients
- **WHEN** one or more recipes cannot be crafted from the carried pack
- **THEN** each unavailable recipe is visibly greyed
- **AND** selecting it still opens its details

#### Scenario: A recipe is ready
- **WHEN** all consumed ingredients, retained requirements, nearby station requirements, and resulting pack capacity permit a recipe
- **THEN** its icon uses the normal available treatment and holding that icon can begin progress

#### Scenario: Scan the recipe grid
- **WHEN** the Craft page is open
- **THEN** no recipe name, `Needs:` sentence, or other prose is rendered inside grid tiles
- **AND** the selected tile's complete identity appears in details

### Requirement: Details use a structured requirement list
The selected recipe details SHALL present requirements as a vertical semantic list, not a paragraph delimited with colons, semicolons, or inline `+` text. Each consumed ingredient SHALL have its own row with carried `Have`, required `Need`, and met/short state. An unmet ingredient row SHALL also show one concise current acquisition hint backed by authoritative resource/recipe behavior. Retained tools and nearby station requirements SHALL be separate rows identified as retained or nearby rather than consumed.

#### Scenario: Inspect a partially supplied recipe
- **WHEN** the player selects a recipe with some but not all required ingredients
- **THEN** every ingredient row shows its current carried amount and required amount
- **AND** only insufficient rows use shortage treatment
- **AND** each insufficient row names a current primary way to obtain that item

#### Scenario: Find Fiber for the first hatchet
- **WHEN** the crude hatchet is selected without enough Fiber
- **THEN** the Fiber row identifies reeds near water as the bootstrap source
- **AND** it does not imply that the hatchet-gated sapling source is required first

#### Scenario: Inspect cooking requirements
- **WHEN** the player selects a cooking recipe away from a fueled cookfire
- **THEN** ingredient rows still show their carried Have/Need amounts
- **AND** a separate nearby cookfire row is marked unmet

#### Scenario: Inspect split firewood
- **WHEN** the player selects Split Firewood without carrying a hatchet
- **THEN** Timber appears as a consumed Have/Need row
- **AND** the hatchet appears as an unmet retained-tool row

### Requirement: Holding an icon crafts with visible progress
Pressing and holding an available recipe icon with pointer, Enter/Space, or controller A SHALL begin a 1.2-second real-time craft cycle. The icon SHALL begin grey and reveal its full color progressively until completion. A quick press/release SHALL still select the recipe but SHALL NOT craft.

#### Scenario: Complete one held craft
- **WHEN** the player holds an available recipe for one complete 1.2-second cycle
- **THEN** the color fill reaches completion and exactly one authoritative recipe transaction is requested

#### Scenario: Release before completion
- **WHEN** the player releases after a partial fill
- **THEN** incomplete progress clears without consuming ingredients, granting output, or queuing later completion

#### Scenario: Hold an unavailable recipe
- **WHEN** the player holds a grey unavailable recipe
- **THEN** no progress cycle or transaction starts and its shortage list remains visible

### Requirement: Continued hold repeats complete crafts
After a successful cycle, continued hold SHALL immediately begin another 1.2-second cycle while the recipe remains available. Each cycle MUST revalidate current requirements before beginning and commit at most one transaction at completion.

#### Scenario: Hold through multiple cycles
- **WHEN** supplies and capacity permit three recipe executions and the player holds through three complete fills
- **THEN** exactly three transactions commit and the fourth cycle does not start if requirements are no longer met

#### Scenario: State changes during a cycle
- **WHEN** load, recovery, page/focus change, modal opening, input loss, or authoritative state change invalidates the active hold
- **THEN** the incomplete cycle cancels without mutation or replay

### Requirement: Craft progress has restrained audible feedback
Each active cycle SHALL play three distinct or varied hammer-on-anvil/metal strike cues synchronized to progress. Cues SHALL respect Effects volume and mute. Sound and fill are presentation-only and MUST NOT grant output or determine transaction completion.

#### Scenario: Craft while effects are muted
- **WHEN** Effects volume is zero and a cycle completes
- **THEN** visual progress and the authoritative craft still complete normally with no audible strikes

### Requirement: Availability remains explanatory rather than authoritative
Pre-activation availability SHALL be derived from the same current recipe costs, retained requirements, station proximity, inventory capacity, and player state enforced by crafting authority. The authoritative transaction MUST revalidate all rules when an available recipe is activated.

#### Scenario: State changes between display and activation
- **WHEN** the displayed recipe was available but authoritative state changes before activation
- **THEN** crafting authority rejects the stale attempt without consuming ingredients or granting output
- **AND** the Craft page refreshes to show current requirements

#### Scenario: Supplies exist only in storage
- **WHEN** a nearby chest contains ingredients that are absent from the carried pack
- **THEN** those stored items do not satisfy the recipe Have amount
- **AND** the player is directed by the displayed shortage to move supplies into the pack

#### Scenario: A source rule changes
- **WHEN** an item no longer comes from the source named by its acquisition hint
- **THEN** source-contract validation fails rather than leaving misleading guidance in the Craft page

### Requirement: Craft clarity works across supported input and layout
Availability styling, semantic requirement rows, selection, focus, scrolling, hold progress, release/cancel, and repeat behavior SHALL remain readable and synchronized for pointer, keyboard, and controller at supported 720p and 4K layouts.

#### Scenario: Navigate mixed recipe states
- **WHEN** the player moves through available and unavailable recipes using pointer, arrows/D-pad, or tab-region navigation
- **THEN** visible focus and details always correspond to the same selected recipe
- **AND** greyed styling does not hide focus indication or requirement text
