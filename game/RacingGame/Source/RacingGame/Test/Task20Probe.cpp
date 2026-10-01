// See header.

#include "Task20Probe.h"
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

ATask20Probe::ATask20Probe()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATask20Probe::BeginPlay()
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

void ATask20Probe::ParkPlayer()
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
	UE_LOG(LogTemp, Display, TEXT("TASK20E2E: player parked"));
}

void ATask20Probe::Tick(float Delta)
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

	// Field: slot 1 is the frozen defender (pace 0.85, line -120, defense
	// layer on), slot 2 the frozen attacker (pace 1.0, line +120, Task 19
	// attack layer on, unchanged).
	if ((!Defender || !Attacker) && Manager->GetParticipantCount() >= Task20Limits::FieldSize)
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
				// t20_racecraft_configured: frozen tiers, lines, and layer
				// assignments read back exactly. The attacker keeps its
				// frozen Task 19 configuration; the defender carries the
				// frozen Task 20 configuration and no attack layer.
				bConfigured = (DriverDef->GetPaceFactor() == Task20Limits::PaceDefender)
					&& (DriverAtt->GetPaceFactor() == Task20Limits::PaceAttacker)
					&& (DriverDef->GetFrozenLineOffset() == Task20Limits::LineDefender)
					&& (DriverAtt->GetFrozenLineOffset() == Task20Limits::LineAttacker)
					&& DriverDef->bDefenseEnabled
					&& (DriverDef->DefenseRearWindowCm == Task20Limits::RearWindowCm)
					&& (DriverDef->DefenseShiftCm == Task20Limits::DefensiveShiftCm)
					&& (DriverDef->DefenseDirectionSign == Task20Limits::DefenseDirectionSign)
					&& !DriverDef->bRacecraftEnabled
					&& DriverAtt->bRacecraftEnabled
					&& !DriverAtt->bDefenseEnabled;
				UE_LOG(LogTemp, Display, TEXT("TASK20E2E: configured=%d"), bConfigured);
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
		// Rear window: read-only absolute separation from the two drivers'
		// unwrapped distances. Progress stays available separately for
		// the defense-window measurement and stall monitoring.
		const float DefProgress = DriverDef->GetProgressDistance();
		const float AttProgress = DriverAtt->GetProgressDistance();
		const float SeparationCm = DriverDef->GetUnwrappedDistance() - DriverAtt->GetUnwrappedDistance();
		const float DefCommanded = DriverDef->GetCommandedLineOffset();
		const float AttCommanded = DriverAtt->GetCommandedLineOffset();

		// Gate 5 second half: the centerline bound holds over EVERY racing
		// tick, inside and outside the window alike.
		MaxDefenderCommandedCm = FMath::Max(MaxDefenderCommandedCm, DefCommanded);

		// Gate 6 invariant population: simultaneous-commit samples only,
		// where the attacker commands exactly its staged commit target
		// (frozen line + staged shift) AND the defender commands exactly
		// its staged defensive target. Never computed over single-sided
		// frames. Targets derive from each driver's staged configuration,
		// not from duplicated constants.
		const float AttCommitTarget = DriverAtt->GetFrozenLineOffset() + DriverAtt->RacecraftCommitShiftCm;
		const float DefCommitTarget = DriverDef->GetFrozenLineOffset() + DriverDef->DefenseShiftCm;
		if (AttCommanded == AttCommitTarget && DefCommanded == DefCommitTarget)
		{
			bSimulObserved = true;
			const float CmdSep = AttCommanded - DefCommanded;
			if (MinCommandedSeparationCm < 0.0f || CmdSep < MinCommandedSeparationCm)
			{
				MinCommandedSeparationCm = CmdSep;
			}
		}

		if (SeparationCm > 0.0f)
		{
			if (MinSeparationCm < 0.0f || SeparationCm < MinSeparationCm)
			{
				MinSeparationCm = SeparationCm;
			}
			if (SeparationCm <= Task20Limits::RearWindowCm)
			{
				// t20_defense_window_detected
				bWindowDetected = true;
				if (!bWindowSeen)
				{
					bWindowSeen = true;
					WindowHits = 1;
					WindowEntryTime = Elapsed;
					WindowEntryProgress = DefProgress;
					UE_LOG(LogTemp, Display, TEXT("TASK20E2E: window entered separation=%.1f"), SeparationCm);
				}
				else
				{
					WindowHits++;
				}

				// t20_defensive_line_committed: the COMMANDED defensive
				// offset must sit on the defensive target within
				// DefenseWindowCm of further defender progress past window
				// entry. No physics assertion.
				const float ProgressPastEntry = DefProgress - WindowEntryProgress;
				if (ProgressPastEntry <= Task20Limits::DefenseWindowCm
					&& FMath::Abs(DefCommanded - (Task20Limits::LineDefender + Task20Limits::DefensiveShiftCm)) <= Task20Limits::CedeToleranceCm)
				{
					bDefenseObserved = true;
					DefensiveOffsetCm = DefCommanded;
				}
			}
			else
			{
				// Outside the window the commanded offset must sit on the
				// frozen line: t20_no_defense_outside_window. A sample is
				// evaluable only when BOTH the probe measurement and the
				// defender layer's own view agree the rival is outside;
				// quantization-dither frames are skipped entirely.
				const float DefViewGap = DriverDef->GetRacecraftRivalGapCm();
				if (DefViewGap > Task20Limits::RearWindowCm)
				{
					// Prove the negative case was actually exercised before
					// it can count as a pass.
					OutsideWindowSamples++;
					if (FMath::Abs(DefCommanded - Task20Limits::LineDefender) > Task20Limits::CedeToleranceCm)
					{
						CommandsOutsideWindow++;
						// Evidence: both views on any illegal command.
						UE_LOG(LogTemp, Display, TEXT("TASK20E2E: ILLEGAL outside-window command sep=%.2f defGap=%.2f commanded=%.2f frame=%d elapsed=%.2f"),
							SeparationCm, DefViewGap, DefCommanded, Frames, Elapsed);
					}
				}
			}
		}

		// t20_defense_holds_centerline (cede half): once the window is
		// cleared the commanded offset must return to the frozen line and
		// stay there.
		if (!DriverDef->IsRacecraftCommitted())
		{
			const float Commanded = DriverDef->GetCommandedLineOffset();
			CedeOffsetCm = Commanded;
			if (FMath::Abs(Commanded - DriverDef->GetFrozenLineOffset()) <= Task20Limits::CedeToleranceCm)
			{
				bCedeObserved = true;
			}
		}

		// t20_scope_frozen runtime invariants: pace tiers never mutated,
		// attacker Task 19 assignment preserved, defender order never
		// written.
		const bool bScopeRuntime = (DriverAtt->GetPaceFactor() == Task20Limits::PaceAttacker)
			&& (DriverDef->GetPaceFactor() == Task20Limits::PaceDefender)
			&& (DriverAtt->GetFrozenLineOffset() == Task20Limits::LineAttacker)
			&& DriverAtt->bRacecraftEnabled
			&& !DriverAtt->bDefenseEnabled;
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
		if ((!Manager->IsParticipantFinished(1) && (Elapsed - LastMoveDef) > Task20Limits::StallWindow)
			|| (!Manager->IsParticipantFinished(2) && (Elapsed - LastMoveAtt) > Task20Limits::StallWindow))
		{
			bDeadlockOk = false;
			Finish(false, TEXT("deadlock: rival stalled past window"));
			return;
		}
	}

	// Terminal state: the program ends once the defend/cede cycle and the
	// exercised negative case have been observed for a settling period,
	// with simultaneous-commit evidence for the separation invariant. A
	// physical block is NOT required.
	if (bWindowSeen && bDefenseObserved && bCedeObserved && (OutsideWindowSamples > 0) && bSimulObserved
		&& Elapsed - RacingStartTime > 45.0)
	{
		bDefenseOk = bDefenseObserved
			&& (DefensiveOffsetCm >= Task20Limits::LineDefender + Task20Limits::DefensiveShiftCm - Task20Limits::CedeToleranceCm)
			&& (DefensiveOffsetCm <= Task20Limits::LineDefender + Task20Limits::DefensiveShiftCm + Task20Limits::CedeToleranceCm);
		bCedeOk = bCedeObserved && (MaxDefenderCommandedCm <= 0.0f);
		// Gate 4 needs exercise AND zero illegal commands, never a vacuous pass.
		bNoDefenseOk = (OutsideWindowSamples > 0) && (CommandsOutsideWindow == 0);
		// Gate 6: runtime invariants plus the separation floor over
		// simultaneous-commit samples. The floor derives from the staged
		// commit targets (attacker +240, defender 0).
		const float SepFloor = (DriverAtt->GetFrozenLineOffset() + DriverAtt->RacecraftCommitShiftCm)
			- (DriverDef->GetFrozenLineOffset() + DriverDef->DefenseShiftCm);
		bScopeOk = bScopeOk && bSimulObserved && (MinCommandedSeparationCm >= SepFloor);
		Finish(true, TEXT("defense and cede observed"));
		return;
	}

	if (Elapsed > Task20Limits::ProgramTimeout)
	{
		bDefenseOk = bDefenseObserved
			&& (DefensiveOffsetCm >= Task20Limits::LineDefender + Task20Limits::DefensiveShiftCm - Task20Limits::CedeToleranceCm)
			&& (DefensiveOffsetCm <= Task20Limits::LineDefender + Task20Limits::DefensiveShiftCm + Task20Limits::CedeToleranceCm);
		bCedeOk = bCedeObserved && (MaxDefenderCommandedCm <= 0.0f);
		bNoDefenseOk = (OutsideWindowSamples > 0) && (CommandsOutsideWindow == 0);
		const float SepFloor = (DriverAtt->GetFrozenLineOffset() + DriverAtt->RacecraftCommitShiftCm)
			- (DriverDef->GetFrozenLineOffset() + DriverDef->DefenseShiftCm);
		bScopeOk = bScopeOk && bSimulObserved && (MinCommandedSeparationCm >= SepFloor);
		Finish(false, TEXT("program timeout before defense/cede cycle observed"));
	}
}

void ATask20Probe::Finish(bool bOk, const FString& Note)
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
	UE_LOG(LogTemp, Display, TEXT("TASK20E2E: finishing (%s)"), *Note);
	UKismetSystemLibrary::QuitGame(GetWorld(), nullptr, EQuitPreference::Quit, false);
}

void ATask20Probe::WriteResults(bool bOk, const FString& Note) const
{
	const bool bAll = bOk && bConfigured && bWindowDetected && bDefenseOk
		&& bNoDefenseOk && bCedeOk && bScopeOk && bDeadlockOk;

	const FString Json = FString::Printf(
		TEXT("{\"t20_racecraft_configured\":%s,\"t20_defense_window_detected\":%s,")
		TEXT("\"t20_defensive_line_committed\":%s,\"t20_no_defense_outside_window\":%s,")
		TEXT("\"t20_defense_holds_centerline\":%s,\"t20_scope_frozen\":%s,")
		TEXT("\"min_separation_cm\":%.1f,\"defensive_offset_cm\":%.1f,\"cede_offset_cm\":%.1f,")
		TEXT("\"max_defender_commanded_cm\":%.1f,\"window_hits\":%d,\"outside_window_samples\":%d,")
		TEXT("\"commands_outside_window\":%d,\"min_commanded_separation_cm\":%.1f,\"no_deadlock\":%s,")
		TEXT("\"frames\":%d,\"note\":\"%s\"}"),
		bConfigured ? TEXT("true") : TEXT("false"),
		bWindowDetected ? TEXT("true") : TEXT("false"),
		bDefenseOk ? TEXT("true") : TEXT("false"),
		bNoDefenseOk ? TEXT("true") : TEXT("false"),
		bCedeOk ? TEXT("true") : TEXT("false"),
		bScopeOk ? TEXT("true") : TEXT("false"),
		MinSeparationCm, DefensiveOffsetCm, CedeOffsetCm, MaxDefenderCommandedCm,
		WindowHits, OutsideWindowSamples, CommandsOutsideWindow,
		MinCommandedSeparationCm,
		bDeadlockOk ? TEXT("true") : TEXT("false"),
		Frames, *Note);

	const FString Dir = FPaths::ProjectSavedDir() + TEXT("Task20E2E/");
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	PF.CreateDirectoryTree(*Dir);
	FFileHelper::SaveStringToFile(Json, *(Dir + TEXT("results.json")));
	UE_LOG(LogTemp, Display, TEXT("TASK20E2E: configured=%d window=%d defense=%d nodefense=%d cede=%d scope=%d all=%d"),
		bConfigured, bWindowDetected, bDefenseOk, bNoDefenseOk, bCedeOk, bScopeOk, bAll);
	UE_LOG(LogTemp, Display, TEXT("TASK20E2E: defensive_offset_cm=%.1f max_defender_commanded_cm=%.1f (bound 0.0) outside_window_samples=%d illegal=%d simul_sep=%.1f"),
		DefensiveOffsetCm, MaxDefenderCommandedCm, OutsideWindowSamples, CommandsOutsideWindow, MinCommandedSeparationCm);
}
