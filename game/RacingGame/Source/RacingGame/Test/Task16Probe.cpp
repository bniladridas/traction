// See header.

#include "Task16Probe.h"
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

ATask16Probe::ATask16Probe()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATask16Probe::BeginPlay()
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


FString ATask16Probe::OrderString() const
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

void ATask16Probe::Tick(float Delta)
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
				UE_LOG(LogTemp, Display, TEXT("RACEFULL16E2E: player acquired"));
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
	if (!bFieldReady && Manager->GetParticipantCount() >= Task16Limits::FieldSize)
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
			UE_LOG(LogTemp, Display, TEXT("RACEFULL16E2E: field acquired"));
		}
	}

	const bool bRacing = static_cast<int32>(Manager->GetPhase()) == 2;
	if (!bStartSent && Elapsed >= 1.0)
	{
		bStartSent = true;
		bConfigured = (Manager->RaceConfig.LapCount == Task16Limits::RaceLaps);
		Manager->StartRace();
		UE_LOG(LogTemp, Display, TEXT("RACEFULL16E2E: start sent configured=%d"), bConfigured);
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
		// Parked clear of every AI line for the whole run: the pace and
		// finish story belongs to the AI field, and a driven sixth car
		// adds traffic chaos without new evidence. Zero input held.
		const FRaceTrackCenterPoint Park = Track->SampleAtDistance(100.0f);
		const FVector Right(-Park.Forward.Y, Park.Forward.X, 0.0f);
		const FVector Spot = Park.Position + Right * 330.0f + FVector(0.0f, 0.0f, 40.0f);
		const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Park.Forward.Y, Park.Forward.X));
		Player->SetActorLocationAndRotation(Spot, FRotator(0.0f, Yaw, 0.0f),
			false, nullptr, ETeleportType::TeleportPhysics);
		Player->ResetMotion();
		bPlayerParked = true;
		UE_LOG(LogTemp, Display, TEXT("RACEFULL16E2E: player parked"));
	}

	// Mid-program reset with grid re-staging, then restart.
	if (!bResetDone && bRacingSeen && Elapsed >= Task16Limits::ResetAt)
	{
		bResetDone = true;
		Player->ResetVehicle();
		Manager->OnVehicleReset();
		bPlayerParked = false;
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
		bPlayerParked = false;
		for (int32 i = 0; i < 5; ++i)
		{
			LastMoveT[i] = Elapsed;
		}
		UE_LOG(LogTemp, Display, TEXT("RACEFULL16E2E: reset cleared=%d"), bResetCleared);
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
		UE_LOG(LogTemp, Display, TEXT("RACEFULL16E2E: racing again"));
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
			if (!Manager->IsParticipantFinished(i + 1) && (Elapsed - LastMoveT[i]) > Task16Limits::StallWindow)
			{
				bDeadlockOk = false;
				Finish(false, TEXT("deadlock: AI stalled past window"));
				return;
			}
		}
	}

	// Completion: every AI finished with 3 laps. The parked player is
	// present but stationary by design and stays out of completion.
	if (bRacingSeen && !bWaitRacing && Manager->GetParticipantCount() == Task16Limits::FieldSize)
	{
		bool bAll = true;
		for (int32 i = 1; i < 6; ++i)
		{
			bAll = bAll && Manager->IsParticipantFinished(i);
		}
		if (bAll)
		{
			bAllFinished = true;
			for (int32 i = 0; i < 6; ++i)
			{
				FinalLaps[i] = Manager->GetParticipantLaps(i);
			}
			FinalOrder = OrderString();
			if (Manager->HasResults())
			{
				const FRaceResults& R = Manager->GetResults();
				bPopulated = (R.Ordered.Num() == 5);
				bConsistent = true;
				for (const FRaceResultEntry& E : R.Ordered)
				{
					bConsistent = bConsistent && (E.CompletedLaps == 3)
						&& (E.BestLapTime > 0.0f) && (E.LastLapTime > 0.0f)
						&& (E.BestLapTime <= E.LastLapTime);
				}
				bool bDistinct = true;
				for (int32 c = 0; c < 6 && bDistinct; ++c)
				{
					for (int32 k = c + 1; k < 6; ++k)
					{
						bDistinct = bDistinct && (Manager->GetPosition(Manager->GetParticipantVehicle(c)) != Manager->GetPosition(Manager->GetParticipantVehicle(k)));
					}
				}
				bPopulated = bPopulated && bDistinct;
				bOrderTotal = bDistinct;
			}
			UE_LOG(LogTemp, Display, TEXT("RACEFULL16E2E: all finished order=%s"), *FinalOrder);
			Finish(true, TEXT("full race complete"));
			return;
		}
	}

	if (Elapsed > Task16Limits::ProgramTimeout)
	{
		Finish(false, TEXT("program timeout"));
	}
}

void ATask16Probe::Finish(bool bOk, const FString& Note)
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
	UE_LOG(LogTemp, Display, TEXT("RACEFULL16E2E: finishing (%s)"), *Note);
	UKismetSystemLibrary::QuitGame(GetWorld(), nullptr, EQuitPreference::Quit, false);
}

void ATask16Probe::WriteResults(bool bOk, const FString& Note) const
{
	const bool bConf = bConfigured;
	const bool bAll = bAllFinished;
	const bool bPop = bPopulated;
	const bool bOrd = bOrderTotal;
	const bool bRes = bResetDone && bResetLaps && bResetCleared;
	const bool bDead = bDeadlockOk;
	const bool bAllOk = bOk && bConf && bAll && bPop && bOrd && bRes && bDead;

	const FString Json = FString::Printf(
		TEXT("{\"configured_laps\":%s,\"all_finished\":%s,\"results_populated\":%s,\"order_total\":%s,\"reset_clears\":%s,\"no_deadlock\":%s,")
		TEXT("\"final_order\":\"%s\",\"frames\":%d,\"note\":\"%s\"}"),
		bConf ? TEXT("true") : TEXT("false"), bAll ? TEXT("true") : TEXT("false"),
		bPop ? TEXT("true") : TEXT("false"), bOrd ? TEXT("true") : TEXT("false"),
		bRes ? TEXT("true") : TEXT("false"), bDead ? TEXT("true") : TEXT("false"),
		*FinalOrder, Frames, *Note);

	const FString Dir = FPaths::ProjectSavedDir() + TEXT("Task16E2E/");
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	PF.CreateDirectoryTree(*Dir);
	FFileHelper::SaveStringToFile(Json, *(Dir + TEXT("results.json")));
	UE_LOG(LogTemp, Display, TEXT("RACEFULL16E2E: configured=%d finished=%d populated=%d order=%d reset=%d deadlock=%d all=%d"),
		bConf, bAll, bPop, bOrd, bRes, bDead, bAllOk);
}
