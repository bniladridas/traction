// See header.

#include "Task10Probe.h"
#include "RaceVehicle.h"
#include "RaceTrack.h"
#include "RaceManager.h"
#include "RaceChaseCamera.h"
#include "RaceAIDriver.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

ATask10Probe::ATask10Probe()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATask10Probe::BeginPlay()
{
	Super::BeginPlay();
	bShotsEnabled = FParse::Param(FCommandLine::Get(), TEXT("Task10Shots"));
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

int32 ATask10Probe::NearestIndex(const FVector& Pos) const
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

void ATask10Probe::DrivePlayer()
{
	const TArray<FRaceTrackCenterPoint>& Pts = Track->GetCenterPoints();
	const float L = Track->GetTrackLength();
	const int32 N = Pts.Num();
	// Degenerate track (fewer than 2 center points): no valid pursuit
	// geometry; the stall/timeout trip below produces the verdict.
	if (N < 2 || L <= 0.0f)
	{
		return;
	}
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

void ATask10Probe::TakeShot(const FString& FileName, const FString& Phase)
{
	if (!bShotsEnabled || bFinished || !Player || !PlayerCam)
	{
		return;
	}
	const FString Dir = FPaths::ProjectSavedDir() + TEXT("Task10E2E/screenshots/");
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	PF.CreateDirectoryTree(*Dir);
	FScreenshotRequest::RequestScreenshot(Dir + FileName, false, false);
	ShotEntries.Add(FString::Printf(
		TEXT("{\"file\":\"%s\",\"phase\":\"%s\",\"t\":%.2f,\"veh_loc\":\"%s\",\"veh_rot\":\"%s\",\"cam_loc\":\"%s\",\"cam_rot\":\"%s\"}"),
		*FileName, *Phase, Elapsed,
		*Player->GetActorLocation().ToString(), *Player->GetActorRotation().ToString(),
		*PlayerCam->GetComponentLocation().ToString(), *PlayerCam->GetComponentRotation().ToString()));
	UE_LOG(LogTemp, Display, TEXT("RACECAM10E2E: shot requested %s (%s)"), *FileName, *Phase);
}

void ATask10Probe::Tick(float Delta)
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
				PlayerCam = Player->GetChaseCamera();
				PlayerDriver = Player->GetChaseCameraDriver();
				UE_LOG(LogTemp, Display, TEXT("RACECAM10E2E: player acquired cam=%d"), PlayerCam != nullptr);
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
			AICam = AI->GetChaseCamera();
			TArray<UActorComponent*> Comps;
			AI->GetComponents(URaceAIDriver::StaticClass(), Comps);
			if (Comps.Num() > 0)
			{
				AIDriver = Cast<URaceAIDriver>(Comps[0]);
			}
			UE_LOG(LogTemp, Display, TEXT("RACECAM10E2E: AI acquired cam=%d driver=%d"),
				AICam != nullptr, AIDriver != nullptr);
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
	}
	if (bRacing)
	{
		DrivePlayer();
	}

	// Initial camera offset once settled, before any turn develops.
	// Travel accumulates path length (a lap returns near its start, so
	// endpoint distance would read near zero).
	if (PlayerCam && !bInitRecorded && Elapsed >= 2.0)
	{
		bInitRecorded = true;
		InitCamOffset = PlayerCam->GetComponentLocation() - Player->GetActorLocation();
		InitCamYawErr = FRotator::NormalizeAxis(
			PlayerCam->GetComponentRotation().Yaw - Player->GetActorRotation().Yaw);
		PrevCamP = PlayerCam->GetComponentLocation();
		PrevPawnP = Player->GetActorLocation();
		if (AICam && AI)
		{
			PrevCamA = AICam->GetComponentLocation();
			PrevPawnA = AI->GetActorLocation();
		}
	}
	// Exclusion state hoisted so window accumulation can share it.
	// Behavior-neutral: pure reads plus the same conditional timestamp
	// update, before any of its readers.
	bool bRecoveryTick = false;
	if (AIDriver && AIDriver->GetRecoveryCount() != LastAIRecoveries)
	{
		LastAIRecoveries = AIDriver->GetRecoveryCount();
		AIExcludeUntil = Elapsed + 1.5;
		bRecoveryTick = true;
	}
	const bool bMeasResetWindow = bResetDone && Elapsed > Task10Limits::ResetAt - 0.5 && Elapsed < Task10Limits::ResetAt + 1.5;
	const bool bMeasAIWindow = Elapsed < AIExcludeUntil;
	float TickCamP = 0.0f;
	float TickPawnP = 0.0f;
	float TickCamA = 0.0f;
	float TickPawnA = 0.0f;
	if (bInitRecorded && PlayerCam && AICam && AI)
	{
		TickCamP = FVector::Dist(PlayerCam->GetComponentLocation(), PrevCamP);
		TickPawnP = FVector::Dist(Player->GetActorLocation(), PrevPawnP);
		TickCamA = FVector::Dist(AICam->GetComponentLocation(), PrevCamA);
		TickPawnA = FVector::Dist(AI->GetActorLocation(), PrevPawnA);
		CamPathP += TickCamP;
		PawnPathP += TickPawnP;
		CamPathA += TickCamA;
		PawnPathA += TickPawnA;
		PrevCamP = PlayerCam->GetComponentLocation();
		PrevPawnP = Player->GetActorLocation();
		PrevCamA = AICam->GetComponentLocation();
		PrevPawnA = AI->GetActorLocation();
		// Revised pop measurement: fixed pawn-travel windows, skipping
		// ticks inside the existing exclusion periods. An excluded tick
		// discards the partial window so no evaluated window can bridge
		// an exclusion boundary; windows begin fresh afterward. Window
		// size and cap are frozen contract thresholds.
		if (!bMeasResetWindow && !bMeasAIWindow)
		{
			WinCamP += TickCamP;
			WinPawnP += TickPawnP;
			WinCamA += TickCamA;
			WinPawnA += TickPawnA;
			if (WinPawnP >= Task10Limits::PawnTravelWindowCm)
			{
				const float R = WinCamP / FMath::Max(WinPawnP, 1.0f);
				WinRatioMaxP = FMath::Max(WinRatioMaxP, R);
				WinCountP++;
				WinCamP = 0.0f;
				WinPawnP = 0.0f;
			}
			if (WinPawnA >= Task10Limits::PawnTravelWindowCm)
			{
				const float R = WinCamA / FMath::Max(WinPawnA, 1.0f);
				WinRatioMaxA = FMath::Max(WinRatioMaxA, R);
				WinCountA++;
				WinCamA = 0.0f;
				WinPawnA = 0.0f;
			}
		}
		else
		{
			WinCamP = 0.0f;
			WinPawnP = 0.0f;
			WinCamA = 0.0f;
			WinPawnA = 0.0f;
		}
	}

	// Look-ahead sampling across turning samples.
	if (PlayerDriver && FMath::Abs(Player->GetYawRate()) > Task10Limits::TurnSampleMinRate)
	{
		LeadSum += PlayerDriver->GetRelativeYaw() * FMath::Sign(Player->GetYawRate());
		LeadN++;
		// Heading traversed while eligible for turn sampling.
		TurnArcDegSum += FMath::Abs(Player->GetYawRate()) * Delta;
	}

	// Per-tick displacement outside discontinuity windows (player reset
	// and AI recovery respawns both snap by design). Recovery bookkeeping
	// runs above so window accumulation can share the exclusion state;
	// the exclusion windows below are unchanged.
	if (bRecoveryTick)
	{
		UE_LOG(LogTemp, Display, TEXT("RACECAM10E2E: AI recovery %d, pop window excluded"), LastAIRecoveries);
	}
	const bool bInResetWindow = bMeasResetWindow;
	const bool bInAIWindow = bMeasAIWindow;
	if (bHaveLast && !bInResetWindow && !bInAIWindow && PlayerCam && AICam)
	{
		MaxPopP = FMath::Max(MaxPopP, FVector::Dist(PlayerCam->GetComponentLocation(), LastCamP));
		MaxPopA = FMath::Max(MaxPopA, FVector::Dist(AICam->GetComponentLocation(), LastCamA));
	}
	if (PlayerCam && AICam)
	{
		LastCamP = PlayerCam->GetComponentLocation();
		LastCamA = AICam->GetComponentLocation();
		bHaveLast = true;
	}

	// Mid-run reset with track-start re-snap, then keep driving.
	if (!bResetDone && Elapsed >= Task10Limits::ResetAt)
	{
		bResetDone = true;
		Player->ResetVehicle();
		Player->SetActorLocationAndRotation(Track->GetStartPosition(),
			FRotator(0.0f, Track->GetStartYawDeg(), 0.0f), false, nullptr, ETeleportType::TeleportPhysics);
		bPlayerAnchored = false;
		bHaveLast = false;
		UE_LOG(LogTemp, Display, TEXT("RACECAM10E2E: player reset"));
	}
	if (bResetDone && !bResetMeasured && Elapsed >= Task10Limits::ResetAt + 1.0)
	{
		bResetMeasured = true;
		const FVector OffNow = PlayerCam->GetComponentLocation() - Player->GetActorLocation();
		ResetPosErr = FVector::Dist(OffNow, InitCamOffset);
		ResetYawErr = FMath::Abs(FRotator::NormalizeAxis(
			(PlayerCam->GetComponentRotation().Yaw - Player->GetActorRotation().Yaw) - InitCamYawErr));
		UE_LOG(LogTemp, Display, TEXT("RACECAM10E2E: reset poserr=%.1f yawerr=%.2f"), ResetPosErr, ResetYawErr);
	}

	if (!bShotStart && Elapsed >= 2.0)
	{
		bShotStart = true;
		TakeShot(TEXT("track-start.png"), TEXT("start"));
	}
	if (!bShotSweeper && Elapsed >= 10.0)
	{
		bShotSweeper = true;
		TakeShot(TEXT("track-sweeper.png"), TEXT("sweeper"));
	}
	if (!bShotHairpin && Elapsed >= 15.0)
	{
		bShotHairpin = true;
		TakeShot(TEXT("track-hairpin.png"), TEXT("hairpin"));
	}
	if (!bShotRace && Elapsed >= 30.0)
	{
		bShotRace = true;
		TakeShot(TEXT("track-race.png"), TEXT("race"));
	}
	if (Elapsed >= Task10Limits::FinishAt)
	{
		Finish(true, TEXT("program complete"));
	}
	if (Elapsed > Task10Limits::FinishAt + 30.0)
	{
		Finish(false, TEXT("program timeout"));
	}
}

void ATask10Probe::Finish(bool bOk, const FString& Note)
{
	if (bFinished)
	{
		return;
	}
	bFinished = true;
	for (ARaceVehicle* V : { Player, AI })
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
	UE_LOG(LogTemp, Display, TEXT("RACECAM10E2E: finishing (%s)"), *Note);
	UKismetSystemLibrary::QuitGame(GetWorld(), nullptr, EQuitPreference::Quit, false);
}

void ATask10Probe::WriteResults(bool bOk, const FString& Note) const
{
	const bool bFollowP = (PawnPathP > 50.0) && (CamPathP > Task10Limits::FollowMinRatio * PawnPathP);
	const bool bFollowA = (PawnPathA > 50.0) && (CamPathA > Task10Limits::FollowMinRatio * PawnPathA);
	const double LeadMean = (LeadN > 0) ? (LeadSum / (double)LeadN) : 0.0;
	// Revised contract: mean plus turn-arc floor (frame-rate independent).
	// The old LeadN sample-count floor is diagnostic only.
	const bool bLead = (LeadMean > Task10Limits::LeadMinDeg) && (TurnArcDegSum >= Task10Limits::TurnArcFloorDeg);
	// Revised contract: every evaluated window under the ratio cap, with
	// at least one evaluated window per camera so the gate cannot pass
	// vacuously. The old per-tick MaxPop criterion is diagnostic only.
	const bool bNoPops = (WinCountP > 0) && (WinCountA > 0)
		&& (WinRatioMaxP <= Task10Limits::WindowTravelRatioCap)
		&& (WinRatioMaxA <= Task10Limits::WindowTravelRatioCap);
	const bool bReset = bResetMeasured && (ResetPosErr >= 0.0f) && (ResetPosErr < Task10Limits::ResetMaxPosCm)
		&& (ResetYawErr < Task10Limits::ResetMaxYawDeg);
	const bool bRace = bRacingSeen;
	const bool bAll = bOk && bFollowP && bFollowA && bLead && bNoPops && bReset && bRace;

	const FString Json = FString::Printf(
		TEXT("{\"follow_player\":%s,\"follow_ai\":%s,\"lookahead_lead\":%s,\"no_pops\":%s,\"reset_snap\":%s,\"race_compatible\":%s,")
		TEXT("\"cam_travel_p\":%.1f,\"pawn_travel_p\":%.1f,\"cam_travel_a\":%.1f,\"pawn_travel_a\":%.1f,")
		TEXT("\"lead_mean_deg\":%.2f,\"lead_samples\":%d,\"max_pop_p_cm\":%.2f,\"max_pop_a_cm\":%.2f,")
		TEXT("\"turn_arc_deg\":%.1f,\"turn_arc_floor_deg\":%.1f,\"window_ratio_max\":%.2f,\"window_ratio_cap\":%.1f,\"window_count\":%d,")
		TEXT("\"window_count_p\":%d,\"window_count_a\":%d,")
		TEXT("\"window_ratio_max_p\":%.2f,\"window_ratio_max_a\":%.2f,")
		TEXT("\"reset_pos_err_cm\":%.1f,\"reset_yaw_err_deg\":%.2f,\"frames\":%d,\"note\":\"%s\"}"),
		bFollowP ? TEXT("true") : TEXT("false"), bFollowA ? TEXT("true") : TEXT("false"),
		bLead ? TEXT("true") : TEXT("false"), bNoPops ? TEXT("true") : TEXT("false"),
		bReset ? TEXT("true") : TEXT("false"), bRace ? TEXT("true") : TEXT("false"),
		CamPathP, PawnPathP, CamPathA, PawnPathA, LeadMean, LeadN, MaxPopP, MaxPopA,
		TurnArcDegSum, Task10Limits::TurnArcFloorDeg,
		FMath::Max(WinRatioMaxP, WinRatioMaxA), Task10Limits::WindowTravelRatioCap,
		WinCountP + WinCountA, WinCountP, WinCountA, WinRatioMaxP, WinRatioMaxA,
		ResetPosErr, ResetYawErr, Frames, *Note);

	const FString Dir = FPaths::ProjectSavedDir() + TEXT("Task10E2E/");
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	PF.CreateDirectoryTree(*Dir);
	// Rendered capture runs record beside the frozen baseline instead of
	// overwriting it: same schema and thresholds, separate file.
	const FString ResultsFile = bShotsEnabled ? TEXT("results-rendered.json") : TEXT("results.json");
	FFileHelper::SaveStringToFile(Json, *(Dir + ResultsFile));
	FString Manifest = TEXT("{\"shots_requested\":");
	Manifest += bShotsEnabled ? TEXT("true") : TEXT("false");
	Manifest += TEXT(",\"renderer_note\":\"Captures go through the real renderer pipeline; PNG files materialize only in rendered (non-nullrhi) runs.\",\"captures\":[");
	Manifest += FString::Join(ShotEntries, TEXT(","));
	Manifest += TEXT("]}");
	PF.CreateDirectoryTree(*(Dir + TEXT("screenshots/")));
	FFileHelper::SaveStringToFile(Manifest, *(Dir + TEXT("screenshots/manifest.json")));
	UE_LOG(LogTemp, Display, TEXT("RACECAM10E2E: manifest written with %d captures"), ShotEntries.Num());
	UE_LOG(LogTemp, Display, TEXT("RACECAM10E2E: followP=%d followA=%d lead=%d pops=%d reset=%d race=%d all=%d"),
		bFollowP, bFollowA, bLead, bNoPops, bReset, bRace, bAll);
	UE_LOG(LogTemp, Display, TEXT("RACECAM10E2E: MEASURE turnarc=%.1f leadmean=%.2f leadn=%d winmaxP=%.2f wincntP=%d winmaxA=%.2f wincntA=%d"),
		TurnArcDegSum, LeadMean, LeadN, WinRatioMaxP, WinCountP, WinRatioMaxA, WinCountA);
}
