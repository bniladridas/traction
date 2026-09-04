# Task 11 E2E Verification: Race Positions

Date: 2026-09-04 (UTC). Engine 5.8.2. Machine M1/8GB, nullrhi standalone.
Implements GitHub issue #29 on branch `feat/task-11-positions`. Tasks 2,
3, 5, 6, 7, 8, 9, 10 contracts frozen; Task 11 evidence is separate in
`Saved/Task11E2E/results.json`. No HUD, multi-car, physics, or visual
work.

## Objective

Live positions (P1..Pn) from manager state: finished participants by
finish sequence, the rest by laps then along-track distance with
deterministic index tie-break. N-agnostic computation, two-participant
proof.

## Architecture

Positions derive inside `ARaceManager` from finished flags, finish
sequence, completed laps, and nearest-centerline distance (full
deterministic scan over the track's existing center points; no new
track representation, documented here per the implementation guard).
Vehicles unchanged.

## E2E program (hop-by-hop teleports, Task 8 mechanics)

Grid order, AI progress order, lap dominance (AI lap 1 vs player lap
0), player overtake to lap 1 plus a 500 cm nudge, reset with AI grid
re-staging, restart, both to two finished laps, extras frozen.

## Measurements (final passing run, exit 0, 86275 frames)

Orders: grid 01, progress 10, dominance 10, overtake 01, reset 01,
finish 10. Both laps 2/2 at finish. First-attempt green.

## Thresholds (fixed before the final passing run)

Step dwell 0.35 s. Program timeout 150 s. LapCount 2 from race config.

## Regression

Final binary: old-map Tasks 2/3/5/6 green, circuit Task 7 6/6,
race-state Task 8 10/10, AI Task 9 6/6, camera Task 10 6/6,
`tools/check_regression.py` 58/58 PASS. Frozen schemas and thresholds
untouched.

## Build result

`Build.sh` exit 0. No new module paths.

## Runtime and log result

Exit 0 via `QuitGame`. Log: 0 load errors, 0 ensures, 0 fatals.

## Rendered verification status

`Rendered verification: Blocked by missing Xcode Metal Toolchain component.`
No run spent, nothing manufactured.

## Limitations

Computed positions only; nothing displayed. Two-participant proof;
6-car position verification stays a release target. No cut/wrong-way
handling: backward driving distorts scores (documented future work).
Nullrhi logic timing only. Visuals, feel, and PIE unverified.

## Next step

Task 12 specification. Deferred: HUD, fields, racecraft.
