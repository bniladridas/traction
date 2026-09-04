// Task 9 E2E probe (verification only, safe to delete).
// Drives the player with centerline pursuit, forces the AI off-track
// once to exercise recovery, and asserts AI participation through the
// race manager. Writes Saved/Task9E2E/results.json, then quits.
// Thresholds below were fixed BEFORE the first passing run. Tasks 2-8
// programs, schemas, and thresholds are untouched.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Task9Probe.generated.h"

class ARaceVehicle;
class ARaceTrack;
class ARaceManager;
class URaceAIDriver;

namespace Task9Limits
{
	// AI covers 3000 cm within 40 s of Racing.
	constexpr float ProgressMinCm = 3000.0f;
	constexpr float ProgressWindow = 40.0f;
	// After the forced off-track event the AI is back in-lane and moving
	// within 10 s.
	constexpr float RecoveryWindow = 10.0f;
	// Overall program timeout, seconds.
	constexpr float ProgramTimeout = 150.0f;
}

UCLASS()
class RACINGGAME_API ATask9Probe : public AActor
{
	GENERATED_BODY()

public:
	ATask9Probe();

	virtual void BeginPlay() override;
	virtual void Tick(float Delta) override;

private:
	void Finish(bool bOk, const FString& Note);
	void WriteResults(bool bOk, const FString& Note) const;
	int32 NearestIndex(const FVector& Pos) const;
	void ParkPlayer();

	ARaceVehicle* Player = nullptr;
	ARaceVehicle* AI = nullptr;
	ARaceTrack* Track = nullptr;
	ARaceManager* Manager = nullptr;
	URaceAIDriver* AIDriver = nullptr;
	int32 AIIndex = -1;
	bool bFinished = false;
	double Elapsed = 0.0;
	int32 Frames = 0;

	bool bStartSent = false;
	bool bRacingSeen = false;
	double RacingStartTime = 0.0;

	// Parked player (off the AI line, on the road, zero input).
	bool bPlayerParked = false;

	// Forced recovery script.
	bool bForced = false;
	double ForceTime = 0.0;
	int32 RecoveriesBefore = 0;
	bool bRecovered = false;
	double RecoverTime = 0.0;
	bool bProgressLogged = false;
	double ProgressTime = 0.0;
	double DoneAt = -1.0;
};
