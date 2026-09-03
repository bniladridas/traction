// See header. Task 6: longitudinal drive requests come from the
// drivetrain (driven axle) and the movement-side service brake; the tire
// path, suspension, steering, and integration are unchanged from Task 5.

#include "RaceVehicleMovement.h"
#include "RaceDrivetrain.h"
#include "Engine/World.h"

bool URaceVehicleMovement::GetWheelContact(int32 Index) const
{
	return (Index >= 0 && Index < 4) ? WheelState[Index].bContact : false;
}

float URaceVehicleMovement::GetWheelCompression(int32 Index) const
{
	return (Index >= 0 && Index < 4) ? WheelState[Index].Compression : 0.0f;
}

float URaceVehicleMovement::GetWheelNormalLoad(int32 Index) const
{
	return (Index >= 0 && Index < 4) ? WheelState[Index].NormalLoad : 0.0f;
}

float URaceVehicleMovement::GetWheelLongForce(int32 Index) const
{
	return (Index >= 0 && Index < 4) ? WheelState[Index].LongForce : 0.0f;
}

float URaceVehicleMovement::GetWheelLatForce(int32 Index) const
{
	return (Index >= 0 && Index < 4) ? WheelState[Index].LatForce : 0.0f;
}

FVector URaceVehicleMovement::GetWheelContactPoint(int32 Index) const
{
	return (Index >= 0 && Index < 4) ? WheelState[Index].ContactPoint : FVector::ZeroVector;
}

FVector URaceVehicleMovement::GetWheelContactNormal(int32 Index) const
{
	return (Index >= 0 && Index < 4) ? WheelState[Index].ContactNormal : FVector::UpVector;
}

void URaceVehicleMovement::ApplyConfig(const FRaceVehicleConfig& Config)
{
	ActiveConfig = Config;
	if (ActiveConfig.Wheels.Num() != 4)
	{
		UE_LOG(LogTemp, Warning, TEXT("RACEMOVE: config has %d wheels, model expects 4"),
			ActiveConfig.Wheels.Num());
	}
	for (const int32 Idx : ActiveConfig.DrivenWheelIndices)
	{
		if (Idx < 0 || Idx >= 4)
		{
			UE_LOG(LogTemp, Warning, TEXT("RACEMOVE: driven index %d out of range"), Idx);
		}
	}
}

bool URaceVehicleMovement::IsDrivenWheel(int32 Index) const
{
	return ActiveConfig.DrivenWheelIndices.Contains(Index);
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
		FRaceWheelState& WS = WheelState[i];
		WS.bContact = false;
		if (!Wheels.IsValidIndex(i))
		{
			continue;
		}
		const FVector Center = ActorTM.TransformPosition(Wheels[i].LocalOffset);
		const FVector Start = Center + FVector(0.0f, 0.0f, 30.0f);
		const FVector End = Center - FVector(0.0f, 0.0f, ActiveConfig.WheelTraceLength);
		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
		{
			WS.bContact = true;
			WS.ContactPoint = Hit.Location;
			WS.ContactNormal = Hit.Normal;
			bAny = true;
		}
	}
	bGrounded = bAny;
}

void URaceVehicleMovement::UpdateSuspension(float DeltaTime)
{
	if (!PawnOwner || !UpdatedComponent || DeltaTime <= 0.0f)
	{
		return;
	}
	const FTransform ActorTM = UpdatedComponent->GetComponentTransform();
	const TArray<FRaceWheelConfig>& Wheels = ActiveConfig.Wheels;
	for (int32 i = 0; i < 4; ++i)
	{
		FRaceWheelState& WS = WheelState[i];
		if (!WS.bContact || !Wheels.IsValidIndex(i))
		{
			WS.NormalLoad = 0.0f;
			WS.CompressionVel = 0.0f;
			continue;
		}
		const FVector Hardpoint = ActorTM.TransformPosition(Wheels[i].LocalOffset);
		const float Length = FVector::DotProduct(Hardpoint - WS.ContactPoint, WS.ContactNormal);
		const float Raw = ActiveConfig.SuspRestLengthCm - Length;
		const float Compr = FMath::Clamp(Raw, -ActiveConfig.SuspMaxExtensionCm, ActiveConfig.SuspMaxCompressionCm);
		const float ComprVel = (Compr - PrevCompression[i]) / DeltaTime;
		PrevCompression[i] = Compr;
		WS.Compression = Compr;
		WS.CompressionVel = ComprVel;
		const float SpringN = ActiveConfig.SpringStiffnessNpm * (Compr / 100.0f)
			+ ActiveConfig.DampingCoeffNspm * (ComprVel / 100.0f);
		WS.NormalLoad = FMath::Max(0.0f, SpringN);
	}
}

void URaceVehicleMovement::UpdateTireForces(float DrivenAxleReq, float BrakeAxleReq)
{
	const TArray<FRaceWheelConfig>& Wheels = ActiveConfig.Wheels;
	const float Vx = ForwardSpeed;
	const float Vy = LateralSpeed;

	// Longitudinal requests in newtons. Driven-axle force (engine drive,
	// reverse, engine braking) comes from the drivetrain and splits over
	// contacting driven wheels. Service braking stays movement-side and
	// splits over all contacting wheels, preserving Task 3 behavior.
	int32 nContact = 0;
	int32 nDrivenContact = 0;
	for (int32 i = 0; i < 4; ++i)
	{
		if (WheelState[i].bContact)
		{
			++nContact;
			if (IsDrivenWheel(i))
			{
				++nDrivenContact;
			}
		}
	}
	const float DrivenShareLocal = (nDrivenContact > 0) ? (DrivenAxleReq / (float)nDrivenContact) : 0.0f;
	const float BrakeShareLocal = (nContact > 0) ? (BrakeAxleReq / (float)nContact) : 0.0f;
	const float Omega = YawRateDegS * (3.14159265f / 180.0f); // rad/s, signed
	float SumX = 0.0f;
	float SumY = 0.0f;
	for (int32 i = 0; i < 4; ++i)
	{
		FRaceWheelState& WS = WheelState[i];
		WS.LongForce = 0.0f;
		WS.LatForce = 0.0f;
		if (!WS.bContact || !Wheels.IsValidIndex(i))
		{
			continue;
		}
		// Steer angle for front wheels, radians. Positive toward +Y.
		const float Delta = Wheels[i].bFrontAxle
			? DriveCommand.Steering * ActiveConfig.MaxSteerAngleDeg * (3.14159265f / 180.0f)
			: 0.0f;
		// Contact-point velocity in m/s, body frame, with yaw lever arm.
		const float LeverX = Wheels[i].LocalOffset.X / 100.0f;
		const float LeverY = Wheels[i].LocalOffset.Y / 100.0f;
		const float Vwx = Vx / 100.0f - Omega * LeverY;
		const float Vwy = Vy / 100.0f + Omega * LeverX;
		// Wheel frame.
		const float C = FMath::Cos(Delta);
		const float S = FMath::Sin(Delta);
		const float VLong = Vwx * C + Vwy * S;
		const float VLat = -Vwx * S + Vwy * C;
		(void)VLong;
		float Flong = BrakeShareLocal + (IsDrivenWheel(i) ? DrivenShareLocal : 0.0f);
		float Flat = -ActiveConfig.LateralStiffness * VLat;
		WS.LongSlipInput = Flong;
		WS.LatSlip = VLat;
		// Friction circle: scale the pair to fit mu*N.
		const float Flim = ActiveConfig.FrictionMu * WS.NormalLoad;
		const float Mag = FMath::Sqrt(Flong * Flong + Flat * Flat);
		if (Mag > Flim && Mag > 0.0f)
		{
			const float K = Flim / Mag;
			Flong *= K;
			Flat *= K;
		}
		WS.LongForce = Flong;
		WS.LatForce = Flat;
		// Back to body frame.
		SumX += Flong * C - Flat * S;
		SumY += Flong * S + Flat * C;
	}
	TotalTireForce = FVector(SumX, SumY, 0.0f);
}

void URaceVehicleMovement::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!PawnOwner || !UpdatedComponent || DeltaTime <= 0.0f || ActiveConfig.MassKg <= 0.0f)
	{
		return;
	}

	UpdateWheelContact();
	UpdateSuspension(DeltaTime);

	float Vx = ForwardSpeed;
	float Vy = LateralSpeed;

	// Longitudinal requests. Driven-axle force (engine drive, reverse,
	// engine braking) comes from the drivetrain. Service braking stays
	// movement-side with the Task 3 treatment: full brake force against
	// forward motion above the engage speed.
	float DrivenReq = 0.0f;
	if (Drivetrain.IsValid())
	{
		DrivenReq = Drivetrain->UpdateDrive(DriveCommand.Throttle, DriveCommand.Brake, Vx);
	}
	float BrakeReq = 0.0f;
	if (DriveCommand.Brake > 0.0f && Vx > ActiveConfig.ReverseEngageSpeed)
	{
		BrakeReq = -DriveCommand.Brake * ActiveConfig.BrakeForceN;
	}

	// Yaw integration keeps the verified Task 3 authority rule.
	const float AbsV = FMath::Abs(Vx);
	float Authority = 0.0f;
	if (AbsV >= ActiveConfig.SteerDeadSpeed)
	{
		const float R = AbsV / ActiveConfig.SteerRefSpeed;
		Authority = 1.0f / (1.0f + R * R);
	}
	const float Direction = (Vx >= 0.0f) ? 1.0f : -1.0f;
	const float YawDeltaDeg = DriveCommand.Steering * ActiveConfig.SteerYawLow * Authority * Direction * DeltaTime;
	YawRateDegS = YawDeltaDeg / DeltaTime;

	UpdateTireForces(DrivenReq, BrakeReq);

	// Longitudinal resistances oppose forward motion (Task 3 treatment).
	const float Vms = Vx / 100.0f;
	const float Resist = ActiveConfig.RollingResistanceN + ActiveConfig.AeroDragCoeff * Vms * FMath::Abs(Vms);
	float NetX = TotalTireForce.X;
	if (Vx > 0.0f)
	{
		NetX -= Resist;
	}
	else if (Vx < 0.0f)
	{
		NetX += Resist;
	}
	else if (FMath::Abs(NetX) <= Resist)
	{
		NetX = 0.0f;
	}
	else
	{
		NetX -= FMath::Sign(NetX) * Resist;
	}

	const float VxOld = Vx;
	Vx += (NetX / ActiveConfig.MassKg) * 100.0f * DeltaTime;
	if (VxOld > 0.0f && NetX >= 0.0f && Vx < 0.0f)
	{
		Vx = 0.0f;
	}
	if (VxOld < 0.0f && NetX <= 0.0f && Vx > 0.0f)
	{
		Vx = 0.0f;
	}
	Vx = FMath::Clamp(Vx, -ActiveConfig.MaxReverseSpeed, ActiveConfig.MaxSpeed);

	// Lateral velocity integrates lateral tire forces directly.
	Vy += (TotalTireForce.Y / ActiveConfig.MassKg) * 100.0f * DeltaTime;

	// Vertical: gravity with ground contact.
	float Vz = VerticalSpeed - ActiveConfig.GravityCmS2 * DeltaTime;
	if (bGrounded && Vz < 0.0f)
	{
		// Resting contact: do not push into the floor every tick.
		Vz = 0.0f;
	}

	// Transport: rotate body-frame velocity by the yaw step so the vector
	// stays inertially consistent while the frame turns.
	const float YawRad = YawDeltaDeg * (3.14159265f / 180.0f);
	const float CosY = FMath::Cos(YawRad);
	const float SinY = FMath::Sin(YawRad);
	const float Wx = Vx * CosY + Vy * SinY;
	const float Wy = -Vx * SinY + Vy * CosY;
	Vx = Wx;
	Vy = Wy;

	FRotator NewRotation = UpdatedComponent->GetComponentRotation();
	NewRotation.Yaw += YawDeltaDeg;

	const FVector Fwd = UpdatedComponent->GetForwardVector();
	const FVector Rgt = UpdatedComponent->GetRightVector();
	FVector Delta = Fwd * Vx * DeltaTime + Rgt * Vy * DeltaTime;
	Delta.Z += Vz * DeltaTime;

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
			Vx = 0.0f;
			Vy = 0.0f;
		}
	}

	ForwardSpeed = Vx;
	LateralSpeed = Vy;
	VerticalSpeed = Vz;
}
