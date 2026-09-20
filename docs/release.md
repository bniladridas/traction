# Release: Testing and Gates

Release targets (not current capability). Verified state follows each
line in brackets.

Physics: accel/brake/corner/reset/collision checklist. [Verified
headlessly through Task 7.]
Race: checkpoints, lap count, finish [verified: full 3-lap race,
  six-car field with 5 AI], positions with 6 cars [verified through
  Task 14], recorded results [verified through Task 15].
AI: follows line, brakes for corners, recovers [verified for one
  rival], no deadlock [verified at 6-car field scale and full 3-lap
  race].
Perf: empty track vs full race vs worst-case shadows, all >= target on
  presets. [Not measured; nullrhi timing only.]

Alpha (internal): everything works. Beta (external): handling/perf/bugs/AI difficulty.
RC1..RC3: bug/perf/crash/UX fixes only. Tag v1.0.0.

Save format versioned: `save_version: 1` (settings, best laps, results).
macOS .app notarization + install test required for RC.
