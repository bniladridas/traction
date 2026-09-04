// Task 8 E2E probe (verification only, safe to delete).
// Exercises the race manager through a scripted teleport program on the
// circuit map: Ready, countdown, Racing, ordered progression, deliberate
// wrong-order and double crossings, reset, clean laps to finish, and the
// finished lock. Writes Saved/Task8E2E/results.json, then quits.
// Thresholds below were fixed BEFORE the first passing run. Task 7 and
// earlier programs, schemas, and thresholds are untouched.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Task8Probe.generated.h"

class ARaceVehicle;
class ARaceTrack;
class ARaceManager;

namespace Task8Limits
{
	// Countdown observed within 5 s of StartRace; Racing within 8 s.
	constexpr float CountdownWindow = 5.0f;
	constexpr float RacingWindow = 8.0f;
	// Dwell per scripted teleport step, seconds.
	constexpr float StepDwell = 0.4f;
	// Reset returns the vehicle to the track start within 50 cm.
	constexpr float ResetPosMaxCm = 50.0f;
	// Overall program timeout, seconds.
	constexpr float ProgramTimeout = 120.0f;
}

UCLASS()
class RACINGGAME_API ATask8Probe : public AActor
{
	GENERATED_BODY()

public:
	ATask8Probe();

	virtual void BeginPlay() override;
	virtual void Tick(float Delta) override;

private:
	struct FStep
	{
		FVector Loc = FVector::ZeroVector;
		FRotator Rot = FRotator::ZeroRotator;
		int32 Tag = 0;
	};
	enum ETag
	{
		TagNone = 0,
		TagAssertSeqA,
		TagDoReset,
		TagAssertReset,
		TagAssertSeqB,
		TagAssertSeqC,
		TagAssertExtras,
		TagFinish
	};

	void Finish(bool bOk, const FString& Note);
	void WriteResults(bool bOk, const FString& Note) const;
	void StagePose(int32 CpIndex, float Along, FVector& Loc, FRotator& Rot) const;
	void AppendCrossing(int32 CpIndex);
	void AppendSequence(bool bClean);

	ARaceVehicle* Vehicle = nullptr;
	ARaceTrack* Track = nullptr;
	ARaceManager* Manager = nullptr;
	bool bFinished = false;
	double Elapsed = 0.0;
	int32 Frames = 0;

	bool bStartRaceSent = false;
	double StartRaceTime = 0.0;
	bool bSawReady = false;
	bool bSawCountdown = false;
	bool bSawRacing = false;
	double SawCountdownTime = 0.0;
	double SawRacingTime = 0.0;

	TArray<FStep> Steps;
	int32 StepIdx = 0;
	double NextStepTime = 0.0;
	bool bStepping = false;
	bool bWaitRacing = false;

	// Recorded assertions.
	bool bSeqAOrder = false;
	bool bSeqAWrongIgnored = false;
	int32 LapsAfterA = -1;
	bool bInvalidAfterA = false;
	bool bResetPhase = false;
	bool bResetLaps = false;
	bool bResetNext = false;
	float ResetPosErr = -1.0f;
	int32 LapsAfterB = -1;
	bool bValidAfterB = false;
	int32 LapsAfterC = -1;
	bool bFinishedAfterC = false;
	float LastLap = 0.0f;
	float BestLap = 0.0f;
	int32 LapsAfterExtras = -1;
};
