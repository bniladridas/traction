// See header.

#include "Task14Probe.h"
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

ATask14Probe::ATask14Probe()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATask14Probe::BeginPlay()
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

void ATask14Probe::ParkPlayer()
{
	// Parked clear of every AI line: ahead of the grid on the far side,
	// on the road, zero input. The pace story belongs to the AI field.
	const FRaceTrackCenterPoint Park = Track->SampleAtDistance(600.0f);
	const FVector Right(-Park.Forward.Y, Park.Forward.X, 0.0f);
	const FVector Spot = Park.Position + Right * 330.0f + FVector(0.0f, 0.0f, 40.0f);
	const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Park.Forward.Y, Park.Forward.X));
	Player->SetActorLocationAndRotation(Spot, FRotator(0.0f, Yaw, 0.0f),
		false, nullptr, ETeleportType::TeleportPhysics);
	Player->ResetMotion();
	bPlayerParked = true;
	UE_LOG(LogTemp, Display, TEXT("RACEFIELD14E2E: player parked"));
}

FString ATask14Probe::OrderString() const
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

void ATask14Probe::Tick(float Delta)
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
				UE_LOG(LogTemp, Display, TEXT("RACEFIELD14E2E: player acquired"));
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
	if (!bFieldReady && Manager->GetParticipantCount() >= Task14Limits::FieldSize)
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
			bool bTiers = true;
			for (int32 i = 0; i < 5; ++i)
			{
				bTiers = bTiers && (Drivers[i]->GetPaceFactor() == Task14Limits::PaceTiers[i]);
			}
			bPaceOk = bTiers;
			UE_LOG(LogTemp, Display, TEXT("RACEFIELD14E2E: field acquired pace=%d"), bPaceOk);
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
	if (bRacing && !bPlayerParked)
	{
		ParkPlayer();
		GridOrder = OrderString();
		bGridOrderOk = (GridOrder == TEXT("012345"));
		UE_LOG(LogTemp, Display, TEXT("RACEFIELD14E2E: grid order=%s"), *GridOrder);
	}

	// Mid-program reset with grid re-staging, then restart.
	if (!bResetDone && bRacingSeen && Elapsed >= Task14Limits::ResetAt)
	{
		bResetDone = true;
		Player->ResetVehicle();
		Manager->OnVehicleReset();
		ParkPlayer();
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
		bool bZeros = true;
		for (int32 i = 0; i < 6; ++i)
		{
			bZeros = bZeros && (Manager->GetParticipantLaps(i) == 0);
		}
		bResetLaps = bZeros;
		ResetOrder = OrderString();
		bResetOrderOk = (ResetOrder == TEXT("012345"));
		Manager->StartRace();
		bWaitRacing = true;
		bRacingSeen = false;
		bPlayerParked = false;
		for (int32 i = 0; i < 5; ++i)
		{
			LastMoveT[i] = Elapsed;
		}
		UE_LOG(LogTemp, Display, TEXT("RACEFIELD14E2E: reset order=%s laps0=%d"), *ResetOrder, bResetLaps);
	}
	if (bWaitRacing && bRacing)
	{
		bWaitRacing = false;
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
		UE_LOG(LogTemp, Display, TEXT("RACEFIELD14E2E: racing again"));
	}

	// Progress, laps, deadlock monitoring across the AI field.
	if (bRacingSeen && bFieldReady && !bWaitRacing)
	{
		bool bAllProgress = true;
		for (int32 i = 0; i < 5; ++i)
		{
			const float D = Drivers[i]->GetProgressDistance();
			bAllProgress = bAllProgress && (D > Task14Limits::ProgressMinCm);
		}
		if (!bProgressLogged && bAllProgress)
		{
			bProgressLogged = true;
			ProgressTime = Elapsed;
			UE_LOG(LogTemp, Display, TEXT("RACEFIELD14E2E: field progress at t=%.1f"), Elapsed - RacingStartTime);
		}
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
			if (!Manager->IsParticipantFinished(i + 1) && (Elapsed - LastMoveT[i]) > Task14Limits::StallWindow)
			{
				bDeadlockOk = false;
				Finish(false, TEXT("deadlock: AI stalled past window"));
				return;
			}
		}
	}

	// Completion: every AI finished.
	if (bRacingSeen && !bWaitRacing && Manager->GetParticipantCount() == Task14Limits::FieldSize)
	{
		bool bAll = true;
		for (int32 i = 1; i <= 5; ++i)
		{
			bAll = bAll && Manager->IsParticipantFinished(i);
		}
		if (bAll)
		{
			FinalOrder = OrderString();
			for (int32 i = 0; i < 6; ++i)
			{
				FinalLaps[i] = Manager->GetParticipantLaps(i);
			}
			Finish(true, TEXT("field finished"));
			return;
		}
	}

	if (Elapsed > Task14Limits::ProgramTimeout)
	{
		Finish(false, TEXT("program timeout"));
	}
}

void ATask14Probe::Finish(bool bOk, const FString& Note)
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
	UE_LOG(LogTemp, Display, TEXT("RACEFIELD14E2E: finishing (%s)"), *Note);
	UKismetSystemLibrary::QuitGame(GetWorld(), nullptr, EQuitPreference::Quit, false);
}

void ATask14Probe::WriteResults(bool bOk, const FString& Note) const
{
	bool bDistinct = (FinalOrder.Len() == 6);
	for (int32 c = 0; bDistinct && c < 6; ++c)
	{
		for (int32 k = c + 1; k < 6; ++k)
		{
			bDistinct = bDistinct && (FinalOrder[c] != FinalOrder[k]);
		}
	}
	const bool bSpawned = bFieldReady && bGridOrderOk;
	const bool bProgress = bProgressLogged
		&& (ProgressTime - RacingStartTime) < Task14Limits::ProgressWindow;
	bool bLaps = true;
	for (int32 i = 1; i <= 5; ++i)
	{
		bLaps = bLaps && (Manager->GetParticipantLaps(i) >= 1) && Manager->WasParticipantLastValid(i);
	}
	const bool bFinish = bDistinct && (FinalLaps[1] == 2) && (FinalLaps[2] == 2) && (FinalLaps[3] == 2)
		&& (FinalLaps[4] == 2) && (FinalLaps[5] == 2);
	const bool bDeadlock = bDeadlockOk;
	const bool bReset = bResetDone && bResetLaps && bResetOrderOk;
	const bool bAll = bOk && bSpawned && bProgress && bLaps && bFinish && bDeadlock && bReset;

	const FString Json = FString::Printf(
		TEXT("{\"field_spawned\":%s,\"field_progress\":%s,\"field_laps\":%s,\"finish_order\":%s,\"no_deadlock\":%s,\"reset_clears\":%s,")
		TEXT("\"grid_order\":\"%s\",\"final_order\":\"%s\",\"reset_order\":\"%s\",")
		TEXT("\"laps_all\":\"%s\",\"max_gap\":%.2f,\"frames\":%d,\"note\":\"%s\"}"),
		bSpawned ? TEXT("true") : TEXT("false"), bProgress ? TEXT("true") : TEXT("false"),
		bLaps ? TEXT("true") : TEXT("false"), bFinish ? TEXT("true") : TEXT("false"),
		bDeadlock ? TEXT("true") : TEXT("false"), bReset ? TEXT("true") : TEXT("false"),
		*GridOrder, *FinalOrder, *ResetOrder, *LapsStr(), MaxGap, Frames, *Note);

	const FString Dir = FPaths::ProjectSavedDir() + TEXT("Task14E2E/");
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	PF.CreateDirectoryTree(*Dir);
	FFileHelper::SaveStringToFile(Json, *(Dir + TEXT("results.json")));
	UE_LOG(LogTemp, Display, TEXT("RACEFIELD14E2E: spawned=%d progress=%d laps=%d finish=%d deadlock=%d reset=%d all=%d"),
		bSpawned, bProgress, bLaps, bFinish, bDeadlock, bReset, bAll);
}

FString ATask14Probe::LapsStr() const
{
	FString S;
	for (int32 i = 0; i < 6; ++i)
	{
		S.AppendChar(TEXT('0') + FMath::Max(0, FMath::Min(9, FinalLaps[i])));
	}
	return S;
}
