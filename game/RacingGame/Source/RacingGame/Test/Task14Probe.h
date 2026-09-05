// Task 14 E2E probe (verification only, safe to delete).
// Races a 6-car field (player parked, 5 tiered-pace AI drivers) with a
// mid-program reset, asserting grid, progress, laps, total finish order,
// deadlock freedom, and reset clearing. Writes
// Saved/Task14E2E/results.json, then quits. Thresholds below were fixed
// BEFORE the first passing run. Tasks 2-13 programs, schemas, and
// thresholds are untouched.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Task14Probe.generated.h"

class ARaceVehicle;
class ARaceTrack;
class ARaceManager;
class URaceAIDriver;

namespace Task14Limits
{
	// Proof field: exactly 6 participants, 1 parked player + 5 AI.
	constexpr int32 FieldSize = 6;
	// Grid slots pairwise distinct by at least 150 cm (proven 224+).
	constexpr float GridMinSeparationCm = 150.0f;
	// Frozen pace tiers per slot 1..5 (ahead to back).
	constexpr float PaceTiers[5] = { 0.85f, 1.0f, 0.9f, 0.95f, 1.05f };
	// Frozen race lines per slot 1..5.
	constexpr float LineTiers[5] = { -120.0f, 120.0f, -120.0f, 120.0f, -120.0f };
	// Each AI covers 3000 cm within 60 s of Racing.
	constexpr float ProgressMinCm = 3000.0f;
	constexpr float ProgressWindow = 60.0f;
	// Stall: under 50 cm over any rolling 20 s while Racing and
	// unfinished; only a respawn with resumed progress clears it.
	constexpr float StallProgressCm = 50.0f;
	constexpr float StallWindow = 20.0f;
	// Mid-program reset time, seconds.
	constexpr float ResetAt = 12.0f;
	// Overall program timeout, seconds.
	constexpr float ProgramTimeout = 300.0f;
}

UCLASS()
class RACINGGAME_API ATask14Probe : public AActor
{
	GENERATED_BODY()

public:
	ATask14Probe();

	virtual void BeginPlay() override;
	virtual void Tick(float Delta) override;

private:
	void Finish(bool bOk, const FString& Note);
	void WriteResults(bool bOk, const FString& Note) const;
	void ParkPlayer();
	FString OrderString() const;
	FString LapsStr() const;

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

	bool bResetDone = false;
	bool bWaitRacing = false;

	// Recorded assertions.
	bool bFieldReady = false;
	bool bPaceOk = false;
	bool bGridCountOk = false;
	FString GridOrder;
	bool bGridOrderOk = false;
	bool bProgressLogged = false;
	double ProgressTime = 0.0;
	FString ResetOrder;
	bool bResetOrderOk = false;
	bool bResetLaps = false;
	FString FinalOrder;
	int32 FinalLaps[6] = { -1, -1, -1, -1, -1, -1 };
	bool bDeadlockOk = true;
	double LastMoveT[5] = { 0.0, 0.0, 0.0, 0.0, 0.0 };
	float LastDist[5] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	int32 LastRec[5] = { 0, 0, 0, 0, 0 };
	double MaxGap = 0.0;
};
