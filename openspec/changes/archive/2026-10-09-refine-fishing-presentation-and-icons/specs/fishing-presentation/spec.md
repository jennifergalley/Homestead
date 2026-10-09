# Spec Delta

## ADDED Requirements

### Requirement: Fishing reads as fishing
While fishing, the heroine SHALL play an original cast, wait, bite, strike and catch (or miss) sequence. A line SHALL run from the pole tip to a visible float, and a landed fish SHALL appear on the line. The phase SHALL follow gameplay's fishing state, with contact beats taken from the shared timing contract.

#### Scenario: Cast and catch
- **WHEN** she casts at water, waits for a bite and clicks inside the hook window
- **THEN** she visibly casts, the float bobs on the bite, and she strikes and lifts the caught fish on the line.

### Requirement: Fishing icons match the tool and crop style
The fishing pole and each of the six fish species SHALL show a distinct original icon in the hotbar and the pack, in the same style as the other tools and finished crops.

#### Scenario: Pack view
- **WHEN** she carries the pole and several species
- **THEN** each one shows its own clear icon.
