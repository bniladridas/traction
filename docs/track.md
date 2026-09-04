# Track: V1 Circuit

Status: first circuit built and raced on (Tasks 7-9). Runtime-built
from data, not hand-authored.

Done: graybox road + collision + barriers, centerline + ordered
checkpoints, start/finish with track-owned spawn, deterministic lap,
race progression, one AI rival lapping.

Present: closed loop (~123 m: straights, sweeper, hairpin), road mesh +
collision, barriers, start/finish line, sequential checkpoints, spawn
poses, deterministic lap evidenced.

Not yet: racing spline resampled for distance lookups, AI racing line,
spawn grid, position system, wrong-way and cut detection. Those belong
to future race work, not to Tasks 8-9 (which verify ordered laps, not
exploit-proofing).

Validate before racing on it: no lap exploit driving backward or cutting.
