# GitHub Pipeline

The pipeline is the repository's automated check layer. Its purpose is
not to replace the local Unreal environment: GitHub-hosted CI checks
what is verifiable without Unreal, while the M1 Mac builds and runs
the game.

## Meaning

The pipeline marks the boundary between **repository correctness** and
**application verification**. GitHub checks repository invariants,
documentation, configuration, and frozen regression-contract
definitions. The local Mac verifies compilation, runtime behavior,
rendering, and performance. This keeps the pipeline honest about what
it can actually prove: it never sees measured results (artifacts are
gitignored), only the contracts those results must satisfy.

## Potential

As the project grows, the pipeline can take on more without depending
on the Unreal runtime: repository and documentation validation, frozen
contract checks, configuration and schema checks, artifact validation
for committed evidence, static architecture checks, release and version
consistency. The goal is not to automate everything, only that every
part checkable automatically is checked consistently.

## Workflows

- `lint.yml`: Markdown, whitespace, and JSON hygiene on ubuntu.
- `test.yml`: contract checks on ubuntu (docs, threshold namespaces,
  headers, tooling, no pending-runner references).
- `site.yml`: static site validation and Pages deployment.

Unreal build and headless E2E run on the Mac per `docs/testing.md`;
`tools/check_regression.py` gates the 82 flags locally.

## Why no macOS Unreal CI

Traction does not run Unreal application verification in GitHub-hosted
macOS Actions. The constraint is practical rather than architectural.

Unreal Engine 5.8.2 occupies approximately 43 GB on the development
Mac, while standard GitHub-hosted macOS runners provide substantially
less free disk space before the repository, build products, and shader
data are considered. Larger macOS runners introduce additional cost and
availability constraints.

The current Unreal installation also depends on the Epic Games Launcher
and account-bound installation flow established during Milestone 1.
Reproducing that installation on ephemeral CI runners would add
credential and provisioning complexity, large cold-start costs, and
substantially longer macOS CI runs.

For these reasons, the verification boundary remains deliberate:

- GitHub-hosted Ubuntu Actions verify repository invariants,
  documentation, configuration, and frozen regression contracts.
- The Apple Silicon development Mac runs the Unreal build, headless E2E
  programs, full regression suite, and rendered Metal verification.

The local Mac currently runs the complete regression suite in minutes.
Until the cost, provisioning, and storage constraints change
materially, adding macOS Unreal CI would add complexity without
providing a proportionate verification benefit.
