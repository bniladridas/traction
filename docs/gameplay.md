# Gameplay: Race Loop

Status: verified through Task 8 (single player) and Task 9 (one AI).
Future UI wording below is a target, not current functionality.

Verified: Ready -> Countdown (configurable) -> Racing -> Finished,
with reset-to-Ready. Laps: start line -> checkpoints 0..N in order ->
finish line; out-of-order crossings are ignored and invalidate the lap;
a completed invalid sequence does not count. Timing: last and best lap
per participant. Default LapCount 2 (configuration, not a promise).

Not verified: positions, spline-distance ordering, multi-car fields,
3-lap default, HUD (POSITION/LAP/TIME/SPEED), menus, results screens.
