# Task 7 E2E Verification: First Track and Track Architecture

Date: 2026-09-04 (UTC). Engine 5.8.2. Machine M1/8GB, nullrhi standalone.
Implements GitHub issue #7 on branch `feat/task-7-track`. Tasks 2, 3, 5,
and 6 contracts frozen; Task 7 evidence is separate in
`Saved/Task7E2E/results.json`. No AI, HUD, rules, audio, or art pass.

## Objective

First drivable closed circuit around the untouched vehicle/physics stack:
data-driven config, road collision, boundaries, centerline, checkpoints,
start/finish, track-owned spawn, deterministic validation, and a complete
lap. The vehicle remains the authority for drivetrain, suspension,
contact, tire forces, and motion.

## Architecture

```text
ARaceTrackTestGameMode (?game= override, track map only)
    ARaceTrack (spawns itself when the map has none)
        road collision + walls + visuals from FRaceTrackConfig
        centerline + checkpoints + start pose (derived, not stored)
    ARaceVehicle (snapped to the track-owned start pose)
    ATask7Probe (validation + deterministic lap driver)
```

The movement component never sees track geometry; it meets road collision
only through wheel traces. `RaceTestGameMode`, `Task2Probe`, and the
Task 2 map flow are untouched.

## Configuration

`FRaceTrackConfig`: name TestCircuit1, width 800 cm, road top Z 0,
walls 120 high and 100 thick, 20-point closed control polygon (main
straight, east sweeper r800, back straight, west hairpin r700),
250 cm subdivision target, 8 checkpoints, start line at s=500,
spawn 100 behind the line. Length, points, checkpoints, and start pose
derive from the polygon at build; no duplicate constants exist.

## Geometry

Loop approx 123 m: main straight (3840 cm), east sweeper
(r800, fast), back straight (~3810 cm), west hairpin (r700, braking
corner), with a start/finish straight and direction changes throughout.
Large enough for acceleration, braking, steering, suspension, tire,
drivetrain, and gear exercise; simple enough to diagnose.

## Road and boundaries

64 road boxes (tops exactly Z 0) with capped joint extensions, 128 wall
boxes on both edges, plain cube visuals matching collision. No damage.

## Centerline and checkpoints

65 ordered points with position, forward, and distance; closure gap
0.00 cm. Eight checkpoints evenly spaced by distance, index 0 on the
start line, contiguous and deterministic. Infrastructure for future AI,
timing, and cameras; no behavior yet.

## Start and spawn

Track-owned: the GameMode snaps the vehicle to the config-derived pose
(position error 20.0 cm, mostly the vertical settle drop; heading error
0.00 deg). No countdown, no UI. The map `Track1_TestCircuit` holds only
PlayerStart (approximate; snapped over at runtime) and a sun; the track
actor builds everything else.

## Measurements (final passing run, exit 0, 36537 frames)

Segments 64, points 65, checkpoints 8, length 12337.9 cm, width 800 cm,
closure 0.00 cm, contact fraction 1.000, start errors 20.0 cm / 0.00 deg,
sequence [0,1,2,3,4,5,6,7], lap distance 10765.4 cm (12.7 percent under
the centerline from corner cutting, inside the 15 percent bound), lap
completed in ~16 s of driving.

## Thresholds (fixed before the final passing run)

Segments at least 20. Contact fraction 1.0. Start errors under 50 cm and
5 deg. Points at least 24, closure under 5 cm, length over 5000 cm.
Sequence exactly 0..7. Lap distance within 15 percent of the centerline.

## Regression

Separate old-map run on the final binary, all green: Task 2 six true,
Task 3 six true, Task 5 six true, Task 6 six true. Frozen schemas and
thresholds untouched.

## Build result

`Build.sh` exit 0 (plus `RacingGame/Track` include path).

## Runtime and log result

Exit 0 via `QuitGame`. Log: 0 load errors, 0 ensures, 0 fatals.

## Rendered verification status

`Rendered verification: PASSED (Metal, September 2026).` The toolchain
block that held at write time is resolved; `-Task10Shots` captures from
the circuit map went through the real Metal renderer with exit 0.

Track presentation evidence (prototype appearance, not quality claims):

- Start/finish straight: `../../site/screenshots/track-start.png`
- Fast sweeper: `../../site/screenshots/track-sweeper.png`
- Hairpin: `../../site/screenshots/track-hairpin.png`
- Driving: `../../site/screenshots/track-race.png`

Originals, manifests, and logs remain under
`game/RacingGame/Saved/Task10E2E/screenshots/` (gitignored runtime
evidence); the site copies are the committed duplicates.

## Limitations

Test-driver steering (pure pursuit), not human driving. Development
geometry and widths, not a designed racing circuit. Frame-delta
integration. Nullrhi logic timing only. Visuals, feel, and PIE
unverified. Map authoring needs the editor (headless Python worked after
two command-line lessons, recorded below).

## Development notes (failing runs before passing)

- Map script never executed: bare `py name` is not a file run (NameError
  in the engine log); the working form is
  `-ExecCmds='py /abs/path/script.py'` with single-quote wrapping so the
  path survives. Editor then idles (no auto-quit); kill it after save.
- Lap pinned twice at hairpin entries. First pin traced with a probe-side
  forward sweep to a wall stub thrown 300 cm past a short curve micro by
  the fixed joint extension. Extension capped at half segment length.
- Second pin traced to the component (`Wall_48_R`): the r450 hairpin sat
  810 cm from straight B while road plus walls need 1300, so the exit
  wall crossed the lane. Redesign (r700 west end moved to x=-2300, width
  800) verified clean by an offline 2D clearance script before rebuild:
  zero wall corners within margin of the lane away from their segment.
- Turn-speed targets sized to the frozen yaw rule (sweeper 600, hairpin
  400). Thresholds frozen throughout; only geometry and driver changed.
- The blockage sweep and heartbeat logs stay in the probe as permanent
  diagnosis infrastructure.

## Next step

Task 8 specification. Deferred: AI, HUD, rules, timing, cameras, art.
