// See header.

#include "Task11Probe.h"
#include "RaceVehicle.h"
#include "RaceTrack.h"
#include "RaceManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

ATask11Probe::ATask11Probe()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATask11Probe::BeginPlay()
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

void ATask11Probe::StagePose(ARaceVehicle* Who, int32 CpIndex, float Along)
{
	(void)Who;
	const TArray<FRaceTrackCheckpoint>& CPs = Track->GetCheckpoints();
	const FRaceTrackCheckpoint& CP = CPs[CpIndex];
	const FVector Loc = CP.Position + CP.Forward * Along + FVector(0.0f, 0.0f, 40.0f);
	const FRotator Rot(0.0f, FMath::RadiansToDegrees(FMath::Atan2(CP.Forward.Y, CP.Forward.X)), 0.0f);
	FStep S;
	S.Who = Who;
	S.Loc = Loc;
	S.Rot = Rot;
	S.Tag = TagNone;
	Steps.Add(S);
}

void ATask11Probe::AppendCrossing(ARaceVehicle* Who, int32 CpIndex)
{
	StagePose(Who, CpIndex, -150.0f);
	StagePose(Who, CpIndex, 150.0f);
}

void ATask11Probe::AppendSequence(ARaceVehicle* Who)
{
	const int32 N = Track->GetCheckpoints().Num();
	for (int32 k = 0; k < N; ++k)
	{
		AppendCrossing(Who, k);
	}
}

FString ATask11Probe::OrderString() const
{
	auto Digit = [](int32 Pos) -> TCHAR
	{
		if (Pos == 1)
		{
			return TEXT('0');
		}
		if (Pos == 2)
		{
			return TEXT('1');
		}
		return TEXT('?');
	};
	FString S;
	S.AppendChar(Digit(Manager->GetPosition(Player)));
	S.AppendChar(Digit(Manager->GetPosition(AI)));
	return S;
}

void ATask11Probe::Tick(float Delta)
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
				Player->ApplyThrottle(0.0f);
				Player->ApplyBrake(0.0f);
				Player->ApplySteering(0.0f);
				UE_LOG(LogTemp, Display, TEXT("RACEPOS11E2E: player acquired"));
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
	if (!AI && Manager->GetParticipantCount() >= 2)
	{
		AI = Manager->GetParticipantVehicle(1);
		if (AI)
		{
			UE_LOG(LogTemp, Display, TEXT("RACEPOS11E2E: second acquired"));
		}
	}

	const int32 PhaseInt = static_cast<int32>(Manager->GetPhase());
	if (!bStartSent && Elapsed >= 1.0)
	{
		bStartSent = true;
		Manager->StartRace();
		UE_LOG(LogTemp, Display, TEXT("RACEPOS11E2E: start sent"));
	}

	// Build the script once Racing begins.
	if (PhaseInt == 2 && !bStepping && Steps.Num() == 0 && Player && AI)
	{
		bStepping = true;
		FStep Tag;
		Tag.Tag = TagAssertGrid;
		Steps.Add(Tag);
		for (int32 k = 0; k <= 5; ++k)
		{
			AppendCrossing(AI, k);
		}
		Tag.Tag = TagAssertProgress;
		Steps.Add(Tag);
		AppendCrossing(AI, 6);
		AppendCrossing(AI, 7);
		Tag.Tag = TagAssertDominance;
		Steps.Add(Tag);
		AppendSequence(Player);
		StagePose(Player, 7, 650.0f);
		Tag.Tag = TagAssertOvertake;
		Steps.Add(Tag);
		Tag.Tag = TagDoReset;
		Steps.Add(Tag);
		// AI back to grid after the reset (plain teleport, then assert).
		StagePose(AI, 0, -300.0f);
		Tag.Tag = TagAssertReset;
		Steps.Add(Tag);
		NextStepTime = Elapsed;
		UE_LOG(LogTemp, Display, TEXT("RACEPOS11E2E: script built, %d steps"), Steps.Num());
	}

	if (bStepping && StepIdx < Steps.Num() && Elapsed >= NextStepTime)
	{
		FStep& S = Steps[StepIdx];
		if (S.Tag != TagNone)
		{
			switch (S.Tag)
			{
			case TagAssertGrid:
				GridOrder = OrderString();
				UE_LOG(LogTemp, Display, TEXT("RACEPOS11E2E: grid %s"), *GridOrder);
				break;
			case TagAssertProgress:
				ProgressOrder = OrderString();
				UE_LOG(LogTemp, Display, TEXT("RACEPOS11E2E: progress %s"), *ProgressOrder);
				break;
			case TagAssertDominance:
				DominanceOrder = OrderString();
				UE_LOG(LogTemp, Display, TEXT("RACEPOS11E2E: dominance %s laps=%d/%d"),
					*DominanceOrder, Manager->GetParticipantLaps(0), Manager->GetParticipantLaps(1));
				break;
			case TagAssertOvertake:
				OvertakeOrder = OrderString();
				UE_LOG(LogTemp, Display, TEXT("RACEPOS11E2E: overtake %s laps=%d/%d"),
					*OvertakeOrder, Manager->GetParticipantLaps(0), Manager->GetParticipantLaps(1));
				break;
			case TagDoReset:
				Player->ResetVehicle();
				Manager->OnVehicleReset();
				UE_LOG(LogTemp, Display, TEXT("RACEPOS11E2E: reset sent"));
				break;
			case TagAssertReset:
				Manager->ReanchorParticipant(AI);
				ResetOrder = OrderString();
				bResetLaps = (Manager->GetParticipantLaps(0) == 0) && (Manager->GetParticipantLaps(1) == 0);
				Manager->StartRace();
				bWaitRacing = true;
				bStepping = false;
				UE_LOG(LogTemp, Display, TEXT("RACEPOS11E2E: reset order=%s laps0=%d, waiting racing"),
					*ResetOrder, bResetLaps);
				// Append the finish program now (needs AI staged at grid,
				// which the reset plus the previous step established).
				AppendSequence(AI);
				AppendSequence(AI);
				AppendSequence(Player);
				AppendSequence(Player);
				AppendCrossing(AI, 0);
				AppendCrossing(Player, 0);
				{
					FStep F;
					F.Tag = TagAssertFinish;
					Steps.Add(F);
				}
				{
					FStep F;
					F.Tag = TagFinish;
					Steps.Add(F);
				}
				break;
			case TagAssertFinish:
				FinishOrder = OrderString();
				LapsAtFinish0 = Manager->GetParticipantLaps(0);
				LapsAtFinish1 = Manager->GetParticipantLaps(1);
				LockedOrder = OrderString();
				UE_LOG(LogTemp, Display, TEXT("RACEPOS11E2E: finish order=%s laps=%d/%d"),
					*FinishOrder, LapsAtFinish0, LapsAtFinish1);
				break;
			case TagFinish:
				Finish(true, TEXT("program complete"));
				return;
			default:
				break;
			}
			StepIdx++;
			NextStepTime = Elapsed + Task11Limits::StepDwell;
			return;
		}
		if (S.Who)
		{
			S.Who->SetActorLocationAndRotation(S.Loc, S.Rot, false, nullptr, ETeleportType::TeleportPhysics);
		}
		StepIdx++;
		NextStepTime = Elapsed + Task11Limits::StepDwell;
	}

	// Resume stepping once the restart reaches Racing.
	if (bWaitRacing && PhaseInt == 2)
	{
		bWaitRacing = false;
		bStepping = true;
		NextStepTime = Elapsed + 0.5;
		UE_LOG(LogTemp, Display, TEXT("RACEPOS11E2E: racing again, resuming"));
	}

	if (Elapsed > Task11Limits::ProgramTimeout)
	{
		Finish(false, TEXT("program timeout"));
	}
}

void ATask11Probe::Finish(bool bOk, const FString& Note)
{
	if (bFinished)
	{
		return;
	}
	bFinished = true;
	WriteResults(bOk, Note);
	UE_LOG(LogTemp, Display, TEXT("RACEPOS11E2E: finishing (%s)"), *Note);
	UKismetSystemLibrary::QuitGame(GetWorld(), nullptr, EQuitPreference::Quit, false);
}

void ATask11Probe::WriteResults(bool bOk, const FString& Note) const
{
	const bool bGrid = (GridOrder == TEXT("01"));
	const bool bProgress = (ProgressOrder == TEXT("10"));
	const bool bDominance = (DominanceOrder == TEXT("10"));
	const bool bOvertake = (OvertakeOrder == TEXT("01"));
	const bool bReset = (ResetOrder == TEXT("01")) && bResetLaps;
	const bool bFinish = (FinishOrder == TEXT("10")) && (LapsAtFinish0 == 2) && (LapsAtFinish1 == 2)
		&& (LockedOrder == TEXT("10"));
	const bool bAll = bOk && bGrid && bProgress && bDominance && bOvertake && bReset && bFinish;

	const FString Json = FString::Printf(
		TEXT("{\"grid_order\":%s,\"progress_order\":%s,\"lap_dominance\":%s,\"overtake_flips\":%s,\"reset_order\":%s,\"finish_order\":%s,")
		TEXT("\"grid\":\"%s\",\"progress\":\"%s\",\"dominance\":\"%s\",\"overtake\":\"%s\",\"reset\":\"%s\",\"finish\":\"%s\",")
		TEXT("\"laps_player\":%d,\"laps_ai\":%d,\"frames\":%d,\"note\":\"%s\"}"),
		bGrid ? TEXT("true") : TEXT("false"), bProgress ? TEXT("true") : TEXT("false"),
		bDominance ? TEXT("true") : TEXT("false"), bOvertake ? TEXT("true") : TEXT("false"),
		bReset ? TEXT("true") : TEXT("false"), bFinish ? TEXT("true") : TEXT("false"),
		*GridOrder, *ProgressOrder, *DominanceOrder, *OvertakeOrder, *ResetOrder, *FinishOrder,
		LapsAtFinish0, LapsAtFinish1, Frames, *Note);

	const FString Dir = FPaths::ProjectSavedDir() + TEXT("Task11E2E/");
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	PF.CreateDirectoryTree(*Dir);
	FFileHelper::SaveStringToFile(Json, *(Dir + TEXT("results.json")));
	UE_LOG(LogTemp, Display, TEXT("RACEPOS11E2E: grid=%d progress=%d dominance=%d overtake=%d reset=%d finish=%d all=%d"),
		bGrid, bProgress, bDominance, bOvertake, bReset, bFinish, bAll);
}
