# Spec Delta

## ADDED Requirements

### Requirement: Eligible planting hints never retire
When carried seeds are selected and eligible tilled soil is focused, the game SHALL display [E] Plant Seeds (or the controller equivalent), regardless of previous successful planting. Occupied, out-of-season or otherwise ineligible soil SHALL NOT advertise successful planting.

#### Scenario: Repeated planting
- **WHEN** she selects in-season seeds and aims at another eligible bare plot after planting more than three times
- **THEN** the keyed Plant Seeds action remains visible and plants the selected crop.

### Requirement: Cultivated seed returns are occasional and small
Cultivated roots SHALL return at most one bonus seed with a 25% deterministic chance instead of two guaranteed seeds. Purchased period crops SHALL keep yielding their produce without bonus seed packets. Existing forage yields and regrowing fruit SHALL remain unchanged.

#### Scenario: Harvest a garden
- **WHEN** she harvests cultivated roots across many plots and days
- **THEN** each harvest gives four roots and zero or one seed, with approximately one seed per four harvests
- **AND** loading the same pre-harvest state does not reroll the outcome.
