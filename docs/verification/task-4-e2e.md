# Task 4 E2E Verification: Vehicle Architecture and Configuration

Date: 2026-09-04 (UTC). Engine 5.8.2. Machine M1/8GB, nullrhi standalone.
Implements GitHub issue #1 on branch `refactor/task-4-vehicle-architecture`.
Task 2 and Task 3 contracts frozen: same phases, thresholds, JSON shape,
probe control path, and pass semantics.

## Objective

Turn the prototype into a reusable, data-driven vehicle architecture while
preserving the verified Task 3 movement model. Architectural stability, not
driving features. No suspension, tires, gears, aero, AI, rules, UI, or
audio.

## Architecture

```text
ARaceVehicle
    |
    +-- URaceVehicleMovement (consumes config and commands, integrates)
    +-- FRaceVehicleConfig (single authoritative tunable set, per instance)
    +-- FRaceDriveCommand (normalized throttle, brake, steering)
    +-- input bindings (keys to commands; movement never sees keys)
    +-- chase camera (consumes transform only)
    +-- read-only accessors (config, state, wheels) for verification
```

Responsibility boundaries: the actor owns configuration, input mapping,
camera, and reset placement. The movement owns forces, integration,
contact, and state. The probe observes state and never implements
gameplay. The movement exposes no key handling and no camera behavior.

## Configuration structure

`FRaceVehicleConfig` (USTRUCT, `Vehicle/RaceVehicleConfig.h`): mass,
gravity, engine force points, brake force, reverse force and cap, max
speed, steering low rate with reference and dead speeds, rolling and aero
drag, reverse engage speed, wheel trace length, and four `FRaceWheelConfig`
entries. The actor holds one `EditAnywhere` instance; the movement keeps a
working copy applied through `ApplyConfig`. No tunable lives anywhere
else. A future variant assigns different values, not new classes. A later
pass may store the struct in a data asset without changing consumers.

## Complete parameter table

Mass 1200 kg. Gravity 980 cm/s^2. Engine points (cm/s to N): (0, 9000),
(1000, 8200), (2000, 6400), (3000, 3800), (4000, 1000), (4500, 0).
Brake 14000 N. Reverse 3000 N with 700 cm/s cap. Max speed 5000 cm/s.
Steer 110 deg/s low, 1200 cm/s reference, 100 cm/s dead. Rolling 250 N,
aero 0.35 N per (m/s)^2. Reverse engage 60 cm/s. Trace length 50 cm. All
migrated unchanged from Task 3 values.

## Wheel and contact table

| Name | Local offset (cm) | Axle | Side |
|---|---|---|---|
| FL | (70, 75, -30) | front | left |
| FR | (70, -75, -30) | front | right |
| RL | (-70, 75, -30) | rear | left |
| RR | (-70, -75, -30) | rear | right |

Roles are data consumed by the trace loop, not code branches. The four
traced contacts function as in Task 3.

## Input boundary

Keys bind to `ARaceVehicle::OnThrottleAxis`, `OnSteerAxis`, and
`RaceReset`, which write a pending `FRaceDriveCommand` pushed whole to
the movement. The harness calls the same public `Apply*` functions.
Reset remains a direct call. No Enhanced Input added; the API is unchanged
for a later migration.

## Camera boundary

Spring arm and camera stay actor components consuming the transform only.
No movement code references the camera. The Task 2 and 3 camera checks
pass unchanged.

## State interface

Read-only: position, rotation, forward and vertical speed, grounded flag,
per-wheel contact, pawn and active configuration. No mutable physics state
is exposed. The probe asserts configuration equality between pawn and
movement copies.

## Variant strategy

`ARaceVehicle` plus a different `FRaceVehicleConfig` value. Demonstrated
structurally: per-instance editable config with pawn-to-movement
application verified equal at runtime. No second vehicle built, per scope.

## Test sequence

Unchanged Task 2 program (11 s) plus the documented Task 3 low-speed
window (to 13.5 s). Separate artifact `Saved/Task4E2E/results.json` holds
the configuration audit and regression flag copies; the Task 2 and 3
schema is untouched.

## Regression results (final run, exit 0)

Task 2 all six true; Task 3 all six true. Baseline comparison against the
Task 3 passing run: forward 2142.6 to 2143.0 cm, brake ratio 0.276 both,
reverse 149.3 to 149.4 cm, steer 46.11 to 46.07 deg, reset 0.125 cm both,
taper 0.839 to 0.836, decel 1194.0 both, steer ratio 0.381 to 0.380.
Differences are frame-timing noise (hundredths of a percent), not tuning:
no threshold, constant, or curve value changed.

## Build result

`GenerateProjectFiles.sh` exit 0. Clean `Build.sh` exit 0.

## Runtime and log result

Exit 0 via `QuitGame`, 24992 frames. Log: 0 load errors, 0 ensures,
0 fatals.

## Rendered verification status

`Rendered verification: Blocked by missing Xcode Metal Toolchain component.`
Capture infrastructure intact; no run spent. No screenshots manufactured.

## Limitations

Frame-delta integration (no fixed substeps); nullrhi logic timing only;
visuals, feel, and PIE unverified; variant capability structural only.

## Next step

Task 5 specification. Deferred: suspension, tire friction, drivetrain,
gears, aero.
