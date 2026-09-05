// See header. Temporary Task 2/3 harness; safe to delete.
// Task 2 timeline (seconds, frozen): 0.5 settle/record, 0.5-3.0 full
// throttle, 3.0-4.0 brake, 4.0-6.0 brake held for reverse, 6.0-6.5 settle,
// 6.5-7.5 regain speed, 7.5-10.0 throttle plus steering, 10.0 reset,
// 10.5 measure, 11.0 Task 2 complete.
// Task 3 extension (documented addition only): 10.5-11.5 gentle throttle,
// 11.5-13.0 low-speed steering window, 13.0 measure low-speed yaw rate,
// 13.5 write results and quit.

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
	constexpr double LowEnd = 11.5;   // Task 3: gentle throttle ends
	constexpr double LowSteerEnd = 13.0; // Task 3: low-speed steer window ends
	constexpr double Task3End = 13.5; // Task 3: write results and quit
	constexpr double LiftEnd = 14.5;   // Task 6: throttle-lift coast window ends
	constexpr double Task6End = 15.0;  // Task 6: write results and quit
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
	// Task 3 extension: low-speed steering window, starting only after the
	// reset measurement so the reset reference stays uncontaminated.
	// Task 2 phases above are untouched.
	else if (Elapsed >= MeasureAt && Elapsed < LowEnd)
	{
		Vehicle->ApplyThrottle(0.4f);
		Vehicle->ApplyBrake(0.0f);
		Vehicle->ApplySteering(0.0f);
	}
	else if (Elapsed >= LowEnd && Elapsed < LowSteerEnd)
	{
		Vehicle->ApplyThrottle(0.3f);
		Vehicle->ApplyBrake(0.0f);
		Vehicle->ApplySteering(0.5f);
	}
	else
	{
		Vehicle->ApplyThrottle(0.0f);
		Vehicle->ApplyBrake(0.0f);
		Vehicle->ApplySteering(0.0f);
	}

	if (Elapsed - LastSample >= SampleEvery)
	{
		LastSample = Elapsed;
		const FVector SL = Vehicle->GetActorLocation();
		const float SYaw = Vehicle->GetActorRotation().Yaw;
		const float SSpeed = Vehicle->GetForwardSpeed();
		UE_LOG(LogTemp, Display, TEXT("TASK2E2E: t=%.2f loc=%s yaw=%.2f speed=%.1f"),
			Elapsed, *SL.ToString(), SYaw, SSpeed);
		// Task 3 metric sampling.
		MinZ = FMath::Min(MinZ, SL.Z);
		TotalSamples++;
		if (Vehicle->IsGrounded())
		{
			GroundSamples++;
		}
		bool bAll = true;
		for (int32 wi = 0; wi < 4; ++wi)
		{
			bAll = bAll && Vehicle->GetWheelContact(wi);
		}
		if (bAll)
		{
			WheelAllSamples++;
		}
		if (SSpeed < 0.0f)
		{
			MaxRevAbs = FMath::Max(MaxRevAbs, -SSpeed);
		}
		if (bHaveLast && Elapsed > LastT)
		{
			const float Acc = (SSpeed - LastV) / float(Elapsed - LastT);
			if (Elapsed <= FwdEnd)
			{
				PeakAccel = FMath::Max(PeakAccel, Acc);
			}
			if (Elapsed >= FwdEnd && Elapsed <= RevEnd)
			{
				PeakDecel = FMath::Max(PeakDecel, -Acc);
			}
		}
		LastV = SSpeed;
		LastT = Elapsed;
		bHaveLast = true;

		// Task 5 wheel/tire sampling.
		const FVector TireTot = Vehicle->GetTotalTireForce();
		if (Elapsed <= 0.5)
		{
			for (int32 wi = 0; wi < 4; ++wi)
			{
				const float C = Vehicle->GetWheelCompression(wi);
				RestComprSum += C;
				RestComprN++;
				RestComprMin = FMath::Min(RestComprMin, C);
				RestComprMax = FMath::Max(RestComprMax, C);
				RestLoadSum += Vehicle->GetWheelNormalLoad(wi);
				RestLoadN++;
				const FVector P = Vehicle->GetWheelContactPoint(wi);
				const FVector N = Vehicle->GetWheelContactNormal(wi);
				RestContactN++;
				if (Vehicle->GetWheelContact(wi) && !P.ContainsNaN() && !N.ContainsNaN() && N.Z > Task5Limits::ContactNormMinZ)
				{
					RestContactOk++;
				}
			}
		}
		if (Elapsed > 0.5 && Elapsed <= 3.0)
		{
			AccelLongSum += FMath::Abs(TireTot.X);
			AccelLongN++;
		}
		if (Elapsed >= 3.0 && Elapsed <= 4.0)
		{
			BrakeLongSum += TireTot.X;
			BrakeLongN++;
		}
		if (Elapsed >= 4.5 && Elapsed <= 6.5)
		{
			RevLongSum += TireTot.X;
			RevLongN++;
		}
		if (Elapsed >= 7.5 && Elapsed <= 10.0)
		{
			SteerLatSum += FMath::Abs(TireTot.Y);
			SteerLatN++;
			// Centripetal criterion (revised after run 1; see verification
			// report): lateral force must act toward the turn center, i.e.
			// share the sign of the measured yaw rate while turning.
			const float YawR = Vehicle->GetYawRate();
			if (FMath::Abs(YawR) > 1.0f)
			{
				SteerOpposeN++;
				if (FMath::Sign(TireTot.Y) == FMath::Sign(YawR))
				{
					SteerOpposeOk++;
				}
			}
		}
		// Task 6 drivetrain sampling.
		const float RPM = Vehicle->GetEngineRPM();
		const float TQ = Vehicle->GetEngineTorque();
		const int32 Gear = Vehicle->GetGearIndex();
		MaxRPM = FMath::Max(MaxRPM, RPM);
		MinRPM = FMath::Min(MinRPM, RPM);
		MaxTorque = FMath::Max(MaxTorque, TQ);
		MaxShaft = FMath::Max(MaxShaft, Vehicle->GetLastShaftTorque());
		if (!bGotInitialGear)
		{
			bGotInitialGear = true;
			InitialGearIdx = Gear;
		}
		if (Gear >= 0)
		{
			MaxGearIdx = FMath::Max(MaxGearIdx, Gear);
		}
		else
		{
			bRevGearSeen = true;
		}
		if (Elapsed > 0.5 && Elapsed <= 3.0)
		{
			float DrivenSum = 0.0f;
			for (const int32 Di : Vehicle->GetVehicleConfig().DrivenWheelIndices)
			{
				DrivenSum += Vehicle->GetWheelLongForce(Di);
			}
			DriveSum += DrivenSum;
			ShaftSum += Vehicle->GetLastShaftTorque();
			DriveN++;
		}
		if (Elapsed >= 13.5 && Elapsed <= LiftEnd)
		{
			float DrivenSum = 0.0f;
			for (const int32 Di : Vehicle->GetVehicleConfig().DrivenWheelIndices)
			{
				DrivenSum += Vehicle->GetWheelLongForce(Di);
			}
			EbSum += DrivenSum;
			EbN++;
		}
		{
			const float Mu = Vehicle->GetVehicleConfig().FrictionMu;
			for (int32 wi = 0; wi < 4; ++wi)
			{
				const float Fl = Vehicle->GetWheelLongForce(wi);
				const float Flat = Vehicle->GetWheelLatForce(wi);
				const float Nl = Vehicle->GetWheelNormalLoad(wi);
				const float Margin = Mu * Nl - FMath::Sqrt(Fl * Fl + Flat * Flat);
				CircleMinMargin = FMath::Min(CircleMinMargin, Margin);
			}
		}
	}

	if (!bGotV10 && Elapsed >= 1.0) { bGotV10 = true; VAt10 = Vehicle->GetForwardSpeed(); }
	if (!bGotV15 && Elapsed >= 1.5) { bGotV15 = true; VAt15 = Vehicle->GetForwardSpeed(); }
	if (!bGotV25 && Elapsed >= 2.5) { bGotV25 = true; VAt25 = Vehicle->GetForwardSpeed(); }
	if (!bGotV30 && Elapsed >= 3.0) { bGotV30 = true; VAt30 = Vehicle->GetForwardSpeed(); }
	if (!bGotY85 && Elapsed >= 8.5) { bGotY85 = true; YawAt85 = Vehicle->GetActorRotation().Yaw; }
	if (!bGotY100 && Elapsed >= 10.0) { bGotY100 = true; YawAt100 = Vehicle->GetActorRotation().Yaw; }
	if (!bGotY115 && Elapsed >= 11.5) { bGotY115 = true; YawAt115 = Vehicle->GetActorRotation().Yaw; }
	if (!bGotY130 && Elapsed >= 13.0) { bGotY130 = true; YawAt130 = Vehicle->GetActorRotation().Yaw; }

	if (Elapsed >= FwdEnd && FwdEndLoc.IsZero())
	{
		FwdEndLoc = Vehicle->GetActorLocation();
		FwdEndSpeed = Vehicle->GetForwardSpeed();
		TakeShot(TEXT("02-acceleration.png"), TEXT("acceleration"));
	}

	if (!bFinalShotTaken && Elapsed >= Task6End - 0.5)
	{
		bFinalShotTaken = true;
		TakeShot(TEXT("05-final.png"), TEXT("final"));
	}

	if (Elapsed >= Task6End)
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

	// Task 3 dynamics metrics from the recorded windows.
	const double AccelLow = (VAt15 - VAt10) / 0.5;
	const double AccelHigh = (VAt30 - VAt25) / 0.5;
	const double TaperRatio = (AccelLow > 1.0) ? (AccelHigh / AccelLow) : -1.0;
	const double YawHighRate = FMath::Abs(FRotator::NormalizeAxis(YawAt100 - YawAt85)) / 1.5 / 0.4;
	const double YawLowRate = FMath::Abs(FRotator::NormalizeAxis(YawAt130 - YawAt115)) / 1.5 / 0.5;
	const double SteerRatio = (YawLowRate > 0.01) ? (YawHighRate / YawLowRate) : -1.0;
	const double GroundedFrac = (TotalSamples > 0) ? (double)GroundSamples / (double)TotalSamples : 0.0;
	const double WheelFrac = (TotalSamples > 0) ? (double)WheelAllSamples / (double)TotalSamples : 0.0;

	const bool bGrav = MinZ >= Task3Limits::MinZCm && GroundedFrac >= Task3Limits::GroundedFrac;
	const bool bMass = PeakAccel >= Task3Limits::AccelPeak && TaperRatio >= 0.0 && TaperRatio < Task3Limits::TaperMaxRatio;
	const bool bBrakeF = PeakDecel >= Task3Limits::DecelPeak;
	const bool bRevB = MaxRevAbs <= Task3Limits::RevMaxAbs;
	const bool bSteerR = SteerRatio >= 0.0 && SteerRatio < Task3Limits::SteerRatioMax;
	const bool bWheels = WheelFrac >= Task3Limits::WheelFrac;

	UE_LOG(LogTemp, Display, TEXT("TASK2E2E: T3 minZ=%.1f gfrac=%.2f wfrac=%.2f aLow=%.0f aHigh=%.0f taper=%.2f peakA=%.0f peakD=%.0f revMax=%.0f yrH=%.1f yrL=%.1f sratio=%.2f"),
		MinZ, GroundedFrac, WheelFrac, AccelLow, AccelHigh, TaperRatio, PeakAccel, PeakDecel, MaxRevAbs,
		YawHighRate, YawLowRate, SteerRatio);

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
		"  \"avg_fps\": %.1f,\n"
		"  \"task3_min_z\": %.1f,\n"
		"  \"task3_grounded_frac\": %.3f,\n"
		"  \"task3_wheel_frac\": %.3f,\n"
		"  \"task3_accel_low\": %.1f,\n"
		"  \"task3_accel_high\": %.1f,\n"
		"  \"task3_taper_ratio\": %.3f,\n"
		"  \"task3_peak_accel\": %.1f,\n"
		"  \"task3_peak_decel\": %.1f,\n"
		"  \"task3_max_rev_abs\": %.1f,\n"
		"  \"task3_yaw_rate_high\": %.2f,\n"
		"  \"task3_yaw_rate_low\": %.2f,\n"
		"  \"task3_steer_ratio\": %.3f,\n"
		"  \"pass_task3_gravity\": %s,\n"
		"  \"pass_task3_mass\": %s,\n"
		"  \"pass_task3_brake_force\": %s,\n"
		"  \"pass_task3_reverse_bound\": %s,\n"
		"  \"pass_task3_steer_rule\": %s,\n"
		"  \"pass_task3_wheels\": %s\n"
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
		Frames, AvgFps,
		MinZ, GroundedFrac, WheelFrac, AccelLow, AccelHigh, TaperRatio,
		PeakAccel, PeakDecel, MaxRevAbs, YawHighRate, YawLowRate, SteerRatio,
		bGrav ? TEXT("true") : TEXT("false"), bMass ? TEXT("true") : TEXT("false"),
		bBrakeF ? TEXT("true") : TEXT("false"), bRevB ? TEXT("true") : TEXT("false"),
		bSteerR ? TEXT("true") : TEXT("false"), bWheels ? TEXT("true") : TEXT("false"));

	const FString Dir = FPaths::ProjectSavedDir() + TEXT("Task2E2E/");
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	PF.CreateDirectoryTree(*Dir);
	// Rendered capture runs record beside the frozen baseline instead of
	// overwriting it: same schema and thresholds, separate file.
	const FString ResultsFile = bShotsEnabled ? TEXT("results-rendered.json") : TEXT("results.json");
	FFileHelper::SaveStringToFile(Json, *(Dir + ResultsFile));
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

	WriteTask4Artifact(bFwd, bBrake, bRev, bSteer, bCam, bReset,
		bGrav, bMass, bBrakeF, bRevB, bSteerR, bWheels);
	WriteTask5Artifact(bFwd, bBrake, bRev, bSteer, bCam, bReset,
		bGrav, bMass, bBrakeF, bRevB, bSteerR, bWheels);
	WriteTask6Artifact(bFwd, bBrake, bRev, bSteer, bCam, bReset,
		bGrav, bMass, bBrakeF, bRevB, bSteerR, bWheels);
}

void ATask2Probe::WriteTask6Artifact(bool bFwd, bool bBrake, bool bRev, bool bSteer, bool bCam, bool bReset,
	bool bGrav, bool bMass, bool bBrakeF, bool bRevB, bool bSteerR, bool bWheels) const
{
	const float DriveMean = (DriveN > 0) ? (DriveSum / (float)DriveN) : 0.0f;
	const float ShaftMean = (DriveN > 0) ? (ShaftSum / (float)DriveN) : 0.0f;
	const float EbMean = (EbN > 0) ? (EbSum / (float)EbN) : 0.0f;
	const int32 Ups = Vehicle ? Vehicle->GetUpshiftCount() : 0;
	const int32 Downs = Vehicle ? Vehicle->GetDownshiftCount() : 0;
	const int32 MaxGear1 = (MaxGearIdx >= 0) ? (MaxGearIdx + 1) : 0;
	const int32 InitGear1 = bGotInitialGear ? ((InitialGearIdx >= 0) ? (InitialGearIdx + 1) : InitialGearIdx) : -99;

	const bool bEngResp = (MaxRPM > Task6Limits::RPMResponse) && (MaxTorque > 0.0f);
	const bool bRpmB = (MinRPM >= Task6Limits::RPMMin) && (MaxRPM <= Task6Limits::RPMMaxOk);
	const bool bGearP = (MaxGear1 >= Task6Limits::GearMaxMin) && (Ups >= Task6Limits::ShiftsMin) && (Downs >= Task6Limits::ShiftsMin);
	const bool bRevD = bRevGearSeen;
	const bool bTorq = (DriveMean > Task6Limits::DriveMeanMin);
	const bool bEb = (EbMean <= -Task6Limits::EbMin) && (EbMean >= -Task6Limits::EbMax);

	UE_LOG(LogTemp, Display, TEXT("TASK2E2E: T6 rpm[%.0f,%.0f] tq=%.0f gearInit=%d gearMax=%d up=%d down=%d rev=%d drive=%.0f shaft=%.0f eb=%.0f"),
		MinRPM, MaxRPM, MaxTorque, InitGear1, MaxGear1, Ups, Downs, bRevGearSeen ? 1 : 0,
		DriveMean, ShaftMean, EbMean);

	const FString Json = FString::Printf(
		TEXT("{\"engine_rpm_max\":%.0f,\"engine_rpm_min\":%.0f,\"engine_torque_max\":%.0f,")
		TEXT("\"initial_gear\":%d,\"maximum_forward_gear\":%d,\"reverse_gear\":%s,")
		TEXT("\"wheel_torque_mean_nm\":%.0f,\"wheel_force_mean_n\":%.0f,")
		TEXT("\"upshift_count\":%d,\"downshift_count\":%d,\"engine_braking_force_n\":%.0f,")
		TEXT("\"engine_response\":%s,\"rpm_bounds\":%s,\"gear_progression\":%s,")
		TEXT("\"reverse_drive\":%s,\"torque_transfer\":%s,\"engine_braking\":%s,")
		TEXT("\"regression\":{\"forward\":%s,\"brake\":%s,\"reverse\":%s,\"steer\":%s,\"camera\":%s,\"reset\":%s,")
		TEXT("\"gravity\":%s,\"mass\":%s,\"brake_force\":%s,\"reverse_bound\":%s,\"steer_rule\":%s,\"wheels_state\":%s,")
		TEXT("\"contact\":%s,\"suspension\":%s,\"load\":%s,\"longitudinal\":%s,\"lateral\":%s,\"combined\":%s}}"),
		MaxRPM, MinRPM, MaxTorque, InitGear1, MaxGear1, bRevGearSeen ? TEXT("true") : TEXT("false"),
		ShaftMean, DriveMean, Ups, Downs, EbMean,
		bEngResp ? TEXT("true") : TEXT("false"), bRpmB ? TEXT("true") : TEXT("false"),
		bGearP ? TEXT("true") : TEXT("false"), bRevD ? TEXT("true") : TEXT("false"),
		bTorq ? TEXT("true") : TEXT("false"), bEb ? TEXT("true") : TEXT("false"),
		bFwd ? TEXT("true") : TEXT("false"), bBrake ? TEXT("true") : TEXT("false"),
		bRev ? TEXT("true") : TEXT("false"), bSteer ? TEXT("true") : TEXT("false"),
		bCam ? TEXT("true") : TEXT("false"), bReset ? TEXT("true") : TEXT("false"),
		bGrav ? TEXT("true") : TEXT("false"), bMass ? TEXT("true") : TEXT("false"),
		bBrakeF ? TEXT("true") : TEXT("false"), bRevB ? TEXT("true") : TEXT("false"),
		bSteerR ? TEXT("true") : TEXT("false"), bWheels ? TEXT("true") : TEXT("false"),
		T5Contact ? TEXT("true") : TEXT("false"), T5Susp ? TEXT("true") : TEXT("false"),
		T5Load ? TEXT("true") : TEXT("false"), T5Long ? TEXT("true") : TEXT("false"),
		T5Lat ? TEXT("true") : TEXT("false"), T5Circle ? TEXT("true") : TEXT("false"));

	const FString Dir = FPaths::ProjectSavedDir() + TEXT("Task6E2E/");
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	PF.CreateDirectoryTree(*Dir);
	// Rendered capture runs record beside the frozen baseline instead of
	// overwriting it: same schema and thresholds, separate file.
	const FString ResultsFile = bShotsEnabled ? TEXT("results-rendered.json") : TEXT("results.json");
	FFileHelper::SaveStringToFile(Json, *(Dir + ResultsFile));
	UE_LOG(LogTemp, Display, TEXT("TASK2E2E: task6 artifact written"));
}

void ATask2Probe::WriteTask5Artifact(bool bFwd, bool bBrake, bool bRev, bool bSteer, bool bCam, bool bReset,
	bool bGrav, bool bMass, bool bBrakeF, bool bRevB, bool bSteerR, bool bWheels) const
{
	const float RestMean = (RestComprN > 0) ? (RestComprSum / (float)RestComprN) : -1.0f;
	const float RestRange = (RestComprN > 0) ? (RestComprMax - RestComprMin) : -1.0f;
	const float RestLoadMean = (RestLoadN > 0) ? (RestLoadSum / (float)RestLoadN) : -1.0f;
	const float ContactFrac = (RestContactN > 0) ? (float)RestContactOk / (float)RestContactN : 0.0f;
	const float AccelMean = (AccelLongN > 0) ? (AccelLongSum / (float)AccelLongN) : 0.0f;
	const float BrakeMean = (BrakeLongN > 0) ? (BrakeLongSum / (float)BrakeLongN) : 0.0f;
	const float RevMean = (RevLongN > 0) ? (RevLongSum / (float)RevLongN) : 0.0f;
	const float LatMean = (SteerLatN > 0) ? (SteerLatSum / (float)SteerLatN) : 0.0f;
	const float OpposeFrac = (SteerOpposeN > 0) ? (float)SteerOpposeOk / (float)SteerOpposeN : 0.0f;
	const float Mass = Vehicle ? Vehicle->GetVehicleConfig().MassKg : 0.0f;
	const float WeightN = Mass * 9.8f;
	const float LoadErr = (WeightN > 0.0f && RestLoadN > 0) ? FMath::Abs(RestLoadMean * 4.0f - WeightN) / WeightN : 1.0f;

	const bool bContact = (RestContactN > 0) && (ContactFrac >= Task5Limits::ContactFrac - 1e-6f);
	const bool bSusp = (RestMean > Task5Limits::ComprMin) && (RestComprMax <= Task5Limits::ComprMax)
		&& (RestRange >= 0.0f) && (RestRange < Task5Limits::OscMaxRange);
	const bool bLoad = (LoadErr < Task5Limits::LoadTotalTol) && (RestLoadMean > 0.0f);
	const bool bLong = (AccelMean > Task5Limits::LongMin) && (BrakeMean < -Task5Limits::LongMin)
		&& (RevMean < Task5Limits::RevMeanMax);
	const bool bLat = (LatMean > Task5Limits::LatMin) && (OpposeFrac >= Task5Limits::OpposeFrac);
	const bool bCircle = (CircleMinMargin > -Task5Limits::CircleTol);

	T5Contact = bContact;
	T5Susp = bSusp;
	T5Load = bLoad;
	T5Long = bLong;
	T5Lat = bLat;
	T5Circle = bCircle;

	UE_LOG(LogTemp, Display, TEXT("TASK2E2E: T5 compr=%.2f range=%.2f load=%.0f/%.0f long=%.0f/%.0f/%.0f lat=%.0f opp=%.2f circ=%.0f"),
		RestMean, RestRange, RestLoadMean * 4.0f, WeightN, AccelMean, BrakeMean, RevMean,
		LatMean, OpposeFrac, CircleMinMargin);

	const FString Json = FString::Printf(
		TEXT("{\"contact_ok\":%s,\"susp_ok\":%s,\"load_ok\":%s,\"long_ok\":%s,\"lat_ok\":%s,\"circle_ok\":%s,")
		TEXT("\"rest_compr_mean_cm\":%.2f,\"rest_compr_range_cm\":%.2f,\"rest_load_total_n\":%.0f,\"weight_n\":%.0f,")
		TEXT("\"long_accel_mean_n\":%.0f,\"long_brake_mean_n\":%.0f,\"long_rev_mean_n\":%.0f,")
		TEXT("\"lat_mean_n\":%.0f,\"oppose_frac\":%.3f,\"circle_min_margin_n\":%.0f,")
		TEXT("\"regression\":{\"forward\":%s,\"brake\":%s,\"reverse\":%s,\"steer\":%s,\"camera\":%s,\"reset\":%s,")
		TEXT("\"gravity\":%s,\"mass\":%s,\"brake_force\":%s,\"reverse_bound\":%s,\"steer_rule\":%s,\"wheels_state\":%s}}"),
		bContact ? TEXT("true") : TEXT("false"), bSusp ? TEXT("true") : TEXT("false"),
		bLoad ? TEXT("true") : TEXT("false"), bLong ? TEXT("true") : TEXT("false"),
		bLat ? TEXT("true") : TEXT("false"), bCircle ? TEXT("true") : TEXT("false"),
		RestMean, RestRange, RestLoadMean * 4.0f, WeightN,
		AccelMean, BrakeMean, RevMean, LatMean, OpposeFrac, CircleMinMargin,
		bFwd ? TEXT("true") : TEXT("false"), bBrake ? TEXT("true") : TEXT("false"),
		bRev ? TEXT("true") : TEXT("false"), bSteer ? TEXT("true") : TEXT("false"),
		bCam ? TEXT("true") : TEXT("false"), bReset ? TEXT("true") : TEXT("false"),
		bGrav ? TEXT("true") : TEXT("false"), bMass ? TEXT("true") : TEXT("false"),
		bBrakeF ? TEXT("true") : TEXT("false"), bRevB ? TEXT("true") : TEXT("false"),
		bSteerR ? TEXT("true") : TEXT("false"), bWheels ? TEXT("true") : TEXT("false"));

	const FString Dir = FPaths::ProjectSavedDir() + TEXT("Task5E2E/");
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	PF.CreateDirectoryTree(*Dir);
	// Rendered capture runs record beside the frozen baseline instead of
	// overwriting it: same schema and thresholds, separate file.
	const FString ResultsFile = bShotsEnabled ? TEXT("results-rendered.json") : TEXT("results.json");
	FFileHelper::SaveStringToFile(Json, *(Dir + ResultsFile));
	UE_LOG(LogTemp, Display, TEXT("TASK2E2E: task5 artifact written"));
}

void ATask2Probe::WriteTask4Artifact(bool bFwd, bool bBrake, bool bRev, bool bSteer, bool bCam, bool bReset,
	bool bGrav, bool bMass, bool bBrakeF, bool bRevB, bool bSteerR, bool bWheels) const
{
	const FRaceVehicleConfig& Cfg = Vehicle ? Vehicle->GetVehicleConfig() : FRaceVehicleConfig();
	const FRaceVehicleConfig& Act = Vehicle ? Vehicle->GetActiveConfig() : FRaceVehicleConfig();

	const bool bMassOk = FMath::IsNearlyEqual(Cfg.MassKg, 1200.0f);
	const bool bGravOk = FMath::IsNearlyEqual(Cfg.GravityCmS2, 980.0f);
	const bool bBrakeOk = FMath::IsNearlyEqual(Cfg.BrakeForceN, 14000.0f);
	const bool bRevOk = FMath::IsNearlyEqual(Cfg.MaxReverseSpeed, 700.0f);
	const bool bEngOk = Cfg.EngineTorqueCurve.Num() == 14;

	FString WheelJson;
	bool bWheelsOk = (Cfg.Wheels.Num() == 4);
	const FString WantNames[4] = { TEXT("FL"), TEXT("FR"), TEXT("RL"), TEXT("RR") };
	const bool WantFront[4] = { true, true, false, false };
	const bool WantLeft[4] = { true, false, true, false };
	for (int32 i = 0; i < Cfg.Wheels.Num(); ++i)
	{
		const FRaceWheelConfig& W = Cfg.Wheels[i];
		if (i > 0)
		{
			WheelJson += TEXT(",");
		}
		WheelJson += FString::Printf(TEXT("{\"name\":\"%s\",\"front\":%s,\"left\":%s,\"offset\":\"%s\"}"),
			*W.Name.ToString(), W.bFrontAxle ? TEXT("true") : TEXT("false"),
			W.bLeftSide ? TEXT("true") : TEXT("false"), *W.LocalOffset.ToString());
		if (i >= 4 || W.Name.ToString() != WantNames[i] || W.bFrontAxle != WantFront[i] || W.bLeftSide != WantLeft[i])
		{
			bWheelsOk = false;
		}
	}
	const bool bMatch = FMath::IsNearlyEqual(Cfg.MassKg, Act.MassKg)
		&& FMath::IsNearlyEqual(Cfg.BrakeForceN, Act.BrakeForceN)
		&& Act.Wheels.Num() == Cfg.Wheels.Num();

	const FString Json = FString::Printf(
		TEXT("{\"config_source\":\"ARaceVehicle.VehicleConfig\",\"mass_ok\":%s,\"gravity_ok\":%s,")
		TEXT("\"brake_ok\":%s,\"revcap_ok\":%s,\"engine_points_ok\":%s,\"wheels_roles_ok\":%s,")
		TEXT("\"movement_matches_pawn\":%s,\"wheels\":[%s],")
		TEXT("\"regression\":{\"forward\":%s,\"brake\":%s,\"reverse\":%s,\"steer\":%s,\"camera\":%s,\"reset\":%s,")
		TEXT("\"gravity\":%s,\"mass\":%s,\"brake_force\":%s,\"reverse_bound\":%s,\"steer_rule\":%s,\"wheels_state\":%s}}"),
		bMassOk ? TEXT("true") : TEXT("false"), bGravOk ? TEXT("true") : TEXT("false"),
		bBrakeOk ? TEXT("true") : TEXT("false"), bRevOk ? TEXT("true") : TEXT("false"),
		bEngOk ? TEXT("true") : TEXT("false"), bWheelsOk ? TEXT("true") : TEXT("false"),
		bMatch ? TEXT("true") : TEXT("false"), *WheelJson,
		bFwd ? TEXT("true") : TEXT("false"), bBrake ? TEXT("true") : TEXT("false"),
		bRev ? TEXT("true") : TEXT("false"), bSteer ? TEXT("true") : TEXT("false"),
		bCam ? TEXT("true") : TEXT("false"), bReset ? TEXT("true") : TEXT("false"),
		bGrav ? TEXT("true") : TEXT("false"), bMass ? TEXT("true") : TEXT("false"),
		bBrakeF ? TEXT("true") : TEXT("false"), bRevB ? TEXT("true") : TEXT("false"),
		bSteerR ? TEXT("true") : TEXT("false"), bWheels ? TEXT("true") : TEXT("false"));

	const FString Dir = FPaths::ProjectSavedDir() + TEXT("Task4E2E/");
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	PF.CreateDirectoryTree(*Dir);
	// Rendered capture runs record beside the frozen baseline instead of
	// overwriting it: same schema and thresholds, separate file.
	const FString ResultsFile = bShotsEnabled ? TEXT("results-rendered.json") : TEXT("results.json");
	FFileHelper::SaveStringToFile(Json, *(Dir + ResultsFile));
	UE_LOG(LogTemp, Display, TEXT("TASK2E2E: task4 artifact written"));
}
