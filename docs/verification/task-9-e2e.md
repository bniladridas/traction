# Task 9 E2E Verification: AI Opponents

Date: 2026-09-04 (UTC). Engine 5.8.2. Machine M1/8GB, nullrhi standalone.
Implements GitHub issue #17 on branch `feat/task-9-ai`. Tasks 2, 3, 5,
6, 7, 8 contracts frozen; Task 9 evidence is separate in
`Saved/Task9E2E/results.json`. No UI, audio, graphics overhaul, or
multiplayer.

## Objective

One narrow AI rival: same vehicle, game-side pursuit driver, race
participation through the existing manager, off-track/stall recovery,
deterministic headless proof. The AI consumes track and race
architecture; no new circuit definition anywhere.

## Architecture

```text
ARaceAITestGameMode (?game= override, circuit map only)
    ARaceTrack (unchanged)
    ARaceManager (per-participant state; player 0, AI 1)
    ARaceVehicle x2 (identical physics, fair race)
        player: parked clear, zero input (single-racer test)
        AI: URaceAIDriver (pursuit + recovery, Apply* only)
    ATask9Probe (forces recovery, asserts gates)
```

The manager refactor (per-participant ExpectIdx, laps, validity,
times, planes) preserves single-player behavior through participant 0;
the Task 8 teleport program passes identically after it.

## Configuration

Driver parameters are data (`URaceAIDriver` properties): lookahead,
steer gain, three speed targets, two curvature thresholds, off-track
margin, stall window. Track and race config untouched.

## E2E program

AI laps from the grid while the player sits parked clear of its line.
Past 3000 cm the probe drops the AI into the void south of the
circuit; recovery respawns it on the centerline, then the race restarts
and the AI drives a full clean lap (18.64 s) through the manager.

## Measurements (final passing run, exit 0, 64032 frames)

AI progress 11688 cm, 1 valid lap, last 18.64 s, 2 recovery respawns
(one forced, one start-line graze the stall window caught). Player
parked, zero interference.

## Thresholds (fixed before the final passing run)

Progress over 3000 cm within 40 s of Racing. Recovery (in-lane plus
moving) within 10 s of the force. Program timeout 150 s.

## Regression

Final binary: old-map Tasks 2/3/5/6 green (24/24), circuit Task 7 6/6,
race-state Task 8 10/10, `tools/check_regression.py` 46/46 PASS. Frozen
schemas and thresholds untouched.

## Build result

`Build.sh` exit 0 (plus `RacingGame/AI` include path).

## Runtime and log result

Exit 0 via `QuitGame`. Log: 0 load errors, 0 ensures, 0 fatals.

## Rendered verification status

`Rendered verification: Blocked by missing Xcode Metal Toolchain component.`
No run spent, nothing manufactured.

## Limitations

Single rival, parked player (multi-car racing later). Test driver lines,
not racecraft: no overtaking, blocking, or mistakes. Lap pace proves
function, not competitiveness. Nullrhi logic timing only. Visuals,
feel, and PIE unverified.

## Development notes (failing runs before passing)

- First run: forced teleport landed 64 cm from another section (the
  folded loop). Force now targets a fixed void point 2000+ cm clear.
- Then: respawns hung at Z=60 with contact true, load zero, drive zero.
  Root cause in the model interaction: traces touch from tick one while
  the box needs free-fall plus box block to settle (spawn path); a
  mid-run drop freezes instead. Respawns now place exact rest height
  (Z=40). Vehicle model untouched.
- Then: AI rammed the re-snapped player 29 times (restart puts the
  player on the start line, in the staged AI's path). The probe re-parks
  the player on restart.
- Thresholds frozen throughout; only test geometry and script changed.

## Next step

Task 10 specification. Deferred: racecraft, multi-car, UI, audio.
