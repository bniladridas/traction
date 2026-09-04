# ROADMAP.md

Target: 1080p60 on Apple Silicon. Freeze features at RC.

## Completed

- M1 Foundation: UE5 install, repo, `game/` project, cube-car prototype.
  Gate met: drive + steer + brake + chase cam + reset on flat plane.
- M2 Driving (vehicle systems): force-based movement, mass/gravity,
  suspension, tire forces with friction circle, drivetrain with gears,
  data-driven config. Gate met headlessly (24/24 regression checks).
  Deferred: aero (explicitly out of the vehicle tasks), feel tuning
  (needs interactive play), cameras beyond chase.
- M3 First Track: runtime-built closed circuit (~123 m), road collision,
  boundaries, centerline, ordered checkpoints, track-owned start,
  deterministic lap. Gate met: 6/6 Task 7 gates, full lap evidenced.

## Current

- M4 Racing: lap state, checkpoint progression, valid/invalid laps,
  timing, reset interaction verified (Task 8); one AI rival verified
  (Task 9). Current: cameras (Task 10). No HUD, multiplayer, or
  presentation yet.

## Future

- M4 continued: race manager, positions, Stage 1-4 AI.
  Gate: full 3-lap race vs 5 AI, correct results.
- M5 Graphics: final car, PBR, environment, lighting, effects.
- M6 Audio+UI: RPM engine audio, tire skid, HUD/menus/settings.
- M7 Optimization: Metal profiling, Low-Ultra presets.
- M8 Beta: external testers, handling/perf tuning only.
- M9 Release: RC1-RC3, macOS packaging, v1.0.0.

Success criteria per phase in `docs/release.md`. Rendered verification
throughout is blocked on the Xcode Metal Toolchain component.
