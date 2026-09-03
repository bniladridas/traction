# Third party components used by traction

No `LICENSE` file exists yet for this repository's own code and docs. That
choice is deferred until the distribution model is decided. The components
below remain under their own terms regardless of what license is later
chosen for original work. This is an inventory, not legal advice.

## Engine and engine content

- Unreal Engine 5.8.2, installed through the Epic Games Launcher.
  Governed by the Epic Games Unreal Engine license, including its
  distribution and royalty terms. Required reading before any release.
- Vehicle Advanced template content (`TP_VehicleAdv`, shared `Vehicles`
  pack, VehicleBasic map). Epic-provided content subject to Epic's
  applicable content and license terms.

## Platform and SDK

- macOS, Xcode, and the Metal SDK. Development and distribution must
  comply with Apple's developer terms.

## Tools (used or planned)

- Blender (GPL): no separate commercial license for its use as a tool.
- Git (GPL-2.0): no separate commercial license.
- GitHub: account and service terms; not a game license.
- OpenCode: terms depend on the version and provider in use.

## Rule for new additions

Every third-party library, asset pack, font, sound, HDRI, or tool output
added to this repository must be recorded here with its source and license
before it is used in a build. Content without clear rights is not added.

## Large files

Git LFS is not configured: the largest tracked file is 17 MB and the
tracked tree totals about 104 MB, both GitHub-friendly. Set up LFS before
any push that would add a file over 50 MB or take the tracked tree over
1 GB (expected when real vehicle and track art lands).
