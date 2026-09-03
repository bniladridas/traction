// Task 7 E2E probe (verification only, safe to delete).
// Validates the track layer (geometry, centerline, checkpoints, start,
// contact) then drives a deterministic centerline-following lap and writes
// Saved/Task7E2E/results.json. Never touches Task2Probe or earlier schemas.
// Thresholds below were fixed BEFORE the first passing run.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Task7Probe.generated.h"

class ARaceVehicle;
class ARaceTrack;

namespace Task7Limits
{
	// Track exists with real road collision behind it.
	constexpr int32 RoadSegMin = 20;
	// Every settle-window sample has all four wheels in contact.
	constexpr float ContactFrac = 1.0f;
	// Track-owned spawn: vehicle sits on the start pose.
	constexpr float StartPosMaxErrCm = 50.0f;
	constexpr float StartYawMaxErrDeg = 5.0f;
	// Centerline is ordered, closed, and measures a real circuit.
	constexpr int32 CenterPtsMin = 24;
	constexpr float ClosureMaxCm = 5.0f;
	constexpr float LengthMinCm = 5000.0f;
	// Lap distance agrees with the measured centerline within 15 percent.
	constexpr float LapDistTol = 0.15f;
	// Deterministic program bounds.
	constexpr float DriveStart = 1.5f;
	constexpr float LapTimeout = 60.0f;
}

UCLASS()
class RACINGGAME_API ATask7Probe : public AActor
{
	GENERATED_BODY()

public:
	ATask7Probe();

	virtual void BeginPlay() override;
	virtual void Tick(float Delta) override;

private:
	void Finish(bool bOk, const FString& Note);
	void WriteResults(bool bOk, const FString& Note) const;
	int32 NearestCenterIndex(const FVector& Pos) const;
	static float NormYaw(float Deg);

	ARaceVehicle* Vehicle = nullptr;
	ARaceTrack* Track = nullptr;
	bool bFinished = false;
	double Elapsed = 0.0;
	int32 Frames = 0;

	// Validation samples.
	int32 ContactOk = 0;
	int32 ContactN = 0;
	float StartPosErr = -1.0f;
	float StartYawErr = -1.0f;
	bool bStartMeasured = false;

	// Lap state.
	bool bDriving = false;
	float UnwrappedS = 0.0f;
	float UnwrappedStart = 0.0f;
	int32 LastIdx = -1;
	TArray<int32> CrossedSeq;
	int32 ExpectIdx = 0;
	float PrevPlaneD = 0.0f;
	bool bLapDone = false;
	float LapDist = 0.0f;
	FVector LapStartLoc = FVector::ZeroVector;
};
