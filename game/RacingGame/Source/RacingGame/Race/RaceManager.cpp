// See header.

#include "RaceManager.h"
#include "RaceTrack.h"
#include "RaceVehicle.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	// Lateral bound beyond the checkpoint half width, cm.
	constexpr float LatMargin = 400.0f;
	// A plane re-arms once the vehicle is this far behind it, cm.
	constexpr float RearmBackoff = 100.0f;
}

ARaceManager::ARaceManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ARaceManager::BeginPlay()
{
	Super::BeginPlay();
	for (TActorIterator<ARaceTrack> It(GetWorld()); It; ++It)
	{
		Track = *It;
	}
	if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
	{
		Vehicle = Cast<ARaceVehicle>(Pawn);
	}
	if (Track)
	{
		PlaneArmed.Init(1, Track->GetCheckpoints().Num());
	}
	UE_LOG(LogTemp, Display, TEXT("RACE8: manager ready cps=%d laps=%d countdown=%.1f"),
		Track ? Track->GetCheckpoints().Num() : -1,
		RaceConfig.LapCount, RaceConfig.CountdownDuration);
}

void ARaceManager::StartRace()
{
	if (Phase == ERacePhase::Ready)
	{
		Phase = ERacePhase::Countdown;
		PhaseTime = 0.0f;
		UE_LOG(LogTemp, Display, TEXT("RACE8: countdown started"));
	}
}

void ARaceManager::OnVehicleReset()
{
	Phase = ERacePhase::Ready;
	PhaseTime = 0.0f;
	CompletedLaps = 0;
	ExpectIdx = 0;
	bLapValid = true;
	bLastSequenceValid = true;
	LastLapTime = 0.0f;
	BestLapTime = 0.0f;
	CrossingLog.Reset();
	IgnoredLog.Reset();
	bPrevInit = false;
	if (Track)
	{
		PlaneArmed.Init(1, Track->GetCheckpoints().Num());
		if (Vehicle)
		{
			Vehicle->SetActorLocationAndRotation(Track->GetStartPosition(),
				FRotator(0.0f, Track->GetStartYawDeg(), 0.0f), false, nullptr, ETeleportType::TeleportPhysics);
		}
	}
	UE_LOG(LogTemp, Display, TEXT("RACE8: reset to ready at track start"));
}

void ARaceManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!Track || !Vehicle)
	{
		return;
	}
	RaceClock += DeltaTime;
	if (Phase == ERacePhase::Countdown)
	{
		PhaseTime += DeltaTime;
		if (PhaseTime >= RaceConfig.CountdownDuration)
		{
			Phase = ERacePhase::Racing;
			LapStartTime = RaceClock;
			UE_LOG(LogTemp, Display, TEXT("RACE8: racing"));
		}
	}
	else if (Phase == ERacePhase::Racing)
	{
		AdvanceRacing(DeltaTime);
	}
}

void ARaceManager::PlaneMetrics(int32 CpIndex, const FVector& Pos, float& D, float& Lat) const
{
	const TArray<FRaceTrackCheckpoint>& CPs = Track->GetCheckpoints();
	const FRaceTrackCheckpoint& CP = CPs[CpIndex];
	const FVector Rel = Pos - CP.Position;
	D = FVector::DotProduct(Rel, CP.Forward);
	Lat = FMath::Abs(FVector::DotProduct(Rel, FVector(-CP.Forward.Y, CP.Forward.X, 0.0f)));
}

void ARaceManager::AdvanceRacing(float DeltaTime)
{
	(void)DeltaTime;
	const TArray<FRaceTrackCheckpoint>& CPs = Track->GetCheckpoints();
	const FVector Pos = Vehicle->GetActorLocation();
	// Previous-tick distances per plane; init on first racing tick.
	if (!bPrevInit || PrevPlaneD.Num() != CPs.Num())
	{
		PrevPlaneD.Init(0.0f, CPs.Num());
		for (int32 i = 0; i < CPs.Num(); ++i)
		{
			float D = 0.0f;
			float Lat = 0.0f;
			PlaneMetrics(i, Pos, D, Lat);
			PrevPlaneD[i] = D;
		}
		bPrevInit = true;
		return;
	}
	for (int32 i = 0; i < CPs.Num(); ++i)
	{
		float D = 0.0f;
		float Lat = 0.0f;
		PlaneMetrics(i, Pos, D, Lat);
		const float Bound = CPs[i].Width * 0.5f + LatMargin;
		if (D < -RearmBackoff)
		{
			PlaneArmed[i] = 1;
		}
		const bool bCross = (PrevPlaneD[i] < 0.0f && D >= 0.0f && Lat < Bound && PlaneArmed[i] != 0);
		PrevPlaneD[i] = D;
		if (!bCross)
		{
			continue;
		}
		PlaneArmed[i] = 0;
		if (i == ExpectIdx)
		{
			CrossingLog.Add(i);
			UE_LOG(LogTemp, Display, TEXT("RACE8: checkpoint %d ok (expect %d)"), i, ExpectIdx);
			ExpectIdx++;
			if (ExpectIdx >= CPs.Num())
			{
				// Sequence complete.
				bLastSequenceValid = bLapValid;
				if (bLapValid)
				{
					CompletedLaps++;
					LastLapTime = RaceClock - LapStartTime;
					if (BestLapTime <= 0.0f || LastLapTime < BestLapTime)
					{
						BestLapTime = LastLapTime;
					}
					UE_LOG(LogTemp, Display, TEXT("RACE8: lap %d valid t=%.2f best=%.2f"),
						CompletedLaps, LastLapTime, BestLapTime);
					if (CompletedLaps >= RaceConfig.LapCount)
					{
						Phase = ERacePhase::Finished;
						UE_LOG(LogTemp, Display, TEXT("RACE8: finished"));
					}
				}
				else
				{
					UE_LOG(LogTemp, Display, TEXT("RACE8: sequence done, lap invalid, not counted"));
				}
				ExpectIdx = 0;
				bLapValid = true;
				LapStartTime = RaceClock;
			}
		}
		else
		{
			IgnoredLog.Add(i);
			bLapValid = false;
			UE_LOG(LogTemp, Display, TEXT("RACE8: checkpoint %d ignored (expect %d), lap invalid"), i, ExpectIdx);
		}
	}
}
