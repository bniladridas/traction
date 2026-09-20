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
  (Task 9); chase cameras verified (Task 10); live positions verified
  (Task 11); 3-car field verified (Task 12); AI pace variation verified
  (Task 13); six-car field verified (Task 14); race results verified
  (Task 15); full 3-lap race verified (Task 16); player-facing race HUD
  verified (Task 17); cockpit camera and view selection verified
  (Task 18). Next: Task 19. No menus, settings, pause, persistence, or
  multiplayer yet.

## Future

- M4 continued: Stage 1-4 AI racecraft, position scale beyond 6 cars.
  Presentation gate met (Task 17): race state player-readable through
  the 20 Hz HUD model (countdown, lap, position, result). Camera gate
  met (Task 18): chase plus rigid cockpit, deterministic view switching.
- M6 Audio+UI: RPM engine audio, tire skid, menus/settings. The HUD
  layer lands here as presentation polish (Task 17 set the data path).
- M7 Optimization: Metal profiling, Low-Ultra presets.
- M8 Beta: external testers, handling/perf tuning only.
- M9 Release: RC1-RC3, macOS packaging, v1.0.0.

Success criteria per phase in `docs/release.md`. Rendered verification
uses the installed Xcode Metal Toolchain; capture path verified.
Human feel, performance, and visual quality remain unverified
(standing limitations).
