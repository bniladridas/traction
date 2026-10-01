// Task 20 E2E probe (verification only, safe to delete).
// Second slice of AI racecraft: defend-the-line only. Stages two
// pace-differentiated AI rivals on frozen parallel lines with the player
// parked clear, then asserts the six frozen t20_* gates against the
// already-public seams only: each rival driver's GetPaceFactor(), frozen
// LineOffset, the commanded offsets, GetUnwrappedDistance(), and
// ARaceManager::GetPosition. Writes no race, manager, lap, or order
// state. Writes Saved/Task20E2E/results.json, then quits.
//
// Thresholds below are frozen in docs/verification/task-20-e2e.md and
// were fixed BEFORE the first run. Tasks 1-19 programs, schemas, and
// thresholds are untouched.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Task20Probe.generated.h"

class ARaceVehicle;
class ARaceTrack;
class ARaceManager;
class URaceAIDriver;

namespace Task20Limits
{
	// Proof field: exactly 3 participants, 1 parked player + 2 AI.
	constexpr int32 FieldSize = 3;
	// Frozen pace tiers (Task 13 PaceSlow/PaceFast verbatim).
	constexpr float PaceDefender = 0.85f;
	constexpr float PaceAttacker = 1.0f;
	// Frozen lines (Task 13 LineSlow/LineFast verbatim). The defensive
	// target is LineDefender + DefensiveShiftCm = 0 (centerline).
	constexpr float LineDefender = -120.0f;
	constexpr float LineAttacker = 120.0f;
	// Rear attack window: an attacker behind within this many cm provokes
	// a defensive response. Same derivation as Task 19 AttackWindowCm.
	constexpr float RearWindowCm = 600.0f;
	// Lateral shift toward the attacker side when defending.
	constexpr float DefensiveShiftCm = 120.0f;
	// Defense direction for the staged defender (+1 = positive lateral).
	constexpr float DefenseDirectionSign = 1.0f;
	// Progress the defender may cover past window entry before the
	// defensive command must be visible on the commanded offset.
	constexpr float DefenseWindowCm = 300.0f;
	// Tolerance when asserting commanded lines
	// (Task 18 PosTolCm verbatim).
	constexpr float CedeToleranceCm = 1.0f;
	// Stall detection (Task 13 + Task 18 verbatim).
	constexpr float StallProgressCm = 50.0f;
	constexpr float StallWindow = 20.0f;
	// Overall program timeout, seconds (Task 13 + Task 18 verbatim).
	constexpr float ProgramTimeout = 240.0f;
}

UCLASS()
class RACINGGAME_API ATask20Probe : public AActor
{
	GENERATED_BODY()

public:
	ATask20Probe();

	virtual void BeginPlay() override;
	virtual void Tick(float Delta) override;

private:
	void Finish(bool bOk, const FString& Note);
	void WriteResults(bool bOk, const FString& Note) const;
	void ParkPlayer();

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

	// Rear-window bookkeeping (read-only from both drivers).
	bool bWindowSeen = false;
	float MinSeparationCm = -1.0f;
	int32 WindowHits = 0;
	double WindowEntryTime = 0.0;
	float WindowEntryProgress = 0.0f;
	// In-window commanded defensive offset. Latched only while the
	// commanded value is on the defensive target, never overwritten by
	// later cede samples. Stays negative when never observed.
	float DefensiveOffsetCm = -1.0f;
	float CedeOffsetCm = 0.0f;
	// Maximum defender commanded offset over ALL racing ticks. Must never
	// exceed the 0 cm centerline bound (gate 5 second half).
	float MaxDefenderCommandedCm = -1000000.0f;
	int32 CommandsOutsideWindow = 0;
	// Count of frames where the outside-window condition actually held.
	// Gate 4 requires this to be > 0 AND zero illegal commands, so the
	// negative case can never pass vacuously.
	int32 OutsideWindowSamples = 0;
	// Minimum attacker-minus-defender commanded separation over
	// simultaneous-commit samples only (attacker +240 AND defender 0).
	// Stays negative when no such sample was observed.
	float MinCommandedSeparationCm = -1.0f;
	bool bDefenseObserved = false;
	bool bCedeObserved = false;
	bool bSimulObserved = false;
	double LastMoveDef = 0.0;
	double LastMoveAtt = 0.0;
	float LastDistDef = 0.0f;
	float LastDistAtt = 0.0f;
	int32 LastRecDef = 0;
	int32 LastRecAtt = 0;
	bool bDeadlockOk = true;

	// Recorded gate outcomes.
	bool bConfigured = false;   // t20_racecraft_configured
	bool bWindowDetected = false; // t20_defense_window_detected
	bool bDefenseOk = false;    // t20_defensive_line_committed
	bool bNoDefenseOk = false;  // t20_no_defense_outside_window
	bool bCedeOk = false;       // t20_defense_holds_centerline
	bool bScopeOk = false;      // t20_scope_frozen
};
