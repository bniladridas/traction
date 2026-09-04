// See header.

#include "RaceAIDriver.h"
#include "RaceTrack.h"
#include "RaceManager.h"
#include "RaceVehicle.h"
#include "Engine/World.h"
#include "EngineUtils.h"

URaceAIDriver::URaceAIDriver()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void URaceAIDriver::BeginPlay()
{
	Super::BeginPlay();
	Vehicle = Cast<ARaceVehicle>(GetOwner());
	for (TActorIterator<ARaceTrack> It(GetWorld()); It; ++It)
	{
		Track = *It;
	}
	for (TActorIterator<ARaceManager> It(GetWorld()); It; ++It)
	{
		Manager = *It;
	}
}

int32 URaceAIDriver::NearestIndex(const FVector& Pos) const
{
	const TArray<FRaceTrackCenterPoint>& Pts = Track->GetCenterPoints();
	int32 Best = 0;
	float BestD = FLT_MAX;
	for (int32 i = 0; i < Pts.Num(); ++i)
	{
		const float D = FVector::DistSquared2D(Pos, Pts[i].Position);
		if (D < BestD)
		{
			BestD = D;
			Best = i;
		}
	}
	return Best;
}

void URaceAIDriver::Respawn(int32 Idx)
{
	const TArray<FRaceTrackCenterPoint>& Pts = Track->GetCenterPoints();
	const FRaceTrackCenterPoint& P = Pts[Idx];
	const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(P.Forward.Y, P.Forward.X));
	// Rest height exactly (box half-height over the road top): the model
	// settles via free-fall plus box block at spawn, but mid-run traces
	// touch immediately and the grounded rule would freeze a drop.
	Vehicle->SetActorLocationAndRotation(FVector(P.Position.X, P.Position.Y, 40.0f),
		FRotator(0.0f, Yaw, 0.0f), false, nullptr, ETeleportType::TeleportPhysics);
	Vehicle->ResetMotion();
	if (Manager)
	{
		Manager->ReanchorParticipant(Vehicle);
	}
	LastIdx = Idx;
	UnwrappedS = P.Distance;
	OfftrackTime = 0.0f;
	CheckTime = Clock;
	CheckS = UnwrappedS;
	RecoveryCount++;
	UE_LOG(LogTemp, Display, TEXT("RACEAI: respawn s=%.0f count=%d"), UnwrappedS, RecoveryCount);
}

void URaceAIDriver::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!Vehicle || !Track || DeltaTime <= 0.0f)
	{
		return;
	}
	Clock += DeltaTime;
	const bool bRacing = Manager && static_cast<int32>(Manager->GetPhase()) == 2;
	if (!bRacing)
	{
		Vehicle->ApplyThrottle(0.0f);
		Vehicle->ApplyBrake(0.0f);
		Vehicle->ApplySteering(0.0f);
		return;
	}
	bDrove = true;

	static double LastBeat = 0.0;
	if (Clock - LastBeat > 2.0)
	{
		LastBeat = Clock;
		int32 ContactCount = 0;
		for (int32 w = 0; w < 4; ++w)
		{
			ContactCount += Vehicle->GetWheelContact(w) ? 1 : 0;
		}
		UE_LOG(LogTemp, Display, TEXT("RACEAI: t=%.1f s=%.0f v=%.0f gear=%d thr=%.1f contact=%d off=%.2f rec=%d pos=%s"),
			Clock, UnwrappedS, Vehicle->GetForwardSpeed(), Vehicle->GetGearIndex(),
			Vehicle->GetThrottleInput(), ContactCount, OfftrackTime, RecoveryCount,
			*Vehicle->GetActorLocation().ToString());
	}

	const TArray<FRaceTrackCenterPoint>& Pts = Track->GetCenterPoints();
	const float L = Track->GetTrackLength();
	const int32 N = Pts.Num();
	if (N < 2 || L <= 0.0f)
	{
		return;
	}
	const int32 Idx = NearestIndex(Vehicle->GetActorLocation());
	if (!bAnchored)
	{
		bAnchored = true;
		LastIdx = Idx;
		UnwrappedS = Pts[Idx].Distance;
		UnwrappedStart = UnwrappedS;
		CheckTime = Clock;
		CheckS = UnwrappedS;
	}
	int32 IdxDelta = Idx - LastIdx;
	if (IdxDelta < -N / 2)
	{
		IdxDelta += N;
	}
	else if (IdxDelta > N / 2)
	{
		IdxDelta -= N;
	}
	LastIdx = Idx;
	float SNow = Pts[Idx].Distance;
	while (SNow < UnwrappedS - L * 0.5f)
	{
		SNow += L;
	}
	while (SNow > UnwrappedS + L * 0.5f)
	{
		SNow -= L;
	}
	UnwrappedS = SNow;

	// Off-track detection against the lane edge.
	const float Lat = FVector::Dist2D(Vehicle->GetActorLocation(), Pts[Idx].Position);
	const float Half = Track->TrackConfig.TrackWidth * 0.5f;
	if (Lat > Half + OfftrackMargin)
	{
		OfftrackTime += DeltaTime;
	}
	else
	{
		OfftrackTime = 0.0f;
	}

	// Stall detection: no forward progress while racing.
	if (Clock - CheckTime >= StallWindow)
	{
		if (UnwrappedS - CheckS < 50.0f)
		{
			Respawn(Idx);
			return;
		}
		CheckTime = Clock;
		CheckS = UnwrappedS;
	}
	if (OfftrackTime >= 1.0f)
	{
		Respawn(Idx);
		return;
	}

	// Pure pursuit with curvature-aware speed (Task 7 program, game-side).
	const float Speed = Vehicle->GetForwardSpeed();
	const FRaceTrackCenterPoint Tgt = Track->SampleAtDistance(UnwrappedS + LookaheadBase + FMath::Abs(Speed) * LookaheadSpeedGain);
	const FVector ToTgt = Tgt.Position - Vehicle->GetActorLocation();
	const float DesiredYaw = FMath::RadiansToDegrees(FMath::Atan2(ToTgt.Y, ToTgt.X));
	const float YawErr = FRotator::NormalizeAxis(DesiredYaw - Vehicle->GetActorRotation().Yaw);
	Vehicle->ApplySteering(FMath::Clamp(YawErr / SteerGainDeg, -1.0f, 1.0f));

	const FVector& DNear = Pts[(Idx + 1) % N].Forward;
	const FVector& DFar = Pts[(Idx + 5) % N].Forward;
	const float TurnDeg = FMath::RadiansToDegrees(
		FMath::Acos(FMath::Clamp(FVector::DotProduct(DNear, DFar), -1.0f, 1.0f)));
	float Target = StraightTarget;
	if (TurnDeg > TurnHardDeg)
	{
		Target = HairpinTarget;
	}
	else if (TurnDeg > TurnSoftDeg)
	{
		Target = TurnTarget;
	}
	if (Speed < Target - 50.0f)
	{
		Vehicle->ApplyThrottle(1.0f);
		Vehicle->ApplyBrake(0.0f);
	}
	else if (Speed > Target + 50.0f && Speed > 200.0f)
	{
		Vehicle->ApplyThrottle(0.0f);
		Vehicle->ApplyBrake(1.0f);
	}
	else
	{
		Vehicle->ApplyThrottle(0.4f);
		Vehicle->ApplyBrake(0.0f);
	}
}
