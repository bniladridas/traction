// RacingGame vehicle dynamics (Tasks 3 to 5).
// Task 5 evolution: per-wheel runtime state (contact, suspension,
// normal load, tire forces), spring-damper suspension from configuration,
// and bounded longitudinal/lateral tire forces on a friction circle.
// Body motion keeps the Task 3 scalar longitudinal model plus a lateral
// velocity state; yaw integration is unchanged. Units: cm, s, kg, N.
// Conventions: body frame X forward, Y right, Z up; positive steering
// turns the front wheels toward +Y; lateral force opposes lateral slip.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
#include "RaceVehicleConfig.h"
#include "RaceVehicleMovement.generated.h"

// Runtime state for one wheel. Forces in newtons, lengths in cm unless
// noted. Read-only to verification through movement getters.
USTRUCT(BlueprintType)
struct FRaceWheelState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "Race|Wheel")
	bool bContact = false;

	UPROPERTY(VisibleAnywhere, Category = "Race|Wheel")
	FVector ContactPoint = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, Category = "Race|Wheel")
	FVector ContactNormal = FVector::UpVector;

	// Suspension compression in cm, positive when compressed.
	UPROPERTY(VisibleAnywhere, Category = "Race|Wheel")
	float Compression = 0.0f;

	// Compression velocity in cm/s, positive when compressing.
	UPROPERTY(VisibleAnywhere, Category = "Race|Wheel")
	float CompressionVel = 0.0f;

	// Normal load in newtons carried by this wheel.
	UPROPERTY(VisibleAnywhere, Category = "Race|Wheel")
	float NormalLoad = 0.0f;

	// Applied longitudinal force in newtons (wheel frame).
	UPROPERTY(VisibleAnywhere, Category = "Race|Wheel")
	float LongForce = 0.0f;

	// Applied lateral force in newtons (wheel frame).
	UPROPERTY(VisibleAnywhere, Category = "Race|Wheel")
	float LatForce = 0.0f;

	// Commanded longitudinal force before friction limiting, newtons.
	// The rigidly-driven wheel approximation: no angular inertia state.
	UPROPERTY(VisibleAnywhere, Category = "Race|Wheel")
	float LongSlipInput = 0.0f;

	// Lateral slip velocity at the contact in m/s, wheel frame.
	UPROPERTY(VisibleAnywhere, Category = "Race|Wheel")
	float LatSlip = 0.0f;
};

UCLASS(ClassGroup = Movement, meta = (BlueprintSpawnableComponent))
class RACINGGAME_API URaceVehicleMovement : public UPawnMovementComponent
{
	GENERATED_BODY()

public:
	// Applies the authoritative configuration. The actor owns the value;
	// the movement keeps a working copy it never edits externally.
	void ApplyConfig(const FRaceVehicleConfig& Config);

	// The only input path. Normalized commands, no key knowledge.
	void SetDriveCommand(const FRaceDriveCommand& Command) { DriveCommand = Command; }
	const FRaceDriveCommand& GetDriveCommand() const { return DriveCommand; }

	const FRaceVehicleConfig& GetActiveConfig() const { return ActiveConfig; }
	float GetForwardSpeed() const { return ForwardSpeed; }
	float GetLateralSpeed() const { return LateralSpeed; }
	float GetVerticalSpeed() const { return VerticalSpeed; }
	float GetYawRate() const { return YawRateDegS; }
	float GetThrottleInput() const { return DriveCommand.Throttle; }
	bool IsGrounded() const { return bGrounded; }
	bool GetWheelContact(int32 Index) const;
	float GetWheelCompression(int32 Index) const;
	float GetWheelNormalLoad(int32 Index) const;
	float GetWheelLongForce(int32 Index) const;
	float GetWheelLatForce(int32 Index) const;
	FVector GetWheelContactPoint(int32 Index) const;
	FVector GetWheelContactNormal(int32 Index) const;
	FVector GetTotalTireForce() const { return TotalTireForce; }

	void ResetSpeed() { ForwardSpeed = 0.0f; LateralSpeed = 0.0f; VerticalSpeed = 0.0f; }

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	float EvalEngineForce(float SpeedCmS) const;
	void UpdateWheelContact();
	void UpdateSuspension(float DeltaTime);
	void UpdateTireForces();

	FRaceVehicleConfig ActiveConfig;
	FRaceDriveCommand DriveCommand;
	float ForwardSpeed = 0.0f;
	float LateralSpeed = 0.0f;
	float VerticalSpeed = 0.0f;
	float YawRateDegS = 0.0f;
	bool bGrounded = false;
	FRaceWheelState WheelState[4];
	float PrevCompression[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	FVector TotalTireForce = FVector::ZeroVector;
};
