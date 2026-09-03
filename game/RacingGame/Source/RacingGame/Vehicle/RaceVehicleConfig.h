// RacingGame vehicle configuration (Task 4).
// Single authoritative owner for every principal dynamics parameter.
// The movement component consumes this struct and owns no tunables itself.
// A future vehicle variant is a different configuration value, not a new
// actor or movement class. A later pass may store this struct in a data
// asset; the consumers would not change.

#pragma once

#include "CoreMinimal.h"
#include "RaceVehicleConfig.generated.h"

// Normalized driving commands. The movement system receives only this;
// it never inspects keyboard keys or input actions.
USTRUCT(BlueprintType)
struct FRaceDriveCommand
{
	GENERATED_BODY()

	// 0 to 1, forward drive request.
	UPROPERTY(EditAnywhere, Category = "Race|Command")
	float Throttle = 0.0f;

	// 0 to 1, brake request (engages reverse near standstill).
	UPROPERTY(EditAnywhere, Category = "Race|Command")
	float Brake = 0.0f;

	// -1 to 1, steering request.
	UPROPERTY(EditAnywhere, Category = "Race|Command")
	float Steering = 0.0f;
};

// One wheel contact definition. Position only; the Task 4 model traces
// straight down from it. Suspension and tire data belong to Task 5+.
USTRUCT(BlueprintType)
struct FRaceWheelConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Race|Wheel")
	FName Name = NAME_None;

	// Local offset from the actor origin, cm.
	UPROPERTY(EditAnywhere, Category = "Race|Wheel")
	FVector LocalOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Race|Wheel")
	bool bFrontAxle = false;

	UPROPERTY(EditAnywhere, Category = "Race|Wheel")
	bool bLeftSide = false;
};

USTRUCT(BlueprintType)
struct FRaceVehicleConfig
{
	GENERATED_BODY()

	// Vehicle mass in kilograms. Acceleration scales as force / mass.
	UPROPERTY(EditAnywhere, Category = "Race|Mass")
	float MassKg = 1200.0f;

	// Gravitational acceleration, cm/s^2.
	UPROPERTY(EditAnywhere, Category = "Race|Gravity")
	float GravityCmS2 = 980.0f;

	// Engine force curve: X is forward speed in cm/s, Y is force in
	// newtons. Linear interpolation, clamped at the ends. Migrated
	// unchanged from the Task 3 movement defaults.
	UPROPERTY(EditAnywhere, Category = "Race|Engine")
	TArray<FVector2D> EngineForcePoints = {
		FVector2D(0.0f, 9000.0f),
		FVector2D(1000.0f, 8200.0f),
		FVector2D(2000.0f, 6400.0f),
		FVector2D(3000.0f, 3800.0f),
		FVector2D(4000.0f, 1000.0f),
		FVector2D(4500.0f, 0.0f)
	};

	// Brake force in newtons, applied against motion.
	UPROPERTY(EditAnywhere, Category = "Race|Brakes")
	float BrakeForceN = 14000.0f;

	// Reverse drive force in newtons, bounded by MaxReverseSpeed. Sized
	// strong with a low cap, like a short reverse gear: brisk pull that
	// the speed cap and the friction circle keep bounded.
	UPROPERTY(EditAnywhere, Category = "Race|Brakes")
	float ReverseForceN = 6000.0f;

	// Reverse speed cap, cm/s (positive value).
	UPROPERTY(EditAnywhere, Category = "Race|Brakes")
	float MaxReverseSpeed = 700.0f;

	// Forward speed hard cap, cm/s. The engine curve tapers before this.
	UPROPERTY(EditAnywhere, Category = "Race|Engine")
	float MaxSpeed = 5000.0f;

	// Yaw rate at low speed, degrees per second at full steer.
	UPROPERTY(EditAnywhere, Category = "Race|Steering")
	float SteerYawLow = 110.0f;

	// Speed where steering authority halves, cm/s. Authority follows
	// 1 / (1 + (speed / ref)^2).
	UPROPERTY(EditAnywhere, Category = "Race|Steering")
	float SteerRefSpeed = 1200.0f;

	// Speed below which steering produces no yaw, cm/s.
	UPROPERTY(EditAnywhere, Category = "Race|Steering")
	float SteerDeadSpeed = 100.0f;

	// Constant rolling resistance in newtons.
	UPROPERTY(EditAnywhere, Category = "Race|Drag")
	float RollingResistanceN = 250.0f;

	// Quadratic drag coefficient in N per (m/s)^2.
	UPROPERTY(EditAnywhere, Category = "Race|Drag")
	float AeroDragCoeff = 0.35f;

	// Speed below which brake input engages reverse, cm/s.
	UPROPERTY(EditAnywhere, Category = "Race|Drive")
	float ReverseEngageSpeed = 60.0f;

	// Downward trace length below each wheel point, cm.
	UPROPERTY(EditAnywhere, Category = "Race|Wheels")
	float WheelTraceLength = 50.0f;

	// Four wheel definitions. Roles are data, not code branches.
	UPROPERTY(EditAnywhere, Category = "Race|Wheels")
	TArray<FRaceWheelConfig> Wheels = {
		{ FName(TEXT("FL")), FVector(70.0f, 75.0f, -30.0f), true, true },
		{ FName(TEXT("FR")), FVector(70.0f, -75.0f, -30.0f), true, false },
		{ FName(TEXT("RL")), FVector(-70.0f, 75.0f, -30.0f), false, true },
		{ FName(TEXT("RR")), FVector(-70.0f, -75.0f, -30.0f), false, false }
	};

	// Suspension rest length, cm: hardpoint-to-contact distance at zero
	// load. Sized so static compression absorbs roughly half the travel.
	UPROPERTY(EditAnywhere, Category = "Race|Suspension")
	float SuspRestLengthCm = 20.0f;

	// Suspension travel limits, cm.
	UPROPERTY(EditAnywhere, Category = "Race|Suspension")
	float SuspMaxCompressionCm = 12.0f;
	UPROPERTY(EditAnywhere, Category = "Race|Suspension")
	float SuspMaxExtensionCm = 8.0f;

	// Spring stiffness in N/m.
	UPROPERTY(EditAnywhere, Category = "Race|Suspension")
	float SpringStiffnessNpm = 30000.0f;

	// Damping coefficient in N s/m, opposing compression velocity.
	UPROPERTY(EditAnywhere, Category = "Race|Suspension")
	float DampingCoeffNspm = 3000.0f;

	// Tire friction coefficient for the combined-force circle.
	UPROPERTY(EditAnywhere, Category = "Race|Tire")
	float FrictionMu = 1.0f;

	// Lateral stiffness in N per (m/s) of lateral slip velocity.
	UPROPERTY(EditAnywhere, Category = "Race|Tire")
	float LateralStiffness = 8000.0f;

	// Front-wheel steer angle at full steering input, degrees. Positive
	// input steers toward vehicle right (+Y). Speed sensitivity lives in
	// the yaw rule, not in this geometric angle.
	UPROPERTY(EditAnywhere, Category = "Race|Steering")
	float MaxSteerAngleDeg = 30.0f;
};
