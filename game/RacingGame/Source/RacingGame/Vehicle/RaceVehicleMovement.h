// RacingGame-owned prototype vehicle movement (Task 2).
// Deliberately simple and deterministic: scalar forward speed, yaw steering
// scaled by speed, flat-surface sweep. No suspension, gears, tires, or aero.
// Task 3 replaces the accel/brake model with engine/torque/gears while
// keeping this component's interface (inputs in, transform out).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
#include "RaceVehicleMovement.generated.h"

UCLASS(ClassGroup = Movement, meta = (BlueprintSpawnableComponent))
class RACINGGAME_API URaceVehicleMovement : public UPawnMovementComponent
{
	GENERATED_BODY()

public:
	// cm/s^2 gained at full throttle.
	UPROPERTY(EditAnywhere, Category = "Race|Drive")
	float AccelRate = 800.0f;

	// cm/s^2 removed while braking a forward-moving car.
	UPROPERTY(EditAnywhere, Category = "Race|Drive")
	float BrakeRate = 1500.0f;

	// cm/s^2 gained backward while holding brake near standstill.
	UPROPERTY(EditAnywhere, Category = "Race|Drive")
	float ReverseAccel = 700.0f;

	// Forward speed cap, cm/s.
	UPROPERTY(EditAnywhere, Category = "Race|Drive")
	float MaxSpeed = 2500.0f;

	// Reverse speed cap, cm/s (positive value).
	UPROPERTY(EditAnywhere, Category = "Race|Drive")
	float MaxReverseSpeed = 700.0f;

	// Yaw rate at full steering and full grip, degrees per second.
	UPROPERTY(EditAnywhere, Category = "Race|Steering")
	float TurnRate = 90.0f;

	// Speed at which steering reaches full authority, cm/s.
	UPROPERTY(EditAnywhere, Category = "Race|Steering")
	float FullGripSpeed = 600.0f;

	// Passive slowdown, cm/s^2.
	UPROPERTY(EditAnywhere, Category = "Race|Drive")
	float RollingDrag = 80.0f;

	// Speed below which brake input engages reverse, cm/s.
	UPROPERTY(EditAnywhere, Category = "Race|Drive")
	float ReverseEngageSpeed = 60.0f;

public:
	void SetThrottle(float Value) { ThrottleInput = FMath::Clamp(Value, 0.0f, 1.0f); }
	void SetBrake(float Value) { BrakeInput = FMath::Clamp(Value, 0.0f, 1.0f); }
	void SetSteering(float Value) { SteeringInput = FMath::Clamp(Value, -1.0f, 1.0f); }

	float GetForwardSpeed() const { return ForwardSpeed; }
	float GetThrottleInput() const { return ThrottleInput; }

	void ResetSpeed() { ForwardSpeed = 0.0f; }

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	float ForwardSpeed = 0.0f;
	float ThrottleInput = 0.0f;
	float BrakeInput = 0.0f;
	float SteeringInput = 0.0f;
};
