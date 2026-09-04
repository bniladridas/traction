// Task 11 E2E probe (verification only, safe to delete).
// Stages two participants through grid, progress, lap dominance,
// overtake, reset, and finish scenarios with hop-by-hop checkpoint
// teleports, asserting the manager's position order at each stage.
// Writes Saved/Task11E2E/results.json, then quits.
// Thresholds below were fixed BEFORE the first passing run. Tasks 2-10
// programs, schemas, and thresholds are untouched.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Task11Probe.generated.h"

class ARaceVehicle;
class ARaceTrack;
class ARaceManager;

namespace Task11Limits
{
	// Dwell per scripted teleport step, seconds.
	constexpr float StepDwell = 0.35f;
	// Overall program timeout, seconds.
	constexpr float ProgramTimeout = 150.0f;
}

UCLASS()
class RACINGGAME_API ATask11Probe : public AActor
{
	GENERATED_BODY()

public:
	ATask11Probe();

	virtual void BeginPlay() override;
	virtual void Tick(float Delta) override;

private:
	struct FStep
	{
		ARaceVehicle* Who = nullptr;
		FVector Loc = FVector::ZeroVector;
		FRotator Rot = FRotator::ZeroRotator;
		int32 Tag = 0;
	};
	enum ETag
	{
		TagNone = 0,
		TagAssertGrid,
		TagAssertProgress,
		TagAssertDominance,
		TagAssertOvertake,
		TagDoReset,
		TagAssertReset,
		TagAssertFinish,
		TagFinish
	};

	void Finish(bool bOk, const FString& Note);
	void WriteResults(bool bOk, const FString& Note) const;
	void StagePose(ARaceVehicle* Who, int32 CpIndex, float Along);
	void AppendCrossing(ARaceVehicle* Who, int32 CpIndex);
	void AppendSequence(ARaceVehicle* Who);
	FString OrderString() const;

	ARaceVehicle* Player = nullptr;
	ARaceVehicle* AI = nullptr;
	ARaceTrack* Track = nullptr;
	ARaceManager* Manager = nullptr;
	bool bFinished = false;
	double Elapsed = 0.0;
	int32 Frames = 0;

	bool bStartSent = false;
	bool bRacingSeen = false;
	bool bStepping = false;
	bool bWaitRacing = false;
	TArray<FStep> Steps;
	int32 StepIdx = 0;
	double NextStepTime = 0.0;

	// Recorded assertions (orders as participant-index strings).
	FString GridOrder;
	FString ProgressOrder;
	FString DominanceOrder;
	FString OvertakeOrder;
	FString ResetOrder;
	bool bResetLaps = false;
	FString FinishOrder;
	int32 LapsAtFinish0 = -1;
	int32 LapsAtFinish1 = -1;
	FString LockedOrder;
};
