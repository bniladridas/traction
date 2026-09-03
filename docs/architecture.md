# Architecture: traction vehicle and track systems

Status: Task 7 state. This document describes the RacingGame-owned
architecture as it exists; the template vehicle code is a separate,
untouched neighbor (project defaults reference it; automated runs
override via `?game=`), not a dependency.

## Module layout

```text
game/RacingGame/Source/
├── RacingGame/            # RacingGame-owned module
│   ├── Vehicle/
│   │   ├── RaceVehicle.*          # pawn: collider, visual, camera, input path
│   │   ├── RaceVehicleMovement.*  # force model + suspension + tires + integration
│   │   ├── RaceDrivetrain.*       # engine, transmission, differential
│   │   └── RaceVehicleConfig.*    # FRaceVehicleConfig: single tunable set
│   ├── Track/
│   │   ├── RaceTrack.*            # circuit actor: collision, centerline, checkpoints
│   │   └── RaceTrackConfig.*      # FRaceTrackConfig: single track data set
│   └── Test/
│       ├── RaceTestGameMode.*         # flat-map harness GameMode (URL-selected)
│       ├── RaceTrackTestGameMode.*    # circuit harness GameMode (URL-selected)
│       ├── Task2Probe.*              # Tasks 2/3/5/6 metrics (frozen schemas)
│       └── Task7Probe.*              # Task 7 validation + lap driver
└── TP_VehicleAdv/         # Epic template code and content (retained, see above)
```

A later rename pass may move `RacingGame` module and types to `Traction`
naming. Until then, `traction` is the repository identity and `RacingGame`
remains the Unreal project and code identity.

## Control loop

```text
Keys / probe
  -> ARaceVehicle::ApplyThrottle / ApplyBrake / ApplySteering / ResetVehicle
  -> FRaceDriveCommand (normalized struct, the only input path)
  -> URaceVehicleMovement (drivetrain request + brake + tires + sweep)
  -> transform, speeds, wheel state, camera follow
```

Keyboard bindings and the automated harnesses call the same `Apply*`
functions. During harness runs the probe owns input exclusively
(`DisableInput` on acquire) because the engine input stack re-fires axis
bindings with 0 every frame.

## Movement model (Tasks 3-6)

Force-based longitudinal model (mass 1200 kg, gravity, engine torque via
the drivetrain, service brake, drag) with semi-implicit Euler and sweep
movement. Per-wheel contact traces feed spring-damper suspension; tire
forces split driven/brake requests across contacting wheels and obey a
friction circle per wheel. Speed-sensitive yaw authority. The drivetrain
owns RPM, gears, and torque; it never applies body force directly.
All parameters live in `FRaceVehicleConfig`, consumed via `ApplyConfig`;
the movement owns no tunables.

## Track model (Task 7)

`ARaceTrack` builds road collision, boundary walls, centerline, ordered
checkpoints, and the start pose from `FRaceTrackConfig` at BeginPlay.
The movement never sees track geometry; it meets the road only through
wheel traces. The track GameMode snaps the vehicle to the track-owned
start; spawn transforms are not hard-coded in the vehicle.

## Test environments

- `/Game/Task2/Task2_TestTrack`: 200 m flat floor, PlayerStart, sun.
  Carries the Tasks 2/3/5/6 regression program (frozen).
- `/Game/Track/Track1_TestCircuit`: PlayerStart, sun; the track actor
  builds the ~123 m circuit at runtime. Carries the Task 7 validation
  and lap program.
- Both maps created by `Content/Python/*_create_map.py`, kept
  regenerable. Test GameModes are selected with `?game=` so project
  defaults stay untouched.

## Conventions

- Small C++ classes with one responsibility each.
- Data-driven tuning: `FRaceVehicleConfig` / `FRaceTrackConfig` own
  parameters; logic holds no magic numbers.
- Regression layers are frozen: new tasks add artifacts, never edit
  earlier schemas, thresholds, or phases.
- Verification harnesses are header-marked verification-only.
