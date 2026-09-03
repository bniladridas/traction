# Gameplay: Race Loop

State machine: Menu -> Countdown (3-2-1-GO) -> Racing -> Finished -> Results.

Laps: Start -> Checkpoint 1..N in order -> Finish. Wrong-way / skip = invalid lap.
Position = lap + checkpoint index + distance along spline.
Timing: current lap, best lap, total race time, 3-lap default.

HUD: POSITION x/y, LAP x/y, TIME mm:ss.mmm, SPEED km/h. Minimal.
