# Release: Testing and Gates

Release targets (not current capability). Verified state follows each
line in brackets.

Physics: accel/brake/corner/reset/collision checklist. [Verified
headlessly through Task 7.]
Race: checkpoints, lap count, finish [verified: 2 laps, 1 AI rival],
  positions with 6 cars [target only, not verified].
AI: follows line, brakes for corners, recovers [verified for one
  rival], no deadlock [target only at field scale].
Perf: empty track vs full race vs worst-case shadows, all >= target on
  presets. [Not measured; nullrhi timing only.]

Alpha (internal): everything works. Beta (external): handling/perf/bugs/AI difficulty.
RC1..RC3: bug/perf/crash/UX fixes only. Tag v1.0.0.

Save format versioned: `save_version: 1` (settings, best laps, results).
macOS .app notarization + install test required for RC.
