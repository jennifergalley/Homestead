# Tasks

- [x] 1.1 `SleepOptions` (until morning / until rested / nap) with wake times; recovery by hours slept.
- [x] 1.2 Energy no longer fails her; out of Energy she dozes off in place (6 h at 6/h) with a toast.
- [x] 1.3 The bed prompt shows the choice and wake time; Up/Down (D-pad) cycles; A confirms.
- [x] 1.4 Native tests (`SleepOptionPolicy`, doze and doze-into-starvation) and FullLoop's outdoor sleeps.
- [ ] 1.5 Verify in PIE: bed at 21:00 part-tired, "until morning", wakes 06:45; up to 05:00, "until
  rested", wakes in the afternoon full; a 1 h nap; draining Energy to empty dozes her off.
- [ ] 1.6 Packaged suites: FullLoop and Hotbar sleep steps pass; the ForageRenewal route (expects fixed
  8 h rests) needs its expectation changed if it's still run.
- [ ] 1.7 Jenny playtests: a night-owl day and an early night.
