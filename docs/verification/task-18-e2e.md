# Task 18 E2E Verification: Cockpit Camera and View Selection

Date: 2026-09-20 (UTC). Engine 5.8.2. Machine M1/8GB, nullrhi standalone.
Implements GitHub issue #65 on branch `feat/cockpit-18`. Tasks 2, 3, 5,
6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17 contracts frozen; Task 18
evidence is separate in `Saved/Task18E2E/results.json`. Scope: two
views and deterministic switching - chase (Task 10, frozen) plus a
rigid cockpit. No vehicle, physics, track, race, results, or HUD
changes; no spring arm, smoothing, look-ahead, roll, head movement,
acceleration feel, or cockpit driver component; no reverse-view,
free-look, menus, settings, pause, or persistence.

## Objective

Player drives the full race in either of two views and switches
deterministically: the chase view (Task 10, unchanged) and a rigid
cockpit view (fixed offset on the vehicle root, configured pitch and
FOV, no spring arm or smoothing). The pawn owns both camera components
and selects the active one. `RaceView` (default C) is bound through the
existing legacy input path; the harness calls the same `CycleView()`.

## Architecture

```text
ARaceCockpitTestGameMode (?game= override, circuit map only)
    grid slot 0 (probe-driven player, Task 15 pattern)
    + 1 AI rival (Task 14 driver, proven stable field)
    ARaceManager (existing race-state authority, unchanged)
    ARaceVehicle (owns ChaseCamera + cockpit CameraComponent; selects
                  the active one via component IsActive)
    URaceChaseCamera (Task 10, frozen - untouched)
    FRaceCameraConfig (additive: DefaultView, CockpitOffset,
                       CockpitPitchDeg, CockpitFov)
    ATask18Probe (drives + recovers player, toggles views, deadlock test)
```

DefaultView defaults to Chase, so every Task 10 flow runs the
unchanged chase camera by default. The second camera is an additive
component on the root; `URaceChaseCamera` update/reset logic is not
restructured.

## Vehicle addition (game code, additive)

- `CockpitCamera`: UCameraComponent on the root, fixed relative offset
  (default 0,0,60 cm), configured pitch and FOV, no spring arm in its
  attachment chain, no pawn control rotation, no absolute rotation.
- `GetViewMode` / `SetViewMode` / `CycleView`: `AActor::CalcCamera`
  selects the first active camera component, so activation flips the
  view. SetActive(false) on the inactive component prevents ambiguity.
- `SetCameraConfig`: re-applies the whole camera block (chase driver,
  cockpit pose, FOV, default view) from a config; harness-facing
  data-driven knob.
- `RaceView` action, default C, bound in `SetupPlayerInputComponent` to
  `OnCycleView` -> `CycleView`.

## E2E program

Apply a non-default cockpit pitch (8 deg) and FOV (72 deg) through
`SetCameraConfig` to prove the values flow from config, run the six
static gates once before racing, start the race, drive the player with
pursuit plus probe-side recovery, toggle views at fixed elapsed
checkpoints during Racing, monitor for AI deadlock, finish both cars.

## Measurements (final passing run, exit 0, 117542 frames)

- `view_configured`: fresh pawn starts DefaultView=Chase with the
  chase camera active and cockpit inactive.
- `view_toggle`: SetViewMode(Cockpit) / CycleView / CycleView move the
  active flag deterministically (cockpit -> chase -> cockpit); the
  `RaceView` action binding is present on the pawn input component.
- `cockpit_rigid`: cockpit parent is the vehicle root; no spring arm in
  the attachment chain; no pawn control rotation; no absolute rotation;
  relative offset equals the configured CockpitOffset (0.00 cm error).
- `pitch_and_fov`: cockpit relative pitch equals configured pitch
  (0.00 deg error) and FOV equals configured FOV (0.00 deg error).
- `reset_preserves_view`: after ResetVehicle and after
  Manager->OnVehicleReset, the selected view and active camera are
  unchanged (mid-setup check in cockpit view).
- `race_compatible`: full 2-lap race; 3 view toggles during Racing all
  switch cleanly, final toggle still flips after both finish, race
  completed with no AI deadlock (max gap 2.3 s).
- No deadlock trip; both cars finished.

## Thresholds (fixed before the first passing run)

Field size 2 (player slot 0 + one AI). RaceLaps 2 (run override via
GameMode only; two-lap struct default untouched). Cockpit pitch 8 deg,
FOV 72 deg, offset 0,0,60 cm (Task 18 config values). Tolerances:
angle 1.0 deg, position 1.0 cm. Timeout 240 s. Stall definition reused
unchanged (50 cm / 20 s). Player pursuit targets reused from Task 15.
Min race toggles 3 at fixed elapsed checkpoints (6 s, 10 s, 15 s).

## Regression

Final binary: all seventeen prior programs green,
`tools/check_regression.py` 100/100 PASS (94 frozen + 6 Task 18).
Frozen schemas and thresholds untouched; `T18_KEYS` +
`Task18Probe.h` + `namespace Task18Limits` wired into CI contract, plus
a CI grep for the `RaceView` action-to-key mapping and the pawn
binding.

## Build result

`Build.sh` exit 0. No new modules, no new dependencies.

## Runtime and log result

Exit 0 via `QuitGame`. Log: 0 load errors, 0 ensures, 0 fatals.

## Rendered verification status

Verified for the capture path (Standing Limitations). No new captures
in this task; nothing manufactured. Cockpit visual framing (blocked
geometry, HUD overlay in-cockpit) unverified in nullrhi; the gates are
logic-only by contract.

## Limitations

Selection logic, attachment, and config propagation verified; rendered
framing and feel unverified (nullrhi, no draw). No smoothing, look
ahead, or banking: those are explicitly out of scope. The cockpit view
is rigid - no head movement, roll, or acceleration effect. Human feel
and PIE unverified. The `RaceView` key event is proven by contract via
the binding grep and the functional `CycleView` path; no synthetic key
injection in the headless run.

## Development notes (failing runs before passing)

None. First Task 18 E2E run passed all six gates (exit 0).

## Next step

Task 19 specification. Deferred: menus/settings/pause, persistence,
AI racecraft, camera expansion (free-look, banking, cockpit feel).