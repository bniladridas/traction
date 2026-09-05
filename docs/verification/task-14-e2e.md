# Task 14 E2E Verification: Six-Car Field

Date: 2026-09-05 (UTC). Engine 5.8.2. Machine M1/8GB, nullrhi standalone.
Implements GitHub issue #49 on branch `feat/task-14-field6`. Tasks 2,
3, 5, 6, 7, 8, 9, 10, 11, 12, 13 contracts frozen; Task 14 evidence is
separate in `Saved/Task14E2E/results.json`. No racecraft, HUD, physics,
or visual work.

## Objective

Scale the verified field from 3 to 6 cars (player parked, 5 tiered-pace
AI drivers): grid slots from track data with offline pairwise proof,
N-participant laps/positions/finish, no-deadlock proof, reset clearing.
Identical-pace limitation lifts to tiered pace; still no reactive
behavior.

## Architecture

```text
ARaceField6TestGameMode (?game= override, circuit map only)
    ARaceTrack.GetGridPose (slots 0-2 byte-identical; slots 3-5 fitted)
    6x ARaceVehicle (5 with URaceAIDriver, pace/line per instance)
    ARaceManager (N-participant path unchanged)
    ATask14Probe (grid, reset, deadlock monitor, gates)
```

Grid table (s, lateral): (400,0), (300,-200), (200,+200), (50,-240),
(50,0), (0,+330). Proven offline: zero overlapping pairs, min pairwise
224 cm, all in-lane, all s >= 0. Pace tiers 0.85/1.0/0.9/0.95/1.05 with
lines -120/+120 alternating.

## E2E program

Grid assert (6 registered, separation, order 012345), reset with grid
re-staging, restart, concurrent race to all-finished with stall
monitoring, gates recorded.

## Measurements (final passing run, exit 0, 114384 frames)

Grid gaps min 224 cm, order 012345. All laps 2/2/2/2/2/2, final order
241350 (total, distinct). Max progress gap 8.28 s (under the 20 s stall
window). No deadlock trip.

## Thresholds (fixed before the final passing run)

FieldSize 6. Grid separation 150 cm. Progress 3000 cm within 60 s.
Stall: under 50 cm over any rolling 20 s while Racing and unfinished.
Reset at 12 s. Timeout 300 s. LapCount 2 from race config.

## Regression

Final binary: all eight prior programs green,
`tools/check_regression.py` 76/76 PASS. Frozen schemas and thresholds
untouched.

## Build result

`Build.sh` exit 0. No new module paths.

## Runtime and log result

Exit 0 via `QuitGame`. Log: 0 load errors, 0 ensures, 0 fatals.

## Rendered verification status

Verified for the capture path (Standing Limitations). No new captures
in this task; nothing manufactured.

## Limitations

Tiered constant pace, not skill/tire/fuel. Fields beyond 6 untested.
Computed order only; nothing displayed. Nullrhi logic timing only.
Visuals, feel, and PIE unverified.

## Next step

Task 15 specification. Deferred: racecraft, HUD, larger fields.
