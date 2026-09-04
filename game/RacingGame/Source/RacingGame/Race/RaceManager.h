// Race progression (Tasks 8-11).
// Observes registered participant vehicles against the track's
// checkpoint planes and owns phase, ordered progression, lap counting,
// validity, timing, and positions. Participant 0 is the player pawn; AI
// racers register additionally. Legacy single-participant getters
// delegate to participant 0, preserving the Task 8 contract exactly.
// The vehicles know nothing of lap rules; future UI reads the getters
// here.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RaceConfig.h"
#include "RaceManager.generated.h"

UENUM(BlueprintType)
enum class ERacePhase : uint8
{
	Ready,
	Countdown,
	Racing,
	Finished
};

class ARaceTrack;
class ARaceVehicle;

// Per-racer progression state. Phases and the clock stay global.
USTRUCT(BlueprintType)
struct FRaceParticipant
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<ARaceVehicle> Vehicle;

	int32 ExpectIdx = 0;
	bool bLapValid = true;
	bool bLastSequenceValid = true;
	bool bFinished = false;
	int32 FinishSeq = -1;
	int32 CompletedLaps = 0;
	float LapStartTime = 0.0f;
	float LastLapTime = 0.0f;
	float BestLapTime = 0.0f;
	TArray<int32> PlaneArmed;
	TArray<float> PrevPlaneD;
	bool bPrevInit = false;
	TArray<int32> CrossingLog;
	TArray<int32> IgnoredLog;
};

UCLASS()
class RACINGGAME_API ARaceManager : public AActor
{
	GENERATED_BODY()

public:
	ARaceManager();

	UPROPERTY(EditAnywhere, Category = "Race|Rules")
	FRaceConfig RaceConfig;

	// Starts the countdown from Ready. Ignored otherwise.
	void StartRace();

	// Reset contract: clears every participant to Ready and returns the
	// player vehicle to the track-owned start. The game layer calls this
	// together with the vehicle's own reset.
	void OnVehicleReset();

	// AI registration. Returns the participant index. Duplicate
	// registrations return the existing index.
	int32 RegisterParticipant(ARaceVehicle* Participant);

	// Re-anchors one participant's plane tracking to its current position
	// without logging events. Used after respawns and test teleports that
	// are not part of the measured program.
	void ReanchorParticipant(ARaceVehicle* Participant);

	// Legacy single-participant view (participant 0). Unchanged Task 8
	// behavior.
	ERacePhase GetPhase() const { return Phase; }
	int32 GetCompletedLaps() const;
	int32 GetNextCheckpoint() const;
	bool IsCurrentLapValid() const;
	bool WasLastSequenceValid() const;
	float GetLastLapTime() const;
	float GetBestLapTime() const;
	int32 GetCrossingCount() const;
	int32 GetIgnoredCount() const;

	// Multi-participant view for AI and future UI.
	int32 GetParticipantCount() const { return Participants.Num(); }
	ARaceVehicle* GetParticipantVehicle(int32 Index) const;
	int32 GetParticipantLaps(int32 Index) const;
	bool IsParticipantLapValid(int32 Index) const;
	bool WasParticipantLastValid(int32 Index) const;
	bool IsParticipantFinished(int32 Index) const;
	float GetParticipantLastLap(int32 Index) const;

	// Live position, 1-based, deterministic. Finished participants sort
	// by finish sequence; the rest by laps, then along-track distance,
	// then participant index. Returns -1 for unknown racers.
	int32 GetPosition(const ARaceVehicle* Participant) const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	int32 FindParticipant(const ARaceVehicle* Participant) const;
	void AdvanceParticipant(FRaceParticipant& P, float DeltaTime);
	// Signed distance to a checkpoint plane; lateral offset alongside.
	void PlaneMetrics(int32 CpIndex, const FVector& Pos, float& D, float& Lat) const;

	ERacePhase Phase = ERacePhase::Ready;
	float PhaseTime = 0.0f;
	float RaceClock = 0.0f;
	int32 FinishCounter = 0;
	TArray<FRaceParticipant> Participants;

	ARaceTrack* Track = nullptr;
};
