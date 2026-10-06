# Vertical Slice Contract (Frozen)

**Vertical slice: Boot → Menu → Car/Track Select → Countdown → Rendered Race → Finish → Results → Restart. One circuit, human-driven player plus AI field, real renderer throughout. Foundation systems are reused, never reimplemented.**

This contract governs the forward game-building phase. It does not reopen
the closed audit, create Task 21, or change any frozen verification
contract. The three architectural escalation triggers stay armed:
T19 `CommandsOutsideWindow > 0`, packaged build exposing test game modes,
and observed vehicle-configuration divergence.

## Renderer requirement

All slice stages are exercised in the real UE renderer. Headless/NullRHI
verification may support individual systems but cannot substitute for
rendered acceptance. Renderer-specific defects become implementation work
only when they prevent a slice acceptance criterion from being met.

There is no standalone renderer issue in this phase. The renderer is the
evidence and execution environment for the slice, subordinate to the game.
If the PR4 rendered race exposes a genuine renderer blocker, a focused
issue/PR is created for that specific blocker.

## Stage 1 - Boot to menu

Production game mode (not another test mode) and production boot path,
with a minimal menu shell. No car/track selection yet.

Acceptance: launch enters the real game mode and presents an interactive
menu, with no test probe, test map override, or console command involved.

## Stage 2 - Car/track select

Selection UI enumerating real vehicle configurations and available
circuit content, passing the selected configuration into race setup.

Acceptance: every listed option corresponds to loadable content, and the
selected content is what actually starts. No placeholder entries.

## Stage 3 - Race presentation / countdown

Verify first what existing HUD/phase presentation already provides
before building anything. Bind Ready → Countdown → Racing, with human
input held until Racing.

Acceptance: (to be confirmed against the existing presentation path
during implementation; keep this PR small if the foundation suffices).

## Stage 4 - Human playable rendered race

Real player input, camera flow, HUD, AI field, checkpoints/laps,
reset/recovery, and the finish path. The largest PR, still one coherent
capability; split further only if implementation reveals genuinely
independent pieces.

Acceptance: A human can complete a full race against the AI field in the
real renderer, with readable HUD, functioning race cameras,
checkpoints/lap progression, reset/recovery, and finish detection.

Headless camera verification remains supporting evidence, not acceptance.

## Stage 5 - Results

Results model binding and results screen, with displayed order/times
verified against the manager's authoritative snapshot.

Acceptance: finishing produces a results screen matching the snapshot.

## Stage 6 - Restart

From results, a clean new race through the production game flow. No
test-only reset mechanism.

Acceptance: From results, restarting enters a clean new race through the
production game flow, with reset lap state, grid, and AI. The second race
completes successfully and its results snapshot is compared against the
first race to demonstrate independent race state.

## Deliberately outside the slice

Audio, pause, settings, persistence/progression, career, vehicle tuning,
new tracks or vehicles, multiplayer, controller-specific work beyond the
existing axes, packaging/distribution. A dev-build rendered run is the
evidence vehicle, not a packaged release.

## PR structure

One PR = one coherent game capability, independently reviewable and
verifiable. Each PR has its own branch, scope, acceptance criteria, only
the implementation needed for that capability, its own tests/verification
where applicable, its own rendered evidence where applicable, its own
documentation update, its own build/verification result, a clean tree
before handoff, and no unrelated cleanup. Each PR must build and leave
the repository coherent; no "works only after a later PR" branches unless
the dependency genuinely requires it.

Branches follow the dependency chain as stacked PRs:

```text
main
  └── PR1 production-mode (feat/vertical-slice-production-mode)
        └── PR2 selection (feat/vertical-slice-selection)
              └── PR3 countdown (feat/vertical-slice-race-start)
                    └── PR4 race (feat/vertical-slice-human-race)
                          └── PR5 results (feat/vertical-slice-results)
                                └── PR6 restart (feat/vertical-slice-restart)
```

Once a PR merges, the next is rebased/retargeted onto `main`. Review
question for each PR: does this establish one piece of the playable
slice without quietly changing another contract?
