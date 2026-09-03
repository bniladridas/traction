// See header.

#include "Task7Probe.h"
#include "RaceVehicle.h"
#include "RaceTrack.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

ATask7Probe::ATask7Probe()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATask7Probe::BeginPlay()
{
	Super::BeginPlay();
	for (TActorIterator<ARaceTrack> It(GetWorld()); It; ++It)
	{
		Track = *It;
	}
	if (!Track)
	{
		Finish(false, TEXT("no race track actor in level"));
	}
}

float ATask7Probe::NormYaw(float Deg)
{
	return FRotator::NormalizeAxis(Deg);
}

int32 ATask7Probe::NearestCenterIndex(const FVector& Pos) const
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

void ATask7Probe::Tick(float Delta)
{
	Super::Tick(Delta);
	if (bFinished || !Track)
	{
		return;
	}
	Elapsed += Delta;
	Frames++;

	if (!Vehicle)
	{
		if (APawn* P = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
		{
			Vehicle = Cast<ARaceVehicle>(P);
			if (Vehicle)
			{
				if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
				{
					Vehicle->DisableInput(PC);
				}
				UE_LOG(LogTemp, Display, TEXT("TRACK7E2E: pawn acquired"));
			}
			else
			{
				Finish(false, TEXT("player pawn is not an ARaceVehicle"));
				return;
			}
		}
		if (!Vehicle)
		{
			if (Elapsed > 10.0)
			{
				Finish(false, TEXT("no player pawn within timeout"));
			}
			return;
		}
	}

	// Settle-window contact sampling.
	if (Elapsed >= 0.2 && Elapsed <= 1.2)
	{
		ContactN++;
		bool bAll = true;
		for (int32 w = 0; w < 4; ++w)
		{
			bAll = bAll && Vehicle->GetWheelContact(w);
		}
		if (bAll)
		{
			ContactOk++;
		}
	}

	// Start alignment: vehicle has received no input yet.
	if (!bStartMeasured && Elapsed >= 1.2)
	{
		bStartMeasured = true;
		StartPosErr = FVector::Dist(Vehicle->GetActorLocation(), Track->GetStartPosition());
		StartYawErr = FMath::Abs(NormYaw(Vehicle->GetActorRotation().Yaw - Track->GetStartYawDeg()));
		UE_LOG(LogTemp, Display, TEXT("TRACK7E2E: start poserr=%.1f yawerr=%.2f"), StartPosErr, StartYawErr);
	}

	if (!bDriving && Elapsed >= Task7Limits::DriveStart)
	{
		bDriving = true;
		LastIdx = NearestCenterIndex(Vehicle->GetActorLocation());
		const TArray<FRaceTrackCenterPoint>& Pts = Track->GetCenterPoints();
		UnwrappedS = Pts[LastIdx].Distance;
		UnwrappedStart = UnwrappedS;
		LapStartLoc = Vehicle->GetActorLocation();
		const TArray<FRaceTrackCheckpoint>& CPs = Track->GetCheckpoints();
		if (CPs.Num() > 0)
		{
			PrevPlaneD = FVector::DotProduct(Vehicle->GetActorLocation() - CPs[0].Position, CPs[0].Forward);
		}
		UE_LOG(LogTemp, Display, TEXT("TRACK7E2E: driving from s=%.1f"), UnwrappedS);
	}

	if (bDriving && !bLapDone)
	{
		const TArray<FRaceTrackCenterPoint>& Pts = Track->GetCenterPoints();
		const float L = Track->GetTrackLength();
		const int32 N = Pts.Num();

		// Unwrap progress along the centerline.
		const int32 Idx = NearestCenterIndex(Vehicle->GetActorLocation());
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
		// Handle wrap relative to the running unwrapped value.
		while (SNow < UnwrappedS - L * 0.5f)
		{
			SNow += L;
		}
		while (SNow > UnwrappedS + L * 0.5f)
		{
			SNow -= L;
		}
		UnwrappedS = SNow;

		// Pure pursuit: steer toward a lookahead point on the line.
		const float Speed = Vehicle->GetForwardSpeed();
		const float Lookahead = 400.0f + FMath::Abs(Speed) * 0.3f;
		const FRaceTrackCenterPoint Tgt = Track->SampleAtDistance(UnwrappedS + Lookahead);
		const FVector ToTgt = Tgt.Position - Vehicle->GetActorLocation();
		const float DesiredYaw = FMath::RadiansToDegrees(FMath::Atan2(ToTgt.Y, ToTgt.X));
		const float YawErr = NormYaw(DesiredYaw - Vehicle->GetActorRotation().Yaw);
		Vehicle->ApplySteering(FMath::Clamp(YawErr / 18.0f, -1.0f, 1.0f));

		// Curvature-aware speed sized to the yaw rule: at 600 cm/s the
		// rule holds ~625 cm radius (sweeper needs 800); at 400 it holds
		// ~300 (hairpin needs 450). Brake early, never near standstill
		// (brake there would engage reverse).
		const FVector& DNear = Pts[(Idx + 1) % N].Forward;
		const FVector& DFar = Pts[(Idx + 5) % N].Forward;
		const float TurnDeg = FMath::RadiansToDegrees(
			FMath::Acos(FMath::Clamp(FVector::DotProduct(DNear, DFar), -1.0f, 1.0f)));
		float Target = 1200.0f;
		if (TurnDeg > 30.0f)
		{
			Target = 400.0f;
		}
		else if (TurnDeg > 12.0f)
		{
			Target = 600.0f;
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

		// Heartbeat for trajectory diagnosis.
		static double LastBeat = 0.0;
		if (Elapsed - LastBeat > 2.0)
		{
			LastBeat = Elapsed;
			int32 ContactCount = 0;
			for (int32 w = 0; w < 4; ++w)
			{
				ContactCount += Vehicle->GetWheelContact(w) ? 1 : 0;
			}
			// Forward blockage probe: what does the sweep see ahead?
			FString BlockInfo = TEXT("clear");
			const float SpeedAbs = FMath::Abs(Speed);
			if (SpeedAbs < 100.0f)
			{
				const FVector Nose = Vehicle->GetActorLocation() + Vehicle->GetActorForwardVector() * 110.0f;
				FCollisionQueryParams QP;
				QP.AddIgnoredActor(Vehicle);
				FHitResult BlockHit;
				if (GetWorld()->SweepSingleByChannel(BlockHit, Nose, Nose + Vehicle->GetActorForwardVector() * 300.0f,
					FQuat::Identity, ECC_WorldStatic, FCollisionShape::MakeBox(FVector(100.0f, 50.0f, 35.0f)), QP))
				{
					BlockInfo = FString::Printf(TEXT("hit=%s comp=%s n=%s at=%s"),
						*GetNameSafe(BlockHit.GetActor()), *GetNameSafe(BlockHit.GetComponent()),
						*BlockHit.Normal.ToString(), *BlockHit.ImpactPoint.ToString());
				}
			}
			UE_LOG(LogTemp, Display, TEXT("TRACK7E2E: t=%.1f s=%.0f v=%.0f yaw=%.0f gear=%d thr=%.1f brk=%.1f totF=%s contact=%d block[%s] pos=%s"),
				Elapsed, UnwrappedS, Speed, Vehicle->GetActorRotation().Yaw, Vehicle->GetGearIndex(),
				Vehicle->GetThrottleInput(), Vehicle->GetBrakeInput(), *Vehicle->GetTotalTireForce().ToString(),
				ContactCount, *BlockInfo, *Vehicle->GetActorLocation().ToString());
		}

		// Checkpoint crossing on the expected plane.
		const TArray<FRaceTrackCheckpoint>& CPs = Track->GetCheckpoints();
		if (ExpectIdx < CPs.Num())
		{
			const FRaceTrackCheckpoint& CP = CPs[ExpectIdx];
			const FVector Rel = Vehicle->GetActorLocation() - CP.Position;
			const float D = FVector::DotProduct(Rel, CP.Forward);
			const float Lat = FMath::Abs(FVector::DotProduct(Rel, FVector(-CP.Forward.Y, CP.Forward.X, 0.0f)));
			if (PrevPlaneD < 0.0f && D >= 0.0f && Lat < CP.Width * 0.5f + 400.0f)
			{
				CrossedSeq.Add(CP.Index);
				UE_LOG(LogTemp, Display, TEXT("TRACK7E2E: crossed checkpoint %d t=%.2f"), CP.Index, Elapsed);
				ExpectIdx++;
				if (ExpectIdx >= CPs.Num())
				{
					// All checkpoints in order; next the finish line (CP0).
					bLapDone = true;
					LapDist = UnwrappedS - UnwrappedStart;
					Finish(true, TEXT("lap complete"));
					return;
				}
				PrevPlaneD = FVector::DotProduct(Vehicle->GetActorLocation() - CPs[ExpectIdx].Position, CPs[ExpectIdx].Forward);
			}
			else
			{
				PrevPlaneD = D;
			}
		}

		if (Elapsed > Task7Limits::LapTimeout)
		{
			Finish(false, TEXT("lap timeout"));
			return;
		}
	}
}

void ATask7Probe::Finish(bool bOk, const FString& Note)
{
	if (bFinished)
	{
		return;
	}
	bFinished = true;
	if (Vehicle)
	{
		Vehicle->ApplyThrottle(0.0f);
		Vehicle->ApplyBrake(0.0f);
		Vehicle->ApplySteering(0.0f);
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			Vehicle->EnableInput(PC);
		}
	}
	WriteResults(bOk, Note);
	UE_LOG(LogTemp, Display, TEXT("TRACK7E2E: finishing (%s)"), *Note);
	UKismetSystemLibrary::QuitGame(GetWorld(), nullptr, EQuitPreference::Quit, false);
}

void ATask7Probe::WriteResults(bool bOk, const FString& Note) const
{
	const double ContactFrac = (ContactN > 0) ? (double)ContactOk / (double)ContactN : 0.0;
	const int32 Segs = Track ? Track->GetRoadSegmentCount() : 0;
	const int32 CenterN = Track ? Track->GetCenterPoints().Num() : 0;
	const int32 CheckN = Track ? Track->GetCheckpoints().Num() : 0;
	const float Length = Track ? Track->GetTrackLength() : 0.0f;
	float Closure = -1.0f;
	if (Track && CenterN > 1)
	{
		const TArray<FRaceTrackCenterPoint>& Pts = Track->GetCenterPoints();
		Closure = FVector::Dist(Pts[0].Position, Pts[CenterN - 1].Position);
	}
	bool bContig = true;
	if (Track)
	{
		const TArray<FRaceTrackCheckpoint>& CPs = Track->GetCheckpoints();
		for (int32 i = 0; i < CPs.Num(); ++i)
		{
			bContig = bContig && (CPs[i].Index == i);
		}
	}
	bool bOrder = bContig && CrossedSeq.Num() == CheckN;
	for (int32 i = 0; bOrder && i < CrossedSeq.Num(); ++i)
	{
		bOrder = (CrossedSeq[i] == i);
	}
	const double LapTol = (Length > 1.0) ? FMath::Abs(LapDist - Length) / Length : -1.0;

	const bool bLoad = Segs >= Task7Limits::RoadSegMin;
	const bool bContact = ContactFrac >= Task7Limits::ContactFrac;
	const bool bAlign = StartPosErr >= 0.0f && StartPosErr < Task7Limits::StartPosMaxErrCm
		&& StartYawErr < Task7Limits::StartYawMaxErrDeg;
	const bool bCenter = CenterN >= Task7Limits::CenterPtsMin && Closure >= 0.0f && Closure < Task7Limits::ClosureMaxCm
		&& Length > Task7Limits::LengthMinCm;
	const bool bOrderOk = bOrder;
	const bool bLap = bLapDone && LapTol >= 0.0 && LapTol < Task7Limits::LapDistTol;
	const bool bAll = bOk && bLoad && bContact && bAlign && bCenter && bOrderOk && bLap;

	FString Seq = TEXT("[");
	for (int32 i = 0; i < CrossedSeq.Num(); ++i)
	{
		Seq += FString::Printf(TEXT("%s%d"), (i > 0) ? TEXT(",") : TEXT(""), CrossedSeq[i]);
	}
	Seq += TEXT("]");

	const float Width = Track ? Track->TrackConfig.TrackWidth : 0.0f;
	const FString Json = FString::Printf(
		TEXT("{\"track_load\":%s,\"road_contact\":%s,\"start_alignment\":%s,\"centerline_valid\":%s,\"checkpoint_order\":%s,\"lap_traversal\":%s,")
		TEXT("\"road_segments\":%d,\"center_points\":%d,\"checkpoint_count\":%d,\"track_length_measured\":%.1f,\"track_width_measured\":%.1f,")
		TEXT("\"closure_gap_cm\":%.2f,\"contact_fraction\":%.3f,\"start_position_error_cm\":%.1f,\"start_heading_error_deg\":%.2f,")
		TEXT("\"checkpoint_sequence\":\"%s\",\"lap_distance_cm\":%.1f,\"lap_completed\":%s,\"frames\":%d,\"note\":\"%s\"}"),
		bLoad ? TEXT("true") : TEXT("false"), bContact ? TEXT("true") : TEXT("false"),
		bAlign ? TEXT("true") : TEXT("false"), bCenter ? TEXT("true") : TEXT("false"),
		bOrderOk ? TEXT("true") : TEXT("false"), bLap ? TEXT("true") : TEXT("false"),
		Segs, CenterN, CheckN, Length, Width, Closure, ContactFrac, StartPosErr, StartYawErr,
		*Seq, LapDist, bLapDone ? TEXT("true") : TEXT("false"), Frames, *Note);

	const FString Dir = FPaths::ProjectSavedDir() + TEXT("Task7E2E/");
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	PF.CreateDirectoryTree(*Dir);
	FFileHelper::SaveStringToFile(Json, *(Dir + TEXT("results.json")));
	UE_LOG(LogTemp, Display, TEXT("TRACK7E2E: load=%d contact=%d align=%d center=%d order=%d lap=%d all=%d"),
		bLoad, bContact, bAlign, bCenter, bOrderOk, bLap, bAll);
}
