// See header.

#include "RaceCockpitTestGameMode.h"
#include "Task18Probe.h"
#include "RaceTrack.h"
#include "RaceManager.h"
#include "RaceVehicle.h"
#include "RaceAIDriver.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ARaceCockpitTestGameMode::ARaceCockpitTestGameMode()
{
	DefaultPawnClass = ARaceVehicle::StaticClass();
}

void ARaceCockpitTestGameMode::BeginPlay()
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
	if (Manager)
	{
		Manager->RaceConfig.LapCount = Task18Limits::RaceLaps;
		UE_LOG(LogTemp, Display, TEXT("RACECOCKPIT18E2E: lap count %d for this run only"), Manager->RaceConfig.LapCount);
	}

	// Player on grid slot 0. The probe drives it with pursuit (Task 15
	// pattern); no AI driver of its own, so participant 0 stays on a
	// proven line and finishes the 2-car race.
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

	// A single AI rival on the second grid slot, Task 14 default line.
	if (Track && Manager)
	{
		FVector Loc;
		float Yaw = 0.0f;
		Track->GetGridPose(1, Loc, Yaw);
		Loc.Z = 40.0f;
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ARaceVehicle* AI = GetWorld()->SpawnActor<ARaceVehicle>(ARaceVehicle::StaticClass(), Loc,
			FRotator(0.0f, Yaw, 0.0f), P))
		{
			URaceAIDriver* Driver = NewObject<URaceAIDriver>(AI, TEXT("AIDriver"));
			Driver->RegisterComponent();
			Manager->RegisterParticipant(AI);
			UE_LOG(LogTemp, Display, TEXT("RACECOCKPIT18E2E: AI rival spawned"));
		}
	}

	FTimerHandle H;
	GetWorldTimerManager().SetTimer(H, [this]()
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		GetWorld()->SpawnActor<ATask18Probe>(ATask18Probe::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, P);
		UE_LOG(LogTemp, Display, TEXT("RACECOCKPIT18E2E: probe spawned"));
	}, 1.0f, false);
}