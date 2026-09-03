# Task 2 E2E Verification: Minimal Driving Prototype

Date: 2026-09-04 (UTC). Engine 5.8.2. Machine M1/8GB, nullrhi standalone.
Map: `/Game/Task2/Task2_TestTrack` (200 m flat floor, PlayerStart, sun).
Harness: temporary `ATask2Probe` via `?game=/Script/RacingGame.RaceTestGameMode`.

## Objective

Establish a RacingGame-owned control loop (accelerate, brake, reverse,
steer, chase camera, reset) with measured, headless-testable behavior
before any realism work. No suspension, torque curves, gears, tires, aero,
AI, rules, UI, or audio in this task.

## Architecture implemented

New `RacingGame` game module (`Source/RacingGame/`, registered in
`RacingGame.uproject` and both target files):

- `ARaceVehicle : APawn` (Vehicle/): box collider root, cube visual
  (`/Engine/BasicShapes/Cube`), spring-arm chase camera. Owns the public
  input path `ApplyThrottle` / `ApplyBrake` / `ApplySteering` /
  `ResetVehicle`, bound to `RaceThrottle` / `RaceSteer` / `RaceReset`
  (legacy bindings in `DefaultInput.ini`; Enhanced Input migration is
  future work and keeps this API).
- `URaceVehicleMovement : UPawnMovementComponent` (Vehicle/): scalar
  signed speed, brake-to-stop with reverse engagement near standstill,
  speed-scaled yaw with reverse mirroring, rolling drag, sweep movement.
  All rates data-driven properties (Task 3 extends this model).
- `ARaceTestGameMode` (Test/): spawns `ARaceVehicle`; selected by URL
  override so project defaults stay untouched.
- `ATask2Probe` (Test/, marked temporary): the E2E driver described below.

Deleted after verification: the Task 1 temporary harness
(`Source/TP_VehicleAdv/Task1E2E/`, 4 files). Template vehicle code itself
is untouched; nothing in Task 2 depends on `TP_VehicleAdv`.

## Test sequence (fixed program, about 11 s)

0 to 0.5 s settle (no inputs, record initial transform); 0.5 to 3.0 full
throttle; 3.0 to 4.0 full brake; 4.0 to 6.0 brake held (reverse); 6.0 to
6.5 settle; 6.5 to 7.5 regain speed; 7.5 to 10.0 throttle plus steering;
10.0 reset through the R-key function; 10.5 measure; 11.0 write
`Saved/Task2E2E/results.json` and quit. Samples at 4 Hz (`TASK2E2E:`).

## Predefined thresholds (fixed before the first passing run)

Forward displacement over 50 cm within 10 deg of initial facing. Brake
entry speed over 500 cm/s with exit under half. Reverse over 50 cm opposite
facing. Straight-run yaw noise under 2 deg (validity) with steer-phase yaw
over 5 deg. Camera travel over half of pawn travel with attachment rooted
at the pawn. Reset errors under 1.0 cm, 0.5 deg, 1.0 cm/s.

## Measurements (final passing run, exit 0)

Forward 2251.1 cm at 0.0 deg, terminal 1800.3 cm/s. Straight noise 0.0 deg.
Brake 1800.3 to 221.1 (ratio 0.123). Reverse 863.4 cm backward. Steering
yaw delta 89.99 deg. Camera chain `ChaseCamera <- CameraArm`, travel
5578.8 cm against pawn travel 5355.0 cm. Reset errors 0.0 cm, 0.0 deg,
0.0 cm/s. 21976 frames; avg 1997.4 fps is the nullrhi logic tick rate,
not render performance.

## Verification table

| Check | Result | Evidence |
|---|---|---|
| Project builds (new module) | Verified | `Build.sh`, exit 0 |
| Test map exists and loads | Verified | `Task2_TestTrack.umap`, `LoadMap` in log |
| Acceleration | Verified | 2251.1 cm, 0.0 deg, thresholds above |
| Braking | Verified | ratio 0.123 (limit 0.50) |
| Reverse | Verified | 863.4 cm backward (limit 50) |
| Steering | Verified | 89.99 deg (limit 5; control noise 0.0) |
| Camera follow | Verified | chain plus travel ratio about 1.04 |
| Reset | Verified | 0.0 / 0.0 / 0.0 against 1.0 / 0.5 / 1.0 limits |
| Stability | Verified | exit 0, 0 load errors, 0 ensures, 0 fatals |
| Render performance | Not yet verified | nullrhi only; Metal toolchain still missing |
| Visual quality, driving feel | Not yet verified | require display access |
| Keyboard feel in PIE | Not yet verified | bindings present and code-reviewed; harness calls the same functions |

## Build and runtime results

- `GenerateProjectFiles.sh`: exit 0. Final clean `Build.sh`: exit 0.
- E2E: exit 0 via `QuitGame`; `results.json` holds all six pass flags true.

## Issues and limitations

- Automated input contention: the engine input stack re-fires axis
  bindings with 0 every frame, overwriting harness writes made earlier in
  the frame. The probe now takes exclusive input ownership
  (`DisableInput` on acquire, `EnableInput` on finish). Keyboard behavior
  is unchanged outside harness runs. (Observed through timestamped call
  evidence, fixed harness-side.)
- Two probe metric defects found by failing runs and fixed before passing:
  driving during the settle window biased the reset reference (fixed with
  an explicit no-input settle), and camera travel measured across the
  reset point read near zero (fixed by freezing metrics at steer end).
- The cube hovers at spawn height: the prototype model has no gravity yet.
  Deliberately deferred; motion is planar and deterministic as specified.
- `Task2_TestTrack.umap` was created by headless editor Python
  (`Content/Python/task2_create_map.py`); the creation script is kept for
  regeneration.
- Rendered verification remains blocked on the missing Xcode Metal
  Toolchain component (unchanged from Milestone 2).

## Rendered Evidence

Capture infrastructure is implemented in the probe and compiled: with the
`-Task2Shots` flag it requests five deterministic frames from the real
renderer (`01-spawn.png` through `05-final.png` under
`Saved/Task2E2E/screenshots/`) with per-frame phase, time, and vehicle and
camera transforms recorded to `screenshots/manifest.json`. Images are never
synthesized. A nullrhi regression run writes the manifest with zero
captures, as expected.

| Frame | Status | Evidence |
|---|---|---|
| Spawn | Blocked | Metal toolchain missing; no rendered run performed |
| Acceleration | Blocked | same as above |
| Steering | Blocked | same as above |
| Reset | Blocked | same as above |
| Final | Blocked | same as above |

Screenshots establish that rendered frames were produced, never visual
quality. No rendered run was spent re-proving the known toolchain block;
the code path activates automatically once the toolchain is installed.

## Next step

Task 3: engine/torque/gear model on top of `URaceVehicleMovement`,
keeping the input API and the harness pattern established here.
