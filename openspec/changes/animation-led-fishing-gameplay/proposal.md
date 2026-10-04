# Proposal

## Why

Jenny selected an animation-led fishing minigame instead of text and a progress bar for the October 4 afternoon build.

## What Changes

- Randomize bounded wait-to-bite and strike timing within an understandable animation-led sequence.
- Keep cast/hook/strike on click or controller tool input, separate from E/A interaction.
- Award a fish only on the successful authored catch contact; cancellation and failure never yield fish.
- Replace progress-bar presentation through the Fishing Art lane, preserving habitat rewards.

## Reuse research

Reuse plain C++ FishingSession, habitat/catch hashes, pack-capacity checks, controller water probe, existing tool input and the project's action/contact pattern. Adapt the readable anticipation/reaction feel of farming-sim fishing without copying Coral Island content or adding tension infrastructure. Art authors original clips and icons; no external content is incorporated.

## Smallest useful result and first playable demonstration

Cast at a bank, respond to bite/strike cues, and see the fish arrive on the successful catch animation beat. Miss or cancel and receive no fish. Branch logic, Art integration and Jenny's acceptance are separate gates.

## Capabilities

### New Capabilities

- `animation-led-bank-fishing`: bounded timing, tool-only input and contact-authorized catches.

### Modified Capabilities

None.

## Impact

Simulation fishing state/API/tests and controller input policy. Fishing Art owns character/clip/widget presentation wiring; exact clip contacts must be agreed before integration. No persistent fishing state, save version or placement changes.
