// RacingGame-owned prototype vehicle (Task 2).
// A cube with a box collider, a chase camera, and a minimal input path:
// public Apply* methods are bound to keys AND called by the E2E harness,
// so the test exercises the same functions as a player.
// Presentation detail (mesh, camera framing) stays here; drive behavior
// lives in URaceVehicleMovement; Task 3 extends the movement model.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "RaceVehicleConfig.h"
#include "RaceVehicle.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class URaceVehicleMovement;
class URaceDrivetrain;

UCLASS()
class RACINGGAME_API ARaceVehicle : public APawn
{
	GENERATED_BODY()

public:
	ARaceVehicle();

	// Input path shared by keyboard bindings and the automated harness.
	void ApplyThrottle(float Value);
	void ApplyBrake(float Value);
	void ApplySteering(float Value);

	// Restores the BeginPlay transform with zero velocity. Bound to R.
	void ResetVehicle();

	float GetForwardSpeed() const;
	float GetLateralSpeed() const;
	float GetYawRate() const;
	float GetVerticalSpeed() const;
	bool IsGrounded() const;
	bool GetWheelContact(int32 Index) const;
	float GetWheelCompression(int32 Index) const;
	float GetWheelNormalLoad(int32 Index) const;
	float GetWheelLongForce(int32 Index) const;
	float GetWheelLatForce(int32 Index) const;
	FVector GetWheelContactPoint(int32 Index) const;
	FVector GetWheelContactNormal(int32 Index) const;
	FVector GetTotalTireForce() const;

	// Authoritative configuration for this vehicle instance. A future
	// variant assigns different values here without new classes.
	const FRaceVehicleConfig& GetVehicleConfig() const { return VehicleConfig; }

	// Read-only view of what the movement actually consumed.
	const FRaceVehicleConfig& GetActiveConfig() const;

	// Read-only drivetrain state for verification.
	float GetEngineRPM() const;
	float GetEngineTorque() const;
	int32 GetGearIndex() const;
	int32 GetUpshiftCount() const;
	int32 GetDownshiftCount() const;
	float GetLastShaftTorque() const;

	UCameraComponent* GetChaseCamera() const { return ChaseCamera; }

protected:
	// Single authoritative tunable set, edited per instance or subclass.
	// The movement component receives it through ApplyConfig and owns none.
	UPROPERTY(EditAnywhere, Category = "Race|Config")
	FRaceVehicleConfig VehicleConfig;

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	// Legacy axis callbacks (see DefaultInput.ini RaceThrottle/RaceSteer).
	void OnThrottleAxis(float Value);
	void OnSteerAxis(float Value);

	// Places the box bottom exactly on the ground below spawn and is the
	// basis for the reset transform. Deterministic on the flat test track.
	void SettleToGround();

	UPROPERTY(VisibleAnywhere, Category = "Race")
	UBoxComponent* CollisionBox;

	UPROPERTY(VisibleAnywhere, Category = "Race")
	UStaticMeshComponent* CubeMesh;

	UPROPERTY(VisibleAnywhere, Category = "Race")
	USpringArmComponent* CameraArm;

	UPROPERTY(VisibleAnywhere, Category = "Race")
	UCameraComponent* ChaseCamera;

	UPROPERTY(VisibleAnywhere, Category = "Race")
	URaceVehicleMovement* VehicleMovement;

	UPROPERTY(VisibleAnywhere, Category = "Race")
	URaceDrivetrain* Drivetrain;

	// Assembled normalized command. Keyboard callbacks and the harness
	// both write here; the movement consumes the whole struct.
	FRaceDriveCommand PendingCommand;

	FTransform InitialTransform;
};
