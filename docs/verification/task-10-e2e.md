# Task 10 E2E Verification: Cameras

Date: 2026-09-04 (UTC). Engine 5.8.2. Machine M1/8GB, nullrhi standalone.
Implements GitHub issue #21 on branch `feat/task-10-camera`. Tasks 2,
3, 5, 6, 7, 8, 9 contracts frozen; Task 10 evidence is separate in
`Saved/Task10E2E/results.json`. No graphics overhaul, cinematics, or UI.

## Objective

Chase cameras for player and AI: velocity look-ahead, smoothing, reset
and teleport snap, race-state compatibility, strict separation from
vehicle physics, deterministic headless measurement of transforms.

## Architecture

```text
ARaceCameraTestGameMode (?game= override, circuit map only)
    ARaceTrack + ARaceManager (unchanged)
    ARaceVehicle x2, each with URaceChaseCamera driving its spring arm
    ATask10Probe (drives player, resets mid-run, measures both cameras)
```

The component reads pawn transform and motion only. Relative-offset
design: the arm keeps its follow inheritance and gains a smoothed yaw
offset, so follow stability is structural, not tuned.

## Configuration

`FRaceCameraConfig` via the vehicle config: arm 500 + speed gain to 700
max, height 140, base pitch -14, look-ahead 0.35 per deg/s clamped 25,
smoothing 6/s, teleport snap over 200 cm. No collision probe (it snaps
near walls; clipping is an art-pass concern).

## E2E program

Race starts, player pursues while the AI races. Player reset with
track-start re-snap at 20 s. Measure to 38 s, finish at 42 s.

## Measurements (final passing run, exit 0, 82943 frames)

Camera path 12825 vs pawn 11339 cm (player), 14013 vs 11966 cm (AI).
Look-ahead mean +11.05 deg over 12569 turning samples. Max per-tick
displacement 18.92 / 17.48 cm outside discontinuity windows. Reset
offset error 0.3 cm, yaw error 0.04 deg.

## Thresholds (fixed before the final passing run)

Follow ratio over 0.5 each. Lead mean over 1 deg with 100+ samples.
Pops under 50 cm. Reset under 30 cm and 5 deg. Reset at 20 s, measure
to 38 s, finish at 42 s.

## Regression

Final binary: old-map Tasks 2/3/5/6 green, circuit Task 7 6/6,
race-state Task 8 10/10, AI Task 9 6/6, `tools/check_regression.py`
52/52 PASS. The camera driver runs in every one of those runs, which
is the separation evidence: identical vehicle gates with the system
active. Frozen schemas and thresholds untouched.

## Build result

`Build.sh` exit 0 (plus `RacingGame/Camera` include path).

## Runtime and log result

Exit 0 via `QuitGame`. Log: 0 load errors, 0 ensures, 0 fatals.

## Rendered verification status

`Rendered verification: Blocked by missing Xcode Metal Toolchain component.`
No run spent, nothing manufactured. Camera transforms measured; pixels
never claimed.

## Limitations

Transform behavior only; framing taste, clipping, and feel unverified
without rendering. Single fixed chase style; no cockpit, cinematics, or
options. Nullrhi logic timing only. Visuals, feel, and PIE unverified.

## Development notes (failing run before passing)

- First run: 5/6, AI camera jumped 145 cm in one tick. Two causes, both
  fixed: the spring-arm collision probe snapping near walls (disabled,
  documented) and AI recovery teleports correctly moving the camera
  with the pawn (now excluded as legitimate discontinuities, logged).
- Thresholds frozen throughout; only the component and script changed.

## Next step

Task 11 specification. Deferred: cockpit, cinematics, UI, art.
