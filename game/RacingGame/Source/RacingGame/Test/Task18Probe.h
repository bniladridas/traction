// Task 18 E2E probe (verification only, safe to delete).
// Proves the deterministic two-view contract: chase (Task 10, frozen)
// and a rigid cockpit. The probe drives the player (Task 15 pursuit)
// against a single AI rival over a full 2-lap race while asserting the
// six gates: configured default view, CycleView toggling (plus the
// RaceView input binding on the pawn), rigid cockpit attachment (fixed
// offset on the root, no spring arm, no control rotation), data-driven
// pitch and FOV applied from the camera config, view preserved across
// ResetVehicle, and full race compatibility. Writes
// Saved/Task18E2E/results.json, then quits. Thresholds below were
// fixed BEFORE the first passing run. Tasks 1-17 programs, schemas,
// and thresholds are untouched; Task 10 chase behavior is not
// restructured.
//
// The input binding is asserted statically: the probe walks the pawn's
// InputComponent action bindings for "RaceView" and reports whether the
// binding exists on the pawn. SetupPlayerInputComponent maps the action
// to OnCycleView, which calls the same CycleView() the probe drives, so
// the presence of the binding plus a functional CycleView closes the
// real player path without injecting synthetic key events.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Task18Probe.generated.h"

class ARaceVehicle;
class ARaceTrack;
class ARaceManager;
class URaceAIDriver;

namespace Task18Limits
{
	// Proof field: exactly player slot 0 + one AI rival.
	constexpr int32 FieldSize = 2;
	// Full-race distance for this run.
	constexpr int32 RaceLaps = 2;
	// Camera config the probe applies via the vehicle's SetCameraConfig
	// knob. Non-default pitch and FOV prove the fields flow from config,
	// not from hardcoded engineering values.
	constexpr float CockpitPitchDeg = 8.0f;
	constexpr float CockpitFov = 72.0f;
	// Geometry tolerances for the rigid cockpit checks, degrees/cm.
	constexpr float AngleTolDeg = 1.0f;
	constexpr float PosTolCm = 1.0f;
	// Stall: under 50 cm over any rolling 20 s while Racing and
	// unfinished; only a respawn with resumed progress clears it.
	constexpr float StallProgressCm = 50.0f;
	constexpr float StallWindow = 20.0f;
	// Overall program timeout, seconds.
	constexpr float ProgramTimeout = 240.0f;
	// Min successful in-race view toggles for the race-compatible gate.
	constexpr int32 MinRaceToggles = 3;
}

UCLASS()
class RACINGGAME_API ATask18Probe : public AActor
{
	GENERATED_BODY()

public:
	ATask18Probe();

	virtual void BeginPlay() override;
	virtual void Tick(float Delta) override;

private:
	void Finish(bool bOk, const FString& Note);
	void WriteResults(bool bOk, const FString& Note) const;
	bool bHasRaceViewBinding() const;
	int32 NearestIndex(const FVector& Pos) const;
	void DrivePlayer();

	ARaceVehicle* Player = nullptr;
	ARaceVehicle* AI = nullptr;
	ARaceTrack* Track = nullptr;
	ARaceManager* Manager = nullptr;
	URaceAIDriver* Driver = nullptr;

	bool bFinished = false;
	double Elapsed = 0.0;
	int32 Frames = 0;

	bool bGatesStarted = false;
	bool bStartSent = false;
	bool bRacingSeen = false;
	bool bWaitRacing = false;
	int32 RaceToggles = 0;

	// Player pursuit state (Task 15 pattern).
	bool bPlayerAnchored = false;
	int32 PlayerIdx = 0;
	float PlayerS = 0.0f;
	float PlayerCheckT = 0.0f;
	float PlayerCheckS = -1e9f;

	// Recorded assertions.
	bool bViewConfigured = false;
	bool bViewToggle = false;
	bool bCockpitRigid = false;
	bool bPitchFov = false;
	bool bResetView = false;
	bool bRaceCompatible = false;
	bool bDeadlockOk = true;
	double MaxGap = 0.0;
	double LastMoveT = 0.0;
	float LastDist = 0.0f;
	int32 LastRec = 0;
};