# DESIGN.md: Game Identity and Architecture

## 1. Identity

Realistic circuit racing focused on weight transfer, speed, precision, immersive visuals.
Single-player only for V1. Tone: clean, believable, motorsport, no arcade gimmicks.

## 2. Driving philosophy: Realistic Simcade

- Believable mass, grip, braking. Forgiving at limit.
- Playable on keyboard + gamepad. Wheel optional, not required.
- 60Hz physics tick. Deterministic input → forces → chassis pipeline.

```text
Input (throttle/brake/steer)
  -> FRaceDriveCommand (normalized struct, the only input path)
  -> URaceVehicleMovement (drivetrain request, brake, tires, sweep)
  -> transform, speeds, wheel state, camera follow
```

Implemented: mass, gears, suspension, tire forces, no aero (deferred).
60Hz physics tick is a target; current integration is frame-delta.## 3. V1 Pillars

1. Driving feel first (prototype is ugly on purpose).
2. One track done well (graybox → collision → art).
3. Complete race loop (Ready → Countdown → Racing → Finished, laps,
   positions verified; results screens future).
4. macOS performance is a feature (1080p60 target, quality presets).

## 4. Module architecture (UE5)

As built (Task 12 state). Future dirs are targets, not existing code.

```text
game/RacingGame/Source/RacingGame/
├── Vehicle/   # pawn, movement, drivetrain, config
├── Camera/    # chase driver + config
├── Track/     # centerline, checkpoints, start/finish, grid
├── Race/      # manager, laps, positions, timing
├── AI/        # pursuit driver + recovery (no overtake yet)
├── Test/      # GameModes + E2E probes (verification only)
└── TP_VehicleAdv/ (template base, retained for project defaults)
```

Future: cockpit cameras, racing line, UI (menu, HUD, pause, settings),
Audio (engine RPM/load, tires, impacts, ambient), Save (settings, best
laps, results, versioned). Per the rules below, those dirs are created
only when their systems land.

Rules:
- No cross-module includes except via public interfaces.
- No placeholder systems. Create dirs/interfaces only when needed.
- Every vehicle param data-driven (`FRaceVehicleConfig` struct today; a
  DataAsset storing the same struct remains a possible later step),
  never hardcoded magic.

## 5. Key tuning parameters (expose incrementally)

Implemented: mass, torque curve, gear ratios + final drive, drag,
brake force, suspension stiffness/damping, steering speed sensitivity.
Future: CoG height, downforce/aero, per-axle grip split, brake force
bias.

## 6. Visual direction

PBR + clearcoat car paint, detailed road material, sun + shadows + reflections + post.
Car: paint/metal/glass/rubber/brake discs. Track: road/barriers/terrain/veg/props.
Lighting first, props last. No beauty work before driving feels good.

## 7. Non-goals V1

See README. Explicitly no multiplayer, open world, customization, weather, career.
