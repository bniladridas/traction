// Race progression (Task 8).
// Observes the vehicle against the track's checkpoint planes and owns
// phase, ordered progression, lap counting, validity, and timing. The
// vehicle knows nothing of lap rules; future UI reads the getters here.

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

	// Reset contract: clears progress to Ready and returns the vehicle to
	// the track-owned start. The game layer calls this together with the
	// vehicle's own reset; the vehicle itself stays track-unaware.
	void OnVehicleReset();

	// Read-only state for future UI and verification.
	ERacePhase GetPhase() const { return Phase; }
	int32 GetCompletedLaps() const { return CompletedLaps; }
	int32 GetNextCheckpoint() const { return ExpectIdx; }
	bool IsCurrentLapValid() const { return bLapValid; }
	bool WasLastSequenceValid() const { return bLastSequenceValid; }
	float GetLastLapTime() const { return LastLapTime; }
	float GetBestLapTime() const { return BestLapTime; }
	int32 GetCrossingCount() const { return CrossingLog.Num(); }
	int32 GetIgnoredCount() const { return IgnoredLog.Num(); }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	void AdvanceRacing(float DeltaTime);
	// Signed distance to a checkpoint plane; lateral offset alongside.
	void PlaneMetrics(int32 CpIndex, const FVector& Pos, float& D, float& Lat) const;

	ERacePhase Phase = ERacePhase::Ready;
	float PhaseTime = 0.0f;
	int32 CompletedLaps = 0;
	int32 ExpectIdx = 0;
	bool bLapValid = true;
	bool bLastSequenceValid = true;
	float LapStartTime = 0.0f;
	float LastLapTime = 0.0f;
	float BestLapTime = 0.0f;
	float RaceClock = 0.0f;
	TArray<int32> PlaneArmed;
	TArray<float> PrevPlaneD;
	bool bPrevInit = false;
	TArray<int32> CrossingLog;
	TArray<int32> IgnoredLog;

	ARaceTrack* Track = nullptr;
	ARaceVehicle* Vehicle = nullptr;
};
