# Track: V1 Circuit

Order: graybox -> road + collision + barriers -> terrain -> racing spline -> props/lighting.

Required: closed loop, road mesh + collision, barriers, start/finish line,
sequential checkpoints, AI racing line spline, spawn grid, pit-less layout.

Spline drives laps, positions, and AI. Resample for distance lookups.
Validate: no lap exploit driving backward or cutting.
