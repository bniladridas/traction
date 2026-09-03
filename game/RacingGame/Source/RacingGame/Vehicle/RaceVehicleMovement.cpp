// See header. Task 5 model: per-wheel contact, spring-damper suspension,
// friction-circle tire forces. Body longitudinal behavior matches Task 3
// in straight phases; lateral velocity state is new.

#include "RaceVehicleMovement.h"
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

void URaceVehicleMovement::UpdateTireForces()
{
	const TArray<FRaceWheelConfig>& Wheels = ActiveConfig.Wheels;
	const float Throttle = DriveCommand.Throttle;
	const float Brake = DriveCommand.Brake;
	const float Vx = ForwardSpeed;
	const float Vy = LateralSpeed;

	// Longitudinal request in newtons, Task 3 logic unchanged.
	float Req = 0.0f;
	if (Brake > 0.0f)
	{
		Req = (Vx > ActiveConfig.ReverseEngageSpeed)
			? -Brake * ActiveConfig.BrakeForceN
			: -Brake * ActiveConfig.ReverseForceN;
	}
	else if (Throttle > 0.0f)
	{
		Req = (Vx < 0.0f)
			? Throttle * ActiveConfig.BrakeForceN
			: Throttle * EvalEngineForce(Vx);
	}

	int32 nContact = 0;
	for (int32 i = 0; i < 4; ++i)
	{
		if (WheelState[i].bContact)
		{
			++nContact;
		}
	}
	const float PerWheelReq = (nContact > 0) ? (Req / (float)nContact) : 0.0f;

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
		float Flong = PerWheelReq;
		float Flat = -ActiveConfig.LateralStiffness * VLat;
		WS.LongSlipInput = PerWheelReq;
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

	UpdateTireForces();

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
