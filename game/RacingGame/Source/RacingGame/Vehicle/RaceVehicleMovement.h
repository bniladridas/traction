// RacingGame vehicle dynamics (Tasks 3 and 4).
// Task 4 refactor: this component owns NO tunable parameters. Everything
// tunable lives in FRaceVehicleConfig, applied through ApplyConfig. Input
// arrives only as FRaceDriveCommand; this component never sees keys.
// The integration model itself is unchanged from the verified Task 3
// behavior: scalar force model, semi-implicit Euler, sweep movement.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
#include "RaceVehicleConfig.h"
#include "RaceVehicleMovement.generated.h"

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
	float GetVerticalSpeed() const { return VerticalSpeed; }
	float GetThrottleInput() const { return DriveCommand.Throttle; }
	bool IsGrounded() const { return bGrounded; }
	bool GetWheelContact(int32 Index) const;

	void ResetSpeed() { ForwardSpeed = 0.0f; VerticalSpeed = 0.0f; }

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	float EvalEngineForce(float SpeedCmS) const;
	void UpdateWheelContact();

	FRaceVehicleConfig ActiveConfig;
	FRaceDriveCommand DriveCommand;
	float ForwardSpeed = 0.0f;
	float VerticalSpeed = 0.0f;
	bool bGrounded = false;
	bool WheelContact[4] = { false, false, false, false };
};
