// RacingGame vehicle dynamics foundation (Task 3).
// Force-based evolution of the Task 2 kinematic model. Same interface
// (inputs in, transform out), same public API plus read-only state for
// verification. Scalar longitudinal model + vertical gravity with ground
// contact + four traced wheel contact points + speed-sensitive steering.
// Units: centimeters, seconds, kilograms, newtons. A newton accelerates
// one kilogram at one m/s^2, so accel(cm/s^2) = force(N) / mass(kg) * 100.
// Integration is semi-implicit Euler at frame delta; Task 4 may move to
// fixed substeps. No suspension, tires, gears, or aero beyond linear and
// quadratic drag. Those arrive in later milestones.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
#include "RaceVehicleMovement.generated.h"

UCLASS(ClassGroup = Movement, meta = (BlueprintSpawnableComponent))
class RACINGGAME_API URaceVehicleMovement : public UPawnMovementComponent
{
	GENERATED_BODY()

public:
	// Vehicle mass in kilograms. Acceleration scales as force / mass.
	UPROPERTY(EditAnywhere, Category = "Race|Mass")
	float MassKg = 1200.0f;

	// Gravitational acceleration, cm/s^2. Unreal default is 980.
	UPROPERTY(EditAnywhere, Category = "Race|Gravity")
	float GravityCmS2 = 980.0f;

	// Engine force curve: X is forward speed in cm/s, Y is force in
	// newtons. Linear interpolation between points, clamped at the ends.
	// Placeholder for a later RPM/torque model, which replaces only the
	// evaluation, not the vehicle actor.
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

	// Reverse drive force in newtons, bounded by MaxReverseSpeed.
	UPROPERTY(EditAnywhere, Category = "Race|Brakes")
	float ReverseForceN = 3000.0f;

	// Reverse speed cap, cm/s (positive value).
	UPROPERTY(EditAnywhere, Category = "Race|Brakes")
	float MaxReverseSpeed = 700.0f;

	// Forward speed hard cap, cm/s. The engine curve tapers before this.
	UPROPERTY(EditAnywhere, Category = "Race|Engine")
	float MaxSpeed = 5000.0f;

	// Yaw rate at standstill reference, degrees per second at full steer.
	UPROPERTY(EditAnywhere, Category = "Race|Steering")
	float SteerYawLow = 110.0f;

	// Speed where steering authority halves, cm/s. Authority follows
	// 1 / (1 + (speed / ref)^2): full control when slow, calmer at speed.
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

	// Wheel layout: half wheelbase (X), half track (Y), wheel center
	// height (Z, relative to actor origin), all cm.
	UPROPERTY(EditAnywhere, Category = "Race|Wheels")
	float WheelBaseHalf = 70.0f;
	UPROPERTY(EditAnywhere, Category = "Race|Wheels")
	float TrackHalf = 75.0f;
	UPROPERTY(EditAnywhere, Category = "Race|Wheels")
	float WheelRestZ = -30.0f;

	// Downward trace length below each wheel point, cm.
	UPROPERTY(EditAnywhere, Category = "Race|Wheels")
	float WheelTraceLength = 50.0f;

public:
	void SetThrottle(float Value) { ThrottleInput = FMath::Clamp(Value, 0.0f, 1.0f); }
	void SetBrake(float Value) { BrakeInput = FMath::Clamp(Value, 0.0f, 1.0f); }
	void SetSteering(float Value) { SteeringInput = FMath::Clamp(Value, -1.0f, 1.0f); }

	float GetForwardSpeed() const { return ForwardSpeed; }
	float GetVerticalSpeed() const { return VerticalSpeed; }
	float GetThrottleInput() const { return ThrottleInput; }
	float GetMassKg() const { return MassKg; }
	bool IsGrounded() const { return bGrounded; }
	bool GetWheelContact(int32 Index) const;

	void ResetSpeed() { ForwardSpeed = 0.0f; VerticalSpeed = 0.0f; }

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	float EvalEngineForce(float SpeedCmS) const;
	void UpdateWheelContact();

	float ForwardSpeed = 0.0f;
	float VerticalSpeed = 0.0f;
	float ThrottleInput = 0.0f;
	float BrakeInput = 0.0f;
	float SteeringInput = 0.0f;
	bool bGrounded = false;
	bool WheelContact[4] = { false, false, false, false };
};
