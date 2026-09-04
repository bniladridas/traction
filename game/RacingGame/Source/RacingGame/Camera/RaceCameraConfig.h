// RacingGame camera configuration (Task 10).
// Owns chase-camera tuning. The component reads the pawn transform and
// motion only; it never writes movement state.

#pragma once

#include "CoreMinimal.h"
#include "RaceCameraConfig.generated.h"

USTRUCT(BlueprintType)
struct FRaceCameraConfig
{
	GENERATED_BODY()

	// Spring-arm length in cm at standstill.
	UPROPERTY(EditAnywhere, Category = "Race|Camera")
	float ArmLength = 500.0f;

	// Arm length gained per cm/s of speed, capped by MaxArmLength.
	UPROPERTY(EditAnywhere, Category = "Race|Camera")
	float ArmSpeedGain = 0.05f;
	UPROPERTY(EditAnywhere, Category = "Race|Camera")
	float MaxArmLength = 700.0f;

	// Camera height above the pawn origin, cm.
	UPROPERTY(EditAnywhere, Category = "Race|Camera")
	float HeightOffset = 140.0f;

	// Base pitch in degrees (negative looks down).
	UPROPERTY(EditAnywhere, Category = "Race|Camera")
	float BasePitchDeg = -14.0f;

	// Pitch change per cm/s of speed, degrees (negative tips down).
	UPROPERTY(EditAnywhere, Category = "Race|Camera")
	float PitchSpeedGain = -0.0005f;

	// Look-ahead: yaw offset per deg/s of pawn yaw rate, clamped.
	UPROPERTY(EditAnywhere, Category = "Race|Camera")
	float LookaheadGain = 0.35f;
	UPROPERTY(EditAnywhere, Category = "Race|Camera")
	float LookaheadMaxDeg = 25.0f;

	// Smoothing rate per second (exponential ease toward targets).
	UPROPERTY(EditAnywhere, Category = "Race|Camera")
	float SmoothingRate = 6.0f;

	// Teleport detection: larger pawn jumps snap instead of smoothing.
	UPROPERTY(EditAnywhere, Category = "Race|Camera")
	float TeleportSnapDist = 200.0f;
};
