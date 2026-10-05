# Task 10 E2E Invocation (Provenance Record)

This file records the exact headless invocation that produced the frozen
Task 10 evidence. It is provenance, not a test contract: thresholds, gates,
and the artifact schema live in `docs/verification/task-10-e2e.md` and
`game/RacingGame/Source/RacingGame/Test/Task10Probe.h`. No new thresholds.

Recorded 2026-10-04. Engine 5.8.2 (`/Users/Shared/UE_5.8`), Apple Silicon,
macOS, nullrhi standalone.

## Command

Executable: `Engine/Binaries/Mac/UnrealEditor` (from the UE 5.8.2 install).

Project: `game/RacingGame/RacingGame.uproject`, workspace-relative.

Map and game mode override:

```text
/Game/Track/Track1_TestCircuit?game=/Script/RacingGame.RaceCameraTestGameMode
```

Flags:

```text
-game -nullrhi -unattended -nosound -nopause -nosplash -log -stdout -FullStdOutLogOutput
```

## Four conditions

The only per-condition change is the throttle, via `-ExecCmds`:

- uncapped: no `-ExecCmds` argument
- 30 FPS: `-ExecCmds="t.MaxFPS 30"`
- 10 FPS: `-ExecCmds="t.MaxFPS 10"`
- normal: no `-ExecCmds` argument

## Artifact

Each run writes `game/RacingGame/Saved/Task10E2E/results.json` (gitignored).
The probe runs about 42 s (reset at 20 s, measure to 38 s, finish at 42 s)
and does not self-exit; the original driver polled for the artifact file and
then killed the process.

## Why this file exists

The frozen invocation was not recorded in the repository and had to be
recovered from local session history after an artifact-loss incident. A
reconstructed invocation using `VehicleBasic` via `UnrealEditor-Cmd`
produced valid-looking but non-equivalent evidence (wrong circuit geometry)
and was discarded. This note exists so future verification reuses the command
that produced the frozen numbers instead of reconstructing one from memory.
