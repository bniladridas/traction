// See header.

#include "Task19RacecraftGameMode.h"
#include "Task19Probe.h"
#include "RaceTrack.h"
#include "RaceManager.h"
#include "RaceVehicle.h"
#include "RaceAIDriver.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

namespace
{
	// Task 19 staging-only correction (no threshold, gate, seam, or
	// contract change).
	//
	// Frozen geometry from ARaceTrack::GetGridPose places slot 1 at
	// S = StartLineDistance - SpawnBackoff - 100 and slot 2 at
	// S = StartLineDistance - SpawnBackoff - 200. With the shipped
	// config that is S = 300 cm and S = 200 cm: the pair starts only
	// 100 cm apart in absolute track distance, which is far inside the
	// frozen 600 cm AttackWindowCm, so the outside-window condition of
	// t19_no_commit_outside_window can never be reached. This pushes the
	// defender forward along the centerline so the absolute separation
	// exceeds the frozen attack window. Paces, lines, and every frozen
	// threshold are untouched.
	constexpr float DefenderAdvanceCm = 900.0f;
}

ATask19RacecraftGameMode::ATask19RacecraftGameMode()
{
	DefaultPawnClass = ARaceVehicle::StaticClass();
}

void ATask19RacecraftGameMode::BeginPlay()
{
	Super::BeginPlay();

	ARaceTrack* Track = nullptr;
	for (TActorIterator<ARaceTrack> It(GetWorld()); It; ++It)
	{
		Track = *It;
	}
	if (!Track)
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Track = GetWorld()->SpawnActor<ARaceTrack>(ARaceTrack::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, P);
	}

	ARaceManager* Manager = nullptr;
	for (TActorIterator<ARaceManager> It(GetWorld()); It; ++It)
	{
		Manager = *It;
	}
	if (!Manager)
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Manager = GetWorld()->SpawnActor<ARaceManager>(ARaceManager::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, P);
	}

	if (Track)
	{
		if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
		{
			FVector Loc;
			float Yaw = 0.0f;
			Track->GetGridPose(0, Loc, Yaw);
			Loc.Z = 40.0f;
			Pawn->SetActorLocationAndRotation(Loc, FRotator(0.0f, Yaw, 0.0f),
				false, nullptr, ETeleportType::TeleportPhysics);
		}
	}

	// Two AI rivals on the frozen tiers and lines: slot 1 is the defender
	// (slower, left line -120) and slot 2 the attacker (faster, right line
	// +120). Only the attacker carries the racecraft layer; the defender
	// stays a plain pursuit driver. Line assignment happens before the
	// driver ticks, so ResetRacecraft freezes the staged line.
	if (Track && Manager)
	{
		const float Tiers[3] = { 1.0f, Task19Limits::PaceDefender, Task19Limits::PaceAttacker };
		const float Lines[3] = { 0.0f, Task19Limits::LineDefender, Task19Limits::LineAttacker };
		for (int32 Slot = 1; Slot <= 2; ++Slot)
		{
			FVector Loc;
			float Yaw = 0.0f;
			if (Slot == 1)
			{
				// Defender starts DefenderAdvanceCm further along the
				// centerline than its frozen grid slot.
				const float BaseS = Track->TrackConfig.StartLineDistance
					- Track->TrackConfig.SpawnBackoff - 100.0f;
				const FRaceTrackCenterPoint P = Track->SampleAtDistance(BaseS + DefenderAdvanceCm);
				Loc = P.Position;
				Yaw = FMath::RadiansToDegrees(FMath::Atan2(P.Forward.Y, P.Forward.X));
			}
			else
			{
				Track->GetGridPose(Slot, Loc, Yaw);
			}
			Loc.Z = 40.0f;
			FActorSpawnParameters P;
			P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (ARaceVehicle* AI = GetWorld()->SpawnActor<ARaceVehicle>(ARaceVehicle::StaticClass(), Loc,
				FRotator(0.0f, Yaw, 0.0f), P))
			{
				URaceAIDriver* Driver = NewObject<URaceAIDriver>(AI, FName(*FString::Printf(TEXT("AIDriver%d"), Slot)));
				Driver->PaceFactor = Tiers[Slot];
				Driver->LineOffset = Lines[Slot];
				if (Slot == 2)
				{
					// Attacker: the Task 19 line-commit layer, with the
					// frozen window constants from the contract.
					Driver->bRacecraftEnabled = true;
					Driver->RacecraftAttackWindowCm = Task19Limits::AttackWindowCm;
					Driver->RacecraftCommitShiftCm = Task19Limits::CommitShiftCm;
					Driver->RacecraftFreeSideSign = Task19Limits::FreeSideSign;
				}
				Driver->RegisterComponent();
				Manager->RegisterParticipant(AI);
				const float SpawnS = (Slot == 1)
					? Track->TrackConfig.StartLineDistance - Track->TrackConfig.SpawnBackoff - 100.0f + DefenderAdvanceCm
					: Track->TrackConfig.StartLineDistance - Track->TrackConfig.SpawnBackoff - 200.0f;
				UE_LOG(LogTemp, Display, TEXT("RACECRAFT19E2E: AI slot %d pace %.2f line %.0f racecraft %d s=%.0f"),
					Slot, Tiers[Slot], Lines[Slot], Slot == 2 ? 1 : 0, SpawnS);
			}
		}
	}

	FTimerHandle H;
	GetWorldTimerManager().SetTimer(H, [this]()
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		GetWorld()->SpawnActor<ATask19Probe>(ATask19Probe::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, P);
		UE_LOG(LogTemp, Display, TEXT("RACECRAFT19E2E: probe spawned"));
	}, 1.0f, false);
}
