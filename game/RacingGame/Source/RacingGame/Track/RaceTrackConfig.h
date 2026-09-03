// RacingGame track configuration (Task 7).
// Single authoritative owner for the first circuit's data. Geometry,
// collision, centerline, checkpoints, and start pose are all derived from
// the control polygon below, so no duplicate constants can diverge.

#pragma once

#include "CoreMinimal.h"
#include "RaceTrackConfig.generated.h"

// One ordered centerline sample. Position at road-top height, forward along
// travel direction, distance accumulated from the control start.
USTRUCT(BlueprintType)
struct FRaceTrackCenterPoint
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "Race|Track")
	FVector Position = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, Category = "Race|Track")
	FVector Forward = FVector::ForwardVector;

	UPROPERTY(VisibleAnywhere, Category = "Race|Track")
	float Distance = 0.0f;
};

// One ordered checkpoint. Crossing is measured against the plane through
// Position along Forward; lateral bound is Width.
USTRUCT(BlueprintType)
struct FRaceTrackCheckpoint
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "Race|Track")
	int32 Index = -1;

	UPROPERTY(VisibleAnywhere, Category = "Race|Track")
	FVector Position = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, Category = "Race|Track")
	FVector Forward = FVector::ForwardVector;

	UPROPERTY(VisibleAnywhere, Category = "Race|Track")
	float Width = 0.0f;
};

USTRUCT(BlueprintType)
struct FRaceTrackConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Race|Track")
	FString TrackName = TEXT("TestCircuit1");

	// Drivable width in cm. Collision boxes, walls, and checkpoints derive
	// from this single value. Sized at 800 so parallel sections clear
	// each other with margin (verified offline, see the report).
	UPROPERTY(EditAnywhere, Category = "Race|Track")
	float TrackWidth = 800.0f;

	// Road top surface height in cm. Collision box tops sit exactly here.
	UPROPERTY(EditAnywhere, Category = "Race|Track")
	float RoadTopZ = 0.0f;

	// Boundary wall height above the road and thickness, cm.
	UPROPERTY(EditAnywhere, Category = "Race|Track")
	float WallHeight = 120.0f;
	UPROPERTY(EditAnywhere, Category = "Race|Track")
	float WallThickness = 100.0f;

	// Closed-loop control polygon (X, Y at road-top height). Last point
	// repeats the first to mark closure explicitly. Loop order: main
	// straight, east sweeper (r=800 at (1500,0)), back straight, west
	// hairpin (r=700 at (-2300,450)). Arc points are 22.5-degree
	// subdivisions of the true circles; the closing span is a short chord.
	// Geometry verified offline for wall/lane clearance before use.
	UPROPERTY(EditAnywhere, Category = "Race|Track")
	TArray<FVector> ControlPoints = {
		FVector(-2300.0f, -250.0f, 0.0f),
		FVector(1500.0f, -800.0f, 0.0f),
		FVector(1806.1f, -739.1f, 0.0f),
		FVector(2065.7f, -565.7f, 0.0f),
		FVector(2239.1f, -306.1f, 0.0f),
		FVector(2300.0f, 0.0f, 0.0f),
		FVector(2239.1f, 306.1f, 0.0f),
		FVector(2065.7f, 565.7f, 0.0f),
		FVector(1806.1f, 739.1f, 0.0f),
		FVector(1500.0f, 800.0f, 0.0f),
		FVector(-2300.0f, 1150.0f, 0.0f),
		FVector(-2567.9f, 1096.7f, 0.0f),
		FVector(-2795.0f, 945.0f, 0.0f),
		FVector(-2946.7f, 717.9f, 0.0f),
		FVector(-3000.0f, 450.0f, 0.0f),
		FVector(-2946.7f, 182.1f, 0.0f),
		FVector(-2795.0f, -45.0f, 0.0f),
		FVector(-2567.9f, -196.7f, 0.0f),
		FVector(-2300.0f, -250.0f, 0.0f)
	};

	// Target subdivision length in cm. Spans divide into pieces of at most
	// this length so pursuit indices map to roughly uniform distance.
	UPROPERTY(EditAnywhere, Category = "Race|Track")
	float SubdivisionLength = 250.0f;

	// Checkpoint count, evenly spaced by centerline distance.
	UPROPERTY(EditAnywhere, Category = "Race|Track")
	int32 CheckpointCount = 8;

	// Start/finish line distance along the centerline from control start.
	UPROPERTY(EditAnywhere, Category = "Race|Track")
	float StartLineDistance = 500.0f;

	// Spawn sits this far behind the start line so the finish plane starts
	// with a negative signed distance.
	UPROPERTY(EditAnywhere, Category = "Race|Track")
	float SpawnBackoff = 100.0f;
};
