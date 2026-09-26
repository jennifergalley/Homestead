# Working with Jenny on Homestead

## Loops

Jenny works in two modes. When she hasn't said which, treat short requests as the Interactive Loop.

- **Interactive Loop** (default when she's around): work in short, incremental loops that each
  deliver one small end-to-end improvement she can playtest (for example, one new animation).
  When an improvement is done:
  1. package a new playable build to `Build\Windows` (`Scripts\Build-Game.ps1 -Package`; her
     desktop shortcut launches it),
  2. commit with a descriptive message,
  3. push to `main`.

  Then report what to try. Don't batch several improvements into one delivery.
- **Autonomous / Autopilot Loop**: when she puts the session on autopilot, work continuously for
  hours on long-running improvements or full feature build-outs, selecting the next thing to
  iterate on as each one completes.

## Judgment calls

- When you're unsure whether to make a change, make it if it would enhance realism and
  verisimilitude (for example, carried sticks lying across her body instead of jutting forward,
  keeping limbs out of her torso, or a more natural pose). Say what you changed when you report.

## Builds

- If the packaged game is running from `Build\Windows` when you need to repackage, close it and
  build in place. She is only experimenting in it for now and prefers getting the newest build.
- The editor skill (`.github/skills/unreal-editor-mcp/SKILL.md`) covers driving the live editor,
  playtesting, and the character lab (`-HomesteadCharacterLab` / `homestead.CharacterLab 1`).
- Record bugs and features as OpenSpec changes under `openspec/changes/`.
