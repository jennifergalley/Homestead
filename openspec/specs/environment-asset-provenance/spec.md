# environment-asset-provenance Specification

## Purpose
Admit only traceable zero-cost environment assets and preserve honest evidence,
offline operation and reversible candidate delivery for the visual upgrade.

## Requirements

### Requirement: Asset admission requires current provenance

Every new asset SHALL have an authoritative source URL, creator, exact license
and license URL, dated zero-price evidence, permitted distribution assessment,
and recorded acquisition prerequisites before admission. Downloaded source files
SHALL have actual byte sizes and SHA-256 receipts. Missing evidence SHALL block
that asset or select the documented admissible fallback, not be inferred from
an expired promotion or another asset's license.

#### Scenario: Unverified marketplace entitlement
- **WHEN** a candidate listing requires an account, claim or license acceptance
  that has not been completed by the authorized human
- **THEN** acquisition remains blocked and the report identifies that prerequisite
  without making a purchase, accepting terms or claiming an entitlement

#### Scenario: Receipt mismatch
- **WHEN** a source file differs from its previously recorded hash or expected size
- **THEN** the acquisition/import process fails visibly and does not overwrite the
  receipt or silently admit changed content

### Requirement: Distribution respects the admitted license

Credits and provenance SHALL accompany admitted assets. Raw sources and editable
projects SHALL be distributed only where the exact license permits it; assets
whose license permits only integration into a product SHALL NOT be exposed as
standalone redistribution. Publisher preview images SHALL remain reference links
unless their own reuse permission is verified.

#### Scenario: Assemble a distributable candidate
- **WHEN** the candidate is packaged or its repository assets are shared
- **THEN** license-specific restrictions, credits and permitted source/cooked
  content boundaries are checked rather than treating every zero-price asset as CC0

### Requirement: Engine execution requires the offline-safe gate

The asset workflow SHALL NOT launch an editor, commandlet, importer, build or
test process until the coordinator approves an offline-safe workflow for those
exact executable types. A listener-free Shipping game result SHALL NOT count as
proof that editor/import tooling is safe. Runtime gameplay SHALL remain offline,
with no new network service dependency or firewall/OS rule change.

#### Scenario: Shipping is safe but editor status is unknown
- **WHEN** the accepted packaged game has passed offline checks but the new-asset
  import executable has not been cleared
- **THEN** acquisition planning can proceed without engine execution, and import
  remains blocked with the missing approval recorded

### Requirement: Evidence separates technical and subjective outcomes

Candidate evidence SHALL record source/package identity, matched camera and
world state, actual output viewport, requested and runtime primary render scale,
Lit mode, Lighting enabled and Shader Complexity disabled. Performance evidence
SHALL distinguish screenshot-free timing from sampled capture and separate
CPU/GPU/frame statistics from physical display/Present/VRR observations.
Technical checks SHALL NOT be labeled Jenny's aesthetic approval.

#### Scenario: Large image without render-state evidence
- **WHEN** an image is 3840 by 2160 but its primary render fraction or show flags
  are unverified
- **THEN** it is not counted as a normal-Lit native-4K/100-percent acceptance capture

#### Scenario: Performance target missed
- **WHEN** the requested 4K/100-percent candidate misses the predeclared timing
  gate or lacks trustworthy GPU evidence
- **THEN** the result names the missed or unmeasured gate and does not claim stable
  physical 60 FPS; any lower-scale comparison is labeled separately

### Requirement: Candidate promotion is explicit and reversible

The upgrade SHALL be packaged to a fresh candidate with its own basic build,
gameplay and visual evidence, keeping cheap rollback to the prior build. The
latest basically verified usable slice MAY become the normal preview under
the user's standing delivery authorization. Old test saves MAY be reset when
necessary, with that reset reported; unrelated data SHALL NOT be deleted.
Incomplete palette/performance work and modest known defects SHALL be disclosed,
not represented as full acceptance or made universal blockers to first delivery.

#### Scenario: Visual regression despite functional success
- **WHEN** gameplay tests pass but the new foliage looks worse or obscures play
- **THEN** blocking readability regressions are corrected before promotion,
  or the prior playable candidate remains selected; test-save preservation
  does not supersede current user instructions
