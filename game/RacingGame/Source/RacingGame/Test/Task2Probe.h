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

namespace Task3Limits
{
	// All thresholds below were fixed before the first Task 3 passing run.
	// Gravity/contact: never below 1 cm under rest height, grounded or
	// wheel contact in at least 90 percent of samples.
	constexpr float MinZCm = 39.0f;
	constexpr float GroundedFrac = 0.9f;
	constexpr float WheelFrac = 0.9f;
	// Mass/acceleration: peak forward accel over 300 cm/s^2 with visible
	// engine-curve taper (late-window accel under 90 percent of early).
	constexpr float AccelPeak = 300.0f;
	constexpr float TaperMaxRatio = 0.9f;
	// Braking: peak decel over 500 cm/s^2 in the brake window.
	constexpr float DecelPeak = 500.0f;
	// Reverse: speed magnitude never over 105 percent of the 700 limit.
	constexpr float RevMaxAbs = 735.0f;
	// Steering rule: normalized high-speed yaw rate under 80 percent of
	// the normalized low-speed rate (normalized = rate / steer input).
	constexpr float SteerRatioMax = 0.8f;
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
	// Task 4 architecture evidence: configuration audit plus regression
	// flag copies. Separate artifact; the Task 2/3 schema is untouched.
	void WriteTask4Artifact(bool bFwd, bool bBrake, bool bRev, bool bSteer, bool bCam, bool bReset,
		bool bGrav, bool bMass, bool bBrakeF, bool bRevB, bool bSteerR, bool bWheels) const;
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

	// Task 3 dynamics metrics (additive; Task 2 fields above untouched).
	float MinZ = 1e9f;
	int32 GroundSamples = 0;
	int32 TotalSamples = 0;
	int32 WheelAllSamples = 0;
	float VAt10 = 0.0f, VAt15 = 0.0f, VAt25 = 0.0f, VAt30 = 0.0f;
	bool bGotV10 = false, bGotV15 = false, bGotV25 = false, bGotV30 = false;
	float PeakAccel = 0.0f;
	float PeakDecel = 0.0f;
	float MaxRevAbs = 0.0f;
	float LastV = 0.0f;
	double LastT = 0.0;
	bool bHaveLast = false;
	float YawAt85 = 0.0f, YawAt100 = 0.0f, YawAt115 = 0.0f, YawAt130 = 0.0f;
	bool bGotY85 = false, bGotY100 = false, bGotY115 = false, bGotY130 = false;
};
