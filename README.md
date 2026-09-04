# traction

A realistic simcade racing game for macOS.

> One excellent car. One excellent track. One complete racing experience.

Realistic simcade circuit racer for macOS (Apple Silicon), built in Unreal Engine 5 with Metal.

The Unreal project and C++ names keep the established `RacingGame` identity
until a later rename pass; `traction` is the repository and public identity.

## Meaning

**Traction** is the force that moves a vehicle forward. For this project
the name also describes the engineering idea: movement, contact,
drivetrain, track, and race state developed as connected parts rather
than isolated effects.

## Potential

The scope is deliberately small: one car, one track, single-player,
macOS. The architecture leaves room to grow toward a deeper simcade
experience (vehicle dynamics, more tracks and vehicles, AI, audio,
damage, race systems) without promising any of it. The potential is the
direction the architecture supports, not a feature list.

## Stack

- Engine: Unreal Engine 5 (C++ + Blueprints)
- Graphics API: Metal
- Modeling: Blender, Textures: Substance 3D Painter
- Audio: UE Audio initially, FMOD later if needed
- VCS: Git + GitHub
- Target: macOS Apple Silicon, 1080p60

## Repo structure

```text
traction/
├── README.md
├── DESIGN.md
├── ROADMAP.md
├── AGENTS.md
├── docs/
│   ├── architecture.md      # current system map (Task 7 state)
│   ├── testing.md           # verification philosophy + regression layers
│   ├── vehicles.md          # V1 car: custom movement, config, drivetrain
│   ├── track.md             # V1 circuit: config, collision, checkpoints
│   ├── gameplay.md
│   ├── graphics.md
│   ├── audio.md
│   ├── release.md
│   └── verification/        # per-task E2E evidence (tasks 1-7)
├── site/                    # static project site (GitHub Pages)
└── game/                    # UE5 project (RacingGame module + template base)
    └── RacingGame/
        ├── Source/RacingGame/
        │   ├── Vehicle/     # pawn, movement, drivetrain, config
        │   ├── Track/       # track actor + config
        │   └── Test/        # GameModes + E2E probes (verification only)
        └── Content/
            ├── Task2/       # flat prototype map
            ├── Track/       # first circuit map
            └── Python/      # headless map-generation scripts
```

Template content (`Source/TP_VehicleAdv/`, `Content/VehicleTemplate/`,
`Content/Variant_*`) is retained: project defaults (startup map, default
map, global GameMode) reference it. Automated runs override via `?game=`.

## V1 Scope

In: 1 car, 1 track, simcade handling, chase + cockpit cam, AI opponents,
countdown / laps / checkpoints / positions / timing, main + pause + settings,
engine/tire sounds, basic damage visuals, macOS build.

Out: multiplayer, open world, customization, weather, career, microtransactions.

See `DESIGN.md` and `ROADMAP.md`.

## Workflow with OpenCode

Read `AGENTS.md`. One small verified task at a time. Build after every change.
Never claim a feature works without verification.
