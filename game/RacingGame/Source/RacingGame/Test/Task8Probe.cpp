// See header.

#include "Task8Probe.h"
#include "RaceVehicle.h"
#include "RaceTrack.h"
#include "RaceManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

ATask8Probe::ATask8Probe()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATask8Probe::BeginPlay()
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

void ATask8Probe::StagePose(int32 CpIndex, float Along, FVector& Loc, FRotator& Rot) const
{
	const TArray<FRaceTrackCheckpoint>& CPs = Track->GetCheckpoints();
	const FRaceTrackCheckpoint& CP = CPs[CpIndex];
	Loc = CP.Position + CP.Forward * Along;
	Loc.Z = 40.0f;
	Rot = FRotator(0.0f, FMath::RadiansToDegrees(FMath::Atan2(CP.Forward.Y, CP.Forward.X)), 0.0f);
}

void ATask8Probe::AppendCrossing(int32 CpIndex)
{
	FVector Loc;
	FRotator Rot;
	StagePose(CpIndex, -150.0f, Loc, Rot);
	Steps.Add({ Loc, Rot, TagNone });
	StagePose(CpIndex, 150.0f, Loc, Rot);
	Steps.Add({ Loc, Rot, TagNone });
}

void ATask8Probe::AppendSequence(bool bClean)
{
	const int32 N = Track->GetCheckpoints().Num();
	for (int32 k = 0; k < N; ++k)
	{
		if (!bClean && k == 3)
		{
			// Deliberate wrong-order: cross 4 while 3 is expected.
			AppendCrossing(4);
		}
		AppendCrossing(k);
		if (!bClean && k == 2)
		{
			// Deliberate double: cross 2 again after it counted.
			AppendCrossing(2);
		}
	}
}

void ATask8Probe::Tick(float Delta)
{
	Super::Tick(Delta);
	if (bFinished || !Track || !Manager)
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
				Vehicle->ApplyThrottle(0.0f);
				Vehicle->ApplyBrake(0.0f);
				Vehicle->ApplySteering(0.0f);
				UE_LOG(LogTemp, Display, TEXT("RACE8E2E: pawn acquired"));
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

	// Phase observations run every tick regardless of script state.
	const int32 PhaseInt = static_cast<int32>(Manager->GetPhase());
	if (PhaseInt == 0)
	{
		bSawReady = true;
	}
	if (PhaseInt == 1 && !bSawCountdown)
	{
		bSawCountdown = true;
		SawCountdownTime = Elapsed;
	}
	if (PhaseInt == 2 && !bSawRacing)
	{
		bSawRacing = true;
		SawRacingTime = Elapsed;
	}

	if (!bStartRaceSent && Elapsed >= 1.0)
	{
		bStartRaceSent = true;
		StartRaceTime = Elapsed;
		Manager->StartRace();
		UE_LOG(LogTemp, Display, TEXT("RACE8E2E: start sent"));
	}

	// Build the script once Racing begins.
	if (bSawRacing && !bStepping && Steps.Num() == 0)
	{
		bStepping = true;
		AppendSequence(false);
		FVector Loc;
		FRotator Rot;
		StagePose(0, -150.0f, Loc, Rot);
		Steps.Add({ Loc, Rot, TagAssertSeqA });
		StagePose(0, -150.0f, Loc, Rot);
		Steps.Add({ Loc, Rot, TagDoReset });
		StagePose(0, -150.0f, Loc, Rot);
		Steps.Add({ Loc, Rot, TagAssertReset });
		AppendSequence(true);
		StagePose(0, -150.0f, Loc, Rot);
		Steps.Add({ Loc, Rot, TagAssertSeqB });
		AppendSequence(true);
		StagePose(0, -150.0f, Loc, Rot);
		Steps.Add({ Loc, Rot, TagAssertSeqC });
		AppendCrossing(0);
		AppendCrossing(1);
		AppendCrossing(2);
		StagePose(0, -150.0f, Loc, Rot);
		Steps.Add({ Loc, Rot, TagAssertExtras });
		StagePose(0, -150.0f, Loc, Rot);
		Steps.Add({ Loc, Rot, TagFinish });
		NextStepTime = Elapsed;
		UE_LOG(LogTemp, Display, TEXT("RACE8E2E: script built, %d steps"), Steps.Num());
	}

	if (bStepping && StepIdx < Steps.Num() && Elapsed >= NextStepTime)
	{
		const FStep& S = Steps[StepIdx];
		// Tag steps are assertion markers only: evaluate in place without
		// teleporting, so measurements see the true post-action state.
		if (S.Tag != TagNone)
		{
			switch (S.Tag)
			{
			case TagAssertSeqA:
				LapsAfterA = Manager->GetCompletedLaps();
				bInvalidAfterA = !Manager->WasLastSequenceValid();
				bSeqAOrder = (Manager->GetCrossingCount() == 8);
				bSeqAWrongIgnored = (Manager->GetIgnoredCount() >= 2);
				UE_LOG(LogTemp, Display, TEXT("RACE8E2E: seqA laps=%d invalid=%d crossings=%d ignored=%d"),
					LapsAfterA, bInvalidAfterA, Manager->GetCrossingCount(), Manager->GetIgnoredCount());
				break;
			case TagDoReset:
				Vehicle->ResetVehicle();
				Manager->OnVehicleReset();
				UE_LOG(LogTemp, Display, TEXT("RACE8E2E: reset sent"));
				break;
			case TagAssertReset:
				bResetPhase = (static_cast<int32>(Manager->GetPhase()) == 0);
				bResetLaps = (Manager->GetCompletedLaps() == 0);
				bResetNext = (Manager->GetNextCheckpoint() == 0);
				ResetPosErr = FVector::Dist(Vehicle->GetActorLocation(), Track->GetStartPosition());
				Manager->StartRace();
				bWaitRacing = true;
				bStepping = false;
				UE_LOG(LogTemp, Display, TEXT("RACE8E2E: reset phase=%d laps=%d next=%d poserr=%.1f; start sent, waiting racing"),
					bResetPhase, Manager->GetCompletedLaps(), Manager->GetNextCheckpoint(), ResetPosErr);
				break;
			case TagAssertSeqB:
				LapsAfterB = Manager->GetCompletedLaps();
				bValidAfterB = Manager->WasLastSequenceValid();
				UE_LOG(LogTemp, Display, TEXT("RACE8E2E: seqB laps=%d valid=%d"), LapsAfterB, bValidAfterB);
				break;
			case TagAssertSeqC:
				LapsAfterC = Manager->GetCompletedLaps();
				bFinishedAfterC = (static_cast<int32>(Manager->GetPhase()) == 3);
				LastLap = Manager->GetLastLapTime();
				BestLap = Manager->GetBestLapTime();
				UE_LOG(LogTemp, Display, TEXT("RACE8E2E: seqC laps=%d finished=%d last=%.2f best=%.2f"),
					LapsAfterC, bFinishedAfterC, LastLap, BestLap);
				break;
			case TagAssertExtras:
				LapsAfterExtras = Manager->GetCompletedLaps();
				UE_LOG(LogTemp, Display, TEXT("RACE8E2E: extras laps=%d"), LapsAfterExtras);
				break;
			case TagFinish:
				Finish(true, TEXT("program complete"));
				return;
			default:
				break;
			}
			StepIdx++;
			NextStepTime = Elapsed + Task8Limits::StepDwell;
			return;
		}
		Vehicle->SetActorLocationAndRotation(S.Loc, S.Rot, false, nullptr, ETeleportType::TeleportPhysics);
		StepIdx++;
		NextStepTime = Elapsed + Task8Limits::StepDwell;
	}

	// Resume stepping once the restart reaches Racing.
	if (bWaitRacing && PhaseInt == 2)
	{
		bWaitRacing = false;
		bStepping = true;
		NextStepTime = Elapsed + 0.5;
		UE_LOG(LogTemp, Display, TEXT("RACE8E2E: racing again, resuming"));
	}

	if (Elapsed > Task8Limits::ProgramTimeout)
	{
		Finish(false, TEXT("program timeout"));
	}
}

void ATask8Probe::Finish(bool bOk, const FString& Note)
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
	UE_LOG(LogTemp, Display, TEXT("RACE8E2E: finishing (%s)"), *Note);
	UKismetSystemLibrary::QuitGame(GetWorld(), nullptr, EQuitPreference::Quit, false);
}

void ATask8Probe::WriteResults(bool bOk, const FString& Note) const
{
	const bool bReady = bSawReady;
	const bool bCountdown = bSawCountdown && (SawCountdownTime - StartRaceTime) < Task8Limits::CountdownWindow;
	const bool bRacing = bSawRacing && (SawRacingTime - StartRaceTime) < Task8Limits::RacingWindow;
	const bool bOrder = bSeqAOrder;
	const bool bWrong = bSeqAWrongIgnored && LapsAfterA == 0 && bInvalidAfterA;
	const bool bSingle = (LapsAfterB == 1) && bValidAfterB;
	const bool bNoDouble = (LapsAfterA == 0);
	const bool bReset = bResetPhase && bResetLaps && bResetNext && ResetPosErr >= 0.0f && ResetPosErr < Task8Limits::ResetPosMaxCm;
	const bool bFinish = bFinishedAfterC && (LapsAfterC == 2) && LastLap > 0.0f && BestLap > 0.0f;
	const bool bLocked = (LapsAfterExtras == 2) && bFinishedAfterC;
	const bool bAll = bOk && bReady && bCountdown && bRacing && bOrder && bWrong && bSingle && bNoDouble && bReset && bFinish && bLocked;

	const FString Json = FString::Printf(
		TEXT("{\"starts_ready\":%s,\"countdown_ok\":%s,\"racing_begins\":%s,\"checkpoint_order\":%s,\"wrong_order_ignored\":%s,")
		TEXT("\"single_increment\":%s,\"no_double_count\":%s,\"reset_behavior\":%s,\"finish_transition\":%s,\"finished_locked\":%s,")
		TEXT("\"laps_after_a\":%d,\"laps_after_b\":%d,\"laps_after_c\":%d,\"laps_after_extras\":%d,")
		TEXT("\"last_lap_time\":%.2f,\"best_lap_time\":%.2f,\"reset_pos_err_cm\":%.1f,")
		TEXT("\"countdown_delay\":%.2f,\"racing_delay\":%.2f,\"frames\":%d,\"note\":\"%s\"}"),
		bReady ? TEXT("true") : TEXT("false"), bCountdown ? TEXT("true") : TEXT("false"),
		bRacing ? TEXT("true") : TEXT("false"), bOrder ? TEXT("true") : TEXT("false"),
		bWrong ? TEXT("true") : TEXT("false"), bSingle ? TEXT("true") : TEXT("false"),
		bNoDouble ? TEXT("true") : TEXT("false"), bReset ? TEXT("true") : TEXT("false"),
		bFinish ? TEXT("true") : TEXT("false"), bLocked ? TEXT("true") : TEXT("false"),
		LapsAfterA, LapsAfterB, LapsAfterC, LapsAfterExtras, LastLap, BestLap, ResetPosErr,
		SawCountdownTime - StartRaceTime, SawRacingTime - StartRaceTime, Frames, *Note);

	const FString Dir = FPaths::ProjectSavedDir() + TEXT("Task8E2E/");
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	PF.CreateDirectoryTree(*Dir);
	FFileHelper::SaveStringToFile(Json, *(Dir + TEXT("results.json")));
	UE_LOG(LogTemp, Display, TEXT("RACE8E2E: ready=%d count=%d racing=%d order=%d wrong=%d single=%d nodouble=%d reset=%d finish=%d locked=%d all=%d"),
		bReady, bCountdown, bRacing, bOrder, bWrong, bSingle, bNoDouble, bReset, bFinish, bLocked, bAll);
}
