// See header. Prototype kinematic model; Task 3 owns realism.

#include "RaceVehicleMovement.h"

void URaceVehicleMovement::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!PawnOwner || !UpdatedComponent || DeltaTime <= 0.0f)
	{
		return;
	}

	float V = ForwardSpeed;

	// Brake, or engage reverse near standstill.
	if (BrakeInput > 0.0f)
	{
		if (V > ReverseEngageSpeed)
		{
			V = FMath::Max(0.0f, V - BrakeRate * BrakeInput * DeltaTime);
		}
		else
		{
			V = FMath::Max(-MaxReverseSpeed, V - ReverseAccel * BrakeInput * DeltaTime);
		}
	}
	else if (ThrottleInput > 0.0f)
	{
		if (V < 0.0f)
		{
			// Throttle while reversing acts as a brake toward standstill.
			V = FMath::Min(0.0f, V + BrakeRate * ThrottleInput * DeltaTime);
		}
		else
		{
			V = FMath::Min(MaxSpeed, V + AccelRate * ThrottleInput * DeltaTime);
		}
	}

	// Rolling drag toward standstill.
	const float DragStep = RollingDrag * DeltaTime;
	if (V > 0.0f)
	{
		V = FMath::Max(0.0f, V - DragStep);
	}
	else if (V < 0.0f)
	{
		V = FMath::Min(0.0f, V + DragStep);
	}

	// Steering needs motion; reversing mirrors the yaw direction.
	const float Grip = FMath::Clamp(FMath::Abs(V) / FullGripSpeed, 0.0f, 1.0f);
	const float Direction = (V >= 0.0f) ? 1.0f : -1.0f;
	const float YawDelta = SteeringInput * TurnRate * Grip * Direction * DeltaTime;

	FRotator NewRotation = UpdatedComponent->GetComponentRotation();
	NewRotation.Yaw += YawDelta;

	const FVector Forward = UpdatedComponent->GetForwardVector();
	const FVector Delta = Forward * V * DeltaTime;

	FHitResult Hit(1.0f);
	SafeMoveUpdatedComponent(Delta, NewRotation.Quaternion(), true, Hit);
	if (Hit.bBlockingHit)
	{
		// Flat test surface: any block is unexpected; stop rather than slide.
		V = 0.0f;
	}

	ForwardSpeed = V;
}
