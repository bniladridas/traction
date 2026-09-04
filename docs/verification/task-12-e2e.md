# Task 12 E2E Verification: Multi-Car Field

Date: 2026-09-04 (UTC). Engine 5.8.2. Machine M1/8GB, nullrhi standalone.
Implements GitHub issue #31 on branch `feat/task-12-field`. Tasks 2, 3,
5, 6, 7, 8, 9, 10, 11 contracts frozen; Task 12 evidence is separate in
`Saved/Task12E2E/results.json`. No racecraft, HUD, physics, or visual
work.

## Objective

A 3-car field (player + 2 identical-pace AI rivals): grid slots from
track data, N-participant laps/positions/finish through the existing
manager, no-deadlock proof, reset clearing. Identical pace is stated,
not hidden: this proves field mechanics, not overtaking.

## Architecture

```text
ARaceFieldTestGameMode (?game= override, circuit map only)
    ARaceTrack.GetGridPose (new; staggered slots from centerline)
    3x ARaceVehicle (2 with URaceAIDriver, 1 probe-driven)
    ARaceManager (N-participant path unchanged)
    ATask12Probe (grid, reset, deadlock monitor, gates)
```

No manager, vehicle, or track-model changes. The only game-side
addition is the grid-pose query.

## E2E program

Grid assert (3 registered, pairwise 150 cm+, order 012), reset with AI
grid re-staging, restart, concurrent race to all-finished with stall
monitoring, gates recorded.

## Measurements (final passing run, exit 0, 128849 frames)

Grid gaps 224/283/412 cm, order 012. Both AI past 3000 cm. All laps
2/2/2, final order 012. Max progress gap 18.11 s (under the 20 s stall
window). Two genuine recovery respawns, staggered, no stacking.

## Thresholds (fixed before the final passing run)

FieldSize 3. Grid separation 150 cm. Progress 3000 cm within 40 s.
Stall: under 50 cm over any rolling 20 s while Racing and unfinished;
only a respawn with resumed progress clears it. Reset at 12 s. Timeout
240 s. LapCount 2 from race config.

## Regression

Final binary: old-map Tasks 2/3/5/6 green, circuit Task 7 6/6,
race-state Task 8 10/10, AI Task 9 6/6, camera Task 10 6/6, positions
Task 11 6/6, `tools/check_regression.py` 64/64 PASS. Frozen schemas and
thresholds untouched.

## Build result

`Build.sh` exit 0. No new module paths.

## Runtime and log result

Exit 0 via `QuitGame`. Log: 0 load errors, 0 ensures, 0 fatals.

## Rendered verification status

`Rendered verification: Blocked by missing Xcode Metal Toolchain component.`
No run spent, nothing manufactured.

## Limitations

Identical AI pace: no overtaking, blocking, or mistakes proven. Fields
beyond 3 untested. Computed order only; nothing displayed. Nullrhi
logic timing only. Visuals, feel, and PIE unverified.

## Development notes (failing runs before passing)

- Run 1: field froze post-reset (all v=0, 180 respawns, timeout).
  Diagnosis with per-driver heartbeats: reset staging teleports cars
  while driver stall state still references pre-teleport progress, so
  every driver false-stalls simultaneously and respawns to the same
  dense point, where exact-overlap depenetration stacks them roof to
  roof and the cycle repeats. Fixed with `URaceAIDriver::Reanchor`
  (called on every test staging) plus staggered respawns and alternating
  lateral offsets. Vehicle and manager untouched.
- Same run exposed the placement rule now recorded: mid-run drops from
  snap height freeze on first trace contact (grounded rule), while
  spawns settle via free-fall plus box block. All staged placements
  land at exact rest height (Z=40). Task 9's second recovery is now
  understood as this mechanism self-healing through a stall respawn.
- Thresholds frozen throughout; only test and driver-support code
  changed.

## Next step

Task 13 specification. Deferred: racecraft, HUD, larger fields.
