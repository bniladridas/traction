// See header. Task 4 refactor: identical integration to Task 3, with all
// tunables read from ActiveConfig. No behavior tuning in this task.

#include "RaceVehicleMovement.h"
#include "Engine/World.h"

bool URaceVehicleMovement::GetWheelContact(int32 Index) const
{
	return (Index >= 0 && Index < 4) ? WheelContact[Index] : false;
}

void URaceVehicleMovement::ApplyConfig(const FRaceVehicleConfig& Config)
{
	ActiveConfig = Config;
	if (ActiveConfig.Wheels.Num() != 4)
	{
		UE_LOG(LogTemp, Warning, TEXT("RACEMOVE: config has %d wheels, model expects 4"),
			ActiveConfig.Wheels.Num());
	}
}

float URaceVehicleMovement::EvalEngineForce(float SpeedCmS) const
{
	const TArray<FVector2D>& Points = ActiveConfig.EngineForcePoints;
	if (Points.Num() == 0)
	{
		return 0.0f;
	}
	const float V = FMath::Max(0.0f, SpeedCmS);
	if (V <= Points[0].X)
	{
		return Points[0].Y;
	}
	for (int32 i = 1; i < Points.Num(); ++i)
	{
		if (V <= Points[i].X)
		{
			const FVector2D& A = Points[i - 1];
			const FVector2D& B = Points[i];
			const float T = (V - A.X) / FMath::Max(1.0f, B.X - A.X);
			return FMath::Lerp(A.Y, B.Y, T);
		}
	}
	return Points.Last().Y;
}

void URaceVehicleMovement::UpdateWheelContact()
{
	UWorld* World = GetWorld();
	if (!World || !PawnOwner || !UpdatedComponent)
	{
		return;
	}
	const FTransform ActorTM = UpdatedComponent->GetComponentTransform();
	const TArray<FRaceWheelConfig>& Wheels = ActiveConfig.Wheels;
	bool bAny = false;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(PawnOwner);
	for (int32 i = 0; i < 4; ++i)
	{
		WheelContact[i] = false;
		if (!Wheels.IsValidIndex(i))
		{
			continue;
		}
		const FVector Center = ActorTM.TransformPosition(Wheels[i].LocalOffset);
		const FVector Start = Center + FVector(0.0f, 0.0f, 30.0f);
		const FVector End = Center - FVector(0.0f, 0.0f, ActiveConfig.WheelTraceLength);
		FHitResult Hit;
		const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params);
		WheelContact[i] = bHit;
		bAny = bAny || bHit;
	}
	bGrounded = bAny;
}

void URaceVehicleMovement::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!PawnOwner || !UpdatedComponent || DeltaTime <= 0.0f || ActiveConfig.MassKg <= 0.0f)
	{
		return;
	}

	// Longitudinal force in newtons. Positive drives forward.
	float Force = 0.0f;
	float V = ForwardSpeed;
	const float Throttle = DriveCommand.Throttle;
	const float Brake = DriveCommand.Brake;
	if (Brake > 0.0f)
	{
		if (V > ActiveConfig.ReverseEngageSpeed)
		{
			Force = -Brake * ActiveConfig.BrakeForceN;
		}
		else
		{
			Force = -Brake * ActiveConfig.ReverseForceN;
		}
	}
	else if (Throttle > 0.0f)
	{
		if (V < 0.0f)
		{
			// Throttle while reversing brakes toward standstill.
			Force = Throttle * ActiveConfig.BrakeForceN;
		}
		else
		{
			Force = Throttle * EvalEngineForce(V);
		}
	}

	// Resistances in newtons oppose motion. Speed in m/s for the curve terms.
	const float Vms = V / 100.0f;
	const float Resist = ActiveConfig.RollingResistanceN + ActiveConfig.AeroDragCoeff * Vms * FMath::Abs(Vms);
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
	V += (Net / ActiveConfig.MassKg) * 100.0f * DeltaTime;
	if (VOld > 0.0f && Force >= 0.0f && V < 0.0f)
	{
		// Drag alone never reverses direction within one step.
		V = 0.0f;
	}
	if (VOld < 0.0f && Force <= 0.0f && V > 0.0f)
	{
		V = 0.0f;
	}
	V = FMath::Clamp(V, -ActiveConfig.MaxReverseSpeed, ActiveConfig.MaxSpeed);

	// Vertical: gravity with ground contact. Blocking hits with an upward
	// normal while falling land the vehicle and clear vertical speed.
	float Vz = VerticalSpeed - ActiveConfig.GravityCmS2 * DeltaTime;
	if (bGrounded && Vz < 0.0f)
	{
		// Resting contact: do not push into the floor every tick. A
		// downward sweep starting in touching contact blocks at time zero
		// and would cancel the horizontal portion of the move.
		Vz = 0.0f;
	}
	FVector Delta = UpdatedComponent->GetForwardVector() * V * DeltaTime;
	Delta.Z += Vz * DeltaTime;

	// Speed-sensitive steering. Authority is full near standstill speed
	// and halves at the configured reference speed.
	const float AbsV = FMath::Abs(V);
	float Authority = 0.0f;
	if (AbsV >= ActiveConfig.SteerDeadSpeed)
	{
		const float R = AbsV / ActiveConfig.SteerRefSpeed;
		Authority = 1.0f / (1.0f + R * R);
	}
	const float Direction = (V >= 0.0f) ? 1.0f : -1.0f;
	FRotator NewRotation = UpdatedComponent->GetComponentRotation();
	NewRotation.Yaw += DriveCommand.Steering * ActiveConfig.SteerYawLow * Authority * Direction * DeltaTime;

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
