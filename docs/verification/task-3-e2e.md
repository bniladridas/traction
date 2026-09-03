# Task 3 E2E Verification: Vehicle Dynamics Foundation

Date: 2026-09-04 (UTC). Engine 5.8.2. Machine M1/8GB, nullrhi standalone.
Map: `/Game/Task2/Task2_TestTrack`. Harness: extended `ATask2Probe` through
`?game=/Script/RacingGame.RaceTestGameMode`. Task 2 phases, thresholds,
and JSON shape frozen; only additive Task 3 fields and one documented
program extension (10.5 to 13.5 s low-speed steering window).

## Objective

Evolve `URaceVehicleMovement` from planar kinematics to a small force-based
dynamics model (mass, gravity, contact, engine curve, brake force, reverse
limit, speed-sensitive steering, wheel representation) while the Task 2
regression contract stays green.

## Architecture

Same RacingGame structure. `ARaceVehicle` unchanged in role plus
settle-to-ground placement and read-only state getters. All dynamics live
in `URaceVehicleMovement`; no second framework.

## Parameters (single location: movement component defaults)

Mass 1200 kg. Gravity 980 cm/s^2. Engine force points (speed cm/s to force
N): (0, 9000), (1000, 8200), (2000, 6400), (3000, 3800), (4000, 1000),
(4500, 0). Brake force 14000 N. Reverse force 3000 N with 700 cm/s cap.
Steering: 110 deg/s low reference with authority 1/(1+(v/1200)^2), dead
below 100 cm/s. Rolling resistance 250 N, quadratic drag 0.35 N per
(m/s)^2. Reverse engagement under 60 cm/s. Wheels at half-base 70 cm,
half-track 75 cm, rest height -30 cm, 60 cm downward traces.

## Units and integration

Centimeters, seconds, kilograms, newtons. Acceleration in cm/s^2 equals
force in newtons divided by mass in kilograms, times 100. Semi-implicit
Euler at frame delta. No bit-determinism claim; repeat runs on this
machine agree within the thresholds below.

## Predefined Task 3 thresholds (fixed before the first passing run)

Floor: actor Z never under 39.0 cm. Grounded samples at least 0.90.
All-four-wheel contact samples at least 0.90. Peak forward accel over
300 cm/s^2 with late-window accel under 0.90 of early-window accel. Peak
brake decel over 500 cm/s^2. Reverse speed magnitude at most 735 cm/s.
Normalized high-speed yaw rate under 0.80 of normalized low-speed rate.

## Measurements (final passing run, exit 0, 26349 frames)

Floor 40.0 cm, grounded 1.000, wheels 1.000. Accel windows 692.2 and
581.0 (taper 0.839), peak accel 723.1. Peak decel 1194.0. Reverse max
330.4. Yaw rates 37.15 (high) and 97.6 (low) deg/s per unit steer, ratio
0.381. Task 2 regression: forward 2142.6 cm at 0.0 deg, brake ratio
0.276, reverse 149.3 cm, steer 46.11 deg, camera travel 6923.9 against
6860.1 cm, reset errors 0.125 cm with 0.0 deg and 0.0 cm/s.

## Verification table

| Check | Result | Evidence |
|---|---|---|
| Gravity and contact | Verified | floor, grounded, wheel fractions above |
| Mass and acceleration | Verified | peak and taper above; force over mass by construction |
| Brake force | Verified | peak decel above; Task 2 brake check green |
| Reverse bound | Verified | max magnitude above; displacement check green |
| Speed-sensitive steering | Verified | ratio above; Task 2 steer check green |
| Wheel representation | Verified | four traced points, fraction above |
| Camera regression | Verified | Task 2 flag true |
| Reset regression | Verified | Task 2 flag true |
| Full Task 2 regression | Verified | all six flags true, thresholds unchanged |
| Render performance | Not yet verified | nullrhi logic rate only |
| Visuals, feel, PIE | Not yet verified | require display access |

## Build and runtime results

Clean `Build.sh` exit 0 after final edit. E2E exit 0 via `QuitGame`.
Log: 0 load errors, 0 ensures, 0 fatals.

## Issues and limitations

- Resting-contact sweep block: a downward sweep starting in touching
  contact blocks at time zero and cancels horizontal motion. Fixed by
  pinning falling speed to zero while grounded (falling and upward motion
  unaffected). Found through frozen-actor evidence with growing velocity
  state, fixed in the model.
- Two probe program defects (P6 phases overlapping the reset measurement
  window) contaminated reset readings across two runs. Fixed with explicit
  lower bounds; the Task 2 phases themselves were never touched.
- No gravity before this task: the cube held spawn height. Settle-to-ground
  now places it deterministically; reset restores that transform.
- Rendered verification remains blocked on the missing Xcode Metal
  Toolchain component, a development environment limitation.

## Rendered verification status

`Rendered verification: Blocked by missing Xcode Metal Toolchain component.`
No screenshots manufactured. The `-Task2Shots` capture path is unchanged
and activates when the toolchain lands.

## Next step

Task 4 specification. Candidates already deferred: suspension, tire
curves, gears, aero.
