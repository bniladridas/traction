// Task 17 E2E probe (verification only, safe to delete).
// Races the player (probe-driven pursuit, Task 15 pattern) plus five
// AI (Task 14 tiers/lines, proven stable field) while the presentation
// model is bound to the manager. Asserts the six HUD gates: bound,
// countdown shown (phase-derived), lap display, live position display,
// finish display from results, and clears on reset. Writes
// Saved/Task17E2E/results.json, then quits. Thresholds below were
// fixed BEFORE the first passing run. Tasks 2-16 programs, schemas,
// and thresholds are untouched.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Task17Probe.generated.h"

class ARaceVehicle;
class ARaceTrack;
class ARaceManager;
class URaceAIDriver;
class URaceHudModel;
class URaceHudWidget;

namespace Task17Limits
{
	// Proof field: exactly 6 participants, player slot 0 + 5 AI.
	constexpr int32 FieldSize = 6;
	// Full-race distance for this run. The struct default stays 2 for
	// all frozen tasks; only this run overrides it via the GameMode.
	constexpr int32 RaceLaps = 3;
	// Stall: under 50 cm over any rolling 20 s while Racing and
	// unfinished; only a respawn with resumed progress clears it.
	constexpr float StallProgressCm = 50.0f;
	constexpr float StallWindow = 20.0f;
	// Mid-program reset time, seconds.
	constexpr float ResetAt = 12.0f;
	// Overall program timeout, seconds.
	constexpr float ProgramTimeout = 400.0f;
	// Min racing samples with a live lap/position binding before verdict.
	constexpr int32 MinRacingSamples = 5;
}

UCLASS()
class RACINGGAME_API ATask17Probe : public AActor
{
	GENERATED_BODY()

public:
	ATask17Probe();

	virtual void BeginPlay() override;
	virtual void Tick(float Delta) override;

private:
	void Finish(bool bOk, const FString& Note);
	void WriteResults(bool bOk, const FString& Note) const;
	void RestageAll();
	bool bBoundModel() const;
	int32 NearestIndex(const FVector& Pos) const;
	void DrivePlayer();

	ARaceVehicle* Player = nullptr;
	ARaceVehicle* AIs[5] = { nullptr, nullptr, nullptr, nullptr, nullptr };
	ARaceTrack* Track = nullptr;
	ARaceManager* Manager = nullptr;
	URaceAIDriver* Drivers[5] = { nullptr, nullptr, nullptr, nullptr, nullptr };
	URaceHudModel* HudModel = nullptr;
	URaceHudWidget* HudWidget = nullptr;

	bool bFinished = false;
	double Elapsed = 0.0;
	int32 Frames = 0;

	bool bStartSent = false;
	bool bRacingSeen = false;
	bool bInCountdown = false;
	int32 CountdownSamples = 0;
	bool bCountdownOk = false;
	bool bCountdownCleared = false;
	bool bLapLiveSeen = false;
	bool bPosLiveSeen = false;
	int32 RacingSamples = 0;
	bool bResetDone = false;
	bool bResetCleared = false;
	bool bWaitRacing = false;

	// Player pursuit state (Task 15 pattern).
	bool bPlayerAnchored = false;
	int32 PlayerIdx = 0;
	float PlayerS = 0.0f;
	float PlayerCheckT = 0.0f;
	float PlayerCheckS = -1e9f;

	// Recorded assertions.
	bool bBound = false;
	bool bCountdown = false;
	bool bLapDisp = false;
	bool bPosDisp = false;
	bool bFinishDisp = false;
	bool bReset = false;
	bool bDeadlockOk = true;
	double MaxGap = 0.0;
	double LastMoveT[5] = { 0.0, 0.0, 0.0, 0.0, 0.0 };
	float LastDist[5] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	int32 LastRec[5] = { 0, 0, 0, 0, 0 };
};