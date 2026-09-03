// See header. Temporary Task 2 harness; safe to delete.
// Timeline (seconds): 0.5 settle/record, 0.5-3.0 full throttle,
// 3.0-4.0 brake, 4.0-6.0 brake held for reverse, 6.0-6.5 settle,
// 6.5-7.5 regain speed, 7.5-10.0 throttle plus steering, 10.0 reset,
// 10.5 measure, 11.0 write results and quit.

#include "Task2Probe.h"
#include "RaceVehicle.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "HAL/PlatformFileManager.h"

namespace
{
	constexpr double SettleEnd = 0.5;
	constexpr double FwdEnd = 3.0;
	constexpr double BrakeEnd = 4.0;
	constexpr double RevEnd = 6.0;
	constexpr double RegainEnd = 7.5;
	constexpr double SteerEnd = 10.0;
	constexpr double ResetAt = 10.0;
	constexpr double MeasureAt = 10.5;
	constexpr double RunEnd = 11.0;
	constexpr double SampleEvery = 0.25;
	constexpr double PawnTimeout = 15.0;
}

ATask2Probe::ATask2Probe()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATask2Probe::BeginPlay()
{
	Super::BeginPlay();
	bShotsEnabled = FParse::Param(FCommandLine::Get(), TEXT("Task2Shots"));
	UE_LOG(LogTemp, Display, TEXT("TASK2E2E: probe BeginPlay map=%s engine=%s shots=%d"),
		*GetWorld()->GetMapName(), *FString(ENGINE_VERSION_STRING), bShotsEnabled ? 1 : 0);
}

void ATask2Probe::TakeShot(const FString& FileName, const FString& Phase)
{
	if (!bShotsEnabled || bFinished || !Vehicle)
	{
		return;
	}
	const FString Dir = FPaths::ProjectSavedDir() + TEXT("Task2E2E/screenshots/");
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	PF.CreateDirectoryTree(*Dir);
	FScreenshotRequest::RequestScreenshot(Dir + FileName, false, false);
	const FString CamLoc = ChaseCam ? ChaseCam->GetComponentLocation().ToString() : TEXT("none");
	const FString CamRot = ChaseCam ? ChaseCam->GetComponentRotation().ToString() : TEXT("none");
	ShotEntries.Add(FString::Printf(
		TEXT("{\"file\":\"%s\",\"phase\":\"%s\",\"t\":%.2f,\"veh_loc\":\"%s\",\"veh_rot\":\"%s\",\"cam_loc\":\"%s\",\"cam_rot\":\"%s\"}"),
		*FileName, *Phase, Elapsed,
		*Vehicle->GetActorLocation().ToString(), *Vehicle->GetActorRotation().ToString(),
		*CamLoc, *CamRot));
	UE_LOG(LogTemp, Display, TEXT("TASK2E2E: shot requested %s (%s)"), *FileName, *Phase);
}

void ATask2Probe::Tick(float Delta)
{
	Super::Tick(Delta);
	if (bFinished)
	{
		return;
	}

	Elapsed += Delta;
	Frames++;
	FrameTimeSum += Delta;

	if (!Vehicle)
	{
		if (APawn* P = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
		{
			Vehicle = Cast<ARaceVehicle>(P);
			if (Vehicle)
			{
				ChaseCam = Vehicle->GetChaseCamera();
				// Own input exclusively for the automated run: otherwise the
				// engine input stack re-fires the axis bindings with 0 every
				// frame and overwrites the probe's values after each write.
				if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
				{
					Vehicle->DisableInput(PC);
				}
				UE_LOG(LogTemp, Display, TEXT("TASK2E2E: pawn acquired class=%s (input owned by probe)"),
					*Vehicle->GetClass()->GetPathName());
			}
			else
			{
				Finish(false, TEXT("player pawn is not an ARaceVehicle"));
				return;
			}
		}
		if (!Vehicle)
		{
			if (Elapsed > PawnTimeout)
			{
				Finish(false, TEXT("no player pawn within timeout"));
			}
			return;
		}
	}

	if (!bStartRecorded && Elapsed >= SettleEnd)
	{
		bStartRecorded = true;
		StartLoc = Vehicle->GetActorLocation();
		StartRot = Vehicle->GetActorRotation();
		if (ChaseCam)
		{
			CamStart = ChaseCam->GetComponentLocation();
		}
		UE_LOG(LogTemp, Display, TEXT("TASK2E2E: start loc=%s yaw=%.2f"),
			*StartLoc.ToString(), StartRot.Yaw);
		TakeShot(TEXT("01-spawn.png"), TEXT("spawn"));
	}

	// Drive through the same public functions the keys call.
	// Settle window first: no inputs until the start transform is recorded.
	if (Elapsed < SettleEnd)
	{
		Vehicle->ApplyThrottle(0.0f);
		Vehicle->ApplyBrake(0.0f);
		Vehicle->ApplySteering(0.0f);
	}
	else if (Elapsed < FwdEnd)
	{
		Vehicle->ApplyThrottle(1.0f);
		Vehicle->ApplyBrake(0.0f);
		Vehicle->ApplySteering(0.0f);
		StraightNoiseDeg = FMath::Max(StraightNoiseDeg,
			FMath::Abs(FRotator::NormalizeAxis(Vehicle->GetActorRotation().Yaw - StartRot.Yaw)));
	}
	else if (Elapsed < BrakeEnd)
	{
		if (BrakeEnterSpeed == 0.0f)
		{
			BrakeEnterSpeed = FMath::Abs(Vehicle->GetForwardSpeed());
		}
		Vehicle->ApplyThrottle(0.0f);
		Vehicle->ApplyBrake(1.0f);
		Vehicle->ApplySteering(0.0f);
		BrakeEndSpeed = FMath::Abs(Vehicle->GetForwardSpeed());
	}
	else if (Elapsed < RevEnd)
	{
		if (RevStartLoc.IsZero())
		{
			RevStartLoc = Vehicle->GetActorLocation();
		}
		Vehicle->ApplyThrottle(0.0f);
		Vehicle->ApplyBrake(1.0f); // held near standstill: engages reverse
		Vehicle->ApplySteering(0.0f);
		RevEndLoc = Vehicle->GetActorLocation();
	}
	else if (Elapsed < RegainEnd)
	{
		Vehicle->ApplyThrottle(1.0f);
		Vehicle->ApplyBrake(0.0f);
		Vehicle->ApplySteering(0.0f);
	}
	else if (Elapsed < SteerEnd)
	{
		if (!bSteerCaptured)
		{
			bSteerCaptured = true;
			SteerStartRot = Vehicle->GetActorRotation();
		}
		Vehicle->ApplyThrottle(0.8f);
		Vehicle->ApplyBrake(0.0f);
		Vehicle->ApplySteering(0.4f);
		SteerEndRot = Vehicle->GetActorRotation();
	}
	else if (!bSteerEndCaptured)
	{
		// Freeze the camera/pawn travel metrics before reset undoes them.
		bSteerEndCaptured = true;
		PawnSteerEndLoc = Vehicle->GetActorLocation();
		if (ChaseCam)
		{
			CamEnd = ChaseCam->GetComponentLocation();
		}
		TakeShot(TEXT("03-steering.png"), TEXT("steering"));
		UE_LOG(LogTemp, Display, TEXT("TASK2E2E: steer end captured"));
	}
	else if (Elapsed >= ResetAt && !bResetDone)
	{
		bResetDone = true;
		Vehicle->ResetVehicle(); // the exact function the R key calls
	}
	else if (Elapsed >= MeasureAt && ResetPosErr < 0.0f)
	{
		ResetPosErr = FVector::Dist(Vehicle->GetActorLocation(), StartLoc);
		ResetYawErr = FMath::Abs(FRotator::NormalizeAxis(Vehicle->GetActorRotation().Yaw - StartRot.Yaw));
		ResetSpeed = FMath::Abs(Vehicle->GetForwardSpeed());
		UE_LOG(LogTemp, Display, TEXT("TASK2E2E: reset posErr=%.3f yawErr=%.3f speed=%.3f"),
			ResetPosErr, ResetYawErr, ResetSpeed);
		TakeShot(TEXT("04-reset.png"), TEXT("reset"));
	}

	if (Elapsed - LastSample >= SampleEvery)
	{
		LastSample = Elapsed;
		UE_LOG(LogTemp, Display, TEXT("TASK2E2E: t=%.2f loc=%s yaw=%.2f speed=%.1f"),
			Elapsed, *Vehicle->GetActorLocation().ToString(),
			Vehicle->GetActorRotation().Yaw, Vehicle->GetForwardSpeed());
	}

	if (Elapsed >= FwdEnd && FwdEndLoc.IsZero())
	{
		FwdEndLoc = Vehicle->GetActorLocation();
		FwdEndSpeed = Vehicle->GetForwardSpeed();
		TakeShot(TEXT("02-acceleration.png"), TEXT("acceleration"));
	}

	if (!bFinalShotTaken && Elapsed >= RunEnd - 0.5)
	{
		bFinalShotTaken = true;
		TakeShot(TEXT("05-final.png"), TEXT("final"));
	}

	if (Elapsed >= RunEnd)
	{
		Finish(true, TEXT("run completed"));
	}
}

void ATask2Probe::Finish(bool bOk, const FString& Note)
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
	UE_LOG(LogTemp, Display, TEXT("TASK2E2E: finishing (%s)"), *Note);
	UKismetSystemLibrary::QuitGame(GetWorld(), nullptr, EQuitPreference::Quit, false);
}

void ATask2Probe::WriteResults(bool bOk, const FString& Note) const
{
	const FVector InitFwd = StartRot.Vector();
	const FVector FwdDisp = FwdEndLoc - StartLoc;
	const double FwdDist = FwdDisp.Size();
	const double FwdAngle = FMath::RadiansToDegrees(
		FMath::Acos(FMath::Clamp(FVector::DotProduct(FwdDisp.GetSafeNormal(), InitFwd), -1.0, 1.0)));
	const double BrakeRatio = (BrakeEnterSpeed > 0.0) ? (BrakeEndSpeed / BrakeEnterSpeed) : -1.0;
	const FVector RevDisp = RevEndLoc - RevStartLoc;
	const double RevBack = FVector::DotProduct(RevDisp, -InitFwd);
	const double SteerDelta = FMath::Abs(FRotator::NormalizeAxis(SteerEndRot.Yaw - SteerStartRot.Yaw));
	const double CamTravel = FVector::Dist(CamStart, CamEnd);
	const double PawnTravel = FVector::Dist(StartLoc, PawnSteerEndLoc);
	const double AvgFps = (FrameTimeSum > 0.0) ? (double)Frames / FrameTimeSum : 0.0;

	FString Attach = TEXT("none");
	if (ChaseCam)
	{
		Attach = ChaseCam->GetName();
		for (const USceneComponent* C = ChaseCam->GetAttachParent(); C; C = C->GetAttachParent())
		{
			Attach += TEXT(" <- ") + C->GetName();
			if (C->GetOwner() == Vehicle)
			{
				break;
			}
		}
	}

	const bool bFwd = bStartRecorded && FwdDist > Task2Limits::FwdMinDispCm && FwdAngle < Task2Limits::FwdMaxAngleDeg;
	const bool bBrake = BrakeEnterSpeed > Task2Limits::BrakeMinEnterSpeed && BrakeRatio >= 0.0 && BrakeRatio < Task2Limits::BrakeMaxRatio;
	const bool bRev = RevBack > Task2Limits::RevMinDispCm;
	const bool bSteer = StraightNoiseDeg < Task2Limits::StraightMaxNoiseDeg && SteerDelta > Task2Limits::SteerMinDeltaDeg;
	const bool bCam = ChaseCam && CamTravel > Task2Limits::CamMinRatio * PawnTravel && PawnTravel > Task2Limits::FwdMinDispCm;
	const bool bReset = ResetPosErr >= 0.0f && ResetPosErr < Task2Limits::ResetMaxPosErrCm
		&& ResetYawErr < Task2Limits::ResetMaxYawErrDeg && ResetSpeed < Task2Limits::ResetMaxSpeed;

	const FString Json = FString::Printf(
		TEXT("{\n"
		"  \"reached_end\": %s,\n"
		"  \"note\": \"%s\",\n"
		"  \"engine\": \"%s\",\n"
		"  \"map\": \"%s\",\n"
		"  \"pawn_class\": \"%s\",\n"
		"  \"thresholds\": {\"fwd_min_cm\": %.1f, \"fwd_max_ang\": %.1f, \"brake_min_enter\": %.1f, \"brake_max_ratio\": %.2f, \"rev_min_cm\": %.1f, \"straight_max_noise\": %.1f, \"steer_min_delta\": %.1f, \"cam_min_ratio\": %.2f, \"reset_max_pos\": %.1f, \"reset_max_yaw\": %.1f, \"reset_max_speed\": %.1f},\n"
		"  \"fwd_dist_cm\": %.1f,\n"
		"  \"fwd_angle_deg\": %.2f,\n"
		"  \"fwd_end_speed\": %.1f,\n"
		"  \"straight_noise_deg\": %.2f,\n"
		"  \"brake_enter_speed\": %.1f,\n"
		"  \"brake_end_speed\": %.1f,\n"
		"  \"brake_ratio\": %.3f,\n"
		"  \"rev_back_cm\": %.1f,\n"
		"  \"steer_delta_deg\": %.2f,\n"
		"  \"camera_chain\": \"%s\",\n"
		"  \"camera_travel_cm\": %.1f,\n"
		"  \"pawn_travel_cm\": %.1f,\n"
		"  \"reset_pos_err_cm\": %.3f,\n"
		"  \"reset_yaw_err_deg\": %.3f,\n"
		"  \"reset_speed\": %.3f,\n"
		"  \"pass_forward\": %s,\n"
		"  \"pass_brake\": %s,\n"
		"  \"pass_reverse\": %s,\n"
		"  \"pass_steer\": %s,\n"
		"  \"pass_camera\": %s,\n"
		"  \"pass_reset\": %s,\n"
		"  \"frames\": %d,\n"
		"  \"avg_fps\": %.1f\n"
		"}"),
		bOk ? TEXT("true") : TEXT("false"), *Note, *FString(ENGINE_VERSION_STRING),
		GetWorld() ? *GetWorld()->GetMapName() : TEXT("null"),
		Vehicle ? *Vehicle->GetClass()->GetPathName() : TEXT("null"),
		Task2Limits::FwdMinDispCm, Task2Limits::FwdMaxAngleDeg,
		Task2Limits::BrakeMinEnterSpeed, Task2Limits::BrakeMaxRatio,
		Task2Limits::RevMinDispCm, Task2Limits::StraightMaxNoiseDeg,
		Task2Limits::SteerMinDeltaDeg, Task2Limits::CamMinRatio,
		Task2Limits::ResetMaxPosErrCm, Task2Limits::ResetMaxYawErrDeg,
		Task2Limits::ResetMaxSpeed,
		FwdDist, FwdAngle, FwdEndSpeed, StraightNoiseDeg,
		BrakeEnterSpeed, BrakeEndSpeed, BrakeRatio, RevBack, SteerDelta,
		*Attach, CamTravel, PawnTravel, ResetPosErr, ResetYawErr, ResetSpeed,
		bFwd ? TEXT("true") : TEXT("false"), bBrake ? TEXT("true") : TEXT("false"),
		bRev ? TEXT("true") : TEXT("false"), bSteer ? TEXT("true") : TEXT("false"),
		bCam ? TEXT("true") : TEXT("false"), bReset ? TEXT("true") : TEXT("false"),
		Frames, AvgFps);

	const FString Dir = FPaths::ProjectSavedDir() + TEXT("Task2E2E/");
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	PF.CreateDirectoryTree(*Dir);
	FFileHelper::SaveStringToFile(Json, *(Dir + TEXT("results.json")));
	UE_LOG(LogTemp, Display, TEXT("TASK2E2E: results written, fwd=%.1f brake=%.3f rev=%.1f steer=%.2f cam=%.1f/%.1f"),
		FwdDist, BrakeRatio, RevBack, SteerDelta, CamTravel, PawnTravel);

	FString Manifest = TEXT("{\"shots_requested\":");
	Manifest += bShotsEnabled ? TEXT("true") : TEXT("false");
	Manifest += TEXT(",\"renderer_note\":\"Captures go through the real renderer pipeline; PNG files materialize only in rendered (non-nullrhi) runs.\",\"captures\":[");
	Manifest += FString::Join(ShotEntries, TEXT(","));
	Manifest += TEXT("]}");
	PF.CreateDirectoryTree(*(Dir + TEXT("screenshots/")));
	FFileHelper::SaveStringToFile(Manifest, *(Dir + TEXT("screenshots/manifest.json")));
	UE_LOG(LogTemp, Display, TEXT("TASK2E2E: manifest written with %d captures"), ShotEntries.Num());
}
