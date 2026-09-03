// Temporary Task 2 E2E probe (verification only, safe to delete).
// Exercises the real vehicle input path (ApplyThrottle/ApplyBrake/
// ApplySteering/ResetVehicle: the same functions the keys call), records
// transforms, checks predefined thresholds, writes
// Saved/Task2E2E/results.json, then quits.
// Thresholds below were fixed BEFORE the first passing run.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Task2Probe.generated.h"

class ARaceVehicle;
class UCameraComponent;

namespace Task2Limits
{
	// Forward: displacement over 0.5 m, within 10 deg of initial forward.
	constexpr float FwdMinDispCm = 50.0f;
	constexpr float FwdMaxAngleDeg = 10.0f;
	// Brake: needs speed over 5 m/s entering, must fall below half.
	constexpr float BrakeMinEnterSpeed = 500.0f;
	constexpr float BrakeMaxRatio = 0.5f;
	// Reverse: over 0.5 m opposite the initial forward direction.
	constexpr float RevMinDispCm = 50.0f;
	// Steering: straight-run noise must stay under 2 deg for the run to
	// count; steer phase must exceed 5 deg.
	constexpr float StraightMaxNoiseDeg = 2.0f;
	constexpr float SteerMinDeltaDeg = 5.0f;
	// Camera: must travel over half the pawn distance, attached to pawn.
	constexpr float CamMinRatio = 0.5f;
	// Reset: position under 1 cm, yaw under 0.5 deg, speed under 1 cm/s.
	constexpr float ResetMaxPosErrCm = 1.0f;
	constexpr float ResetMaxYawErrDeg = 0.5f;
	constexpr float ResetMaxSpeed = 1.0f;
}

UCLASS()
class RACINGGAME_API ATask2Probe : public AActor
{
	GENERATED_BODY()

public:
	ATask2Probe();

	virtual void BeginPlay() override;
	virtual void Tick(float Delta) override;

private:
	void Finish(bool bOk, const FString& Note);
	void WriteResults(bool bOk, const FString& Note) const;
	// Rendered-capture infrastructure (Task 2 screenshots addition).
	// Records real renderer frames only; under nullrhi no PNGs materialize.
	// Capture code is compiled and built now; exercised once the Metal
	// Toolchain limitation is resolved. Never synthesizes images.
	void TakeShot(const FString& FileName, const FString& Phase);

	ARaceVehicle* Vehicle = nullptr;
	UCameraComponent* ChaseCam = nullptr;
	bool bFinished = false;
	double Elapsed = 0.0;
	double LastSample = -1.0;
	int32 Frames = 0;
	double FrameTimeSum = 0.0;

	FVector StartLoc = FVector::ZeroVector;
	FRotator StartRot = FRotator::ZeroRotator;
	FVector CamStart = FVector::ZeroVector;
	bool bStartRecorded = false;

	FVector FwdEndLoc = FVector::ZeroVector;
	float FwdEndSpeed = 0.0f;
	float StraightNoiseDeg = 0.0f;
	float BrakeEnterSpeed = 0.0f;
	float BrakeEndSpeed = 0.0f;
	FVector RevStartLoc = FVector::ZeroVector;
	FVector RevEndLoc = FVector::ZeroVector;
	FRotator SteerStartRot = FRotator::ZeroRotator;
	FRotator SteerEndRot = FRotator::ZeroRotator;
	bool bSteerCaptured = false;
	FVector CamEnd = FVector::ZeroVector;
	FVector PawnSteerEndLoc = FVector::ZeroVector;
	bool bSteerEndCaptured = false;
	bool bShotsEnabled = false;
	bool bFinalShotTaken = false;
	TArray<FString> ShotEntries;
	float ResetPosErr = -1.0f;
	float ResetYawErr = -1.0f;
	float ResetSpeed = -1.0f;
	bool bResetDone = false;
};
