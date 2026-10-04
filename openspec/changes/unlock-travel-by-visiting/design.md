# Design

## Context

Map markers cover Manor, Mine, Cove, Mill, Gateway, Town and Store. Only Manor and Town currently have travel actions, with Store incorrectly sharing Town. Existing road travel advances time on a trial copy and controller ground-snap rollback protects failed arrivals.

## Goals / Non-Goals

Goals: stable per-destination discovery, inexpensive proximity checks, complete map actions and one simulation gate across all entry points.
Non-goals: moving manor arrival, new signs/markers, asset edits, bypassing travel energy/time or speculative destination systems.

## Decisions

- Append destinations after existing Manor/Town values. Keep a simulation-owned catalogue keyed to existing anchors and labels so discovery and travel use the same identities.
- Track bounded discovery state with a counted optional lowercase `travel` section, omitted at defaults. Default Manor access reflects beginning there; missing legacy sections leave other places locked until physically visited. Reject malformed/duplicate sections transactionally.
- Preserve the road plan and exact manor stop arrival for Manor/Town. Use existing anchor positions/yaws/heights for other places with a distance/time preview and trial advancement. Store arrival remains outside its door.
- Check only the small fixed catalogue during settled gameplay, not resource/placement lists. Reuse normal notices and autosave routes.
- Own travel files/map-action presentation; coordinate only State/command/save hooks with Farming Fishing. Coordinator receives exact save shape before ready.

## Risks / Trade-offs

- Close Town/Store markers -> give each its own appropriate visit radius; visiting the store also establishes physical presence in Town.
- Streaming can fail -> preserve ground-snap simulation rollback for every destination.
- Old builds cannot read new tags -> current reader preserves saves without tags; notify coordinator that rollback readers may refuse newer discovery saves.
