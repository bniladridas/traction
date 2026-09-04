# Testing philosophy: traction

Three verification categories, never mixed:

1. **Functional**: does the behavior happen? Headless standalone runs with
   scripted input through the real gameplay path, transform logs, and
   `results.json` with predefined thresholds. Example: 1238.3 cm forward
   displacement with a 0.006 brake ratio on the Task 6 baseline.

## Regression layers (frozen, cumulative)

Each layer keeps its own artifact, schema, thresholds, and phases. New
tasks add layers; none edit earlier ones.

- Task 2 (`Saved/Task2E2E/`): forward, brake, reverse, steer, camera,
  reset. Six flags, frozen program on the flat map.
- Task 3 (same artifact, `pass_task3_*`): gravity/contact, mass/taper,
  brake force, reverse bound, steer rule, wheels.
- Task 5 (`Saved/Task5E2E/`): contact, suspension, load, longitudinal,
  lateral, friction circle.
- Task 6 (`Saved/Task6E2E/`): RPM bounds, gear progression, reverse
  drive, torque transfer, engine braking.
- Task 7 (`Saved/Task7E2E/`): track load, road contact, start
  alignment, centerline validity, checkpoint order, lap traversal on
  the circuit map.

CI (`test.yml`) validates repository invariants on GitHub-hosted runners:
verification docs, frozen threshold namespaces, headers, tooling syntax,
and no pending-runner references. It never builds or runs the game.

## Verification policy

No self-hosted runner. GitHub-hosted CI checks repository correctness;
the local Apple Silicon Mac verifies the Unreal application:

```text
GitHub PR -> GitHub-hosted CI (lint, docs, contract checks)
Local M1 Mac -> UE build, headless E2E, logs, rendered checks
```

Unreal runtime verification is performed locally on the development Mac.
Task reports record this explicitly rather than implying CI ran the game.

## Local verification procedure (M1 Mac)

Build once:

```text
GenerateProjectFiles.sh -project="game/RacingGame/RacingGame.uproject" -game -engine
Build.sh TP_VehicleAdvEditor Mac Development -project="game/RacingGame/RacingGame.uproject"
```

Run all three E2E programs (flat map, circuit map, race-state map),
then require every flag true across all six artifacts (30 frozen
regression flags plus 10 Task 8 gates):

```text
python3 tools/check_regression.py
```
2. **Rendered**: was a real frame produced? Only from the Unreal renderer
   (Metal), never synthetic images. Screenshots prove frames were produced,
   not that the game looks good.
3. **Human**: does it feel and look right? Requires interactive display,
   mouse, or keyboard access, which this environment does not provide.

## Rules

- Thresholds are fixed in code before the first passing run and recorded
  in the results file. Tolerances are never invented after seeing a pass.
- NullRHI measurements are labeled simulation and logic measurements.
  Never present a headless tick rate as FPS.
- A missing capability is reported as Blocked with its exact cause, never
  worked around with fabricated evidence.
- Failing runs are diagnostic material: each must point at the harness,
  the program, or the vehicle, and each fix is verified by the next run.
- Compilation alone never completes a task; only a clean E2E run does.
- Reports live in `docs/verification/`; the chronological record lives in
  the scholarship Gist. Both distinguish Verified, Observed, Not yet
  verified, and Blocked.
