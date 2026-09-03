# Release: Testing and Gates

Physics: accel/brake/corner/reset/collision checklist.
Race: checkpoints, lap count, finish, positions with 6 cars.
AI: follows line, brakes for corners, recovers, no deadlock.
Perf: empty track vs full race vs worst-case shadows, all >= target on presets.

Alpha (internal): everything works. Beta (external): handling/perf/bugs/AI difficulty.
RC1..RC3: bug/perf/crash/UX fixes only. Tag v1.0.0.

Save format versioned: `save_version: 1` (settings, best laps, results).
macOS .app notarization + install test required for RC.
