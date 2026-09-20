# Task 17 E2E Verification: Player-Facing Race HUD

Date: 2026-09-20 (UTC). Engine 5.8.2. Machine M1/8GB, nullrhi standalone.
Implements GitHub issue #62 on branch `feat/hud-17`. Tasks 2, 3, 5,
6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 contracts frozen; Task 17
evidence is separate in `Saved/Task17E2E/results.json`. Scope: race
state becomes player-readable (countdown, lap, position, result).
No racecraft, AI, camera, vehicle, physics, or track changes; no
menus, settings, pause, or persistence; no new input bindings (reset
consumes the existing `OnVehicleReset` path).

## Objective

Player can read: countdown seconds, current lap / total laps, live
position / field, the single-line finish result, and a cleared HUD
after the mid-program vehicle reset. All presentation data derives
from `ARaceManager` (sole race-state authority) through a plain data
layer - `URaceHudModel` transforms and reads only, refreshed at 20 Hz
from the widget; the widget itself is a thin UMG shell.

## Architecture

```text
ARaceHudTestGameMode (?game= override, circuit map only)
    grid slot 0 (probe-driven player, Task 15 pattern)
    + 5 tiered-pace AI (Task 14 tiers/lines/grid, proven stable field)
    ARaceManager (sole race-state authority; existing state only)
    FRaceHudConfig   (data-driven formats + 20 Hz update rate)
    URaceHudModel    (presentation-data layer; reads manager getters)
    URaceHudWidget   (UMG shell, polls model at UpdateRateHz)
    ATask17Probe (drives + recovers player, reset, deadlock monitor)
```

New module dependency: `UMG` (with `SlateCore` transitively) in
`RacingGame.Build.cs`. No new input bindings - the reset test uses the
existing `OnVehicleReset` consumed before re-entering the countdown.

## Manager addition (game code, additive)

`ARaceManager::GetCountdownRemaining()` returns
`countdown duration - elapsed phase time` while in the Countdown phase
(0 otherwise). Phase-derived only - no second countdown timer; the HUD
sampler `Refresh()`/`GetCountdownSeconds()` reads this and `GetPhase()`.

## E2E program

Start, drive player with pursuit plus probe-side recovery, race 5 AI,
reset at 12 s with grid re-staging, restart, concurrent race until all
six finish 3 laps, gates recorded.

## Measurements (final passing run, exit 0, 298711 frames)

- `hud_bound`: model bound to manager; widget's model is that instance;
  countdown element enabled in config.
- `hud_countdown_shown`: model countdown seconds equal the manager's
  phase-derived remaining (ceil) while showing; cleared to 0 and empty
  text at Racing.
- `hud_lap_display`: during Racing, model lap == participant-0 laps,
  total == 3, text `Lap {l}/{3}` per config template.
- `hud_position_display`: during Racing, model position == manager
  `GetPosition(player)` (live), field == 6, text `{pos}/{field}`.
- `hud_finish_display`: after all finished, model `HasFinish()`, text
  `Finished {pos} of {field}` - run log `'Finished 6 of 6'`, position
  6/6 matches the live manager value for participant 0 at completion.
- `hud_clears_on_reset`: after `OnVehicleReset` + model refresh, lap 0,
  countdown 0/empty, no finish, finish text empty.
- No deadlock trip (max gap 3.0 s, all six finished 3 laps).

## Thresholds (fixed before the final passing run)

RaceLaps 3 (run override via GameMode only; two-lap struct default
untouched). Reset at 12 s. Timeout 400 s. Stall definition reused
unchanged (50 cm / 20 s). Player pursuit targets reused from Task 15.
Lap/position binding sampled across 285416 racing frames (min 5).

## Regression

Final binary: all sixteen prior programs green,
`tools/check_regression.py` 94/94 PASS (88 frozen + 6 Task 17).
Frozen schemas and thresholds untouched; `T17_KEYS` +
`Task17Probe.h` + `namespace Task17Limits` wired into CI contract.

## Build result

`Build.sh` exit 0. New dependency: UMG. No new module paths.

## Runtime and log result

Exit 0 via `QuitGame`. Log: 0 load errors, 0 ensures, 0 fatals.

## Rendered verification status

Verified for the capture path (Standing Limitations). No new captures
in this task; nothing manufactured. HUD visual quality unverified
(deterministic gates are logic-only, by contract).

## Limitations

Presentation-data and polling logic verified; widget pixel output and
layout unverified in nullrhi (UMG created, bound, and ticked headless,
no draw). No menus/settings/pause/persistence. Human feel and PIE
unverified. Finish text shows the participant-0 (player) result only.

## Development notes (failing runs before passing)

- Run 1: `hud_lap_display`/`hud_finish_display` false at timeout -
  player was AI-driven at pace 1.15 (unproven tier; field max 1.05),
  over-drove into an off-track recovery loop and never completed a lap,
  so player-0 laps stayed 0. Switched the player to the proven Task 15
  probe-driven pursuit pattern; thresholds frozen throughout.
- Run 2: passing (above).

## Next step

Task 18 specification. Deferred: menus/settings/pause, persistence,
AI racecraft, camera expansion.