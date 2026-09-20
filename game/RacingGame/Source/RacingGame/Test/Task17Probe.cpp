// See header.

#include "Task17Probe.h"
#include "RaceHudTestGameMode.h"
#include "RaceHudModel.h"
#include "RaceHudWidget.h"
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

ATask17Probe::ATask17Probe()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATask17Probe::BeginPlay()
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
		return;
	}
	if (ARaceHudTestGameMode* GM = Cast<ARaceHudTestGameMode>(GetWorld()->GetAuthGameMode()))
	{
		HudModel = GM->GetHudModel();
		HudWidget = GM->GetHudWidget();
	}
}

int32 ATask17Probe::NearestIndex(const FVector& Pos) const
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

void ATask17Probe::DrivePlayer()
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

bool ATask17Probe::bBoundModel() const
{
	return HudModel && HudModel->IsBound() && HudWidget
		&& HudWidget->GetModel() == HudModel
		&& HudModel->GetConfig().bShowCountdown;
}

void ATask17Probe::RestageAll()
{
	if (!Player)
	{
		return;
	}
	FVector Loc;
	float Yaw = 0.0f;
	Track->GetGridPose(0, Loc, Yaw);
	Loc.Z = 40.0f;
	Player->SetActorLocationAndRotation(Loc, FRotator(0.0f, Yaw, 0.0f),
		false, nullptr, ETeleportType::TeleportPhysics);
	Player->ResetMotion();
	Manager->ReanchorParticipant(Player);
	for (int32 Slot = 1; Slot <= 5; ++Slot)
	{
		ARaceVehicle* AI = AIs[Slot - 1];
		if (!AI)
		{
			continue;
		}
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
}

void ATask17Probe::Tick(float Delta)
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
			if (!Player)
			{
				Finish(false, TEXT("player pawn is not an ARaceVehicle"));
				return;
			}
			if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
			{
				Player->DisableInput(PC);
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

	for (int32 i = 1; i <= 5; ++i)
	{
		if (AIs[i - 1])
		{
			continue;
		}
		ARaceVehicle* AI = Manager->GetParticipantVehicle(i);
		if (AI)
		{
			AIs[i - 1] = AI;
			TArray<UActorComponent*> Comps;
			AI->GetComponents(URaceAIDriver::StaticClass(), Comps);
			if (Comps.Num() > 0)
			{
				Drivers[i - 1] = Cast<URaceAIDriver>(Comps[0]);
			}
		}
	}
	const bool bField = AIs[0] && AIs[1] && AIs[2] && AIs[3] && AIs[4];

	const ERacePhase Phase = Manager->GetPhase();
	const bool bRacing = (Phase == ERacePhase::Racing);

	// Gate 1: model bound, widget bound to it, countdown enabled.
	if (!bStartSent && Elapsed >= 1.0)
	{
		bStartSent = true;
		bBound = bBoundModel();
		UE_LOG(LogTemp, Display, TEXT("RACEHUD17E2E: bound gate=%d"), bBound);
		if (!bBound)
		{
			Finish(false, TEXT("hud bound gate failed"));
			return;
		}
		Manager->StartRace();
	}

	// Gate 2: countdown derived from manager alone, reaching 0 at Racing.
	// Sampled only above the 0-second boundary (Expected >= 2) so the
	// manager-tick/probe-tick ordering race at the phase switch cannot
	// fabricate a failure; the cleared check covers the boundary.
	if (Phase == ERacePhase::Countdown)
	{
		bInCountdown = true;
		HudModel->Refresh();
		const int32 Secs = HudModel->GetCountdownSeconds();
		const float Remaining = Manager->GetCountdownRemaining();
		const int32 Expected = FMath::CeilToInt32(Remaining);
		if (Expected >= 2
			&& !HudModel->GetCountdownText().IsEmpty()
			&& Secs == Expected)
		{
			bCountdownOk = true;
			CountdownSamples++;
		}
	}
	if (bInCountdown && bRacing)
	{
		HudModel->Refresh();
		bCountdownCleared = (HudModel->GetCountdownSeconds() == 0)
			&& HudModel->GetCountdownText().IsEmpty();
		if (!bCountdownCleared)
		{
			Finish(false, TEXT("countdown did not clear at racing"));
			return;
		}
		bRacingSeen = true;
		for (int32 i = 0; i < 5; ++i)
		{
			LastMoveT[i] = Elapsed;
		}
	}

	// Drive the player with pursuit (Task 15 pattern); recovery mirrors
	// the AI driver rule so a traffic-wrecked car cannot circle invalid.
	if (bRacing)
	{
		DrivePlayer();
		if (!Manager->IsParticipantFinished(0))
		{
			if ((PlayerS - PlayerCheckS) > Task17Limits::StallProgressCm)
			{
				PlayerCheckT = Elapsed;
				PlayerCheckS = PlayerS;
			}
			if ((Elapsed - PlayerCheckT) > Task17Limits::StallWindow)
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
				UE_LOG(LogTemp, Display, TEXT("RACEHUD17E2E: player recovered at s=%.0f"), PlayerS);
			}
		}
	}

	// Gates 3/4: live lap and position binding while Racing.
	if (bRacing)
	{
		HudModel->Refresh();
		const int32 ManagerLap = Manager->GetParticipantLaps(0);
		const int32 ModelLap = HudModel->GetLap();
		const int32 ModelTotal = HudModel->GetTotalLaps();
		const bool bLapOk = (ModelLap == ManagerLap)
			&& (ModelTotal == Task17Limits::RaceLaps)
			&& (HudModel->GetLapText() == FString::Printf(
				TEXT("Lap %d/%d"), ModelLap, ModelTotal));
		if (!bLapOk)
		{
			Finish(false, TEXT("lap display mismatch"));
			return;
		}
		const int32 ManagerPos = Manager->GetPosition(Player);
		const int32 ModelPos = HudModel->GetPosition();
		const int32 ModelField = HudModel->GetFieldSize();
		const bool bPosOk = (ManagerPos > 0) && (ModelPos == ManagerPos)
			&& (ModelField == Manager->GetParticipantCount())
			&& (HudModel->GetPositionText() == FString::Printf(
				TEXT("%d/%d"), ModelPos, ModelField));
		if (!bPosOk)
		{
			Finish(false, TEXT("position display mismatch"));
			return;
		}
		RacingSamples++;
		if (ManagerLap > 0)
		{
			bLapLiveSeen = true;
		}
		if (ManagerPos != Manager->GetParticipantCount())
		{
			bPosLiveSeen = true;
		}
	}

	// Deadlock monitoring across the AI field.
	if (bRacingSeen && bField && !bWaitRacing)
	{
		for (int32 i = 0; i < 5; ++i)
		{
			if (!Drivers[i])
			{
				continue;
			}
			const float D = Drivers[i]->GetProgressDistance();
			if ((D - LastDist[i]) > 5.0f || Drivers[i]->GetRecoveryCount() != LastRec[i])
			{
				LastMoveT[i] = Elapsed;
				LastDist[i] = D;
				LastRec[i] = Drivers[i]->GetRecoveryCount();
			}
			MaxGap = FMath::Max(MaxGap, Elapsed - LastMoveT[i]);
			if (!Manager->IsParticipantFinished(i + 1) && (Elapsed - LastMoveT[i]) > Task17Limits::StallWindow)
			{
				bDeadlockOk = false;
				Finish(false, TEXT("deadlock: AI stalled past window"));
				return;
			}
		}
	}

	// Mid-program reset with grid re-staging, then restart.
	if (!bResetDone && bRacingSeen && Elapsed >= Task17Limits::ResetAt)
	{
		bResetDone = true;
		RestageAll();
		Manager->OnVehicleReset();
		HudModel->Refresh();
		bResetCleared = (HudModel->GetLap() == 0)
			&& (HudModel->GetCountdownSeconds() == 0)
			&& (HudModel->GetCountdownText().IsEmpty())
			&& !HudModel->HasFinish()
			&& (HudModel->GetFinishText().IsEmpty());
		Manager->StartRace();
		bWaitRacing = true;
		bInCountdown = false;
		bPlayerAnchored = false;
		PlayerCheckT = Elapsed;
		PlayerCheckS = -1e9f;
		UE_LOG(LogTemp, Display, TEXT("RACEHUD17E2E: reset cleared=%d"), bResetCleared);
	}
	if (bWaitRacing && bRacing)
	{
		bWaitRacing = false;
		bRacingSeen = true;
		for (int32 i = 0; i < 5; ++i)
		{
			LastMoveT[i] = Elapsed;
			if (Drivers[i])
			{
				LastDist[i] = Drivers[i]->GetProgressDistance();
				LastRec[i] = Drivers[i]->GetRecoveryCount();
			}
		}
		UE_LOG(LogTemp, Display, TEXT("RACEHUD17E2E: racing again"));
	}

	// Completion: all six finished and the HUD shows participant 0's
	// finish through the model (gate 5).
	if (bRacingSeen && !bWaitRacing
		&& Manager->GetParticipantCount() == Task17Limits::FieldSize)
	{
		bool bAll = true;
		for (int32 i = 0; i < 6; ++i)
		{
			bAll = bAll && Manager->IsParticipantFinished(i);
		}
		if (bAll)
		{
			HudModel->Refresh();
			const bool bHas = HudModel->HasFinish();
			const FString Fin = HudModel->GetFinishText();
			const int32 FinPos = HudModel->GetPosition();
			const int32 Field = HudModel->GetFieldSize();
			bFinishDisp = bHas
				&& !Fin.IsEmpty()
				&& (Fin == FString::Printf(TEXT("Finished %d of %d"), FinPos, Field));
			UE_LOG(LogTemp, Display, TEXT("RACEHUD17E2E: finish disp=%d (has=%d '%s')"),
				bFinishDisp, bHas, *Fin);
			Finish(true, TEXT("hud race complete"));
			return;
		}
	}

	if (Elapsed > Task17Limits::ProgramTimeout)
	{
		Finish(false, TEXT("program timeout"));
	}
}

void ATask17Probe::Finish(bool bOk, const FString& Note)
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
	UE_LOG(LogTemp, Display, TEXT("RACEHUD17E2E: finishing (%s)"), *Note);
	UKismetSystemLibrary::QuitGame(GetWorld(), nullptr, EQuitPreference::Quit, false);
}

void ATask17Probe::WriteResults(bool bOk, const FString& Note) const
{
	const bool bCnt = bCountdownOk && bCountdownCleared && (CountdownSamples >= 1);
	const bool bLap = bLapLiveSeen && (RacingSamples >= Task17Limits::MinRacingSamples);
	const bool bPos = bPosLiveSeen && (RacingSamples >= Task17Limits::MinRacingSamples);
	const bool bAllOk = bOk && bBound && bCnt && bLap && bPos && bFinishDisp && bResetCleared && bDeadlockOk;

	const FString Json = FString::Printf(
		TEXT("{\"hud_bound\":%s,\"hud_countdown_shown\":%s,\"hud_lap_display\":%s,\"hud_position_display\":%s,\"hud_finish_display\":%s,\"hud_clears_on_reset\":%s,")
		TEXT("\"racing_samples\":%d,\"countdown_samples\":%d,\"max_gap\":%.1f,\"frames\":%d,\"note\":\"%s\"}"),
		bBound ? TEXT("true") : TEXT("false"), bCnt ? TEXT("true") : TEXT("false"),
		bLap ? TEXT("true") : TEXT("false"), bPos ? TEXT("true") : TEXT("false"),
		bFinishDisp ? TEXT("true") : TEXT("false"), bResetCleared ? TEXT("true") : TEXT("false"),
		RacingSamples, CountdownSamples, MaxGap, Frames, *Note);

	const FString Dir = FPaths::ProjectSavedDir() + TEXT("Task17E2E/");
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	PF.CreateDirectoryTree(*Dir);
	FFileHelper::SaveStringToFile(Json, *(Dir + TEXT("results.json")));
	UE_LOG(LogTemp, Display, TEXT("RACEHUD17E2E: bound=%d countdown=%d lap=%d pos=%d finish=%d reset=%d deadlock=%d all=%d"),
		bBound, bCnt, bLap, bPos, bFinishDisp, bResetCleared, bDeadlockOk, bAllOk);
}