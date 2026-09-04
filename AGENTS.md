# AGENTS.md: OpenCode Rules

Read relevant existing code before editing. Keep changes small and modular.

1. Never modify unrelated systems. One task = one system.
2. Small incremental diffs. No speculative architecture or placeholders.
3. Data-driven tuning: expose params via config/DataAsset, no magic numbers in logic.
4. Build after every significant change (`UnrealBuildTool` / Xcode build). Report files changed + build result.
5. Never claim works without verification: cite build log + headless E2E evidence (or manual test checklist where interactive runs apply).
6. Document public interfaces changed.
7. Prefer UE5 conventions: `ARaceVehicle`, `URaceVehicleMovement`, etc.
8. macOS/Metal constraints: ARM-native, no DirectX-only plugins, profile draw calls/shadows.
9. If blocked (missing asset, engine version mismatch), stop and report. Do not invent workarounds silently.
10. Scholarship Gist edits are append-only file operations: fetch content
via the API content path, verify with grep before editing, and never
feed `gh gist view` presentation output back into `gh gist edit` (its
headers pollute the stored file).

## Task prompt template

```text
Project: Racing Game (UE5, macOS ARM, Metal)

Task: <single system only>
Constraints: Do not modify <list>. Read <files> first.
Requirements:
- <bullet list, testable>
- Configurable params: <list>
- Build project, report changed files + build result + test checklist.
```

## Commit Rules

Use Conventional Commits: `<type>(<scope>): <imperative description>`.

Allowed types: feat, fix, refactor, test, build, perf, docs, chore, ci, assets.

Rules:
- Keep the subject concise, lowercase type and scope, imperative wording.
- Do not end the subject with a period.
- One commit is one coherent change; do not mix unrelated changes.
- Do not commit generated build artifacts.
- Do not claim verification in a commit message unless it was performed.
- Prefer scopes: vehicle, physics, camera, track, audio, ui, build, test, ci, docs, assets.
- Never use commits as marketing statements.
