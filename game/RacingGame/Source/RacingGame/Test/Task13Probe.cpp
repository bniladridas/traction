// See header.

#include "Task13Probe.h"
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

ATask13Probe::ATask13Probe()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATask13Probe::BeginPlay()
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

void ATask13Probe::ParkPlayer()
{
	// Parked clear of both AI lines: behind the grid, near the right
	// edge, on the road, zero input. Single overtake story between the
	// AIs; the player is present but stationary.
	const FRaceTrackCenterPoint Park = Track->SampleAtDistance(100.0f);
	const FVector Right(-Park.Forward.Y, Park.Forward.X, 0.0f);
	const FVector Spot = Park.Position + Right * 330.0f + FVector(0.0f, 0.0f, 40.0f);
	const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Park.Forward.Y, Park.Forward.X));
	Player->SetActorLocationAndRotation(Spot, FRotator(0.0f, Yaw, 0.0f),
		false, nullptr, ETeleportType::TeleportPhysics);
	Player->ResetMotion();
	bPlayerParked = true;
	UE_LOG(LogTemp, Display, TEXT("RACEPACE13E2E: player parked"));
}

FString ATask13Probe::OrderString() const
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

void ATask13Probe::Tick(float Delta)
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
				UE_LOG(LogTemp, Display, TEXT("RACEPACE13E2E: player acquired"));
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
			if (Driver0 && Driver1)
			{
				bPaceOk = (Driver0->GetPaceFactor() == Task13Limits::PaceSlow)
					&& (Driver1->GetPaceFactor() == Task13Limits::PaceFast);
			}
			UE_LOG(LogTemp, Display, TEXT("RACEPACE13E2E: field acquired pace=%d"), bPaceOk);
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
		LastMoveT0 = Elapsed;
		LastMoveT1 = Elapsed;
	}
	if (bRacing && !bPlayerParked)
	{
		ParkPlayer();
		EarlyOrder = OrderString();
		bEarlyOk = (EarlyOrder == TEXT("120"));
		UE_LOG(LogTemp, Display, TEXT("RACEPACE13E2E: early order=%s"), *EarlyOrder);
	}

	// Mid-program reset with AI grid re-staging, then restart.
	if (!bResetDone && bRacingSeen && Elapsed >= Task13Limits::ResetAt)
	{
		bResetDone = true;
		Player->ResetVehicle();
		Manager->OnVehicleReset();
		ParkPlayer();
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
		bResetOrderOk = (ResetOrder == TEXT("120"));
		Manager->StartRace();
		bWaitRacing = true;
		bRacingSeen = false;
		bPlayerParked = false;
		LastMoveT0 = Elapsed;
		LastMoveT1 = Elapsed;
		UE_LOG(LogTemp, Display, TEXT("RACEPACE13E2E: reset order=%s laps0=%d"), *ResetOrder, bResetLaps);
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
		UE_LOG(LogTemp, Display, TEXT("RACEPACE13E2E: racing again"));
	}

	// Progress, laps, deadlock monitoring across the AI pair.
	if (bRacingSeen && Driver0 && Driver1 && !bWaitRacing)
	{
		const float D0 = Driver0->GetProgressDistance();
		const float D1 = Driver1->GetProgressDistance();
		if (!bProgressLogged && D0 > Task13Limits::ProgressMinCm && D1 > Task13Limits::ProgressMinCm)
		{
			bProgressLogged = true;
			ProgressTime = Elapsed;
			UE_LOG(LogTemp, Display, TEXT("RACEPACE13E2E: field progress at t=%.1f"), Elapsed - RacingStartTime);
		}
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
		if ((bUnfinished0 && (Elapsed - LastMoveT0) > Task13Limits::StallWindow)
			|| (bUnfinished1 && (Elapsed - LastMoveT1) > Task13Limits::StallWindow))
		{
			bDeadlockOk = false;
			Finish(false, TEXT("deadlock: AI stalled past window"));
			return;
		}
	}

	// Completion: both AI finished.
	if (bRacingSeen && !bWaitRacing && Manager->GetParticipantCount() == Task13Limits::FieldSize)
	{
		if (Manager->IsParticipantFinished(1) && Manager->IsParticipantFinished(2))
		{
			FinalOrder = OrderString();
			Laps1 = Manager->GetParticipantLaps(1);
			Laps2 = Manager->GetParticipantLaps(2);
			Best1 = Manager->GetParticipantBestLap(1);
			Best2 = Manager->GetParticipantBestLap(2);
			Finish(true, TEXT("pace race finished"));
			return;
		}
	}

	if (Elapsed > Task13Limits::ProgramTimeout)
	{
		Finish(false, TEXT("program timeout"));
	}
}

void ATask13Probe::Finish(bool bOk, const FString& Note)
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
	UE_LOG(LogTemp, Display, TEXT("RACEPACE13E2E: finishing (%s)"), *Note);
	UKismetSystemLibrary::QuitGame(GetWorld(), nullptr, EQuitPreference::Quit, false);
}

void ATask13Probe::WriteResults(bool bOk, const FString& Note) const
{
	const bool bSpread = (Best1 > 0.0f) && (Best2 > 0.0f) && ((Best1 - Best2) > Task13Limits::LapMargin);
	const bool bOvertake = (EarlyOrder == TEXT("120")) && (FinalOrder == TEXT("210"));
	const bool bFinish = (FinalOrder == TEXT("210")) && (Laps1 == 2) && (Laps2 == 2);
	const bool bProgress = bProgressLogged
		&& (ProgressTime - RacingStartTime) < Task13Limits::ProgressWindow;
	const bool bDeadlock = bDeadlockOk;
	const bool bReset = bResetDone && bResetLaps && bResetOrderOk;
	const bool bAll = bOk && bPaceOk && bSpread && bOvertake && bFinish && bDeadlock && bReset;

	const FString Json = FString::Printf(
		TEXT("{\"pace_assigned\":%s,\"lap_spread\":%s,\"overtake_emerged\":%s,\"finish_pace\":%s,\"no_deadlock\":%s,\"reset_clears\":%s,")
		TEXT("\"early_order\":\"%s\",\"final_order\":\"%s\",\"reset_order\":\"%s\",")
		TEXT("\"laps_1\":%d,\"laps_2\":%d,\"best_1\":%.2f,\"best_2\":%.2f,\"max_gap\":%.2f,\"frames\":%d,\"note\":\"%s\"}"),
		bPaceOk ? TEXT("true") : TEXT("false"), bSpread ? TEXT("true") : TEXT("false"),
		bOvertake ? TEXT("true") : TEXT("false"), bFinish ? TEXT("true") : TEXT("false"),
		bDeadlock ? TEXT("true") : TEXT("false"), bReset ? TEXT("true") : TEXT("false"),
		*EarlyOrder, *FinalOrder, *ResetOrder, Laps1, Laps2, Best1, Best2, MaxGap, Frames, *Note);

	const FString Dir = FPaths::ProjectSavedDir() + TEXT("Task13E2E/");
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	PF.CreateDirectoryTree(*Dir);
	FFileHelper::SaveStringToFile(Json, *(Dir + TEXT("results.json")));
	UE_LOG(LogTemp, Display, TEXT("RACEPACE13E2E: pace=%d spread=%d overtake=%d finish=%d deadlock=%d reset=%d all=%d"),
		bPaceOk, bSpread, bOvertake, bFinish, bDeadlock, bReset, bAll);
}
