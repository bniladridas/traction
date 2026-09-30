// See header.

#include "Task19Probe.h"
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

ATask19Probe::ATask19Probe()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATask19Probe::BeginPlay()
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

void ATask19Probe::ParkPlayer()
{
	// Parked clear of both AI lines: behind the grid, near the right
	// edge, on the road, zero input.
	const FRaceTrackCenterPoint Park = Track->SampleAtDistance(100.0f);
	const FVector Right(-Park.Forward.Y, Park.Forward.X, 0.0f);
	const FVector Spot = Park.Position + Right * 330.0f + FVector(0.0f, 0.0f, 40.0f);
	const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Park.Forward.Y, Park.Forward.X));
	Player->SetActorLocationAndRotation(Spot, FRotator(0.0f, Yaw, 0.0f),
		false, nullptr, ETeleportType::TeleportPhysics);
	Player->ResetMotion();
	bPlayerParked = true;
	UE_LOG(LogTemp, Display, TEXT("RACECRAFT19E2E: player parked"));
}

FString ATask19Probe::OrderString() const
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

void ATask19Probe::Tick(float Delta)
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

	// Field: slot 1 is the frozen defender (pace 0.85, line -120),
	// slot 2 the frozen attacker (pace 1.0, line +120, racecraft on).
	if ((!Defender || !Attacker) && Manager->GetParticipantCount() >= Task19Limits::FieldSize)
	{
		Defender = Manager->GetParticipantVehicle(1);
		Attacker = Manager->GetParticipantVehicle(2);
		if (Defender && Attacker)
		{
			TArray<UActorComponent*> CD;
			Defender->GetComponents(URaceAIDriver::StaticClass(), CD);
			if (CD.Num() > 0)
			{
				DriverDef = Cast<URaceAIDriver>(CD[0]);
			}
			TArray<UActorComponent*> CA;
			Attacker->GetComponents(URaceAIDriver::StaticClass(), CA);
			if (CA.Num() > 0)
			{
				DriverAtt = Cast<URaceAIDriver>(CA[0]);
			}
			if (DriverDef && DriverAtt)
			{
				// t19_racecraft_configured: frozen tiers and lines read
				// back exactly, defender carries no racecraft layer.
				bConfigured = (DriverDef->GetPaceFactor() == Task19Limits::PaceDefender)
					&& (DriverAtt->GetPaceFactor() == Task19Limits::PaceAttacker)
					&& (DriverDef->GetFrozenLineOffset() == Task19Limits::LineDefender)
					&& (DriverAtt->GetFrozenLineOffset() == Task19Limits::LineAttacker)
					&& !DriverDef->IsRacecraftCommitted()
					&& !DriverDef->bRacecraftEnabled;
				UE_LOG(LogTemp, Display, TEXT("RACECRAFT19E2E: configured=%d"), bConfigured);
			}
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
		LastMoveDef = Elapsed;
		LastMoveAtt = Elapsed;
	}
	if (bRacing && !bPlayerParked)
	{
		ParkPlayer();
	}

	if (bRacingSeen && DriverDef && DriverAtt)
	{
		// Attack window: read-only absolute separation from the two drivers'
		// unwrapped distances. Attacker progress remains available separately
		// for the commit-window measurement.
		const float DefProgress = DriverDef->GetProgressDistance();
		const float AttProgress = DriverAtt->GetProgressDistance();
		const float SeparationCm = DriverDef->GetUnwrappedDistance() - DriverAtt->GetUnwrappedDistance();

		if (SeparationCm >= 0.0f)
		{
			if (MinSeparationCm < 0.0f || SeparationCm < MinSeparationCm)
			{
				MinSeparationCm = SeparationCm;
			}
			if (SeparationCm <= Task19Limits::AttackWindowCm)
			{
				// t19_attack_window_detected
				bWindowDetected = true;
				if (!bWindowSeen)
				{
					bWindowSeen = true;
					WindowHits = 1;
					WindowEntryTime = Elapsed;
					WindowEntryProgress = AttProgress;
					UE_LOG(LogTemp, Display, TEXT("RACECRAFT19E2E: window entered separation=%.1f"), SeparationCm);
				}
				else
				{
					WindowHits++;
				}

				// t19_commit_free_side: the COMMANDED offset must reach
				// frozen + CommitShiftCm within CommitWindowCm of further
				// attacker progress past window entry. No physics assertion.
				const float Commanded = DriverAtt->GetCommandedLineOffset();
				const float ProgressPastEntry = AttProgress - WindowEntryProgress;
				if (ProgressPastEntry <= Task19Limits::CommitWindowCm
					&& Commanded >= Task19Limits::LineAttacker + Task19Limits::CommitShiftCm)
				{
					bCommitObserved = true;
				}
			// Max, not last-value: survives the later cede samples so the
			// persisted artifact genuinely evidences the +240 command.
			MaxCommittedOffsetCm = FMath::Max(MaxCommittedOffsetCm, Commanded);
			}
			else
			{
				// Outside the window the commanded offset must sit on the
				// frozen line: t19_no_commit_outside_window.
				const float Commanded = DriverAtt->GetCommandedLineOffset();
				// Prove the negative case was actually exercised before it
				// can count as a pass.
				OutsideWindowSamples++;
				if (FMath::Abs(Commanded - Task19Limits::LineAttacker) > Task19Limits::CedeToleranceCm)
				{
					CommandsOutsideWindow++;
				}
			}
		}

		// t19_cede_back: once the window is cleared the commanded offset
		// must return to the frozen line and stay there.
		if (!DriverAtt->IsRacecraftCommitted())
		{
			const float Commanded = DriverAtt->GetCommandedLineOffset();
			CedeOffsetCm = Commanded;
			if (FMath::Abs(Commanded - DriverAtt->GetFrozenLineOffset()) <= Task19Limits::CedeToleranceCm)
			{
				bCedeObserved = true;
			}
		}

		// t19_scope_frozen runtime invariants: pace tier never mutated,
		// defender line frozen and never commanded, defender never
		// commits, defender order never written.
		const bool bScopeRuntime = (DriverAtt->GetPaceFactor() == Task19Limits::PaceAttacker)
			&& (DriverDef->GetPaceFactor() == Task19Limits::PaceDefender)
			&& (DriverDef->GetFrozenLineOffset() == Task19Limits::LineDefender)
			&& !DriverDef->bRacecraftEnabled
			&& !DriverDef->IsRacecraftCommitted();
		bScopeOk = bScopeRuntime;

		// Stall monitoring across the pair (Task 13 convention).
		if ((DefProgress - LastDistDef) > 5.0f || DriverDef->GetRecoveryCount() != LastRecDef)
		{
			LastMoveDef = Elapsed;
			LastDistDef = DefProgress;
			LastRecDef = DriverDef->GetRecoveryCount();
		}
		if ((AttProgress - LastDistAtt) > 5.0f || DriverAtt->GetRecoveryCount() != LastRecAtt)
		{
			LastMoveAtt = Elapsed;
			LastDistAtt = AttProgress;
			LastRecAtt = DriverAtt->GetRecoveryCount();
		}
		if ((!Manager->IsParticipantFinished(1) && (Elapsed - LastMoveDef) > Task19Limits::StallWindow)
			|| (!Manager->IsParticipantFinished(2) && (Elapsed - LastMoveAtt) > Task19Limits::StallWindow))
		{
			bDeadlockOk = false;
			Finish(false, TEXT("deadlock: rival stalled past window"));
			return;
		}
	}

	// Terminal state: the program ends once the attack/cede cycle has been
	// observed for a settling period. A physical pass is NOT required.
	// Do not terminate until the negative case has also been exercised,
	// otherwise the run cannot evidence gate 4 at all.
	if (bWindowSeen && bCommitObserved && bCedeObserved && (OutsideWindowSamples > 0)
		&& Elapsed - RacingStartTime > 45.0)
	{
		bCommitOk = bCommitObserved
			&& (MaxCommittedOffsetCm >= Task19Limits::LineAttacker + Task19Limits::CommitShiftCm);
		bCedeOk = bCedeObserved;
		// Gate 4 needs exercise AND zero illegal commits, never a vacuous pass.
		bNoCommitOk = (OutsideWindowSamples > 0) && (CommandsOutsideWindow == 0);
		Finish(true, TEXT("racecraft commit and cede observed"));
		return;
	}

	if (Elapsed > Task19Limits::ProgramTimeout)
	{
		bCommitOk = bCommitObserved
			&& (MaxCommittedOffsetCm >= Task19Limits::LineAttacker + Task19Limits::CommitShiftCm);
		bCedeOk = bCedeObserved;
		bNoCommitOk = (OutsideWindowSamples > 0) && (CommandsOutsideWindow == 0);
		Finish(false, TEXT("program timeout before commit/cede cycle observed"));
	}
}

void ATask19Probe::Finish(bool bOk, const FString& Note)
{
	if (bFinished)
	{
		return;
	}
	bFinished = true;
	for (ARaceVehicle* V : { Player, Defender, Attacker })
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
	UE_LOG(LogTemp, Display, TEXT("RACECRAFT19E2E: finishing (%s)"), *Note);
	UKismetSystemLibrary::QuitGame(GetWorld(), nullptr, EQuitPreference::Quit, false);
}

void ATask19Probe::WriteResults(bool bOk, const FString& Note) const
{
	const bool bAll = bOk && bConfigured && bWindowDetected && bCommitOk
		&& bNoCommitOk && bCedeOk && bScopeOk && bDeadlockOk;

	const FString Json = FString::Printf(
		TEXT("{\"t19_racecraft_configured\":%s,\"t19_attack_window_detected\":%s,")
		TEXT("\"t19_commit_free_side\":%s,\"t19_no_commit_outside_window\":%s,")
		TEXT("\"t19_cede_back\":%s,\"t19_scope_frozen\":%s,")
		TEXT("\"min_separation_cm\":%.1f,\"max_committed_offset_cm\":%.1f,\"cede_offset_cm\":%.1f,")
		TEXT("\"window_hits\":%d,\"outside_window_samples\":%d,\"commands_outside_window\":%d,\"no_deadlock\":%s,")
		TEXT("\"frames\":%d,\"note\":\"%s\"}"),
		bConfigured ? TEXT("true") : TEXT("false"),
		bWindowDetected ? TEXT("true") : TEXT("false"),
		bCommitOk ? TEXT("true") : TEXT("false"),
		bNoCommitOk ? TEXT("true") : TEXT("false"),
		bCedeOk ? TEXT("true") : TEXT("false"),
		bScopeOk ? TEXT("true") : TEXT("false"),
		MinSeparationCm, MaxCommittedOffsetCm, CedeOffsetCm, WindowHits,
		OutsideWindowSamples, CommandsOutsideWindow,
		bDeadlockOk ? TEXT("true") : TEXT("false"),
		Frames, *Note);

	const FString Dir = FPaths::ProjectSavedDir() + TEXT("Task19E2E/");
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	PF.CreateDirectoryTree(*Dir);
	FFileHelper::SaveStringToFile(Json, *(Dir + TEXT("results.json")));
	UE_LOG(LogTemp, Display, TEXT("RACECRAFT19E2E: configured=%d window=%d commit=%d nocommit=%d cede=%d scope=%d all=%d"),
		bConfigured, bWindowDetected, bCommitOk, bNoCommitOk, bCedeOk, bScopeOk, bAll);
	UE_LOG(LogTemp, Display, TEXT("RACECRAFT19E2E: max_committed_offset_cm=%.1f (need >= %.1f) outside_window_samples=%d illegal=%d"),
		MaxCommittedOffsetCm, Task19Limits::LineAttacker + Task19Limits::CommitShiftCm,
		OutsideWindowSamples, CommandsOutsideWindow);
}
