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
`tools/check_regression.py` gates the 40 flags locally.
