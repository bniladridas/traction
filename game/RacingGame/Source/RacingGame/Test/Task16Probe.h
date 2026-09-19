// Task 16 E2E probe (verification only, safe to delete).
// Races a driven player plus 5 AI drivers to an all-finished 3-lap
// race, asserting configured laps, all-finish with 3 laps each, results
// populated with 6 valid entries, total order, reset clearing, and
// deadlock freedom. Writes Saved/Task16E2E/results.json, then quits.
// Thresholds below were fixed BEFORE the first passing run. Tasks 2-15
// programs, schemas, and thresholds are untouched.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Task16Probe.generated.h"

class ARaceVehicle;
class ARaceTrack;
class ARaceManager;
class URaceAIDriver;

namespace Task16Limits
{
	// Proof field: exactly 6 participants, 1 driven player + 5 AI.
	constexpr int32 FieldSize = 6;
	// Full-race distance for this run. The struct default stays 2 for
	// all frozen tasks; only this run overrides it via the GameMode.
	constexpr int32 RaceLaps = 3;
	// Stall: under 50 cm over any rolling 20 s while Racing and
	// unfinished; only a respawn with resumed progress clears it.
	constexpr float StallProgressCm = 50.0f;
	constexpr float StallWindow = 20.0f;
	// Mid-program reset time, seconds.
	constexpr float ResetAt = 12.0f;
	// Overall program timeout, seconds.
	constexpr float ProgramTimeout = 400.0f;
}

UCLASS()
class RACINGGAME_API ATask16Probe : public AActor
{
	GENERATED_BODY()

public:
	ATask16Probe();

	virtual void BeginPlay() override;
	virtual void Tick(float Delta) override;

private:
	void Finish(bool bOk, const FString& Note);
	void WriteResults(bool bOk, const FString& Note) const;
	void ParkPlayer();
	FString OrderString() const;

	ARaceVehicle* Player = nullptr;
	ARaceVehicle* AIs[5] = { nullptr, nullptr, nullptr, nullptr, nullptr };
	ARaceTrack* Track = nullptr;
	ARaceManager* Manager = nullptr;
	URaceAIDriver* Drivers[5] = { nullptr, nullptr, nullptr, nullptr, nullptr };
	bool bFinished = false;
	double Elapsed = 0.0;
	int32 Frames = 0;

	bool bStartSent = false;
	bool bRacingSeen = false;
	double RacingStartTime = 0.0;
	bool bPlayerParked = false;
	bool bFieldReady = false;

	bool bResetDone = false;
	bool bWaitRacing = false;

	// Recorded assertions.
	bool bConfigured = false;
	bool bAllFinished = false;
	bool bPopulated = false;
	bool bConsistent = false;
	bool bOrderTotal = false;
	bool bResetCleared = false;
	bool bResetLaps = false;
	bool bDeadlockOk = true;
	FString FinalOrder;
	int32 FinalLaps[6] = { -1, -1, -1, -1, -1, -1 };
	double LastMoveT[5] = { 0.0, 0.0, 0.0, 0.0, 0.0 };
	float LastDist[5] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	int32 LastRec[5] = { 0, 0, 0, 0, 0 };
	double MaxGap = 0.0;
};
