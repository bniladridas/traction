// See header.

#include "Task15Probe.h"
#include "RaceVehicle.h"
#include "RaceTrack.h"
#include "RaceManager.h"
#include "RaceAIDriver.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

ATask15Probe::ATask15Probe()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATask15Probe::BeginPlay()
{
	Super::BeginPlay();
	for (TActorIterator<ARaceTrack> It(GetWorld()); It; ++It)
	{
		Track = *It;
	}
	for (TActorIterator<ARaceManager> It(GetWorld()); It; ++It)
	{
		Manager = *It;
	}
	if (!Track || !Manager)
	{
		Finish(false, TEXT("missing track or race manager"));
	}
}

int32 ATask15Probe::NearestIndex(const FVector& Pos) const
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

FString ATask15Probe::OrderString() const
{
	struct FEntry
	{
		int32 Pos;
		int32 Idx;
	};
	TArray<FEntry> Es;
	for (int32 i = 0; i < Manager->GetParticipantCount(); ++i)
	{
		FEntry E;
		E.Pos = Manager->GetPosition(Manager->GetParticipantVehicle(i));
		E.Idx = i;
		Es.Add(E);
	}
	Es.Sort([](const FEntry& A, const FEntry& B) { return A.Pos < B.Pos; });
	FString S;
	for (const FEntry& E : Es)
	{
		S.AppendChar(TEXT('0') + E.Idx);
	}
	return S;
}

void ATask15Probe::DrivePlayer()
{	const TArray<FRaceTrackCenterPoint>& Pts = Track->GetCenterPoints();
	const float L = Track->GetTrackLength();
	const int32 N = Pts.Num();
	const int32 Idx = NearestIndex(Player->GetActorLocation());
	if (!bPlayerAnchored)
	{
		bPlayerAnchored = true;
		PlayerIdx = Idx;
		PlayerS = Pts[Idx].Distance;
	}
	int32 IdxDelta = Idx - PlayerIdx;
	if (IdxDelta < -N / 2)
	{
		IdxDelta += N;
	}
	else if (IdxDelta > N / 2)
	{
		IdxDelta -= N;
	}
	PlayerIdx = Idx;
	float SNow = Pts[Idx].Distance;
	while (SNow < PlayerS - L * 0.5f)
	{
		SNow += L;
	}
	while (SNow > PlayerS + L * 0.5f)
	{
		SNow -= L;
	}
	PlayerS = SNow;

	const float Speed = Player->GetForwardSpeed();
	const FRaceTrackCenterPoint Tgt = Track->SampleAtDistance(PlayerS + 400.0f + FMath::Abs(Speed) * 0.3f);
	const FVector ToTgt = Tgt.Position - Player->GetActorLocation();
	const float DesiredYaw = FMath::RadiansToDegrees(FMath::Atan2(ToTgt.Y, ToTgt.X));
	const float YawErr = FRotator::NormalizeAxis(DesiredYaw - Player->GetActorRotation().Yaw);
	Player->ApplySteering(FMath::Clamp(YawErr / 18.0f, -1.0f, 1.0f));

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
		Player->ApplyThrottle(1.0f);
		Player->ApplyBrake(0.0f);
	}
	else if (Speed > Target + 50.0f && Speed > 200.0f)
	{
		Player->ApplyThrottle(0.0f);
		Player->ApplyBrake(1.0f);
	}
	else
	{
		Player->ApplyThrottle(0.4f);
		Player->ApplyBrake(0.0f);
	}
}

void ATask15Probe::Tick(float Delta)
{
	Super::Tick(Delta);
	if (bFinished || !Track || !Manager)
	{
		return;
	}
	Elapsed += Delta;
	Frames++;

	if (!Player)
	{
		if (APawn* P = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
		{
			Player = Cast<ARaceVehicle>(P);
			if (Player)
			{
				if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
				{
					Player->DisableInput(PC);
				}
				UE_LOG(LogTemp, Display, TEXT("RACERES15E2E: player acquired"));
			}
			else
			{
				Finish(false, TEXT("player pawn is not an ARaceVehicle"));
				return;
			}
		}
		if (!Player)
		{
			if (Elapsed > 10.0)
			{
				Finish(false, TEXT("no player pawn within timeout"));
			}
			return;
		}
	}
	if (!bFieldReady && Manager->GetParticipantCount() >= Task15Limits::FieldSize)
	{
		bool bAll = true;
		for (int32 i = 1; i <= 5; ++i)
		{
			ARaceVehicle* AI = Manager->GetParticipantVehicle(i);
			AIs[i - 1] = AI;
			bAll = bAll && (AI != nullptr);
			if (AI)
			{
				TArray<UActorComponent*> Comps;
				AI->GetComponents(URaceAIDriver::StaticClass(), Comps);
				if (Comps.Num() > 0)
				{
					Drivers[i - 1] = Cast<URaceAIDriver>(Comps[0]);
				}
				bAll = bAll && (Drivers[i - 1] != nullptr);
			}
		}
		if (bAll)
		{
			bFieldReady = true;
			UE_LOG(LogTemp, Display, TEXT("RACERES15E2E: field acquired"));
		}
	}

	const bool bRacing = static_cast<int32>(Manager->GetPhase()) == 2;
	if (!bStartSent && Elapsed >= 1.0)
	{
		bStartSent = true;
		Manager->StartRace();
	}
	if (bRacing && !bRacingSeen)
	{
		bRacingSeen = true;
		RacingStartTime = Elapsed;
		for (int32 i = 0; i < 5; ++i)
		{
			LastMoveT[i] = Elapsed;
		}
	}
	if (bRacing)
	{
		DrivePlayer();
		// Player recovery mirrors the AI driver rule: without it a
		// traffic-wrecked player can circle invalidly forever while every
		// AI finishes, failing the field on one car.
		if (!Manager->IsParticipantFinished(0))
		{
			if ((PlayerS - PlayerCheckS) > 50.0f)
			{
				PlayerCheckT = Elapsed;
				PlayerCheckS = PlayerS;
			}
			if ((Elapsed - PlayerCheckT) > Task15Limits::StallWindow)
			{
				const TArray<FRaceTrackCenterPoint>& Pts = Track->GetCenterPoints();
				const int32 Idx = NearestIndex(Player->GetActorLocation());
				const FRaceTrackCenterPoint& P = Pts[Idx];
				const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(P.Forward.Y, P.Forward.X));
				Player->SetActorLocationAndRotation(FVector(P.Position.X, P.Position.Y, 40.0f),
					FRotator(0.0f, Yaw, 0.0f), false, nullptr, ETeleportType::TeleportPhysics);
				Player->ResetMotion();
				Manager->ReanchorParticipant(Player);
				bPlayerAnchored = false;
				PlayerCheckT = Elapsed;
				PlayerCheckS = PlayerS;
				UE_LOG(LogTemp, Display, TEXT("RACERES15E2E: player recovered at s=%.0f"), PlayerS);
			}
		}
	}

	// Empty-before-finish latch: no results while nobody has finished.
	if (!bEmptyChecked && bRacingSeen)
	{
		bool bAnyFinished = false;
		for (int32 i = 0; i < 6; ++i)
		{
			bAnyFinished = bAnyFinished || Manager->IsParticipantFinished(i);
		}
		if (!bAnyFinished && Elapsed - RacingStartTime > 5.0)
		{
			bEmptyChecked = true;
			bEmptyEarly = !Manager->HasResults();
			UE_LOG(LogTemp, Display, TEXT("RACERES15E2E: empty early=%d"), bEmptyEarly);
		}
	}

	// Mid-program reset with grid re-staging, then restart.
	if (!bResetDone && bRacingSeen && Elapsed >= Task15Limits::ResetAt)
	{
		bResetDone = true;
		Player->ResetVehicle();
		Manager->OnVehicleReset();
		{
			FVector Start = Track->GetStartPosition();
			Start.Z = 40.0f;
			Player->SetActorLocationAndRotation(Start,
				FRotator(0.0f, Track->GetStartYawDeg(), 0.0f), false, nullptr, ETeleportType::TeleportPhysics);
			Player->ResetMotion();
		}
		for (int32 Slot = 1; Slot <= 5; ++Slot)
		{
			ARaceVehicle* AI = AIs[Slot - 1];
			FVector Loc;
			float Yaw = 0.0f;
			Track->GetGridPose(Slot, Loc, Yaw);
			Loc.Z = 40.0f;
			AI->SetActorLocationAndRotation(Loc, FRotator(0.0f, Yaw, 0.0f),
				false, nullptr, ETeleportType::TeleportPhysics);
			AI->ResetMotion();
			Manager->ReanchorParticipant(AI);
			if (Drivers[Slot - 1])
			{
				Drivers[Slot - 1]->Reanchor();
			}
		}
		bResetLaps = true;
		for (int32 i = 0; i < 6; ++i)
		{
			bResetLaps = bResetLaps && (Manager->GetParticipantLaps(i) == 0);
		}
		bResetCleared = bResetLaps && !Manager->HasResults();
		Manager->StartRace();
		bWaitRacing = true;
		bRacingSeen = false;
		bPlayerAnchored = false;
		PlayerCheckT = Elapsed;
		PlayerCheckS = -1e9f;
		for (int32 i = 0; i < 5; ++i)
		{
			LastMoveT[i] = Elapsed;
		}
		UE_LOG(LogTemp, Display, TEXT("RACERES15E2E: reset cleared=%d"), bResetCleared);
	}
	if (bWaitRacing && bRacing)
	{
		bWaitRacing = false;
		bRacingSeen = true;
		RacingStartTime = Elapsed;
		for (int32 i = 0; i < 5; ++i)
		{
			LastMoveT[i] = Elapsed;
			if (Drivers[i])
			{
				LastDist[i] = Drivers[i]->GetProgressDistance();
				LastRec[i] = Drivers[i]->GetRecoveryCount();
			}
		}
		UE_LOG(LogTemp, Display, TEXT("RACERES15E2E: racing again"));
	}

	// Deadlock monitoring across the AI field.
	if (bRacingSeen && bFieldReady && !bWaitRacing)
	{
		for (int32 i = 0; i < 5; ++i)
		{
			const float D = Drivers[i]->GetProgressDistance();
			if ((D - LastDist[i]) > 5.0f || Drivers[i]->GetRecoveryCount() != LastRec[i])
			{
				LastMoveT[i] = Elapsed;
				LastDist[i] = D;
				LastRec[i] = Drivers[i]->GetRecoveryCount();
			}
			MaxGap = FMath::Max(MaxGap, Elapsed - LastMoveT[i]);
			if (!Manager->IsParticipantFinished(i + 1) && (Elapsed - LastMoveT[i]) > Task15Limits::StallWindow)
			{
				bDeadlockOk = false;
				Finish(false, TEXT("deadlock: AI stalled past window"));
				return;
			}
		}
	}

	// Periodic status: per-participant laps and order.
	static double LastStatus = 0.0;
	if (Elapsed - LastStatus > 10.0)
	{
		LastStatus = Elapsed;
		UE_LOG(LogTemp, Display, TEXT("RACERES15E2E: t=%.1f phase=%d laps=%d/%d/%d/%d/%d/%d order=%s"),
			Elapsed, static_cast<int32>(Manager->GetPhase()),
			Manager->GetParticipantLaps(0), Manager->GetParticipantLaps(1), Manager->GetParticipantLaps(2),
			Manager->GetParticipantLaps(3), Manager->GetParticipantLaps(4), Manager->GetParticipantLaps(5),
			*OrderString());
	}

	// Completion: every participant finished.
	if (bRacingSeen && !bWaitRacing && !bImmutableStaged
		&& Manager->GetParticipantCount() == Task15Limits::FieldSize)
	{
		bool bAll = true;
		for (int32 i = 0; i < 6; ++i)
		{
			bAll = bAll && Manager->IsParticipantFinished(i);
		}
		if (bAll)
		{
			bPopulated = Manager->HasResults();
			if (bPopulated)
			{
				const FRaceResults& R = Manager->GetResults();
				bPopulated = (R.Ordered.Num() == 6);
				bConsistent = true;
				for (const FRaceResultEntry& E : R.Ordered)
				{
					bConsistent = bConsistent && (E.CompletedLaps == 2)
						&& (E.BestLapTime > 0.0f) && (E.LastLapTime > 0.0f)
						&& (E.BestLapTime <= E.LastLapTime);
				}
				// Order matches finish sequence by construction check:
				// positions 1..6 assigned bijectively.
				bool bDistinct = true;
				for (int32 c = 0; c < 6 && bDistinct; ++c)
				{
					for (int32 k = c + 1; k < 6; ++k)
					{
						bDistinct = bDistinct && (Manager->GetPosition(Manager->GetParticipantVehicle(c)) != Manager->GetPosition(Manager->GetParticipantVehicle(k)));
					}
				}
				bPopulated = bPopulated && bDistinct;
			}
			// Immutability probe, staged across ticks so the crossing is
			// genuinely observed: park before CP0, cross it next tick.
			const TArray<FRaceTrackCheckpoint>& CPs = Track->GetCheckpoints();
			FVector Loc = CPs[0].Position - CPs[0].Forward * 150.0f + FVector(0.0f, 0.0f, 40.0f);
			FRotator Rot = FRotator(0.0f, FMath::RadiansToDegrees(FMath::Atan2(CPs[0].Forward.Y, CPs[0].Forward.X)), 0.0f);
			Player->SetActorLocationAndRotation(Loc, Rot, false, nullptr, ETeleportType::TeleportPhysics);
			for (int32 i = 0; i < 6; ++i)
			{
				SnapLaps[i] = Manager->GetParticipantLaps(i);
			}
			SnapOrder = OrderString();
			// Snapshot the immutable results table itself (not live
			// positions, which keep updating as cars move post-finish).
			{
				const FRaceResults& R = Manager->GetResults();
				for (int32 i = 0; i < 6; ++i)
				{
					SnapResultOrder[i] = R.Ordered[i].ParticipantIndex;
					SnapBest[i] = R.Ordered[i].BestLapTime;
					SnapLast[i] = R.Ordered[i].LastLapTime;
				}
			}
			bImmutableArmed = true;
			bImmutableStaged = true;
			ImmutableStageTime = Elapsed;
			UE_LOG(LogTemp, Display, TEXT("RACERES15E2E: all finished, immutable probe staged"));
		}
	}
	if (bImmutableArmed && !bImmutableCrossed && (Elapsed - ImmutableStageTime) > 0.5)
	{
		// Cross CP0: must be ignored in Finished phase.
		const TArray<FRaceTrackCheckpoint>& CPs = Track->GetCheckpoints();
		FVector Loc = CPs[0].Position + CPs[0].Forward * 150.0f + FVector(0.0f, 0.0f, 40.0f);
		FRotator Rot = FRotator(0.0f, FMath::RadiansToDegrees(FMath::Atan2(CPs[0].Forward.Y, CPs[0].Forward.X)), 0.0f);
		Player->SetActorLocationAndRotation(Loc, Rot, false, nullptr, ETeleportType::TeleportPhysics);
		bImmutableCrossed = true;
		ImmutableCheckAt = Elapsed + 1.0;
	}
	if (bImmutableCrossed && Elapsed >= ImmutableCheckAt)
	{
		const FRaceResults& R = Manager->GetResults();
		bImmutable = Manager->HasResults() && (R.Ordered.Num() == 6);
		// The results table must not move: same finish order, laps, and
		// times as snapshotted. Live positions may differ (cars keep
		// moving); only the table is asserted.
		for (int32 i = 0; bImmutable && i < 6; ++i)
		{
			bImmutable = bImmutable
				&& (R.Ordered[i].ParticipantIndex == SnapResultOrder[i])
				&& (R.Ordered[i].BestLapTime == SnapBest[i])
				&& (R.Ordered[i].LastLapTime == SnapLast[i])
				&& (Manager->GetParticipantLaps(i) == SnapLaps[i]);
		}
		UE_LOG(LogTemp, Display, TEXT("RACERES15E2E: immutable=%d"), bImmutable);
		Finish(true, TEXT("results complete"));
		return;
	}

	if (Elapsed > Task15Limits::ProgramTimeout)
	{
		Finish(false, TEXT("program timeout"));
	}
}

void ATask15Probe::Finish(bool bOk, const FString& Note)
{
	if (bFinished)
	{
		return;
	}
	bFinished = true;
	if (Player)
	{
		Player->ApplyThrottle(0.0f);
		Player->ApplyBrake(0.0f);
		Player->ApplySteering(0.0f);
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			Player->EnableInput(PC);
		}
	}
	WriteResults(bOk, Note);
	UE_LOG(LogTemp, Display, TEXT("RACERES15E2E: finishing (%s)"), *Note);
	UKismetSystemLibrary::QuitGame(GetWorld(), nullptr, EQuitPreference::Quit, false);
}

void ATask15Probe::WriteResults(bool bOk, const FString& Note) const
{
	const bool bEmpty = bEmptyChecked && bEmptyEarly;
	const bool bPop = bPopulated;
	const bool bCons = bConsistent;
	const bool bClear = bResetDone && bResetLaps && bResetCleared;
	const bool bImm = bImmutable;
	const bool bDead = bDeadlockOk;
	const bool bAll = bOk && bEmpty && bPop && bCons && bClear && bImm && bDead;

	const FString Json = FString::Printf(
		TEXT("{\"results_empty\":%s,\"results_populated\":%s,\"times_consistent\":%s,\"reset_clears\":%s,\"results_immutable\":%s,\"no_deadlock\":%s,")
		TEXT("\"frames\":%d,\"note\":\"%s\"}"),
		bEmpty ? TEXT("true") : TEXT("false"), bPop ? TEXT("true") : TEXT("false"),
		bCons ? TEXT("true") : TEXT("false"), bClear ? TEXT("true") : TEXT("false"),
		bImm ? TEXT("true") : TEXT("false"), bDead ? TEXT("true") : TEXT("false"),
		Frames, *Note);

	const FString Dir = FPaths::ProjectSavedDir() + TEXT("Task15E2E/");
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	PF.CreateDirectoryTree(*Dir);
	FFileHelper::SaveStringToFile(Json, *(Dir + TEXT("results.json")));
	UE_LOG(LogTemp, Display, TEXT("RACERES15E2E: empty=%d populated=%d consistent=%d cleared=%d immutable=%d deadlock=%d all=%d"),
		bEmpty, bPop, bCons, bClear, bImm, bDead, bAll);
}
