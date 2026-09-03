# Task 1 CLI E2E Verification: RacingGame (VehicleBasic template)

Date: 2026-09-03 (UTC). Engine: 5.8.2 CL 56702186. Machine: M1/8GB/arm64,
macOS 26.6.2. No display, mouse, or keyboard access; all checks via CLI
and editor automation. Full scholarship narrative lives in the public
Gist (`SCHOLARSHIP.md`, Entry 2); this file is the machine-checkable record.

## Method (no shipped-file edits)

Temporary, isolated harness under `game/RacingGame/Source/TP_VehicleAdv/Task1E2E/`
(4 files, header-marked verification-only, safe to delete):
- `ATask1E2EGameMode : ATP_VehicleAdvGameMode`: sets DefaultPawnClass to the
  shipped `/Game/VehicleTemplate/Blueprints/SportsCar/BP_SportsCar_Pawn`,
  uses a concrete base `APlayerController` (the template C++ PC is abstract),
  spawns the probe 1s after BeginPlay. Selected at launch via the documented
  URL override `?game=/Script/TP_VehicleAdv.Task1E2EGameMode`.
- `ATask1E2EProbe : AActor`: drives through the pawn's own public input path
  (`DoThrottle`/`DoSteering`), samples transforms at 4Hz with `TASK1E2E:`
  log lines, writes `Saved/Task1E2E/results.json`, then `QuitGame`.

Drive program: 0.5s settle, Phase A 2.5s full throttle straight, Phase B 3s
0.7 throttle + 0.6 steer, then stop/quit. Content fix applied during diagnosis
(see Problems): copied the official shared pack
`Templates/TemplateResources/Standard/Vehicles/Content/*` (59 files) to
`Content/Vehicles/` per its manifest (`VehiclesStandard`,
`DestinationFilesFolder: Vehicles`). This is what the editor wizard installs;
the file-level Task 1 creation had missed it.

## Results (run7, nullrhi -game, exit 0)

| # | Check | Status | Evidence |
|---|-------|--------|----------|
| 1 | Project launches | VERIFIED | Engine init 5.8.2 in `~/Library/Logs/RacingGame/RacingGame.log`, exit 0 |
| 2 | VehicleBasic loads | VERIFIED | `LoadMap: .../VehicleBasic`, pawn spawned at PlayerStart (4450, 0, 102) |
| 3 | Session starts, stays alive | VERIFIED | 12538 frames over 6.5s gameplay, clean `QuitGame` exit 0, 0 fatals |
| 4 | Sports car spawned, valid | VERIFIED | class `BP_SportsCar_Pawn_C`, mesh bound (`upd=VehicleMesh`), 4 wheels, camera chain `Front Camera <- Front Spring Arm` (rooted at pawn) |
| 5 | Throttle moves car forward | VERIFIED | 3614.1cm displacement, 3600.8cm along facing (99.6%), 0 to 2232.4cm/s (22.3m/s); yaw held 90.00 to 91.21 (straight) |
| 6 | Steering changes heading | VERIFIED | yaw excursion 69.12 to 111.81 (42.7 deg sweep) inside steer window; travel direction reversed (+Y to -Y); lateral X drift |
| 7 | Chase camera follows | VERIFIED | attach chain to pawn plus camera travel 5058.5cm vs pawn travel 5058.8cm (ratio 1.000) |
| 8 | No crash/instability in window | VERIFIED | 0 LoadErrors, 0 ensures, 0 fatals in run7; exit 0 |
| 9 | Rendered screenshots | BLOCKED | Metal run exits at boot: missing Xcode Metal Toolchain component (see Problems) |
| 10 | Performance (fps/frame time) | NOT VERIFIED | `avg_fps 1928.8` is the nullrhi logic tick rate, not render performance. No render claim made |

## Problems encountered (with resolutions)

1. `?game=` run died instantly: `Couldn't spawn player: Failed to spawn player
   controller`. Cause: inherited abstract `ATP_VehicleAdvPlayerController`.
   Fixed harness-side with concrete base PC. No shipped change.
2. Car frozen (thr echo 1.00, speed 0, `sim=0`, rpm 0). Two red herrings
   eliminated with engine-source reads: `IsSimulatingPhysics=false` is
   expected for Chaos-driven bodies, and the `GetSkinnedAsset` ensure is
   non-fatal. Real cause: `BP_SportsCar_Pawn` references
   `/Game/Vehicles/SportsCar/SKM_SportsCar` etc., but the file-level template
   copy lacked the shared `Vehicles` content pack, so the mesh asset was null
   and the vehicle sim had no bodies. Fixed with the manifest-directed copy
   above. Side effect fixed: the Task 1 `LoadErrors` for
   `/Game/Vehicles/PhysicsMaterials/*` are gone (0 in run7).
3. Rendered run blocked: `cannot execute tool 'metal' due to missing Metal
   Toolchain`. The toolchain binary exists but Apple's component receipt is
   absent; `xcodebuild -downloadComponent MetalToolchain` fails fetching the
   catalog (exit 70), and even direct binary invocation refuses. User remedy:
   Xcode Settings, Platforms/Components, install Metal Toolchain (or retry
   the xcodebuild command when reachable), then rerun with `-Task1E2EShots`
   (harness already waits for shader completion before the final shot).

## Commands executed

- Build (incremental, after each harness edit): `Engine/Build/BatchFiles/Mac/Build.sh TP_VehicleAdvEditor Mac Development -project=.../RacingGame.uproject` (exit 0 each time, 12 to 24s).
- Functional: `UnrealEditor .../RacingGame.uproject "/Game/VehicleTemplate/Maps/VehicleBasic?game=/Script/TP_VehicleAdv.Task1E2EGameMode" -game -nullrhi -unattended -nosound -nopause -nosplash -log` (exit 0; JSON at `game/RacingGame/Saved/Task1E2E/results.json`).
- Rendered attempt: same without `-nullrhi`, plus `-Task1E2EShots` (exits at MetalRHI init; toolchain, above).

## What remains unverified

Human driving feel, visual quality, interactive PIE, render performance,
variant maps (TimeTrial/OffRoad), and anything requiring the Metal toolchain
until it is installed. None of these gate Task 2 (unpolished cube-car prototype,
itself headless-testable the same way).

## Recommendation

Release Task 2. The functional Task 1 gate is established with measurements.
Install the Xcode Metal Toolchain in parallel so the rendered pass (already
coded behind `-Task1E2EShots`) can run later.
