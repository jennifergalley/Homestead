# Tasks

## 1. Animation-led fishing

- [x] 1.1 Implement native randomized timing/contact/reward/cancel/failure rules and tool-only controller policy against Art's agreed API; focused native tests prove no timer reward, exact successful-contact reward and bounded retries; compile combined interfaces. Candidate 9958a359: 861 fishing checks; 13-action editor compile/source stamp green; cancellation, hitch and 0.65-second strike regressions pass.
- [ ] 1.2 Jenny checks the integrated Art/gameplay sequence at a bank: readable cast/bite/strike/catch cues, variable wait/strike timing, fish awarded on the successful lifted-fish beat, and no catch on miss/cancel.
