// RacingGame vehicle configuration (Task 4).
// Single authoritative owner for every principal dynamics parameter.
// The movement component consumes this struct and owns no tunables itself.
// A future vehicle variant is a different configuration value, not a new
// actor or movement class. A later pass may store this struct in a data
// asset; the consumers would not change.

#pragma once

#include "CoreMinimal.h"
#include "RaceCameraConfig.h"
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

	// Engine torque curve: X is engine speed in RPM, Y is torque in Nm.
	// Linear interpolation, clamped at the ends. Sized like a small
	// naturally aspirated engine; Task 6 development parameters, not a
	// production claim. Replaces the Task 3 force curve, which was a
	// stand-in removed by this refactor rather than duplicated.
	UPROPERTY(EditAnywhere, Category = "Race|Engine")
	TArray<FVector2D> EngineTorqueCurve = {
		FVector2D(800.0f, 150.0f),
		FVector2D(1000.0f, 180.0f),
		FVector2D(1500.0f, 230.0f),
		FVector2D(2000.0f, 265.0f),
		FVector2D(2500.0f, 300.0f),
		FVector2D(3000.0f, 325.0f),
		FVector2D(3500.0f, 340.0f),
		FVector2D(4000.0f, 335.0f),
		FVector2D(4500.0f, 320.0f),
		FVector2D(5000.0f, 300.0f),
		FVector2D(5500.0f, 275.0f),
		FVector2D(6000.0f, 250.0f),
		FVector2D(6500.0f, 150.0f),
		FVector2D(6800.0f, 0.0f)
	};

	// Engine speed bounds in RPM.
	UPROPERTY(EditAnywhere, Category = "Race|Engine")
	float EngineIdleRPM = 1000.0f;
	UPROPERTY(EditAnywhere, Category = "Race|Engine")
	float EngineRedlineRPM = 6200.0f;
	UPROPERTY(EditAnywhere, Category = "Race|Engine")
	float EngineMaxRPM = 6800.0f;

	// Forward gear ratios, low to high. Sized with the shift points below
	// so the frozen 2.5 s full-throttle window exercises a shift;
	// production calibration is later work.
	UPROPERTY(EditAnywhere, Category = "Race|Transmission")
	TArray<float> GearRatios = { 3.0f, 1.7f, 1.25f, 0.95f };

	UPROPERTY(EditAnywhere, Category = "Race|Transmission")
	float FinalDriveRatio = 3.5f;

	// Shift points in RPM with hysteresis (down strictly below up). Sized
	// for the frozen test program as above, not as production calibration.
	// The upshift lands early enough that the high-speed accel window sits
	// fully in second gear, which is what makes the taper measurable.
	UPROPERTY(EditAnywhere, Category = "Race|Transmission")
	float ShiftUpRPM = 2200.0f;
	UPROPERTY(EditAnywhere, Category = "Race|Transmission")
	float ShiftDownRPM = 1000.0f;

	// Forward speed above which coasting throttle lift produces engine
	// braking, cm/s. Above the lift-window entry speed with margin.
	UPROPERTY(EditAnywhere, Category = "Race|Engine")
	float EngineBrakeMinSpeed = 200.0f;

	// Reverse gear ratio. Reverse pull stays bounded by the speed cap
	// and the friction circle.
	UPROPERTY(EditAnywhere, Category = "Race|Transmission")
	float ReverseRatio = 3.0f;

	// Fraction of shaft torque reaching the wheels.
	UPROPERTY(EditAnywhere, Category = "Race|Transmission")
	float DrivetrainEfficiency = 0.9f;

	// Fixed driven-wheel configuration: indices into Wheels (rear axle).
	UPROPERTY(EditAnywhere, Category = "Race|Transmission")
	TArray<int32> DrivenWheelIndices = { 2, 3 };

	// Rolling radius in meters, used for torque-to-force and RPM mapping.
	UPROPERTY(EditAnywhere, Category = "Race|Wheels")
	float WheelRadiusM = 0.35f;

	// Small engine-braking request in newtons when coasting.
	UPROPERTY(EditAnywhere, Category = "Race|Engine")
	float EngineBrakeForceN = 800.0f;

	// Forward speed above which reverse state releases back to first gear.
	UPROPERTY(EditAnywhere, Category = "Race|Transmission")
	float ForwardEngageSpeed = 150.0f;

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
	// 1 / (1 + (speed / ref)^2). Sized at 600 so the differentiation zone
	// covers the 2 to 8 m/s exercised by the frozen steer windows; a 1200
	// reference left both windows near full authority and the rule
	// unmeasurable. Production calibration is later work.
	UPROPERTY(EditAnywhere, Category = "Race|Steering")
	float SteerRefSpeed = 600.0f;

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

	// Chase camera tuning, consumed by the camera driver component.
	UPROPERTY(EditAnywhere, Category = "Race|Camera")
	FRaceCameraConfig Camera;
};
