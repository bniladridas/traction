// Task 19 E2E probe (verification only, safe to delete).
// First slice of AI racecraft: line-commit overtake only. Stages two
// pace-differentiated AI rivals on frozen parallel lines with the player
// parked clear, then asserts the six frozen t19_* gates against the
// already-public seams only: each rival driver's GetPaceFactor(), frozen
// LineOffset, the racecraft commanded offset, GetProgressDistance(), and
// ARaceManager::GetPosition. Writes no race, manager, lap, or order
// state. Writes Saved/Task19E2E/results.json, then quits.
//
// Thresholds below are frozen in docs/verification/task-19-e2e.md and
// were fixed BEFORE the first run. Tasks 1-18 programs, schemas, and
// thresholds are untouched.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Task19Probe.generated.h"

class ARaceVehicle;
class ARaceTrack;
class ARaceManager;
class URaceAIDriver;

namespace Task19Limits
{
	// Proof field: exactly 3 participants, 1 parked player + 2 AI.
	constexpr int32 FieldSize = 3;
	// Frozen pace tiers (Task 13 PaceSlow/PaceFast verbatim).
	constexpr float PaceDefender = 0.85f;
	constexpr float PaceAttacker = 1.0f;
	// Frozen lines (Task 13 LineSlow/LineFast verbatim). Free side is
	// away from the defender's -120, so commit target is >= +240.
	constexpr float LineDefender = -120.0f;
	constexpr float LineAttacker = 120.0f;
	// Attack window: a rival ahead within this many cm provokes a commit.
	constexpr float AttackWindowCm = 600.0f;
	// Lateral shift committed toward the free side.
	constexpr float CommitShiftCm = 120.0f;
	// Free side sign for the staged attacker (+1 = positive lateral).
	constexpr float FreeSideSign = 1.0f;
	// Progress the attacker may cover past window entry before the
	// commitment must be visible on the commanded offset.
	constexpr float CommitWindowCm = 300.0f;
	// Tolerance when asserting the cede back to the frozen line
	// (Task 18 PosTolCm verbatim).
	constexpr float CedeToleranceCm = 1.0f;
	// Stall detection (Task 13 + Task 18 verbatim).
	constexpr float StallProgressCm = 50.0f;
	constexpr float StallWindow = 20.0f;
	// Overall program timeout, seconds (Task 13 + Task 18 verbatim).
	constexpr float ProgramTimeout = 240.0f;
}

UCLASS()
class RACINGGAME_API ATask19Probe : public AActor
{
	GENERATED_BODY()

public:
	ATask19Probe();

	virtual void BeginPlay() override;
	virtual void Tick(float Delta) override;

private:
	void Finish(bool bOk, const FString& Note);
	void WriteResults(bool bOk, const FString& Note) const;
	void ParkPlayer();
	FString OrderString() const;

	ARaceVehicle* Player = nullptr;
	ARaceVehicle* Defender = nullptr;
	ARaceVehicle* Attacker = nullptr;
	ARaceTrack* Track = nullptr;
	ARaceManager* Manager = nullptr;
	URaceAIDriver* DriverDef = nullptr;
	URaceAIDriver* DriverAtt = nullptr;
	bool bFinished = false;
	double Elapsed = 0.0;
	int32 Frames = 0;

	bool bStartSent = false;
	bool bRacingSeen = false;
	double RacingStartTime = 0.0;
	bool bPlayerParked = false;

	// Attack-window bookkeeping (read-only from both drivers).
	bool bWindowSeen = false;
	// Minimum observed nonnegative absolute separation, cm. -1 means no
	// nonnegative separation has been observed yet.
	float MinSeparationCm = -1.0f;
	int32 WindowHits = 0;
	double WindowEntryTime = 0.0;
	float WindowEntryProgress = 0.0f;
	// Maximum commanded offset observed while in the attack window. This
	// is a max, never overwritten by later cede samples, so the persisted
	// artifact can actually evidence the +240 commit. -1 means the
	// committed offset was never observed at all.
	float MaxCommittedOffsetCm = -1.0f;
	float CedeOffsetCm = 0.0f;
	int32 CommandsOutsideWindow = 0;
	// Count of frames where the outside-window condition actually held.
	// Gate 4 requires this to be > 0 AND zero illegal commands, so the
	// negative case can never pass vacuously.
	int32 OutsideWindowSamples = 0;
	bool bCommitObserved = false;
	bool bCedeObserved = false;
	double LastMoveDef = 0.0;
	double LastMoveAtt = 0.0;
	float LastDistDef = 0.0f;
	float LastDistAtt = 0.0f;
	int32 LastRecDef = 0;
	int32 LastRecAtt = 0;
	bool bDeadlockOk = true;

	// Recorded gate outcomes.
	bool bConfigured = false;   // t19_racecraft_configured
	bool bWindowDetected = false; // t19_attack_window_detected
	bool bCommitOk = false;     // t19_commit_free_side
	bool bNoCommitOk = false;  // t19_no_commit_outside_window
	bool bCedeOk = false;       // t19_cede_back
	bool bScopeOk = false;      // t19_scope_frozen
};
