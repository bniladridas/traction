# Task 8 E2E Verification: Lap and Race State

Date: 2026-09-04 (UTC). Engine 5.8.2. Machine M1/8GB, nullrhi standalone.
Implements GitHub issue #13 on branch `feat/task-8-race-state`. Tasks 2,
3, 5, 6, 7 contracts frozen; Task 8 evidence is separate in
`Saved/Task8E2E/results.json`. No AI, HUD, audio, multiplayer, physics,
or visual work.

## Objective

Race progression on the existing TestCircuit1 checkpoints: Ready,
Countdown, Racing, Finished phases, ordered progression, lap counting
with validity, configurable lap count, reset interaction, and read-only
state for future UI.

## Architecture

```text
ARaceStateTestGameMode (?game= override, circuit map only)
    ARaceTrack (geometry + checkpoints, unchanged)
    ARaceManager (new: phase, progression, laps, timing)
    ARaceVehicle (unaware of lap rules; position observed)
    ATask8Probe (scripted teleport program)
```

The manager observes vehicle position against checkpoint planes each
tick. The Task 7 flow, probe, and map program are untouched; the map
file is shared infrastructure, the GameMode selects the program.

## Configuration

`FRaceConfig`: LapCount 2, CountdownDuration 3.0 s. Checkpoint count,
start/finish, and widths resolve from the track at runtime. Duplicating
them in race config was specified but deliberately refused: a second
constant for the same fact can diverge from the track.

## Race rules

Ordered crossing required; anything else is ignored and marks the lap
invalid. A completed invalid sequence does not count and does not time.
Planes re-arm 100 cm behind. Finish locks the state; further crossings
change nothing. Reset contract: progress cleared to Ready and the
vehicle returned to the track-owned start (game layer calls the vehicle
reset and `OnVehicleReset` together).

## E2E program (scripted teleports, zero drive input)

Ready observed, StartRace, countdown to Racing. Dirty sequence (ordered
crossings plus a deliberate wrong-order CP4 and a double CP2):
8 expected, 2 ignored, 0 laps, invalid. Reset asserted (Ready, zeros,
at start). Restart to Racing. Two clean sequences: lap 1 valid, lap 2
finishes (times recorded). Extra crossings stay at 2 laps.

## Measurements (final passing run, exit 0, 65379 frames)

Laps after dirty/clean/clean/extras: 0/1/2/2. Last lap 6.80 s, best
6.50 s (functional timing through teleports, not representative pace).
Reset error 0.0 cm. Countdown delay 0.0 s, racing delay 3.0 s in engine
time (nullrhi timing runs dilated; self-consistent).

## Thresholds (fixed before the final passing run)

Countdown seen within 5 s of StartRace, Racing within 8 s. Step dwell
0.4 s. Reset within 50 cm of start. No timeout (120 s program).

## Regression

Old-map run on the final binary: Tasks 2, 3, 5, 6 all green (24/24).
Circuit-map Task 7 run on the final binary: 6/6 green. Frozen schemas
and thresholds untouched.

## Build result

`Build.sh` exit 0 (plus `RacingGame/Race` include path).

## Runtime and log result

Exit 0 via `QuitGame`. Log: 0 load errors, 0 ensures, 0 fatals.

## Rendered verification status

`Rendered verification: Blocked by missing Xcode Metal Toolchain component.`
No run spent, nothing manufactured.

## Limitations

Teleport-driven state test, not a driven race (Task 7 covers driven
traversal of the same planes). Lap times prove timing functions, not
pace. Two-vehicle, UI, and audio integration pending. Nullrhi logic
timing only. Visuals, feel, and PIE unverified.

## Development notes (failing run before passing)

- First run: 8/10. Reset error 53.9 cm traced to the script's own tag
  steps teleporting after measuring; tags now evaluate in place.
- Same run: clean sequence started during the restart countdown (3 of 8
  crossings silently skipped, rest ignored with expect 0). Script now
  pauses stepping until Racing is observed again.
- Thresholds frozen throughout; only the script changed.

## Next step

Task 9 specification. Deferred: AI, HUD, multiplayer, presentation.
