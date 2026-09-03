# Task 6 E2E Verification: Drivetrain and Power Delivery

Date: 2026-09-04 (UTC). Engine 5.8.2. Machine M1/8GB, nullrhi standalone.
Implements GitHub issue #5 on branch `feat/task-6-drivetrain`. Task 2, 3,
and 5 contracts frozen; Task 6 evidence is separate in
`Saved/Task6E2E/results.json`. No clutch, manual mode, aero, or aids.

## Objective

Power delivery through engine torque, RPM, automatic transmission, and an
open differential into the existing tire path. Suspension, contact,
lateral forces, and friction limiting unchanged as the wheel-ground
authority.

## Architecture

```text
ARaceVehicle
    FRaceDriveCommand
        URaceVehicleMovement (integration and coordination)
            URaceDrivetrain (engine, RPM, gear, torque math)
                driven-wheel longitudinal requests
                    existing per-wheel tire model
```

The drivetrain never applies body force. Service braking stays
movement-side as before. The pawn owns both components and wires them in
`BeginPlay`.

## Configuration

Engine torque curve in Nm over RPM: (800, 150), (1000, 180), (1500, 230),
(2000, 265), (2500, 300), (3000, 325), (3500, 340), (4000, 335),
(4500, 320), (5000, 300), (5500, 275), (6000, 250), (6500, 150),
(6800, 0). Idle 1000, redline 6200, max 6800. Gears 3.0, 1.7, 1.25, 0.95
with final drive 3.5 and 0.9 efficiency. Shift up 2200, down 1000 RPM
with hysteresis. Reverse ratio 3.0. Fixed driven wheels RL and RR.
Wheel radius 0.35 m. Engine braking 800 N above 200 cm/s. All in
`FRaceVehicleConfig`; gear and shift sizing targets the frozen 2.5 s
full-throttle window and is recorded as development calibration, not
production tuning.

## Engine model

Torque from the curve at current RPM, scaled by throttle. RPM follows the
wheels through gear and final drive above idle, floored at idle, capped at
the maximum. No clutch slip or converter model; the simplification is
documented, not hidden.

## Transmission model

Four forward gears with hysteresis shifts while rolling, first gear from
standstill, explicit reverse state entered by holding brake near
standstill and released by throttle past the forward threshold. No
oscillation guards beyond the asymmetric thresholds.

## Differential model

Open differential: axle torque split equally across contacting driven
wheels. No vectoring, no limited slip. The tire friction circle remains
the final authority per wheel.

## Reverse behavior

Reverse requests flow through gear ratio and the tire path with the
positive speed cap enforced. Throttle in reverse recovers forward, which
releases the state back to first.

## Measurements (final passing run, exit 0, 29440 frames)

RPM 1000 to 2094, torque peak 272 Nm. Initial gear 1, maximum forward
gear 2, reverse observed. Mean shaft torque 1793 Nm with mean driven
force 4898 N in acceleration. Two upshifts, two downshifts. Engine
braking -800 N in the lift window.

## Thresholds (fixed before the final passing run)

RPM within 990 and 6800 with maximum over 1500. Maximum gear at least 2
with at least one shift each way. Reverse gear observed. Mean driven
force over 500 N accelerating. Lift-window engine braking magnitude
between 200 and 3000 N opposing motion.

## Task 2 regression

All six true: forward 1238.3 cm, brake 0.006, reverse 774.5 cm backward,
steer 66.28 deg, camera 1502.3 against 1450.1 cm, reset zeros. Frozen
contract intact.

## Task 3 regression

All six true: floor 40.0, grounded and wheels 1.000, taper 0.672, peak
decel 1010.3, reverse max exactly at the 700 cap, steer ratio 0.627.
Meaningful changes inside frozen thresholds: braking now friction-capped
through RWD, steer ratio shifted with the trajectory, reverse stronger
within its cap.

## Task 5 regression

All six true: contact 4 of 4, compression 10.0 cm, support 12000 N,
longitudinal 4898 with -11850 and -2595 N, lateral 2629 N at center
fraction 1.000, circle margin exactly 0 N.

## Build result

`GenerateProjectFiles.sh` exit 0. Clean `Build.sh` exit 0.

## Runtime and log result

Exit 0 via `QuitGame`. Log: 0 load errors, 0 ensures, 0 fatals.

## Rendered verification status

`Rendered verification: Blocked by missing Xcode Metal Toolchain component.`
No run spent, nothing manufactured. The `-Task2Shots` path is intact.

## Limitations

Rigid coupling without clutch dynamics. Development-calibrated shift
points and gears for the frozen program. Reverse sizing targets the
frozen reverse window. Frame-delta integration. Nullrhi logic timing
only. Visuals, feel, and PIE unverified.

## Development notes (failing runs before passing)

- Stale-binary episode: one run behaved as new reverse logic with old
  shift points, traced to a build that did not pick up header-only
  config changes. Resolved by rebuilding; the permanent `RACEDRIVE`
  config log line now prints live values every run so recurrence is
  immediately visible.
- Reverse threshold chatter around the engage speed from a discontinuous
  hand-off branch. Removed the branch so reverse force stays continuous
  across standstill.
- Opposition metric lessons from Task 5 applied unchanged.
- Speed-rule reference sized so the frozen steer windows differentiate;
  thresholds frozen throughout.

## Next step

Task 7 specification. Deferred: clutch, manual mode, aero, driver aids,
suspension geometry, advanced slip.
