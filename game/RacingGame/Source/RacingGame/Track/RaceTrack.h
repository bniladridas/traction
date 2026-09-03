// First drivable circuit (Task 7).
// Builds road collision, boundary walls, centerline, checkpoints, and the
// start pose from FRaceTrackConfig at BeginPlay. The vehicle never knows
// about this actor; it only meets road collision through wheel traces.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RaceTrackConfig.h"
#include "RaceTrack.generated.h"

UCLASS()
class RACINGGAME_API ARaceTrack : public AActor
{
	GENERATED_BODY()

public:
	ARaceTrack();

	// Authoritative configuration. Everything below derives from it.
	UPROPERTY(EditAnywhere, Category = "Race|Track")
	FRaceTrackConfig TrackConfig;

	// Derived data, built once in BeginPlay.
	const TArray<FRaceTrackCenterPoint>& GetCenterPoints() const { return CenterPoints; }
	const TArray<FRaceTrackCheckpoint>& GetCheckpoints() const { return Checkpoints; }
	float GetTrackLength() const { return TrackLength; }
	int32 GetRoadSegmentCount() const { return RoadSegmentCount; }
	FVector GetStartPosition() const { return StartPosition; }
	float GetStartYawDeg() const { return StartYawDeg; }

	// Centerline sample at wrapped distance S. Public for test drivers.
	FRaceTrackCenterPoint SampleAtDistance(float S) const;

protected:
	virtual void BeginPlay() override;

private:
	void BuildTrack();

	TArray<FRaceTrackCenterPoint> CenterPoints;
	TArray<FRaceTrackCheckpoint> Checkpoints;
	float TrackLength = 0.0f;
	int32 RoadSegmentCount = 0;
	FVector StartPosition = FVector::ZeroVector;
	float StartYawDeg = 0.0f;

	UPROPERTY()
	UStaticMesh* CubeMesh = nullptr;
};
