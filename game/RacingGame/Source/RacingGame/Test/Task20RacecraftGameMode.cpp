// See header.

#include "Task20RacecraftGameMode.h"
#include "Task20Probe.h"
#include "Task19Probe.h"
#include "RaceTrack.h"
#include "RaceManager.h"
#include "RaceVehicle.h"
#include "RaceAIDriver.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ATask20RacecraftGameMode::ATask20RacecraftGameMode()
{
	DefaultPawnClass = ARaceVehicle::StaticClass();
}

namespace
{
	// Task 20 staging: the defender starts this far ahead of its frozen
	// grid slot along the centerline so the run begins outside the frozen
	// rear window. Same load-bearing geometry as the Task 19 contract.
	constexpr float DefenderAdvanceCm = 900.0f;
}

void ATask20RacecraftGameMode::BeginPlay()
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

	// Slot 1 is the defender (slower, left line -120, Task 20 defense
	// layer with the frozen rear-window constants) and slot 2 the attacker
	// (faster, right line +120, unchanged Task 19 attack layer). Line and
	// pace assignment happens before the drivers tick, so each layer
	// freezes its own staged line. Neither layer reads or writes the
	// other's commanded state.
	if (Track && Manager)
	{
		const float Tiers[3] = { 1.0f, Task20Limits::PaceDefender, Task20Limits::PaceAttacker };
		const float Lines[3] = { 0.0f, Task20Limits::LineDefender, Task20Limits::LineAttacker };
		for (int32 Slot = 1; Slot <= 2; ++Slot)
		{
			FVector Loc;
			float Yaw = 0.0f;
			if (Slot == 1)
			{
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
				if (Slot == 1)
				{
					// Defender: the Task 20 defend-the-line layer, with the
					// frozen rear-window constants from the contract. The
					// Task 19 attack layer stays off on this car.
					Driver->bDefenseEnabled = true;
					Driver->DefenseRearWindowCm = Task20Limits::RearWindowCm;
					Driver->DefenseShiftCm = Task20Limits::DefensiveShiftCm;
					Driver->DefenseDirectionSign = Task20Limits::DefenseDirectionSign;
				}
				else
				{
					// Attacker: the unchanged Task 19 line-commit layer,
					// staged from the frozen Task 19 constants so this
					// program can never drift from Task 19 behavior.
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
				UE_LOG(LogTemp, Display, TEXT("TASK20E2E: AI slot %d pace %.2f line %.0f defense %d attack %d s=%.0f"),
					Slot, Tiers[Slot], Lines[Slot], Slot == 1 ? 1 : 0, Slot == 2 ? 1 : 0, SpawnS);
			}
		}
	}

	FTimerHandle H;
	GetWorldTimerManager().SetTimer(H, [this]()
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		GetWorld()->SpawnActor<ATask20Probe>(ATask20Probe::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, P);
		UE_LOG(LogTemp, Display, TEXT("TASK20E2E: probe spawned"));
	}, 1.0f, false);
}
