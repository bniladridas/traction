// See header. Task 3 force model; interface unchanged from Task 2.

#include "RaceVehicleMovement.h"
#include "Engine/World.h"

bool URaceVehicleMovement::GetWheelContact(int32 Index) const
{
	return (Index >= 0 && Index < 4) ? WheelContact[Index] : false;
}

float URaceVehicleMovement::EvalEngineForce(float SpeedCmS) const
{
	if (EngineForcePoints.Num() == 0)
	{
		return 0.0f;
	}
	const float V = FMath::Max(0.0f, SpeedCmS);
	if (V <= EngineForcePoints[0].X)
	{
		return EngineForcePoints[0].Y;
	}
	for (int32 i = 1; i < EngineForcePoints.Num(); ++i)
	{
		if (V <= EngineForcePoints[i].X)
		{
			const FVector2D& A = EngineForcePoints[i - 1];
			const FVector2D& B = EngineForcePoints[i];
			const float T = (V - A.X) / FMath::Max(1.0f, B.X - A.X);
			return FMath::Lerp(A.Y, B.Y, T);
		}
	}
	return EngineForcePoints.Last().Y;
}

void URaceVehicleMovement::UpdateWheelContact()
{
	UWorld* World = GetWorld();
	if (!World || !PawnOwner)
	{
		return;
	}
	const FTransform ActorTM = UpdatedComponent->GetComponentTransform();
	const float Half[2] = { WheelBaseHalf, -WheelBaseHalf };
	int32 Idx = 0;
	bool bAny = false;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(PawnOwner);
	for (int32 ix = 0; ix < 2; ++ix)
	{
		for (int32 iy = 0; iy < 2; ++iy)
		{
			const FVector Local(Half[ix], iy == 0 ? TrackHalf : -TrackHalf, WheelRestZ);
			const FVector Center = ActorTM.TransformPosition(Local);
			const FVector Start = Center + FVector(0.0f, 0.0f, 30.0f);
			const FVector End = Center - FVector(0.0f, 0.0f, WheelTraceLength);
			FHitResult Hit;
			const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params);
			WheelContact[Idx] = bHit;
			bAny = bAny || bHit;
			++Idx;
		}
	}
	bGrounded = bAny;
}

void URaceVehicleMovement::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!PawnOwner || !UpdatedComponent || DeltaTime <= 0.0f || MassKg <= 0.0f)
	{
		return;
	}

	// Longitudinal force in newtons. Positive drives forward.
	float Force = 0.0f;
	float V = ForwardSpeed;
	if (BrakeInput > 0.0f)
	{
		if (V > ReverseEngageSpeed)
		{
			Force = -BrakeInput * BrakeForceN;
		}
		else
		{
			Force = -BrakeInput * ReverseForceN;
		}
	}
	else if (ThrottleInput > 0.0f)
	{
		if (V < 0.0f)
		{
			// Throttle while reversing brakes toward standstill.
			Force = ThrottleInput * BrakeForceN;
		}
		else
		{
			Force = ThrottleInput * EvalEngineForce(V);
		}
	}

	// Resistances in newtons oppose motion. Speed in m/s for the curve terms.
	const float Vms = V / 100.0f;
	const float Resist = RollingResistanceN + AeroDragCoeff * Vms * FMath::Abs(Vms);
	float Net = Force;
	if (V > 0.0f)
	{
		Net = Force - Resist;
	}
	else if (V < 0.0f)
	{
		Net = Force + Resist;
	}
	else if (FMath::Abs(Force) <= Resist)
	{
		Net = 0.0f;
	}
	else
	{
		Net = Force - FMath::Sign(Force) * Resist;
	}

	// Semi-implicit Euler: force -> accel -> velocity.
	const float VOld = V;
	V += (Net / MassKg) * 100.0f * DeltaTime;
	if (VOld > 0.0f && Force >= 0.0f && V < 0.0f)
	{
		// Drag alone never reverses direction within one step.
		V = 0.0f;
	}
	if (VOld < 0.0f && Force <= 0.0f && V > 0.0f)
	{
		V = 0.0f;
	}
	V = FMath::Clamp(V, -MaxReverseSpeed, MaxSpeed);

	// Vertical: gravity with ground contact. Blocking hits with an upward
	// normal while falling land the vehicle and clear vertical speed.
	float Vz = VerticalSpeed - GravityCmS2 * DeltaTime;
	if (bGrounded && Vz < 0.0f)
	{
		// Resting contact: do not push into the floor every tick. A
		// downward sweep starting in touching contact blocks at time zero
		// and would cancel the horizontal portion of the move.
		Vz = 0.0f;
	}
	FVector Delta = UpdatedComponent->GetForwardVector() * V * DeltaTime;
	Delta.Z += Vz * DeltaTime;

	// Speed-sensitive steering. Authority is full near standstill speed,
	// halves at SteerRefSpeed, and vanishes below SteerDeadSpeed.
	// Reversing mirrors the yaw direction.
	const float AbsV = FMath::Abs(V);
	float Authority = 0.0f;
	if (AbsV >= SteerDeadSpeed)
	{
		const float R = AbsV / SteerRefSpeed;
		Authority = 1.0f / (1.0f + R * R);
	}
	const float Direction = (V >= 0.0f) ? 1.0f : -1.0f;
	FRotator NewRotation = UpdatedComponent->GetComponentRotation();
	NewRotation.Yaw += SteeringInput * SteerYawLow * Authority * Direction * DeltaTime;

	FHitResult Hit(1.0f);
	SafeMoveUpdatedComponent(Delta, NewRotation.Quaternion(), true, Hit);
	if (Hit.bBlockingHit)
	{
		if (Vz <= 0.0f && Hit.Normal.Z > 0.7f)
		{
			Vz = 0.0f;
		}
		else if (Hit.Normal.Z <= 0.7f)
		{
			// Unexpected side contact on the flat track: stop, do not slide.
			V = 0.0f;
		}
	}

	ForwardSpeed = V;
	VerticalSpeed = Vz;
	UpdateWheelContact();
}
