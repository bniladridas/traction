# ROADMAP.md

Target: 1080p60 on Apple Silicon. Freeze features at RC.

- M1 Foundation (wks 1-2): UE5 install, repo, `game/` project creation, ugly cube-car prototype.
  Gate: drive + steer + brake + chase cam + reset on flat plane.
- M2 Driving (3-6): vehicle physics, suspension, aero, cameras.
  Gate: driving feels good with keyboard/gamepad.
- M3 First Track (7-9): graybox circuit, collision, checkpoints, racing spline.
  Gate: clean laps countable, no wrong-way exploit.
- M4 Racing (10-12): race manager, timing, positions, Stage 1-4 AI.
  Gate: full 3-lap race vs 5 AI, correct results.
- M5 Graphics (13-17): final car, PBR, environment, lighting, effects.
- M6 Audio+UI (18-20): RPM engine audio, tire skid, HUD/menus/settings.
- M7 Optimization (21-23): Metal profiling, Low-Ultra presets.
- M8 Beta (24-26): external testers, handling/perf tuning only.
- M9 Release (27-28): RC1-RC3, macOS packaging, v1.0.0.

Success criteria per phase in `docs/release.md`.
