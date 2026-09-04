// Task 10 E2E probe (verification only, safe to delete).
// Drives the player with centerline pursuit while the AI races, resets
// the player mid-run, and measures chase-camera behavior for both cars:
// follow travel, look-ahead lead, per-tick displacement (no pops), and
// reset snap. Writes Saved/Task10E2E/results.json, then quits.
// Thresholds below were fixed BEFORE the first passing run. Tasks 2-9
// programs, schemas, and thresholds are untouched.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Task10Probe.generated.h"

class ARaceVehicle;
class ARaceTrack;
class ARaceManager;
class UCameraComponent;
class URaceChaseCamera;

namespace Task10Limits
{
	// Follow: camera travel over half the pawn travel each.
	constexpr float FollowMinRatio = 0.5f;
	// Look-ahead: mean signed relative yaw (with turn direction) over 1
	// degree across turning samples.
	constexpr float LeadMinDeg = 1.0f;
	constexpr float TurnSampleMinRate = 15.0f;
	// No pops: per-tick camera world displacement under 50 cm outside
	// the reset window.
	constexpr float PopMaxCm = 50.0f;
	// Reset: offset error under 30 cm and yaw error under 5 deg.
	constexpr float ResetMaxPosCm = 30.0f;
	constexpr float ResetMaxYawDeg = 5.0f;
	// Program: reset at 20 s, measure until 38 s, finish at 42 s.
	constexpr float ResetAt = 20.0f;
	constexpr float MeasureEnd = 38.0f;
	constexpr float FinishAt = 42.0f;
}

UCLASS()
class RACINGGAME_API ATask10Probe : public AActor
{
	GENERATED_BODY()

public:
	ATask10Probe();

	virtual void BeginPlay() override;
	virtual void Tick(float Delta) override;

private:
	void Finish(bool bOk, const FString& Note);
	void WriteResults(bool bOk, const FString& Note) const;
	int32 NearestIndex(const FVector& Pos) const;
	void DrivePlayer();

	ARaceVehicle* Player = nullptr;
	ARaceVehicle* AI = nullptr;
	ARaceTrack* Track = nullptr;
	ARaceManager* Manager = nullptr;
	UCameraComponent* PlayerCam = nullptr;
	UCameraComponent* AICam = nullptr;
	URaceChaseCamera* PlayerDriver = nullptr;
	class URaceAIDriver* AIDriver = nullptr;
	int32 LastAIRecoveries = 0;
	double AIExcludeUntil = 0.0;
	bool bFinished = false;
	double Elapsed = 0.0;
	int32 Frames = 0;

	bool bStartSent = false;
	bool bRacingSeen = false;

	int32 PlayerIdx = -1;
	float PlayerS = 0.0f;
	bool bPlayerAnchored = false;

	bool bResetDone = false;
	bool bInitRecorded = false;
	FVector InitCamOffset = FVector::ZeroVector;
	float InitCamYawErr = 0.0f;
	float CamPathP = 0.0f;
	float PawnPathP = 0.0f;
	float CamPathA = 0.0f;
	float PawnPathA = 0.0f;
	FVector PrevCamP = FVector::ZeroVector;
	FVector PrevPawnP = FVector::ZeroVector;
	FVector PrevCamA = FVector::ZeroVector;
	FVector PrevPawnA = FVector::ZeroVector;
	float LeadSum = 0.0f;
	int32 LeadN = 0;
	float MaxPopP = 0.0f;
	float MaxPopA = 0.0f;
	FVector LastCamP = FVector::ZeroVector;
	FVector LastCamA = FVector::ZeroVector;
	bool bHaveLast = false;
	float ResetPosErr = -1.0f;
	float ResetYawErr = -1.0f;
	bool bResetMeasured = false;
};
