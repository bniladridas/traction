# Task 13 E2E Verification: AI Pace Variation

Date: 2026-09-05 (UTC). Engine 5.8.2. Machine M1/8GB, nullrhi standalone.
Implements GitHub issue #44 on branch `feat/task-13-pace`. Tasks 2, 3,
5, 6, 7, 8, 9, 10, 11, 12 contracts frozen; Task 13 evidence is separate
in `Saved/Task13E2E/results.json`. No racecraft, HUD, physics, or visual
work.

## Objective

Per-driver pace factors producing genuinely different lap times, so a
position overtake emerges from pace with no teleports, scripting, or
contact. Deterministic outcome claimed only conditional on fixed
ordering, spawn, pace factors, and conditions.

## Architecture

```text
ARacePaceTestGameMode (?game= override, circuit map only)
    grid slots 1-2 with frozen tiers (0.85 behind-left, 1.0 ahead-right)
    2x URaceAIDriver (target-speed multiplier + line offset, Apply* only)
    ARaceManager (laps, order, timing; unchanged)
    ATask13Probe (parks player, monitors, resets, asserts)
```

`PaceFactor` scales driver speed targets; `LineOffset` staggers pursuit
lines so the pass completes on parallel lines. Neither reacts to other
cars. Player parked clear, present but stationary.

## E2E program

Start, park player, grid order 120. Reset with grid re-staging at 12 s,
restart, concurrent race to both AI finished. Overtake emerges mid-race.

## Measurements (final passing run, exit 0, 112475 frames)

Pace factors read back 0.85/1.0. Best laps 20.11 vs 15.93 (4.18 s
spread over the 1.0 s margin). Order 120 early, 210 late and final.
Both laps 2/2. Max progress gap 8.28 s (under the 20 s stall window).

## Thresholds (fixed before the final passing run)

Tiers 0.85/1.0, lines -120/+120. Lap margin 1.0 s. Progress 3000 cm
within 40 s. Stall: under 50 cm over any rolling 20 s while Racing and
unfinished. Reset at 12 s. Timeout 240 s. LapCount 2 from race config.

## Regression

Final binary: old-map Tasks 2/3/5/6 green, circuit Task 7 6/6,
race-state Task 8 10/10, AI Task 9 6/6, camera Task 10 6/6, positions
Task 11 6/6, field Task 12 6/6, `tools/check_regression.py` 70/70 PASS.
Frozen schemas and thresholds untouched.

## Build result

`Build.sh` exit 0. No new module paths.

## Runtime and log result

Exit 0 via `QuitGame`. Log: 0 load errors, 0 ensures, 0 fatals.

## Rendered verification status

Verified for the capture path (Standing Limitations). No new captures
in this task; nothing manufactured.

## Limitations

Constant pace factors, not skill, tire, or fuel modeling. Two tiers;
larger fields later. No blocking, defending, or mistakes. Computed
order only; nothing displayed. Nullrhi logic timing only. Visuals,
feel, and PIE unverified.

## Development notes (failing run before passing)

- First run: 3/6, faster car lapped slower (19.71 vs 19.07) with no
  respawns. Diagnosis: identical pursuit lines force tailgating, so the
  pace edge dies in formation. Fixed with per-driver `LineOffset`
  (parallel lines, still non-reactive) plus a shared-static heartbeat
  made per-instance. Thresholds frozen throughout; only driver support
  and test setup changed.

## Next step

Task 14 specification. Deferred: racecraft behaviors, HUD, fields.
