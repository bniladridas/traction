// Task 13 E2E probe (verification only, safe to delete).
// Races two pace-differentiated AI rivals with the player parked clear,
// asserting assignment, lap-time spread, an emerged overtake, pace-ranked
// finish, deadlock freedom, and reset clearing. Writes
// Saved/Task13E2E/results.json, then quits. Thresholds below were fixed
// BEFORE the first passing run. Deterministic outcome claimed only
// conditional on fixed ordering, spawn, pace factors, and conditions.
// Tasks 2-12 programs, schemas, and thresholds are untouched.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Task13Probe.generated.h"

class ARaceVehicle;
class ARaceTrack;
class ARaceManager;
class URaceAIDriver;

namespace Task13Limits
{
	// Proof field: exactly 3 participants, 1 parked player + 2 AI.
	constexpr int32 FieldSize = 3;
	// Frozen pace tiers: slot 1 (ahead) slower, slot 2 (behind) faster.
	constexpr float PaceSlow = 0.85f;
	constexpr float PaceFast = 1.0f;
	// Frozen race lines: parallel offsets so the pace overtake completes
	// without contact (slot 1 left, slot 2 right).
	constexpr float LineSlow = -120.0f;
	constexpr float LineFast = 120.0f;
	// Faster best lap beats slower best by over 1 s (engine time).
	constexpr float LapMargin = 1.0f;
	// Each AI covers 3000 cm within 40 s of Racing.
	constexpr float ProgressMinCm = 3000.0f;
	constexpr float ProgressWindow = 40.0f;
	// Stall: under 50 cm over any rolling 20 s while Racing and
	// unfinished; only a respawn with resumed progress clears it.
	constexpr float StallProgressCm = 50.0f;
	constexpr float StallWindow = 20.0f;
	// Mid-program reset time, seconds.
	constexpr float ResetAt = 12.0f;
	// Overall program timeout, seconds.
	constexpr float ProgramTimeout = 240.0f;
}

UCLASS()
class RACINGGAME_API ATask13Probe : public AActor
{
	GENERATED_BODY()

public:
	ATask13Probe();

	virtual void BeginPlay() override;
	virtual void Tick(float Delta) override;

private:
	void Finish(bool bOk, const FString& Note);
	void WriteResults(bool bOk, const FString& Note) const;
	void ParkPlayer();
	FString OrderString() const;

	ARaceVehicle* Player = nullptr;
	ARaceVehicle* AI0 = nullptr;
	ARaceVehicle* AI1 = nullptr;
	ARaceTrack* Track = nullptr;
	ARaceManager* Manager = nullptr;
	URaceAIDriver* Driver0 = nullptr;
	URaceAIDriver* Driver1 = nullptr;
	bool bFinished = false;
	double Elapsed = 0.0;
	int32 Frames = 0;

	bool bStartSent = false;
	bool bRacingSeen = false;
	double RacingStartTime = 0.0;
	bool bPlayerParked = false;

	bool bResetDone = false;
	bool bWaitRacing = false;

	// Recorded assertions.
	bool bPaceOk = false;
	FString EarlyOrder;
	bool bEarlyOk = false;
	bool bProgressLogged = false;
	double ProgressTime = 0.0;
	FString LateOrder;
	bool bOvertakeOk = false;
	FString FinalOrder;
	int32 Laps1 = -1;
	int32 Laps2 = -1;
	float Best1 = 0.0f;
	float Best2 = 0.0f;
	bool bFinishOk = false;
	bool bSpreadOk = false;
	bool bDeadlockOk = true;
	bool bResetOk = false;
	FString ResetOrder;
	bool bResetOrderOk = false;
	bool bResetLaps = false;
	double LastMoveT0 = 0.0;
	double LastMoveT1 = 0.0;
	float LastDist0 = 0.0f;
	float LastDist1 = 0.0f;
	int32 LastRec0 = 0;
	int32 LastRec1 = 0;
	double MaxGap = 0.0;
};
