# Task 5 E2E Verification: Wheel Contact, Suspension, and Tire Forces

Date: 2026-09-04 (UTC). Engine 5.8.2. Machine M1/8GB, nullrhi standalone.
Implements GitHub issue #3 on branch `feat/task-5-wheel-tire-suspension`.
Task 2 and Task 3 contracts frozen; Task 5 evidence is separate in
`Saved/Task5E2E/results.json`. No drivetrain, gears, aero, AI, or UI.

## Objective

First real wheel behavior: per-wheel contact state, spring-damper
suspension, and bounded longitudinal/lateral tire forces on a friction
circle, with braking, reverse, and steering acting through the tire path.
Drivetrain and transmission stay out.

## Architecture

Unchanged Task 4 boundaries. `URaceVehicleMovement` gains wheel state,
suspension, and tire force computation internally; no new gameplay
components. Configuration extends `FRaceVehicleConfig`; commands, camera,
and state boundaries unchanged.

## Wheel model

Runtime `FRaceWheelState` per wheel: contact flag, point, normal,
compression (cm) and velocity, normal load (N), longitudinal and lateral
forces (N, wheel frame), commanded longitudinal input (N), lateral slip
velocity (m/s). Read-only to verification. No wheel angular inertia: the
rigidly-driven approximation is documented, not hidden.

## Contact model

One downward trace per configured wheel per tick (shared 50 cm length).
Records grounded flag, point, and normal. Positions come from
configuration, never hard-coded. Flat-track validity (normal Z over 0.9)
is a measured check, not an assumption in code.

## Suspension model

Per wheel: length from hardpoint to contact along the normal, compression
against the 20 cm rest length clamped to 12 cm compression and 8 cm
extension, finite-difference compression velocity, spring force from
30000 N/m stiffness plus 3000 N s/m damping, clamped at zero (no
tension). Supports the 1200 kg car at about 10 cm static compression.
No geometry, no anti-roll, no dampers beyond the single coefficient.

## Tire model

Longitudinal request per grounded wheel (drive, brake, or reverse share
from the Task 3 command logic), lateral force as negative stiffness
(8000 N per m/s) times wheel-frame lateral slip including the yaw lever
arm. Front wheels rotate the force frame by up to 30 deg geometric steer
angle; speed sensitivity stays in the yaw rule. Friction circle per
wheel (mu 1.0 times normal load) scales an over-limit pair. No Pacejka,
no temperature, no wear, no ABS or traction control.

## Configuration

Suspension: rest 20 cm, compression 12 cm, extension 8 cm, stiffness
30000 N/m, damping 3000 N s/m. Tire: mu 1.0, lateral stiffness 8000,
max steer 30 deg. Reverse force sized at 6000 N (short-gear pull, still
capped by the 700 cm/s limit and the friction circle). All values in
`FRaceVehicleConfig`; the movement owns none.

## Units

Centimeters, seconds, kilograms, newtons. Lengths convert to meters for
force math. Body frame X forward, Y right, Z up. Positive steering turns
front wheels toward +Y.

## Force integration

Semi-implicit Euler at frame delta: tire sums drive planar velocity,
gravity drives vertical velocity, yaw follows the unchanged Task 3
authority rule, body-frame velocity rotates by the yaw step, one sweep
move. No fixed substeps claimed.

## Test sequence

Frozen Task 2 program (0 to 11 s) plus the Task 3 low-speed window (to
13.5 s). Task 5 windows reuse existing phases: rest (0 to 0.5 s),
acceleration (0.5 to 3.0 s), braking (3.0 to 4.0 s), reverse (4.5 to
6.5 s), steering (7.5 to 10.0 s). No timeline change was needed.

## Fixed thresholds (set before the first passing run)

Contact valid on every rest sample. Rest compression mean over 0.5 cm
within 12 cm travel with range under 2.0 cm. Total support within 15
percent of weight with every grounded wheel loaded. Longitudinal means
over 1000 N in acceleration, opposing under -1000 N in braking, under
-500 N in reverse. Lateral mean over 200 N toward the turn center in at
least 80 percent of turning samples. No wheel over mu times N by more
than 50 N at any sample.

## Measurements (final passing run, exit 0)

Contact 4 of 4 valid. Compression mean 10.0 cm, range 0.0. Support
12000 N against 11760 N weight (2 percent over). Longitudinal means
8270, -11850, and -2269 N. Lateral mean 5829 N with turn-center fraction
1.000. Circle minimum margin exactly 0 N (saturated, never exceeded).

## Task 2 regression

All six true: forward 2143.4 cm, brake 0.384, reverse 218.8 cm backward,
steer 67.3 deg, camera 5249.6 against 5076.2 cm, reset 0.0 with 0.0 deg
and 0.0 cm/s. Thresholds, phases, and JSON shape frozen.

## Task 3 regression

All six true: floor 40.0, grounded and wheels 1.000, taper 0.839, peak
decel 1015.0, reverse max 620.5, steer ratio 0.621. Meaningful changes
from the Task 3 baseline, all inside frozen thresholds: braking now
friction-capped (1194 to 1015), steer ratio shifted by the new entry
state (0.381 to 0.621), reverse displacement grew (149.3 to 218.8) from
the stronger reverse pull. Frame timing alone is never described as a
behavioral change; these are model consequences, recorded here.

## Build result

`GenerateProjectFiles.sh` exit 0. Clean `Build.sh` exit 0.

## Runtime and log result

Exit 0 via `QuitGame`, 26042 frames. Log: 0 load errors, 0 ensures,
0 fatals.

## Rendered verification status

`Rendered verification: Blocked by missing Xcode Metal Toolchain component.`
No run spent, no screenshots manufactured. The `-Task2Shots` path is
intact.

## Limitations

No wheel angular inertia (rigidly-driven approximation). No load transfer
beyond static plus dynamic compression (body neither pitches nor rolls).
No tire curves beyond linear stiffness with circle cap. Frame-delta
integration. Nullrhi logic timing only. Visuals, feel, and PIE unverified.

## Next step

Task 6 specification. Deferred: drivetrain, gearbox, aero, suspension
geometry, advanced slip.
