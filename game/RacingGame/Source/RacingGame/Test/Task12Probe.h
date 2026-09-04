// Task 12 E2E probe (verification only, safe to delete).
// Races a 3-car field (player driven by pursuit, 2 AI drivers) with a
// mid-program reset, asserting grid, progress, laps, finish order, and
// no-deadlock for every AI. Writes Saved/Task12E2E/results.json, then
// quits. Thresholds below were fixed BEFORE the first passing run.
// Stall is defined here, not tuned later: while Racing and unfinished,
// an AI whose driven distance advances under 50 cm over any rolling
// 20 s window is stalled; recovery means a driver respawn followed by
// resumed progress. Tasks 2-11 programs, schemas, thresholds untouched.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Task12Probe.generated.h"

class ARaceVehicle;
class ARaceTrack;
class ARaceManager;
class URaceAIDriver;

namespace Task12Limits
{
	// Proof field: exactly 3 participants, 1 player + 2 AI.
	constexpr int32 FieldSize = 3;
	// Grid slots pairwise distinct by at least 150 cm.
	constexpr float GridMinSeparationCm = 150.0f;
	// Each AI covers 3000 cm within 40 s of Racing.
	constexpr float ProgressMinCm = 3000.0f;
	constexpr float ProgressWindow = 40.0f;
	// Stall window and recovery: under 50 cm over any rolling 20 s while
	// Racing and unfinished is a stall; only a driver respawn with
	// resumed progress clears it.
	constexpr float StallProgressCm = 50.0f;
	constexpr float StallWindow = 20.0f;
	// Mid-program reset time, seconds.
	constexpr float ResetAt = 12.0f;
	// Overall program timeout, seconds.
	constexpr float ProgramTimeout = 240.0f;
}

UCLASS()
class RACINGGAME_API ATask12Probe : public AActor
{
	GENERATED_BODY()

public:
	ATask12Probe();

	virtual void BeginPlay() override;
	virtual void Tick(float Delta) override;

private:
	void Finish(bool bOk, const FString& Note);
	void WriteResults(bool bOk, const FString& Note) const;
	int32 NearestIndex(const FVector& Pos) const;
	void DrivePlayer();
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

	int32 PlayerIdx = -1;
	float PlayerS = 0.0f;
	bool bPlayerAnchored = false;

	bool bResetDone = false;
	bool bWaitRacing = false;

	// Recorded assertions.
	FString GridOrder;
	bool bGridOrderOk = false;
	bool bGridCountOk = false;
	bool bProgressLogged = false;
	double ProgressTime = 0.0;
	FString ResetOrder;
	bool bResetOrderOk = false;
	bool bResetLaps = false;
	FString FinalOrder;
	int32 LapsP = -1;
	int32 Laps0 = -1;
	int32 Laps1 = -1;
	bool bDeadlockOk = true;
	double ProgressTime0 = -1.0;
	double ProgressTime1 = -1.0;
	double LastMoveT0 = 0.0;
	double LastMoveT1 = 0.0;
	float LastDist0 = 0.0f;
	float LastDist1 = 0.0f;
	int32 LastRec0 = 0;
	int32 LastRec1 = 0;
	double MaxGap = 0.0;
};
