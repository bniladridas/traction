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
  -> VehiclePhysics (engine, trans, aero, suspension)
  -> Wheel forces (grip/slip/brake)
  -> Chassis movement
  -> Wheel visuals + audio + camera
```

## 3. V1 Pillars

1. Driving feel first (prototype is ugly on purpose).
2. One track done well (graybox → collision → art).
3. Complete race loop (countdown → laps → positions → finish → results).
4. macOS performance is a feature (1080p60 target, quality presets).

## 4. Module architecture (UE5)

```text
game/Source/RacingGame/
├── Vehicle/   # controller, physics, engine, transmission, aero
├── Wheels/    # suspension, tire grip/slip, brakes, steering
├── Track/     # spline, checkpoints, start/finish, racing line
├── Race/      # manager, laps, positions, timing
├── AI/        # spline follow, speed/brake control, overtake
├── Camera/    # chase, cockpit
├── UI/        # menu, HUD, pause, settings
├── Audio/     # engine (RPM/load), tires, impacts, ambient
└── Save/      # settings, best laps, results (versioned)
```

Rules:
- No cross-module includes except via public interfaces.
- No placeholder systems. Create dirs/interfaces only when needed.
- Every vehicle param data-driven (DataAsset / Curve), never hardcoded magic.

## 5. Key tuning parameters (expose incrementally)

mass, CoG height, torque/HP curve, gear ratios + final drive,
drag, downforce, tire grip front/rear, brake force bias,
suspension stiffness/damping, steering ratio/speed sensitivity.

## 6. Visual direction

PBR + clearcoat car paint, detailed road material, sun + shadows + reflections + post.
Car: paint/metal/glass/rubber/brake discs. Track: road/barriers/terrain/veg/props.
Lighting first, props last. No beauty work before driving feels good.

## 7. Non-goals V1

See README. Explicitly no multiplayer, open world, customization, weather, career.
