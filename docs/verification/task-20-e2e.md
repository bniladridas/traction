# Task 20 E2E Verification: AI racecraft, defend-the-line (second slice)

Date: 2026-10-01 (UTC). Branch: `feat/defend-20`, forked from
`main @ 1a829e3` (Task 19 complete: 106 flags, 16 loaded E2E artifacts).
Program: headless nullrhi standalone, `?game=Task20RacecraftGameMode` on the
circuit map `Track1_TestCircuit`. Evidence: `Saved/Task20E2E/results.json`.

## Provenance of this contract

This is a **fresh Task 20 contract derived from repository evidence**. It is
**not** a recovery of any earlier racecraft contract. Task 19
(`docs/verification/task-19-e2e.md`, Revision 1) owns the attacker's line
commitment and is frozen; this contract adds only the defender's response
and does not redefine, re-tune, or override any Task 19 gate, threshold,
or behavior. Every name and numeric value below was chosen from the
current committed tree and is justified in-line.

## Scope

Task 20 = **second slice of AI racecraft: defend-the-line only**.

- When an attacker enters the defender's **rear attack window**, the
  defender commands its existing `LineOffset` toward a bounded
  **defensive line**.
- When the rear-window condition clears, the defender **cedes** and returns
  to its frozen line.

Out of scope for this task: physical contact or blocking physics, race
order and completed-pass semantics, pace changes, multi-rival behavior,
larger grids, `ARaceManager` rules, HUD, camera, and any new race state.

**A physical block is NOT required by this contract.** No gate asserts
contact, a denied pass, or an order change. Nothing here is a hidden
seventh gate about blocking physics.

## Command precedence (frozen)

Task 19 owns the attacker's commitment; Task 20 adds the defender's
response. Precedence is structural, not arbitrated at runtime:

- Each layer commands only its own car. The attacker layer never writes
  the defender's commanded offset and vice versa; neither layer reads
  the other's commanded state.
- The Task 19 attacker rule (window, shift, cede) is unchanged and is
  never overridden, suppressed, or re-tuned by defender logic.
- Both layers read the same `SeparationCm` and their own frozen values
  only. No shared mutable racecraft state is introduced.
- Separation by construction: attacker committed (+240 cm) versus
  defender defensive (0 cm) keeps 240 cm of commanded separation. The
  defender's defensive target never crosses the centerline (0 cm), so the
  two commanded lines cannot meet or cross while both layers are
  committed.
- Shared arbitration between layers, if ever needed, is Stage 3+ scope,
  not this contract.

## Setup (frozen)

Circuit map `Track1_TestCircuit`, selected with `?game=Task20RacecraftGameMode`
(a new GameMode mirroring `Task19RacecraftGameMode`: spawn track and manager
when absent, snap the player to grid slot 0, stage the two AI rivals below
with the frozen tiers and lines, register all three participants, then spawn
`ATask20Probe`).

| Participant | Staging source | Staged spawn | Frozen line | Frozen pace |
|---|---|---|---|---|
| Player (parked clear) | grid slot 0 | on track edge | n/a (input disabled) | n/a |
| Defender | grid-slot-1 geometry advanced 900 cm along the centerline | `S = 1200 cm` | `LineDefender` = -120 cm | `PaceDefender` = 0.85 |
| Attacker | grid slot 2 | `S = 200 cm` | `LineAttacker` = +120 cm | `PaceAttacker` = 1.0 |

The initial staged absolute separation is therefore `1000 cm`, which is
greater than `RearWindowCm` (600 cm). This staged geometry is required so
the run begins outside the rear window; it does not change paces, lines,
or thresholds. Both AI drivers carry their racecraft layers: the attacker
with the frozen Task 19 configuration, the defender with the frozen
Task 20 configuration below.

### Frozen separation measurement

`SeparationCm` is the defender's absolute unwrapped centerline distance minus
the attacker's absolute unwrapped centerline distance, reusing the Task 19
read-only seam (no new measurement seam is required):

```text
SeparationCm =
    Defender.GetUnwrappedDistance() -
    Attacker.GetUnwrappedDistance()
```

`GetProgressDistance()` retains its frozen Task 9 meaning and is used only
for progress-window measurements and stall monitoring, not for separation.

The probe mirrors `Task19Probe`: player parked clear of both AI lines, input
disabled, `Manager->StartRace()` after 1 s, running order resolved **read-only**
via `ARaceManager::GetPosition`.

### Definition of "defensive line"

The defensive line is the defender's frozen line shifted toward the
attacker's side by exactly `DefensiveShiftCm`, bounded to never cross the
centerline. The defender holds `LineDefender` = -120 cm and
`DefensiveShiftCm` = 120 cm, so defending means commanding **exactly 0 cm**
(the centerline). All staged and commanded lines (-120, 0, +120, +240)
sit inside the frozen half-width of 400 cm (`TrackWidth` 800 cm in
`FRaceTrackConfig`) with margin. "Defensive line" is therefore a testable
numeric condition, not an interpretation.

## Frozen thresholds (`Task20Limits`)

These are **fresh Task 20 thresholds derived from the current repository**,
fixed BEFORE the first run. Derivations:

| Constant | Value | Derivation from repository evidence |
|---|---|---|
| `FieldSize` | 3 | Task 13 / Task 18 `FieldSize` convention: parked player + 2 AI |
| `PaceDefender` | 0.85f | `Task13Limits::PaceSlow` verbatim |
| `PaceAttacker` | 1.0f | `Task13Limits::PaceFast` verbatim |
| `LineDefender` | -120.0f | `Task13Limits::LineSlow` verbatim |
| `LineAttacker` | 120.0f | `Task13Limits::LineFast` verbatim |
| `RearWindowCm` | 600.0f | 0.5 s decision horizon at `StraightTarget` 1200 cm/s (`RaceAIDriver.h:38`), same derivation as Task 19 `AttackWindowCm` |
| `DefensiveShiftCm` | 120.0f | equals the frozen Task 13 parallel-line separation (+120 vs -120), same derivation as Task 19 `CommitShiftCm`; defender -120 commands exactly the 0 cm centerline |
| `DefenseWindowCm` | 300.0f | 0.25 s of cruise at 1200 cm/s; 10% of Task 13 `ProgressMinCm` (3000 cm), same derivation as Task 19 `CommitWindowCm` |
| `CedeToleranceCm` | 1.0f | `Task18Limits::PosTolCm` verbatim |
| `StallProgressCm` | 50.0f | Task 13 + Task 18 verbatim (deadlock monitoring) |
| `StallWindow` | 20.0f | Task 13 + Task 18 verbatim (deadlock monitoring) |
| `ProgramTimeout` | 240.0f | Task 13 + Task 18 verbatim |

## The six gates

Every gate reads only frozen public seams and the read-only absolute
separation seam (`GetPaceFactor`, frozen `LineOffset`,
`GetUnwrappedDistance`, `ARaceManager::GetPosition`). No gate writes race
state, manager position, laps, or order. `GetProgressDistance()` is used only
for progress windows and stall monitoring.

### 1. `t20_racecraft_configured`
Exactly `FieldSize` (3) participants staged, both AI drivers present, and the
frozen assignments read back exactly: `GetPaceFactor()` = 0.85 (defender) and
1.0 (attacker), frozen `LineOffset` = -120 (defender) and +120 (attacker),
defender racecraft layer enabled with the frozen Task 20 configuration,
attacker racecraft layer enabled with the frozen Task 19 configuration
unchanged. Proves the field is staged on the frozen tiers, lines, and
layer assignments before any decision is exercised.

### 2. `t20_defense_window_detected`
The absolute unwrapped separation, `SeparationCm`, is observed in the rear
attack window,

```text
0 cm < SeparationCm <= RearWindowCm
```

at least once while Racing. Records `min_separation_cm`, defined as the
minimum observed positive separation. Proves the window condition is
reachable and observable rather than vacuous.

### 3. `t20_defensive_line_committed`
While the absolute separation is inside the rear window,

```text
0 cm < SeparationCm <= RearWindowCm
```

the defender's **commanded** `LineOffset` satisfies

```text
commanded_offset == LineDefender + DefensiveShiftCm     (i.e. 0 cm)
```

within `CedeToleranceCm` (1.0 cm), within `DefenseWindowCm` (300 cm) of
further defender progress past the first window entry. Defender progress
continues to use the frozen `GetProgressDistance()` meaning. This gate
reads the **commanded offset**, not the pawn's physical lateral position.
It does **not** require the vehicle to physically move 120 cm laterally,
so it does not depend on vehicle physics, suspension, or collision
behavior. Records `defensive_offset_cm`.

### 4. `t20_no_defense_outside_window`
On every probe tick where the absolute separation exceeds the rear window,

```text
SeparationCm > RearWindowCm
```

the defender's commanded `LineOffset` stays within `CedeToleranceCm` (1.0 cm)
of the frozen `LineDefender` (-120 cm). This gate is satisfied only if both
conditions hold:

- the outside-window condition is actually exercised at least once
  (`outside_window_samples > 0`);
- zero illegal commands occur during those samples
  (`commands_outside_window == 0`).

This is the discriminating negative case: it fails if the negative condition
is never exercised, if the decision layer is unconditional, or if the
defender line was simply changed permanently rather than commanded
conditionally.

### 5. `t20_defense_holds_centerline`
After the absolute separation leaves the rear window,

```text
SeparationCm > RearWindowCm or SeparationCm <= 0 cm
```

the defender's commanded `LineOffset` returns to within `CedeToleranceCm`
(1.0 cm) of the frozen -120 cm and stays within tolerance for the remainder
of the run. Records `cede_offset_cm`. Additionally, the defender's commanded
offset never exceeds the centerline target at any racing tick: the recorded
`max_defender_commanded_cm` must be at most 0 cm. Together these prove the
response is conditional, reversible, and bounded, completing the defend then
cede cycle without ever crossing into the attacker's half.

### 6. `t20_scope_frozen`
Runtime invariants plus static repository checks, proving the seam is additive
and read-only:

- Runtime, asserted by `ATask20Probe`:
  - both drivers' `GetPaceFactor()` remain exactly their frozen tiers for
    the whole run (pace semantics untouched by either layer);
  - defensive behavior is limited to the rear-window response, the
    commanded defensive line, the centerline bound, and the simultaneous
    two-layer separation invariant below;
  - whenever both layers are simultaneously committed, the commanded
    separation (attacker commanded minus defender commanded) stays at or
    above 240 cm; records `min_commanded_separation_cm`. A
    simultaneous-commit sample is a frame where the attacker command is
    +240 cm and the defender command is 0 cm; `min_commanded_separation_cm`
    is computed over those samples only, never over frames where only one
    side has committed;
  - defender's `GetPosition` is never altered by the probe or either
    racecraft layer (order is read, never written).
- Task 19 attacker behavior remains unchanged and is evidenced by its
  existing artifact; this gate does not re-assert it.
- Static, enforced by CI and `tools/check_regression.py` (not by the runtime
  probe alone):
  - `ARaceManager` position/lap/state members are not written by any
    racecraft code;
  - no new race-state type or namespace is introduced by this task.

## Expected behavior

The attacker runs +120 cm at pace 1.0; the defender runs -120 cm at pace
0.85. The staged absolute separation begins at 1000 cm, so the run starts
outside the 600 cm rear window. As the attacker closes the absolute
separation into the window, the attacker's **commanded** line steps to at
least +240 cm (Task 19, unchanged) while the defender's **commanded** line
steps to exactly 0 cm within 300 cm of defender progress past window entry.
While the absolute separation remains outside the window, both commanded
lines stay on their frozen lines. Once the absolute separation leaves the
window, both commanded lines return to their frozen lines and remain there.
No pace change, no collision behavior change, and no order change are
required or claimed.

## Regression and CI contract

- `tools/check_regression.py` gains `T20_KEYS` and loads `Task20E2E`, taking
  the suite from **106 flags to 112 flags** (106 existing + these 6). Existing
  Task 1-19 keys, thresholds, and artifacts are untouched.
- `.github/workflows/test.yml` requires `docs/verification/task-20-e2e.md`
  present, `namespace Task20Limits` present, `Task20Probe.h` present, and the
  static half of gate 6 (no new race-state namespace; racecraft does not write
  manager state).
- No new map is required; this task reuses `Track1_TestCircuit`.

## Out-of-scope guarantees

Frozen Task 1-19 gates, thresholds, and artifacts are not modified. No
finishing-order or race-result gate is added. No physics, collision, HUD,
camera, or manager-rule change is part of this task. No pace-semantics
change is part of this task.

## Evidence to record after the run

`Saved/Task20E2E/results.json` with the six `t20_*` keys plus recorded
`min_separation_cm`, `defensive_offset_cm`, `cede_offset_cm`,
`max_defender_commanded_cm`, `window_hits`, `outside_window_samples`,
`commands_outside_window`, `min_commanded_separation_cm`, `no_deadlock`,
`frames`, and `note`. Regression total recorded as 112/112.
