# traction

A realistic simcade racing game for macOS.

> One excellent car. One excellent track. One complete racing experience.

Realistic simcade circuit racer for macOS (Apple Silicon), built in Unreal Engine 5 with Metal.

The Unreal project and C++ names keep the established `RacingGame` identity
until a later rename pass; `traction` is the repository and public identity.

## Stack

- Engine: Unreal Engine 5 (C++ + Blueprints)
- Graphics API: Metal
- Modeling: Blender, Textures: Substance 3D Painter
- Audio: UE Audio initially, FMOD later if needed
- VCS: Git + GitHub
- Target: macOS Apple Silicon, 1080p60

## Repo structure

```text
racing-game/
├── README.md
├── DESIGN.md
├── ROADMAP.md
├── AGENTS.md
├── docs/
│   ├── gameplay.md
│   ├── vehicles.md
│   ├── track.md
│   ├── graphics.md
│   ├── audio.md
│   └── release.md
└── game/               # UE5 project lives here (not yet created)
```

`game/` is intentionally empty until UE5 project creation in-editor.

## V1 Scope

In: 1 car, 1 track, simcade handling, chase + cockpit cam, AI opponents,
countdown / laps / checkpoints / positions / timing, main + pause + settings,
engine/tire sounds, basic damage visuals, macOS build.

Out: multiplayer, open world, customization, weather, career, microtransactions.

See `DESIGN.md` and `ROADMAP.md`.

## Workflow with OpenCode

Read `AGENTS.md`. One small verified task at a time. Build after every change.
Never claim a feature works without verification.
