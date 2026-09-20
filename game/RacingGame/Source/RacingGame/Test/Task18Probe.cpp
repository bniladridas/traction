// See header.

#include "Task18Probe.h"
#include "RaceCockpitTestGameMode.h"
#include "RaceVehicle.h"
#include "RaceTrack.h"
#include "RaceManager.h"
#include "RaceAIDriver.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

ATask18Probe::ATask18Probe()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATask18Probe::BeginPlay()
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

int32 ATask18Probe::NearestIndex(const FVector& Pos) const
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

void ATask18Probe::DrivePlayer()
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

bool ATask18Probe::bHasRaceViewBinding() const
{
	UInputComponent* IC = Player ? Player->InputComponent : nullptr;
	if (!IC)
	{
		return false;
	}
	for (int32 i = 0; i < IC->GetNumActionBindings(); ++i)
	{
		const FInputActionBinding& B = IC->GetActionBinding(i);
		if (B.GetActionName() == FName(TEXT("RaceView")) && B.ActionDelegate.IsBound())
		{
			return true;
		}
	}
	return false;
}

void ATask18Probe::Tick(float Delta)
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
	if (!AI)
	{
		AI = Manager->GetParticipantVehicle(1);
		if (AI)
		{
			TArray<UActorComponent*> Comps;
			AI->GetComponents(URaceAIDriver::StaticClass(), Comps);
			if (Comps.Num() > 0)
			{
				Driver = Cast<URaceAIDriver>(Comps[0]);
			}
			LastMoveT = Elapsed;
			LastDist = Driver ? Driver->GetProgressDistance() : 0.0f;
			LastRec = Driver ? Driver->GetRecoveryCount() : 0;
			UE_LOG(LogTemp, Display, TEXT("RACECOCKPIT18E2E: AI acquired driver=%d"), Driver != nullptr);
		}
	}

	const ERacePhase Phase = Manager->GetPhase();
	const bool bRacing = (Phase == ERacePhase::Racing);

	// Gate 1-5: static camera contract, once, before racing.
	if (!bGatesStarted && Elapsed >= 1.5)
	{
		bGatesStarted = true;
		UCameraComponent* Chase = Player->GetChaseCamera();
		UCameraComponent* Cockpit = Player->GetCockpitCamera();
		if (!Chase || !Cockpit || !Player->GetChaseCameraDriver())
		{
			Finish(false, TEXT("missing camera components on player"));
			return;
		}

		// Gate 1: configured default view is Chase (Task 10 preserved).
		UE_LOG(LogTemp, Display, TEXT("RACECOCKPIT18E2E: default view=%d"), (int32)Player->GetViewMode());
		bViewConfigured = (Player->GetViewMode() == ERaceViewMode::Chase)
			&& Chase->IsActive() && !Cockpit->IsActive();

		// Apply the run's camera config through the data knob, then
		// verify the cockpit pose/FOV reflect it (gates 3/4). Default
		// view stays Chase, so Task 10 behavior is untouched.
		FRaceCameraConfig Cfg;
		Cfg.DefaultView = ERaceViewMode::Chase;
		Cfg.CockpitPitchDeg = Task18Limits::CockpitPitchDeg;
		Cfg.CockpitFov = Task18Limits::CockpitFov;
		Player->SetCameraConfig(Cfg);

		const FRaceVehicleConfig& V = Player->GetVehicleConfig();
		// Gate 3: rigid cockpit - attached to the root, no spring arm,
		// fixed offset from config, no pawn control rotation.
		bool bRigid = true;
		USceneComponent* Ancestor = Cockpit;
		while (Ancestor)
		{
			if (Cast<USpringArmComponent>(Ancestor))
			{
				bRigid = false;
				break;
			}
			Ancestor = Ancestor->GetAttachParent();
		}
		bRigid = bRigid && Cockpit->GetAttachParent() == Player->GetRootComponent()
			&& !Cockpit->bUsePawnControlRotation
			&& !Cockpit->IsUsingAbsoluteRotation();
		const float OffsetErr = FVector::Dist(Cockpit->GetRelativeLocation(), V.Camera.CockpitOffset);
		bRigid = bRigid && (OffsetErr <= Task18Limits::PosTolCm);
		bCockpitRigid = bRigid;
		UE_LOG(LogTemp, Display, TEXT("RACECOCKPIT18E2E: rigid=%d offset_err=%.2f"), bRigid, OffsetErr);

		// Gate 4: pitch and FOV are the configured values, not magic.
		const FRotator RelRot = Cockpit->GetRelativeRotation();
		const float PitchErr = FMath::Abs(FRotator::NormalizeAxis(RelRot.Pitch - V.Camera.CockpitPitchDeg));
		const float FovErr = FMath::Abs(Cockpit->FieldOfView - V.Camera.CockpitFov);
		bPitchFov = (PitchErr <= Task18Limits::AngleTolDeg) && (FovErr <= Task18Limits::AngleTolDeg);
		UE_LOG(LogTemp, Display, TEXT("RACECOCKPIT18E2E: pitch_err=%.2f fov_err=%.2f"), PitchErr, FovErr);

		// Gate 2: CycleView toggles deterministically and the RaceView
		// action reaches the pawn's input path.
		Player->SetViewMode(ERaceViewMode::Cockpit);
		const bool bCockpitActive = (Player->GetViewMode() == ERaceViewMode::Cockpit)
			&& !Chase->IsActive() && Cockpit->IsActive();
		Player->CycleView();
		const bool bChaseActive = (Player->GetViewMode() == ERaceViewMode::Chase)
			&& Chase->IsActive() && !Cockpit->IsActive();
		Player->CycleView(); // back to cockpit
		const bool bCockpitAgain = (Player->GetViewMode() == ERaceViewMode::Cockpit)
			&& !Chase->IsActive() && Cockpit->IsActive();
		bViewToggle = bCockpitActive && bChaseActive && bCockpitAgain && bHasRaceViewBinding();
		UE_LOG(LogTemp, Display, TEXT("RACECOCKPIT18E2E: toggle=%d binding=%d"),
			bViewToggle, bHasRaceViewBinding());

		// Gate 5: reset preserves the selected view (currently cockpit).
		Player->ResetVehicle();
		const bool bAfterVehicleReset = (Player->GetViewMode() == ERaceViewMode::Cockpit)
			&& !Chase->IsActive() && Cockpit->IsActive();
		Manager->OnVehicleReset();
		const bool bAfterManagerReset = (Player->GetViewMode() == ERaceViewMode::Cockpit)
			&& !Chase->IsActive() && Cockpit->IsActive();
		bResetView = bAfterVehicleReset && bAfterManagerReset;
		UE_LOG(LogTemp, Display, TEXT("RACECOCKPIT18E2E: reset_view=%d"), bResetView);

		if (!bViewConfigured || !bViewToggle || !bCockpitRigid || !bPitchFov || !bResetView)
		{
			Finish(false, TEXT("static gate failed"));
			return;
		}
	}

	// Reset the player to the grid lined up with the AI after the
	// mid-setup manager reset, then start the race.
	if (bGatesStarted && !bStartSent && Elapsed >= 2.5)
	{
		bStartSent = true;
		Manager->StartRace();
	}

	const bool bRacingSeenThis = (bRacing && !bRacingSeen);
	if (bRacingSeenThis)
	{
		bRacingSeen = true;
		bWaitRacing = false;
		bPlayerAnchored = false;
		PlayerCheckT = Elapsed;
		PlayerCheckS = -1e9f;
		if (Driver)
		{
			LastMoveT = Elapsed;
			LastDist = Driver->GetProgressDistance();
			LastRec = Driver->GetRecoveryCount();
		}
		UE_LOG(LogTemp, Display, TEXT("RACECOCKPIT18E2E: racing"));
	}

	if (bRacing)
	{
		DrivePlayer();

		// Recovery mirrors the AI driver rule so a traffic-wrecked car
		// cannot add infinite loop iterations.
		if (!Manager->IsParticipantFinished(0))
		{
			if ((PlayerS - PlayerCheckS) > Task18Limits::StallProgressCm)
			{
				PlayerCheckT = Elapsed;
				PlayerCheckS = PlayerS;
			}
			if ((Elapsed - PlayerCheckT) > Task18Limits::StallWindow)
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
				UE_LOG(LogTemp, Display, TEXT("RACECOCKPIT18E2E: player recovered at s=%.0f"), PlayerS);
			}
		}

		// In-race CycleView toggles at fixed elapsed checkpoints for
		// deterministic timing, proving the switch runs during racing.
		if (RaceToggles < Task18Limits::MinRaceToggles
			&& ((Elapsed >= 6.0 && RaceToggles == 0)
				|| (Elapsed >= 10.0 && RaceToggles == 1)
				|| (Elapsed >= 15.0 && RaceToggles == 2)))
		{
			Player->CycleView();
			RaceToggles++;
			UE_LOG(LogTemp, Display, TEXT("RACECOCKPIT18E2E: race toggle %d mode=%d"),
				RaceToggles, (int32)Player->GetViewMode());
		}

		// Deadlock monitoring across the AI field.
		if (Driver)
		{
			const float D = Driver->GetProgressDistance();
			if ((D - LastDist) > 5.0f || Driver->GetRecoveryCount() != LastRec)
			{
				LastMoveT = Elapsed;
				LastDist = D;
				LastRec = Driver->GetRecoveryCount();
			}
			MaxGap = FMath::Max(MaxGap, Elapsed - LastMoveT);
			if (!Manager->IsParticipantFinished(1) && (Elapsed - LastMoveT) > Task18Limits::StallWindow)
			{
				bDeadlockOk = false;
				Finish(false, TEXT("deadlock: AI stalled past window"));
				return;
			}
		}
	}

	// Completion: both cars finished after at least the required number
	// of in-race toggles, and view switching still works at the end.
	if (bRacingSeen && Manager->GetParticipantCount() == Task18Limits::FieldSize)
	{
		const bool bBoth = Manager->IsParticipantFinished(0) && Manager->IsParticipantFinished(1);
		if (bBoth && RaceToggles >= Task18Limits::MinRaceToggles)
		{
			const ERaceViewMode Final = Player->GetViewMode();
			Player->CycleView();
			const ERaceViewMode After = Player->GetViewMode();
			bRaceCompatible = (After != Final);
			UE_LOG(LogTemp, Display, TEXT("RACECOCKPIT18E2E: race_compatible=%d final=%d after=%d"),
				bRaceCompatible, (int32)Final, (int32)After);
			Finish(true, TEXT("cockpit race complete"));
			return;
		}
	}

	if (Elapsed > Task18Limits::ProgramTimeout)
	{
		Finish(false, TEXT("program timeout"));
	}
}

void ATask18Probe::Finish(bool bOk, const FString& Note)
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
	UE_LOG(LogTemp, Display, TEXT("RACECOCKPIT18E2E: finishing (%s)"), *Note);
	UKismetSystemLibrary::QuitGame(GetWorld(), nullptr, EQuitPreference::Quit, false);
}

void ATask18Probe::WriteResults(bool bOk, const FString& Note) const
{
	const bool bAllOk = bOk && bViewConfigured && bViewToggle && bCockpitRigid
		&& bPitchFov && bResetView && bRaceCompatible && bDeadlockOk;

	const FString Json = FString::Printf(
		TEXT("{\"view_configured\":%s,\"view_toggle\":%s,\"cockpit_rigid\":%s,\"pitch_and_fov\":%s,")
		TEXT("\"reset_preserves_view\":%s,\"race_compatible\":%s,")
		TEXT("\"race_toggles\":%d,\"max_gap\":%.1f,\"frames\":%d,\"note\":\"%s\"}"),
		bViewConfigured ? TEXT("true") : TEXT("false"), bViewToggle ? TEXT("true") : TEXT("false"),
		bCockpitRigid ? TEXT("true") : TEXT("false"), bPitchFov ? TEXT("true") : TEXT("false"),
		bResetView ? TEXT("true") : TEXT("false"), bRaceCompatible ? TEXT("true") : TEXT("false"),
		RaceToggles, MaxGap, Frames, *Note);

	const FString Dir = FPaths::ProjectSavedDir() + TEXT("Task18E2E/");
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	PF.CreateDirectoryTree(*Dir);
	FFileHelper::SaveStringToFile(Json, *(Dir + TEXT("results.json")));
	UE_LOG(LogTemp, Display, TEXT("RACECOCKPIT18E2E: configured=%d toggle=%d rigid=%d pitchfov=%d reset=%d race=%d deadlock=%d all=%d"),
		bViewConfigured, bViewToggle, bCockpitRigid, bPitchFov, bResetView, bRaceCompatible, bDeadlockOk, bAllOk);
}