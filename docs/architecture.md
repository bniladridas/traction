# Architecture: traction vehicle systems

Status: Task 2 prototype. This document describes the RacingGame-owned
architecture as it exists; the template vehicle code is a separate,
untouched neighbor, not a dependency.

## Module layout

```text
game/RacingGame/Source/
├── RacingGame/            # RacingGame-owned module (Task 2+)
│   ├── Vehicle/
│   │   ├── RaceVehicle.*          # pawn: collider, cube visual, camera, input path
│   │   └── RaceVehicleMovement.*  # drive model: inputs in, transform out
│   └── Test/
│       ├── RaceTestGameMode.*     # spawns ARaceVehicle (URL-selected)
│       └── Task2Probe.*           # temporary E2E harness (verification only)
└── TP_VehicleAdv/         # Epic template code and content (reference only)
```

A later rename pass may move `RacingGame` module and types to `Traction`
naming. Until then, `traction` is the repository identity and `RacingGame`
remains the Unreal project and code identity.

## Control loop

```text
Keys / probe
  -> ARaceVehicle::ApplyThrottle / ApplyBrake / ApplySteering / ResetVehicle
  -> URaceVehicleMovement inputs (throttle, brake, steering)
  -> Tick integration with sweep
  -> transform, speed, camera follow
```

Keyboard bindings and the automated harness call the same `Apply*`
functions. During harness runs the probe owns input exclusively
(`DisableInput` on acquire) because the engine input stack re-fires axis
bindings with 0 every frame.

## Movement model (Task 2)

Scalar signed forward speed with brake-to-stop, reverse engagement near
standstill, speed-scaled yaw (mirrored in reverse), rolling drag, and
sweep movement on a flat surface. All rates are data-driven properties on
the movement component. No gravity yet; motion is planar by design.

Task 3 extends this component with engine torque, gears, and tires while
keeping the input API unchanged.

## Test environment

`/Game/Task2/Task2_TestTrack`: 200 m flat floor, one PlayerStart, one
directional light. Created by `Content/Python/task2_create_map.py` and
kept regenerable. The test GameMode is selected with `?game=` so project
defaults stay untouched.

## Conventions

- Small C++ classes with one responsibility each.
- No speculative abstraction; Task 3 must fit without another rewrite.
- Verification harnesses are temporary, header-marked, and deleted when
  their architecture is replaced.
