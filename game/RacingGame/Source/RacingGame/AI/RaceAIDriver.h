// AI test driver (Task 9).
// Game-code pursuit driver: follows the track centerline with
// curvature-aware speed, detects stuck/off-track states, and respawns
// at the nearest centerline pose. Issues only the public Apply*
// commands, exactly like a player or probe would. No new circuit
// definition; everything resolves from ARaceTrack.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RaceAIDriver.generated.h"

class ARaceTrack;
class ARaceManager;
class ARaceVehicle;

UCLASS(ClassGroup = AI, meta = (BlueprintSpawnableComponent))
class RACINGGAME_API URaceAIDriver : public UActorComponent
{
	GENERATED_BODY()

public:
	URaceAIDriver();

	// Lookahead in cm: base plus speed gain.
	UPROPERTY(EditAnywhere, Category = "Race|AI")
	float LookaheadBase = 400.0f;
	UPROPERTY(EditAnywhere, Category = "Race|AI")
	float LookaheadSpeedGain = 0.3f;
	// Steering normalizer in degrees.
	UPROPERTY(EditAnywhere, Category = "Race|AI")
	float SteerGainDeg = 18.0f;
	// Speed targets in cm/s, scaled by PaceFactor.
	UPROPERTY(EditAnywhere, Category = "Race|AI")
	float StraightTarget = 1200.0f;
	UPROPERTY(EditAnywhere, Category = "Race|AI")
	float TurnTarget = 600.0f;
	UPROPERTY(EditAnywhere, Category = "Race|AI")
	float HairpinTarget = 400.0f;
	// Pace multiplier on all speed targets. Per-instance assignment
	// produces lap-time spread through identical physics; 1.0 is nominal.
	UPROPERTY(EditAnywhere, Category = "Race|AI")
	float PaceFactor = 1.0f;
	float GetPaceFactor() const { return PaceFactor; }
	// Lateral line offset in cm applied to the pursuit target (right
	// positive). Lets two rivals run parallel lines so a pace overtake
	// completes without contact. Not a reaction to other cars.
	UPROPERTY(EditAnywhere, Category = "Race|AI")
	float LineOffset = 0.0f;
	// Curvature thresholds in degrees over the lookahead window.
	UPROPERTY(EditAnywhere, Category = "Race|AI")
	float TurnSoftDeg = 12.0f;
	UPROPERTY(EditAnywhere, Category = "Race|AI")
	float TurnHardDeg = 30.0f;
	// Recovery: off-track margin beyond the half width, and stall window.
	UPROPERTY(EditAnywhere, Category = "Race|AI")
	float OfftrackMargin = 200.0f;
	UPROPERTY(EditAnywhere, Category = "Race|AI")
	float StallWindow = 5.0f;

	float GetProgressDistance() const { return UnwrappedS - UnwrappedStart; }
	// Absolute unwrapped centerline distance in cm. Unlike progress, this is
	// not normalized by spawn position and is therefore suitable for staged
	// absolute separation. Read-only.
	float GetUnwrappedDistance() const { return UnwrappedS; }
	int32 GetRecoveryCount() const { return RecoveryCount; }
	bool IsDriving() const { return bDrove; }

	// ---- Task 19 racecraft seam (line-commit overtake, first slice) ----
	// Additive and opt-in. The frozen LineOffset above is NEVER mutated:
	// this layer publishes a separate commanded offset that the pursuit
	// uses when resolving its target. Reads only public seams (ARaceManager
	// participants and each rival driver's absolute unwrapped distance) and
	// writes no race, manager, lap, or order state. Defender behavior,
	// right-of-way, and pace press are out of scope for Task 19.
	UPROPERTY(EditAnywhere, Category = "Race|AI|Racecraft")
	bool bRacecraftEnabled = false;
	// Longitudinal window (cm) inside which a rival ahead provokes a
	// commit. Frozen at contract time.
	UPROPERTY(EditAnywhere, Category = "Race|AI|Racecraft")
	float RacecraftAttackWindowCm = 600.0f;
	// Lateral shift (cm) committed toward the free side when committed.
	UPROPERTY(EditAnywhere, Category = "Race|AI|Racecraft")
	float RacecraftCommitShiftCm = 120.0f;
	// Free side: +1 commits toward positive lateral (right), -1 toward
	// negative (left). Staged, not inferred from other cars.
	UPROPERTY(EditAnywhere, Category = "Race|AI|Racecraft")
	float RacecraftFreeSideSign = 1.0f;

	// LineOffset captured at BeginPlay; the frozen baseline the commanded
	// offset always returns to.
	float GetFrozenLineOffset() const { return FrozenLineOffset; }
	// The commanded offset this layer currently publishes. Equals
	// FrozenLineOffset when not committed.
	float GetCommandedLineOffset() const { return CommandedLineOffset; }
	bool IsRacecraftCommitted() const { return bRacecraftCommitted; }
	// Absolute separation to the nearest rival ahead, cm; negative when none
	// is ahead.
	float GetRacecraftRivalGapCm() const { return RivalGapCm; }

	// Re-arms the committed offset back to the frozen line.
	void ResetRacecraft();

	// Re-anchors tracking to the current position after an external
	// teleport (test staging, reset). Without this, the stall detector
	// compares fresh positions against pre-teleport progress and fires
	// falsely for every car at once.
	void Reanchor();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	int32 NearestIndex(const FVector& Pos) const;
	void Respawn(int32 Idx);

	ARaceTrack* Track = nullptr;
	ARaceManager* Manager = nullptr;
	ARaceVehicle* Vehicle = nullptr;
	int32 LastIdx = -1;
	float UnwrappedS = 0.0f;
	float UnwrappedStart = 0.0f;
	bool bAnchored = false;
	int32 RecoveryCount = 0;
	bool bDrove = false;
	double LastBeat = 0.0;
	float OfftrackTime = 0.0f;
	float CheckTime = 0.0f;
	float CheckS = 0.0f;
	float Clock = 0.0f;

	// Task 19 racecraft seam state (additive).
	float FrozenLineOffset = 0.0f;
	float CommandedLineOffset = 0.0f;
	float RivalGapCm = -1.0f;
	bool bRacecraftCommitted = false;

	// Resolves the commanded offset from a read-only rival scan.
	void UpdateRacecraft(float DeltaTime);
};
