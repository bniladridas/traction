// See header.

#include "Task9Probe.h"
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

ATask9Probe::ATask9Probe()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATask9Probe::BeginPlay()
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

int32 ATask9Probe::NearestIndex(const FVector& Pos) const
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

void ATask9Probe::ParkPlayer()
{
	// Parked clear of the AI line: behind the start, near the right edge,
	// on the road, zero input. Single-racer AI test; the manager still
	// tracks both participants.
	const FRaceTrackCenterPoint Park = Track->SampleAtDistance(100.0f);
	const FVector Right(-Park.Forward.Y, Park.Forward.X, 0.0f);
	const FVector Spot = Park.Position + Right * 300.0f + FVector(0.0f, 0.0f, 60.0f);
	const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Park.Forward.Y, Park.Forward.X));
	Player->SetActorLocationAndRotation(Spot, FRotator(0.0f, Yaw, 0.0f),
		false, nullptr, ETeleportType::TeleportPhysics);
	Player->ResetMotion();
	bPlayerParked = true;
	UE_LOG(LogTemp, Display, TEXT("RACEAI9E2E: player parked at %s"), *Spot.ToString());
}

void ATask9Probe::Tick(float Delta)
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
				UE_LOG(LogTemp, Display, TEXT("RACEAI9E2E: player acquired"));
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
		AIIndex = 1;
		if (AI)
		{
			TArray<UActorComponent*> Comps;
			AI->GetComponents(URaceAIDriver::StaticClass(), Comps);
			if (Comps.Num() > 0)
			{
				AIDriver = Cast<URaceAIDriver>(Comps[0]);
			}
			UE_LOG(LogTemp, Display, TEXT("RACEAI9E2E: AI acquired driver=%d"), AIDriver != nullptr);
		}
	}

	const bool bRacing = static_cast<int32>(Manager->GetPhase()) == 2;
	if (!bStartSent && Elapsed >= 1.0)
	{
		bStartSent = true;
		Manager->StartRace();
		UE_LOG(LogTemp, Display, TEXT("RACEAI9E2E: start sent"));
	}
	if (bRacing && !bRacingSeen)
	{
		bRacingSeen = true;
		RacingStartTime = Elapsed;
	}
	if (bRacing && !bPlayerParked)
	{
		ParkPlayer();
	}

	if (AIDriver && bRacingSeen && !bProgressLogged
		&& AIDriver->GetProgressDistance() > Task9Limits::ProgressMinCm)
	{
		bProgressLogged = true;
		ProgressTime = Elapsed;
		UE_LOG(LogTemp, Display, TEXT("RACEAI9E2E: progress %.0f at t=%.1f"),
			AIDriver->GetProgressDistance(), Elapsed - RacingStartTime);
	}
	// Forced off-track event once the AI is rolling. Fixed void point
	// south of the circuit (no road within 2000 cm): the folded loop
	// makes relative offsets land on other sections.
	if (AIDriver && bRacingSeen && !bForced && AIDriver->GetProgressDistance() > 3000.0f)
	{
		bForced = true;
		ForceTime = Elapsed;
		RecoveriesBefore = AIDriver->GetRecoveryCount();
		const FVector Off(0.0f, -3000.0f, 200.0f);
		AI->SetActorLocationAndRotation(Off, AI->GetActorRotation(), false, nullptr, ETeleportType::TeleportPhysics);
		UE_LOG(LogTemp, Display, TEXT("RACEAI9E2E: forced off-track at %s"), *Off.ToString());
	}
	if (bForced && !bRecovered && AIDriver)
	{
		const int32 RecNow = AIDriver->GetRecoveryCount();
		const int32 NearIdx = NearestIndex(AI->GetActorLocation());
		const float Lat = FVector::Dist2D(AI->GetActorLocation(), Track->GetCenterPoints()[NearIdx].Position);
		const float Half = Track->TrackConfig.TrackWidth * 0.5f;
		if (RecNow > RecoveriesBefore && Lat < Half && FMath::Abs(AI->GetForwardSpeed()) > 100.0f)
		{
			bRecovered = true;
			RecoverTime = Elapsed;
			UE_LOG(LogTemp, Display, TEXT("RACEAI9E2E: recovered lat=%.0f v=%.0f"), Lat, AI->GetForwardSpeed());
			// Stage a clean lap from the grid: full driven lap, known state.
			const FRaceTrackCenterPoint Grid = Track->SampleAtDistance(300.0f);
			const float GridYaw = FMath::RadiansToDegrees(FMath::Atan2(Grid.Forward.Y, Grid.Forward.X));
			AI->SetActorLocationAndRotation(FVector(Grid.Position.X, Grid.Position.Y, 60.0f),
				FRotator(0.0f, GridYaw, 0.0f), false, nullptr, ETeleportType::TeleportPhysics);
			AI->ResetMotion();
			Manager->ReanchorParticipant(AI);
			Manager->OnVehicleReset();
			Manager->StartRace();
			// The reset re-snaps the player to the start line, directly
			// in the AI's path: park it clear again.
			ParkPlayer();
			UE_LOG(LogTemp, Display, TEXT("RACEAI9E2E: race restarted for clean lap"));
		}
		else if (Elapsed - ForceTime > Task9Limits::RecoveryWindow)
		{
			Finish(false, TEXT("recovery timeout"));
			return;
		}
	}

	// Completion: AI lap plus recovery observed.
	if (AIIndex >= 0 && Manager->GetParticipantLaps(AIIndex) >= 1 && (!bForced || bRecovered))
	{
		// Let the lap timing settle one second after the last event.
		if (DoneAt < 0.0)
		{
			DoneAt = Elapsed;
		}
		if (Elapsed - DoneAt > 1.0)
		{
			Finish(true, TEXT("ai lap and recovery complete"));
			return;
		}
	}

	if (Elapsed > Task9Limits::ProgramTimeout)
	{
		Finish(false, TEXT("program timeout"));
	}
}

void ATask9Probe::Finish(bool bOk, const FString& Note)
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
	UE_LOG(LogTemp, Display, TEXT("RACEAI9E2E: finishing (%s)"), *Note);
	UKismetSystemLibrary::QuitGame(GetWorld(), nullptr, EQuitPreference::Quit, false);
}

void ATask9Probe::WriteResults(bool bOk, const FString& Note) const
{
	const bool bSpawned = (AI != nullptr) && (AIDriver != nullptr) && AIDriver->IsDriving();
	const float AIProgress = AIDriver ? AIDriver->GetProgressDistance() : 0.0f;
	const bool bProgress = bProgressLogged
		&& (ProgressTime - RacingStartTime) < Task9Limits::ProgressWindow;
	const int32 AILaps = (AIIndex >= 0) ? Manager->GetParticipantLaps(AIIndex) : 0;
	const bool bLap = (AILaps >= 1);
	const bool bValid = bLap && Manager->WasParticipantLastValid(AIIndex);
	const bool bRecovery = bForced && bRecovered;
	const float AILast = (AIIndex >= 0) ? Manager->GetParticipantLastLap(AIIndex) : 0.0f;
	const bool bTiming = bLap && AILast > 0.0f;
	const int32 AIRecoveries = AIDriver ? AIDriver->GetRecoveryCount() : 0;
	const bool bAll = bOk && bSpawned && bProgress && bLap && bValid && bRecovery && bTiming;

	const FString Json = FString::Printf(
		TEXT("{\"ai_spawned\":%s,\"ai_progress\":%s,\"ai_lap\":%s,\"ai_valid\":%s,\"ai_recovery\":%s,\"ai_timing\":%s,")
		TEXT("\"ai_progress_cm\":%.1f,\"ai_laps\":%d,\"ai_recoveries\":%d,\"ai_last_lap\":%.2f,")
		TEXT("\"frames\":%d,\"note\":\"%s\"}"),
		bSpawned ? TEXT("true") : TEXT("false"), bProgress ? TEXT("true") : TEXT("false"),
		bLap ? TEXT("true") : TEXT("false"), bValid ? TEXT("true") : TEXT("false"),
		bRecovery ? TEXT("true") : TEXT("false"), bTiming ? TEXT("true") : TEXT("false"),
		AIProgress, AILaps, AIRecoveries, AILast, Frames, *Note);

	const FString Dir = FPaths::ProjectSavedDir() + TEXT("Task9E2E/");
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	PF.CreateDirectoryTree(*Dir);
	FFileHelper::SaveStringToFile(Json, *(Dir + TEXT("results.json")));
	UE_LOG(LogTemp, Display, TEXT("RACEAI9E2E: spawned=%d progress=%d lap=%d valid=%d recovery=%d timing=%d all=%d"),
		bSpawned, bProgress, bLap, bValid, bRecovery, bTiming, bAll);
}
