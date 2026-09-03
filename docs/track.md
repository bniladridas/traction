# Track: V1 Circuit

Status: first circuit built (Task 7). Runtime-built from data, not
hand-authored.

Order done: graybox road + collision + barriers -> terrain-free lot ->
centerline + ordered checkpoints -> start/finish with track-owned spawn.

Present: closed loop (~123 m: straights, sweeper, hairpin), road mesh +
collision, barriers, start/finish line, sequential checkpoints, spawn
pose, deterministic lap evidenced.

Not yet: racing spline resampled for distance lookups, AI racing line,
spawn grid, lap/position systems, wrong-way and cut detection (Task 8+).

Validate before racing on it: no lap exploit driving backward or cutting.
