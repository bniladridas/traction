# Testing philosophy: traction

Three verification categories, never mixed:

1. **Functional**: does the behavior happen? Headless standalone runs with
   scripted input through the real gameplay path, transform logs, and
   `results.json` with predefined thresholds. Example: 2251.1 cm forward
   displacement, 89.99 deg steering excursion.
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
