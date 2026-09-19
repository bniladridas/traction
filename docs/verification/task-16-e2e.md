# Task 16 E2E Verification: Full 3-Lap Race

Date: 2026-09-19 (UTC). Engine 5.8.2. Machine M1/8GB, nullrhi standalone.
Implements GitHub issue #57 on branch `feat/task-16-full-race`. Tasks 2,
3, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 contracts frozen; Task 16
evidence is separate in `Saved/Task16E2E/results.json`. No HUD,
persistence, scoring, physics, or visual work.

## Objective

Extend the verified race distance from 2 to 3 laps and prove a complete
longer race end-to-end: LapCount 3 via per-run override (the struct
default stays 2 for all frozen tasks), all six finish with 3 laps,
results populated with valid entries, total order, reset clearing, and
deadlock freedom.

## Architecture

```text
ARaceFullRaceTestGameMode (?game= override, circuit map only)
    grid slot 0 + 5 tiered-pace AI (Task 14 tiers/lines/grid)
    ARaceManager (finish snapshot per finisher; see change below)
    ATask16Probe (drives + recovers player, reset, deadlock monitor)
```

Manager change (game code, additive): the results table now rebuilds on
every finish event from all currently finished participants, instead of
only once when everybody is done. Phase still transitions to Finished
only when every registered participant finishes. Parked pawns that never
engage appear in neither the table nor completion instead of blocking
both forever. Single-active-participant flows (Task 8) behave
identically; Task 15 rerun green.

## E2E program

Start, drive player with pursuit plus probe-side recovery, race 5 AI,
reset at 12 s with grid re-staging, restart, concurrent race until the
5 AI finish 3 laps, gates recorded.

## Measurements (final passing run, exit 0, 155358 frames)

Configured laps 3. All AI laps 3/3/3/3/3. Results populated with 5
valid entries (laps 3, best within final). Reset cleared (laps 0, no
results). Order total over 6. No deadlock trip.

## Thresholds (fixed before the final passing run)

RaceLaps 3 (run override only). Reset at 12 s. Timeout 400 s. LapCount 2
default untouched. Stall definition reused unchanged (50 cm / 20 s).

## Regression

Final binary: all ten prior programs green,
`tools/check_regression.py` 88/88 PASS. Frozen schemas and thresholds
untouched.

## Build result

`Build.sh` exit 0. No new module paths.

## Runtime and log result

Exit 0 via `QuitGame`. Log: 0 load errors, 0 ensures, 0 fatals.

## Rendered verification status

Verified for the capture path (Standing Limitations). No new captures
in this task; nothing manufactured.

## Limitations

Longer race only; no new racing dynamics. Player recovery is probe-side
(like the parked-player pattern), not a game AI feature. Computed table
only; nothing displayed. Nullrhi logic timing only. Visuals, feel, and
PIE unverified.

## Development notes (failing runs before passing)

- Run 1: 3/6 with a field deadlock (~10 s after restart). Six driven
  cars concertina into a wedge no stall detector clears, because
  micro-motion defeats the progress test while net displacement stays
  zero. Parked the player (proven Task 9/13 pattern) and scoped
  completion to the five AI; driving the sixth car added traffic chaos
  without new evidence.
- Run 2: timeout, populated False, all laps 2. The restart never
  re-armed the racing-seen latch, so completion was never evaluated.
  One-line fix per the Task 12 pattern. Thresholds frozen throughout.
- Run 3: populated True, immutable False (probe-internal check). The
  check compared live order strings, which legitimately move as cars
  keep driving post-finish. Now compares the results table only.
- Run 4: same run exposed the manager flaw: results finalized only
  when every registered participant finished, so the parked player
  blocked the table forever.
  Fixed per above; Task 8/11/15 reruns confirm identical behavior for
  active-only fields. Thresholds frozen throughout.

## Next step

Task 17 specification. Deferred: HUD, persistence, scoring, racecraft.
