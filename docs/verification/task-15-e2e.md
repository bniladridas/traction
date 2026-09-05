# Task 15 E2E Verification: Race Results

Date: 2026-09-05 (UTC). Engine 5.8.2. Machine M1/8GB, nullrhi standalone.
Implements GitHub issue #51 on branch `feat/task-15-results`. Tasks 2,
3, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 contracts frozen; Task 15 evidence
is separate in `Saved/Task15E2E/results.json`. No persistence, UI,
scoring, physics, or visual work.

## Objective

Recorded race results at finish: ordered participant indices, laps,
best and final-valid lap times per finisher, completion flag. Read-only
for future UI. Completes the loop from race state to recorded outcome.

## Architecture

`FRaceResults` in `ARaceManager`, filled once when every participant
has finished, cleared on reset. Entries exist only for participants
that actually finished. No new actor, track, or vehicle changes.

## E2E program

Driven player plus five AI to an all-finished race, asserting empty
before finish, populated with six valid entries at finish, best/last
consistency, reset clearing, and immutability across a genuine observed
post-finish crossing. "Last lap" means the recorded final valid lap
time throughout.

## Measurements (final passing run, exit 0, 121328 frames)

Empty latched pre-finish. Six entries, all laps 2, best times positive
and within final laps. Reset cleared (laps 0, no results). Post-finish
CP0 crossing changed nothing. No deadlock trip.

## Thresholds (fixed before the final passing run)

Reset at 12 s. Timeout 300 s. LapCount 2 from race config. Stall
definition reused unchanged (50 cm / 20 s).

## Regression

Final binary: all nine prior programs green,
`tools/check_regression.py` 82/82 PASS. Frozen schemas and thresholds
untouched.

## Build result

`Build.sh` exit 0. No new module paths.

## Runtime and log result

Exit 0 via `QuitGame`. Log: 0 load errors, 0 ensures, 0 fatals.

## Rendered verification status

Verified for the capture path (Standing Limitations). No new captures
in this task; nothing manufactured.

## Limitations

In-memory only; no save file. Computed table only; nothing displayed.
Nullrhi logic timing only. Visuals, feel, and PIE unverified.

## Development notes (failing runs before passing)

- Run 1: 4/6 with a stalled player. The probe drove the player but had
  no recovery for it, unlike every AI. Added probe-side player recovery
  mirroring the driver rule. Thresholds frozen throughout.
- Run 2: all finished but immutable failed. The snapshot compared live
  order strings, which legitimately move as cars keep driving
  post-finish. The check now compares the results table only.
- Run 3: populated failed with all laps 2. The restart never re-armed
  the racing-seen latch, so completion was never evaluated. One-line
  fix per the Task 12 pattern. Thresholds frozen throughout.

## Next step

Task 16 specification. Deferred: persistence, HUD, scoring.
