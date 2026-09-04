// Chase camera driver (Task 10).
// Drives the pawn's spring arm with a relative yaw offset (look-ahead
// into turns), speed-sensitive arm length and pitch, all smoothed. Reads
// pawn transform and motion only; never touches movement, drivetrain, or
// race state. Snaps on resets and teleports instead of smoothing across
// discontinuities.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RaceCameraConfig.h"
#include "RaceChaseCamera.generated.h"

class USpringArmComponent;
class UCameraComponent;
class ARaceVehicle;

UCLASS(ClassGroup = Camera, meta = (BlueprintSpawnableComponent))
class RACINGGAME_API URaceChaseCamera : public UActorComponent
{
	GENERATED_BODY()

public:
	URaceChaseCamera();

	UPROPERTY(EditAnywhere, Category = "Race|Camera")
	FRaceCameraConfig CameraConfig;

	// Wires the pawn's arm and camera. Called by the pawn after creation.
	void Init(USpringArmComponent* InArm, UCameraComponent* InCam);

	// Snaps smoothed state to current targets (reset/teleport path).
	void SnapToTarget();

	// Read-only verification views.
	float GetRelativeYaw() const { return SmoothedRelYaw; }
	float GetArmLength() const;

protected:
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY()
	USpringArmComponent* Arm = nullptr;
	UPROPERTY()
	UCameraComponent* Cam = nullptr;

	float SmoothedRelYaw = 0.0f;
	float SmoothedPitch = -14.0f;
	float SmoothedArm = 500.0f;
	FVector LastPawnLoc = FVector::ZeroVector;
	bool bHasLast = false;
};
