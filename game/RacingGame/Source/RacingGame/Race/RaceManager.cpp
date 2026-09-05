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
		if (ARaceVehicle* RV = Cast<ARaceVehicle>(Pawn))
		{
			RegisterParticipant(RV);
		}
	}
	UE_LOG(LogTemp, Display, TEXT("RACE8: manager ready cps=%d laps=%d countdown=%.1f racers=%d"),
		Track ? Track->GetCheckpoints().Num() : -1,
		RaceConfig.LapCount, RaceConfig.CountdownDuration, Participants.Num());
}

int32 ARaceManager::FindParticipant(const ARaceVehicle* Participant) const
{
	for (int32 i = 0; i < Participants.Num(); ++i)
	{
		if (Participants[i].Vehicle.Get() == Participant)
		{
			return i;
		}
	}
	return -1;
}

int32 ARaceManager::RegisterParticipant(ARaceVehicle* Participant)
{
	if (!Participant)
	{
		return -1;
	}
	const int32 Existing = FindParticipant(Participant);
	if (Existing >= 0)
	{
		return Existing;
	}
	FRaceParticipant P;
	P.Vehicle = Participant;
	if (Track)
	{
		P.PlaneArmed.Init(1, Track->GetCheckpoints().Num());
	}
	Participants.Add(P);
	UE_LOG(LogTemp, Display, TEXT("RACE8: registered participant %d"), Participants.Num() - 1);
	return Participants.Num() - 1;
}

void ARaceManager::ReanchorParticipant(ARaceVehicle* Participant)
{
	const int32 Index = FindParticipant(Participant);
	if (Index < 0 || !Track)
	{
		return;
	}
	FRaceParticipant& P = Participants[Index];
	P.PlaneArmed.Init(1, Track->GetCheckpoints().Num());
	P.bPrevInit = false;
}

int32 ARaceManager::GetCompletedLaps() const
{
	return Participants.IsValidIndex(0) ? Participants[0].CompletedLaps : 0;
}

int32 ARaceManager::GetNextCheckpoint() const
{
	return Participants.IsValidIndex(0) ? Participants[0].ExpectIdx : 0;
}

bool ARaceManager::IsCurrentLapValid() const
{
	return Participants.IsValidIndex(0) ? Participants[0].bLapValid : true;
}

bool ARaceManager::WasLastSequenceValid() const
{
	return Participants.IsValidIndex(0) ? Participants[0].bLastSequenceValid : true;
}

float ARaceManager::GetLastLapTime() const
{
	return Participants.IsValidIndex(0) ? Participants[0].LastLapTime : 0.0f;
}

float ARaceManager::GetBestLapTime() const
{
	return Participants.IsValidIndex(0) ? Participants[0].BestLapTime : 0.0f;
}

int32 ARaceManager::GetCrossingCount() const
{
	return Participants.IsValidIndex(0) ? Participants[0].CrossingLog.Num() : 0;
}

int32 ARaceManager::GetIgnoredCount() const
{
	return Participants.IsValidIndex(0) ? Participants[0].IgnoredLog.Num() : 0;
}

ARaceVehicle* ARaceManager::GetParticipantVehicle(int32 Index) const
{
	return Participants.IsValidIndex(Index) ? Participants[Index].Vehicle.Get() : nullptr;
}

int32 ARaceManager::GetParticipantLaps(int32 Index) const
{
	return Participants.IsValidIndex(Index) ? Participants[Index].CompletedLaps : 0;
}

bool ARaceManager::IsParticipantLapValid(int32 Index) const
{
	return Participants.IsValidIndex(Index) ? Participants[Index].bLapValid : true;
}

bool ARaceManager::WasParticipantLastValid(int32 Index) const
{
	return Participants.IsValidIndex(Index) ? Participants[Index].bLastSequenceValid : true;
}

bool ARaceManager::IsParticipantFinished(int32 Index) const
{
	return Participants.IsValidIndex(Index) ? Participants[Index].bFinished : false;
}

float ARaceManager::GetParticipantLastLap(int32 Index) const
{
	return Participants.IsValidIndex(Index) ? Participants[Index].LastLapTime : 0.0f;
}

float ARaceManager::GetParticipantBestLap(int32 Index) const
{
	return Participants.IsValidIndex(Index) ? Participants[Index].BestLapTime : 0.0f;
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
	FinishCounter = 0;
	Results.bFinalized = false;
	Results.Ordered.Reset();
	for (FRaceParticipant& P : Participants)
	{
		P.ExpectIdx = 0;
		P.bLapValid = true;
		P.bLastSequenceValid = true;
		P.bFinished = false;
		P.FinishSeq = -1;
		P.CompletedLaps = 0;
		P.LapStartTime = 0.0f;
		P.LastLapTime = 0.0f;
		P.BestLapTime = 0.0f;
		P.bPrevInit = false;
		P.CrossingLog.Reset();
		P.IgnoredLog.Reset();
		if (Track)
		{
			P.PlaneArmed.Init(1, Track->GetCheckpoints().Num());
		}
	}
	if (Track && Participants.IsValidIndex(0) && Participants[0].Vehicle.IsValid())
	{
		Participants[0].Vehicle->SetActorLocationAndRotation(Track->GetStartPosition(),
			FRotator(0.0f, Track->GetStartYawDeg(), 0.0f), false, nullptr, ETeleportType::TeleportPhysics);
	}
	UE_LOG(LogTemp, Display, TEXT("RACE8: reset to ready at track start"));
}

void ARaceManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!Track)
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
			for (FRaceParticipant& P : Participants)
			{
				P.LapStartTime = RaceClock;
			}
			UE_LOG(LogTemp, Display, TEXT("RACE8: racing"));
		}
	}
	else if (Phase == ERacePhase::Racing)
	{
		for (FRaceParticipant& P : Participants)
		{
			if (P.Vehicle.IsValid() && !P.bFinished)
			{
				AdvanceParticipant(P, DeltaTime);
			}
		}
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

void ARaceManager::AdvanceParticipant(FRaceParticipant& P, float DeltaTime)
{
	(void)DeltaTime;
	const TArray<FRaceTrackCheckpoint>& CPs = Track->GetCheckpoints();
	const FVector Pos = P.Vehicle->GetActorLocation();
	if (P.PlaneArmed.Num() != CPs.Num())
	{
		P.PlaneArmed.Init(1, CPs.Num());
	}
	// Previous-tick distances per plane; init on first racing tick.
	if (!P.bPrevInit || P.PrevPlaneD.Num() != CPs.Num())
	{
		P.PrevPlaneD.Init(0.0f, CPs.Num());
		for (int32 i = 0; i < CPs.Num(); ++i)
		{
			float D = 0.0f;
			float Lat = 0.0f;
			PlaneMetrics(i, Pos, D, Lat);
			P.PrevPlaneD[i] = D;
		}
		P.bPrevInit = true;
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
			P.PlaneArmed[i] = 1;
		}
		const bool bCross = (P.PrevPlaneD[i] < 0.0f && D >= 0.0f && Lat < Bound && P.PlaneArmed[i] != 0);
		P.PrevPlaneD[i] = D;
		if (!bCross)
		{
			continue;
		}
		P.PlaneArmed[i] = 0;
		if (i == P.ExpectIdx)
		{
			P.CrossingLog.Add(i);
			UE_LOG(LogTemp, Display, TEXT("RACE8: checkpoint %d ok (expect %d)"), i, P.ExpectIdx);
			P.ExpectIdx++;
			if (P.ExpectIdx >= CPs.Num())
			{
				// Sequence complete.
				P.bLastSequenceValid = P.bLapValid;
				if (P.bLapValid)
				{
					P.CompletedLaps++;
					P.LastLapTime = RaceClock - P.LapStartTime;
					if (P.BestLapTime <= 0.0f || P.LastLapTime < P.BestLapTime)
					{
						P.BestLapTime = P.LastLapTime;
					}
					UE_LOG(LogTemp, Display, TEXT("RACE8: lap %d valid t=%.2f best=%.2f"),
						P.CompletedLaps, P.LastLapTime, P.BestLapTime);
					if (P.CompletedLaps >= RaceConfig.LapCount)
					{
						P.bFinished = true;
						P.FinishSeq = FinishCounter++;
						UE_LOG(LogTemp, Display, TEXT("RACE8: participant finished (%d laps, seq %d)"), P.CompletedLaps, P.FinishSeq);
						bool bAllDone = true;
						for (const FRaceParticipant& Q : Participants)
						{
							bAllDone = bAllDone && Q.bFinished;
						}
						if (bAllDone)
						{
							Phase = ERacePhase::Finished;
							Results.bFinalized = true;
							Results.Ordered.Reset();
							TArray<int32> Order;
							for (int32 PIdx = 0; PIdx < Participants.Num(); ++PIdx)
							{
								Order.Add(PIdx);
							}
							Order.Sort([this](int32 A, int32 B)
							{
								return Participants[A].FinishSeq < Participants[B].FinishSeq;
							});
							for (int32 Idx : Order)
							{
								FRaceResultEntry E;
								E.ParticipantIndex = Idx;
								E.CompletedLaps = Participants[Idx].CompletedLaps;
								E.BestLapTime = Participants[Idx].BestLapTime;
								E.LastLapTime = Participants[Idx].LastLapTime;
								Results.Ordered.Add(E);
							}
							UE_LOG(LogTemp, Display, TEXT("RACE8: finished, results finalized (%d)"), Results.Ordered.Num());
						}
					}
				}
				else
				{
					UE_LOG(LogTemp, Display, TEXT("RACE8: sequence done, lap invalid, not counted"));
				}
				P.ExpectIdx = 0;
				P.bLapValid = true;
				P.LapStartTime = RaceClock;
			}
		}
		else
		{
			P.IgnoredLog.Add(i);
			P.bLapValid = false;
			UE_LOG(LogTemp, Display, TEXT("RACE8: checkpoint %d ignored (expect %d), lap invalid"), i, P.ExpectIdx);
		}
	}
}

int32 ARaceManager::GetPosition(const ARaceVehicle* Participant) const
{
	const int32 Self = FindParticipant(Participant);
	if (Self < 0 || !Track)
	{
		return -1;
	}
	const float L = Track->GetTrackLength();
	const TArray<FRaceTrackCenterPoint>& Pts = Track->GetCenterPoints();
	// Along-track distance per participant: nearest centerline point by
	// 2D distance (full deterministic scan, same method as the test
	// drivers; documented in task-11-e2e.md). No new track data.
	TArray<float> Prog;
	Prog.SetNum(Participants.Num());
	for (int32 i = 0; i < Participants.Num(); ++i)
	{
		float Best = FLT_MAX;
		float BestS = 0.0f;
		if (Participants[i].Vehicle.IsValid() && Pts.Num() > 0)
		{
			const FVector Pos = Participants[i].Vehicle->GetActorLocation();
			for (int32 k = 0; k < Pts.Num(); ++k)
			{
				const float D = FVector::DistSquared2D(Pos, Pts[k].Position);
				if (D < Best)
				{
					Best = D;
					BestS = Pts[k].Distance;
				}
			}
		}
		Prog[i] = Participants[i].CompletedLaps * L + BestS;
	}
	// Total order: finished by finish sequence, then score, then index.
	TArray<int32> Order;
	for (int32 i = 0; i < Participants.Num(); ++i)
	{
		Order.Add(i);
	}
	Order.Sort([this, &Prog](int32 A, int32 B)
	{
		const FRaceParticipant& PA = Participants[A];
		const FRaceParticipant& PB = Participants[B];
		if (PA.bFinished != PB.bFinished)
		{
			return PA.bFinished > PB.bFinished;
		}
		if (PA.bFinished && PB.bFinished)
		{
			return PA.FinishSeq < PB.FinishSeq;
		}
		if (Prog[A] != Prog[B])
		{
			return Prog[A] > Prog[B];
		}
		return A < B;
	});
	for (int32 Rank = 0; Rank < Order.Num(); ++Rank)
	{
		if (Order[Rank] == Self)
		{
			return Rank + 1;
		}
	}
	return -1;
}
