// See header.

#include "Task12Probe.h"
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

ATask12Probe::ATask12Probe()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATask12Probe::BeginPlay()
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

int32 ATask12Probe::NearestIndex(const FVector& Pos) const
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

void ATask12Probe::DrivePlayer()
{
	const TArray<FRaceTrackCenterPoint>& Pts = Track->GetCenterPoints();
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

FString ATask12Probe::OrderString() const
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

void ATask12Probe::Tick(float Delta)
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
				UE_LOG(LogTemp, Display, TEXT("RACEFIELD12E2E: player acquired"));
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
	if ((!AI0 || !AI1) && Manager->GetParticipantCount() >= 3)
	{
		AI0 = Manager->GetParticipantVehicle(1);
		AI1 = Manager->GetParticipantVehicle(2);
		if (AI0 && AI1)
		{
			TArray<UActorComponent*> C0;
			AI0->GetComponents(URaceAIDriver::StaticClass(), C0);
			if (C0.Num() > 0)
			{
				Driver0 = Cast<URaceAIDriver>(C0[0]);
			}
			TArray<UActorComponent*> C1;
			AI1->GetComponents(URaceAIDriver::StaticClass(), C1);
			if (C1.Num() > 0)
			{
				Driver1 = Cast<URaceAIDriver>(C1[0]);
			}
			UE_LOG(LogTemp, Display, TEXT("RACEFIELD12E2E: field acquired drivers=%d/%d"),
				Driver0 != nullptr, Driver1 != nullptr);
		}
	}

	const bool bRacing = static_cast<int32>(Manager->GetPhase()) == 2;
	if (!bStartSent && Elapsed >= 1.0)
	{
		bStartSent = true;
		Manager->StartRace();
		// Grid assertion on the staged field.
		if (Player && AI0 && AI1)
		{
			const float D01 = FVector::Dist(Player->GetActorLocation(), AI0->GetActorLocation());
			const float D02 = FVector::Dist(Player->GetActorLocation(), AI1->GetActorLocation());
			const float D12 = FVector::Dist(AI0->GetActorLocation(), AI1->GetActorLocation());
			bGridCountOk = (Manager->GetParticipantCount() == Task12Limits::FieldSize)
				&& (D01 > Task12Limits::GridMinSeparationCm)
				&& (D02 > Task12Limits::GridMinSeparationCm)
				&& (D12 > Task12Limits::GridMinSeparationCm);
			GridOrder = OrderString();
			bGridOrderOk = (GridOrder == TEXT("012"));
			UE_LOG(LogTemp, Display, TEXT("RACEFIELD12E2E: grid ok=%d order=%s d=%.0f/%.0f/%.0f"),
				bGridCountOk, *GridOrder, D01, D02, D12);
		}
	}
	if (bRacing && !bRacingSeen)
	{
		bRacingSeen = true;
		RacingStartTime = Elapsed;
		LastMoveT0 = Elapsed;
		LastMoveT1 = Elapsed;
	}
	if (bRacing)
	{
		DrivePlayer();
	}

	// Mid-program reset: player reset plus manager reset, AIs re-staged
	// to grid slots, then restart. Exercises reset clearing mid-field.
	if (!bResetDone && bRacingSeen && Elapsed >= Task12Limits::ResetAt)
	{
		bResetDone = true;
		Player->ResetVehicle();
		Manager->OnVehicleReset();
		// Rest height exactly, not the snap height: a mid-run drop from
		// snap height freezes on first trace contact (grounded rule), so
		// every staged placement lands at box rest height instead.
		{
			FVector Start = Track->GetStartPosition();
			Start.Z = 40.0f;
			Player->SetActorLocationAndRotation(Start,
				FRotator(0.0f, Track->GetStartYawDeg(), 0.0f), false, nullptr, ETeleportType::TeleportPhysics);
			Player->ResetMotion();
		}
		for (int32 Slot = 1; Slot <= 2; ++Slot)
		{
			ARaceVehicle* AI = (Slot == 1) ? AI0 : AI1;
			FVector Loc;
			float Yaw = 0.0f;
			Track->GetGridPose(Slot, Loc, Yaw);
			Loc.Z = 40.0f;
			AI->SetActorLocationAndRotation(Loc, FRotator(0.0f, Yaw, 0.0f),
				false, nullptr, ETeleportType::TeleportPhysics);
			AI->ResetMotion();
			Manager->ReanchorParticipant(AI);
			if (URaceAIDriver* D = (Slot == 1) ? Driver0 : Driver1)
			{
				D->Reanchor();
			}
		}
		bResetLaps = (Manager->GetParticipantLaps(0) == 0)
			&& (Manager->GetParticipantLaps(1) == 0)
			&& (Manager->GetParticipantLaps(2) == 0);
		ResetOrder = OrderString();
		bResetOrderOk = (ResetOrder == TEXT("012"));
		Manager->StartRace();
		bWaitRacing = true;
		bRacingSeen = false;
		bPlayerAnchored = false;
		LastMoveT0 = Elapsed;
		LastMoveT1 = Elapsed;
		UE_LOG(LogTemp, Display, TEXT("RACEFIELD12E2E: reset order=%s laps0=%d"), *ResetOrder, bResetLaps);
	}
	if (bWaitRacing && bRacing)
	{
		bWaitRacing = false;
		RacingStartTime = Elapsed;
		LastMoveT0 = Elapsed;
		LastMoveT1 = Elapsed;
		if (Driver0)
		{
			LastDist0 = Driver0->GetProgressDistance();
			LastRec0 = Driver0->GetRecoveryCount();
		}
		if (Driver1)
		{
			LastDist1 = Driver1->GetProgressDistance();
			LastRec1 = Driver1->GetRecoveryCount();
		}
		UE_LOG(LogTemp, Display, TEXT("RACEFIELD12E2E: racing again"));
	}

	// Progress, laps, deadlock monitoring across the field.
	if (bRacingSeen && Driver0 && Driver1 && !bWaitRacing)
	{
		const float D0 = Driver0->GetProgressDistance();
		const float D1 = Driver1->GetProgressDistance();
		if (!bProgressLogged && D0 > Task12Limits::ProgressMinCm && D1 > Task12Limits::ProgressMinCm)
		{
			bProgressLogged = true;
			ProgressTime = Elapsed;
			UE_LOG(LogTemp, Display, TEXT("RACEFIELD12E2E: field progress at t=%.1f"), Elapsed - RacingStartTime);
		}
		// Movement (or a recovery respawn) resets the stall gap.
		if ((D0 - LastDist0) > 5.0f || Driver0->GetRecoveryCount() != LastRec0)
		{
			LastMoveT0 = Elapsed;
			LastDist0 = D0;
			LastRec0 = Driver0->GetRecoveryCount();
		}
		if ((D1 - LastDist1) > 5.0f || Driver1->GetRecoveryCount() != LastRec1)
		{
			LastMoveT1 = Elapsed;
			LastDist1 = D1;
			LastRec1 = Driver1->GetRecoveryCount();
		}
		MaxGap = FMath::Max(MaxGap, FMath::Max(Elapsed - LastMoveT0, Elapsed - LastMoveT1));
		const bool bUnfinished0 = !Manager->IsParticipantFinished(1);
		const bool bUnfinished1 = !Manager->IsParticipantFinished(2);
		if ((bUnfinished0 && (Elapsed - LastMoveT0) > Task12Limits::StallWindow)
			|| (bUnfinished1 && (Elapsed - LastMoveT1) > Task12Limits::StallWindow))
		{
			bDeadlockOk = false;
			Finish(false, TEXT("deadlock: AI stalled past window"));
			return;
		}
	}

	// Heartbeat: positions, speeds, phase, laps across the field.
	static double LastBeat = 0.0;
	if (Elapsed - LastBeat > 5.0)
	{
		LastBeat = Elapsed;
		UE_LOG(LogTemp, Display, TEXT("RACEFIELD12E2E: t=%.1f phase=%d laps=%d/%d/%d v=%.0f/%.0f/%.0f pos=%s / %s / %s"),
			Elapsed, static_cast<int32>(Manager->GetPhase()),
			Manager->GetParticipantLaps(0), Manager->GetParticipantLaps(1), Manager->GetParticipantLaps(2),
			Player ? Player->GetForwardSpeed() : -999.0f,
			AI0 ? AI0->GetForwardSpeed() : -999.0f,
			AI1 ? AI1->GetForwardSpeed() : -999.0f,
			Player ? *Player->GetActorLocation().ToString() : TEXT("none"),
			AI0 ? *AI0->GetActorLocation().ToString() : TEXT("none"),
			AI1 ? *AI1->GetActorLocation().ToString() : TEXT("none"));
	}

	// Completion: every participant finished.
	if (bRacingSeen && !bWaitRacing && Manager->GetParticipantCount() == Task12Limits::FieldSize)
	{
		bool bAll = true;
		for (int32 i = 0; i < Task12Limits::FieldSize; ++i)
		{
			bAll = bAll && Manager->IsParticipantFinished(i);
		}
		if (bAll)
		{
			FinalOrder = OrderString();
			LapsP = Manager->GetParticipantLaps(0);
			Laps0 = Manager->GetParticipantLaps(1);
			Laps1 = Manager->GetParticipantLaps(2);
			Finish(true, TEXT("field finished"));
			return;
		}
	}

	if (Elapsed > Task12Limits::ProgramTimeout)
	{
		Finish(false, TEXT("program timeout"));
	}
}

void ATask12Probe::Finish(bool bOk, const FString& Note)
{
	if (bFinished)
	{
		return;
	}
	bFinished = true;
	for (ARaceVehicle* V : { Player, AI0, AI1 })
	{
		if (V)
		{
			V->ApplyThrottle(0.0f);
			V->ApplyBrake(0.0f);
			V->ApplySteering(0.0f);
		}
	}
	if (Player)
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			Player->EnableInput(PC);
		}
	}
	WriteResults(bOk, Note);
	UE_LOG(LogTemp, Display, TEXT("RACEFIELD12E2E: finishing (%s)"), *Note);
	UKismetSystemLibrary::QuitGame(GetWorld(), nullptr, EQuitPreference::Quit, false);
}

void ATask12Probe::WriteResults(bool bOk, const FString& Note) const
{
	const bool bSpawned = bGridCountOk && bGridOrderOk;
	const bool bProgress = bProgressLogged
		&& (ProgressTime - RacingStartTime) < Task12Limits::ProgressWindow;
	const bool bLaps = (Manager->GetParticipantLaps(1) >= 1) && (Manager->GetParticipantLaps(2) >= 1)
		&& Manager->WasParticipantLastValid(1) && Manager->WasParticipantLastValid(2);
	const bool bFinish = (FinalOrder.Len() == 3) && (LapsP == 2) && (Laps0 == 2) && (Laps1 == 2)
		&& (FinalOrder == TEXT("012"));
	const bool bDeadlock = bDeadlockOk;
	const bool bReset = bResetDone && bResetLaps && bResetOrderOk;
	const bool bAll = bOk && bSpawned && bProgress && bLaps && bFinish && bDeadlock && bReset;

	const FString Json = FString::Printf(
		TEXT("{\"field_spawned\":%s,\"field_progress\":%s,\"field_laps\":%s,\"finish_order\":%s,\"no_deadlock\":%s,\"reset_clears\":%s,")
		TEXT("\"grid_order\":\"%s\",\"final_order\":\"%s\",\"reset_order\":\"%s\",")
		TEXT("\"laps_p\":%d,\"laps_0\":%d,\"laps_1\":%d,\"max_gap\":%.2f,\"frames\":%d,\"note\":\"%s\"}"),
		bSpawned ? TEXT("true") : TEXT("false"), bProgress ? TEXT("true") : TEXT("false"),
		bLaps ? TEXT("true") : TEXT("false"), bFinish ? TEXT("true") : TEXT("false"),
		bDeadlock ? TEXT("true") : TEXT("false"), bReset ? TEXT("true") : TEXT("false"),
		*GridOrder, *FinalOrder, *ResetOrder, LapsP, Laps0, Laps1, MaxGap, Frames, *Note);

	const FString Dir = FPaths::ProjectSavedDir() + TEXT("Task12E2E/");
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	PF.CreateDirectoryTree(*Dir);
	FFileHelper::SaveStringToFile(Json, *(Dir + TEXT("results.json")));
	UE_LOG(LogTemp, Display, TEXT("RACEFIELD12E2E: spawned=%d progress=%d laps=%d finish=%d deadlock=%d reset=%d all=%d"),
		bSpawned, bProgress, bLaps, bFinish, bDeadlock, bReset, bAll);
}
