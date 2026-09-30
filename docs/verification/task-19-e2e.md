# Task 19 E2E Verification: AI racecraft, line-commit overtake (first slice)

Date: 2026-09-30 (UTC). Branch: `feat/ai-racecraft-19`, forked from
`main @ 0cdbfb8` (Task 18 complete: 100 flags, 15 loaded E2E artifacts).
Program: headless nullrhi standalone, `?game=Task19RacecraftGameMode` on the
circuit map `Track1_TestCircuit`. Evidence: `Saved/Task19E2E/results.json`.

## Provenance of this contract

This is a **fresh Task 19 contract derived from repository evidence**. It is
**not** a recovery of any earlier Task 19 contract. A previously reported
freeze commit (`ccd2e6e`) does not exist in this repository or on `origin`,
and its gate names and thresholds are therefore **not** authoritative and are
**not** reproduced here. Every name and numeric value below was chosen from
the current committed tree and is justified in-line.

## Contract revision 1 - Gate 4 measurement and staging (2026-09-30)

This revision supersedes only the original frozen-contract definition of
Gate 4 and the setup
assumption that adjacent grid slots can exercise it. Post-checkpoint runs
showed that frozen grid slots place the defender and attacker about 100 cm
apart in absolute track distance, while each driver anchors its own
`UnwrappedStart` at its spawn position. The original progress-difference gap
therefore reduced to distance traveled, became zero and then negative, and
could never exceed 600 cm. Widening absolute spawn separation left
`outside_window_samples` at zero, confirming that the defect was definitional
rather than geometric tuning.

Accordingly, this revision defines `SeparationCm` as absolute unwrapped
track-distance separation and freezes the initial staged geometry needed to
exercise the negative case. Threshold values are unchanged. The wider staged
separation is load-bearing test staging, not production tuning.

## Scope

Task 19 = **first slice of AI racecraft: line-commit overtake only**.

`DESIGN.md:40` records the AI directory as "pursuit driver + recovery (no
overtake yet)". `ROADMAP.md:32` lists "Stage 1-4 AI racecraft" as M4-continued
work. This task adds exactly one behavior:

- When a rival enters a defined **attack window**, the pursuing attacker
  commands its existing `LineOffset` toward the **free side**.
- When the attack-window condition clears, the attacker **cedes** and returns
  to its frozen line.

Out of scope for this task: defending, right-of-way, pace press, broader
racecraft stages, physics, collision behavior, `ARaceManager` rules, HUD,
camera, and any new race state.

**Line commitment is the behavior under test. A physical pass is NOT
required by this contract.** The existing vehicle model may or may not
produce an order change under this setup, and no gate asserts one. Nothing
here is a hidden seventh gate about overtaking physics.

## Setup (frozen)

Circuit map `Track1_TestCircuit`, selected with `?game=Task19RacecraftGameMode`
(a new GameMode mirroring `RacePaceTestGameMode`: spawn track and manager when
absent, snap the player to grid slot 0, stage the two AI rivals below with the
frozen tiers and lines, register all three participants, then spawn
`ATask19Probe`).

| Participant | Staging source | Staged spawn | Frozen line | Frozen pace |
|---|---|---|---|---|
| Player (parked clear) | grid slot 0 | on track edge | n/a (input disabled) | n/a |
| Defender | grid-slot-1 geometry advanced 900 cm along the centerline | `S = 1200 cm` | `LineDefender` = -120 cm | `PaceDefender` = 0.85 |
| Attacker | grid slot 2 | `S = 200 cm` | `LineAttacker` = +120 cm | `PaceAttacker` = 1.0 |

The initial staged absolute separation is therefore `1000 cm`, which is
greater than `AttackWindowCm` (600 cm). This staged geometry is required so
the run begins outside the attack window; it does not change paces, lines, or
thresholds.

### Frozen separation measurement

`SeparationCm` is the defender's absolute unwrapped centerline distance minus
the attacker's absolute unwrapped centerline distance:

```text
SeparationCm =
    Defender.GetUnwrappedDistance() -
    Attacker.GetUnwrappedDistance()
```

`GetUnwrappedDistance()` is an additive public read-only accessor exposing the
driver's current unwrapped centerline distance. It must not normalize the
result by spawn position and must not write race, manager, lap, order, pace,
or line state. `GetProgressDistance()` retains its frozen Task 9 meaning and
is used only for attacker progress and stall monitoring, not for separation.

The probe mirrors `Task13Probe`: player parked clear of both AI lines, input
disabled, `Manager->StartRace()` after 1 s, running order resolved **read-only**
via `ARaceManager::GetPosition`.

### Definition of "free side"

The free side is the side of the track **away from the defender's line**.
The defender holds `LineDefender` = -120 cm, so the side away from it is the
positive-lateral side. The attacker already runs `LineAttacker` = +120 cm, so
committing to the free side means commanding **at least +240 cm**
(`LineAttacker` + `CommitShiftCm`) or beyond. "Free side" is therefore a
testable numeric condition, not an interpretation.

## Frozen thresholds (`Task19Limits`)

These are **fresh Task 19 thresholds derived from the current repository**,
fixed BEFORE the first run. Derivations:

| Constant | Value | Derivation from repository evidence |
|---|---|---|
| `FieldSize` | 3 | Task 13 / Task 18 `FieldSize` convention: parked player + 2 AI |
| `PaceDefender` | 0.85f | `Task13Limits::PaceSlow` verbatim |
| `PaceAttacker` | 1.0f | `Task13Limits::PaceFast` verbatim |
| `LineDefender` | -120.0f | `Task13Limits::LineSlow` verbatim |
| `LineAttacker` | 120.0f | `Task13Limits::LineFast` verbatim |
| `AttackWindowCm` | 600.0f | 0.5 s decision horizon at `StraightTarget` 1200 cm/s (`RaceAIDriver.h:38`) |
| `CommitShiftCm` | 120.0f | equals the frozen Task 13 parallel-line separation (+120 vs -120) |
| `CommitWindowCm` | 300.0f | 0.25 s of cruise at 1200 cm/s; 10% of Task 13 `ProgressMinCm` (3000 cm) |
| `CedeToleranceCm` | 1.0f | `Task18Limits::PosTolCm` verbatim |
| `StallProgressCm` | 50.0f | Task 13 + Task 18 verbatim (deadlock monitoring) |
| `StallWindow` | 20.0f | Task 13 + Task 18 verbatim (deadlock monitoring) |
| `ProgramTimeout` | 240.0f | Task 13 + Task 18 verbatim |

## The six gates

Every gate reads only frozen public seams and the additive read-only absolute
separation seam (`GetPaceFactor`, frozen `LineOffset`,
`GetUnwrappedDistance`, `ARaceManager::GetPosition`). No gate writes race
state, manager position, laps, or order. `GetProgressDistance()` is used only
for attacker progress and stall monitoring.

### 1. `t19_racecraft_configured`
Exactly `FieldSize` (3) participants staged, both AI drivers present, and the
frozen assignments read back exactly: `GetPaceFactor()` = 0.85 (defender) and
1.0 (attacker), frozen `LineOffset` = -120 (defender) and +120 (attacker).
Proves the field is staged on the frozen tiers and lines before any decision
is exercised.

### 2. `t19_attack_window_detected`
The absolute unwrapped separation, `SeparationCm`, is observed in the attack
window,

```text
0 cm <= SeparationCm <= AttackWindowCm
```

at least once while Racing. Records `min_separation_cm`, defined as the
minimum observed nonnegative separation. Proves the window condition is
reachable and observable rather than vacuous.

### 3. `t19_commit_free_side`
While the absolute separation is inside the window,

```text
0 cm <= SeparationCm <= AttackWindowCm
```

the attacker's **commanded** `LineOffset` satisfies

```text
commanded_offset >= LineAttacker + CommitShiftCm     (i.e. >= 240 cm)
```

within `CommitWindowCm` (300 cm) of further attacker progress past the first
window entry. Attacker progress continues to use the frozen
`GetProgressDistance()` meaning. This gate reads the **commanded offset**, not
the pawn's physical lateral position. It does **not** require the vehicle to
physically move 120 cm laterally, so it does not depend on vehicle physics,
suspension, or collision behavior. Records `max_committed_offset_cm`.

### 4. `t19_no_commit_outside_window`
On every probe tick where the absolute separation exceeds the attack window,

```text
SeparationCm > AttackWindowCm
```

the attacker's commanded `LineOffset` stays within `CedeToleranceCm` (1.0 cm)
of the frozen `LineAttacker` (120 cm). This gate is satisfied only if both
conditions hold:

- the outside-window condition is actually exercised at least once
  (`outside_window_samples > 0`);
- zero illegal commands occur during those samples
  (`commands_outside_window == 0`).

This is the discriminating negative case: it fails if the negative condition
is never exercised, if the decision layer is unconditional, or if the attacker
line was simply changed permanently rather than committed conditionally.

### 5. `t19_cede_back`
After the absolute separation leaves the attack window,

```text
SeparationCm > AttackWindowCm or SeparationCm < 0 cm
```

the attacker's commanded `LineOffset` returns to within `CedeToleranceCm`
(1.0 cm) of the frozen 120 cm and stays within tolerance for the remainder of
the run. Records `cede_offset_cm`. Proves the commitment is conditional and
reversible, completing the attack then cede cycle.

### 6. `t19_scope_frozen`
Runtime invariants plus static repository checks, proving the seam is additive
and read-only:

- Runtime, asserted by `ATask19Probe`:
  - attacker's `GetPaceFactor()` remains exactly 1.0 for the whole run (the
    frozen tier is never mutated by the racecraft layer);
  - defender's frozen `LineOffset` remains exactly -120 cm and the defender
    receives no racecraft command of any kind;
  - defender's `GetPosition` is never altered by the probe or the racecraft
    layer (order is read, never written).
- Static, enforced by CI and `tools/check_regression.py` (not by the runtime
  probe alone):
  - `ARaceManager` position/lap/state members are not written by any racecraft
    code;
  - no new race-state type or namespace is introduced by this task.

## Expected behavior

The attacker runs +120 cm at pace 1.0; the defender runs -120 cm at pace 0.85.
The staged absolute separation begins at 1000 cm, so the run starts outside
the 600 cm window. As the attacker closes the absolute separation into the
window, its **commanded** line steps toward the free side to at least +240 cm
within 300 cm of attacker progress past window entry. While the absolute
separation remains outside the window, the commanded line stays at 120 cm.
Once the absolute separation leaves the window, the commanded line returns to
120 cm and remains there. No pace change, no collision behavior change, and no
defender reaction are required or claimed.

## Regression and CI contract

- `tools/check_regression.py` gains `T19_KEYS` and loads `Task19E2E`, taking
  the suite from **100 flags to 106 flags** (100 existing + these 6). Existing
  Task 1-18 keys, thresholds, and artifacts are untouched.
- `.github/workflows/test.yml` requires `docs/verification/task-19-e2e.md`
  present, `namespace Task19Limits` present, `Task19Probe.h` present, and the
  static half of gate 6 (no new race-state namespace; racecraft does not write
  manager state).
- No new map is required; this task reuses `Track1_TestCircuit`.

## Out-of-scope guarantees

Frozen Task 1-18 gates, thresholds, and artifacts are not modified. No
finishing-order or race-result gate is added. No physics, collision, HUD,
camera, or manager-rule change is part of this task.

## Evidence to record after the run

`Saved/Task19E2E/results.json` with the six `t19_*` keys plus recorded
`min_separation_cm`, `max_committed_offset_cm`, `cede_offset_cm`,
`window_hits`, `outside_window_samples`, `commands_outside_window`,
`no_deadlock`, `frames`, and `note`. Regression total recorded as
106/106.

## Verification record (2026-09-30, UTC)

Final full-suite result: **106/106 PASS** (`python3
tools/check_regression.py`, exit 0) on branch `feat/ai-racecraft-19`
at the regression-contract commit.

- All fourteen E2E programs were rerun headless nullrhi standalone on the
  current binary: Task 2's program on the flat `Task2_TestTrack` (producing
  the Task 2/3/5/6 artifacts) and the remaining thirteen programs on the
  circuit map `Track1_TestCircuit`, each with its own `?game=` mode.
- All sixteen `Saved/Task*E2E/results.json` artifacts are fresh products of
  those runs. They remain gitignored diagnostic output and are not committed
  as evidence.
- The first full-checker run after the sequence returned 101/106 with five
  false flags (`T10 no_pops`; `T15 results_populated`, `times_consistent`,
  `results_immutable`, `no_deadlock`). Both were investigated rather than
  suppressed: Task 10 had completed but recorded single-tick camera jumps of
  161/133 cm against the 50 cm limit, and Task 15 had tripped its deadlock
  detector. Each was rerun in isolation and cleared 6/6 (Task 10 max pops
  1.5/2.4 cm; Task 15 `results complete`), consistent with transient nullrhi
  timing sensitivity rather than a regression. No source, contract, tooling,
  CI, or threshold change was made between the failing and passing runs.
- Task 19 slice in the final verified run: all six `t19_*` gates true
  (`min_separation_cm=0.0`, `max_committed_offset_cm=240.0`,
  `cede_offset_cm=120.0`, `window_hits=5253`,
  `outside_window_samples=6771`, `commands_outside_window=0`,
  `no_deadlock=true`, `frames=96179`).

Historical Task 2-18 reports are untouched; this record establishes current
full-suite verification only.
