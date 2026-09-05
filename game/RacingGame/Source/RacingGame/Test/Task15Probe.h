// Task 15 E2E probe (verification only, safe to delete).
// Races a driven player plus 5 AI drivers to an all-finished race,
// asserting the results snapshot is empty before finish, populated at
// finish, internally consistent, cleared by reset, and immutable after.
// Writes Saved/Task15E2E/results.json, then quits. Thresholds below
// were fixed BEFORE the first passing run. "Last lap" throughout means
// the participant's recorded final valid lap time. Tasks 2-14 programs,
// schemas, and thresholds are untouched.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Task15Probe.generated.h"

class ARaceVehicle;
class ARaceTrack;
class ARaceManager;
class URaceAIDriver;

namespace Task15Limits
{
	// Proof field: exactly 6 participants, 1 driven player + 5 AI.
	constexpr int32 FieldSize = 6;
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
class RACINGGAME_API ATask15Probe : public AActor
{
	GENERATED_BODY()

public:
	ATask15Probe();

	virtual void BeginPlay() override;
	virtual void Tick(float Delta) override;

private:
	void Finish(bool bOk, const FString& Note);
	void WriteResults(bool bOk, const FString& Note) const;
	int32 NearestIndex(const FVector& Pos) const;
	void DrivePlayer();
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

	int32 PlayerIdx = -1;
	float PlayerS = 0.0f;
	bool bPlayerAnchored = false;

	bool bResetDone = false;
	bool bWaitRacing = false;

	// Recorded assertions.
	bool bEmptyChecked = false;
	bool bEmptyEarly = false;
	bool bPopulated = false;
	bool bConsistent = false;
	bool bResetLaps = false;
	bool bResetCleared = false;
	bool bImmutableArmed = false;
	bool bImmutableCrossed = false;
	bool bImmutableStaged = false;
	bool bImmutable = false;
	double ImmutableStageTime = 0.0;
	double ImmutableCheckAt = 0.0;
	int32 SnapLaps[6] = { -1, -1, -1, -1, -1, -1 };
	FString SnapOrder;
	int32 SnapResultOrder[6] = { -1, -1, -1, -1, -1, -1 };
	float SnapBest[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	float SnapLast[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	bool bDeadlockOk = true;
	double LastMoveT[5] = { 0.0, 0.0, 0.0, 0.0, 0.0 };
	float LastDist[5] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	int32 LastRec[5] = { 0, 0, 0, 0, 0 };
	double MaxGap = 0.0;
	double PlayerCheckT = 0.0;
	float PlayerCheckS = 0.0f;
};
