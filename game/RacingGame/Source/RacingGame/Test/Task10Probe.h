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
	// No pops (legacy per-tick criterion, now diagnostic only): per-tick
	// camera world displacement under 50 cm outside the reset window.
	constexpr float PopMaxCm = 50.0f;
	// No pops (revised contract): every evaluated fixed pawn-travel
	// window must keep camera/pawn travel ratio at or under this cap.
	// Frozen from the three-rate measurement matrix (observed 1.66-4.41x
	// across uncapped/30/10 FPS; genuine teleports read 10x and above).
	constexpr float WindowTravelRatioCap = 8.0f;
	// Pawn-travel window size for the revised pop measurement.
	constexpr float PawnTravelWindowCm = 100.0f;
	// Look-ahead: turn-arc floor replacing the old sample-count floor.
	// Accumulated heading traversal during eligible turn sampling;
	// frozen from the three-rate matrix (observed 202.8-222.2 deg).
	constexpr float TurnArcFloorDeg = 150.0f;
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
	void TakeShot(const FString& FileName, const FString& Phase);

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
	// Turn-arc and per-window travel-ratio measurement (revised Task 10
	// contract). Frame-rate independent by construction: arc integrates
	// heading over time, windows partition by pawn travel, not ticks.
	double TurnArcDegSum = 0.0;
	float WinCamP = 0.0f;
	float WinPawnP = 0.0f;
	float WinCamA = 0.0f;
	float WinPawnA = 0.0f;
	float WinRatioMaxP = 0.0f;
	float WinRatioMaxA = 0.0f;
	int32 WinCountP = 0;
	int32 WinCountA = 0;
	FVector LastCamP = FVector::ZeroVector;
	FVector LastCamA = FVector::ZeroVector;
	bool bHaveLast = false;
	float ResetPosErr = -1.0f;
	float ResetYawErr = -1.0f;
	bool bResetMeasured = false;
	bool bShotsEnabled = false;
	bool bShotStart = false;
	bool bShotSweeper = false;
	bool bShotHairpin = false;
	bool bShotRace = false;
	TArray<FString> ShotEntries;
};
